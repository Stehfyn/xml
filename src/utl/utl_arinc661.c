/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6 Definition File schema:
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 * FACE Technical Standard, Appendix H.3, DisplayManagement.xsd:
 * https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "utl/utl_arinc661.h"
#include "utl/utl_scan.h"
#include "utl/utl_xml.h"

#include "os/os_assert.h"

#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_XML_NODE_CAPACITY      (65536)
#define ARINC661_XML_ATTRIBUTE_CAPACITY (65536)
#define ARINC661_XML_WIDGET_CAPACITY    (65536)
#define ARINC661_XML_DECIMAL_TEXT_BYTES (96)

/* FACE DisplayManagement.xsd element names. */
#define ARINC661_DMC_ELEMENT_CONFIGURATION       ("configuration")
#define ARINC661_DMC_ELEMENT_METADATA            ("metaData")
#define ARINC661_DMC_ELEMENT_SCREEN              ("screen")
#define ARINC661_DMC_ELEMENT_PIXEL_SIZE          ("pixelSize")
#define ARINC661_DMC_ELEMENT_PHYSICAL_DIMENSIONS ("physicalDimensions")
#define ARINC661_DMC_ELEMENT_LAYOUT              ("layout")
#define ARINC661_DMC_ELEMENT_WINDOW              ("window")
#define ARINC661_DMC_ELEMENT_DESCRIPTION         ("description")
#define ARINC661_DMC_ELEMENT_PIXEL_AREA          ("pixelArea")
#define ARINC661_DMC_ELEMENT_SCALING_FACTOR      ("scalingFactor")
#define ARINC661_DMC_ELEMENT_USER_APPLICATION    ("ua")
#define ARINC661_DMC_ELEMENT_EXTERNAL_SOURCE     ("externalSource")

/* ARINC 661 Supplement 6 Definition File element names. */
#define ARINC661_DF_ELEMENT_ROOT     ("a661_df")
#define ARINC661_DF_ELEMENT_LAYER    ("a661_layer")
#define ARINC661_DF_ELEMENT_WIDGET   ("a661_widget")
#define ARINC661_DF_ELEMENT_MODEL    ("model")
#define ARINC661_DF_ELEMENT_PROPERTY ("prop")

/* Display Management and Definition File attribute names. */
#define ARINC661_XML_ATTRIBUTE_ID                 ("id")
#define ARINC661_XML_ATTRIBUTE_NAME               ("name")
#define ARINC661_XML_ATTRIBUTE_WIDTH              ("width")
#define ARINC661_XML_ATTRIBUTE_HEIGHT             ("height")
#define ARINC661_XML_ATTRIBUTE_UNITS              ("units")
#define ARINC661_XML_ATTRIBUTE_X                  ("x")
#define ARINC661_XML_ATTRIBUTE_Y                  ("y")
#define ARINC661_XML_ATTRIBUTE_X_SCALE            ("xScale")
#define ARINC661_XML_ATTRIBUTE_Y_SCALE            ("yScale")
#define ARINC661_XML_ATTRIBUTE_BASE_UNIT          ("baseUnit")
#define ARINC661_XML_ATTRIBUTE_PER_UNIT           ("perUnit")
#define ARINC661_XML_ATTRIBUTE_APPLICATION_ID     ("applicationId")
#define ARINC661_XML_ATTRIBUTE_DF_PATH            ("dFPath")
#define ARINC661_XML_ATTRIBUTE_STYLE_FILE_PATH    ("styleFilePath")
#define ARINC661_XML_ATTRIBUTE_MAP_SOURCE_LAYER   ("mapSourceLayer")
#define ARINC661_XML_ATTRIBUTE_WINDOW             ("window")
#define ARINC661_XML_ATTRIBUTE_VISIBLE            ("visible")
#define ARINC661_XML_ATTRIBUTE_TYPE               ("type")
#define ARINC661_XML_ATTRIBUTE_VALUE              ("value")
#define ARINC661_XML_ATTRIBUTE_LIBRARY_VERSION    ("library_version")
#define ARINC661_XML_ATTRIBUTE_SUPPLEMENT_VERSION ("supp_version")

/* XML Schema boolean lexical values. */
#define ARINC661_XML_BOOLEAN_TRUE  ("true")
#define ARINC661_XML_BOOLEAN_FALSE ("false")
#define ARINC661_XML_BOOLEAN_ONE   ("1")
#define ARINC661_XML_BOOLEAN_ZERO  ("0")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Temporary XML data owned by the caller's opaque workspace.
 */
typedef struct XmlWorkspace_s
{
    Utl_Xml_Node_t*      nodes;
    Utl_Xml_Attribute_t* attributes;
    Utl_Xml_Node_t**     widgetNodes;
    byte_t*              text;
    uint64_t             textCapacity;
    uint64_t             textByteCount;
    uint32_t             nodeCount;
    uint32_t             attributeCount;
    Utl_Xml_Document_t   document;
} XmlWorkspace_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

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
static bool_t HasElementName(const Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds a direct child element by name.
 *
 * \param[in] parent   Parent element
 * \param[in] name     Expected child name
 *
 * \return Matching child, or NULL when absent.
 */
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds an attribute by name.
 *
 * \param[in] node   Element to search
 * \param[in] name   Expected attribute name
 *
 * \return Matching attribute, or NULL when absent.
 */
static const Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Compares a counted attribute value with a literal.
 *
 * \param[in] attribute   Attribute containing a counted value
 * \param[in] value       Expected value
 *
 * \retval    TRUE        if the value matches exactly.
 * \retval    FALSE       if the byte count or contents differ.
 */
static bool_t HasAttributeValue(const Utl_Xml_Attribute_t* attribute, const char_t* value);

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
static bool_t CopyAttribute(const Utl_Xml_Node_t* node, const char_t* name, char_t* destination, uint32_t capacity);

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
static bool_t ReadInteger(const Utl_Xml_Node_t* node, const char_t* name, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Reads one finite decimal attribute.
 *
 * \param[in]  node    Element containing the attribute
 * \param[in]  name    Attribute name
 * \param[out] value   Parsed decimal
 *
 * \retval     TRUE        if the complete attribute is a finite number.
 * \retval     FALSE       if the attribute is absent, malformed, or out of range.
 */
static bool_t ReadDecimal(const Utl_Xml_Node_t* node, const char_t* name, float32_t* value);

/*************************************************************************************************/
/**
 * \brief Reads one FACE Units attribute.
 *
 * \param[in]  node          Element containing the attribute
 * \param[in]  name          Attribute name
 * \param[in]  defaultUnit   Value used when the attribute is absent
 * \param[out] unit          Parsed or default unit
 *
 * \retval     TRUE        if the attribute is absent or names a FACE unit.
 * \retval     FALSE       if the attribute names an unsupported unit.
 */
static bool_t ReadUnit(const Utl_Xml_Node_t* node, const char_t* name, Utl_Arinc661_Unit_t defaultUnit, Utl_Arinc661_Unit_t* unit);

/*************************************************************************************************/
/**
 * \brief Reads an optional XML Schema boolean attribute.
 *
 * \param[in]  node    Element containing the attribute
 * \param[in]  name    Attribute name
 * \param[out] value   Parsed value, or FALSE when absent
 *
 * \retval     TRUE        if the attribute is absent or has XML Schema boolean syntax.
 * \retval     FALSE       if the attribute has another value.
 */
static bool_t ReadBoolean(const Utl_Xml_Node_t* node, const char_t* name, bool_t* value);

/*************************************************************************************************/
/**
 * \brief Appends an initialized XML element to temporary node storage.
 *
 * \param[in,out] xml    Temporary XML storage
 * \param[in]     name   Element name
 *
 * \return Unlinked element in temporary storage, or NULL when storage is exhausted.
 */
static Utl_Xml_Node_t* AppendElement(XmlWorkspace_t* xml, const char_t* name);

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
static bool_t AppendAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Formats a signed integer into temporary value storage.
 *
 * \param[in,out] xml     Temporary XML storage
 * \param[in]     value   Integer to format
 *
 * \return Stable temporary text, or NULL when storage is exhausted.
 */
static const char_t* FormatInteger(XmlWorkspace_t* xml, int64_t value);

/*************************************************************************************************/
/**
 * \brief Formats a finite decimal into temporary value storage.
 *
 * \param[in,out] xml     Temporary XML storage
 * \param[in]     value   Decimal to format
 *
 * \return Stable temporary text, or NULL when storage is exhausted.
 */
static const char_t* FormatDecimal(XmlWorkspace_t* xml, float32_t value);

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
static bool_t AppendIntegerAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, int64_t value);

/*************************************************************************************************/
/**
 * \brief Appends one decimal attribute to an XML element.
 *
 * \param[in,out] xml     Temporary XML storage
 * \param[in,out] node    Element receiving the attribute
 * \param[in]     name    Attribute name
 * \param[in]     value   Decimal value
 *
 * \retval        TRUE        if the attribute was appended.
 * \retval        FALSE       if temporary storage was exhausted or the value is not finite.
 */
static bool_t AppendDecimalAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, float32_t value);

