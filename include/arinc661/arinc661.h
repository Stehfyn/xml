#ifndef ARINC661_H
#define ARINC661_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6, Cockpit Display System Interfaces to User Systems:
 * https://saemobilus.sae.org/standards/arinc661-6-cockpit-display-system-interfaces-user-systems
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd):
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 * ARINC 661 Part 1, Supplement 7, Creation Structure tables, Table 3.3-1, section 4.5.4.5,
 * Tables 4.6-10 and 4.6-11, and Table 4.6-13.
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
#define ARINC661_MAX_NAME_BYTES     (128UL) ///< Name or type string capacity in bytes, including the zero terminator
#define ARINC661_MAX_PROPERTY_BYTES (256UL) ///< Property value string capacity in bytes, including the zero terminator

/* Values of constants in Table 4.6-10, Boolean Constant Values, and Table 4.6-11, Integer Constant Values,
   to compare a property's number with; Arinc661_Runtime_GetConstantValue gives every constant's value. */
#define ARINC661_A661_FALSE         (0x00U) ///< A661_FALSE (Table 4.6-10)
#define ARINC661_A661_TRUE          (0x01U) ///< A661_TRUE (Table 4.6-10)
#define ARINC661_A661_SELECTED      (0x01U) ///< A661_SELECTED (Table 4.6-11)
#define ARINC661_A661_HORZ_LINE     (0x02U) ///< A661_HORZ_LINE (Table 4.6-11)
#define ARINC661_A661_VERT_LINE     (0x03U) ///< A661_VERT_LINE (Table 4.6-11)
#define ARINC661_A661_ABSENT        (0x10U) ///< A661_ABSENT (Table 4.6-11)
#define ARINC661_A661_TOP           (0x11U) ///< A661_TOP (Table 4.6-11)
#define ARINC661_A661_BOTTOM        (0x12U) ///< A661_BOTTOM (Table 4.6-11)
#define ARINC661_A661_LEFT          (0x13U) ///< A661_LEFT (Table 4.6-11)
#define ARINC661_A661_RIGHT         (0x14U) ///< A661_RIGHT (Table 4.6-11)
#define ARINC661_A661_OPEN_UP       (0x16U) ///< A661_OPEN_UP (Table 4.6-11)
#define ARINC661_A661_OPEN_CENTERED (0x17U) ///< A661_OPEN_CENTERED (Table 4.6-11)
#define ARINC661_A661_BOTTOM_CENTER (0x30U) ///< A661_BOTTOM_CENTER (Table 4.6-11)
#define ARINC661_A661_BOTTOM_LEFT   (0x31U) ///< A661_BOTTOM_LEFT (Table 4.6-11)
#define ARINC661_A661_BOTTOM_RIGHT  (0x32U) ///< A661_BOTTOM_RIGHT (Table 4.6-11)
#define ARINC661_A661_TOP_CENTER    (0x33U) ///< A661_TOP_CENTER (Table 4.6-11)
#define ARINC661_A661_TOP_LEFT      (0x34U) ///< A661_TOP_LEFT (Table 4.6-11)
#define ARINC661_A661_TOP_RIGHT     (0x35U) ///< A661_TOP_RIGHT (Table 4.6-11)
#define ARINC661_A661_BOTTOM_TO_TOP (0x40U) ///< A661_BOTTOM_TO_TOP (Table 4.6-11)
#define ARINC661_A661_RIGHT_TO_LEFT (0x42U) ///< A661_RIGHT_TO_LEFT (Table 4.6-11)
#define ARINC661_A661_TOP_TO_BOTTOM (0x43U) ///< A661_TOP_TO_BOTTOM (Table 4.6-11)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief What a property belongs to: the definition itself, one of its layers, or one of its widgets.
 *
 * \par Details:
 *  Selects the array that Arinc661_Property_t.ownerIndex indexes. In the XML form,
 *  each of the three carries its properties in a model element (a661Strict.xsd
 *  dfType, layerType, and widgetType).
 */
typedef enum Arinc661_PropertyOwner_e
{
    ARINC661_PROPERTY_DEFINITION, ///< The definition, for properties such as ApplicationId; ownerIndex is zero
    ARINC661_PROPERTY_LAYER,      ///< layers[ownerIndex], for properties such as LayerId and ContextNumber
    ARINC661_PROPERTY_WIDGET      ///< widgets[ownerIndex], a widget or extension, for WidgetIdent and Creation Structure parameter values
} Arinc661_PropertyOwner_t;

