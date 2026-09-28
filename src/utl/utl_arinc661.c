/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6, Cockpit Display System Interfaces to User Systems:
 * https://saemobilus.sae.org/standards/arinc661-6-cockpit-display-system-interfaces-user-systems
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd):
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 * FACE Technical Standard, Edition 3.1, Appendix H.3, DisplayManagement.xsd:
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

#define XML_INTEGER_TEXT_BYTES (32UL)    ///< Integer formatting buffer bytes, including the zero terminator
#define XML_DECIMAL_TEXT_BYTES (96UL)    ///< Fixed-point formatting buffer bytes, including the zero terminator

/* FACE Edition 3.1, Appendix H.3, DisplayManagement.xsd: configuration and child elements. */
#define DMC_ELEMENT_CONFIGURATION       ("configuration")
#define DMC_ELEMENT_METADATA            ("metaData")
#define DMC_ELEMENT_SCREEN              ("screen")
#define DMC_ELEMENT_PIXEL_SIZE          ("pixelSize")
#define DMC_ELEMENT_PHYSICAL_DIMENSIONS ("physicalDimensions")
#define DMC_ELEMENT_LAYOUT              ("layout")
#define DMC_ELEMENT_WINDOW              ("window")
#define DMC_ELEMENT_DESCRIPTION         ("description")
#define DMC_ELEMENT_PIXEL_AREA          ("pixelArea")
#define DMC_ELEMENT_SCALING_FACTOR      ("scalingFactor")
#define DMC_ELEMENT_USER_APPLICATION    ("ua")
#define DMC_ELEMENT_EXTERNAL_SOURCE     ("externalSource")

/* Configuration schema hints are retained as XML attributes, without loading the schema. */
#define XML_ATTRIBUTE_XMLNS_XSI       ("xmlns:xsi")
#define XML_ATTRIBUTE_SCHEMA_LOCATION ("xsi:noNamespaceSchemaLocation")
#define XML_SCHEMA_INSTANCE_NAMESPACE ("http://www.w3.org/2001/XMLSchema-instance")
#define DMC_DEFAULT_SCHEMA_LOCATION   ("H3.xsd")

/* Supplement 6 a661Strict.xsd: dfType, layerType, widgetType, modelType, and propType. */
#define DF_ELEMENT_ROOT     ("a661_df")
#define DF_ELEMENT_LAYER    ("a661_layer")
#define DF_ELEMENT_WIDGET   ("a661_widget")
#define DF_ELEMENT_MODEL    ("model")
#define DF_ELEMENT_PROPERTY ("prop")

/* FACE Edition 3.1, Appendix H.3, and Supplement 6 a661Strict.xsd attribute names. */
#define XML_ATTRIBUTE_ID                 ("id")
#define XML_ATTRIBUTE_NAME               ("name")
#define XML_ATTRIBUTE_WIDTH              ("width")
#define XML_ATTRIBUTE_HEIGHT             ("height")
#define XML_ATTRIBUTE_UNITS              ("units")
#define XML_ATTRIBUTE_X                  ("x")
#define XML_ATTRIBUTE_Y                  ("y")
#define XML_ATTRIBUTE_X_SCALE            ("xScale")
#define XML_ATTRIBUTE_Y_SCALE            ("yScale")
#define XML_ATTRIBUTE_BASE_UNIT          ("baseUnit")
#define XML_ATTRIBUTE_PER_UNIT           ("perUnit")
#define XML_ATTRIBUTE_APPLICATION_ID     ("applicationId")
#define XML_ATTRIBUTE_DF_PATH            ("dFPath")
#define XML_ATTRIBUTE_STYLE_FILE_PATH    ("styleFilePath")
#define XML_ATTRIBUTE_MAP_SOURCE_LAYER   ("mapSourceLayer")
#define XML_ATTRIBUTE_WINDOW             ("window")
#define XML_ATTRIBUTE_VISIBLE            ("visible")
#define XML_ATTRIBUTE_TYPE               ("type")
#define XML_ATTRIBUTE_VALUE              ("value")
#define XML_ATTRIBUTE_LIBRARY_VERSION    ("library_version")
#define XML_ATTRIBUTE_SUPPLEMENT_VERSION ("supp_version")

/* FACE Edition 3.1, Appendix H.3, UserApplication mapSourceLayer and visible (xs:boolean). */
#define XML_BOOLEAN_TRUE  ("true")
#define XML_BOOLEAN_FALSE ("false")
#define XML_BOOLEAN_ONE   ("1")
#define XML_BOOLEAN_ZERO  ("0")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Temporary XML data owned by the caller's opaque workspace.
 */
