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

#include "arinc661/arinc661.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Arbitrarily chosen capacities for the XML arrays stored in the caller's workspace. */
#define ARINC661_XML_NODE_CAPACITY      (65536UL) ///< Temporary XML node slots for declarations, elements, and descendants
#define ARINC661_XML_ATTRIBUTE_CAPACITY (65536UL) ///< Temporary XML attribute slots shared by all elements
#define ARINC661_XML_WIDGET_CAPACITY    (65536UL) ///< XML node pointer slots indexed by widget array position

/* Arbitrarily chosen default workspace size, with room for these XML arrays and formatted values. */
#define ARINC661_WORKSPACE_BYTES (16777216UL) ///< Workspace buffer size for temporary XML arrays and formatted value strings

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief One ARINC 661 definition: its layers, its widgets, and the properties of all three, in caller-owned arrays.
 *
 * \par Details:
 *  Read from and written to a Definition File in the XML form (a661Strict.xsd
 *  a661_df root, dfType) or the binary form (Table 4.5.3.2-1). The arrays are flat;
 *  widgets point to their layer and parent widget by array index, and properties
 *  to their owner, which together describe the tree:
 *
 * \verbatim
 * definition         properties[] with owner DEFINITION              ApplicationId
 *   layers[i]        properties[] with owner LAYER, ownerIndex i     LayerId, ContextNumber
 *     widgets[j]     layerIndex i, parentWidgetIndex UINT32_MAX      properties[] with owner WIDGET, ownerIndex j
 *       widgets[k]   layerIndex i, parentWidgetIndex j               a widget nested in widgets[j]
 *       widgets[m]   layerIndex i, parentWidgetIndex j, isExtension  an extension of widgets[j]
 * \endverbatim
 *
 *  The binary form's symbol, picture, and animation law blocks are kept byte for
 *  byte in blocks.
 */
