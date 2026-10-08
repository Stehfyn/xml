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
 * ARINC 661 Part 1, Supplement 7, Creation Structure tables and Table 4.6-13.
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
    Arinc661_PropertyOwner_t owner;                              ///< Owning model kind
    uint32_t                 ownerIndex;                         ///< Layer or widget array index; zero for the Definition File
    char_t                   name[ARINC661_MAX_NAME_BYTES];      ///< Zero-terminated property name
    char_t                   value[ARINC661_MAX_PROPERTY_BYTES]; ///< Zero-terminated property value
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
    uint32_t layerIndex;                    ///< Array index of the owning layer
    uint32_t parentWidgetIndex;             ///< Earlier parent widget array index, or UINT32_MAX for a direct layer child
    bool_t   isExtension;                   ///< TRUE for an a661_extension of parentWidgetIndex (Part 1-7, section 8); type names the extension
} Arinc661_Widget_t;

/*************************************************************************************************/
/**
 * \brief Charset encoding of a Definition File (Part 1-7, Table 4.6-13).
 *
 * \par Details:
 *  Section 3.2.5.1: the encoding applies to all string parameters in the
 *  Definition File and to the run-time parameters of its widgets. A file
 *  without a charset encoding uses A661_ASCII_EXTENDED. Values from OEM_FIRST
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
 * \brief Caller-owned ARINC 661 Definition File data.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: a661_df root (dfType), with model and a661_layer children.
 */
typedef struct Arinc661_Definition_s
{
    char_t                     name[ARINC661_MAX_NAME_BYTES]; ///< Zero-terminated optional Definition File name
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
    byte_t*                    blocks;                        ///< Pointer to the caller's storage for binary symbol, picture, and animation law blocks, kept byte for byte
    uint64_t                   blockCapacity;                 ///< Block storage bytes
    uint64_t                   blockByteCount;                ///< Block storage bytes in use
} Arinc661_Definition_t;

/*************************************************************************************************/
/**
 * \brief Binary encoding of a Creation Structure parameter (Part 1-7, section 3.3 data types).
 */
typedef enum Arinc661_ParameterKind_e
{
    ARINC661_PARAMETER_UCHAR,        ///< Unsigned 8-bit integer; decimal or constant name
    ARINC661_PARAMETER_USHORT,       ///< Unsigned 16-bit integer; decimal or constant name
    ARINC661_PARAMETER_LONG,         ///< Signed 32-bit integer
    ARINC661_PARAMETER_ULONG,        ///< Unsigned 32-bit integer
    ARINC661_PARAMETER_FLOAT,        ///< IEEE 754 single precision decimal
    ARINC661_PARAMETER_FR180,        ///< Angle in degrees, -180 to 180
    ARINC661_PARAMETER_FR32768,      ///< Fraction of 32768
    ARINC661_PARAMETER_STRING,       ///< Text
    ARINC661_PARAMETER_BYTES,        ///< Fixed byte count as hexadecimal digits
    ARINC661_PARAMETER_SIZED_BYTES,  ///< StaticParamBuffer entry as hexadecimal digits
    ARINC661_PARAMETER_REGION,       ///< Excluded region structure as hexadecimal digits
    ARINC661_PARAMETER_LAYER_REQUEST ///< EventHandler LayerReqResult entry as hexadecimal digits
} Arinc661_ParameterKind_t;

/*************************************************************************************************/
/**
 * \brief One named parameter of a widget or extension Creation Structure.
 */
typedef struct Arinc661_Parameter_s
{
    const char_t*            name;             ///< Property name
    Arinc661_ParameterKind_t kind;             ///< Binary encoding
    uint32_t                 byteCount;        ///< Bytes of an ARINC661_PARAMETER_BYTES value
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
