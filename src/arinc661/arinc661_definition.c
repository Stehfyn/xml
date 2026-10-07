/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6, Cockpit Display System Interfaces to User Systems:
 * https://saemobilus.sae.org/standards/arinc661-6-cockpit-display-system-interfaces-user-systems
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd):
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_scan.h"
#include "faceoe_utl/faceoe_utl_xml.h"

#include "os/os_assert.h"

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define XML_INTEGER_TEXT_BYTES (32UL)    ///< Integer formatting buffer bytes, including the zero terminator

/* Supplement 6 a661Strict.xsd: dfType, layerType, widgetType, modelType, and propType. */
#define DF_ELEMENT_ROOT     ("a661_df")
#define DF_ELEMENT_LAYER    ("a661_layer")
#define DF_ELEMENT_WIDGET   ("a661_widget")
#define DF_ELEMENT_MODEL    ("model")
#define DF_ELEMENT_PROPERTY ("prop")

/* ARINC 661 Part 1-7, section 6.2.4: array properties and their entry forms. */
#define DF_ELEMENT_ARRAY_PROPERTY ("arrayprop")
#define DF_ELEMENT_ENTRY          ("entry")
#define DF_ELEMENT_XY_ENTRY       ("xyentry")
#define DF_ELEMENT_STRUCT_ENTRY   ("structentry")
#define DF_ELEMENT_FIELD          ("field")

/* ARINC 661 Part 1-7, sections 6.2.1 and 8: charset encoding and widget extension elements. */
#define DF_ELEMENT_CHARSET_ENCODING ("charset-encoding")
#define DF_ELEMENT_EXTENSION        ("a661_extension")

/* Supplement 6 a661Strict.xsd attribute names. */
#define XML_ATTRIBUTE_NAME               ("name")
#define XML_ATTRIBUTE_X                  ("x")
#define XML_ATTRIBUTE_Y                  ("y")
#define XML_ATTRIBUTE_TYPE               ("type")
#define XML_ATTRIBUTE_VALUE              ("value")
#define XML_ATTRIBUTE_LIBRARY_VERSION    ("library_version")
#define XML_ATTRIBUTE_SUPPLEMENT_VERSION ("supp_version")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Temporary XML data owned by the caller's opaque workspace.
 */
typedef struct XmlWorkspace_s
{
    FACEOE_Utl_Xml_Node_t*      nodes;          ///< Pointer to temporary XML node storage
    FACEOE_Utl_Xml_Attribute_t* attributes;     ///< Pointer to temporary XML attribute storage
    FACEOE_Utl_Xml_Node_t**     widgetNodes;    ///< Pointer to the mapping from widget indices to XML nodes
    byte_t*              text;           ///< Pointer to formatted attribute value storage
    uint64_t             textCapacity;   ///< Available text bytes after the fixed workspace arrays
    uint64_t             textByteCount;  ///< Text bytes used, including each string's zero terminator
    uint32_t             nodeCount;      ///< XML node slots used while constructing output
    uint32_t             attributeCount; ///< XML attribute slots used while constructing output
    FACEOE_Utl_Xml_Document_t   document;       ///< Temporary document with nodes in the workspace
} XmlWorkspace_t;

/*************************************************************************************************/
/**
 * \brief Charset encoding name and its A661_Charset_Encoding value.
 */
typedef struct CharsetEncoding_s
{
    const char_t* name;  ///< Name attribute of the charset-encoding element
    uint32_t      value; ///< Value in Part 1-7, Table 4.6-13
} CharsetEncoding_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Charset encodings of Part 1-7, Table 4.6-13.
 */