/*************************************************************************************************/
/**
 * \brief One named value owned by the definition, a layer, or a widget.
 *
 * \par Details:
 *  owner and ownerIndex say which. A widget's properties are its WidgetIdent and
 *  the values of its Creation Structure parameters, each named after the
 *  parameter that Arinc661_Parameter_t describes; an array parameter has one
 *  property per element, all with the array's name, in element order. XML form:
 *  a661Strict.xsd modelType/propType, with name and value attributes, or one
 *  value of an arrayprop.
 */
typedef struct Arinc661_Property_s
{
    Arinc661_PropertyOwner_t owner;                              ///< What the property belongs to
    uint32_t                 ownerIndex;                         ///< Index into the layers or widgets of the definition; zero for ARINC661_PROPERTY_DEFINITION
    char_t                   name[ARINC661_MAX_NAME_BYTES];      ///< Zero-terminated property name
    char_t                   value[ARINC661_MAX_PROPERTY_BYTES]; ///< Zero-terminated property value
} Arinc661_Property_t;

/*************************************************************************************************/
/**
 * \brief One layer of a definition; it holds the widgets whose layerIndex names it.
 *
 * \par Details:
 *  Holds only the name. LayerId, ContextNumber, and the layer's other values are properties
 *  the layer owns.
 *  Binary form: one layer block (Table 4.5.3.2-4). XML form: a661Strict.xsd
 *  dfType/a661_layer (layerType).
 */
typedef struct Arinc661_Layer_s
{
    char_t name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional layer name
} Arinc661_Layer_t;

/*************************************************************************************************/
/**
 * \brief One widget, or one extension of a widget, in a definition.
 *
 * \par Details:
 *  layerIndex names the layer the widget is in. parentWidgetIndex names the earlier
 *  widget it is nested in, or is UINT32_MAX for a widget placed directly in its layer.
 *  An extension (section 8) is an entry with isExtension set: parentWidgetIndex names
 *  the widget it extends, and type names the extension type. A widget's values,
 *  including its WidgetIdent, are properties it owns; so are an extension's values,
 *  but an extension has no WidgetIdent. Binary form: one A661_CMD_CREATE or
 *  A661_CMD_EXTEND command (Tables 4.5.3.3.1 and 4.5.3.3.2). XML form: a661Strict.xsd
 *  layerType/a661_widget, the nested widgetType/a661_widget, and a661_extension.
 */