typedef struct Arinc661_Definition_s
{
    char_t                     name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional name of the definition
    uint32_t                   libraryVersion;                ///< Library version
    uint32_t                   supplementVersion;             ///< Supplement version
    Arinc661_Layer_t*          layers;                        ///< Pointer to the caller's layer array
    Arinc661_Widget_t*         widgets;                       ///< Pointer to the caller's widget array
    Arinc661_Property_t*       properties;                    ///< Pointer to the caller's property array
    uint32_t                   layerCapacity;                 ///< Layer slots
    uint32_t                   widgetCapacity;                ///< Widget slots
    uint32_t                   propertyCapacity;              ///< Property slots
    uint32_t                   layerCount;                    ///< Layers in use
    uint32_t                   widgetCount;                   ///< Widgets in use
    uint32_t                   propertyCount;                 ///< Properties in use
    Arinc661_CharsetEncoding_t charsetEncoding;               ///< Charset encoding, or ARINC661_CHARSET_ENCODING_NONE when none is given
    byte_t*                    blocks;                        ///< Pointer to the caller's storage for the binary form's symbol, picture, and animation law blocks
    uint64_t                   blockCapacity;                 ///< Block storage bytes
    uint64_t                   blockByteCount;                ///< Block storage bytes in use
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
 * \brief Attaches caller storage and starts an empty Definition File.
 *
 * \param[out] definition         Definition File
 * \param[in]  layers             Layer storage
 * \param[in]  layerCapacity      Layer slots
 * \param[in]  widgets            Widget storage
 * \param[in]  widgetCapacity     Widget slots
 * \param[in]  properties         Property storage
 * \param[in]  propertyCapacity   Property slots
 * \param[in]  blocks             Binary block storage, or NULL
 * \param[in]  blockCapacity      Binary block bytes
 *
 * \retval     None
 */
void Arinc661_Definition_Initialize(Arinc661_Definition_t* definition, Arinc661_Layer_t* layers, uint32_t layerCapacity, Arinc661_Widget_t* widgets, uint32_t widgetCapacity, Arinc661_Property_t* properties, uint32_t propertyCapacity, byte_t* blocks, uint64_t blockCapacity);

/*************************************************************************************************/
/**
 * \brief Counts the widget types or extension types known to the binary reader and writer.
 *
 * \param[in] isExtension   TRUE for extension types (Table 4.6-12), FALSE for widget types (Table 4.6-7)
 *
 * \retval    uint32_t      Number of types.
 */
uint32_t Arinc661_Definition_GetTypeCount(bool_t isExtension);

/*************************************************************************************************/
/**
 * \brief Returns the name of a widget or extension type.
 *
 * \param[in] isExtension   TRUE for extension types, FALSE for widget types
 * \param[in] typeIndex     Index below Arinc661_Definition_GetTypeCount
 *
 * \retval    const char_t*   Zero-terminated type name, such as A661_PANEL.
 */
const char_t* Arinc661_Definition_GetTypeName(bool_t isExtension, uint32_t typeIndex);

/*************************************************************************************************/
/**
 * \brief Looks up a widget or extension type by name and counts its named Creation Structure parameters.
 *
 * \param[in]  typeName         Zero-terminated type name
 * \param[in]  isExtension      TRUE for extension types, FALSE for widget types
 * \param[out] parameterCount   Number of named parameters, excluding unused pads
 *
 * \retval     TRUE             if the type is known.
 * \retval     FALSE            otherwise.
 */
bool_t Arinc661_Definition_FindType(const char_t* typeName, bool_t isExtension, uint32_t* parameterCount);

/*************************************************************************************************/
/**
 * \brief Describes one named Creation Structure parameter of a type.
 *
 * \par Details:
 *  Parameters follow Creation Structure order; WidgetType, WidgetIdent, ParentIdent, and
 *  ExtensionType are not included. The listed constant names come from the Value/Range
 *  column of the Creation Structure table.
 *
 * \param[in]  typeName         Zero-terminated type name
 * \param[in]  isExtension      TRUE for extension types, FALSE for widget types
 * \param[in]  parameterIndex   Index below the count from Arinc661_Definition_FindType
 * \param[out] parameter        Parameter description; its pointers stay valid for the program lifetime
 *
 * \retval     TRUE             if the type and parameter exist.
 * \retval     FALSE            otherwise.
 */
bool_t Arinc661_Definition_GetParameter(const char_t* typeName, bool_t isExtension, uint32_t parameterIndex, Arinc661_Parameter_t* parameter);

/*************************************************************************************************/
/**
 * \brief Counts the section 4.6 constant names accepted as property values.
 *
 * \retval    uint32_t   Number of constants.
 */
uint32_t Arinc661_Definition_GetConstantCount(void);

/*************************************************************************************************/
/**
 * \brief Returns a constant name.
 *
 * \param[in] constantIndex   Index below Arinc661_Definition_GetConstantCount
 *
 * \retval    const char_t*   Zero-terminated constant name, such as A661_TRUE.
 */
const char_t* Arinc661_Definition_GetConstantName(uint32_t constantIndex);

/*************************************************************************************************/
/**
 * \brief Returns the value the binary form stores for a constant.
 *
 * \param[in] constantIndex   Index below Arinc661_Definition_GetConstantCount
 *
 * \retval    uint32_t        Constant value.
 */
uint32_t Arinc661_Definition_GetConstantValue(uint32_t constantIndex);

/*************************************************************************************************/
/**
 * \brief Tells whether an extension may be attached to a widget type.
 *
 * \par Details:
 *  Part 1-7, Table 8.4, Possible Widgets for Extension.
 *
 * \param[in] extensionType   Extension type name
 * \param[in] widgetType      Widget type name
 *
 * \retval    TRUE            if the table lists the widget for the extension.
 * \retval    FALSE           otherwise.
 */
bool_t Arinc661_Definition_ExtensionApplies(const char_t* extensionType, const char_t* widgetType);

/*************************************************************************************************/
/**
 * \brief Lists the widget types an extension may be attached to.
 *
 * \param[in] extensionType   Extension type name
 * \param[in] index           Position in the list
 *
 * \retval    const char_t*   Widget type name, or NULL past the end.
 */
const char_t* Arinc661_Definition_GetExtensionTarget(const char_t* extensionType, uint32_t index);

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
 *  structprop and datablock, are rejected. An a661_extension inside a widget
 *  (section 8) becomes a widget array entry flagged isExtension. A top-level
 *  charset-encoding element (section 6.2.1) sets charsetEncoding from its name;
 *  symbol, picture, and animation law definitions are not read, and
 *  blockByteCount is cleared.
 *  Strings are copied into the result arrays; the result retains no pointers
 *  into the input buffer or temporary workspace.
 *  Input may include an XML declaration; it is not retained in the result.
 *  The input buffer must have one writable byte after sizeOfXmlBuffer bytes and is
 *  changed by FACEOE_Utl_Xml_ReadDocument while parsing.
 *
 * \par Requirements:
 *  - 001 This function shall read Definition File, layer, widget, property, and array property values into caller-owned storage.
 *  - 002 This function shall clear all result counts when a supported value is invalid or a result array is too small.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall return FALSE when FACEOE_Utl_Xml_ReadDocument rejects the input.
 *  - 005 This function shall accept input with or without an XML declaration.
 *  - 006 This function shall fail an assertion if definition, bufferToReadXmlFrom, or workspace is NULL.
 *
 * \param[in,out] definition            Definition File with storage pointers and capacities initialized
 * \param[in,out] bufferToReadXmlFrom   Input XML bytes, followed by one writable byte
 * \param[in]     sizeOfXmlBuffer       Number of input bytes, excluding the extra byte
 * \param[in,out] workspace             Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize         Number of temporary bytes
 *
 * \retval        TRUE                  if the supported Definition File values were read.
 * \retval        FALSE                 if the XML or a supported value is invalid, or supplied storage is insufficient.
 */
bool_t Arinc661_Definition_Read(Arinc661_Definition_t* definition, byte_t* bufferToReadXmlFrom, uint64_t sizeOfXmlBuffer, void* workspace, uint64_t workspaceSize);

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
 *  An entry flagged isExtension is written as an a661_extension inside its parent
 *  widget. A charsetEncoding other than ARINC661_CHARSET_ENCODING_NONE writes a
 *  charset-encoding element. Binary
 *  blocks have no XML form here and are not written.
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
 *  - 006 This function shall fail an assertion if definition, bufferToWriteXmlTo, or workspace is NULL.
 *  - 007 This function shall fail an assertion if a count exceeds its capacity, or an array with a nonzero count is NULL.
 *
 * \param[in]     definition            Definition File data to write
 * \param[out]    bufferToWriteXmlTo    Output XML bytes
 * \param[in]     capacityOfXmlBuffer   Output buffer capacity
 * \param[in,out] workspace             Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize         Number of temporary bytes
 *
 * \retval        uint64_t              Number of bytes written, or zero when a value or supplied storage is invalid.
 */
uint64_t Arinc661_Definition_Write(const Arinc661_Definition_t* definition, byte_t* bufferToWriteXmlTo, uint64_t capacityOfXmlBuffer, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Reads a binary ARINC 661 Definition File from memory into caller-owned data.
 *
 * \par Details:
 *  Reads the binary Definition File form of ARINC 661 Part 1, Supplement 7,
 *  section 4.5.3.2: header, optional charset encoding block, symbol, picture,
 *  and animation law blocks, layer blocks of A661_CMD_CREATE and A661_CMD_EXTEND
 *  commands, and footer.
 *  Each Creation Structure parameter becomes a property named as in the widget
 *  or extension Creation Structure table (sections 3 and 8), with a decimal
 *  value; constant names such as A661_TRUE are not restored. AppliIdent,
 *  LayerIdent, and ContextNumber become the ApplicationId, LayerId, and
 *  ContextNumber properties. An A661_CMD_EXTEND command becomes a widget array
 *  entry flagged isExtension whose parent is the widget it follows (section 8.1).
 *  The charset encoding block sets charsetEncoding; symbol, picture, and
 *  animation law blocks are copied unchanged into blocks, so the file can be
 *  written back byte for byte. The binary form carries no names, so the
 *  Definition File, layer, and widget names are empty. String bytes are copied
 *  unchanged, whatever the character set encoding.
 *  Each array element becomes its own property, all sharing the array's name,
 *  in element order; arrays of pairs, such as Vertices and BufferStructure,
 *  alternate the two members. Parameters with no numeric type, such as
 *  DefaultStyleText, StaticParamBuffer entries, and excluded region structures,
 *  become two uppercase hexadecimal digits per byte.
 *  A661_SELECTOR cannot be read because Table 4.6-7 assigns it no WidgetType code.
 *  The result retains no pointers into the input buffer.
 *
 * \par Requirements:
 *  - 001 This function shall read Definition File, charset encoding, layer, widget, extension, and Creation Structure values into caller-owned storage.
 *  - 002 This function shall retain symbol, picture, and animation law blocks unchanged in caller-owned storage.
 *  - 003 This function shall clear all result counts when the input is malformed or a result array is too small.
 *  - 004 This function shall not read outside the supplied input bytes.
 *  - 005 This function shall fail an assertion if definition or bufferToReadBinaryFrom is NULL, or blocks is NULL with a nonzero blockCapacity.
 *
 * \param[in,out] definition               Definition File with storage pointers and capacities initialized
 * \param[in]     bufferToReadBinaryFrom   Input binary bytes
 * \param[in]     sizeOfBinaryBuffer       Number of input bytes
 *
 * \retval        TRUE                     if the supported Definition File values were read.
 * \retval        FALSE                    if the input is malformed or unsupported, or supplied storage is insufficient.
 */
bool_t Arinc661_Definition_ReadBinary(Arinc661_Definition_t* definition, const byte_t* bufferToReadBinaryFrom, uint64_t sizeOfBinaryBuffer);

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
 *  DefaultStyleText, StaticParamBuffer entries, and excluded region structures,
 *  take two hexadecimal digits per byte. An entry flagged isExtension is written
 *  as an A661_CMD_EXTEND command and must directly follow, in array order, the
 *  widget named by its parentWidgetIndex. A charsetEncoding other than
 *  ARINC661_CHARSET_ENCODING_NONE writes the
 *  charset encoding block, and blocks are written unchanged before the layers.
 *  A661_SELECTOR cannot be written because Table 4.6-7 assigns it no WidgetType code.
 *
 * \par Requirements:
 *  - 001 This function shall write the header, the charset encoding block when set, the retained blocks, one layer block per layer with its widgets and extensions in array order, and the footer.
 *  - 002 This function shall return the number of bytes written.
 *  - 003 This function shall return zero when a widget type is unsupported, a widget property names no Creation Structure parameter, a value does not fit its parameter, or an array does not match its count.
 *  - 004 This function shall return zero without writing past capacityOfBinaryBuffer when the output does not fit.
 *  - 005 This function shall fail an assertion if definition or bufferToWriteBinaryTo is NULL.
 *  - 006 This function shall fail an assertion if a count exceeds its capacity, or an array with a nonzero count is NULL.
 *
 * \param[in]     definition               Definition File data to write
 * \param[out]    bufferToWriteBinaryTo    Output binary bytes
 * \param[in]     capacityOfBinaryBuffer   Output buffer capacity
 *
 * \retval        uint64_t                 Number of bytes written, or zero when a value is unsupported or the capacity is insufficient.
 */
uint64_t Arinc661_Definition_WriteBinary(const Arinc661_Definition_t* definition, byte_t* bufferToWriteBinaryTo, uint64_t capacityOfBinaryBuffer);

#endif /* ARINC661_DEFINITION_H */
