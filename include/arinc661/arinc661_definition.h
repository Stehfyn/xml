#ifndef ARINC661_DEFINITION_H
#define ARINC661_DEFINITION_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6, Cockpit Display System Interfaces to User Systems:
 * https://saemobilus.sae.org/standards/arinc661-6-cockpit-display-system-interfaces-user-systems
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd):
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 * ARINC 661 Part 1, Supplement 7, binary Definition File structure, section 4.5.3.2.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Arbitrarily chosen string capacities; these are not limits imposed by the XML schemas. */
#define ARINC661_MAX_NAME_BYTES        (128UL)      ///< Name or type string capacity in bytes, including the zero terminator
#define ARINC661_MAX_DESCRIPTION_BYTES (256UL)      ///< Description string capacity in bytes, including the zero terminator
#define ARINC661_MAX_PROPERTY_BYTES    (256UL)      ///< Property value string capacity in bytes, including the zero terminator
#define ARINC661_MAX_PATH_BYTES        (512UL)      ///< File path string capacity in bytes, including the zero terminator

/* Arbitrarily chosen capacities for the XML arrays stored in the caller's workspace. */
#define ARINC661_XML_NODE_CAPACITY      (65536UL) ///< Temporary XML node slots for declarations, elements, and descendants
#define ARINC661_XML_ATTRIBUTE_CAPACITY (65536UL) ///< Temporary XML attribute slots shared by all elements
#define ARINC661_XML_WIDGET_CAPACITY    (65536UL) ///< XML node pointer slots indexed by widget array position

/* Arbitrarily chosen default workspace size, with room for these XML arrays and formatted values. */
#define ARINC661_WORKSPACE_BYTES       (16777216UL) ///< Workspace buffer size for temporary XML arrays and formatted value strings

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Owner of an ARINC 661 Definition File property.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: dfType, layerType, and widgetType each contain a model.
 */
typedef enum Arinc661_PropertyOwner_e
{
    ARINC661_PROPERTY_DEFINITION,
    ARINC661_PROPERTY_LAYER,
    ARINC661_PROPERTY_WIDGET
} Arinc661_PropertyOwner_t;

/*************************************************************************************************/
/**
 * \brief Property in a Definition File model.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: modelType/propType, with name and value attributes.
 */
typedef struct Arinc661_Property_s
{
    Arinc661_PropertyOwner_t owner;                                  ///< Owning model kind
    uint32_t                     ownerIndex;                             ///< Layer or widget array index; zero for the Definition File
    char_t                       name[ARINC661_MAX_NAME_BYTES];      ///< Zero-terminated property name
    char_t                       value[ARINC661_MAX_PROPERTY_BYTES]; ///< Zero-terminated property value
} Arinc661_Property_t;

/*************************************************************************************************/
/**
 * \brief Layer in an ARINC 661 Definition File.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: dfType/a661_layer (layerType).
 */
typedef struct Arinc661_Layer_s
{
    char_t name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional layer name
} Arinc661_Layer_t;

/*************************************************************************************************/
/**
 * \brief Widget in an ARINC 661 Definition File.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: layerType/a661_widget and nested widgetType/a661_widget.
 */