typedef struct Arinc661_Widget_s
{
    char_t   name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional widget name
    char_t   type[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated required widget type
    uint32_t layerIndex;                    ///< Array index of the layer holding the widget
    uint32_t parentWidgetIndex;             ///< Array index of the earlier parent widget, or UINT32_MAX for a widget placed directly in the layer
    bool_t   isExtension;                   ///< TRUE for an extension of widgets[parentWidgetIndex]; type then names the extension
} Arinc661_Widget_t;

/*************************************************************************************************/
/**
 * \brief Character set encoding of a definition's strings (Part 1-7, Table 4.6-13).
 *
 * \par Details:
 *  A definition has one encoding, Arinc661_Definition_t.charsetEncoding. Per
 *  section 3.2.5.1, it applies to every string parameter of the definition's
 *  widgets, both in the file and at run time. Binary form: the charset encoding
 *  block (Table 4.5.3.2-6). XML form: the charset-encoding element (section
 *  6.2.1). Without either, strings use A661_ASCII_EXTENDED. Values from OEM_FIRST
 *  to OEM_LAST are implementation-dependent encodings the table reserves.
 */
typedef enum Arinc661_CharsetEncoding_e
{
    ARINC661_CHARSET_ENCODING_NONE           = 0,      ///< No charset-encoding element or block; strings use A661_ASCII_EXTENDED
    ARINC661_CHARSET_ENCODING_ASCII_EXTENDED = 1,      ///< A661_ASCII_EXTENDED
    ARINC661_CHARSET_ENCODING_UTF8           = 2,      ///< A661_UTF8
    ARINC661_CHARSET_ENCODING_GBK            = 3,      ///< A661_GBK
    ARINC661_CHARSET_ENCODING_OEM_FIRST      = 0xB800, ///< First value reserved for OEM customization
    ARINC661_CHARSET_ENCODING_OEM_LAST       = 0xBFFF  ///< Last value reserved for OEM customization
} Arinc661_CharsetEncoding_t;

/*************************************************************************************************/
/**
 * \brief How a Creation Structure or run-time parameter is encoded in the binary form (Part 1-7, Table 3.3-1), and the text its property value holds.
 */
typedef enum Arinc661_ParameterKind_e
{
    ARINC661_PARAMETER_UCHAR,        ///< Unsigned 8-bit integer; decimal or constant name
    ARINC661_PARAMETER_USHORT,       ///< Unsigned 16-bit integer; decimal or constant name
    ARINC661_PARAMETER_SHORT,        ///< Signed 16-bit integer, set only at run time; decimal
    ARINC661_PARAMETER_LONG,         ///< Signed 32-bit integer; decimal or constant name
    ARINC661_PARAMETER_ULONG,        ///< Unsigned 32-bit integer; decimal or constant name
    ARINC661_PARAMETER_FLOAT,        ///< IEEE 754 single precision; decimal
    ARINC661_PARAMETER_FR180,        ///< fr(180) scaled integer; decimal degrees, -180 to 180
    ARINC661_PARAMETER_FR32768,      ///< fr(32768) scaled integer; decimal, -32768 to 32768
    ARINC661_PARAMETER_STRING,       ///< Text
    ARINC661_PARAMETER_BYTES,        ///< Fixed byte count as hexadecimal digits
    ARINC661_PARAMETER_SIZED_BYTES,  ///< StaticParamBuffer entry as hexadecimal digits
    ARINC661_PARAMETER_REGION,       ///< Excluded region structure as hexadecimal digits
    ARINC661_PARAMETER_LAYER_REQUEST ///< EventHandler LayerReqResult entry as hexadecimal digits
} Arinc661_ParameterKind_t;

/*************************************************************************************************/
/**
 * \brief The parameter structure carrying a run-time value (Part 1-7, section 4.5.4.5).
 */
typedef enum Arinc661_ParameterStructure_e
{
    ARINC661_PARAMETER_STRUCTURE_1BYTE,       ///< A661_ParameterStructure_1Byte: ParameterIdent, a 1-byte value, a pad
    ARINC661_PARAMETER_STRUCTURE_2BYTES,      ///< A661_ParameterStructure_2Bytes: ParameterIdent, a 2-byte value
    ARINC661_PARAMETER_STRUCTURE_4BYTES,      ///< A661_ParameterStructure_4Bytes: ParameterIdent, a pad, a 4-byte value
    ARINC661_PARAMETER_STRUCTURE_8BYTES,      ///< A661_ParameterStructure_8Bytes: ParameterIdent, a pad, two 4-byte values
    ARINC661_PARAMETER_STRUCTURE_STRING,      ///< A661_ParameterStructure_String: ParameterIdent, a size, a padded string
    ARINC661_PARAMETER_STRUCTURE_STRING_ARRAY ///< A661_ParameterStructure_StringArray: ParameterIdent, a count, indexed padded strings
} Arinc661_ParameterStructure_t;

/*************************************************************************************************/
/**
 * \brief Description of one parameter of a widget or extension type's Creation Structure.
 *
 * \par Details:
 *  Describes a parameter, not a value: Arinc661_Definition_GetParameter returns one
 *  per named row of the type's Creation Structure table (sections 3 and 8), without
 *  WidgetType, WidgetIdent, ParentIdent, ExtensionType, and unused pads. A widget's
 *  value for the parameter is the property it owns with the same name. An array
 *  parameter is counted by the parameter named countName, with elementsPerEntry
 *  elements per entry.
 */
typedef struct Arinc661_Parameter_s
{
    const char_t*            name;             ///< Parameter name, which is also the name of the property holding its value
    Arinc661_ParameterKind_t kind;             ///< Binary encoding
    uint32_t                 sizeOfValue;      ///< Encoded bytes of one value; zero when the value sizes itself
    uint32_t                 elementsPerEntry; ///< Array elements per counted entry; zero for a single value
    const char_t*            countName;        ///< Name of the parameter holding an array's entry count, or NULL
    const char_t* const*     values;           ///< Constant names the Creation Structure table lists for the value, or NULL
    uint32_t                 valueCount;       ///< Number of listed constant names
} Arinc661_Parameter_t;

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

#endif /* ARINC661_H */