typedef struct XmlWorkspace_s
{
    Utl_Xml_Node_t*      nodes;          ///< Pointer to temporary XML node storage
    Utl_Xml_Attribute_t* attributes;     ///< Pointer to temporary XML attribute storage
    Utl_Xml_Node_t**     widgetNodes;    ///< Pointer to the mapping from widget indices to XML nodes
    byte_t*              text;           ///< Pointer to formatted attribute value storage
    uint64_t             textCapacity;   ///< Available text bytes after the fixed workspace arrays
    uint64_t             textByteCount;  ///< Text bytes used, including each string's zero terminator
    uint32_t             nodeCount;      ///< XML node slots used while constructing output
    uint32_t             attributeCount; ///< XML attribute slots used while constructing output
    Utl_Xml_Document_t   document;       ///< Temporary document with nodes in the workspace
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
 * \brief Reads the configuration's optional no-namespace schema hint.
 *
 * \param[in]  root            Configuration root element
 * \param[out] configuration   Configuration receiving a copied schema location
 *
 * \retval     TRUE            if the hint is absent or has the expected namespace and fits.
 * \retval     FALSE           if the namespace binding or location is invalid.
 */
static bool_t ReadConfigurationSchema(const Utl_Xml_Node_t* root, Utl_Arinc661_Configuration_t* configuration);

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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Units simpleType and Size/Scale defaults.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, UserApplication mapSourceLayer and visible.
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
 * \brief Appends the schema-instance namespace and configuration schema location.
 *
 * \param[in,out] xml             Temporary XML storage
 * \param[in,out] root            Configuration root element
 * \param[in]     configuration   Configuration containing the schema location
 *
 * \retval        TRUE            if both root attributes were appended.
 * \retval        FALSE           if a value or workspace storage is invalid.
 */
static bool_t WriteConfigurationSchema(XmlWorkspace_t* xml, Utl_Xml_Node_t* root, const Utl_Arinc661_Configuration_t* configuration);

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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Size complexType; sections H.3.4 and H.3.5.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Rectangle complexType (Window.pixelArea).
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Scale complexType (Window.scalingFactor).
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Screen and Window types; sections H.3.2 through H.3.6.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, UserApplication type and section H.3.1.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, ExternalSource type and section H.3.7.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Units simpleType.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Size complexType.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Rectangle complexType (Window.pixelArea).
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Scale complexType (Window.scalingFactor).
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, UserApplication type and section H.3.1.
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
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, ExternalSource type and section H.3.7.
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
 * \par Details:
 *  Supplement 6 a661Strict.xsd, modelType/propType.
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
 * \par Details:
 *  Supplement 6 a661Strict.xsd, modelType/propType.
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
    node_bytes      = (uint64_t)UTL_ARINC661_XML_NODE_CAPACITY * sizeof(Utl_Xml_Node_t);
    attribute_bytes = (uint64_t)UTL_ARINC661_XML_ATTRIBUTE_CAPACITY * sizeof(Utl_Xml_Attribute_t);
    widget_bytes    = (uint64_t)UTL_ARINC661_XML_WIDGET_CAPACITY * sizeof(Utl_Xml_Node_t*);
    reserved_bytes  = ((node_bytes + attribute_bytes) + widget_bytes);
    is_aligned      = (0 == ((uintptr_t)bytes % sizeof(uint64_t)));
    has_capacity    = (reserved_bytes < byteCount);
    if (is_aligned && has_capacity)
    {
        // Each region points into the supplied workspace; no separate allocation is made
        xml->nodes          = (Utl_Xml_Node_t*)bytes;
        xml->attributes     = (Utl_Xml_Attribute_t*)((byte_t*)bytes + node_bytes);
        xml->widgetNodes    = (Utl_Xml_Node_t**)((byte_t*)bytes + (node_bytes + attribute_bytes));
        xml->text           = (byte_t*)bytes + reserved_bytes;
        xml->textCapacity   = byteCount - reserved_bytes;
        // Reset usage and document links without clearing the entire workspace
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
static bool_t HasElementName(const Utl_Xml_Node_t* node, const char_t* name)
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
        is_element = (UTL_XML_NODE_ELEMENT == node->type);
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
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* found_node;
    uint32_t        node_index;

    node       = parent->firstChild;
    found_node = NULL;
    for (node_index = 0; node_index < UTL_ARINC661_XML_NODE_CAPACITY; ++node_index)
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
 *  Compares a counted attribute value with a complete, case-sensitive literal:
 *
 * \verbatim
 * Attribute bytes   Literal   Result
 * pixel             pixel     TRUE
 * pixel             pix       FALSE
 * Pixel             pixel     FALSE
 * \endverbatim
 *
 *  The attribute's valueByteCount supplies its length, so its value does not need
 *  a zero terminator. Only the expected literal is measured with strlen.
 */
static bool_t HasAttributeValue(const Utl_Xml_Attribute_t* attribute, const char_t* value)
{
    uint32_t byte_count;
    bool_t   has_length;
    bool_t   has_value;

    byte_count = (uint32_t)strlen(value);
    // Reject unequal lengths before comparing the value bytes
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
static bool_t CopyAttribute(const Utl_Xml_Node_t* node, const char_t* name, char_t* destination, uint32_t capacity)
{
    const Utl_Xml_Attribute_t* attribute;
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
 *  Copies the schema hint from the historical configuration root form:
 *
 * \code{.xml}
 * <configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *                xsi:noNamespaceSchemaLocation="H3.xsd"/>
 * \endcode
 *
 *  The xsi prefix must have the schema-instance namespace binding when a
 *  location is supplied. A missing location leaves schemaLocation empty,
 *  which selects H3.xsd when the configuration is written. The hint is copied
 *  into caller-owned storage; no schema file is opened or validated.
 */
static bool_t ReadConfigurationSchema(const Utl_Xml_Node_t* root, Utl_Arinc661_Configuration_t* configuration)
{
    const Utl_Xml_Attribute_t* location;
    const Utl_Xml_Attribute_t* namespace_attribute;
    bool_t                    is_valid;

    configuration->schemaLocation[0] = '\0';
    location = FindAttribute(root, XML_ATTRIBUTE_SCHEMA_LOCATION);
    is_valid = TRUE;
    if (NULL != location)
    {
        // Require the namespace binding that gives the xsi attribute its meaning
        namespace_attribute = FindAttribute(root, XML_ATTRIBUTE_XMLNS_XSI);
        is_valid = (NULL != namespace_attribute);
        if (is_valid)
        {
            is_valid = HasAttributeValue(namespace_attribute, XML_SCHEMA_INSTANCE_NAMESPACE);
        }
        if (is_valid)
        {
            // Copy the hint so saving does not depend on the original XML buffer
            is_valid = CopyAttribute(root, XML_ATTRIBUTE_SCHEMA_LOCATION, configuration->schemaLocation, sizeof(configuration->schemaLocation));
        }
    }

    return is_valid;
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
static bool_t ReadInteger(const Utl_Xml_Node_t* node, const char_t* name, int64_t* value)
{
    const Utl_Xml_Attribute_t* attribute;
    int64_t                    parsed_value;
    int32_t                    consumed;
    bool_t                     is_valid;

    attribute = FindAttribute(node, name);
    // The numeric attribute is required
    is_valid  = (NULL != attribute);
    if (is_valid)
    {
        consumed = Utl_Scan_Int64((const char_t*)attribute->value, &parsed_value);
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
 *  Reads a required numeric attribute and converts it to float32_t. For example:
 *
 * \code{.xml}
 * <scalingFactor xScale="1.25" yScale="0.5"/>
 * \endcode
 *
 *  Reading "xScale" stores 1.25. The scan must consume the complete counted value,
 *  and the parsed number must lie between -FLT_MAX and FLT_MAX before conversion.
 *  Missing, partially scanned, or out-of-range values return FALSE without
 *  changing the output.
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
        // Reject values with an unconsumed suffix
        is_valid = (0 < consumed) && ((uint32_t)consumed == attribute->valueByteCount);
        if (is_valid)
        {
            // Check the float32_t range before narrowing the parsed value
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
 *  Converts an optional unit attribute to its enum value. For example:
 *
 * \code{.xml}
 * <physicalDimensions width="20" height="8" units="inch"/>
 * \endcode
 *
 *  The supported spellings are "inch", "mm", "screen", and "pixel". An absent
 *  attribute succeeds with defaultUnit. An unrecognized spelling returns FALSE
 *  and leaves the output at defaultUnit; matching is case-sensitive.
 */
static bool_t ReadUnit(const Utl_Xml_Node_t* node, const char_t* name, Utl_Arinc661_Unit_t defaultUnit, Utl_Arinc661_Unit_t* unit)
{
    const Utl_Xml_Attribute_t* attribute;
    bool_t                     is_valid;

    attribute = FindAttribute(node, name);
    // Set the caller's default before looking for an explicit supported spelling
    (*unit) = defaultUnit;
    is_valid = TRUE;
    if (NULL != attribute)
    {
        // A present attribute must match one of the four unit literals
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
 *  Reads an optional boolean attribute using these exact spellings:
 *
 * \verbatim
 * Attribute value   Output   Return
 * true or 1         TRUE     TRUE
 * false or 0        FALSE    TRUE
 * absent            FALSE    TRUE
 * other spelling    FALSE    FALSE
 * \endverbatim
 *
 *  For example, visible="1" enables visibility. The output is initialized to
 *  FALSE before looking up the attribute, so failure does not retain an old value.
 */
static bool_t ReadBoolean(const Utl_Xml_Node_t* node, const char_t* name, bool_t* value)
{
    const Utl_Xml_Attribute_t* attribute;
    bool_t                     is_true;
    bool_t                     is_one;
    bool_t                     is_false;
    bool_t                     is_zero;
    bool_t                     is_valid;

    // An absent boolean attribute uses FALSE
    (*value) = FALSE;
    attribute = FindAttribute(node, name);
    is_valid  = TRUE;
    if (NULL != attribute)
    {
        is_valid = FALSE;
        // Recognize both the word and numeric forms without changing case
        is_true  = HasAttributeValue(attribute, XML_BOOLEAN_TRUE);
        is_one   = HasAttributeValue(attribute, XML_BOOLEAN_ONE);
        is_false = HasAttributeValue(attribute, XML_BOOLEAN_FALSE);
        is_zero  = HasAttributeValue(attribute, XML_BOOLEAN_ZERO);
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
static Utl_Xml_Node_t* AppendElement(XmlWorkspace_t* xml, const char_t* name)
{
    Utl_Xml_Node_t* node;

    node = NULL;
    // Initialize a node only when a workspace slot is available
    if (xml->nodeCount < UTL_ARINC661_XML_NODE_CAPACITY)
    {
        node = &xml->nodes[xml->nodeCount];
        Utl_Xml_InitializeNode(node, UTL_XML_NODE_ELEMENT, (const byte_t*)name, (uint32_t)strlen(name));
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
static bool_t AppendAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* attribute;
    uint32_t             byte_count;
    bool_t               has_storage;
    bool_t               has_value;
    bool_t               is_valid;

    // A formatter may return NULL, and the shared attribute array may be full
    has_storage = (xml->attributeCount < UTL_ARINC661_XML_ATTRIBUTE_CAPACITY);
    has_value   = (NULL != value);
    is_valid    = FALSE;
    if (has_storage && has_value)
    {
        byte_count = (uint32_t)strlen(value);
        // Validate literal value bytes before the XML append function asserts its preconditions
        is_valid   = Utl_Xml_IsAttributeValue((const byte_t*)value, byte_count);
        if (is_valid)
        {
            attribute = &xml->attributes[xml->attributeCount];
            Utl_Xml_InitializeAttribute(attribute, (const byte_t*)name, (uint32_t)strlen(name), (const byte_t*)value, byte_count);
            Utl_Xml_AppendAttribute(node, attribute);
            // Count the attribute only after it has been linked
            xml->attributeCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds the schema hint used by configuration XML files. With an empty
 *  schemaLocation field, the root starts with these attributes:
 *
 * \code{.xml}
 * <configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="H3.xsd"/>
 * \endcode
 *
 *  A nonempty field replaces H3.xsd, allowing a location read from a file to
 *  survive serialization. The namespace attribute precedes its qualified
 *  schema-location attribute. Failure leaves any already appended attributes
 *  in the temporary workspace and prevents document serialization.
 */
static bool_t WriteConfigurationSchema(XmlWorkspace_t* xml, Utl_Xml_Node_t* root, const Utl_Arinc661_Configuration_t* configuration)
{
    const char_t* location;
    bool_t        is_valid;

    // Configurations without an explicit hint use the historical schema file name
    location = DMC_DEFAULT_SCHEMA_LOCATION;
    if ('\0' != configuration->schemaLocation[0])
    {
        location = configuration->schemaLocation;
    }

    // Bind xsi before writing the attribute that uses it
    is_valid = AppendAttribute(xml, root, XML_ATTRIBUTE_XMLNS_XSI, XML_SCHEMA_INSTANCE_NAMESPACE);
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, root, XML_ATTRIBUTE_SCHEMA_LOCATION, location);
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
 *  Formats a finite float32_t without an exponent, then removes unused fractional
 *  zeros and a trailing decimal point:
 *
 * \verbatim
 * value       Initial formatted text         Stored text
 * 1.25        1.25000... (54 fraction digits)  1.25
 * 2.0         2.00000... (54 fraction digits)  2
 * \endverbatim
 *
 *  Fifty-four fractional digits retain very small float32_t values. The returned
 *  pointer refers to the workspace, and textByteCount includes the terminating
 *  zero. A nonfinite value or insufficient formatting space returns NULL without
 *  advancing textByteCount.
 */
static const char_t* FormatDecimal(XmlWorkspace_t* xml, float32_t value)
{
    char_t*  text;
    int      byte_count;
    uint32_t byte_index;
    bool_t   is_finite;
    bool_t   has_capacity;

    text         = NULL;
    // Reject nonfinite values and reserve space for the full fixed-point representation
    is_finite    = (value <= FLT_MAX) && (value >= -FLT_MAX);
    has_capacity = (XML_DECIMAL_TEXT_BYTES <= (xml->textCapacity - xml->textByteCount));
    if (is_finite && has_capacity)
    {
        text       = (char_t*)&xml->text[xml->textByteCount];
        byte_count = snprintf(text, XML_DECIMAL_TEXT_BYTES, "%.54f", value);
        LOGASSERT((0 < byte_count) && ((uint32_t)byte_count < XML_DECIMAL_TEXT_BYTES));
        // Bound the backward scan and stop at the first nonzero fractional byte
        for (byte_index = 0; byte_index < XML_DECIMAL_TEXT_BYTES; ++byte_index)
        {
            if ('0' != text[byte_count - 1])
            {
                break;
            }
            byte_count -= 1;
        }
        // An integral value does not need the now-empty fractional separator
        if ('.' == text[byte_count - 1])
        {
            byte_count -= 1;
        }
        // Terminate the shortened text and retain only the bytes actually used
        text[byte_count] = '\0';
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
static bool_t AppendIntegerAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, int64_t value)
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
 *  Formats a float32_t as fixed-point text, then appends the resulting attribute.
 *  For example, name="xScale" and value=1.25 add xScale="1.25" to the element.
 *  The attribute points into the workspace text area.
 *
 *  Formatting or appending failure returns FALSE. A successful format may consume
 *  workspace text even if the attribute cannot subsequently be appended.
 */
static bool_t AppendDecimalAttribute(XmlWorkspace_t* xml, Utl_Xml_Node_t* node, const char_t* name, float32_t value)
{
    const char_t* text;
    bool_t        is_appended;

    // Convert to a directly writable decimal value before linking the attribute
    text        = FormatDecimal(xml, value);
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
static Utl_Xml_Node_t* FindDocumentElement(const Utl_Xml_Document_t* document)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* element;
    uint32_t        node_index;

    node    = document->firstNode;
    element = NULL;
    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_ROOT_NODES; ++node_index)
    {
        // Stop at the end of the top-level list
        if (NULL == node)
        {
            break;
        }
        // The declaration is skipped; the document element is the result
        if (UTL_XML_NODE_ELEMENT == node->type)
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
 *  Reads the dimensions and optional unit from a size element, for example:
 *
 * \code{.xml}
 * <pixelSize width="1280" height="720" units="pixel"/>
 * \endcode
 *
 *  This stores width=1280, height=720, and units=UTL_ARINC661_UNIT_PIXEL.
 *  Width and height are required; units defaults to pixel when absent.
 *  A missing node or invalid attribute returns FALSE. Fields already read remain
 *  updated if a later field fails.
 */
static bool_t ReadSize(const Utl_Xml_Node_t* node, Utl_Arinc661_Size_t* size)
{
    bool_t is_valid;

    // The size child must exist before reading its required dimensions
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = ReadInteger(node, XML_ATTRIBUTE_WIDTH, &size->width);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, XML_ATTRIBUTE_HEIGHT, &size->height);
    }
    if (is_valid)
    {
        // Use pixel units when the optional units attribute is absent
        is_valid = ReadUnit(node, XML_ATTRIBUTE_UNITS, UTL_ARINC661_UNIT_PIXEL, &size->units);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads all four required integer attributes of a pixel rectangle:
 *
 * \code{.xml}
 * <pixelArea x="10" y="20" width="640" height="480"/>
 * \endcode
 *
 *  The example stores origin (10,20) and dimensions (640,480). This helper
 *  converts the values without imposing a positivity constraint. A missing node
 *  or invalid field returns FALSE; fields read before the failure remain updated.
 */
static bool_t ReadRectangle(const Utl_Xml_Node_t* node, Utl_Arinc661_Rectangle_t* rectangle)
{
    bool_t is_valid;

    // Require the pixelArea child before reading its origin and dimensions
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = ReadInteger(node, XML_ATTRIBUTE_X, &rectangle->x);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, XML_ATTRIBUTE_Y, &rectangle->y);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, XML_ATTRIBUTE_WIDTH, &rectangle->width);
    }
    if (is_valid)
    {
        is_valid = ReadInteger(node, XML_ATTRIBUTE_HEIGHT, &rectangle->height);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads two required scale factors and two optional units, for example:
 *
 * \code{.xml}
 * <scalingFactor xScale="1.25" yScale="0.5"/>
 * \endcode
 *
 *  The example stores xScale=1.25 and yScale=0.5, with baseUnit defaulting to
 *  screen and perUnit to pixel. Explicit unit attributes override those defaults.
 *  Failure returns FALSE without undoing fields that were already read.
 */
static bool_t ReadScale(const Utl_Xml_Node_t* node, Utl_Arinc661_Scale_t* scale)
{
    bool_t is_valid;

    // Require the scalingFactor child and read both numeric factors
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = ReadDecimal(node, XML_ATTRIBUTE_X_SCALE, &scale->xScale);
    }
    if (is_valid)
    {
        is_valid = ReadDecimal(node, XML_ATTRIBUTE_Y_SCALE, &scale->yScale);
    }
    if (is_valid)
    {
        // Apply the screen-to-pixel defaults when unit attributes are absent
        is_valid = ReadUnit(node, XML_ATTRIBUTE_BASE_UNIT, UTL_ARINC661_UNIT_SCREEN, &scale->baseUnit);
    }
    if (is_valid)
    {
        is_valid = ReadUnit(node, XML_ATTRIBUTE_PER_UNIT, UTL_ARINC661_UNIT_PIXEL, &scale->perUnit);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Converts a screen subtree into separate screen, layout, and window arrays.
 *  A complete supported screen can look like this:
 *
 * \code{.xml}
 * <screen id="2">
 *     <pixelSize width="1280" height="720"/>
 *     <physicalDimensions width="12" height="7" units="inch"/>
 *     <layout id="3">
 *         <window id="7" name="Main">
 *             <description/>
 *             <pixelArea x="0" y="0" width="1280" height="720"/>
 *             <scalingFactor xScale="1" yScale="1"/>
 *         </window>
 *     </layout>
 * </screen>
 * \endcode
 *
 *  If the arrays are initially empty, the links become:
 *
 * \verbatim
 * screens[0].id         = 2
 * layouts[0].id         = 3    layouts[0].screenIndex = 0
 * windows[0].id         = 7    windows[0].layoutIndex = 0
 * \endverbatim
 *
 *  The indices refer to array entries, not the XML id values. Both screen sizes
 *  and each window's rectangle and scale are required. Unknown screen, layout,
 *  or window-detail children are rejected. Description text is unsupported by
 *  the XML reader, so window descriptions remain empty.
 *
 *  Counts advance as entries are accepted. Failure may leave partial entries
 *  and counts; the public configuration reader clears the result counts.
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

    // Reserve the screen's array index before reading any dependent layouts
    is_valid = (configuration->screenCount < configuration->screenCapacity);
    if (is_valid)
    {
        // Keep this array index distinct from the screen's XML id
        screen_array_index = configuration->screenCount;
        screen             = &configuration->screens[screen_array_index];
        (void)memset(screen, 0, sizeof((*screen)));
        is_valid = ReadInteger(element, XML_ATTRIBUTE_ID, &screen->id);
        if (is_valid)
        {
            detail_node = FindChild(element, DMC_ELEMENT_PIXEL_SIZE);
            is_valid    = ReadSize(detail_node, &screen->pixelSize);
        }
        if (is_valid)
        {
            detail_node = FindChild(element, DMC_ELEMENT_PHYSICAL_DIMENSIONS);
            is_valid    = ReadSize(detail_node, &screen->physicalDimensions);
        }
        if (is_valid)
        {
            // Publish the screen before collecting layouts that refer to its index
            configuration->screenCount += 1;
            layout_node = element->firstChild;
            for (screen_index = 0; screen_index < UTL_ARINC661_XML_NODE_CAPACITY; ++screen_index)
            {
                if (NULL == layout_node)
                {
                    break;
                }
                if (HasElementName(layout_node, DMC_ELEMENT_LAYOUT))
                {
                    is_valid = (configuration->layoutCount < configuration->layoutCapacity);
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    // Each layout records the array index of its owning screen
                    layout_array_index = configuration->layoutCount;
                    layout             = &configuration->layouts[layout_array_index];
                    (void)memset(layout, 0, sizeof((*layout)));
                    layout->screenIndex = screen_array_index;
                    is_valid            = ReadInteger(layout_node, XML_ATTRIBUTE_ID, &layout->id);
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    configuration->layoutCount += 1;
                    window_node = layout_node->firstChild;
                    for (layout_index = 0; layout_index < UTL_ARINC661_XML_NODE_CAPACITY; ++layout_index)
                    {
                        if (NULL == window_node)
                        {
                            break;
                        }
                        if (HasElementName(window_node, DMC_ELEMENT_WINDOW))
                        {
                            is_valid = (configuration->windowCount < configuration->windowCapacity);
                            if (FALSE == is_valid)
                            {
                                break;
                            }
                            // Each window records the array index of its owning layout
                            window = &configuration->windows[configuration->windowCount];
                            (void)memset(window, 0, sizeof((*window)));
                            window->layoutIndex = layout_array_index;
                            is_valid            = ReadInteger(window_node, XML_ATTRIBUTE_ID, &window->id);
                            if (is_valid)
                            {
                                is_valid = CopyAttribute(window_node, XML_ATTRIBUTE_NAME, window->name, sizeof(window->name));
                            }
                            if (is_valid)
                            {
                                detail_node = FindChild(window_node, DMC_ELEMENT_PIXEL_AREA);
                                is_valid    = ReadRectangle(detail_node, &window->pixelArea);
                            }
                            if (is_valid)
                            {
                                detail_node = FindChild(window_node, DMC_ELEMENT_SCALING_FACTOR);
                                is_valid    = ReadScale(detail_node, &window->scalingFactor);
                            }
                            if (is_valid)
                            {
                                // Reject unsupported window children after reading the required details
                                detail_node = window_node->firstChild;
                                for (detail_index = 0; detail_index < UTL_ARINC661_XML_NODE_CAPACITY; ++detail_index)
                                {
                                    if (NULL == detail_node)
                                    {
                                        break;
                                    }
                                    is_description    = HasElementName(detail_node, DMC_ELEMENT_DESCRIPTION);
                                    is_pixel_area     = HasElementName(detail_node, DMC_ELEMENT_PIXEL_AREA);
                                    is_scaling_factor = HasElementName(detail_node, DMC_ELEMENT_SCALING_FACTOR);
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
                            // Count the window only after all of its supported details have been checked
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
                    // The screen's size children were already read; other non-layout children are invalid
                    is_pixel_size          = HasElementName(layout_node, DMC_ELEMENT_PIXEL_SIZE);
                    is_physical_dimensions = HasElementName(layout_node, DMC_ELEMENT_PHYSICAL_DIMENSIONS);
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
 *  Copies one user application into the next available result slot:
 *
 * \code{.xml}
 * <ua applicationId="1" dFPath="display.xml" window="7" visible="true"/>
 * \endcode
 *
 *  applicationId, window, and the dFPath attribute must be present. A missing
 *  styleFilePath becomes an empty string, and missing mapSourceLayer or visible
 *  attributes become FALSE. Paths are copied into the result arrays with zero
 *  terminators. userApplicationCount advances only when every value fits and
 *  parses successfully; the unused slot may contain partial data on failure.
 */
static bool_t ReadUserApplication(Utl_Arinc661_Configuration_t* configuration, const Utl_Xml_Node_t* node)
{
    Utl_Arinc661_UserApplication_t* application;
    const Utl_Xml_Attribute_t*      df_path;
    bool_t                          is_valid;

    // Check caller-provided storage before initializing the next application
    is_valid = (configuration->userApplicationCount < configuration->userApplicationCapacity);
    if (is_valid)
    {
        application = &configuration->userApplications[configuration->userApplicationCount];
        (void)memset(application, 0, sizeof((*application)));
        is_valid = ReadInteger(node, XML_ATTRIBUTE_APPLICATION_ID, &application->applicationId);
        if (is_valid)
        {
            is_valid = ReadInteger(node, XML_ATTRIBUTE_WINDOW, &application->window);
        }
        if (is_valid)
        {
            // Require dFPath even though the generic string-copy helper accepts absent attributes
            df_path  = FindAttribute(node, XML_ATTRIBUTE_DF_PATH);
            is_valid = (NULL != df_path);
        }
        if (is_valid)
        {
            is_valid = CopyAttribute(node, XML_ATTRIBUTE_DF_PATH, application->dFPath, sizeof(application->dFPath));
        }
        if (is_valid)
        {
            is_valid = CopyAttribute(node, XML_ATTRIBUTE_STYLE_FILE_PATH, application->styleFilePath, sizeof(application->styleFilePath));
        }
        if (is_valid)
        {
            is_valid = ReadBoolean(node, XML_ATTRIBUTE_MAP_SOURCE_LAYER, &application->mapSourceLayer);
        }
        if (is_valid)
        {
            is_valid = ReadBoolean(node, XML_ATTRIBUTE_VISIBLE, &application->visible);
        }
        if (is_valid)
        {
            // Publish the application only after paths and boolean settings are valid
            configuration->userApplicationCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads an external source with an identifier and optional name and type:
 *
 * \code{.xml}
 * <externalSource id="12" name="Camera" type="video"/>
 * \endcode
 *
 *  The id is required and must fit the range 0 through UINT16_MAX. Missing name
 *  and type attributes become empty strings. Child elements are unsupported.
 *  externalSourceCount advances only after all checks succeed; a failed read may
 *  have written part of the unused result slot.
 */
static bool_t ReadExternalSource(Utl_Arinc661_Configuration_t* configuration, const Utl_Xml_Node_t* node)
{
    Utl_Arinc661_ExternalSource_t* source;
    int64_t                        id;
    bool_t                         is_valid;

    // Check storage before initializing the source entry
    is_valid = (configuration->externalSourceCount < configuration->externalSourceCapacity);
    if (is_valid)
    {
        source = &configuration->externalSources[configuration->externalSourceCount];
        (void)memset(source, 0, sizeof((*source)));
        is_valid = ReadInteger(node, XML_ATTRIBUTE_ID, &id);
        if (is_valid)
        {
            // Validate both bounds before narrowing the parsed identifier
            is_valid = (0 <= id);
        }
        if (is_valid)
        {
            is_valid = (UINT16_MAX >= id);
        }
        if (is_valid)
        {
            source->id = (uint32_t)id;
            is_valid   = CopyAttribute(node, XML_ATTRIBUTE_NAME, source->name, sizeof(source->name));
        }
        if (is_valid)
        {
            is_valid = CopyAttribute(node, XML_ATTRIBUTE_TYPE, source->type, sizeof(source->type));
        }
        if (is_valid)
        {
            // Only the externalSource attributes are supported
            is_valid = (NULL == node->firstChild);
        }
        if (is_valid)
        {
            // Publish the source after validating its values and absence of children
            configuration->externalSourceCount += 1;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Maps a unit enum to the XML attribute literal used by the writers:
 *
 * \verbatim
 * UTL_ARINC661_UNIT_INCH    -> "inch"
 * UTL_ARINC661_UNIT_MM      -> "mm"
 * UTL_ARINC661_UNIT_SCREEN  -> "screen"
 * UTL_ARINC661_UNIT_PIXEL   -> "pixel"
 * \endverbatim
 *
 *  The returned pointer refers to a constant string. An unsupported enum value
 *  returns NULL, causing the subsequent attribute append to fail.
 */
static const char_t* UnitName(Utl_Arinc661_Unit_t unit)
{
    const char_t* name;

    // Leave unsupported enum values unmapped
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
 *  Creates and appends a size child using the supplied element name. For example,
 *  name="physicalDimensions", width=20, height=8, and
 *  units=UTL_ARINC661_UNIT_INCH produce:
 *
 * \code{.xml}
 * <physicalDimensions width="20" height="8" units="inch"/>
 * \endcode
 *
 *  The unit attribute is always explicit. The node is linked to parent only
 *  after both integer attributes and the unit are appended. Failure returns FALSE
 *  and may consume workspace entries without linking the child.
 */
static bool_t WriteSize(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const char_t* name, const Utl_Arinc661_Size_t* size)
{
    Utl_Xml_Node_t* node;
    const char_t*   unit_name;
    bool_t          is_valid;

    // The caller selects pixelSize or physicalDimensions
    node     = AppendElement(xml, name);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_WIDTH, size->width);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_HEIGHT, size->height);
    }
    if (is_valid)
    {
        // Convert the enum to its literal; an unsupported unit makes the append fail
        unit_name = UnitName(size->units);
        is_valid  = AppendAttribute(xml, node, XML_ATTRIBUTE_UNITS, unit_name);
    }
    if (is_valid)
    {
        // Link the size child only after all attributes are complete
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates a pixelArea child with the rectangle's origin and dimensions:
 *
 * \code{.xml}
 * <pixelArea x="10" y="20" width="640" height="480"/>
 * \endcode
 *
 *  All four attributes are written as signed integers. The complete child is
 *  linked to parent only after all appends succeed. Failure returns FALSE;
 *  temporary workspace entries already used are not reclaimed.
 */
static bool_t WriteRectangle(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Rectangle_t* rectangle)
{
    Utl_Xml_Node_t* node;
    bool_t          is_valid;

    // Create the pixelArea node before formatting its four integer attributes
    node     = AppendElement(xml, DMC_ELEMENT_PIXEL_AREA);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_X, rectangle->x);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_Y, rectangle->y);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_WIDTH, rectangle->width);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_HEIGHT, rectangle->height);
    }
    if (is_valid)
    {
        // Attach the rectangle only after every attribute is ready
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates a scalingFactor child with fixed-point numeric values and explicit
 *  units. For example, factors 1.25 and 0.5 with screen/pixel units produce:
 *
 * \code{.xml}
 * <scalingFactor xScale="1.25" yScale="0.5" baseUnit="screen" perUnit="pixel"/>
 * \endcode
 *
 *  Nonfinite factors, unsupported units, and exhausted workspace storage cause
 *  FALSE. The child is linked only on success; intermediate allocations remain
 *  in the temporary workspace if a later attribute fails.
 */
static bool_t WriteScale(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_Scale_t* scale)
{
    Utl_Xml_Node_t* node;
    const char_t*   base_unit_name;
    const char_t*   per_unit_name;
    bool_t          is_valid;

    // Build both decimal factors and both unit attributes before linking
    node     = AppendElement(xml, DMC_ELEMENT_SCALING_FACTOR);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendDecimalAttribute(xml, node, XML_ATTRIBUTE_X_SCALE, scale->xScale);
    }
    if (is_valid)
    {
        is_valid = AppendDecimalAttribute(xml, node, XML_ATTRIBUTE_Y_SCALE, scale->yScale);
    }
    if (is_valid)
    {
        // Resolve each unit enum to the literal used in the XML attribute
        base_unit_name = UnitName(scale->baseUnit);
        is_valid       = AppendAttribute(xml, node, XML_ATTRIBUTE_BASE_UNIT, base_unit_name);
    }
    if (is_valid)
    {
        per_unit_name = UnitName(scale->perUnit);
        is_valid      = AppendAttribute(xml, node, XML_ATTRIBUTE_PER_UNIT, per_unit_name);
    }
    if (is_valid)
    {
        // Append the completed scalingFactor child
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds a ua element from one application record, for example:
 *
 * \code{.xml}
 * <ua applicationId="1" dFPath="display.xml" window="7" mapSourceLayer="false" visible="true"/>
 * \endcode
 *
 *  Both boolean settings are always written as "true" or "false". styleFilePath
 *  is omitted when its string is empty. String values are referenced directly
 *  until serialization; numeric values use workspace text storage. The element
 *  is linked only after all attributes succeed.
 */
static bool_t WriteUserApplication(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_UserApplication_t* application)
{
    Utl_Xml_Node_t* node;
    const char_t*   map_source_layer_text;
    const char_t*   visible_text;
    bool_t          is_valid;

    // Write boolean settings using their word forms
    map_source_layer_text = XML_BOOLEAN_FALSE;
    if (application->mapSourceLayer)
    {
        map_source_layer_text = XML_BOOLEAN_TRUE;
    }
    visible_text = XML_BOOLEAN_FALSE;
    if (application->visible)
    {
        visible_text = XML_BOOLEAN_TRUE;
    }
    node     = AppendElement(xml, DMC_ELEMENT_USER_APPLICATION);
    is_valid = (NULL != node);
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_APPLICATION_ID, application->applicationId);
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, node, XML_ATTRIBUTE_DF_PATH, application->dFPath);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_WINDOW, application->window);
    }
    if (is_valid)
    {
        // Omit the optional style path when no path was supplied
        if ('\0' != application->styleFilePath[0])
        {
            is_valid = AppendAttribute(xml, node, XML_ATTRIBUTE_STYLE_FILE_PATH, application->styleFilePath);
        }
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, node, XML_ATTRIBUTE_MAP_SOURCE_LAYER, map_source_layer_text);
    }
    if (is_valid)
    {
        is_valid = AppendAttribute(xml, node, XML_ATTRIBUTE_VISIBLE, visible_text);
    }
    if (is_valid)
    {
        // Attach the application after all required and optional attributes succeed
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates an externalSource element containing only its supported attributes:
 *
 * \code{.xml}
 * <externalSource id="12" name="Camera" type="video"/>
 * \endcode
 *
 *  The id must not exceed UINT16_MAX. Empty name and type strings are omitted,
 *  so a source with only its identifier becomes:
 *
 * \code{.xml}
 * <externalSource id="12"/>
 * \endcode
 *
 *  The node is linked to parent only after successful attribute construction.
 */
static bool_t WriteExternalSource(XmlWorkspace_t* xml, Utl_Xml_Node_t* parent, const Utl_Arinc661_ExternalSource_t* source)
{
    Utl_Xml_Node_t* node;
    bool_t          is_valid;

    // Reject an out-of-range identifier before allocating an XML node
    is_valid = (UINT16_MAX >= source->id);
    if (is_valid)
    {
        node     = AppendElement(xml, DMC_ELEMENT_EXTERNAL_SOURCE);
        is_valid = (NULL != node);
    }
    if (is_valid)
    {
        is_valid = AppendIntegerAttribute(xml, node, XML_ATTRIBUTE_ID, source->id);
    }
    if (is_valid)
    {
        // Write optional descriptive attributes only when their strings are nonempty
        if ('\0' != source->name[0])
        {
            is_valid = AppendAttribute(xml, node, XML_ATTRIBUTE_NAME, source->name);
        }
    }
    if (is_valid)
    {
        if ('\0' != source->type[0])
        {
            is_valid = AppendAttribute(xml, node, XML_ATTRIBUTE_TYPE, source->type);
        }
    }
    if (is_valid)
    {
        // Link the completed source element
        Utl_Xml_AppendChild(parent, node);
    }

    return is_valid;
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
 *  inside this subtree; ancestor traversal is bounded by UTL_XML_NODE_MAX_DEPTH.
 */
static Utl_Xml_Node_t* NextSubtreeNode(Utl_Xml_Node_t* root, Utl_Xml_Node_t* current)
{
    Utl_Xml_Node_t* next_node;
    Utl_Xml_Node_t* ancestor;
    uint32_t        ancestor_index;

    // Descend first so parents are visited before their descendants
    next_node = current->firstChild;
    ancestor  = current;
    if (NULL == next_node)
    {
        for (ancestor_index = 0; ancestor_index < UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
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
 *  Copies the direct prop children of a required model into the shared property
 *  array, for example:
 *
 * \code{.xml}
 * <model>
 *     <prop name="WidgetIdent" value="10"/>
 *     <prop name="Visible" value="A661_TRUE"/>
 * </model>
 * \endcode
 *
 *  With owner=UTL_ARINC661_PROPERTY_WIDGET and ownerIndex=2, both copied entries
 *  refer to widgets[2]. An empty model succeeds without adding entries. Missing
 *  name or value attributes become empty strings through CopyAttribute.
 *  A missing model, unexpected child, or exhausted result storage returns FALSE.
 *  Properties already counted remain until the public reader clears the counts.
 */
static bool_t ReadProperties(Utl_Arinc661_Definition_t* definition, Utl_Xml_Node_t* model, Utl_Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    Utl_Xml_Node_t*          property_node;
    Utl_Arinc661_Property_t* property;
    uint32_t                 node_index;
    bool_t                   is_valid;

    // Every supported owner needs a model, even when it has no properties
    is_valid = (NULL != model);
    if (is_valid)
    {
        property_node = model->firstChild;
        for (node_index = 0; node_index < UTL_ARINC661_XML_NODE_CAPACITY; ++node_index)
        {
            if (NULL == property_node)
            {
                break;
            }
            if (HasElementName(property_node, DF_ELEMENT_PROPERTY))
            {
                // Reserve a result slot before copying this property's strings
                is_valid = (definition->propertyCount < definition->propertyCapacity);
                if (FALSE == is_valid)
                {
                    break;
                }
                property = &definition->properties[definition->propertyCount];
                (void)memset(property, 0, sizeof((*property)));
                // Record the owning model kind and its array index with each flat property
                property->owner      = owner;
                property->ownerIndex = ownerIndex;
                is_valid             = CopyAttribute(property_node, XML_ATTRIBUTE_NAME, property->name, sizeof(property->name));
                if (is_valid)
                {
                    is_valid = CopyAttribute(property_node, XML_ATTRIBUTE_VALUE, property->value, sizeof(property->value));
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                // Publish the property only after both copied strings fit
                definition->propertyCount += 1;
            }
            else
            {
                is_valid = FALSE;
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
 *  belonging to other definitions, layers, or widgets are skipped. If none match,
 *  an empty model element is still appended. A partially built model is not
 *  linked to parent when an attribute or workspace allocation fails.
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
            if (is_owner && has_index)
            {
                property_node = AppendElement(xml, DF_ELEMENT_PROPERTY);
                is_valid      = (NULL != property_node);
                if (is_valid)
                {
                    is_valid = AppendAttribute(xml, property_node, XML_ATTRIBUTE_NAME, property->name);
                }
                if (is_valid)
                {
                    is_valid = AppendAttribute(xml, property_node, XML_ATTRIBUTE_VALUE, property->value);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                // Preserve the order of matching properties
                Utl_Xml_AppendChild(model, property_node);
            }
        }
    }
    if (is_valid)
    {
        // Attach the model only after all selected properties have been built
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
 *  Parses a supported configuration into caller-provided arrays. For example:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * <configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="H3.xsd">
 *     <metaData/>
 *     <screen id="2">
 *         <pixelSize width="1280" height="720" units="pixel"/>
 *         <physicalDimensions width="12" height="7" units="inch"/>
 *         <layout id="3">
 *             <window id="7" name="Main">
 *                 <description/>
 *                 <pixelArea x="0" y="0" width="1280" height="720"/>
 *                 <scalingFactor xScale="1" yScale="1"/>
 *             </window>
 *         </layout>
 *     </screen>
 *     <ua applicationId="1" dFPath="display.xml" window="7" visible="true"/>
 *     <externalSource id="12" name="Camera" type="video"/>
 * </configuration>
 * \endcode
 *
 *  The schema location is copied into schemaLocation, and hasXmlDeclaration
 *  records whether the optional declaration was present. Both are cleared on
 *  failure. Schema contents are not fetched or validated.
 *
 *  The example adds one entry to each result array and sets hasMetaData.
 *  Layouts and windows store their owning array indices; application.window
 *  retains the XML window id. Text fields are copied into the result storage,
 *  so the result keeps no pointers into the input or temporary XML tree.
 *
 *  The workspace holds all temporary nodes and attributes. On failure, all result
 *  counts and hasMetaData are cleared; partially written array entries are not
 *  erased. The input needs the writable extra byte required by the XML reader.
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
    // Start a new result while preserving the caller's storage pointers and capacities
    configuration->screenCount          = 0;
    configuration->layoutCount          = 0;
    configuration->windowCount          = 0;
    configuration->userApplicationCount = 0;
    configuration->externalSourceCount  = 0;
    configuration->hasMetaData          = FALSE;
    configuration->hasXmlDeclaration    = FALSE;
    configuration->schemaLocation[0]    = '\0';
    has_workspace                       = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid                            = has_workspace;
    if (has_workspace)
    {
        // Parse into temporary workspace nodes before interpreting configuration values
        is_valid = (0 < Utl_Xml_ReadDocument(&xml.document, buffer, byteCount, xml.nodes, UTL_ARINC661_XML_NODE_CAPACITY, xml.attributes, UTL_ARINC661_XML_ATTRIBUTE_CAPACITY));
    }
    if (is_valid)
    {
        // Select the document element and require the configuration root name
        root             = FindDocumentElement(&xml.document);
        is_configuration = HasElementName(root, DMC_ELEMENT_CONFIGURATION);
        is_valid         = is_configuration;
    }
    if (is_valid)
    {
        // Preserve the optional declaration and copy the root's schema hint
        configuration->hasXmlDeclaration = (UTL_XML_NODE_XML_DECLARATION == xml.document.firstNode->type);
        is_valid = ReadConfigurationSchema(root, configuration);
    }
    if (is_valid)
    {
        // Dispatch direct configuration children to their typed readers
        child_node = root->firstChild;
        for (node_index = 0; node_index < UTL_ARINC661_XML_NODE_CAPACITY; ++node_index)
        {
            if (NULL == child_node)
            {
                break;
            }
            if (HasElementName(child_node, DMC_ELEMENT_METADATA))
            {
                // Only one metaData marker may be present
                is_valid = (FALSE == configuration->hasMetaData);
                if (is_valid)
                {
                    configuration->hasMetaData = TRUE;
                }
            }
            else if (HasElementName(child_node, DMC_ELEMENT_SCREEN))
            {
                is_valid = ReadScreenHierarchy(configuration, child_node);
            }
            else if (HasElementName(child_node, DMC_ELEMENT_USER_APPLICATION))
            {
                is_valid = ReadUserApplication(configuration, child_node);
            }
            else if (HasElementName(child_node, DMC_ELEMENT_EXTERNAL_SOURCE))
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
    if (FALSE == is_valid)
    {
        // Discard the partial result logically by clearing counts and the metadata flag
        configuration->screenCount          = 0;
        configuration->layoutCount          = 0;
        configuration->windowCount          = 0;
        configuration->userApplicationCount = 0;
        configuration->externalSourceCount  = 0;
        configuration->hasMetaData          = FALSE;
        configuration->hasXmlDeclaration    = FALSE;
        configuration->schemaLocation[0]    = '\0';
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds a temporary XML tree from the separate configuration arrays, then
 *  serializes it. For example, one screen and one application can produce:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * <configuration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="H3.xsd">
 *     <screen id="0">
 *         <pixelSize width="800" height="600" units="pixel"/>
 *         <physicalDimensions width="10" height="8" units="inch"/>
 *         <layout id="0">
 *             <window id="4" name="Display">
 *                 <description/>
 *                 <pixelArea x="0" y="0" width="800" height="600"/>
 *                 <scalingFactor xScale="2" yScale="2" baseUnit="screen" perUnit="pixel"/>
 *             </window>
 *         </layout>
 *     </screen>
 *     <ua applicationId="5" dFPath="main.xml" window="4" mapSourceLayer="false" visible="true"/>
 * </configuration>
 * \endcode
 *
 *  hasXmlDeclaration controls the optional declaration shown above. The root
 *  always includes the schema-instance binding and no-namespace schema hint;
 *  an empty schemaLocation selects H3.xsd, while a supplied location is retained.
 *
 *  A layout's screenIndex and a window's layoutIndex select their parents.
 *  Children are grouped in the order metaData, screens, applications, then
 *  external sources. The caller supplies valid array counts and parent indices.
 *  Window descriptions must be empty because text content is unsupported.
 *
 *  Output uses four-space indentation and LF line endings, including the final
 *  LF, with no zero terminator. Zero indicates invalid supported values or
 *  insufficient storage; output may contain a prefix if final serialization fails.
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
    // Keep the failure result until the complete temporary tree can be serialized
    byte_count    = 0;
    has_workspace = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid      = has_workspace;
    if (is_valid)
    {
        // Add the declaration only when requested by the configuration
        if (configuration->hasXmlDeclaration)
        {
            declaration = &xml.nodes[xml.nodeCount];
            Utl_Xml_InitializeNode(declaration, UTL_XML_NODE_XML_DECLARATION, NULL, 0);
            xml.nodeCount += 1;
            Utl_Xml_AppendNode(&xml.document, declaration);
        }
        root     = AppendElement(&xml, DMC_ELEMENT_CONFIGURATION);
        is_valid = (NULL != root);
        if (is_valid)
        {
            Utl_Xml_AppendNode(&xml.document, root);
        }
    }
    if (is_valid)
    {
        // Every configuration root carries the schema-instance binding and location
        is_valid = WriteConfigurationSchema(&xml, root, configuration);
    }
    if (is_valid)
    {
        // Emit the optional metadata marker before the configuration's data children
        if (configuration->hasMetaData)
        {
            metadata_node = AppendElement(&xml, DMC_ELEMENT_METADATA);
            is_valid      = (NULL != metadata_node);
            if (is_valid)
            {
                Utl_Xml_AppendChild(root, metadata_node);
            }
        }
    }
    if (is_valid)
    {
        // Build each screen with its layouts and windows selected by array indices
        for (screen_index = 0; screen_index < configuration->screenCount; ++screen_index)
        {
            screen      = &configuration->screens[screen_index];
            screen_node = AppendElement(&xml, DMC_ELEMENT_SCREEN);
            is_valid    = (NULL != screen_node);
            if (is_valid)
            {
                is_valid = AppendIntegerAttribute(&xml, screen_node, XML_ATTRIBUTE_ID, screen->id);
            }
            if (is_valid)
            {
                is_valid = WriteSize(&xml, screen_node, DMC_ELEMENT_PIXEL_SIZE, &screen->pixelSize);
            }
            if (is_valid)
            {
                is_valid = WriteSize(&xml, screen_node, DMC_ELEMENT_PHYSICAL_DIMENSIONS, &screen->physicalDimensions);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            for (layout_index = 0; layout_index < configuration->layoutCount; ++layout_index)
            {
                layout = &configuration->layouts[layout_index];
                // Attach only layouts belonging to the current screen
                if (screen_index == layout->screenIndex)
                {
                    layout_node = AppendElement(&xml, DMC_ELEMENT_LAYOUT);
                    is_valid    = (NULL != layout_node);
                    if (is_valid)
                    {
                        is_valid = AppendIntegerAttribute(&xml, layout_node, XML_ATTRIBUTE_ID, layout->id);
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    for (window_index = 0; window_index < configuration->windowCount; ++window_index)
                    {
                        window = &configuration->windows[window_index];
                        // Attach only windows belonging to the current layout
                        if (layout_index == window->layoutIndex)
                        {
                            window_node = AppendElement(&xml, DMC_ELEMENT_WINDOW);
                            is_valid    = (NULL != window_node);
                            if (is_valid)
                            {
                                is_valid = AppendIntegerAttribute(&xml, window_node, XML_ATTRIBUTE_ID, window->id);
                            }
                            if (is_valid)
                            {
                                is_valid = AppendAttribute(&xml, window_node, XML_ATTRIBUTE_NAME, window->name);
                            }
                            if (is_valid)
                            {
                                // Description text is unsupported; only an empty description can be written
                                is_valid = ('\0' == window->description[0]);
                            }
                            if (is_valid)
                            {
                                description_node = AppendElement(&xml, DMC_ELEMENT_DESCRIPTION);
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
        // Applications follow all screens in the output
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
        // External sources follow the application entries
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
        // Serialize only after every requested entry has been built successfully
        byte_count = Utl_Xml_WriteDocument(&xml.document, buffer, capacity);
    }

    return byte_count;
}

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
 * properties[0].owner           = UTL_ARINC661_PROPERTY_DEFINITION
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
    // Reset result counts while retaining the supplied arrays and capacities
    definition->layerCount    = 0;
    definition->widgetCount   = 0;
    definition->propertyCount = 0;
    has_workspace            = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid                 = has_workspace;
    if (is_valid)
    {
        // Parse the source into the temporary XML workspace
        is_valid = (0 < Utl_Xml_ReadDocument(&xml.document, buffer, byteCount, xml.nodes, UTL_ARINC661_XML_NODE_CAPACITY, xml.attributes, UTL_ARINC661_XML_ATTRIBUTE_CAPACITY));
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
        is_valid   = ReadProperties(definition, model_node, UTL_ARINC661_PROPERTY_DEFINITION, 0);
    }
    if (is_valid)
    {
        // Process layers in source order and walk each layer's complete subtree
        layer_node = root->firstChild;
        for (layer_index = 0; layer_index < UTL_ARINC661_XML_NODE_CAPACITY; ++layer_index)
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
                    is_valid   = ReadProperties(definition, model_node, UTL_ARINC661_PROPERTY_LAYER, definition->layerCount);
                }
                if (FALSE == is_valid)
                {
                    break;
                }
                definition->layerCount += 1;
                current_node = layer_node->firstChild;
                for (node_index = 0; node_index < UTL_ARINC661_XML_NODE_CAPACITY; ++node_index)
                {
                    if (NULL == current_node)
                    {
                        break;
                    }
                    if (HasElementName(current_node, DF_ELEMENT_WIDGET))
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
                        if (current_node->parent != layer_node)
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
                            is_valid   = ReadProperties(definition, model_node, UTL_ARINC661_PROPERTY_WIDGET, definition->widgetCount);
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
                        is_model    = HasElementName(current_node, DF_ELEMENT_MODEL);
                        is_property = HasElementName(current_node, DF_ELEMENT_PROPERTY);
                        is_valid    = is_model || is_property;
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
uint64_t Utl_Arinc661_WriteDefinition(const Utl_Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize)
{
    XmlWorkspace_t               xml;
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
    if (is_valid)
    {
        // The root model precedes every layer
        is_valid = WriteProperties(&xml, root, definition, UTL_ARINC661_PROPERTY_DEFINITION, 0);
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
                is_valid = WriteProperties(&xml, layer_node, definition, UTL_ARINC661_PROPERTY_LAYER, layer_index);
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
                    widget_node = AppendElement(&xml, DF_ELEMENT_WIDGET);
                    is_valid    = (NULL != widget_node);
                    if (is_valid)
                    {
                        is_valid = AppendAttribute(&xml, widget_node, XML_ATTRIBUTE_NAME, widget->name);
                    }
                    if (is_valid)
                    {
                        is_valid = AppendAttribute(&xml, widget_node, XML_ATTRIBUTE_TYPE, widget->type);
                    }
                    if (is_valid)
                    {
                        is_valid = WriteProperties(&xml, widget_node, definition, UTL_ARINC661_PROPERTY_WIDGET, widget_index);
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
                    Utl_Xml_AppendChild(parent_node, widget_node);
                    // Record the built node for subsequent children to use as their parent
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
        // Append the completed root and serialize the temporary document
        Utl_Xml_AppendNode(&xml.document, root);
        byte_count = Utl_Xml_WriteDocument(&xml.document, buffer, capacity);
    }

    return byte_count;
}