/*************************************************************************************************/
/**
 * \brief Finds the document element after an optional XML declaration.
 *
 * \param[in] document   XML document
 *
 * \return First top-level element, or NULL when absent.
 */
static Utl_Xml_Node_t* FindDocumentElement(const Utl_Xml_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Reads a FACE Size element.
 *
 * \param[in]  node   Element with width, height, and optional units
 * \param[out] size   Parsed size
 *
 * \retval     TRUE        if required values are valid.
 * \retval     FALSE       if a required value is absent or invalid.
 */
static bool_t ReadSize(const Utl_Xml_Node_t* node, Utl_Arinc661_Size_t* size);

/*************************************************************************************************/
/**
 * \brief Reads a FACE Rectangle element.
 *
 * \param[in]  node        Element with x, y, width, and height
 * \param[out] rectangle   Parsed rectangle
 *
 * \retval     TRUE        if required values are valid.
 * \retval     FALSE       if a required value is absent or invalid.
 */
static bool_t ReadRectangle(const Utl_Xml_Node_t* node, Utl_Arinc661_Rectangle_t* rectangle);

/*************************************************************************************************/
/**
 * \brief Reads a FACE Scale element.
 *
 * \param[in]  node    Element with scale factors and optional units
 * \param[out] scale   Parsed scaling factor
 *
 * \retval     TRUE        if required values are valid.
 * \retval     FALSE       if a required value is absent or invalid.
 */
static bool_t ReadScale(const Utl_Xml_Node_t* node, Utl_Arinc661_Scale_t* scale);

/*************************************************************************************************/
/**
 * \brief Reads one configuration screen and its child layouts and windows.
 *
 * \param[in,out] configuration   Caller-owned result arrays
 * \param[in]     element         Screen XML element
 *
 * \retval        TRUE        if the supported screen data fits and is valid.
 * \retval        FALSE       if a value is invalid or storage is exhausted.
 */
static bool_t ReadScreenHierarchy(Utl_Arinc661_Configuration_t* configuration, Utl_Xml_Node_t* element);

/*************************************************************************************************/
/**
 * \brief Reads a user application from the display management configuration.
 *
 * \param[in,out] configuration   Caller-owned result arrays
 * \param[in]     node            User application XML element
 *
 * \retval        TRUE        if the application was read.
 * \retval        FALSE       if a required value is absent, malformed, or storage is exhausted.
 */
static bool_t ReadUserApplication(Utl_Arinc661_Configuration_t* configuration, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Reads the standard attributes of an external source.
 *
 * \param[in,out] configuration   Caller-owned result arrays
 * \param[in]     node            External source XML element
 *
 * \retval        TRUE        if the external source was read.
 * \retval        FALSE       if a value is invalid or storage is exhausted.
 */
static bool_t ReadExternalSource(Utl_Arinc661_Configuration_t* configuration, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Returns the FACE XSD spelling of a unit.
 *
 * \param[in] unit   Unit value
 *
 * \return Unit spelling, or NULL for an invalid value.
 */
static const char_t* UnitName(Utl_Arinc661_Unit_t unit);

/*************************************************************************************************/
/**
 * \brief Writes a FACE Size as an XML child of its owner.
 *
 * \param[in,out] xml      Temporary XML tree and value storage
 * \param[in,out] parent   Owning XML element
 * \param[in]     name     Child element name
 * \param[in]     size     Size values
 *
 * \retval        TRUE        if the size element was appended.
 * \retval        FALSE       if a value is invalid or storage is exhausted.
 */
static bool_t WriteSize(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const char_t* name, const Utl_Arinc661_Size_t* size);

/*************************************************************************************************/
/**
 * \brief Writes a FACE Rectangle as an XML child of its owner.
 *
 * \param[in,out] xml         Temporary XML tree and value storage
 * \param[in,out] parent      Owning XML element
 * \param[in]     rectangle   Rectangle values
 *
 * \retval        TRUE        if the rectangle element was appended.
 * \retval        FALSE       if storage is exhausted.
 */
static bool_t WriteRectangle(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Rectangle_t* rectangle);

/*************************************************************************************************/
/**
 * \brief Writes a FACE Scale as an XML child of its owner.
 *
 * \param[in,out] xml      Temporary XML tree and value storage
 * \param[in,out] parent   Owning XML element
 * \param[in]     scale    Scaling factor values
 *
 * \retval        TRUE        if the scaling factor element was appended.
 * \retval        FALSE       if a value is invalid or storage is exhausted.
 */
static bool_t WriteScale(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Scale_t* scale);

/*************************************************************************************************/
/**
 * \brief Writes one user application as a child of the configuration element.
 *
 * \param[in,out] xml           Temporary XML tree and value storage
 * \param[in,out] parent        Configuration XML element
 * \param[in]     application   User application values
 *
 * \retval        TRUE        if the user application was appended.
 * \retval        FALSE       if a value is invalid or storage is exhausted.
 */
static bool_t WriteUserApplication(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief Writes one external source with its standard attributes.
 *
 * \param[in,out] xml      Temporary XML tree and value storage
 * \param[in,out] parent   Configuration XML element
 * \param[in]     source   External source attributes
 *
 * \retval        TRUE        if the external source was appended.
 * \retval        FALSE       if its identifier is out of range or storage is exhausted.
 */
static bool_t WriteExternalSource(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_ExternalSource_t* source);

/*************************************************************************************************/
/**
 * \brief Advances within one XML subtree without recursion.
 *
 * \param[in] root      Root of the traversal
 * \param[in] current   Node just visited
 *
 * \return Next node in document order, or NULL after the subtree.
 */
static Utl_Xml_Node_t* NextSubtreeNode(Utl_Xml_Node_t* root, Utl_Xml_Node_t* current);

/*************************************************************************************************/
/**
 * \brief Reads direct prop children of one Definition File model.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     model        XML model element
 * \param[in]     owner        Kind of entity containing the model
 * \param[in]     ownerIndex   Layer or widget index, or zero for the Definition File
 *
 * \retval        TRUE        if all properties fit the supplied storage.
 * \retval        FALSE       if property storage or string capacity was exceeded.
 */
static bool_t ReadProperties(Utl_Arinc661_Definition_t* definition, Utl_Xml_Node_t* model, Utl_Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*************************************************************************************************/
/**
 * \brief Writes simple properties for one Definition File model.
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
static bool_t WriteProperties(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Definition_t* definition, Utl_Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses fixed XML capacities and leaves the remaining workspace for formatted values.
 */
static bool_t InitializeXmlWorkspace(XmlWorkspace_t* xml, void* bytes, uint64_t byteCount)
{
    uint64_t node_bytes;
    uint64_t attribute_bytes;
    uint64_t widget_bytes;
    uint64_t reserved_bytes;
    bool_t   is_aligned;
    bool_t   has_capacity;

    node_bytes      = (uint64_t)ARINC661_XML_NODE_CAPACITY * sizeof(Utl_Xml_Node_t);
    attribute_bytes = (uint64_t)ARINC661_XML_ATTRIBUTE_CAPACITY * sizeof(Utl_Xml_Attribute_t);
    widget_bytes    = (uint64_t)ARINC661_XML_WIDGET_CAPACITY * sizeof(Utl_Xml_Node_t*);
    reserved_bytes  = ((node_bytes + attribute_bytes) + widget_bytes);
    is_aligned      = (0 == ((uintptr_t)bytes % sizeof(uint64_t)));
    has_capacity    = (reserved_bytes < byteCount);
    if (is_aligned && has_capacity)
    {
        xml->nodes          = (Utl_Xml_Node_t*)bytes;
        xml->attributes     = (Utl_Xml_Attribute_t*)((byte_t*)bytes + node_bytes);
        xml->widgetNodes    = (Utl_Xml_Node_t**)((byte_t*)bytes + (node_bytes + attribute_bytes));
        xml->text           = (byte_t*)bytes + reserved_bytes;
        xml->textCapacity   = byteCount - reserved_bytes;
        xml->textByteCount  = 0;
        xml->nodeCount      = 0;
        xml->attributeCount = 0;
        Utl_Xml_InitializeDocument(&xml->document);
    }

    return is_aligned && has_capacity;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML parser names are counted byte spans.
 */
static bool_t HasElementName(const Utl_Xml_Node_t* node, const char_t* name)
{
    uint32_t byte_count;
    bool_t   is_element;
    bool_t   has_length;
    bool_t   has_name;

    byte_count = (uint32_t)strlen(name);
    is_element = (NULL != node);
    if (is_element)
    {
        is_element = (UTL_XML_NODE_ELEMENT == node->type);
    }
    has_length = FALSE;
    has_name   = FALSE;
    if (is_element)
    {
        has_length = (byte_count == node->nameByteCount);
        if (has_length)
        {
            has_name = (0 == memcmp(node->name, name, byte_count));
        }
    }

    return has_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Searches siblings without entering their descendants.
 */
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* found_node;
    uint32_t        node_index;

    node       = parent->firstChild;
    found_node = NULL;
    for (node_index = 0; node_index < ARINC661_XML_NODE_CAPACITY; ++node_index)
    {
        if (NULL == node)
        {
            break;
        }
        if (HasElementName(node, name))
        {
            found_node = node;
            break;
        }
        node = node->nextSibling;
    }
    if (NULL == found_node)
    {
        LOGASSERT(NULL == node);
    }

    return found_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Matches the complete counted attribute name.
 */
static const Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const char_t* name)
{
    const Utl_Xml_Attribute_t* attribute;
    const Utl_Xml_Attribute_t* found_attribute;
    uint32_t                   byte_count;
    uint32_t                   attribute_index;
    bool_t                     has_length;
    bool_t                     has_name;

    attribute       = node->firstAttribute;
    found_attribute = NULL;
    byte_count      = (uint32_t)strlen(name);
    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != attribute);
        has_length = (byte_count == attribute->nameByteCount);
        has_name   = FALSE;
        if (has_length)
        {
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
        LOGASSERT(NULL == attribute);
    }

    return found_attribute;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares the value bytes only after checking their count.
 */
static bool_t HasAttributeValue(const Utl_Xml_Attribute_t* attribute, const char_t* value)
{
    uint32_t byte_count;
    bool_t   has_length;
    bool_t   has_value;

    byte_count = (uint32_t)strlen(value);
    has_length = (byte_count == attribute->valueByteCount);
    has_value  = FALSE;
    if (has_length)
    {
        has_value = (0 == memcmp(attribute->value, value, byte_count));
    }

    return has_value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Absent optional attributes produce an empty string.
 */
static bool_t CopyAttribute(const Utl_Xml_Node_t* node, const char_t* name, char_t* destination, uint32_t capacity)
{
    const Utl_Xml_Attribute_t* attribute;
    bool_t                     fits;

    attribute      = FindAttribute(node, name);
    destination[0] = '\0';
    fits           = TRUE;
    if (NULL != attribute)
    {
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
 *  Requires the scanned prefix to fill the counted attribute value.
 */
static bool_t ReadInteger(const Utl_Xml_Node_t* node, const char_t* name, int64_t* value)
{
    const Utl_Xml_Attribute_t* attribute;
    int64_t                    parsed_value;
    int32_t                    consumed;
    bool_t                     is_valid;

    attribute = FindAttribute(node, name);
    is_valid  = (NULL != attribute);
    if (is_valid)
    {
        consumed = Utl_Scan_Int64((const char_t*)attribute->value, &parsed_value);
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
 *  Converts a complete counted decimal attribute to float32_t.
 */
static bool_t ReadDecimal(const Utl_Xml_Node_t* node, const char_t* name, float32_t* value)
{
    const Utl_Xml_Attribute_t* attribute;
    float64_t                  parsed_value;
    int32_t                    consumed;
    bool_t                     is_valid;

    attribute = FindAttribute(node, name);
    is_valid  = (NULL != attribute);
    if (is_valid)
    {
        consumed = Utl_Scan_Float((const char_t*)attribute->value, &parsed_value);
        is_valid = (0 < consumed) && ((uint32_t)consumed == attribute->valueByteCount);
        if (is_valid)
        {
            is_valid = (parsed_value <= FLT_MAX) && (parsed_value >= -FLT_MAX);
        }
        if (is_valid)
        {
            (*value) = (float32_t)parsed_value;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Applies the XSD default when the optional unit attribute is absent.
 */
static bool_t ReadUnit(const Utl_Xml_Node_t* node, const char_t* name, Utl_Arinc661_Unit_t defaultUnit, Utl_Arinc661_Unit_t* unit)
{
    const Utl_Xml_Attribute_t* attribute;
    bool_t                     is_valid;

    attribute = FindAttribute(node, name);
    (*unit) = defaultUnit;
    is_valid = TRUE;
    if (NULL != attribute)
    {
        is_valid = FALSE;
        if (HasAttributeValue(attribute, UTL_ARINC661_XML_UNIT_INCH))
        {
            (*unit) = UTL_ARINC661_UNIT_INCH;
            is_valid = TRUE;
        }
        else if (HasAttributeValue(attribute, UTL_ARINC661_XML_UNIT_MM))
        {
            (*unit) = UTL_ARINC661_UNIT_MM;
            is_valid = TRUE;
        }
        else if (HasAttributeValue(attribute, UTL_ARINC661_XML_UNIT_SCREEN))
        {
            (*unit) = UTL_ARINC661_UNIT_SCREEN;
            is_valid = TRUE;
        }
        else if (HasAttributeValue(attribute, UTL_ARINC661_XML_UNIT_PIXEL))
        {
            (*unit) = UTL_ARINC661_UNIT_PIXEL;
            is_valid = TRUE;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML Schema boolean lexical values include true, false, 1, and 0.
 */
static bool_t ReadBoolean(const Utl_Xml_Node_t* node, const char_t* name, bool_t* value)
{
    const Utl_Xml_Attribute_t* attribute;
    bool_t                     is_true;
    bool_t                     is_one;
    bool_t                     is_false;
    bool_t                     is_zero;
    bool_t                     is_valid;

    (*value) = FALSE;
    attribute = FindAttribute(node, name);
    is_valid  = TRUE;
    if (NULL != attribute)
    {
        is_valid = FALSE;
        is_true  = HasAttributeValue(attribute, ARINC661_XML_BOOLEAN_TRUE);
        is_one   = HasAttributeValue(attribute, ARINC661_XML_BOOLEAN_ONE);
        is_false = HasAttributeValue(attribute, ARINC661_XML_BOOLEAN_FALSE);
        is_zero  = HasAttributeValue(attribute, ARINC661_XML_BOOLEAN_ZERO);
        if (is_true || is_one)
        {
            (*value) = TRUE;
            is_valid = TRUE;
        }
        else if (is_false || is_zero)
        {
            (*value) = FALSE;
            is_valid = TRUE;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller links the stored element into the document.
 */
static Utl_Xml_Node_t* AppendElement(XmlWorkspace_t* xml, const char_t* name)
{
    Utl_Xml_Node_t* node;

    node = NULL;
    if (xml->nodeCount < ARINC661_XML_NODE_CAPACITY)
    {
        node = &xml->nodes[xml->nodeCount];
        Utl_Xml_InitializeNode(node, UTL_XML_NODE_ELEMENT, (const byte_t*)name, (uint32_t)strlen(name));
        xml->nodeCount += 1;
    }

    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the value before Utl_Xml_AppendAttribute asserts its preconditions.
 */
static bool_t AppendAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* attribute;
    uint32_t             byte_count;
    bool_t               has_storage;
    bool_t               has_value;
    bool_t               is_valid;

    has_storage = (xml->attributeCount < ARINC661_XML_ATTRIBUTE_CAPACITY);
    has_value   = (NULL != value);
    is_valid    = FALSE;
    if (has_storage && has_value)
    {
        byte_count = (uint32_t)strlen(value);
        is_valid   = Utl_Xml_IsAttributeValue((const byte_t*)value, byte_count);
        if (is_valid)
        {
            attribute = &xml->attributes[xml->attributeCount];
            Utl_Xml_InitializeAttribute(attribute, (const byte_t*)name, (uint32_t)strlen(name), (const byte_t*)value, byte_count);
            Utl_Xml_AppendAttribute(node, attribute);
            xml->attributeCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Allocates one terminated value in the caller's temporary byte span.
 */
static const char_t* FormatInteger(XmlWorkspace_t* xml, int64_t value)
{
    char_t* text;
    int     byte_count;

    text = NULL;
    if (32 <= (xml->textCapacity - xml->textByteCount))
    {
        text       = (char_t*)&xml->text[xml->textByteCount];
        byte_count = snprintf(text, 32, "%" PRId64, value);
        LOGASSERT((0 < byte_count) && (byte_count < 32));
        xml->textByteCount += ((uint64_t)byte_count + 1);
    }

    return text;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes XML Schema decimal syntax without an exponent. Fifty-four fractional
 *  digits preserve the smallest float32_t values; unused zeros are removed.
 */
static const char_t* FormatDecimal(XmlWorkspace_t* xml, float32_t value)
{
    char_t* text;
    int     byte_count;
    bool_t  is_finite;
    bool_t  has_capacity;

    text         = NULL;
    is_finite    = (value <= FLT_MAX) && (value >= -FLT_MAX);
    has_capacity = (ARINC661_XML_DECIMAL_TEXT_BYTES <= (xml->textCapacity - xml->textByteCount));
    if (is_finite && has_capacity)
    {
        text       = (char_t*)&xml->text[xml->textByteCount];
        byte_count = snprintf(text, ARINC661_XML_DECIMAL_TEXT_BYTES, "%.54f", value);
        LOGASSERT((0 < byte_count) && (byte_count < ARINC661_XML_DECIMAL_TEXT_BYTES));
        while ('0' == text[byte_count - 1])
        {
            byte_count -= 1;
        }
        if ('.' == text[byte_count - 1])
        {
            byte_count -= 1;
        }
        text[byte_count] = '\0';
        xml->textByteCount += ((uint64_t)byte_count + 1);
    }

    return text;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Formats the value before linking its attribute.
 */
static bool_t AppendIntegerAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, int64_t value)
{
    const char_t* text;
    bool_t        is_appended;

    text        = FormatInteger(xml, value);
    is_appended = AppendAttribute(xml, node, name, text);

    return is_appended;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Formats the value before linking its attribute.
 */
static bool_t AppendDecimalAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, float32_t value)
{
    const char_t* text;
    bool_t        is_appended;

    text        = FormatDecimal(xml, value);
    is_appended = AppendAttribute(xml, node, name, text);

    return is_appended;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static Utl_Xml_Node_t* FindDocumentElement(const Utl_Xml_Document_t* document)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* element;
    uint32_t        node_index;

    node    = document->firstNode;
    element = NULL;
    for (node_index = 0; node_index < ARINC661_XML_NODE_CAPACITY; ++node_index)
    {
        if (NULL == node)
        {
            break;
        }
        if (UTL_XML_NODE_ELEMENT == node->type)
        {
            element = node;
            break;
        }
        node = node->nextSibling;
    }
    if (NULL == element)
    {
        LOGASSERT(NULL == node);
    }

    return element;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadSize(const Utl_Xml_Node_t* node, Utl_Arinc661_Size_t* size)
{
    bool_t is_valid;

    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_WIDTH, &size->width);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_HEIGHT, &size->height);
    }
    if (is_valid)
    {
        is_valid = ReadUnit(node, ARINC661_XML_ATTRIBUTE_UNITS, UTL_ARINC661_UNIT_PIXEL, &size->units);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadRectangle(const Utl_Xml_Node_t* node, Utl_Arinc661_Rectangle_t* rectangle)
{
    bool_t is_valid;

    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_X, &rectangle->x);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_Y, &rectangle->y);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_WIDTH, &rectangle->width);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_HEIGHT, &rectangle->height);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadScale(const Utl_Xml_Node_t* node, Utl_Arinc661_Scale_t* scale)
{
    bool_t is_valid;

    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = ReadDecimal(node, ARINC661_XML_ATTRIBUTE_X_SCALE, &scale->xScale);
    }
    if (is_valid)
    {
        is_valid = ReadDecimal(node, ARINC661_XML_ATTRIBUTE_Y_SCALE, &scale->yScale);
    }
    if (is_valid)
    {
        is_valid = ReadUnit(node, ARINC661_XML_ATTRIBUTE_BASE_UNIT, UTL_ARINC661_UNIT_SCREEN, &scale->baseUnit);
    }
    if (is_valid)
    {
        is_valid = ReadUnit(node, ARINC661_XML_ATTRIBUTE_PER_UNIT, UTL_ARINC661_UNIT_PIXEL, &scale->perUnit);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadScreenHierarchy(Utl_Arinc661_Configuration_t* configuration, Utl_Xml_Node_t* element)
{
    Utl_Arinc661_Screen_t* screen;
    Utl_Arinc661_Layout_t* layout;
    Utl_Arinc661_Window_t* window;
    Utl_Xml_Node_t*        layout_node;
    Utl_Xml_Node_t*        window_node;
    Utl_Xml_Node_t*        detail_node;
    uint32_t               screen_index;
    uint32_t               layout_index;
    uint32_t               detail_index;
    uint32_t               screen_array_index;
    uint32_t               layout_array_index;
    bool_t                 is_pixel_size;
    bool_t                 is_physical_dimensions;
    bool_t                 is_description;
    bool_t                 is_pixel_area;
    bool_t                 is_scaling_factor;
    bool_t                 is_valid;

    is_valid = (configuration->screenCount < configuration->screenCapacity);
    if (is_valid)
    {
        screen_array_index = configuration->screenCount;
        screen             = &configuration->screens[screen_array_index];
        (void)memset(screen, 0, sizeof((*screen)));
        is_valid = ReadInteger(element, ARINC661_XML_ATTRIBUTE_ID, &screen->id);
        if (is_valid)
        {
            detail_node = FindChild(element, ARINC661_DMC_ELEMENT_PIXEL_SIZE);
            is_valid    = ReadSize(detail_node, &screen->pixelSize);
        }
        if (is_valid)
        {
            detail_node = FindChild(element, ARINC661_DMC_ELEMENT_PHYSICAL_DIMENSIONS);
            is_valid    = ReadSize(detail_node, &screen->physicalDimensions);
        }
        if (is_valid)
        {
            configuration->screenCount += 1;
            layout_node = element->firstChild;
            for (screen_index = 0; screen_index < ARINC661_XML_NODE_CAPACITY; ++screen_index)
            {
                if (NULL == layout_node)
                {
                    break;
                }
                if (HasElementName(layout_node, ARINC661_DMC_ELEMENT_LAYOUT))
                {
                    is_valid = (configuration->layoutCount < configuration->layoutCapacity);
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    layout_array_index = configuration->layoutCount;
                    layout             = &configuration->layouts[layout_array_index];
                    (void)memset(layout, 0, sizeof((*layout)));
                    layout->screenIndex = screen_array_index;
                    is_valid            = ReadInteger(layout_node, ARINC661_XML_ATTRIBUTE_ID, &layout->id);
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    configuration->layoutCount += 1;
                    window_node = layout_node->firstChild;
                    for (layout_index = 0; layout_index < ARINC661_XML_NODE_CAPACITY; ++layout_index)
                    {
                        if (NULL == window_node)
                        {
                            break;
                        }
                        if (HasElementName(window_node, ARINC661_DMC_ELEMENT_WINDOW))
                        {
                            is_valid = (configuration->windowCount < configuration->windowCapacity);
                            if (FALSE == is_valid)
                            {
                                break;
                            }
                            window = &configuration->windows[configuration->windowCount];
                            (void)memset(window, 0, sizeof((*window)));
                            window->layoutIndex = layout_array_index;
                            is_valid            = ReadInteger(window_node, ARINC661_XML_ATTRIBUTE_ID, &window->id);
                            if (is_valid)
                            {
                                is_valid = CopyAttribute(window_node, ARINC661_XML_ATTRIBUTE_NAME, window->name, sizeof(window->name));
                            }
                            if (is_valid)
                            {
                                detail_node = FindChild(window_node, ARINC661_DMC_ELEMENT_PIXEL_AREA);
                                is_valid    = ReadRectangle(detail_node, &window->pixelArea);
                            }
                            if (is_valid)
                            {
                                detail_node = FindChild(window_node, ARINC661_DMC_ELEMENT_SCALING_FACTOR);
                                is_valid    = ReadScale(detail_node, &window->scalingFactor);
                            }
                            if (is_valid)
                            {
                                detail_node = window_node->firstChild;
                                for (detail_index = 0; detail_index < ARINC661_XML_NODE_CAPACITY; ++detail_index)
                                {
                                    if (NULL == detail_node)
                                    {
                                        break;
                                    }
                                    is_description    = HasElementName(detail_node, ARINC661_DMC_ELEMENT_DESCRIPTION);
                                    is_pixel_area     = HasElementName(detail_node, ARINC661_DMC_ELEMENT_PIXEL_AREA);
                                    is_scaling_factor = HasElementName(detail_node, ARINC661_DMC_ELEMENT_SCALING_FACTOR);
                                    is_valid          = is_description || is_pixel_area || is_scaling_factor;
                                    if (FALSE == is_valid)
                                    {
                                        break;
                                    }
                                    detail_node = detail_node->nextSibling;
                                }
                                if (is_valid)
                                {
                                    LOGASSERT(NULL == detail_node);
                                }
                            }
                            if (FALSE == is_valid)
                            {
                                break;
                            }
                            configuration->windowCount += 1;
                        }
                        else
                        {
                            is_valid = FALSE;
                            break;
                        }
                        window_node = window_node->nextSibling;
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    LOGASSERT(NULL == window_node);
                }
                else
                {
                    is_pixel_size          = HasElementName(layout_node, ARINC661_DMC_ELEMENT_PIXEL_SIZE);
                    is_physical_dimensions = HasElementName(layout_node, ARINC661_DMC_ELEMENT_PHYSICAL_DIMENSIONS);
                    is_valid               = is_pixel_size || is_physical_dimensions;
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                }
                layout_node = layout_node->nextSibling;
            }
            if (is_valid)
            {
                LOGASSERT(NULL == layout_node);
            }
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadUserApplication(Utl_Arinc661_Configuration_t* configuration, const Utl_Xml_Node_t* node)
{
    Utl_Arinc661_UserApplication_t* application;
    const Utl_Xml_Attribute_t*      df_path;
    bool_t                          is_valid;

    is_valid = (configuration->userApplicationCount < configuration->userApplicationCapacity);
    if (is_valid)
    {
        application = &configuration->userApplications[configuration->userApplicationCount];
        (void)memset(application, 0, sizeof((*application)));
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_APPLICATION_ID, &application->applicationId);
        if (is_valid)
        {
            is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_WINDOW, &application->window);
        }
        if (is_valid)
        {
            df_path  = FindAttribute(node, ARINC661_XML_ATTRIBUTE_DF_PATH);
            is_valid = (NULL != df_path);
        }
        if (is_valid)
        {
            is_valid = CopyAttribute(node, ARINC661_XML_ATTRIBUTE_DF_PATH, application->dFPath, sizeof(application->dFPath));
        }
        if (is_valid)
        {
            is_valid = CopyAttribute(node, ARINC661_XML_ATTRIBUTE_STYLE_FILE_PATH, application->styleFilePath, sizeof(application->styleFilePath));
        }
        if (is_valid)
        {
            is_valid = ReadBoolean(node, ARINC661_XML_ATTRIBUTE_MAP_SOURCE_LAYER, &application->mapSourceLayer);
        }
        if (is_valid)
        {
            is_valid = ReadBoolean(node, ARINC661_XML_ATTRIBUTE_VISIBLE, &application->visible);
        }
        if (is_valid)
        {
            configuration->userApplicationCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadExternalSource(Utl_Arinc661_Configuration_t* configuration, const Utl_Xml_Node_t* node)
{
    Utl_Arinc661_ExternalSource_t* source;
    int64_t                        id;
    bool_t                         is_valid;

    is_valid = (configuration->externalSourceCount < configuration->externalSourceCapacity);
    if (is_valid)
    {
        source = &configuration->externalSources[configuration->externalSourceCount];
        (void)memset(source, 0, sizeof((*source)));
        is_valid = ReadInteger(node, ARINC661_XML_ATTRIBUTE_ID, &id);
        if (is_valid)
        {
            is_valid = (0 <= id);
        }
        if (is_valid)
        {
            is_valid = (UINT16_MAX >= id);
        }
        if (is_valid)
        {
            source->id = (uint32_t)id;
            is_valid   = CopyAttribute(node, ARINC661_XML_ATTRIBUTE_NAME, source->name, sizeof(source->name));
        }
        if (is_valid)
        {
            is_valid = CopyAttribute(node, ARINC661_XML_ATTRIBUTE_TYPE, source->type, sizeof(source->type));
        }
        if (is_valid)
        {
            is_valid = (NULL == node->firstChild);
        }
        if (is_valid)
        {
            configuration->externalSourceCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static const char_t* UnitName(Utl_Arinc661_Unit_t unit)
{
    const char_t* name;

    name = NULL;
    if (UTL_ARINC661_UNIT_INCH == unit)
    {
        name = UTL_ARINC661_XML_UNIT_INCH;
    }
    else if (UTL_ARINC661_UNIT_MM == unit)
    {
        name = UTL_ARINC661_XML_UNIT_MM;
    }
    else if (UTL_ARINC661_UNIT_SCREEN == unit)
    {
        name = UTL_ARINC661_XML_UNIT_SCREEN;
    }
    else if (UTL_ARINC661_UNIT_PIXEL == unit)
    {
        name = UTL_ARINC661_XML_UNIT_PIXEL;
    }

    return name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteSize(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const char_t* name, const Utl_Arinc661_Size_t* size)
{
    Utl_Xml_Node_t* node;
    const char_t*   unit_name;
    bool_t          is_valid;

    node     = AppendElement(xml, name);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_WIDTH, size->width);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_HEIGHT, size->height);
    }
    if (is_valid)
    {
        unit_name = UnitName(size->units);
        is_valid  = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_UNITS, unit_name);
    }
    if (is_valid)
    {
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteRectangle(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Rectangle_t* rectangle)
{
    Utl_Xml_Node_t* node;
    bool_t          is_valid;

    node     = AppendElement(xml, ARINC661_DMC_ELEMENT_PIXEL_AREA);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_X, rectangle->x);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_Y, rectangle->y);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_WIDTH, rectangle->width);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_HEIGHT, rectangle->height);
    }
    if (is_valid)
    {
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteScale(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Scale_t* scale)
{
    Utl_Xml_Node_t* node;
    const char_t*   base_unit_name;
    const char_t*   per_unit_name;
    bool_t          is_valid;

    node     = AppendElement(xml, ARINC661_DMC_ELEMENT_SCALING_FACTOR);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendDecimalAttribute(xml, node, ARINC661_XML_ATTRIBUTE_X_SCALE, scale->xScale);
    }
    if (is_valid)
    {
        is_valid = AppendDecimalAttribute(xml, node, ARINC661_XML_ATTRIBUTE_Y_SCALE, scale->yScale);
    }
    if (is_valid)
    {
        base_unit_name = UnitName(scale->baseUnit);
        is_valid       = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_BASE_UNIT, base_unit_name);
    }
    if (is_valid)
    {
        per_unit_name = UnitName(scale->perUnit);
        is_valid      = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_PER_UNIT, per_unit_name);
    }
    if (is_valid)
    {
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteUserApplication(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_UserApplication_t* application)
{
    Utl_Xml_Node_t* node;
    const char_t*   map_source_layer_text;
    const char_t*   visible_text;
    bool_t          is_valid;

    map_source_layer_text = ARINC661_XML_BOOLEAN_FALSE;
    if (application->mapSourceLayer)
    {
        map_source_layer_text = ARINC661_XML_BOOLEAN_TRUE;
    }
    visible_text = ARINC661_XML_BOOLEAN_FALSE;
    if (application->visible)
    {
        visible_text = ARINC661_XML_BOOLEAN_TRUE;
    }
    node     = AppendElement(xml, ARINC661_DMC_ELEMENT_USER_APPLICATION);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_APPLICATION_ID, application->applicationId);
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_DF_PATH, application->dFPath);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_WINDOW, application->window);
    }
    if (is_valid)
    {
        if ('\0' != application->styleFilePath[0])
        {
            is_valid = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_STYLE_FILE_PATH, application->styleFilePath);
        }
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_MAP_SOURCE_LAYER, map_source_layer_text);
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_VISIBLE, visible_text);
    }
    if (is_valid)
    {
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteExternalSource(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_ExternalSource_t* source)
{
    Utl_Xml_Node_t* node;
    bool_t          is_valid;

    is_valid = (UINT16_MAX >= source->id);
    if (is_valid)
    {
        node     = AppendElement(xml, ARINC661_DMC_ELEMENT_EXTERNAL_SOURCE);
        is_valid = (NULL != node);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, ARINC661_XML_ATTRIBUTE_ID, source->id);
    }
    if (is_valid)
    {
        if ('\0' != source->name[0])
        {
            is_valid = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_NAME, source->name);
        }
    }
    if (is_valid)
    {
        if ('\0' != source->type[0])
        {
            is_valid = AppendAttribute(xml, node, ARINC661_XML_ATTRIBUTE_TYPE, source->type);
        }
    }
    if (is_valid)
    {
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static Utl_Xml_Node_t* NextSubtreeNode(Utl_Xml_Node_t* root, Utl_Xml_Node_t* current)
{
    Utl_Xml_Node_t* next_node;
    Utl_Xml_Node_t* ancestor;
    uint32_t        ancestor_index;

    next_node = current->firstChild;
    ancestor  = current;
    if (NULL == next_node)
    {
        for (ancestor_index = 0; ancestor_index < UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
        {
            if (root == ancestor)
            {
                break;
            }
            next_node = ancestor->nextSibling;
            if (NULL != next_node)
            {
                break;
            }
            ancestor = ancestor->parent;
            LOGASSERT(NULL != ancestor);
        }
    }

    return next_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadProperties(Utl_Arinc661_Definition_t* definition, Utl_Xml_Node_t* model, Utl_Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    Utl_Xml_Node_t*          property_node;
    Utl_Arinc661_Property_t* property;
    uint32_t                 node_index;
    bool_t                   is_valid;

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
            if (HasElementName(property_node, ARINC661_DF_ELEMENT_PROPERTY))
            {
                is_valid = (definition->propertyCount < definition->propertyCapacity);
                if (FALSE == is_valid)
                {
                    break;
                }
                property = &definition->properties[definition->propertyCount];
                (void)memset(property, 0, sizeof((*property)));
                property->owner      = owner;
                property->ownerIndex = ownerIndex;
                is_valid             = CopyAttribute(property_node, ARINC661_XML_ATTRIBUTE_NAME, property->name, sizeof(property->name));
                if (is_valid)
                {
                    is_valid = CopyAttribute(property_node, ARINC661_XML_ATTRIBUTE_VALUE, property->value, sizeof(property->value));
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                definition->propertyCount += 1;
            }
            else
            {
                is_valid = FALSE;
                break;
            }
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
 *  None
 */
static bool_t WriteProperties(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Definition_t* definition, Utl_Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    Utl_Xml_Node_t*                model;
    Utl_Xml_Node_t*                property_node;
    const Utl_Arinc661_Property_t* property;
    uint32_t                       property_index;
    bool_t                         is_owner;
    bool_t                         has_index;
    bool_t                         is_valid;

    model    = AppendElement(xml, ARINC661_DF_ELEMENT_MODEL);
    is_valid = (NULL != model);
    if (is_valid)
    {
        for (property_index = 0; property_index < definition->propertyCount; ++property_index)
        {
            property  = &definition->properties[property_index];
            is_owner  = (owner == property->owner);
            has_index = (ownerIndex == property->ownerIndex);
            if (is_owner && has_index)
            {
                property_node = AppendElement(xml, ARINC661_DF_ELEMENT_PROPERTY);
                is_valid      = (NULL != property_node);
                if (is_valid)
                {
                    is_valid = AppendAttribute(xml, property_node, ARINC661_XML_ATTRIBUTE_NAME, property->name);
                }
                if (is_valid)
                {
                    is_valid = AppendAttribute(xml, property_node, ARINC661_XML_ATTRIBUTE_VALUE, property->value);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                Utl_Xml_AppendChild(model, property_node);
            }
        }
    }
    if (is_valid)
    {
        Utl_Xml_AppendChild(parent, model);
    }

    return is_valid;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  XML nodes and attributes live only in the opaque temporary workspace.
 */
bool_t Utl_Arinc661_ReadConfiguration(Utl_Arinc661_Configuration_t* configuration, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t  xml;
    Utl_Xml_Node_t* root;
    Utl_Xml_Node_t* child_node;
    uint32_t        node_index;
    bool_t          has_workspace;
    bool_t          is_configuration;
    bool_t          is_valid;

    LOGASSERT(NULL != configuration);
    LOGASSERT(NULL != buffer);
    LOGASSERT(NULL != workspace);
    configuration->screenCount          = 0;
    configuration->layoutCount          = 0;
    configuration->windowCount          = 0;
    configuration->userApplicationCount = 0;
    configuration->externalSourceCount  = 0;
    configuration->hasMetaData          = FALSE;
    has_workspace                       = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid                            = has_workspace;
    if (has_workspace)
    {
        Utl_Xml_ReadDocument(&xml.document, buffer, byteCount, xml.nodes, ARINC661_XML_NODE_CAPACITY, xml.attributes, ARINC661_XML_ATTRIBUTE_CAPACITY);
        root             = FindDocumentElement(&xml.document);
        is_configuration = HasElementName(root, ARINC661_DMC_ELEMENT_CONFIGURATION);
        is_valid         = is_configuration;
        if (is_valid)
        {
            child_node = root->firstChild;
            for (node_index = 0; node_index < ARINC661_XML_NODE_CAPACITY; ++node_index)
            {
                if (NULL == child_node)
                {
                    break;
                }
                if (HasElementName(child_node, ARINC661_DMC_ELEMENT_METADATA))
                {
                    is_valid = (FALSE == configuration->hasMetaData);
                    if (is_valid)
                    {
                        configuration->hasMetaData = TRUE;
                    }
                }
                else if (HasElementName(child_node, ARINC661_DMC_ELEMENT_SCREEN))
                {
                    is_valid = ReadScreenHierarchy(configuration, child_node);
                }
                else if (HasElementName(child_node, ARINC661_DMC_ELEMENT_USER_APPLICATION))
                {
                    is_valid = ReadUserApplication(configuration, child_node);
                }
                else if (HasElementName(child_node, ARINC661_DMC_ELEMENT_EXTERNAL_SOURCE))
                {
                    is_valid = ReadExternalSource(configuration, child_node);
                }
                else
                {
                    is_valid = FALSE;
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                child_node = child_node->nextSibling;
            }
            if (is_valid)
            {
                LOGASSERT(NULL == child_node);
            }
        }
    }
    if (FALSE == is_valid)
    {
        configuration->screenCount          = 0;
        configuration->layoutCount          = 0;
        configuration->windowCount          = 0;
        configuration->userApplicationCount = 0;
        configuration->externalSourceCount  = 0;
        configuration->hasMetaData          = FALSE;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Constructs an XML document from the separate configuration values and serializes it in memory.
 */
uint64_t Utl_Arinc661_WriteConfiguration(const Utl_Arinc661_Configuration_t* configuration, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t               xml;
    Utl_Xml_Node_t*              declaration;
    Utl_Xml_Node_t*              root;
    Utl_Xml_Node_t*              screen_node;
    Utl_Xml_Node_t*              layout_node;
    Utl_Xml_Node_t*              window_node;
    Utl_Xml_Node_t*              description_node;
    Utl_Xml_Node_t*              metadata_node;
    const Utl_Arinc661_Screen_t* screen;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Window_t* window;
    uint64_t                     byte_count;
    uint32_t                     screen_index;
    uint32_t                     layout_index;
    uint32_t                     window_index;
    uint32_t                     application_index;
    uint32_t                     source_index;
    bool_t                       has_workspace;
    bool_t                       is_valid;

    LOGASSERT(NULL != configuration);
    LOGASSERT(NULL != buffer);
    LOGASSERT(NULL != workspace);
    byte_count    = 0;
    has_workspace = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid      = has_workspace;
    if (is_valid)
    {
        declaration = &xml.nodes[xml.nodeCount];
        Utl_Xml_InitializeNode(declaration, UTL_XML_NODE_XML_DECLARATION, NULL, 0);
        xml.nodeCount += 1;
        Utl_Xml_AppendNode(&xml.document, declaration);
        root     = AppendElement(&xml, ARINC661_DMC_ELEMENT_CONFIGURATION);
        is_valid = (NULL != root);
        if (is_valid)
        {
            Utl_Xml_AppendNode(&xml.document, root);
        }
    }
    if (is_valid)
    {
        if (configuration->hasMetaData)
        {
            metadata_node = AppendElement(&xml, ARINC661_DMC_ELEMENT_METADATA);
            is_valid      = (NULL != metadata_node);
            if (is_valid)
            {
                Utl_Xml_AppendChild(root, metadata_node);
            }
        }
    }
    if (is_valid)
    {
        for (screen_index = 0; screen_index < configuration->screenCount; ++screen_index)
        {
            screen      = &configuration->screens[screen_index];
            screen_node = AppendElement(&xml, ARINC661_DMC_ELEMENT_SCREEN);
            is_valid    = (NULL != screen_node);
            if (is_valid)
            {
                is_valid = AppendIntegerAttribute(&xml, screen_node, ARINC661_XML_ATTRIBUTE_ID, screen->id);
            }
            if (is_valid)
            {
                is_valid = WriteSize(&xml, screen_node, ARINC661_DMC_ELEMENT_PIXEL_SIZE, &screen->pixelSize);
            }
            if (is_valid)
            {
                is_valid = WriteSize(&xml, screen_node, ARINC661_DMC_ELEMENT_PHYSICAL_DIMENSIONS, &screen->physicalDimensions);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            for (layout_index = 0; layout_index < configuration->layoutCount; ++layout_index)
            {
                layout = &configuration->layouts[layout_index];
                if (screen_index == layout->screenIndex)
                {
                    layout_node = AppendElement(&xml, ARINC661_DMC_ELEMENT_LAYOUT);
                    is_valid    = (NULL != layout_node);
                    if (is_valid)
                    {
                        is_valid = AppendIntegerAttribute(&xml, layout_node, ARINC661_XML_ATTRIBUTE_ID, layout->id);
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    for (window_index = 0; window_index < configuration->windowCount; ++window_index)
                    {
                        window = &configuration->windows[window_index];
                        if (layout_index == window->layoutIndex)
                        {
                            window_node = AppendElement(&xml, ARINC661_DMC_ELEMENT_WINDOW);
                            is_valid    = (NULL != window_node);
                            if (is_valid)
                            {
                                is_valid = AppendIntegerAttribute(&xml, window_node, ARINC661_XML_ATTRIBUTE_ID, window->id);
                            }
                            if (is_valid)
                            {
                                is_valid = AppendAttribute(&xml, window_node, ARINC661_XML_ATTRIBUTE_NAME, window->name);
                            }
                            if (is_valid)
                            {
                                is_valid = ('\0' == window->description[0]);
                            }
                            if (is_valid)
                            {
                                description_node = AppendElement(&xml, ARINC661_DMC_ELEMENT_DESCRIPTION);
                                is_valid         = (NULL != description_node);
                            }
                            if (is_valid)
                            {
                                Utl_Xml_AppendChild(window_node, description_node);
                                is_valid = WriteRectangle(&xml, window_node, &window->pixelArea);
                            }
                            if (is_valid)
                            {
                                is_valid = WriteScale(&xml, window_node, &window->scalingFactor);
                            }
                            if (FALSE == is_valid)
                            {
                                break;
                            }
                            Utl_Xml_AppendChild(layout_node, window_node);
                        }
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    Utl_Xml_AppendChild(screen_node, layout_node);
                }
            }
            if (FALSE == is_valid)
            {
                break;
            }
            Utl_Xml_AppendChild(root, screen_node);
        }
    }
    if (is_valid)
    {
        for (application_index = 0; application_index < configuration->userApplicationCount; ++application_index)
        {
            is_valid = WriteUserApplication(&xml, root, &configuration->userApplications[application_index]);
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }
    if (is_valid)
    {
        for (source_index = 0; source_index < configuration->externalSourceCount; ++source_index)
        {
            is_valid = WriteExternalSource(&xml, root, &configuration->externalSources[source_index]);
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }
    if (is_valid)
    {
        byte_count = Utl_Xml_WriteDocument(&xml.document, buffer, capacity);
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Interprets layers, nested widgets, and simple model properties from a Definition File.
 */
bool_t Utl_Arinc661_ReadDefinition(Utl_Arinc661_Definition_t* definition, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t         xml;
    Utl_Xml_Node_t*        root;
    Utl_Xml_Node_t*        layer_node;
    Utl_Xml_Node_t*        current_node;
    Utl_Xml_Node_t*        model_node;
    Utl_Arinc661_Layer_t*  layer;
    Utl_Arinc661_Widget_t* widget;
    int64_t                version;
    uint32_t               layer_index;
    uint32_t               node_index;
    uint32_t               widget_index;
    uint32_t               parent_index;
    bool_t                 has_workspace;
    bool_t                 is_model;
    bool_t                 is_property;
    bool_t                 is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    LOGASSERT(NULL != workspace);
    definition->layerCount    = 0;
    definition->widgetCount   = 0;
    definition->propertyCount = 0;
    has_workspace             = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid                  = has_workspace;
    if (is_valid)
    {
        Utl_Xml_ReadDocument(&xml.document, buffer, byteCount, xml.nodes, ARINC661_XML_NODE_CAPACITY, xml.attributes, ARINC661_XML_ATTRIBUTE_CAPACITY);
        root     = FindDocumentElement(&xml.document);
        is_valid = HasElementName(root, ARINC661_DF_ELEMENT_ROOT);
    }
    if (is_valid)
    {
        is_valid = CopyAttribute(root, ARINC661_XML_ATTRIBUTE_NAME, definition->name, sizeof(definition->name));
    }
    if (is_valid)
    {
        is_valid = ReadInteger(root, ARINC661_XML_ATTRIBUTE_LIBRARY_VERSION, &version);
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
        is_valid = ReadInteger(root, ARINC661_XML_ATTRIBUTE_SUPPLEMENT_VERSION, &version);
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
        model_node = FindChild(root, ARINC661_DF_ELEMENT_MODEL);
        is_valid   = ReadProperties(definition, model_node, UTL_ARINC661_PROPERTY_DEFINITION, 0);
    }
    if (is_valid)
    {
        layer_node = root->firstChild;
        for (layer_index = 0; layer_index < ARINC661_XML_NODE_CAPACITY; ++layer_index)
        {
            if (NULL == layer_node)
            {
                break;
            }
            if (HasElementName(layer_node, ARINC661_DF_ELEMENT_LAYER))
            {
                is_valid = (definition->layerCount < definition->layerCapacity);
                if (FALSE == is_valid)
                {
                    break;
                }
                layer = &definition->layers[definition->layerCount];
                (void)memset(layer, 0, sizeof((*layer)));
                is_valid = CopyAttribute(layer_node, ARINC661_XML_ATTRIBUTE_NAME, layer->name, sizeof(layer->name));
                if (is_valid)
                {
                    model_node = FindChild(layer_node, ARINC661_DF_ELEMENT_MODEL);
                    is_valid   = ReadProperties(definition, model_node, UTL_ARINC661_PROPERTY_LAYER, definition->layerCount);
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
                    if (HasElementName(current_node, ARINC661_DF_ELEMENT_WIDGET))
                    {
                        is_valid = (definition->widgetCount < definition->widgetCapacity);
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                        widget = &definition->widgets[definition->widgetCount];
                        (void)memset(widget, 0, sizeof((*widget)));
                        widget->layerIndex        = definition->layerCount - 1;
                        widget->parentWidgetIndex = UINT32_MAX;
                        if (current_node->parent != layer_node)
                        {
                            for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
                            {
                                if (xml.widgetNodes[widget_index] == current_node->parent)
                                {
                                    widget->parentWidgetIndex = widget_index;
                                    break;
                                }
                            }
                            is_valid = (UINT32_MAX != widget->parentWidgetIndex);
                        }
                        if (is_valid)
                        {
                            is_valid = CopyAttribute(current_node, ARINC661_XML_ATTRIBUTE_NAME, widget->name, sizeof(widget->name));
                        }
                        if (is_valid)
                        {
                            is_valid = CopyAttribute(current_node, ARINC661_XML_ATTRIBUTE_TYPE, widget->type, sizeof(widget->type));
                        }
                        if (is_valid)
                        {
                            model_node = FindChild(current_node, ARINC661_DF_ELEMENT_MODEL);
                            is_valid   = ReadProperties(definition, model_node, UTL_ARINC661_PROPERTY_WIDGET, definition->widgetCount);
                        }
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                        xml.widgetNodes[definition->widgetCount] = current_node;
                        definition->widgetCount += 1;
                    }
                    else
                    {
                        is_model    = HasElementName(current_node, ARINC661_DF_ELEMENT_MODEL);
                        is_property = HasElementName(current_node, ARINC661_DF_ELEMENT_PROPERTY);
                        is_valid    = is_model || is_property;
                        if (FALSE == is_valid)
                        {
                            break;
                        }
                    }
                    current_node = NextSubtreeNode(layer_node, current_node);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                LOGASSERT(NULL == current_node);
            }
            else
            {
                is_valid = HasElementName(layer_node, ARINC661_DF_ELEMENT_MODEL);
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
        definition->layerCount    = 0;
        definition->widgetCount   = 0;
        definition->propertyCount = 0;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds an XML Definition File from separate layer, widget, and property values.
 */
uint64_t Utl_Arinc661_WriteDefinition(const Utl_Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t               xml;
    Utl_Xml_Node_t*              declaration;
    Utl_Xml_Node_t*              root;
    Utl_Xml_Node_t*              layer_node;
    Utl_Xml_Node_t*              widget_node;
    Utl_Xml_Node_t*              parent_node;
    const Utl_Arinc661_Layer_t*  layer;
    const Utl_Arinc661_Widget_t* widget;
    uint64_t                     byte_count;
    uint32_t                     layer_index;
    uint32_t                     widget_index;
    bool_t                       has_workspace;
    bool_t                       has_parent;
    bool_t                       is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    LOGASSERT(NULL != workspace);
    byte_count    = 0;
    has_workspace = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid      = has_workspace;
    if (is_valid)
    {
        declaration = &xml.nodes[xml.nodeCount];
        Utl_Xml_InitializeNode(declaration, UTL_XML_NODE_XML_DECLARATION, NULL, 0);
        xml.nodeCount += 1;
        Utl_Xml_AppendNode(&xml.document, declaration);
        root     = AppendElement(&xml, ARINC661_DF_ELEMENT_ROOT);
        is_valid = (NULL != root);
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(&xml, root, ARINC661_XML_ATTRIBUTE_NAME, definition->name);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(&xml, root, ARINC661_XML_ATTRIBUTE_LIBRARY_VERSION, definition->libraryVersion);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(&xml, root, ARINC661_XML_ATTRIBUTE_SUPPLEMENT_VERSION, definition->supplementVersion);
    }
    if (is_valid)
    {
        is_valid = WriteProperties(&xml, root, definition, UTL_ARINC661_PROPERTY_DEFINITION, 0);
    }
    if (is_valid)
    {
        for (layer_index = 0; layer_index < definition->layerCount; ++layer_index)
        {
            layer      = &definition->layers[layer_index];
            layer_node = AppendElement(&xml, ARINC661_DF_ELEMENT_LAYER);
            is_valid   = (NULL != layer_node);
            if (is_valid)
            {
                is_valid = AppendAttribute(&xml, layer_node, ARINC661_XML_ATTRIBUTE_NAME, layer->name);
            }
            if (is_valid)
            {
                is_valid = WriteProperties(&xml, layer_node, definition, UTL_ARINC661_PROPERTY_LAYER, layer_index);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
            {
                widget = &definition->widgets[widget_index];
                if (layer_index == widget->layerIndex)
                {
                    widget_node = AppendElement(&xml, ARINC661_DF_ELEMENT_WIDGET);
                    is_valid    = (NULL != widget_node);
                    if (is_valid)
                    {
                        is_valid = AppendAttribute(&xml, widget_node, ARINC661_XML_ATTRIBUTE_NAME, widget->name);
                    }
                    if (is_valid)
                    {
                        is_valid = AppendAttribute(&xml, widget_node, ARINC661_XML_ATTRIBUTE_TYPE, widget->type);
                    }
                    if (is_valid)
                    {
                        is_valid = WriteProperties(&xml, widget_node, definition, UTL_ARINC661_PROPERTY_WIDGET, widget_index);
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    parent_node = layer_node;
                    if (UINT32_MAX != widget->parentWidgetIndex)
                    {
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
                    Utl_Xml_AppendChild(parent_node, widget_node);
                    xml.widgetNodes[widget_index] = widget_node;
                }
            }
            if (FALSE == is_valid)
            {
                break;
            }
            Utl_Xml_AppendChild(root, layer_node);
        }
    }
    if (is_valid)
    {
        Utl_Xml_AppendNode(&xml.document, root);
        byte_count = Utl_Xml_WriteDocument(&xml.document, buffer, capacity);
    }

    return byte_count;
}