static const CharsetEncoding_t c_charsetEncodings[] =
{
    {"A661_ASCII_EXTENDED", 1U},
    {"A661_UTF8",           2U},
    {"A661_GBK",            3U}
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Divides caller-owned temporary bytes into XML reader and writer storage.
 *
 * \param[out] xml         Partitioned temporary storage
 * \param[in]  bytes       Eight-byte-aligned workspace bytes
 * \param[in]  byteCount   Workspace size
 *
 * \retval     TRUE        if the workspace has enough bytes.
 * \retval     FALSE       if the workspace is too small or misaligned.
 */
static bool_t InitializeXmlWorkspace(XmlWorkspace_t* xml, void* bytes, uint64_t byteCount);

/*************************************************************************************************/
/**
 * \brief Compares a counted element name with a literal.
 *
 * \param[in] node   XML node
 * \param[in] name   Expected element name
 *
 * \retval    TRUE        if the node is an element with the expected name.
 * \retval    FALSE       if the type or name differs.
 */
static bool_t HasElementName(const FACEOE_Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds a direct child element by name.
 *
 * \param[in] parent   Parent element
 * \param[in] name     Expected child name
 *
 * \retval    non-NULL   Matching child.
 * \retval    NULL       if the child is absent.
 */
static FACEOE_Utl_Xml_Node_t* FindChild(FACEOE_Utl_Xml_Node_t* parent, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds an attribute by name.
 *
 * \param[in] node   Element to search
 * \param[in] name   Expected attribute name
 *
 * \retval    non-NULL   Matching attribute.
 * \retval    NULL       if the attribute is absent.
 */
static const FACEOE_Utl_Xml_Attribute_t* FindAttribute(const FACEOE_Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Copies a counted XML attribute into terminated caller storage.
 *
 * \param[in]  node          Element to search
 * \param[in]  name          Attribute name
 * \param[out] destination   Output character storage
 * \param[in]  capacity      Number of destination bytes
 *
 * \retval     TRUE        if the attribute is absent or fits the destination.
 * \retval     FALSE       if the attribute exceeds the destination.
 */
static bool_t CopyAttribute(const FACEOE_Utl_Xml_Node_t* node, const char_t* name, char_t* destination, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Reads one signed integer attribute.
 *
 * \param[in]  node    Element containing the attribute
 * \param[in]  name    Attribute name
 * \param[out] value   Parsed integer
 *
 * \retval     TRUE        if the complete attribute is a signed 64-bit integer.
 * \retval     FALSE       if the attribute is absent, malformed, or out of range.
 */
static bool_t ReadInteger(const FACEOE_Utl_Xml_Node_t* node, const char_t* name, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Appends an initialized XML element to temporary node storage.
 *
 * \param[in,out] xml    Temporary XML storage
 * \param[in]     name   Element name
 *
 * \retval    non-NULL   Unlinked element in temporary storage.
 * \retval    NULL       if storage is exhausted.
 */
static FACEOE_Utl_Xml_Node_t* AppendElement(XmlWorkspace_t* xml, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Appends one valid attribute to a temporary XML element.
 *
 * \param[in,out] xml     Temporary XML storage
 * \param[in,out] node    Element receiving the attribute
 * \param[in]     name    Attribute name
 * \param[in]     value   Attribute value
 *
 * \retval        TRUE        if the attribute was appended.
 * \retval        FALSE       if storage is exhausted or the value cannot be written directly.
 */
static bool_t AppendAttribute(XmlWorkspace_t* xml, FACEOE_Utl_Xml_Node_t* node, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Formats a signed integer into temporary value storage.
 *
 * \param[in,out] xml     Temporary XML storage
 * \param[in]     value   Integer to format
 *
 * \retval    non-NULL   Stable temporary text.
 * \retval    NULL       if storage is exhausted.
 */
static const char_t* FormatInteger(XmlWorkspace_t* xml, int64_t value);

/*************************************************************************************************/
/**
 * \brief Appends one signed integer attribute to an XML element.
 *
 * \param[in,out] xml     Temporary XML storage
 * \param[in,out] node    Element receiving the attribute
 * \param[in]     name    Attribute name
 * \param[in]     value   Integer value
 *
 * \retval        TRUE        if the attribute was appended.
 * \retval        FALSE       if temporary storage was exhausted.
 */
static bool_t AppendIntegerAttribute(XmlWorkspace_t* xml, FACEOE_Utl_Xml_Node_t* node, const char_t* name, int64_t value);

/*************************************************************************************************/
/**
 * \brief Finds the document element after an optional XML declaration.
 *
 * \param[in] document   XML document
 *
 * \retval    non-NULL   First top-level element.
 * \retval    NULL       if no top-level element is present.
 */
static FACEOE_Utl_Xml_Node_t* FindDocumentElement(const FACEOE_Utl_Xml_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Advances within one XML subtree without recursion.
 *
 * \param[in] root      Root of the traversal
 * \param[in] current   Node just visited
 *
 * \retval    non-NULL   Next node in document order.
 * \retval    NULL       after the subtree.
 */
static FACEOE_Utl_Xml_Node_t* NextSubtreeNode(FACEOE_Utl_Xml_Node_t* root, FACEOE_Utl_Xml_Node_t* current);

/*************************************************************************************************/
/**
 * \brief Checks whether an element may appear inside a model.
 *
 * \param[in] node   XML node to check
 *
 * \retval    TRUE   if the element is a model, prop, arrayprop, or an array entry form.
 * \retval    FALSE  otherwise.
 */
static bool_t IsModelContent(const FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Appends one property whose name and value come from two XML elements.
 *
 * \param[in,out] definition       Caller-owned Definition File data
 * \param[in]     owner            Kind of entity containing the model
 * \param[in]     ownerIndex       Layer or widget index, or zero for the Definition File
 * \param[in]     nameNode         Element carrying the name attribute
 * \param[in]     valueNode        Element carrying the value
 * \param[in]     valueAttribute   Name of the value attribute, such as value or x
 *
 * \retval        TRUE        if the property fit the supplied storage.
 * \retval        FALSE       if property storage or string capacity was exceeded.
 */
static bool_t AppendProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const FACEOE_Utl_Xml_Node_t* nameNode, const FACEOE_Utl_Xml_Node_t* valueNode, const char_t* valueAttribute);

/*************************************************************************************************/
/**
 * \brief Reads the entries of one arrayprop element as properties sharing its name.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     array        XML arrayprop element
 * \param[in]     owner        Kind of entity containing the model
 * \param[in]     ownerIndex   Layer or widget index, or zero for the Definition File
 *
 * \retval        TRUE        if all entries fit the supplied storage.
 * \retval        FALSE       if an entry form is unsupported or storage was exceeded.
 */
static bool_t ReadArrayProperty(Arinc661_Definition_t* definition, FACEOE_Utl_Xml_Node_t* array, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*************************************************************************************************/
/**
 * \brief Reads direct prop and arrayprop children of one Definition File model.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd, modelType/propType, and Part 1-7 section 6.2.4 arrayprop.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     model        XML model element
 * \param[in]     owner        Kind of entity containing the model
 * \param[in]     ownerIndex   Layer or widget index, or zero for the Definition File
 *
 * \retval        TRUE        if all properties fit the supplied storage.
 * \retval        FALSE       if property storage or string capacity was exceeded.
 */
static bool_t ReadProperties(Arinc661_Definition_t* definition, FACEOE_Utl_Xml_Node_t* model, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*************************************************************************************************/
/**
 * \brief Finds the next property with the same owner and name as a given property.
 *
 * \param[in] definition      Definition File values
 * \param[in] propertyIndex   Property whose owner and name are matched
 * \param[in] startIndex      First property index to examine
 *
 * \retval    uint32_t        Index of the match, or UINT32_MAX when there is none.
 */
static uint32_t FindSameNameProperty(const Arinc661_Definition_t* definition, uint32_t propertyIndex, uint32_t startIndex);

/*************************************************************************************************/
/**
 * \brief Writes properties for one Definition File model.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd, modelType/propType, and Part 1-7 section 6.2.4 arrayprop.
 *
 * \param[in,out] xml          Temporary XML tree and value storage
 * \param[in,out] parent       Element receiving its model child
 * \param[in]     definition   Definition File values
 * \param[in]     owner        Kind of property owner
 * \param[in]     ownerIndex   Layer or widget index, or zero for the Definition File
 *
 * \retval        TRUE        if the model and its properties were appended.
 * \retval        FALSE       if storage or a property value is invalid.
 */
static bool_t WriteProperties(XmlWorkspace_t* xml, FACEOE_Utl_Xml_Node_t* parent, const Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Divides an aligned caller-provided buffer into fixed arrays and a remaining
 *  text area. The layout is:
 *
 * \verbatim
 * bytes -> [ XML nodes ][ XML attributes ][ widget node pointers ][ text bytes ]
 *          xml.nodes    xml.attributes    xml.widgetNodes         xml.text
 * \endverbatim
 *
 *  The text area stores zero-terminated numeric attribute values for writers.
 *  At least one text byte must remain after the fixed arrays. Misalignment or
 *  insufficient space returns FALSE without initializing the workspace.
 *  Successful initialization clears the counts and document links; array contents
 *  are initialized as entries are used.
 */
static bool_t InitializeXmlWorkspace(XmlWorkspace_t* xml, void* bytes, uint64_t byteCount)
{
    uint64_t node_bytes;
    uint64_t attribute_bytes;
    uint64_t widget_bytes;
    uint64_t reserved_bytes;
    bool_t   is_aligned;
    bool_t   has_capacity;

    // Reserve the fixed arrays before assigning the remaining bytes to text
    node_bytes      = (uint64_t)ARINC661_XML_NODE_CAPACITY * sizeof(FACEOE_Utl_Xml_Node_t);
    attribute_bytes = (uint64_t)ARINC661_XML_ATTRIBUTE_CAPACITY * sizeof(FACEOE_Utl_Xml_Attribute_t);
    widget_bytes    = (uint64_t)ARINC661_XML_WIDGET_CAPACITY * sizeof(FACEOE_Utl_Xml_Node_t*);
    reserved_bytes  = ((node_bytes + attribute_bytes) + widget_bytes);
    is_aligned      = (0 == ((uintptr_t)bytes % sizeof(uint64_t)));
    has_capacity    = (reserved_bytes < byteCount);
    if (is_aligned && has_capacity)
    {
        // Each region points into the supplied workspace; no separate allocation is made
        xml->nodes          = (FACEOE_Utl_Xml_Node_t*)bytes;
        xml->attributes     = (FACEOE_Utl_Xml_Attribute_t*)((byte_t*)bytes + node_bytes);
        xml->widgetNodes    = (FACEOE_Utl_Xml_Node_t**)((byte_t*)bytes + (node_bytes + attribute_bytes));
        xml->text           = (byte_t*)bytes + reserved_bytes;
        xml->textCapacity   = byteCount - reserved_bytes;
        // Reset usage and document links without clearing the entire workspace
        xml->textByteCount  = 0;
        xml->nodeCount      = 0;
        xml->attributeCount = 0;
        FACEOE_Utl_Xml_InitializeDocument(&xml->document);
    }

    return is_aligned && has_capacity;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares an element's counted UTF-8 name with a zero-terminated literal.
 *  For example, a node parsed from the following tag matches "screen":
 *
 * \code{.xml}
 * <screen id="2"/>
 * \endcode
 *
 *  The node name has six bytes and does not need a zero terminator. NULL nodes,
 *  non-element nodes, different lengths, and different byte contents return FALSE.
 *  The comparison is case-sensitive.
 */
static bool_t HasElementName(const FACEOE_Utl_Xml_Node_t* node, const char_t* name)
{
    uint32_t byte_count;
    bool_t   is_element;
    bool_t   has_length;
    bool_t   has_name;

    byte_count = (uint32_t)strlen(name);
    // Check the pointer and node type before examining the counted name
    is_element = (NULL != node);
    if (is_element)
    {
        is_element = (FACEOE_UTL_XML_NODE_ELEMENT == node->type);
    }
    has_length = FALSE;
    has_name   = FALSE;
    if (is_element)
    {
        has_length = (byte_count == node->nameByteCount);
        if (has_length)
        {
            // Compare contents only when both names have the same byte count
            has_name = (0 == memcmp(node->name, name, byte_count));
        }
    }

    return has_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Searches only the parent's direct child list, in source order. For example,
 *  searching screen for "layout" returns its layout child in this fragment:
 *
 * \code{.xml}
 * <screen id="2">
 *     <layout id="3">
 *         <window id="7"/>
 *     </layout>
 * </screen>
 * \endcode
 *
 *  Searching the same screen for "window" returns NULL because descendants of
 *  layout are not visited. The first matching child is returned; a completed
 *  unsuccessful search must reach the end of the sibling list within the bound.
 */
static FACEOE_Utl_Xml_Node_t* FindChild(FACEOE_Utl_Xml_Node_t* parent, const char_t* name)
{
    FACEOE_Utl_Xml_Node_t* node;
    FACEOE_Utl_Xml_Node_t* found_node;
    uint32_t        node_index;

    node       = parent->firstChild;
    found_node = NULL;
    for (node_index = 0; node_index < ARINC661_XML_NODE_CAPACITY; ++node_index)
    {
        // Stop at the end of the direct child list
        if (NULL == node)
        {
            break;
        }
        if (HasElementName(node, name))
        {
            found_node = node;
            break;
        }
        // Continue with the next sibling without descending into children
        node = node->nextSibling;
    }
    if (NULL == found_node)
    {
        // An unsuccessful bounded search must have exhausted the sibling list
        LOGASSERT(NULL == node);
    }

    return found_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds the first attribute whose complete name matches the requested literal.
 *  In this tag, searching for "id" returns the attribute containing "7":
 *
 * \code{.xml}
 * <window id="7" name="Main"/>
 * \endcode
 *
 *  Matching is case-sensitive and checks the byte count before the contents.
 *  The element's attributeCount bounds the walk; a missing match must leave the
 *  cursor at NULL. No attribute or string bytes are copied.
 */
static const FACEOE_Utl_Xml_Attribute_t* FindAttribute(const FACEOE_Utl_Xml_Node_t* node, const char_t* name)
{
    const FACEOE_Utl_Xml_Attribute_t* attribute;
    const FACEOE_Utl_Xml_Attribute_t* found_attribute;
    uint32_t                   byte_count;
    uint32_t                   attribute_index;
    bool_t                     has_length;
    bool_t                     has_name;

    attribute       = node->firstAttribute;
    found_attribute = NULL;
    byte_count      = (uint32_t)strlen(name);
    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        // Every attribute included in the count must have a corresponding list entry
        LOGASSERT(NULL != attribute);
        has_length = (byte_count == attribute->nameByteCount);
        has_name   = FALSE;
        if (has_length)
        {
            // Equal lengths make a complete byte comparison safe
            has_name = (0 == memcmp(attribute->name, name, byte_count));
        }
        if (has_name)
        {
            found_attribute = attribute;
            break;
        }
        attribute = attribute->next;
    }
    if (NULL == found_attribute)
    {
        // Reject a list that continues beyond its declared attribute count
        LOGASSERT(NULL == attribute);
    }

    return found_attribute;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies an optional attribute into a caller-provided character array and adds
 *  a zero terminator. For example, copying "name" from this tag needs five bytes:
 *
 * \code{.xml}
 * <window name="Main"/>
 * \endcode
 *
 * \verbatim
 * Capacity   Destination bytes   Return
 * 5          M a i n \0          TRUE
 * 4          \0                  FALSE
 * \endverbatim
 *
 *  An absent attribute succeeds with an empty string. Capacity must include space
 *  for the terminator and be nonzero because destination[0] is always initialized.
 */
static bool_t CopyAttribute(const FACEOE_Utl_Xml_Node_t* node, const char_t* name, char_t* destination, uint32_t capacity)
{
    const FACEOE_Utl_Xml_Attribute_t* attribute;
    bool_t                     fits;

    attribute      = FindAttribute(node, name);
    // Missing or oversized attributes leave an empty destination
    destination[0] = '\0';
    fits           = TRUE;
    if (NULL != attribute)
    {
        // Reserve one byte beyond the copied value for its terminator
        fits = (attribute->valueByteCount < capacity);
        if (fits)
        {
            (void)memcpy(destination, attribute->value, attribute->valueByteCount);
            destination[attribute->valueByteCount] = '\0';
        }
    }

    return fits;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a required integer attribute only when the scanner consumes its entire
 *  counted value. For example:
 *
 * \code{.xml}
 * <pixelArea x="-20"/>
 * \endcode
 *
 *  Reading "x" stores -20 and returns TRUE. A missing attribute or a value such as
 *  "-20px" returns FALSE; accepting only the numeric prefix would lose input.
 *  The output value changes only after the complete scan succeeds.
 */
static bool_t ReadInteger(const FACEOE_Utl_Xml_Node_t* node, const char_t* name, int64_t* value)
{
    const FACEOE_Utl_Xml_Attribute_t* attribute;
    int64_t                    parsed_value;
    int32_t                    consumed;
    bool_t                     is_valid;

    attribute = FindAttribute(node, name);
    // The numeric attribute is required
    is_valid  = (NULL != attribute);
    if (is_valid)
    {
        consumed = FACEOE_Utl_Scan_Int64((const char_t*)attribute->value, &parsed_value);
        // Require a nonempty scan that consumes the complete attribute value
        is_valid = (0 < consumed) && ((uint32_t)consumed == attribute->valueByteCount);
        if (is_valid)
        {
            (*value) = parsed_value;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Takes one node slot from the workspace and initializes an unlinked element.
 *  For example, requesting "screen" creates the node for this empty element:
 *
 * \code{.xml}
 * <screen/>
 * \endcode
 *
 *  The caller appends attributes and links the node to its parent or document.
 *  The name pointer refers to the supplied literal; its bytes are not copied.
 *  Exhausted node storage returns NULL without increasing nodeCount.
 */
static FACEOE_Utl_Xml_Node_t* AppendElement(XmlWorkspace_t* xml, const char_t* name)
{
    FACEOE_Utl_Xml_Node_t* node;

    node = NULL;
    // Initialize a node only when a workspace slot is available
    if (xml->nodeCount < ARINC661_XML_NODE_CAPACITY)
    {
        node = &xml->nodes[xml->nodeCount];
        FACEOE_Utl_Xml_InitializeNode(node, FACEOE_UTL_XML_NODE_ELEMENT, (const byte_t*)name, (uint32_t)strlen(name));
        // Count the initialized node; linking it is the caller's responsibility
        xml->nodeCount += 1;
    }

    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Initializes and links one attribute using pointers to the supplied name and
 *  value. For example, appending name="Main" to a window produces:
 *
 * \code{.xml}
 * <window name="Main"/>
 * \endcode
 *
 *  A NULL value, exhausted attribute storage, or a value that cannot be written
 *  literally returns FALSE without appending an attribute. Name and value bytes
 *  are not copied and must remain valid until serialization finishes. The caller
 *  supplies a valid, unique name and an element with room for the attribute.
 */
static bool_t AppendAttribute(XmlWorkspace_t* xml, FACEOE_Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    FACEOE_Utl_Xml_Attribute_t* attribute;
    uint32_t             byte_count;
    bool_t               has_storage;
    bool_t               has_value;
    bool_t               is_valid;

    // A formatter may return NULL, and the shared attribute array may be full
    has_storage = (xml->attributeCount < ARINC661_XML_ATTRIBUTE_CAPACITY);
    has_value   = (NULL != value);
    is_valid    = FALSE;
    if (has_storage && has_value)
    {
        byte_count = (uint32_t)strlen(value);
        // Validate literal value bytes before the XML append function asserts its preconditions
        is_valid   = FACEOE_Utl_Xml_IsAttributeValue((const byte_t*)value, byte_count);
        if (is_valid)
        {
            attribute = &xml->attributes[xml->attributeCount];
            FACEOE_Utl_Xml_InitializeAttribute(attribute, (const byte_t*)name, (uint32_t)strlen(name), (const byte_t*)value, byte_count);
            FACEOE_Utl_Xml_AppendAttribute(node, attribute);
            // Count the attribute only after it has been linked
            xml->attributeCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Formats a signed integer into the workspace text area and returns a pointer
 *  to its zero-terminated representation:
 *
 * \verbatim
 * value:                 -42
 * new text bytes:        '-' '4' '2' '\0'
 * textByteCount increase: 4
 * \endverbatim
 *
 *  The remaining workspace must have room for XML_INTEGER_TEXT_BYTES.
 *  Only the bytes actually used, including the terminator, are retained in
 *  textByteCount. Insufficient space returns NULL without advancing that count.
 */
static const char_t* FormatInteger(XmlWorkspace_t* xml, int64_t value)
{
    char_t* text;
    int     byte_count;

    text = NULL;
    // Reserve enough temporary room before formatting a signed 64-bit value
    if (XML_INTEGER_TEXT_BYTES <= (xml->textCapacity - xml->textByteCount))
    {
        text       = (char_t*)&xml->text[xml->textByteCount];
        byte_count = snprintf(text, XML_INTEGER_TEXT_BYTES, "%" PRId64, value);
        LOGASSERT((0 < byte_count) && ((uint32_t)byte_count < XML_INTEGER_TEXT_BYTES));
        // Keep the terminator in workspace storage for subsequent strlen calls
        xml->textByteCount += ((uint64_t)byte_count + 1);
    }

    return text;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Formats an integer in workspace storage, then appends the resulting attribute.
 *  For example, name="id" and value=7 add id="7" to the element. The attribute
 *  points to the formatted text; it does not copy it again.
 *
 *  Formatting or appending failure returns FALSE. Text already reserved by a
 *  successful formatter is not reclaimed if the subsequent append fails.
 */
static bool_t AppendIntegerAttribute(XmlWorkspace_t* xml, FACEOE_Utl_Xml_Node_t* node, const char_t* name, int64_t value)
{
    const char_t* text;
    bool_t        is_appended;

    // Keep the formatted value in workspace storage until serialization finishes
    text        = FormatInteger(xml, value);
    is_appended = AppendAttribute(xml, node, name, text);

    return is_appended;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Searches only the document's top-level nodes for its element, skipping an
 *  optional declaration. For example:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * <configuration/>
 * \endcode
 *
 *  The configuration node is returned. Descendants are not visited, and a
 *  document containing no element returns NULL. The root-node limit bounds this
 *  search independently of storage reserved for descendants.
 */
static FACEOE_Utl_Xml_Node_t* FindDocumentElement(const FACEOE_Utl_Xml_Document_t* document)
{
    FACEOE_Utl_Xml_Node_t* node;
    FACEOE_Utl_Xml_Node_t* element;
    uint32_t        node_index;

    node    = document->firstNode;
    element = NULL;
    for (node_index = 0; node_index < FACEOE_UTL_XML_DOCUMENT_MAX_ROOT_NODES; ++node_index)
    {
        // Stop at the end of the top-level list
        if (NULL == node)
        {
            break;
        }
        // The declaration is skipped; the document element is the result
        if (FACEOE_UTL_XML_NODE_ELEMENT == node->type)
        {
            element = node;
            break;
        }
        node = node->nextSibling;
    }
    if (NULL == element)
    {
        // An unsuccessful search must have reached the end within the root-node limit
        LOGASSERT(NULL == node);
    }

    return element;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the next node in a depth-first walk confined to root. For example:
 *
 * \verbatim
 * layer (root)
 * +-- model
 * |   +-- prop
 * +-- widget
 *     +-- model
 *
 * Visit order: layer -> model -> prop -> widget -> model -> NULL
 * \endverbatim
 *
 *  Children are visited before siblings. When neither is available, parent links
 *  are followed until an ancestor has a next sibling or root is reached. The
 *  root's own sibling is never returned. The caller supplies a current node
 *  inside this subtree; ancestor traversal is bounded by FACEOE_UTL_XML_NODE_MAX_DEPTH.
 */
static FACEOE_Utl_Xml_Node_t* NextSubtreeNode(FACEOE_Utl_Xml_Node_t* root, FACEOE_Utl_Xml_Node_t* current)
{
    FACEOE_Utl_Xml_Node_t* next_node;
    FACEOE_Utl_Xml_Node_t* ancestor;
    uint32_t        ancestor_index;

    // Descend first so parents are visited before their descendants
    next_node = current->firstChild;
    ancestor  = current;
    if (NULL == next_node)
    {
        for (ancestor_index = 0; ancestor_index < FACEOE_UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
        {
            // Keep traversal inside the selected subtree
            if (root == ancestor)
            {
                break;
            }
            // After a branch ends, look for the next sibling at this level
            next_node = ancestor->nextSibling;
            if (NULL != next_node)
            {
                break;
            }
            // Climb until another branch is available or the subtree root is reached
            ancestor = ancestor->parent;
            LOGASSERT(NULL != ancestor);
        }
    }

    return next_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts model, prop, arrayprop, entry, xyentry, structentry, and field, which
 *  are the elements the subtree walk of a layer may meet inside a model.
 */
static bool_t IsModelContent(const FACEOE_Utl_Xml_Node_t* node)
{
    return HasElementName(node, DF_ELEMENT_MODEL) || HasElementName(node, DF_ELEMENT_PROPERTY) || HasElementName(node, DF_ELEMENT_ARRAY_PROPERTY) ||
           HasElementName(node, DF_ELEMENT_ENTRY) || HasElementName(node, DF_ELEMENT_XY_ENTRY) || HasElementName(node, DF_ELEMENT_STRUCT_ENTRY) ||
           HasElementName(node, DF_ELEMENT_FIELD);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reserves the next result slot, records the owner, and copies the name
 *  attribute of nameNode and the valueAttribute of valueNode. For a plain prop
 *  both nodes are the prop itself; for an array entry the name comes from the
 *  arrayprop and the value from the entry. Missing attributes become empty
 *  strings through CopyAttribute. The count is published only after both
 *  strings fit.
 */
static bool_t AppendProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const FACEOE_Utl_Xml_Node_t* nameNode, const FACEOE_Utl_Xml_Node_t* valueNode, const char_t* valueAttribute)
{
    Arinc661_Property_t* property;
    bool_t                   is_valid;

    is_valid = (definition->propertyCount < definition->propertyCapacity);
    if (is_valid)
    {
        property = &definition->properties[definition->propertyCount];
        (void)memset(property, 0, sizeof((*property)));
        // Record the owning model kind and its array index with each flat property
        property->owner      = owner;
        property->ownerIndex = ownerIndex;
        is_valid             = CopyAttribute(nameNode, XML_ATTRIBUTE_NAME, property->name, sizeof(property->name));
    }
    if (is_valid)
    {
        is_valid = CopyAttribute(valueNode, valueAttribute, property->value, sizeof(property->value));
    }
    if (is_valid)
    {
        definition->propertyCount += 1;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Flattens one array into properties that share the array's name, one per
 *  value, in document order. The three entry forms of section 6.2.4 are:
 *
 * \code{.xml}
 * <arrayprop name="EntryList">
 *     <entry value="ON"/>
 *     <entry value="OFF"/>
 * </arrayprop>
 * <arrayprop name="Vertices">
 *     <xyentry x="-500" y="-500"/>
 * </arrayprop>
 * <arrayprop name="BufferStructure">
 *     <structentry>
 *         <field name="WidgetIdent" value="2"/>
 *         <field name="ParameterIdent" value="A661_ENABLE"/>
 *     </structentry>
 * </arrayprop>
 * \endcode
 *
 *  An xyentry yields x then y; a structentry yields each field value in order,
 *  and field names are not retained. The result is the same flat form the
 *  binary reader produces. Any other child returns FALSE.
 */
static bool_t ReadArrayProperty(Arinc661_Definition_t* definition, FACEOE_Utl_Xml_Node_t* array, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    FACEOE_Utl_Xml_Node_t* entry_node;
    FACEOE_Utl_Xml_Node_t* field_node;
    uint32_t        entry_index;
    uint32_t        field_index;
    bool_t          is_valid;

    is_valid   = TRUE;
    entry_node = array->firstChild;
    for (entry_index = 0; entry_index < ARINC661_XML_NODE_CAPACITY; ++entry_index)
    {
        if (NULL == entry_node)
        {
            break;
        }
        if (HasElementName(entry_node, DF_ELEMENT_ENTRY))
        {
            is_valid = AppendProperty(definition, owner, ownerIndex, array, entry_node, XML_ATTRIBUTE_VALUE);
        }
        else if (HasElementName(entry_node, DF_ELEMENT_XY_ENTRY))
        {
            is_valid = AppendProperty(definition, owner, ownerIndex, array, entry_node, XML_ATTRIBUTE_X);
            if (is_valid)
            {
                is_valid = AppendProperty(definition, owner, ownerIndex, array, entry_node, XML_ATTRIBUTE_Y);
            }
        }
        else if (HasElementName(entry_node, DF_ELEMENT_STRUCT_ENTRY))
        {
            field_node = entry_node->firstChild;
            for (field_index = 0; field_index < ARINC661_XML_NODE_CAPACITY; ++field_index)
            {
                if (NULL == field_node)
                {
                    break;
                }
                is_valid = HasElementName(field_node, DF_ELEMENT_FIELD);
                if (is_valid)
                {
                    is_valid = AppendProperty(definition, owner, ownerIndex, array, field_node, XML_ATTRIBUTE_VALUE);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                field_node = field_node->nextSibling;
            }
        }
        else
        {
            is_valid = FALSE;
        }
        if (FALSE == is_valid)
        {
            break;
        }
        entry_node = entry_node->nextSibling;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies the direct prop and arrayprop children of a required model into the
 *  shared property array, for example:
 *
 * \code{.xml}
 * <model>
 *     <prop name="WidgetIdent" value="10"/>
 *     <prop name="Visible" value="A661_TRUE"/>
 * </model>
 * \endcode
 *
 *  With owner=ARINC661_PROPERTY_WIDGET and ownerIndex=2, both copied entries
 *  refer to widgets[2]. An arrayprop adds one property per entry value, all
 *  named after the array. An empty model succeeds without adding entries.
 *  A missing model, unexpected child, or exhausted result storage returns FALSE.
 *  Properties already counted remain until the public reader clears the counts.
 */
static bool_t ReadProperties(Arinc661_Definition_t* definition, FACEOE_Utl_Xml_Node_t* model, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    FACEOE_Utl_Xml_Node_t* property_node;
    uint32_t        node_index;
    bool_t          is_valid;

    // Every supported owner needs a model, even when it has no properties
    is_valid = (NULL != model);
    if (is_valid)
    {
        property_node = model->firstChild;
        for (node_index = 0; node_index < ARINC661_XML_NODE_CAPACITY; ++node_index)
        {
            if (NULL == property_node)
            {
                break;
            }
            if (HasElementName(property_node, DF_ELEMENT_PROPERTY))
            {
                is_valid = AppendProperty(definition, owner, ownerIndex, property_node, property_node, XML_ATTRIBUTE_VALUE);
            }
            else if (HasElementName(property_node, DF_ELEMENT_ARRAY_PROPERTY))
            {
                is_valid = ReadArrayProperty(definition, property_node, owner, ownerIndex);
            }
            else
            {
                is_valid = FALSE;
            }
            if (FALSE == is_valid)
            {
                break;
            }
            // Only direct children of this model belong to the requested owner
            property_node = property_node->nextSibling;
        }
        if (is_valid)
        {
            LOGASSERT(NULL == property_node);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares owner kind, owner index, and name, so the properties of one array
 *  can be enumerated by repeated calls with an increasing startIndex.
 */
static uint32_t FindSameNameProperty(const Arinc661_Definition_t* definition, uint32_t propertyIndex, uint32_t startIndex)
{
    const Arinc661_Property_t* property;
    const Arinc661_Property_t* candidate;
    uint32_t                       match_index;
    uint32_t                       candidate_index;

    property    = &definition->properties[propertyIndex];
    match_index = UINT32_MAX;
    for (candidate_index = startIndex; candidate_index < definition->propertyCount; ++candidate_index)
    {
        candidate = &definition->properties[candidate_index];
        if ((property->owner == candidate->owner) && (property->ownerIndex == candidate->ownerIndex) && (0 == strcmp(property->name, candidate->name)))
        {
            match_index = candidate_index;
            break;
        }
    }

    return match_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Selects properties whose owner kind and ownerIndex both match the requested
 *  model. For example, the properties for one layer can produce:
 *
 * \code{.xml}
 * <model>
 *     <prop name="LayerId" value="1"/>
 *     <prop name="Width" value="10000"/>
 * </model>
 * \endcode
 *
 *  Matching entries retain their order in the flat property array. Properties
 *  belonging to other definitions, layers, or widgets are skipped. Properties
 *  sharing a name are an array and are written once, at the position of the
 *  first, as an arrayprop with one entry per value:
 *
 * \code{.xml}
 * <arrayprop name="EntryList">
 *     <entry value="ON"/>
 *     <entry value="OFF"/>
 * </arrayprop>
 * \endcode
 *
 *  A single-element array is indistinguishable from a plain property and is
 *  written as a prop; the xyentry and structentry forms are never written.
 *  If none match, an empty model element is still appended. A partially built
 *  model is not linked to parent when an attribute or workspace allocation fails.
 */
static bool_t WriteProperties(XmlWorkspace_t* xml, FACEOE_Utl_Xml_Node_t* parent, const Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    FACEOE_Utl_Xml_Node_t*                model;
    FACEOE_Utl_Xml_Node_t*                property_node;
    FACEOE_Utl_Xml_Node_t*                entry_node;
    const Arinc661_Property_t* property;
    const char_t*                  element_name;
    uint32_t                       property_index;
    uint32_t                       entry_index;
    uint32_t                       element_index;
    bool_t                         is_owner;
    bool_t                         has_index;
    bool_t                         is_array;
    bool_t                         is_valid;

    // Always create the owner's model, including when no properties match
    model    = AppendElement(xml, DF_ELEMENT_MODEL);
    is_valid = (NULL != model);
    if (is_valid)
    {
        for (property_index = 0; property_index < definition->propertyCount; ++property_index)
        {
            property  = &definition->properties[property_index];
            // Both owner kind and array index must match this model
            is_owner  = (owner == property->owner);
            has_index = (ownerIndex == property->ownerIndex);
            // ponytail: accept the quadratic same-name scan; index properties by owner if models grow large
            if (is_owner && has_index && (FindSameNameProperty(definition, property_index, 0) == property_index))
            {
                is_array      = (UINT32_MAX != FindSameNameProperty(definition, property_index, property_index + 1));
                element_name  = DF_ELEMENT_PROPERTY;
                if (is_array)
                {
                    element_name = DF_ELEMENT_ARRAY_PROPERTY;
                }
                property_node = AppendElement(xml, element_name);
                is_valid      = (NULL != property_node);
                if (is_valid)
                {
                    is_valid = AppendAttribute(xml, property_node, XML_ATTRIBUTE_NAME, property->name);
                }
                if (is_valid && (FALSE == is_array))
                {
                    is_valid = AppendAttribute(xml, property_node, XML_ATTRIBUTE_VALUE, property->value);
                }
                if (is_valid && is_array)
                {
                    // Turn every later property with this name into an entry, in array order
                    entry_index = property_index;
                    for (element_index = 0; element_index < definition->propertyCount; ++element_index)
                    {
                        if (UINT32_MAX == entry_index)
                        {
                            break;
                        }
                        entry_node = AppendElement(xml, DF_ELEMENT_ENTRY);
                        is_valid   = (NULL != entry_node);
                        if (is_valid)
                        {
                            is_valid = AppendAttribute(xml, entry_node, XML_ATTRIBUTE_VALUE, definition->properties[entry_index].value);
                        }
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                        FACEOE_Utl_Xml_AppendChild(property_node, entry_node);
                        entry_index = FindSameNameProperty(definition, property_index, entry_index + 1);
                    }
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                // Preserve the order of matching properties
                FACEOE_Utl_Xml_AppendChild(model, property_node);
            }
        }
    }
    if (is_valid)
    {
        // Attach the model only after all selected properties have been built
        FACEOE_Utl_Xml_AppendChild(parent, model);
    }

    return is_valid;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a Definition File into flat layer, widget, and property arrays. This
 *  supported example contains a nested widget:
 *
 * \code{.xml}
 * <a661_df name="Demo" library_version="0" supp_version="6">
 *     <model>
 *         <prop name="ApplicationId" value="1"/>
 *     </model>
 *     <a661_layer name="Main">
 *         <model/>
 *         <a661_widget name="Panel" type="A661_PANEL">
 *             <model/>
 *             <a661_widget name="Status" type="A661_LABEL">
 *                 <model/>
 *             </a661_widget>
 *         </a661_widget>
 *     </a661_layer>
 * </a661_df>
 * \endcode
 *
 * \verbatim
 * layers[0].name                = Main
 * widgets[0].layerIndex         = 0    parentWidgetIndex = UINT32_MAX
 * widgets[1].layerIndex         = 0    parentWidgetIndex = 0
 * properties[0].owner           = ARINC661_PROPERTY_DEFINITION
 * properties[0].ownerIndex      = 0
 * \endverbatim
 *
 *  Widgets are visited before their descendants. Temporary XML node pointers
 *  map a nested widget's parent to an earlier widget array entry. Each supported
 *  owner needs a model, including an empty one. Strings are copied into the
 *  result, so no input or workspace pointers remain there.
 *
 *  Input may include an XML declaration; it is not retained in the result.
 *
 *  Versions must fit uint32_t. Failure clears layerCount, widgetCount, and
 *  propertyCount; scalar fields and array entries may still contain partial data.
 */
bool_t Arinc661_ReadDefinition(Arinc661_Definition_t* definition, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t         xml;
    FACEOE_Utl_Xml_Node_t*        root;
    FACEOE_Utl_Xml_Node_t*        layer_node;
    FACEOE_Utl_Xml_Node_t*        current_node;
    FACEOE_Utl_Xml_Node_t*        model_node;
    Arinc661_Layer_t*  layer;
    Arinc661_Widget_t* widget;
    char_t                 charset_name[ARINC661_MAX_NAME_BYTES];
    int64_t                version;
    uint32_t               layer_index;
    uint32_t               node_index;
    uint32_t               widget_index;
    uint32_t               parent_index;
    uint32_t               charset_index;
    bool_t                 has_workspace;
    bool_t                 is_extension;
    bool_t                 is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    LOGASSERT(NULL != workspace);
    // Reset result counts while retaining the supplied arrays and capacities; XML carries no binary blocks
    definition->layerCount      = 0;
    definition->widgetCount     = 0;
    definition->propertyCount   = 0;
    definition->blockByteCount  = 0;
    definition->charsetEncoding = 0;
    has_workspace               = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid                    = has_workspace;
    if (is_valid)
    {
        // Parse the source into the temporary XML workspace
        is_valid = (0 < FACEOE_Utl_Xml_ReadDocument(&xml.document, buffer, byteCount, xml.nodes, ARINC661_XML_NODE_CAPACITY, xml.attributes, ARINC661_XML_ATTRIBUTE_CAPACITY));
    }
    if (is_valid)
    {
        // Require a Definition File root before reading its attributes
        root     = FindDocumentElement(&xml.document);
        is_valid = HasElementName(root, DF_ELEMENT_ROOT);
    }
    if (is_valid)
    {
        is_valid = CopyAttribute(root, XML_ATTRIBUTE_NAME, definition->name, sizeof(definition->name));
    }
    if (is_valid)
    {
        // Read and range-check the version values before storing them as uint32_t
        is_valid = ReadInteger(root, XML_ATTRIBUTE_LIBRARY_VERSION, &version);
        if (is_valid)
        {
            is_valid = (0 <= version);
            if (is_valid)
            {
                is_valid = (UINT32_MAX >= version);
            }
            if (is_valid)
            {
                definition->libraryVersion = (uint32_t)version;
            }
        }
    }
    if (is_valid)
    {
        is_valid = ReadInteger(root, XML_ATTRIBUTE_SUPPLEMENT_VERSION, &version);
        if (is_valid)
        {
            is_valid = (0 <= version);
            if (is_valid)
            {
                is_valid = (UINT32_MAX >= version);
            }
            if (is_valid)
            {
                definition->supplementVersion = (uint32_t)version;
            }
        }
    }
    if (is_valid)
    {
        // Copy properties belonging to the Definition File itself
        model_node = FindChild(root, DF_ELEMENT_MODEL);
        is_valid   = ReadProperties(definition, model_node, ARINC661_PROPERTY_DEFINITION, 0);
    }
    if (is_valid)
    {
        // Process layers in source order and walk each layer's complete subtree
        layer_node = root->firstChild;
        for (layer_index = 0; layer_index < ARINC661_XML_NODE_CAPACITY; ++layer_index)
        {
            if (NULL == layer_node)
            {
                break;
            }
            if (HasElementName(layer_node, DF_ELEMENT_LAYER))
            {
                is_valid = (definition->layerCount < definition->layerCapacity);
                if (FALSE == is_valid)
                {
                    break;
                }
                layer = &definition->layers[definition->layerCount];
                (void)memset(layer, 0, sizeof((*layer)));
                is_valid = CopyAttribute(layer_node, XML_ATTRIBUTE_NAME, layer->name, sizeof(layer->name));
                if (is_valid)
                {
                    model_node = FindChild(layer_node, DF_ELEMENT_MODEL);
                    is_valid   = ReadProperties(definition, model_node, ARINC661_PROPERTY_LAYER, definition->layerCount);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                definition->layerCount += 1;
                current_node = layer_node->firstChild;
                for (node_index = 0; node_index < ARINC661_XML_NODE_CAPACITY; ++node_index)
                {
                    if (NULL == current_node)
                    {
                        break;
                    }
                    is_extension = HasElementName(current_node, DF_ELEMENT_EXTENSION);
                    if (is_extension || HasElementName(current_node, DF_ELEMENT_WIDGET))
                    {
                        is_valid = (definition->widgetCount < definition->widgetCapacity);
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                        widget = &definition->widgets[definition->widgetCount];
                        (void)memset(widget, 0, sizeof((*widget)));
                        // Default to a direct layer child until a widget parent is found
                        widget->layerIndex        = definition->layerCount - 1;
                        widget->parentWidgetIndex = UINT32_MAX;
                        widget->isExtension       = is_extension;
                        // Require a widget parent for an extension, never a layer or another extension
                        is_valid = (FALSE == is_extension) || (current_node->parent != layer_node);
                        if (is_valid && (current_node->parent != layer_node))
                        {
                            // Map the parent XML pointer to a widget already visited in this layer
                            for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
                            {
                                if (xml.widgetNodes[widget_index] == current_node->parent)
                                {
                                    widget->parentWidgetIndex = widget_index;
                                    break;
                                }
                            }
                            is_valid = (UINT32_MAX != widget->parentWidgetIndex);
                            if (is_valid)
                            {
                                is_valid = (FALSE == definition->widgets[widget->parentWidgetIndex].isExtension);
                            }
                        }
                        if (is_valid)
                        {
                            is_valid = CopyAttribute(current_node, XML_ATTRIBUTE_NAME, widget->name, sizeof(widget->name));
                        }
                        if (is_valid)
                        {
                            is_valid = CopyAttribute(current_node, XML_ATTRIBUTE_TYPE, widget->type, sizeof(widget->type));
                        }
                        if (is_valid)
                        {
                            model_node = FindChild(current_node, DF_ELEMENT_MODEL);
                            is_valid   = ReadProperties(definition, model_node, ARINC661_PROPERTY_WIDGET, definition->widgetCount);
                        }
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                        // Remember the XML node so later descendants can identify this widget as parent
                        xml.widgetNodes[definition->widgetCount] = current_node;
                        definition->widgetCount += 1;
                    }
                    else
                    {
                        is_valid = IsModelContent(current_node);
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                    }
                    // Advance depth-first without recursively consuming C stack space
                    current_node = NextSubtreeNode(layer_node, current_node);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                LOGASSERT(NULL == current_node);
            }
            else if (HasElementName(layer_node, DF_ELEMENT_CHARSET_ENCODING))
            {
                // Select the Table 4.6-13 encoding by name (section 6.2.1) and allow one per file
                is_valid = (0 == definition->charsetEncoding);
                if (is_valid)
                {
                    is_valid = CopyAttribute(layer_node, XML_ATTRIBUTE_NAME, charset_name, sizeof(charset_name));
                }
                if (is_valid)
                {
                    for (charset_index = 0; charset_index < COUNT_OF(c_charsetEncodings); ++charset_index)
                    {
                        if (0 == strcmp(charset_name, c_charsetEncodings[charset_index].name))
                        {
                            definition->charsetEncoding = c_charsetEncodings[charset_index].value;
                            break;
                        }
                    }
                    is_valid = (0 != definition->charsetEncoding);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
            }
            else
            {
                is_valid = HasElementName(layer_node, DF_ELEMENT_MODEL);
                if (FALSE == is_valid)
                {
                    break;
                }
            }
            layer_node = layer_node->nextSibling;
        }
        if (is_valid)
        {
            LOGASSERT(NULL == layer_node);
        }
    }
    if (FALSE == is_valid)
    {
        // Hide all partially read entries by clearing the published counts
        definition->layerCount    = 0;
        definition->widgetCount   = 0;
        definition->propertyCount = 0;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rebuilds a Definition File tree from layer, widget, and property arrays.
 *  For example, a layer with two sibling widgets can produce:
 *
 * \code{.xml}
 * <a661_df name="Controls" library_version="0" supp_version="6">
 *     <model/>
 *     <a661_layer name="Main">
 *         <model/>
 *         <a661_widget name="Start" type="A661_PUSH_BUTTON">
 *             <model>
 *                 <prop name="WidgetIdent" value="10"/>
 *             </model>
 *         </a661_widget>
 *         <a661_widget name="Stop" type="A661_PUSH_BUTTON">
 *             <model/>
 *         </a661_widget>
 *     </a661_layer>
 * </a661_df>
 * \endcode
 *
 *  Output starts directly with the a661_df element, without an XML declaration.
 *
 *  layerIndex selects the owning layer. UINT32_MAX in parentWidgetIndex places a
 *  widget directly under that layer; otherwise it names an earlier widget in the
 *  same layer. The caller supplies valid array counts and these ownership links.
 *  Each owner receives a model, even when no properties match.
 *
 *  XML nodes, attributes, and formatted numbers use the temporary workspace;
 *  text fields point to the supplied records until writing finishes. The result
 *  includes a final LF and no zero terminator. Zero indicates construction or
 *  serialization failure; serialization may already have written an output prefix.
 */
uint64_t Arinc661_WriteDefinition(const Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t               xml;
    FACEOE_Utl_Xml_Node_t*              root;
    FACEOE_Utl_Xml_Node_t*              layer_node;
    FACEOE_Utl_Xml_Node_t*              widget_node;
    FACEOE_Utl_Xml_Node_t*              parent_node;
    FACEOE_Utl_Xml_Node_t*              charset_node;
    const Arinc661_Layer_t*  layer;
    const Arinc661_Widget_t* widget;
    const char_t*                element_name;
    uint64_t                     byte_count;
    uint32_t                     layer_index;
    uint32_t                     widget_index;
    uint32_t                     charset_index;
    bool_t                       has_workspace;
    bool_t                       has_parent;
    bool_t                       is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    LOGASSERT(NULL != workspace);
    // Keep zero as the failure result until the complete document is written
    byte_count    = 0;
    has_workspace = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid      = has_workspace;
    if (is_valid)
    {
        // Start the Definition File with its document element
        root     = AppendElement(&xml, DF_ELEMENT_ROOT);
        is_valid = (NULL != root);
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(&xml, root, XML_ATTRIBUTE_NAME, definition->name);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(&xml, root, XML_ATTRIBUTE_LIBRARY_VERSION, definition->libraryVersion);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(&xml, root, XML_ATTRIBUTE_SUPPLEMENT_VERSION, definition->supplementVersion);
    }
    if (is_valid && (0 != definition->charsetEncoding))
    {
        // Place the charset-encoding element before the model and layers (section 6.2.1)
        charset_node = AppendElement(&xml, DF_ELEMENT_CHARSET_ENCODING);
        is_valid     = (NULL != charset_node);
        if (is_valid)
        {
            is_valid = FALSE;
            for (charset_index = 0; charset_index < COUNT_OF(c_charsetEncodings); ++charset_index)
            {
                if (definition->charsetEncoding == c_charsetEncodings[charset_index].value)
                {
                    is_valid = AppendAttribute(&xml, charset_node, XML_ATTRIBUTE_NAME, c_charsetEncodings[charset_index].name);
                    break;
                }
            }
        }
        if (is_valid)
        {
            FACEOE_Utl_Xml_AppendChild(root, charset_node);
        }
    }
    if (is_valid)
    {
        // The root model precedes every layer
        is_valid = WriteProperties(&xml, root, definition, ARINC661_PROPERTY_DEFINITION, 0);
    }
    if (is_valid)
    {
        // Build layers in array order, each with its own model and widgets
        for (layer_index = 0; layer_index < definition->layerCount; ++layer_index)
        {
            layer      = &definition->layers[layer_index];
            layer_node = AppendElement(&xml, DF_ELEMENT_LAYER);
            is_valid   = (NULL != layer_node);
            if (is_valid)
            {
                is_valid = AppendAttribute(&xml, layer_node, XML_ATTRIBUTE_NAME, layer->name);
            }
            if (is_valid)
            {
                is_valid = WriteProperties(&xml, layer_node, definition, ARINC661_PROPERTY_LAYER, layer_index);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
            {
                widget = &definition->widgets[widget_index];
                // Select widgets belonging to this layer while preserving their array order
                if (layer_index == widget->layerIndex)
                {
                    element_name = DF_ELEMENT_WIDGET;
                    if (widget->isExtension)
                    {
                        element_name = DF_ELEMENT_EXTENSION;
                    }
                    widget_node = AppendElement(&xml, element_name);
                    is_valid    = (NULL != widget_node);
                    if (is_valid && widget->isExtension)
                    {
                        // Require a parent widget for an extension
                        is_valid = (UINT32_MAX != widget->parentWidgetIndex);
                    }
                    if (is_valid && (FALSE == widget->isExtension))
                    {
                        // Omit the name attribute from a661_extension, which carries only its type (Table 6.2.3-3)
                        is_valid = AppendAttribute(&xml, widget_node, XML_ATTRIBUTE_NAME, widget->name);
                    }
                    if (is_valid)
                    {
                        is_valid = AppendAttribute(&xml, widget_node, XML_ATTRIBUTE_TYPE, widget->type);
                    }
                    if (is_valid)
                    {
                        is_valid = WriteProperties(&xml, widget_node, definition, ARINC661_PROPERTY_WIDGET, widget_index);
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    // A sentinel parent index places the widget directly under its layer
                    parent_node = layer_node;
                    if (UINT32_MAX != widget->parentWidgetIndex)
                    {
                        // A nested parent must have been built before the current widget
                        has_parent = (widget->parentWidgetIndex < widget_index);
                        is_valid   = has_parent;
                        if (is_valid)
                        {
                            parent_node = xml.widgetNodes[widget->parentWidgetIndex];
                            is_valid    = (NULL != parent_node);
                        }
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    FACEOE_Utl_Xml_AppendChild(parent_node, widget_node);
                    // Record the built node for subsequent children to use as their parent
                    xml.widgetNodes[widget_index] = widget_node;
                }
            }
            if (FALSE == is_valid)
            {
                break;
            }
            FACEOE_Utl_Xml_AppendChild(root, layer_node);
        }
    }
    if (is_valid)
    {
        // Append the completed root and serialize the temporary document
        FACEOE_Utl_Xml_AppendNode(&xml.document, root);
        byte_count = FACEOE_Utl_Xml_WriteDocument(&xml.document, buffer, capacity);
    }

    return byte_count;
}