typedef struct Arinc661_Widget_s
{
    char_t   name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional widget name
    char_t   type[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated required widget type
    uint32_t layerIndex;                        ///< Array index of the owning layer
    uint32_t parentWidgetIndex;                 ///< Earlier parent widget array index, or UINT32_MAX for a direct layer child
} Arinc661_Widget_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned ARINC 661 Definition File data.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: a661_df root (dfType), with model and a661_layer children.
 */
typedef struct Arinc661_Definition_s
{
    char_t                   name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional Definition File name
    uint32_t                 libraryVersion;                    ///< Library version
    uint32_t                 supplementVersion;                 ///< Supplement version
    Arinc661_Layer_t*    layers;                            ///< Pointer to the caller's layer array
    Arinc661_Widget_t*   widgets;                           ///< Pointer to the caller's widget array
    Arinc661_Property_t* properties;                        ///< Pointer to the caller's property array
    uint32_t                 layerCapacity;                     ///< Layer slots
    uint32_t                 widgetCapacity;                    ///< Widget slots
    uint32_t                 propertyCapacity;                  ///< Property slots
    uint32_t                 layerCount;                        ///< Layers in use
    uint32_t                 widgetCount;                       ///< Widgets in use
    uint32_t                 propertyCount;                     ///< Properties in use
} Arinc661_Definition_t;

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Reads an ARINC 661 Definition File from memory into caller-owned data.
 *
 * \par Details:
 *  Reads the ARINC 661 Supplement 6 XML Definition File form. The strict
 *  a661Strict.xsd schema names a661_df (dfType), a661_layer (layerType),
 *  a661_widget (widgetType), model (modelType), and prop (propType):
 *  https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 *  This reader supports layers, nested widgets, prop elements, and arrayprop
 *  elements with entry, xyentry, or structentry children (Part 1-7, section
 *  6.2.4). Each array value becomes one property named after the array, in
 *  document order: an xyentry yields x then y, and a structentry yields its
 *  field values with the field names dropped. Other property forms, such as
 *  structprop and datablock, are rejected.
 *  Strings are copied into the result arrays; the result retains no pointers
 *  into the input buffer or temporary workspace.
 *  Input may include an XML declaration; it is not retained in the result.
 *  The input buffer must have one writable byte after byteCount bytes and is
 *  changed by FACEOE_Utl_Xml_ReadDocument while parsing.
 *
 * \par Requirements:
 *  - 001 This function shall read Definition File, layer, widget, property, and array property values into caller-owned storage.
 *  - 002 This function shall clear all result counts when a supported value is invalid or a result array is too small.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall return FALSE when FACEOE_Utl_Xml_ReadDocument rejects the input.
 *  - 005 This function shall accept input with or without an XML declaration.
 *  - 006 This function shall fail an assertion if definition, buffer, or workspace is NULL.
 *
 * \param[in,out] definition      Definition File with storage pointers and capacities initialized
 * \param[in,out] buffer          Input XML bytes, followed by one writable byte
 * \param[in]     byteCount       Number of input bytes, excluding the extra byte
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \retval        TRUE        if the supported Definition File values were read.
 * \retval        FALSE       if the XML or a supported value is invalid, or supplied storage is insufficient.
 */
bool_t Arinc661_ReadDefinition(Arinc661_Definition_t* definition, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Writes caller-owned ARINC 661 Definition File data to memory.
 *
 * \par Details:
 *  Writes the ARINC 661 Supplement 6 XML Definition File form described by
 *  a661Strict.xsd, using a661_df, a661_layer, a661_widget, model, and prop:
 *  https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 *  This writer supports layers, nested widgets, and prop elements. Properties
 *  of one model that share a name are an array and are written as one arrayprop
 *  with an entry child per value; a lone property is always written as a prop.
 *  The result is XML in the caller's output buffer.
 *  Storage arrays and their string fields must remain valid until writing finishes.
 *  Output starts with the a661_df element, without an XML declaration.
 *
 * \par Requirements:
 *  - 001 This function shall write Definition File, layer, widget, and property values from caller-owned data.
 *  - 002 This function shall return the number of bytes written without adding a zero terminator.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall return zero when FACEOE_Utl_Xml_WriteDocument rejects the output capacity.
 *  - 005 This function shall write the document element without an XML declaration.
 *  - 006 This function shall fail an assertion if definition, buffer, or workspace is NULL.
 *
 * \param[in]     definition      Definition File data to write
 * \param[out]    buffer          Output XML bytes
 * \param[in]     capacity        Output buffer capacity
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \retval    uint64_t   Number of bytes written, or zero when a value or supplied storage is invalid.
 */
uint64_t Arinc661_WriteDefinition(const Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Reads a binary ARINC 661 Definition File from memory into caller-owned data.
 *
 * \par Details:
 *  Reads the binary Definition File form of ARINC 661 Part 1, Supplement 7,
 *  section 4.5.3.2: header, layer blocks of A661_CMD_CREATE commands, and footer.
 *  Each Creation Structure parameter becomes a property named as in the widget's
 *  Creation Structure table, with a decimal value; constant names such as
 *  A661_TRUE are not restored. AppliIdent, LayerIdent, and ContextNumber become
 *  the ApplicationId, LayerId, and ContextNumber properties. The binary form
 *  carries no names, so the Definition File, layer, and widget names are empty.
 *  String bytes are copied unchanged, whatever the character set encoding.
 *  Each array element becomes its own property, all sharing the array's name,
 *  in element order; arrays of pairs, such as Vertices and BufferStructure,
 *  alternate the two members. Parameters with no numeric type, such as
 *  DefaultStyleText, become two uppercase hexadecimal digits per byte.
 *  A661_SELECTOR, which has no WidgetType code in section 4.6, and charset
 *  encoding, symbol, picture, animation law, and A661_CMD_EXTEND content are
 *  not supported.
 *  The result retains no pointers into the input buffer.
 *
 * \par Requirements:
 *  - 001 This function shall read Definition File, layer, widget, and Creation Structure values into caller-owned storage.
 *  - 002 This function shall clear all result counts when the input is malformed or unsupported, or a result array is too small.
 *  - 003 This function shall not read outside the supplied input bytes.
 *  - 004 This function shall fail an assertion if definition or buffer is NULL.
 *
 * \param[in,out] definition   Definition File with storage pointers and capacities initialized
 * \param[in]     buffer       Input binary bytes
 * \param[in]     byteCount    Number of input bytes
 *
 * \retval        TRUE        if the supported Definition File values were read.
 * \retval        FALSE       if the input is malformed or unsupported, or supplied storage is insufficient.
 */
bool_t Arinc661_ReadDefinitionBinary(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount);

/*************************************************************************************************/
/**
 * \brief Writes caller-owned ARINC 661 Definition File data to memory in binary form.
 *
 * \par Details:
 *  Writes the binary Definition File form of ARINC 661 Part 1, Supplement 7,
 *  section 4.5.3.2. Widget properties are matched by name to the parameters of
 *  the widget type's Creation Structure. Integer parameters take a decimal value
 *  or a constant name from section 4.6, such as A661_TRUE or A661_CENTER; float
 *  and fr() parameters take a decimal value. Other symbolic values, such as
 *  color or font names, cannot be written. An absent property is written as zero
 *  or as an empty string. Names, and Definition File and layer properties other
 *  than ApplicationId, LayerId, and ContextNumber, are not written.
 *  An array takes its elements from the properties sharing its name, in order;
 *  their number must match the widget's count parameter, such as
 *  NumberOfEntries. Arrays of pairs, such as Vertices and BufferStructure,
 *  take two properties per entry. Parameters with no numeric type, such as
 *  DefaultStyleText, take two hexadecimal digits per byte.
 *  A661_SELECTOR, which has no WidgetType code in section 4.6, is not supported.
 *
 * \par Requirements:
 *  - 001 This function shall write the header, one layer block per layer with its widgets in array order, and the footer.
 *  - 002 This function shall return the number of bytes written.
 *  - 003 This function shall return zero when a widget type is unsupported, a widget property names no Creation Structure parameter, a value does not fit its parameter, or an array does not match its count.
 *  - 004 This function shall return zero without writing past capacity when the output does not fit.
 *  - 005 This function shall fail an assertion if definition or buffer is NULL.
 *
 * \param[in]     definition   Definition File data to write
 * \param[out]    buffer       Output binary bytes
 * \param[in]     capacity     Output buffer capacity
 *
 * \retval    uint64_t   Number of bytes written, or zero when a value is unsupported or the capacity is insufficient.
 */
uint64_t Arinc661_WriteDefinitionBinary(const Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity);

#endif /* ARINC661_DEFINITION_H */
