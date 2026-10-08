/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6, Cockpit Display System Interfaces to User Systems:
 * https://saemobilus.sae.org/standards/arinc661-6-cockpit-display-system-interfaces-user-systems
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd):
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 * ARINC 661 Part 1, Supplement 7, binary Definition File:
 * section 4.5.3.2 (Definition File structure), section 4.5.3.3 (layer block commands),
 * section 4.6 (constants), section 7 (picture blocks), section 8 (widget extensions),
 * section 9 (animation law blocks), and the Creation Structure tables in sections 3 and 8.
 * ARINC 661 Part 1-7, Table 8.4, Possible Widgets for Extension: which widget types each
 * extension may be attached to. Generated from the specification's word positions.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_scan.h"
#include "faceoe_utl/faceoe_utl_xml.h"

#include "os/os_assert.h"

#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
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

/* ARINC 661 Part 1-7, Tables 4.6-1, 4.6-2, and 4.6-3a. */
#define BINARY_DF_MAGIC_NUMBER        (0xA661U) ///< A661_DF_MAGIC_NUMBER
#define BINARY_BEGIN_LAYER_BLOCK      (0xA0U)   ///< A661_BEGIN_LAYER_BLOCK
#define BINARY_END_LAYER_BLOCK        (0xC0U)   ///< A661_END_LAYER_BLOCK
#define BINARY_BEGIN_CHARSET_BLOCK    (0xC8U)   ///< A661_BEGIN_CHARSET_ENCODING_BLOCK
#define BINARY_END_CHARSET_BLOCK      (0xC9U)   ///< A661_END_CHARSET_ENCODING_BLOCK
#define BINARY_BEGIN_SYMBOL_BLOCK     (0xF0U)   ///< A661_BEGIN_SYMBOL_BLOCK
#define BINARY_END_SYMBOL_BLOCK       (0xF8U)   ///< A661_END_SYMBOL_BLOCK
#define BINARY_BEGIN_PICTURE_BLOCK    (0xBBU)   ///< A661_BEGIN_PICTURE_BLOCK
#define BINARY_END_PICTURE_BLOCK      (0xBEU)   ///< A661_END_PICTURE_BLOCK
#define BINARY_BEGIN_PICTURE          (0xBCU)   ///< A661_BEGIN_PICTURE
#define BINARY_END_PICTURE            (0xBDU)   ///< A661_END_PICTURE
#define BINARY_BEGIN_ANIMATION_BLOCK  (0xCAU)   ///< A661_BEGIN_ANIMATION_LAW_BLOCK
#define BINARY_END_ANIMATION_BLOCK    (0xCBU)   ///< A661_END_ANIMATION_LAW_BLOCK
#define BINARY_DF_FOOTER              (0xE0U)   ///< A661_DF_FOOTER
#define BINARY_CMD_CREATE             (0xCA01U) ///< A661_CMD_CREATE
#define BINARY_CMD_EXTEND             (0xCA05U) ///< A661_CMD_EXTEND

/* ARINC 661 Part 1-7, Table 7-3 pixel formats and their bytes per pixel or color table entry. */
#define BINARY_PIX_FMT_RGBA_8            (0xC0U) ///< A661_PIX_FMT_RGBA_8, four bytes
#define BINARY_PIX_FMT_LUMINANCE_ALPHA_8 (0xC1U) ///< A661_PIX_FMT_LUMINANCE_ALPHA_8, two bytes
#define BINARY_PIX_FMT_COLOR_INDEXED_8   (0xC2U) ///< A661_PIX_FMT_COLOR_INDEXED_8, one byte

/* ARINC 661 Part 1-7, Tables 4.5.3.2-2 through 4.5.3.2-6, 4.5.3.3.1, 4.5.3.3.2, 7-1, 7-2, and 9.2-1. */
#define BINARY_HEADER_BYTES         (8U)  ///< Magic number, versions, AppliIdent, and OEM data size
#define BINARY_LAYER_HEADER_BYTES   (8U)  ///< Block code, LayerIdent, ContextNumber, and Block Size
#define BINARY_SIZED_HEADER_BYTES   (8U)  ///< Block code, unused pad, and Block Size of symbol and animation law blocks
#define BINARY_CHARSET_BLOCK_BYTES  (8U)  ///< Complete charset encoding block
#define BINARY_PICTURE_HEADER_BYTES (8U)  ///< Picture block code, formats, counts, and unused pad
#define BINARY_PICTURE_BYTES        (8U)  ///< Picture code, pad, reference, width, and height
#define BINARY_BLOCK_END_BYTES      (4U)  ///< End or footer code followed by its unused pad
#define BINARY_COMMAND_BYTES        (10U) ///< Command code, CommandSize, WidgetType, WidgetIdent, and ParentIdent
#define BINARY_EXTEND_BYTES         (6U)  ///< Command code, CommandSize, and ExtensionType
#define BINARY_WORD_BYTES           (4U)  ///< Blocks and commands are aligned to 32 bits
#define BINARY_ALIGN(byteCount)     ((((uint64_t)(byteCount)) + (BINARY_WORD_BYTES - 1U)) & ~((uint64_t)BINARY_WORD_BYTES - 1U)) ///< Byte count rounded up to the 32-bit alignment
#define ARRAY_AND_COUNT(array)      (array), ((uint32_t)COUNT_OF(array)) ///< Pointer and element count of a true array object, for table initializers
#define BINARY_MAX_FIELDS           (40U) ///< Largest Creation Structure parameter count in the tables

/* ARINC 661 Part 1-7, section 3.3 data types: big-endian field widths. */
#define BINARY_UCHAR_BYTES  (1U) ///< uchar
#define BINARY_USHORT_BYTES (2U) ///< ushort
#define BINARY_ULONG_BYTES  (4U) ///< ulong, long, float, and fr()

/* ARINC 661 Part 1-7, Table 4.5.3.2-2: Definition File header field offsets. */
#define BINARY_HEADER_MAGIC_OFFSET              (0U) ///< A661_DF_MAGIC_NUMBER, ushort
#define BINARY_HEADER_LIBRARY_VERSION_OFFSET    (2U) ///< LibraryVersion, uchar
#define BINARY_HEADER_SUPPLEMENT_VERSION_OFFSET (3U) ///< SuppVersion, uchar
#define BINARY_HEADER_APPLI_IDENT_OFFSET        (4U) ///< AppliIdent, ushort
#define BINARY_HEADER_OEM_DATA_SIZE_OFFSET      (6U) ///< Size of the OEM data after the header, ushort

/* ARINC 661 Part 1-7, Tables 4.5.3.2-4 through 4.5.3.2-6 and 9.2-1: block field offsets. */
#define BINARY_BLOCK_CODE_SHIFT        (24U) ///< A block code is the most significant byte of its 32-bit word
#define BINARY_LAYER_IDENT_OFFSET      (1U)  ///< LayerIdent, uchar
#define BINARY_CONTEXT_NUMBER_OFFSET   (2U)  ///< ContextNumber, ushort
#define BINARY_BLOCK_SIZE_OFFSET       (4U)  ///< Block Size of layer, symbol, and animation law blocks, ulong
#define BINARY_CHARSET_ENCODING_OFFSET (2U)  ///< Charset encoding of the charset block, ushort
#define BINARY_CHARSET_END_OFFSET      (4U)  ///< A661_END_CHARSET_ENCODING_BLOCK of the charset block

/* ARINC 661 Part 1-7, Tables 4.5.3.3.1 and 4.5.3.3.2: command field offsets. */
#define BINARY_COMMAND_SIZE_OFFSET (2U) ///< CommandSize, ushort
#define BINARY_COMMAND_TYPE_OFFSET (4U) ///< WidgetType or ExtensionType, ushort
#define BINARY_WIDGET_IDENT_OFFSET (6U) ///< WidgetIdent, ushort
#define BINARY_PARENT_IDENT_OFFSET (8U) ///< ParentIdent, ushort

/* ARINC 661 Part 1-7, Tables 7-1 and 7-2: picture block and picture field offsets. */
#define BINARY_PIXEL_FORMAT_OFFSET   (1U) ///< Pixel format of the block's pictures, uchar
#define BINARY_PICTURE_COUNT_OFFSET  (2U) ///< Number of pictures, uchar
#define BINARY_COLOR_COUNT_OFFSET    (3U) ///< Number of color table entries, uchar
#define BINARY_COLOR_FORMAT_OFFSET   (4U) ///< Pixel format of the color table entries, uchar
#define BINARY_PICTURE_WIDTH_OFFSET  (4U) ///< Picture width in pixels, ushort
#define BINARY_PICTURE_HEIGHT_OFFSET (6U) ///< Picture height in pixels, ushort

/* ARINC 661 Part 1-7, Tables 8.6.5-3, 3.9.17-6, and 8.6.7-3: offsets inside variable-size parameters. */
#define BINARY_SIZED_PAD_OFFSET    (2U) ///< UnusedPad after the ParamBufferSize of a StaticParamBuffer, ushort
#define BINARY_REQUEST_SIZE_OFFSET (2U) ///< CommandSize of a LayerReqResult entry, ushort
#define BINARY_REGION_TYPE_OFFSET  (1U) ///< ExcludedRegionType of an excluded region structure, uchar

/* ARINC 661 Part 1-7, Table 8.6.7-3: ExcludedRegionType values. */
#define BINARY_REGION_NOT_USED  (0U) ///< A661_NOT_USED
#define BINARY_REGION_RECTANGLE (1U) ///< A661_EXCLUDED_RECTANGLE
#define BINARY_REGION_TRIANGLE  (2U) ///< A661_EXCLUDED_TRIANGLE
#define BINARY_REGION_CIRCLE    (3U) ///< A661_EXCLUDED_CIRCLE

/* Hexadecimal property text of byte parameters. */
#define HEX_DIGITS_PER_BYTE (2U)    ///< Two digits per byte, high digit first
#define HEX_DIGIT_BITS      (4U)    ///< Bits per digit
#define HEX_DIGIT_MASK      (0x0FU) ///< Low digit of a byte

/* ARINC 661 Part 1-7, Tables 8.6.7-4 through 8.6.7-7: excluded region structure sizes by ExcludedRegionType. */
#define BINARY_REGION_HEADER_BYTES    (4U)  ///< ExcludedItemIndex, ExcludedRegionType, and unused pad
#define BINARY_REQUEST_HEADER_BYTES   (4U)  ///< LayerIdent, unused pad, and CommandSize of a LayerReqResult entry
#define BINARY_REGION_NOT_USED_BYTES  (4U)  ///< A661_NOT_USED (0)
#define BINARY_REGION_RECTANGLE_BYTES (20U) ///< A661_EXCLUDED_RECTANGLE (1)
#define BINARY_REGION_TRIANGLE_BYTES  (28U) ///< A661_EXCLUDED_TRIANGLE (2)
#define BINARY_REGION_CIRCLE_BYTES    (24U) ///< A661_EXCLUDED_CIRCLE (3)

/* Property names shared with the XML Definition File form (section 6.5.1.2). */
#define BINARY_PROPERTY_APPLICATION_ID ("ApplicationId")
#define BINARY_PROPERTY_LAYER_ID       ("LayerId")
#define BINARY_PROPERTY_CONTEXT_NUMBER ("ContextNumber")
#define BINARY_PROPERTY_WIDGET_IDENT   ("WidgetIdent")

#define BINARY_INTEGER_TEXT_BYTES (32U)           ///< Integer formatting buffer bytes, including the zero terminator
#define BINARY_FR_FULL_SCALE      (2147483648.0)  ///< Two raised to the number of fr() bits minus one
#define BINARY_FR180_RANGE        (180.0)         ///< fr(180): degrees from -180 to 180
#define BINARY_FR32768_RANGE      (32768.0)       ///< fr(32768): -32768 to 32768
#define BINARY_FR_PRECISION       (10)            ///< Most fractional digits tried when formatting a fr() value; one fr(180) LSB is 8.4e-8
#define BINARY_FLOAT_PRECISION    (60)            ///< Most fractional digits tried when formatting a float; the smallest subnormal needs 45

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
    byte_t*                     text;           ///< Pointer to formatted attribute value storage
    uint64_t                    textCapacity;   ///< Available text bytes after the fixed workspace arrays
    uint64_t                    textByteCount;  ///< Text bytes used, including each string's zero terminator
    uint32_t                    nodeCount;      ///< XML node slots used while constructing output
    uint32_t                    attributeCount; ///< XML attribute slots used while constructing output
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

/*************************************************************************************************/
/**
 * \brief Encoding of one Creation Structure parameter (Part 1-7, section 3.3 data types).
 */
typedef enum BinaryFieldType_e
{
    BINARY_FIELD_PAD,          ///< UnusedPad bytes, written as zero
    BINARY_FIELD_UCHAR,        ///< Unsigned 8-bit integer
    BINARY_FIELD_USHORT,       ///< Unsigned 16-bit integer
    BINARY_FIELD_LONG,         ///< Signed 32-bit integer
    BINARY_FIELD_ULONG,        ///< Unsigned 32-bit integer
    BINARY_FIELD_FLOAT,        ///< IEEE 754 single precision
    BINARY_FIELD_FR180,        ///< Signed 32-bit fraction of 180
    BINARY_FIELD_FR32768,      ///< Signed 32-bit fraction of 32768
    BINARY_FIELD_STRING,       ///< Zero-terminated bytes
    BINARY_FIELD_BYTES,        ///< Fixed number of bytes with no numeric type, given as hexadecimal digits
    BINARY_FIELD_ALIGN,        ///< Zero bytes up to the next 32-bit boundary
    BINARY_FIELD_SIZED_BYTES,  ///< Table 8.6.5-3 StaticParamBuffer: byte count, pad, bytes, and alignment, given as hexadecimal digits
    BINARY_FIELD_REGION,       ///< Tables 8.6.7-4 to 8.6.7-7 excluded region structure, sized by its type byte, given as hexadecimal digits
    BINARY_FIELD_LAYER_REQUEST ///< Table 3.9.17-6 LayerReqResult: LayerIdent, pad, CommandSize, and the request, given as hexadecimal digits
} BinaryFieldType_t;

/*************************************************************************************************/
/**
 * \brief One parameter of a widget or extension Creation Structure.
 */
typedef struct BinaryField_s
{
    const char_t*        name;       ///< Property name, or NULL for an unused pad or alignment
    BinaryFieldType_t    type;       ///< Binary encoding
    uint32_t             byteCount;  ///< Encoded bytes of one value; zero when the value sizes itself
    uint32_t             countField; ///< For an array, index among the structure's parameters of the one holding its entry count
    uint32_t             repeat;     ///< Array elements per counted entry; zero for a single value
    const char_t* const* values;     ///< Constant names the table lists for this parameter, or NULL
    uint32_t             valueCount; ///< Number of listed constant names
} BinaryField_t;

/*************************************************************************************************/
/**
 * \brief Widget or extension type with its Creation Structure parameters.
 */
typedef struct BinaryWidget_s
{
    const char_t*        name;       ///< Type name, for example A661_PANEL or A661_LEGEND_ALIGN_EXTENSION
    uint16_t             code;       ///< WidgetType value in Table 4.6-7, or ExtensionType value in Table 4.6-12
    const BinaryField_t* fields;     ///< Creation Structure parameters, including unused pads
    uint32_t             fieldCount; ///< Number of parameters
} BinaryWidget_t;

/*************************************************************************************************/
/**
 * \brief Symbolic property value and its number.
 */
typedef struct BinaryConstant_s
{
    const char_t* name;  ///< Constant name, for example A661_TRUE
    uint32_t      value; ///< Value in Tables 4.6-8, 4.6-10, 4.6-11, 4.6-13, 4.6-14, and 4.6-15
} BinaryConstant_t;

/** One extension and the widget types it applies to. */
typedef struct ExtensionTargets_s
{
    const char_t*        extension;   ///< Extension type name
    const char_t* const* widgets;     ///< Widget type names
    uint32_t             widgetCount; ///< Number of widget type names
} ExtensionTargets_t;

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

/*************************************************************************************************/
/**
 * \brief Digits of a hexadecimal property value; one array, so digit pointers can be subtracted.
 */
static const char_t c_binaryHexDigits[] = "0123456789ABCDEF";

/*************************************************************************************************/
/**
 * \brief Bytes per pixel or color table entry of each Table 7-3 format, from A661_PIX_FMT_RGBA_8 on.
 */
static const uint32_t c_pixelFormatBytes[] = {4U, 2U, 1U};

/*************************************************************************************************/
/**
 * \brief Constant names each Creation Structure table lists for a parameter, shared where equal.
 */
static const char_t* const c_enableValues[] = {"A661_FALSE", "A661_TRUE", "A661_TRUE_WITH_VALIDATION"};
static const char_t* const c_booleanValues[] = {"A661_FALSE", "A661_TRUE"};
static const char_t* const c_rotationDirectionValues[] = {"A661_CLOCKWISE", "A661_COUNTER_CLOCKWISE"};
static const char_t* const c_checkButtonStateValues[] = {"A661_SELECTED", "A661_UNSELECTED"};
static const char_t* const c_buttonAlignmentValues[] = {"A661_LEFT", "A661_CENTER", "A661_RIGHT"};
static const char_t* const c_checkButtonPicturePositionValues[] = {"A661_LEFT", "A661_RIGHT"};
static const char_t* const c_comboBoxOpeningModeValues[] = {"A661_OPEN_UP", "A661_OPEN_CENTERED", "A661_OPEN_DOWN"};
static const char_t* const c_editBoxReportAllChangesValues[] = {"A661_EDB_CHANGE_CONFIRMED", "A661_EDB_ALL_CHANGE", "A661_EDB_OPEN_CLOSE"};
static const char_t* const c_editBoxAlignmentValues[] = {"A661_CENTER", "A661_LEFT", "A661_RIGHT"};
static const char_t* const c_positionReportModeValues[] = {"A661_REPORT_ON_TRANSITION", "A661_REPORT_ALL"};
static const char_t* const c_clampingValues[] = {"A661_NOT_CLAMPED", "A661_BOTH_CLAMPED"};
static const char_t* const c_scrollBarPositionValues[] = {"A661_TOP", "A661_BOTTOM", "A661_LEFT", "A661_RIGHT", "A661_ABSENT"};
static const char_t* const c_legendPositionValues[] = {"A661_LEFT", "A661_RIGHT", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_editBoxNumericReportAllChangesValues[] = {"A661_EDB_CHANGE_CONFIRMED", "A661_EDB_OPEN_CLOSE", "A661_EDB_ALL_CHANGE"};
static const char_t* const c_haloValues[] = {"A661_FALSE", "A661_TRUE", "A661_SAME_LEVEL"};
static const char_t* const c_labelAlignmentValues[] = {"A661_BOTTOM_CENTER", "A661_BOTTOM_LEFT", "A661_BOTTOM_RIGHT", "A661_CENTER", "A661_LEFT", "A661_RIGHT", "A661_TOP_CENTER", "A661_TOP_LEFT", "A661_TOP_RIGHT"};
static const char_t* const c_mapDataFormatValues[] = {"A661_MDF_BRG_DIST_ACHDG", "A661_MDF_DIST_DIST", "A661_MDF_LAT_LONG", "A661_MDF_DIST_DIST_FIX", "A661_MDF_LEGACY"};
static const char_t* const c_eventFlagValues[] = {"A661_EVENT_NONE", "A661_VALIDATION", "A661_VALIDATION_AND_WHEEL", "A661_WHEEL"};
static const char_t* const c_mapDataFormatXValues[] = {"A661_MDF_ABSOLUTE", "A661_MDF_RELATIVE", "A661_MDF_X_DIST"};
static const char_t* const c_mapDataFormatYValues[] = {"A661_MDF_ABSOLUTE", "A661_MDF_RELATIVE", "A661_MDF_Y_ALT"};
static const char_t* const c_mapBoundaryAlgorithmValues[] = {"A661_ARC", "A661_HORZ_LINE", "A661_VERT_LINE"};
static const char_t* const c_pushButtonAlignmentValues[] = {"A661_LEFT", "A661_RIGHT", "A661_CENTER"};
static const char_t* const c_wrappingTypeValues[] = {"A661_WRAP_NONE", "A661_WRAP_BOTH", "A661_WRAP_FIRST_TO_LAST", "A661_WRAP_LAST_TO_FIRST"};
static const char_t* const c_pagingControlPositionValues[] = {"A661_ABSENT", "A661_BOTTOM_CENTER", "A661_BOTTOM_LEFT", "A661_BOTTOM_RIGHT", "A661_CENTER", "A661_LEFT", "A661_RIGHT", "A661_TOP_CENTER", "A661_TOP_LEFT", "A661_TOP_RIGHT"};
static const char_t* const c_picturePositionValues[] = {"A661_LEFT", "A661_CENTER", "A661_RIGHT", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_toggleStateValues[] = {"A661_UNSELECTED", "A661_SELECTED"};
static const char_t* const c_popUpMenuOpeningModeValues[] = {"A661_OPEN_UP", "A661_OPEN_DOWN", "A661_CDS_DEPENDENT"};
static const char_t* const c_popUpButtonPicturePositionValues[] = {"A661_CENTER", "A661_LEFT", "A661_RIGHT", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_popUpMenuButtonOpeningModeValues[] = {"A661_OPEN_UP", "A661_OPEN_DOWN"};
static const char_t* const c_shuffleToFitModeValues[] = {"A661_SHUFFLE_UP", "A661_SHUFFLE_DOWN", "A661_SHUFFLE_LEFT", "A661_SHUFFLE_RIGHT", "A661_NO_SHUFFLE"};
static const char_t* const c_trueFalseValues[] = {"A661_TRUE", "A661_FALSE"};
static const char_t* const c_sliderOrientationValues[] = {"A661_BOTTOM_TO_TOP", "A661_TOP_TO_BOTTOM", "A661_LEFT_TO_RIGHT", "A661_RIGHT_TO_LEFT"};
static const char_t* const c_sliderAlignmentValues[] = {"A661_LEFT", "A661_RIGHT", "A661_CENTER", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_loopTypeValues[] = {"A661_LOOP_FORWARD", "A661_LOOP_FORWARD_AND_RESET", "A661_LOOP_FORWARD_AND_BLANK", "A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK"};
static const char_t* const c_animationTypeValues[] = {"A661_DONT_RUN", "A661_RUN", "A661_RUN_ONCE"};
static const char_t* const c_tabPositionValues[] = {"A661_ABSENT", "A661_TOP", "A661_BOTTOM", "A661_LEFT", "A661_RIGHT"};
static const char_t* const c_boundaryCheckModeValues[] = {"A661_OUT_OF_BOUNDS"};
static const char_t* const c_coordSystemReportModeValues[] = {"A661_REPORT_ON_CDS", "A661_REPORT_ALL"};

/*************************************************************************************************/
/**
 * \brief Creation Structure parameters, one array per widget type and then per extension type.
 *
 * \par Details:
 *  Transcribed from the Part 1-7 Creation Structure tables named in each group.
 *  Widget groups start after ParentIdent; extension groups start after
 *  ExtensionType. An array holds repeat elements for each entry counted by the
 *  parameter at countField; arrays of pairs, such as Vertices and
 *  BufferStructure, alternate the two members. Arrays are padded to 32 bits
 *  only where the table says so, which is recorded as an alignment entry.
 *  A661_SELECTOR has no array because Table 4.6-7 gives it no WidgetType code.
 */
/*************************************************************************************************/
/**
 * \brief A661_ACTIVE_AREA parameters: Table 3.3.1-2, ActiveArea Creation Structure.
 */
static const BinaryField_t c_activeAreaFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_GROUP parameters: Table 3.9.12-2, AnimationGroup Creation Structure.
 */
static const BinaryField_t c_animationGroupFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Type"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_ONPARAM parameters: Table 3.9.8-2, AnimationOnParam Creation Structure.
 */
static const BinaryField_t c_animationOnparamFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TargetParamID"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("FromValue"),                     BINARY_FIELD_BYTES,        4U,  0U, 0U, NULL, 0U},
    {("ToValue"),                       BINARY_FIELD_BYTES,        4U,  0U, 0U, NULL, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_ROTATION parameters: Table 3.9.9-2, AnimationRotation Creation Structure.
 */
static const BinaryField_t c_animationRotationFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("CenterX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("CenterY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("FromAngle"),                     BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("ToAngle"),                       BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Direction"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_rotationDirectionValues)},
    {("NumberOfTurns"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_SCALE parameters: Table 3.9.10-2, AnimationScale Creation Structure.
 */
static const BinaryField_t c_animationScaleFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("FromScaleX"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("FromScaleY"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("ToScaleX"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("ToScaleY"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_TRANSLATION parameters: Table 3.9.11-2, AnimationTranslation Creation Structure.
 */
static const BinaryField_t c_animationTranslationFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("FromValueX"),                    BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("FromValueY"),                    BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("ToValueX"),                      BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("ToValueY"),                      BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_BASIC_CONTAINER parameters: Table 3.3.2-2, BasicContainer Creation Structure.
 */
static const BinaryField_t c_basicContainerFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_BLINKING_CONTAINER parameters: Table 3.3.3-2, BlinkingContainer Creation Structure.
 */
static const BinaryField_t c_blinkingContainerFields[] =
{
    {("BlinkingType"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_BROADCAST_RECEIVER parameters: Table 3.8.9-2, BroadcastReceiver Creation Structure.
 */
static const BinaryField_t c_broadcastReceiverFields[] =
{
    {("NumberOfFields"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ParameterIdent"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("WidgetStructure"),               BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_BUFFER_FORMAT parameters: Table 3.3.4-2, BufferFormat Creation Structure.
 */
static const BinaryField_t c_bufferFormatFields[] =
{
    {("NumberOfFields"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("BufferStructure"),               BINARY_FIELD_USHORT,       2U,  0U, 2U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CHECK_BUTTON parameters: Table 3.3.5-2, CheckButton Creation Structure.
 */
static const BinaryField_t c_checkButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("CheckButtonState"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_checkButtonStateValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_checkButtonPicturePositionValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_COMBO_BOX parameters: Table 3.3.6-2, ComboBox Creation Structure.
 */
static const BinaryField_t c_comboBoxFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_comboBoxOpeningModeValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("EntryList"),                     BINARY_FIELD_STRING,       0U, 11U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_COMBO_BOX_EDIT parameters: Table 3.4.7-2, ComboBoxEdit Creation Structure.
 */
static const BinaryField_t c_comboBoxEditFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_comboBoxOpeningModeValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxReportAllChangesValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxAlignmentValues)},
    {("EntryList"),                     BINARY_FIELD_STRING,       0U, 11U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CONNECTOR parameters: Table 3.3.7-2, Connector Creation Structure.
 */
static const BinaryField_t c_connectorFields[] =
{
    {("ConnectorReference"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_OVER parameters: Table 3.6.3-2, CursorOver Creation Structure.
 */
static const BinaryField_t c_cursorOverFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("PositionReportMode"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_positionReportModeValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_POS_OVERLAY parameters: Table 3.3.8-2, CursorPosOverlay Creation Structure.
 */
static const BinaryField_t c_cursorPosOverlayFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_REF parameters: Table 3.6.2-2, CursorRef Creation Structure.
 */
static const BinaryField_t c_cursorRefFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_CONNECTOR parameters: Table 3.9.16-2, DataConnector Creation Structure.
 */
static const BinaryField_t c_dataConnectorFields[] =
{
    {("ConnectorReference"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("DataLength"),                    BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Data"),                          BINARY_FIELD_ULONG,        4U,  3U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_SCALING_FR180 parameters: Table 3.8.8-2, DataScalingFR180 Creation Structure.
 */
static const BinaryField_t c_dataScalingFr180Fields[] =
{
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_clampingValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("StartScaledValue"),              BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("EndScaledValue"),                BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_SCALING_LONG parameters: Table 3.8.6-2, DataScalingLong Creation Structure.
 */
static const BinaryField_t c_dataScalingLongFields[] =
{
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_clampingValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("StartScaledValue"),              BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("EndScaledValue"),                BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_SCALING_ULONG parameters: Table 3.8.7-2, DataScalingULong Creation Structure.
 */
static const BinaryField_t c_dataScalingUlongFields[] =
{
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_clampingValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("StartScaledValue"),              BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("EndScaledValue"),                BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_MASKED parameters: Table 3.3.9-2, EditBoxMasked Creation Structure.
 */
static const BinaryField_t c_editBoxMaskedFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AlphaMask"),                     BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("NumericMask"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxReportAllChangesValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_MULTI_LINE parameters: Table 3.4.6-2, EditBoxMultiLine Creation Structure.
 */
static const BinaryField_t c_editBoxMultiLineFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxReportAllChangesValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxAlignmentValues)},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_scrollBarPositionValues)},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_NUMERIC parameters: Table 3.3.10-2, EditBoxNumeric Creation Structure.
 */
static const BinaryField_t c_editBoxNumericFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Value"),                         BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("TicsCoarse"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("TicsFine"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("MinValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("MaxValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StartCursorPosByte"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_legendPositionValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxNumericReportAllChangesValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxAlignmentValues)},
    {("LegendRemoved"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("NumericKeyFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("CyclicFlag"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("FormatString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("LegendString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_NUMERIC_BCD parameters: Table 3.6.1-5, EditBoxNumericBCD Creation Structure.
 */
static const BinaryField_t c_editBoxNumericBcdFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Value"),                         BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("MinValue"),                      BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("MaxValue"),                      BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("MinFieldsValues"),               BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("MaxFieldsValues"),               BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("TicsCoarse"),                    BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("TicsFine"),                      BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL, 0U},
    {("NumberOfFields"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("LegendRemoved"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("PositiveStringLength"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NegativeStringLength"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NumericKeyFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("CyclicFlag"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("SizeOfFields"),                  BINARY_FIELD_UCHAR,        1U, 16U, 1U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL, 0U},
    {("FormatString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("LegendString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("PositiveString"),                BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("NegativeString"),                BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_TEXT parameters: Table 3.3.11-2, EditBoxText Creation Structure.
 */
static const BinaryField_t c_editBoxTextFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxReportAllChangesValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EVENT_HANDLER parameters: Table 3.9.17-3, EventHandler Creation Structure.
 */
static const BinaryField_t c_eventHandlerFields[] =
{
    {("EnableHandler"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("TriggerLayerIdent"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("TriggerWidgetIdent"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TriggerEvent"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("NumWidgetParamResults"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NumLayerReqResults"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("WidgetParamResultArray"),        BINARY_FIELD_BYTES,       20U,  5U, 1U, NULL, 0U},
    {("LayerReqResultArray"),           BINARY_FIELD_LAYER_REQUEST,  0U,  6U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EXTERNAL_SOURCE parameters: Table 3.4.2-2, ExternalSource Creation Structure.
 */
static const BinaryField_t c_externalSourceFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SourceReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("SourceX"),                       BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SourceY"),                       BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SourceDX"),                      BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SourceDY"),                      BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_IN parameters: Table 3.6.4.2-2, FocusIn Creation Structure.
 */
static const BinaryField_t c_focusInFields[] =
{
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_LINK parameters: Table 3.6.4.1-2, FocusLink Creation Structure.
 */
static const BinaryField_t c_focusLinkFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          4U,  0U, 0U, NULL, 0U},
    {("NextLayerConnectorRef"),         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_OUT parameters: Table 3.6.4.3-2, FocusOut Creation Structure.
 */
static const BinaryField_t c_focusOutFields[] =
{
    {("NextFocusInAppId"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusInLayerId"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("NextFocusInId"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_FRAMING_REFRESH_CONTAINER parameters: Table 3.9.18-2, FramingRefreshContainer Creation Structure.
 */
static const BinaryField_t c_framingRefreshContainerFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("CachingEnabled"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GESTURE_AREA parameters: Table 3.9.6-4, GestureArea Creation Structure.
 */
static const BinaryField_t c_gestureAreaFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("AllowOutside"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NumberOfEvents"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("EventIdArray"),                  BINARY_FIELD_UCHAR,        1U, 10U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_ARC_CIRCLE parameters: Table 3.3.13-2, GpArcCircle Creation Structure.
 */
static const BinaryField_t c_gpArcCircleFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("Radius"),                        BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_ARC_ELLIPSE parameters: Table 3.3.12-2, GpArcEllipse Creation Structure.
 */
static const BinaryField_t c_gpArcEllipseFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_CROWN parameters: Table 3.3.14-2, GpCrown Creation Structure.
 */
static const BinaryField_t c_gpCrownFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("InnerRadius"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("OuterRadius"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_LINE parameters: Table 3.3.15-2, GpLine Creation Structure.
 */
static const BinaryField_t c_gpLineFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosXStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosYStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosXEnd"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosYEnd"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_LINE_POLAR parameters: Table 3.3.16-2, GpLinePolar Creation Structure.
 */
static const BinaryField_t c_gpLinePolarFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosXStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosYStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("LineLength"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_POLYLINE parameters: Table 3.8.1-2, GpPolyline Creation Structure.
 */
static const BinaryField_t c_gpPolylineFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {("Closed"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  8U, 2U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_RECTANGLE parameters: Table 3.3.17-2, GpRectangle Creation Structure.
 */
static const BinaryField_t c_gpRectangleFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_TRIANGLE parameters: Table 3.3.18-2, GpTriangle Creation Structure.
 */
static const BinaryField_t c_gpTriangleFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosX2"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY2"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosX3"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY3"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_TRIANGLE_FAN parameters: Table 3.10.5-2, GpTriangleFan Creation Structure.
 */
static const BinaryField_t c_gpTriangleFanFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  9U, 2U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_TRIANGLE_STRIP parameters: Table 3.10.6-2, GpTriangleStrip Creation Structure.
 */
static const BinaryField_t c_gpTriangleStripFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_haloValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  9U, 2U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_INK_AREA parameters: Table 3.9.7-2, InkArea Creation Structure.
 */
static const BinaryField_t c_inkAreaFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("MaxBufferSize"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_KEYBOARD_AREA parameters: Table 3.9.2-2, KeyboardArea Creation Structure.
 */
static const BinaryField_t c_keyboardAreaFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NumberOfKeys"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfKeys"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("KeyArray"),                      BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_LABEL parameters: Table 3.3.20-2, Label Creation Structure.
 */
static const BinaryField_t c_labelFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Font"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_labelAlignmentValues)},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_LABEL_COMPLEX parameters: Table 3.3.21-2, LabelComplex Creation Structure.
 */
static const BinaryField_t c_labelComplexFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_labelAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("DefaultStyleText"),              BINARY_FIELD_BYTES,       12U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ parameters: Table 3.3.25-2a, MapHorz Creation Structure.
 */
static const BinaryField_t c_maphorzFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("ScreenReferencePointX"),         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("ScreenReferencePointY"),         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("Range"),                         BINARY_FIELD_FR32768,      4U,  0U, 0U, NULL, 0U},
    {("ScreenRange"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("ProjectionType"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_CONTAINER parameters: Table 3.8.4-2, MapHorz_Container Creation Structure.
 */
static const BinaryField_t c_maphorzContainerFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_ITEMLIST parameters: Table 3.3.22-2, MapHorz_ItemList Creation Structure.
 */
static const BinaryField_t c_maphorzItemlistFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MaxNumberOfItem"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxItemSize"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_PANEL parameters: Table 3.8.5-2, MapHorz_Panel Creation Structure.
 */
static const BinaryField_t c_maphorzPanelFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_SOURCE parameters: Table 3.3.24-2a, MapHorz_Source Creation Structure.
 */
static const BinaryField_t c_maphorzSourceFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MapDataFormat"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_mapDataFormatValues)},
    {("EventFlag"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_eventFlagValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_VERTEXBUFFER parameters: Table 3.9.4-2, MapHorz_VertexBuffer Creation Structure.
 */
static const BinaryField_t c_maphorzVertexbufferFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  3U, 2U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT parameters: Table 3.4.3-2a, MapVert Creation Structure.
 */
static const BinaryField_t c_mapvertFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("RangeX"),                        BINARY_FIELD_FR32768,      4U,  0U, 0U, NULL, 0U},
    {("RangeY"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("RefPosX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("RefPosY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("RefGeoPosX"),                    BINARY_FIELD_FR32768,      4U,  0U, 0U, NULL, 0U},
    {("RefGeoPosY"),                    BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_CONTAINER parameters: Table 3.9.13-2, MapVert_Container Creation Structure.
 */
static const BinaryField_t c_mapvertContainerFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_ITEMLIST parameters: Table 3.4.5-2a, MapVert_ItemList Creation Structure.
 */
static const BinaryField_t c_mapvertItemlistFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MaxNumberOfItem"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxItemSize"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_PANEL parameters: Table 3.9.14-2, MapVert_Panel Creation Structure.
 */
static const BinaryField_t c_mapvertPanelFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_SOURCE parameters: Table 3.4.4-2a, MapVert_Source Creation Structure.
 */
static const BinaryField_t c_mapvertSourceFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MapDataFormatX"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_mapDataFormatXValues)},
    {("MapDataFormatY"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_mapDataFormatYValues)},
    {("EventFlag"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAP_BOUNDARY parameters: Table 3.9.15-2, MapBoundary Creation Structure.
 */
static const BinaryField_t c_mapBoundaryFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("BeginAngleArray"),               BINARY_FIELD_FR180,        4U,  3U, 1U, NULL, 0U},
    {("DistanceArray"),                 BINARY_FIELD_ULONG,        4U,  3U, 1U, NULL, 0U},
    {("AlgorithmArray"),                BINARY_FIELD_ULONG,        4U,  3U, 1U, ARRAY_AND_COUNT(c_mapBoundaryAlgorithmValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAP_EXTERNAL_SOURCE parameters: Table 3.10.4-2, MapExternalSource Creation Structure.
 */
static const BinaryField_t c_mapExternalSourceFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SourceReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAP_GRID parameters: Table 3.4.1-2, MapGrid Creation Structure.
 */
static const BinaryField_t c_mapGridFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("OffsetX"),                       BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("OffsetY"),                       BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("IncrementX"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("IncrementY"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("CountX"),                        BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("CountY"),                        BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MASK_CONTAINER parameters: Table 3.3.26-2, MaskContainer Creation Structure.
 */
static const BinaryField_t c_maskContainerFields[] =
{
    {("MaskEnabled"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("MaskReference"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MENU_BAR parameters: Table 3.4.8-2, MenuBar Creation Structure.
 */
static const BinaryField_t c_menuBarFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Horizontal"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("ButtonPos"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("ButtonSize"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MULTI_STATE_BUTTON parameters: Table 3.9.1-2, MultiStateButton Creation Structure.
 */
static const BinaryField_t c_multiStateButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ButtonState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_pushButtonAlignmentValues)},
    {("MaxNumberOfStates"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NumberOfStates"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,       0U, 13U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MUTUALLY_EXCLUSIVE_CONTAINER parameters: Table 3.5.1-2, MutuallyExclusiveContainer Creation Structure.
 */
static const BinaryField_t c_mutuallyExclusiveContainerFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("VisibleChild"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_NO_SERVICE_MONITOR parameters: Table 3.8.10-2, NoServiceMonitor Creation Structure.
 */
static const BinaryField_t c_noServiceMonitorFields[] =
{
    {("ShowNoServiceIdent"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_NUMERIC_READOUT parameters: Table 3.8.3-2, NumericReadout Creation Structure.
 */
static const BinaryField_t c_numericReadoutFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_labelAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("Value"),                         BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_legendPositionValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("FormatString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("LegendString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PAGING_CONTAINER parameters: Table 3.8.2-2, PagingContainer Creation Structure.
 */
static const BinaryField_t c_pagingContainerFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("WrappingType"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_wrappingTypeValues)},
    {("PagingControlPosition"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_pagingControlPositionValues)},
    {("FinePageDelta"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("CoarsePageDelta"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("VisibleChild"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ReportVisibleChild"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_PANEL parameters: Table 3.3.27-2, Panel Creation Structure.
 */
static const BinaryField_t c_panelFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE parameters: Table 3.3.19-2, Picture Creation Structure.
 */
static const BinaryField_t c_pictureFields[] =
{
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_ANIMATED parameters: Table 3.5.5-2, PictureAnimated Creation Structure.
 */
static const BinaryField_t c_pictureAnimatedFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfPictures"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("IndexOfFrequency"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("LoopFlag"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AnimationFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U,  7U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_PUSH_BUTTON parameters: Table 3.3.28-2, Picture PushButton Creation Structure.
 */
static const BinaryField_t c_picturePushButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_picturePositionValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_TOGGLE_BUTTON parameters: Table 3.3.29-2, PictureToggleButton Creation Structure.
 */
static const BinaryField_t c_pictureToggleButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AlternatePictureReference"),     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_picturePositionValues)},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_toggleStateValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_MENU parameters: Table 3.3.31-2, PopUpMenu Creation Structure.
 */
static const BinaryField_t c_popUpMenuFields[] =
{
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_popUpMenuOpeningModeValues)},
    {("NumberOfEntries"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PopUpIdentArray"),               BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL, 0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL, 0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U,  1U, 1U, NULL, 0U},
    {("StringArray"),                   BINARY_FIELD_STRING,       0U,  1U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_MENU_BUTTON parameters: Table 3.3.32-2, PopUpMenuButton Creation Structure.
 */
static const BinaryField_t c_popUpMenuButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PopupPosX"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PopupPosY"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PopupSizeX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("PopupSizeY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLengthPopUp"),          BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_popUpButtonPicturePositionValues)},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_popUpMenuButtonOpeningModeValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL, 0U},
    {("PopUpIdentArray"),               BINARY_FIELD_USHORT,       2U, 15U, 1U, NULL, 0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U, 15U, 1U, NULL, 0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U, 15U, 1U, NULL, 0U},
    {("StringArray"),                   BINARY_FIELD_STRING,       0U, 15U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_PANEL parameters: Table 3.3.30-2, PopUpPanel Creation Structure.
 */
static const BinaryField_t c_popUpPanelFields[] =
{
    {("UAPositionFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AutomaticClosure"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_PANEL_BUTTON parameters: Table 3.7.3-2, PopUpPanelButton Creation Structure.
 */
static const BinaryField_t c_popUpPanelButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PopupPosX"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PopupPosY"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PopupSizeX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("PopupSizeY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_popUpButtonPicturePositionValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {("UAPositionFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AutomaticClosure"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PROXY_BUTTON parameters: Table 3.5.2-2, ProxyButton Creation Structure.
 */
static const BinaryField_t c_proxyButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("DedicatedKeyIdent"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TargetWidgetIdent"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PUSH_BUTTON parameters: Table 3.3.33-2, PushButton Creation Structure.
 */
static const BinaryField_t c_pushButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_pushButtonAlignmentValues)},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_RADIO_BOX parameters: Table 3.3.34-2, RadioBox Creation Structure.
 */
static const BinaryField_t c_radioBoxFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_ROTATION_CONTAINER parameters: Table 3.3.35-2, RotationContainer Creation Structure.
 */
static const BinaryField_t c_rotationContainerFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("CenterX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("CenterY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SCALE_CONTAINER parameters: Table 3.10.1-2, ScaleContainer Creation Structure.
 */
static const BinaryField_t c_scaleContainerFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("CenterX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("CenterY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("ScaleX"),                        BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("ScaleY"),                        BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SCROLL_LIST parameters: Table 3.3.37-2, ScrollList Creation Structure.
 */
static const BinaryField_t c_scrollListFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("FirstAccessibleEntry"),          BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("FirstVisibleEntry"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_scrollBarPositionValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_pushButtonAlignmentValues)},
    {("FlagReportVisibleEntry"),        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("DefaultStyleText"),              BINARY_FIELD_BYTES,       12U,  0U, 0U, NULL, 0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U,  9U, 1U, NULL, 0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,       0U,  9U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SCROLL_PANEL parameters: Table 3.3.36-2, ScrollPanel Creation Structure.
 */
static const BinaryField_t c_scrollPanelFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("LineDeltaX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("LineDeltaY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("PageDeltaX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("PageDeltaY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("HomeX"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("HomeY"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("FrameX"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("FrameY"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeXsheet"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeYsheet"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("BoundX"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("BoundY"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeXbound"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeYbound"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("HorizontalScroll"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_scrollBarPositionValues)},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_scrollBarPositionValues)},
    {("FlagReportFramePos"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SCROLL_WHEEL_AREA parameters: Table 3.9.3-2, ScrollWheelArea Creation Structure.
 */
static const BinaryField_t c_scrollWheelAreaFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SELECTION_LIST_BUTTON parameters: Table 3.5.7-2, SelectionListButton Creation Structure.
 */
static const BinaryField_t c_selectionListButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_comboBoxOpeningModeValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL, 0U},
    {("EntryList"),                     BINARY_FIELD_STRING,       0U, 11U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SHUFFLE_TO_FIT_CONTAINER parameters: Table 3.6.6-2, ShuffleToFitContainer Creation Structure.
 */
static const BinaryField_t c_shuffleToFitContainerFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("NumberOfVisibleChildren"),       BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ShuffleToFitMode"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_shuffleToFitModeValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("ItemSpacing"),                   BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SIZE_TO_FIT_CONTAINER parameters: Table 3.6.5-2, SizeToFitContainer Creation Structure.
 */
static const BinaryField_t c_sizeToFitContainerFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("NumberOfVisibleChildren"),       BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SizeToFitMode"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("ItemSpacing"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SLIDER parameters: Table 3.5.4-2, Slider Creation Structure.
 */
static const BinaryField_t c_sliderFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MinValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("MaxValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("Value"),                         BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("MajorTickInterval"),             BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U},
    {("ShowMajorLabels"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_trueFalseValues)},
    {("Orientation"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_sliderOrientationValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_sliderAlignmentValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("LsbMultiple"),                   BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MinorTickMultiple"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MajorTickReference"),            BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL parameters: Table 3.3.38-2, Symbol Creation Structure.
 */
static const BinaryField_t c_symbolFields[] =
{
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_ANIMATED parameters: Table 3.5.6-2, SymbolAnimated Creation Structure.
 */
static const BinaryField_t c_symbolAnimatedFields[] =
{
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("LoopType"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_loopTypeValues)},
    {("NumberOfSymbols"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("IndexOfFrequency"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("AnimationType"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_animationTypeValues)},
    {("SymbolArray"),                   BINARY_FIELD_USHORT,       2U,  7U, 1U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL, 0U},
    {("OrientationArray"),              BINARY_FIELD_FR180,        4U,  7U, 1U, NULL, 0U},
    {("MovementXArray"),                BINARY_FIELD_LONG,         4U,  7U, 1U, NULL, 0U},
    {("MovementYArray"),                BINARY_FIELD_LONG,         4U,  7U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_PUSH_BUTTON parameters: Table 3.7.1-2, SymbolPushButton Creation Structure.
 */
static const BinaryField_t c_symbolPushButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_picturePositionValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_TOGGLE_BUTTON parameters: Table 3.7.2-2, SymbolToggleButton Creation Structure.
 */
static const BinaryField_t c_symbolToggleButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("SymbolReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AlternateSymbolReference"),      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_picturePositionValues)},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_toggleStateValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_buttonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TABBED_PANEL parameters: Table 3.3.39-2, TabbedPanel Creation Structure.
 */
static const BinaryField_t c_tabbedPanelFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_popUpButtonPicturePositionValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_pushButtonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("InsetSize"),                     BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TABBED_PANEL_GROUP parameters: Table 3.3.40-2, TabbedPanelGroup Creation Structure.
 */
static const BinaryField_t c_tabbedPanelGroupFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ActiveTabbedPanelID"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TabPosition"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_tabPositionValues)},
    {("AutomaticInsetSizeFlag"),        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TOGGLE_BUTTON parameters: Table 3.3.41-2, ToggleButton Creation Structure.
 */
static const BinaryField_t c_toggleButtonFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_toggleStateValues)},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_pushButtonAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,       0U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TOUCH_AREA parameters: Table 3.9.5-2, TouchArea Creation Structure.
 */
static const BinaryField_t c_touchAreaFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_positionReportModeValues)},
    {("AllowOutside"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("MaxTouchPoints"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("NumberOfTouchPoints"),           BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TRANSLATION_CONTAINER parameters: Table 3.3.42-2, TranslationContainer Creation Structure.
 */
static const BinaryField_t c_translationContainerFields[] =
{
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("TranslationX"),                  BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("TranslationY"),                  BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TREE parameters: Table 3.10.3-2, Tree Creation Structure.
 */
static const BinaryField_t c_treeFields[] =
{
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_enableValues)},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL, 0U},
    {("FirstVisibleEntry"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("FlagReportVisibleEntry"),        BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxAlignmentValues)},
    {("NodeStyleArray"),                BINARY_FIELD_USHORT,       2U, 12U, 1U, NULL, 0U},
    {("ParentArray"),                   BINARY_FIELD_USHORT,       2U, 12U, 1U, NULL, 0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U, 12U, 1U, NULL, 0U},
    {("OpenArray"),                     BINARY_FIELD_UCHAR,        1U, 12U, 1U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL, 0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,       0U, 12U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_WATCHDOG_CONTAINER parameters: Table 3.5.3-2, WatchdogContainer Creation Structure.
 */
static const BinaryField_t c_watchdogContainerFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("TimePeriod"),                    BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ValidCountLimit"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("FailCountLimit"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("Refresh"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("ShowIfFailIdent"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ShowFail"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_trueFalseValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_DIRECTIONAL_TABBING_EXTENSION parameters: Table 8.5.1-2, DirectionalTabbingExtension Creation Structure.
 */
static const BinaryField_t c_directionalTabbingExtensionFields[] =
{
    {("NumDirections"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("TabDirectionWidgetIdArray"),     BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_EVENTS_EXTENSION parameters: Table 8.5.2-2, CursorEventsExtension Creation Structure.
 */
static const BinaryField_t c_cursorEventsExtensionFields[] =
{
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_LEGEND_ALIGN_EXTENSION parameters: Table 8.5.3-2, LegendStringAlignment Creation Structure.
 */
static const BinaryField_t c_legendAlignExtensionFields[] =
{
    {("LegendAlignment"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_editBoxAlignmentValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CHILD_INDEX_EXTENSION parameters: Table 8.5.4-2, VisibleChildIndexExtension Creation Structure.
 */
static const BinaryField_t c_childIndexExtensionFields[] =
{
    {("VisibleChildIndex"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_INITIAL_FOCUS_EXTENSION parameters: Table 8.5.5-2, InitialFocusExtension Creation Structure.
 */
static const BinaryField_t c_initialFocusExtensionFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("InitialFocus"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_STOP_EXTENSION parameters: Table 8.5.6-2, FocusStopExtension Creation Structure.
 */
static const BinaryField_t c_focusStopExtensionFields[] =
{
    {("StopForward"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("StopBackward"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_SHAPE_EXTENSION parameters: Table 8.5.7-2, CursorShapeExtension Creation Structure.
 */
static const BinaryField_t c_cursorShapeExtensionFields[] =
{
    {("AllowChange"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_booleanValues)},
    {("CursorIndex"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_EXTENSION parameters: Table 8.6.1-2, PictureExtension Creation Structure.
 */
static const BinaryField_t c_pictureExtensionFields[] =
{
    {("MaxNumberOfPictures"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfPictures"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_picturePositionValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_EXTENSION parameters: Table 8.6.2-2, SymbolExtension Creation Structure.
 */
static const BinaryField_t c_symbolExtensionFields[] =
{
    {("MaxNumberOfSymbols"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfSymbols"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_picturePositionValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("SymbolArray"),                   BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_TICS_ARRAY_EXTENSION parameters: Table 8.6.3-2, TicsArrayExtension Creation Structure.
 */
static const BinaryField_t c_ticsArrayExtensionFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfTicsCoarse"),         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfTicsCoarse"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("MaxNumberOfTicsFine"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("NumberOfTicsFine"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("TicsCoarseArray"),               BINARY_FIELD_FLOAT,        4U,  2U, 2U, NULL, 0U},
    {("TicsFineArray"),                 BINARY_FIELD_FLOAT,        4U,  4U, 2U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_STYLE_SET_EXTENSION parameters: Table 8.6.4-2, StyleSetExtension Creation Structure.
 */
static const BinaryField_t c_styleSetExtensionFields[] =
{
    {("MaxStyleSetArraySize"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("StyleSetArraySize"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("StyleSetArray"),                 BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_STATIC_PARAM_BUFFER_EXTENSION parameters: Table 8.6.5-2, StaticParamBufferExtension Creation Structure.
 */
static const BinaryField_t c_staticParamBufferExtensionFields[] =
{
    {("NumberOfBuffers"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("BufferArray"),                   BINARY_FIELD_SIZED_BYTES,  0U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MULTI_SELECTION_EXTENSION parameters: Table 8.6.6-2, MultiSelectionExtension Creation Structure.
 */
static const BinaryField_t c_multiSelectionExtensionFields[] =
{
    {("MaximumSelectableEntries"),      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_EXCLUDED_REGIONS_EXTENSION parameters: Table 8.6.7-2, ExcludedRegionsExtension Creation Structure.
 */
static const BinaryField_t c_excludedRegionsExtensionFields[] =
{
    {("MaxNumberOfRegions"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U},
    {("EffectivityArray"),              BINARY_FIELD_UCHAR,        1U,  0U, 1U, NULL, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL, 0U},
    {("ExcludedRegionsList"),           BINARY_FIELD_REGION,       0U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_BOUNDARY_CHECK_EXTENSION parameters: Table 8.6.8-2, BoundaryCheckExtension Creation Structure.
 */
static const BinaryField_t c_boundaryCheckExtensionFields[] =
{
    {("BoundaryCheckMode"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_boundaryCheckModeValues)},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_REORDER_EXTENSION parameters: Table 8.7.1-2, ReorderExtension Creation Structure.
 */
static const BinaryField_t c_reorderExtensionFields[] =
{
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_WORD_WRAP_EXTENSION parameters: Table 8.7.2-2, WordWrapExtension Creation Structure.
 */
static const BinaryField_t c_wordWrapExtensionFields[] =
{
    {("WordWrap"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U},
    {("WrapStyle"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_ENTRY_EVENTS_EXTENSION parameters: Table 8.7.3-2, CursorEntryEventsExtension Creation Structure.
 */
static const BinaryField_t c_cursorEntryEventsExtensionFields[] =
{
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION parameters: Table 8.7.4-2, MapHorzCoordSystemEventExtension Creation Structure.
 */
static const BinaryField_t c_maphorzCoordSystemEventExtensionFields[] =
{
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_coordSystemReportModeValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION parameters: Table 8.7.5-2, MapVertCoordSystemEventExtension Creation Structure.
 */
static const BinaryField_t c_mapvertCoordSystemEventExtensionFields[] =
{
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL, 0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, ARRAY_AND_COUNT(c_coordSystemReportModeValues)},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL, 0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL, 0U}
};

/*************************************************************************************************/
/**
 * \brief Supported widget types in name order, with WidgetType codes from Table 4.6-7.
 */
static const BinaryWidget_t c_binaryWidgets[] =
{
    {("A661_ACTIVE_AREA"),                    0xA010U, ARRAY_AND_COUNT(c_activeAreaFields)},
    {("A661_ANIMATION_GROUP"),                0xA1E3U, ARRAY_AND_COUNT(c_animationGroupFields)},
    {("A661_ANIMATION_ONPARAM"),              0xA1E4U, ARRAY_AND_COUNT(c_animationOnparamFields)},
    {("A661_ANIMATION_ROTATION"),             0xA1E5U, ARRAY_AND_COUNT(c_animationRotationFields)},
    {("A661_ANIMATION_SCALE"),                0xA1E6U, ARRAY_AND_COUNT(c_animationScaleFields)},
    {("A661_ANIMATION_TRANSLATION"),          0xA1E7U, ARRAY_AND_COUNT(c_animationTranslationFields)},
    {("A661_BASIC_CONTAINER"),                0xA020U, ARRAY_AND_COUNT(c_basicContainerFields)},
    {("A661_BLINKING_CONTAINER"),             0xA030U, ARRAY_AND_COUNT(c_blinkingContainerFields)},
    {("A661_BROADCAST_RECEIVER"),             0xA032U, ARRAY_AND_COUNT(c_broadcastReceiverFields)},
    {("A661_BUFFER_FORMAT"),                  0xA040U, ARRAY_AND_COUNT(c_bufferFormatFields)},
    {("A661_CHECK_BUTTON"),                   0xA050U, ARRAY_AND_COUNT(c_checkButtonFields)},
    {("A661_COMBO_BOX"),                      0xA070U, ARRAY_AND_COUNT(c_comboBoxFields)},
    {("A661_COMBO_BOX_EDIT"),                 0xA0E0U, ARRAY_AND_COUNT(c_comboBoxEditFields)},
    {("A661_CONNECTOR"),                      0xA080U, ARRAY_AND_COUNT(c_connectorFields)},
    {("A661_CURSOR_OVER"),                    0xA480U, ARRAY_AND_COUNT(c_cursorOverFields)},
    {("A661_CURSOR_POS_OVERLAY"),             0xA090U, ARRAY_AND_COUNT(c_cursorPosOverlayFields)},
    {("A661_CURSOR_REF"),                     0xA470U, ARRAY_AND_COUNT(c_cursorRefFields)},
    {("A661_DATA_CONNECTOR"),                 0xA088U, ARRAY_AND_COUNT(c_dataConnectorFields)},
    {("A661_DATA_SCALING_FR180"),             0xA2E0U, ARRAY_AND_COUNT(c_dataScalingFr180Fields)},
    {("A661_DATA_SCALING_LONG"),              0xA2D8U, ARRAY_AND_COUNT(c_dataScalingLongFields)},
    {("A661_DATA_SCALING_ULONG"),             0xA2D9U, ARRAY_AND_COUNT(c_dataScalingUlongFields)},
    {("A661_EDIT_BOX_MASKED"),                0xA0A0U, ARRAY_AND_COUNT(c_editBoxMaskedFields)},
    {("A661_EDIT_BOX_MULTI_LINE"),            0xA0B0U, ARRAY_AND_COUNT(c_editBoxMultiLineFields)},
    {("A661_EDIT_BOX_NUMERIC"),               0xA0C0U, ARRAY_AND_COUNT(c_editBoxNumericFields)},
    {("A661_EDIT_BOX_NUMERIC_BCD"),           0xA0C2U, ARRAY_AND_COUNT(c_editBoxNumericBcdFields)},
    {("A661_EDIT_BOX_TEXT"),                  0xA0D0U, ARRAY_AND_COUNT(c_editBoxTextFields)},
    {("A661_EVENT_HANDLER"),                  0xA600U, ARRAY_AND_COUNT(c_eventHandlerFields)},
    {("A661_EXTERNAL_SOURCE"),                0xA188U, ARRAY_AND_COUNT(c_externalSourceFields)},
    {("A661_FOCUS_IN"),                       0xA498U, ARRAY_AND_COUNT(c_focusInFields)},
    {("A661_FOCUS_LINK"),                     0xA490U, ARRAY_AND_COUNT(c_focusLinkFields)},
    {("A661_FOCUS_OUT"),                      0xA499U, ARRAY_AND_COUNT(c_focusOutFields)},
    {("A661_FRAMING_REFRESH_CONTAINER"),      0xA380U, ARRAY_AND_COUNT(c_framingRefreshContainerFields)},
    {("A661_GESTURE_AREA"),                   0xA501U, ARRAY_AND_COUNT(c_gestureAreaFields)},
    {("A661_GP_ARC_CIRCLE"),                  0xA0F0U, ARRAY_AND_COUNT(c_gpArcCircleFields)},
    {("A661_GP_ARC_ELLIPSE"),                 0xA100U, ARRAY_AND_COUNT(c_gpArcEllipseFields)},
    {("A661_GP_CROWN"),                       0xA110U, ARRAY_AND_COUNT(c_gpCrownFields)},
    {("A661_GP_LINE"),                        0xA120U, ARRAY_AND_COUNT(c_gpLineFields)},
    {("A661_GP_LINE_POLAR"),                  0xA130U, ARRAY_AND_COUNT(c_gpLinePolarFields)},
    {("A661_GP_POLYLINE"),                    0xA138U, ARRAY_AND_COUNT(c_gpPolylineFields)},
    {("A661_GP_RECTANGLE"),                   0xA140U, ARRAY_AND_COUNT(c_gpRectangleFields)},
    {("A661_GP_TRIANGLE"),                    0xA150U, ARRAY_AND_COUNT(c_gpTriangleFields)},
    {("A661_GP_TRIANGLE_FAN"),                0xA151U, ARRAY_AND_COUNT(c_gpTriangleFanFields)},
    {("A661_GP_TRIANGLE_STRIP"),              0xA152U, ARRAY_AND_COUNT(c_gpTriangleStripFields)},
    {("A661_INK_AREA"),                       0xA502U, ARRAY_AND_COUNT(c_inkAreaFields)},
    {("A661_KEYBOARD_AREA"),                  0xA1E1U, ARRAY_AND_COUNT(c_keyboardAreaFields)},
    {("A661_LABEL"),                          0xA160U, ARRAY_AND_COUNT(c_labelFields)},
    {("A661_LABEL_COMPLEX"),                  0xA170U, ARRAY_AND_COUNT(c_labelComplexFields)},
    {("A661_MAPHORZ"),                        0xA1B0U, ARRAY_AND_COUNT(c_maphorzFields)},
    {("A661_MAPHORZ_CONTAINER"),              0xA1B8U, ARRAY_AND_COUNT(c_maphorzContainerFields)},
    {("A661_MAPHORZ_ITEMLIST"),               0xA180U, ARRAY_AND_COUNT(c_maphorzItemlistFields)},
    {("A661_MAPHORZ_PANEL"),                  0xA189U, ARRAY_AND_COUNT(c_maphorzPanelFields)},
    {("A661_MAPHORZ_SOURCE"),                 0xA1A0U, ARRAY_AND_COUNT(c_maphorzSourceFields)},
    {("A661_MAPHORZ_VERTEXBUFFER"),           0xA1A9U, ARRAY_AND_COUNT(c_maphorzVertexbufferFields)},
    {("A661_MAPVERT"),                        0xA1B2U, ARRAY_AND_COUNT(c_mapvertFields)},
    {("A661_MAPVERT_CONTAINER"),              0xA1B5U, ARRAY_AND_COUNT(c_mapvertContainerFields)},
    {("A661_MAPVERT_ITEMLIST"),               0xA1B3U, ARRAY_AND_COUNT(c_mapvertItemlistFields)},
    {("A661_MAPVERT_PANEL"),                  0xA1B6U, ARRAY_AND_COUNT(c_mapvertPanelFields)},
    {("A661_MAPVERT_SOURCE"),                 0xA1B4U, ARRAY_AND_COUNT(c_mapvertSourceFields)},
    {("A661_MAP_BOUNDARY"),                   0xA1BAU, ARRAY_AND_COUNT(c_mapBoundaryFields)},
    {("A661_MAP_EXTERNAL_SOURCE"),            0xA224U, ARRAY_AND_COUNT(c_mapExternalSourceFields)},
    {("A661_MAP_GRID"),                       0xA178U, ARRAY_AND_COUNT(c_mapGridFields)},
    {("A661_MASK_CONTAINER"),                 0xA1C0U, ARRAY_AND_COUNT(c_maskContainerFields)},
    {("A661_MENU_BAR"),                       0xA1D0U, ARRAY_AND_COUNT(c_menuBarFields)},
    {("A661_MULTI_STATE_BUTTON"),             0xA348U, ARRAY_AND_COUNT(c_multiStateButtonFields)},
    {("A661_MUTUALLY_EXCLUSIVE_CONTAINER"),   0xA400U, ARRAY_AND_COUNT(c_mutuallyExclusiveContainerFields)},
    {("A661_NO_SERVICE_MONITOR"),             0xA4F0U, ARRAY_AND_COUNT(c_noServiceMonitorFields)},
    {("A661_NUMERIC_READOUT"),                0xA406U, ARRAY_AND_COUNT(c_numericReadoutFields)},
    {("A661_PAGING_CONTAINER"),               0xA408U, ARRAY_AND_COUNT(c_pagingContainerFields)},
    {("A661_PANEL"),                          0xA1F0U, ARRAY_AND_COUNT(c_panelFields)},
    {("A661_PICTURE"),                        0xA200U, ARRAY_AND_COUNT(c_pictureFields)},
    {("A661_PICTURE_ANIMATED"),               0xA210U, ARRAY_AND_COUNT(c_pictureAnimatedFields)},
    {("A661_PICTURE_PUSH_BUTTON"),            0xA240U, ARRAY_AND_COUNT(c_picturePushButtonFields)},
    {("A661_PICTURE_TOGGLE_BUTTON"),          0xA250U, ARRAY_AND_COUNT(c_pictureToggleButtonFields)},
    {("A661_POP_UP_MENU"),                    0xA270U, ARRAY_AND_COUNT(c_popUpMenuFields)},
    {("A661_POP_UP_MENU_BUTTON"),             0xA280U, ARRAY_AND_COUNT(c_popUpMenuButtonFields)},
    {("A661_POP_UP_PANEL"),                   0xA290U, ARRAY_AND_COUNT(c_popUpPanelFields)},
    {("A661_POP_UP_PANEL_BUTTON"),            0xA4E0U, ARRAY_AND_COUNT(c_popUpPanelButtonFields)},
    {("A661_PROXY_BUTTON"),                   0xA420U, ARRAY_AND_COUNT(c_proxyButtonFields)},
    {("A661_PUSH_BUTTON"),                    0xA2A0U, ARRAY_AND_COUNT(c_pushButtonFields)},
    {("A661_RADIO_BOX"),                      0xA2B0U, ARRAY_AND_COUNT(c_radioBoxFields)},
    {("A661_ROTATION_CONTAINER"),             0xA2D0U, ARRAY_AND_COUNT(c_rotationContainerFields)},
    {("A661_SCALE_CONTAINER"),                0xA2F2U, ARRAY_AND_COUNT(c_scaleContainerFields)},
    {("A661_SCROLL_LIST"),                    0xA2F0U, ARRAY_AND_COUNT(c_scrollListFields)},
    {("A661_SCROLL_PANEL"),                   0xA300U, ARRAY_AND_COUNT(c_scrollPanelFields)},
    {("A661_SCROLL_WHEEL_AREA"),              0xA1E2U, ARRAY_AND_COUNT(c_scrollWheelAreaFields)},
    {("A661_SELECTION_LIST_BUTTON"),          0xA370U, ARRAY_AND_COUNT(c_selectionListButtonFields)},
    {("A661_SHUFFLE_TO_FIT_CONTAINER"),       0xA4B0U, ARRAY_AND_COUNT(c_shuffleToFitContainerFields)},
    {("A661_SIZE_TO_FIT_CONTAINER"),          0xA4A0U, ARRAY_AND_COUNT(c_sizeToFitContainerFields)},
    {("A661_SLIDER"),                         0xA440U, ARRAY_AND_COUNT(c_sliderFields)},
    {("A661_SYMBOL"),                         0xA310U, ARRAY_AND_COUNT(c_symbolFields)},
    {("A661_SYMBOL_ANIMATED"),                0xA450U, ARRAY_AND_COUNT(c_symbolAnimatedFields)},
    {("A661_SYMBOL_PUSH_BUTTON"),             0xA4C0U, ARRAY_AND_COUNT(c_symbolPushButtonFields)},
    {("A661_SYMBOL_TOGGLE_BUTTON"),           0xA4D0U, ARRAY_AND_COUNT(c_symbolToggleButtonFields)},
    {("A661_TABBED_PANEL"),                   0xA320U, ARRAY_AND_COUNT(c_tabbedPanelFields)},
    {("A661_TABBED_PANEL_GROUP"),             0xA330U, ARRAY_AND_COUNT(c_tabbedPanelGroupFields)},
    {("A661_TOGGLE_BUTTON"),                  0xA340U, ARRAY_AND_COUNT(c_toggleButtonFields)},
    {("A661_TOUCH_AREA"),                     0xA500U, ARRAY_AND_COUNT(c_touchAreaFields)},
    {("A661_TRANSLATION_CONTAINER"),          0xA360U, ARRAY_AND_COUNT(c_translationContainerFields)},
    {("A661_TREE"),                           0xA222U, ARRAY_AND_COUNT(c_treeFields)},
    {("A661_WATCHDOG_CONTAINER"),             0xA460U, ARRAY_AND_COUNT(c_watchdogContainerFields)}
};

/*************************************************************************************************/
/**
 * \brief Supported widget extension types, with ExtensionType codes from Table 4.6-12.
 */
static const BinaryWidget_t c_binaryExtensions[] =
{
    {("A661_DIRECTIONAL_TABBING_EXTENSION"),        0x8003U, ARRAY_AND_COUNT(c_directionalTabbingExtensionFields)},
    {("A661_CURSOR_EVENTS_EXTENSION"),              0x8002U, ARRAY_AND_COUNT(c_cursorEventsExtensionFields)},
    {("A661_LEGEND_ALIGN_EXTENSION"),               0x8004U, ARRAY_AND_COUNT(c_legendAlignExtensionFields)},
    {("A661_CHILD_INDEX_EXTENSION"),                0x8001U, ARRAY_AND_COUNT(c_childIndexExtensionFields)},
    {("A661_INITIAL_FOCUS_EXTENSION"),              0x8006U, ARRAY_AND_COUNT(c_initialFocusExtensionFields)},
    {("A661_FOCUS_STOP_EXTENSION"),                 0x8007U, ARRAY_AND_COUNT(c_focusStopExtensionFields)},
    {("A661_CURSOR_SHAPE_EXTENSION"),               0x8008U, ARRAY_AND_COUNT(c_cursorShapeExtensionFields)},
    {("A661_PICTURE_EXTENSION"),                    0x8009U, ARRAY_AND_COUNT(c_pictureExtensionFields)},
    {("A661_SYMBOL_EXTENSION"),                     0x800AU, ARRAY_AND_COUNT(c_symbolExtensionFields)},
    {("A661_TICS_ARRAY_EXTENSION"),                 0x800BU, ARRAY_AND_COUNT(c_ticsArrayExtensionFields)},
    {("A661_STYLE_SET_EXTENSION"),                  0x800CU, ARRAY_AND_COUNT(c_styleSetExtensionFields)},
    {("A661_STATIC_PARAM_BUFFER_EXTENSION"),        0x800DU, ARRAY_AND_COUNT(c_staticParamBufferExtensionFields)},
    {("A661_MULTI_SELECTION_EXTENSION"),            0x800FU, ARRAY_AND_COUNT(c_multiSelectionExtensionFields)},
    {("A661_EXCLUDED_REGIONS_EXTENSION"),           0x800EU, ARRAY_AND_COUNT(c_excludedRegionsExtensionFields)},
    {("A661_BOUNDARY_CHECK_EXTENSION"),             0x8010U, ARRAY_AND_COUNT(c_boundaryCheckExtensionFields)},
    {("A661_REORDER_EXTENSION"),                    0x8011U, ARRAY_AND_COUNT(c_reorderExtensionFields)},
    {("A661_WORD_WRAP_EXTENSION"),                  0x8012U, ARRAY_AND_COUNT(c_wordWrapExtensionFields)},
    {("A661_CURSOR_ENTRY_EVENTS_EXTENSION"),        0x8013U, ARRAY_AND_COUNT(c_cursorEntryEventsExtensionFields)},
    {("A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION"), 0x8014U, ARRAY_AND_COUNT(c_maphorzCoordSystemEventExtensionFields)},
    {("A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION"), 0x8015U, ARRAY_AND_COUNT(c_mapvertCoordSystemEventExtensionFields)}
};

/*************************************************************************************************/
/**
 * \brief Constant names accepted as property values, from Tables 4.6-8, 4.6-10, 4.6-11, 4.6-13, 4.6-14, and 4.6-15.
 */
static const BinaryConstant_t c_binaryConstants[] =
{
    {("A661_ABORT"),                                      0x02U},
    {("A661_ABORTED"),                                    0x02U},
    {("A661_ABSENT"),                                     0x10U},
    {("A661_ACTIVE_TABBED_PANEL"),                        0xB050U},
    {("A661_AC_LAT"),                                     0xB010U},
    {("A661_AC_LAT_LONG"),                                0xB030U},
    {("A661_AC_LONG"),                                    0xB020U},
    {("A661_AC_ORIENTATION"),                             0xB040U},
    {("A661_ALIGNMENT"),                                  0xB49AU},
    {("A661_ALLOW_CHANGE"),                               0xB6A1U},
    {("A661_ALPHA_MASK"),                                 0xB060U},
    {("A661_ALTERNATE_FLAG"),                             0xB088U},
    {("A661_ALTERN_PICTURE_REFERENCE"),                   0xB080U},
    {("A661_ALTERN_SYMBOL_REFERENCE"),                    0xB4C1U},
    {("A661_ANIMATION_DELAY"),                            0xB702U},
    {("A661_ANIMATION_DURATION"),                         0xB703U},
    {("A661_ANIMATION_FLAG"),                             0xB090U},
    {("A661_ANIMATION_LAW_REF"),                          0xB704U},
    {("A661_ANIMATION_REPETITION"),                       0xB705U},
    {("A661_ANIMATION_STATE"),                            0xB706U},
    {("A661_ANIMATION_TYPE"),                             0xB098U},
    {("A661_APP_DATA"),                                   0xB6D0U},
    {("A661_ARC"),                                        0x01U},
    {("A661_ASCII_EXTENDED"),                             0x01U},
    {("A661_AUTO_CLOSURE"),                               0xB1C2U},
    {("A661_AUTO_FOCUS_MOTION"),                          0xB2B9U},
    {("A661_BLINKING_TYPE"),                              0xB0A8U},
    {("A661_BLOCKED_BUFFER_OF_MAPITEM"),                  0xB121U},
    {("A661_BLOCKED_BUFFER_OF_MAPVERT_ITEMS"),            0xB126U},
    {("A661_BOTH_CLAMPED"),                               0x01U},
    {("A661_BOTTOM"),                                     0x12U},
    {("A661_BOTTOM_CENTER"),                              0x30U},
    {("A661_BOTTOM_LEFT"),                                0x31U},
    {("A661_BOTTOM_RIGHT"),                               0x32U},
    {("A661_BOTTOM_TO_TOP"),                              0x40U},
    {("A661_BOUNDARY_ARRAY"),                             0xB6C1U},
    {("A661_BOUNDARY_CHECK"),                             0xBCU},
    {("A661_BOUNDARY_CHECK_MODE"),                        0xB122U},
    {("A661_BOUND_SIZE_X"),                               0xB0E0U},
    {("A661_BOUND_SIZE_XY"),                              0xB100U},
    {("A661_BOUND_SIZE_Y"),                               0xB0F0U},
    {("A661_BOUND_X"),                                    0xB0B0U},
    {("A661_BOUND_XY"),                                   0xB0D0U},
    {("A661_BOUND_Y"),                                    0xB0C0U},
    {("A661_BROADCAST_DATA"),                             0xB0C2U},
    {("A661_BUFFER_OF_FILL_STYLES"),                      0xB0F8U},
    {("A661_BUFFER_OF_MAPITEM"),                          0xB120U},
    {("A661_BUFFER_OF_MAPVERT_ITEMS"),                    0xB125U},
    {("A661_BUFFER_OF_PARAM"),                            0xB110U},
    {("A661_BUTTON_POS"),                                 0xB641U},
    {("A661_BUTTON_SIZE"),                                0xB642U},
    {("A661_CACHING_ENABLE"),                             0xB117U},
    {("A661_CDS_DEPENDENT"),                              0x1EU},
    {("A661_CENTER"),                                     0x15U},
    {("A661_CENTER_X"),                                   0xB130U},
    {("A661_CENTER_XY"),                                  0xB150U},
    {("A661_CENTER_Y"),                                   0xB140U},
    {("A661_CIRCULAR"),                                   0x02U},
    {("A661_CLAMPING"),                                   0xB501U},
    {("A661_CLEAR"),                                      0xB000U},
    {("A661_CLOCKWISE"),                                  0x00U},
    {("A661_CLOSED"),                                     0xB1E2U},
    {("A661_COARSE_PAGE_DELTA"),                          0xB6E3U},
    {("A661_COLOR_INDEX"),                                0xB160U},
    {("A661_COMPLETED"),                                  0x01U},
    {("A661_COUNTER_CLOCKWISE"),                          0x01U},
    {("A661_CRP_LAT"),                                    0xB162U},
    {("A661_CRP_LAT_LONG"),                               0xB166U},
    {("A661_CRP_LONG"),                                   0xB164U},
    {("A661_CURSOR_CLICKED"),                             0xB1D3U},
    {("A661_CURSOR_DOUBLE_CLICKED"),                      0xB1D4U},
    {("A661_CURSOR_INDEX"),                               0xB6A0U},
    {("A661_CURSOR_POS"),                                 0xB170U},
    {("A661_CURSOR_POS_BYTE"),                            0xB172U},
    {("A661_CURSOR_PRESSED"),                             0xB1D1U},
    {("A661_CURSOR_RELEASED"),                            0xB1D2U},
    {("A661_CURSOR_RIGHT_CLICKED"),                       0xB1D5U},
    {("A661_DEACTIVATE"),                                 0x04U},
    {("A661_DIRECTION"),                                  0xB713U},
    {("A661_DONT_RUN"),                                   0x00U},
    {("A661_DOWN"),                                       0x1AU},
    {("A661_DRAW_LINE_TO_CURSOR"),                        0x36U},
    {("A661_EASING_IN"),                                  0x01U},
    {("A661_EASING_INOUT"),                               0x03U},
    {("A661_EASING_OUT"),                                 0x02U},
    {("A661_EDB_ALL_CHANGE"),                             0x01U},
    {("A661_EDB_CHANGE_CONFIRMED"),                       0x00U},
    {("A661_EDB_OPEN_CLOSE"),                             0x02U},
    {("A661_EDITING"),                                    0x1BU},
    {("A661_EFFECTIVITY_ARRAY"),                          0xB6C8U},
    {("A661_ENABLE"),                                     0xB180U},
    {("A661_ENABLE_ENTRY_ARRAY"),                         0xB1A8U},
    {("A661_ENABLE_HANDLER"),                             0xB1A1U},
    {("A661_END"),                                        0x03U},
    {("A661_END_ANGLE"),                                  0xB1B0U},
    {("A661_END_SCALED_VALUE"),                           0xB505U},
    {("A661_END_VALUE"),                                  0xB503U},
    {("A661_ENTRY_ARRAY"),                                0xB190U},
    {("A661_ENTRY_POP_UP_ARRAY"),                         0xB1C0U},
    {("A661_ENTRY_VALID"),                                0xB570U},
    {("A661_ERROR"),                                      0x1CU},
    {("A661_EVENT_FLAG"),                                 0xB1D0U},
    {("A661_EVENT_NONE"),                                 0x00U},
    {("A661_EXCLUDED_CIRCLE"),                            0x03U},
    {("A661_EXCLUDED_RECTANGLE"),                         0x01U},
    {("A661_EXCLUDED_REGIONS_LIST"),                      0xB6C9U},
    {("A661_EXCLUDED_TRIANGLE"),                          0x02U},
    {("A661_FALSE"),                                      0x00U},
    {("A661_FILLED"),                                     0xB1E1U},
    {("A661_FILLED_POLY_START"),                          0x2BU},
    {("A661_FILLED_POLY_START_INTERACTIVE"),              0xABU},
    {("A661_FILL_INDEX"),                                 0xB1E0U},
    {("A661_FINE_PAGE_DELTA"),                            0xB6E2U},
    {("A661_FIRST_ACCESS_ENTRY"),                         0xB1F0U},
    {("A661_FIRST_ACCESS_UA_ENTRY"),                      0xB1F8U},
    {("A661_FIRST_VISIBLE_ENTRY"),                        0xB200U},
    {("A661_FONT"),                                       0xB590U},
    {("A661_FORMAT_STRING"),                              0xB550U},
    {("A661_FRAME_X"),                                    0xB210U},
    {("A661_FRAME_XY"),                                   0xB230U},
    {("A661_FRAME_Y"),                                    0xB220U},
    {("A661_FROM"),                                       0x05U},
    {("A661_FROM_ANGLE"),                                 0xB707U},
    {("A661_FROM_SCALE_X"),                               0xB708U},
    {("A661_FROM_SCALE_XY"),                              0xB715U},
    {("A661_FROM_SCALE_Y"),                               0xB709U},
    {("A661_FROM_VALUE"),                                 0xB70AU},
    {("A661_FROM_VALUE_X"),                               0xB70BU},
    {("A661_FROM_VALUE_XY"),                              0xB717U},
    {("A661_FROM_VALUE_Y"),                               0xB70CU},
    {("A661_GBK"),                                        0x03U},
    {("A661_GESTURE_ABORT"),                              0x04U},
    {("A661_GESTURE_CANDIDATE"),                          0x05U},
    {("A661_GESTURE_DOUBLE_TAP"),                         0x02U},
    {("A661_GESTURE_DRAG"),                               0x06U},
    {("A661_GESTURE_END"),                                0x03U},
    {("A661_GESTURE_FLICK"),                              0x07U},
    {("A661_GESTURE_FLICK_DOWN"),                         0x0BU},
    {("A661_GESTURE_FLICK_LEFT"),                         0x08U},
    {("A661_GESTURE_FLICK_RIGHT"),                        0x09U},
    {("A661_GESTURE_FLICK_UP"),                           0x0AU},
    {("A661_GESTURE_HOLD"),                               0x0CU},
    {("A661_GESTURE_PINCH"),                              0x04U},
    {("A661_GESTURE_PRESS_AND_HOLD"),                     0x03U},
    {("A661_GESTURE_ROTATE"),                             0x05U},
    {("A661_GESTURE_START"),                              0x01U},
    {("A661_GESTURE_TAP"),                                0x01U},
    {("A661_GESTURE_UPDATE"),                             0x02U},
    {("A661_GRID"),                                       0x03U},
    {("A661_HALO"),                                       0xB1E8U},
    {("A661_HIGHLIGHT_ONLY"),                             0x04U},
    {("A661_HOME_X"),                                     0xB219U},
    {("A661_HOME_Y"),                                     0xB21AU},
    {("A661_HORIZONTAL"),                                 0x01U},
    {("A661_HORIZONTAL_SCROLL"),                          0xB217U},
    {("A661_HORZ_LINE"),                                  0x02U},
    {("A661_HYPHENATE"),                                  0x02U},
    {("A661_INDEX_OF_FREQUENCY"),                         0xB091U},
    {("A661_INITIAL_FOCUS"),                              0xB6B0U},
    {("A661_INNER_RADIUS"),                               0xB240U},
    {("A661_INNER_STATE_CHECK"),                          0xB244U},
    {("A661_INNER_STATE_TOGGLE"),                         0xB258U},
    {("A661_INSET_SIZE"),                                 0xB051U},
    {("A661_IN_BOUNDS"),                                  0x01U},
    {("A661_ITEM_SPACING"),                               0xB5C1U},
    {("A661_ITEM_STYLE"),                                 0x20U},
    {("A661_ITEM_SYNCHRONIZATION"),                       0x51U},
    {("A661_KEY_ARRAY"),                                  0xB700U},
    {("A661_LAYOUT"),                                     0xB5F0U},
    {("A661_LEFT"),                                       0x13U},
    {("A661_LEFT_TO_RIGHT"),                              0x41U},
    {("A661_LEGEND"),                                     0x21U},
    {("A661_LEGEND_ALIGNMENT"),                           0xB49BU},
    {("A661_LEGEND_ANCHOR"),                              0x22U},
    {("A661_LEGEND_ANCHOR_ROTATED"),                      0x35U},
    {("A661_LEGEND_AND_HIGHLIGHT"),                       0x05U},
    {("A661_LEGEND_AND_POPUP"),                           0x03U},
    {("A661_LEGEND_AND_POPUP_AND_HIGHLIGHT"),             0x07U},
    {("A661_LEGEND_AREA_SIZE_X"),                         0xB451U},
    {("A661_LEGEND_COMBO"),                               0x38U},
    {("A661_LEGEND_HIGHLIGHT"),                           0x58U},
    {("A661_LEGEND_ONLY"),                                0x01U},
    {("A661_LEGEND_POP_UP"),                              0x23U},
    {("A661_LEGEND_POSITION"),                            0xB260U},
    {("A661_LEGEND_READOUT"),                             0x45U},
    {("A661_LEGEND_READOUT_COMBO"),                       0x48U},
    {("A661_LEGEND_READOUT_FORMAT_STRING"),               0x44U},
    {("A661_LEGEND_REMOVED"),                             0xB452U},
    {("A661_LINEAR"),                                     0x00U},
    {("A661_LINE_ARC"),                                   0x24U},
    {("A661_LINE_ARC_INTERACTIVE"),                       0xA4U},
    {("A661_LINE_DELTA_X"),                               0xB211U},
    {("A661_LINE_DELTA_Y"),                               0xB212U},
    {("A661_LINE_LENGTH"),                                0xB270U},
    {("A661_LINE_SEGMENT"),                               0x25U},
    {("A661_LINE_SEGMENT_INTERACTIVE"),                   0xA5U},
    {("A661_LINE_START"),                                 0x26U},
    {("A661_LINE_START_INTERACTIVE"),                     0xA6U},
    {("A661_LOOP_FORWARD"),                               0x00U},
    {("A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK"),        0x03U},
    {("A661_LOOP_FORWARD_AND_BLANK"),                     0x02U},
    {("A661_LOOP_FORWARD_AND_RESET"),                     0x01U},
    {("A661_LSB_MULTIPLE"),                               0xB631U},
    {("A661_MAJOR_TICK_INTERVAL"),                        0xB600U},
    {("A661_MAJOR_TICK_REFERENCE"),                       0xB608U},
    {("A661_MAPGRID_CELLSIZE"),                           0xB274U},
    {("A661_MAPGRID_OFFSET"),                             0xB278U},
    {("A661_MAP_SYNCHRONIZATION_NUMBER"),                 0xB277U},
    {("A661_MASK_ENABLED"),                               0xB290U},
    {("A661_MASK_REFERENCE"),                             0xB280U},
    {("A661_MAX_VALUE"),                                  0xB610U},
    {("A661_MDF_ABSOLUTE"),                               0x65U},
    {("A661_MDF_BRG_DIST_ACHDG"),                         0x60U},
    {("A661_MDF_DIST_DIST"),                              0x67U},
    {("A661_MDF_DIST_DIST_FIX"),                          0x6AU},
    {("A661_MDF_LAT_LONG"),                               0x61U},
    {("A661_MDF_LEGACY"),                                 0x62U},
    {("A661_MDF_RELATIVE"),                               0x66U},
    {("A661_MDF_X_DIST"),                                 0x68U},
    {("A661_MDF_Y_ALT"),                                  0x69U},
    {("A661_MENU_HORIZONTAL"),                            0xB640U},
    {("A661_MINIMUM_SIZE"),                               0x02U},
    {("A661_MINMAX_VALUES"),                              0xB580U},
    {("A661_MINOR_TICK_MULTIPLE"),                        0xB630U},
    {("A661_MIN_VALUE"),                                  0xB620U},
    {("A661_MULTI_STATE_VALUE"),                          0xB5A0U},
    {("A661_NEGATIVE_STRING"),                            0xB4D4U},
    {("A661_NEXT_FOCUSED_WIDGET"),                        0xB2B8U},
    {("A661_NEXT_FOCUS_IN_APP_ID"),                       0xB2BAU},
    {("A661_NEXT_FOCUS_IN_ID"),                           0xB2BCU},
    {("A661_NEXT_FOCUS_IN_LAYER_ID"),                     0xB2BBU},
    {("A661_NORMAL"),                                     0x1DU},
    {("A661_NOT_CLAMPED"),                                0x00U},
    {("A661_NOT_USED"),                                   0x00U},
    {("A661_NO_SHUFFLE"),                                 0x84U},
    {("A661_NO_SIZING"),                                  0x74U},
    {("A661_NUMBER_OF_ENTRIES"),                          0xB2A0U},
    {("A661_NUMBER_OF_KEYS"),                             0xB701U},
    {("A661_NUMBER_OF_PICTURES"),                         0xB2A2U},
    {("A661_NUMBER_OF_STATES"),                           0xB5A1U},
    {("A661_NUMBER_OF_SYMBOLS"),                          0xB2A4U},
    {("A661_NUMBER_OF_TICS_COARSE"),                      0xB0A2U},
    {("A661_NUMBER_OF_TICS_FINE"),                        0xB0A4U},
    {("A661_NUMBER_OF_TOUCH_POINTS"),                     0xB0A0U},
    {("A661_NUMBER_OF_TURNS"),                            0xB714U},
    {("A661_NUMBER_OF_UA_ENTRIES"),                       0xB2A8U},
    {("A661_NUMBER_OF_VISIBLE_CHILDREN"),                 0xB5D0U},
    {("A661_NUMERIC_MASK"),                               0xB2B0U},
    {("A661_OPENING_ENTRY"),                              0xB560U},
    {("A661_OPENING_MODE"),                               0xB561U},
    {("A661_OPEN_CENTERED"),                              0x17U},
    {("A661_OPEN_DOWN"),                                  0x18U},
    {("A661_OPEN_ENTRY_ARRAY"),                           0xB627U},
    {("A661_OPEN_UP"),                                    0x16U},
    {("A661_OPERATOR_ADD_ASSIGN_ARG"),                    0x0BU},
    {("A661_OPERATOR_ADD_ASSIGN_SRC"),                    0x08U},
    {("A661_OPERATOR_ASSIGNMENT"),                        0x00U},
    {("A661_OPERATOR_CONSTANT"),                          0x02U},
    {("A661_OPERATOR_INVERSE"),                           0x01U},
    {("A661_OPERATOR_LOGICAL_AND"),                       0x06U},
    {("A661_OPERATOR_LOGICAL_OR"),                        0x07U},
    {("A661_OPERATOR_PRODUCT"),                           0x05U},
    {("A661_OPERATOR_PROD_ASSIGN_ARG"),                   0x0DU},
    {("A661_OPERATOR_PROD_ASSIGN_SRC"),                   0x0AU},
    {("A661_OPERATOR_SUB_ASSIGN_ARG"),                    0x0CU},
    {("A661_OPERATOR_SUB_ASSIGN_SRC"),                    0x09U},
    {("A661_OPERATOR_SUM"),                               0x04U},
    {("A661_ORIENTATION"),                                0xB1D2U},
    {("A661_OUTER_RADIUS"),                               0xB2D0U},
    {("A661_OUT_OF_BOUNDS"),                              0x00U},
    {("A661_PAGE_CONTROL_POSITION"),                      0xB6E1U},
    {("A661_PAGE_DELTA_X"),                               0xB213U},
    {("A661_PAGE_DELTA_Y"),                               0xB214U},
    {("A661_PAGE_WRAP_TYPE"),                             0xB6E0U},
    {("A661_PARALLEL"),                                   0x00U},
    {("A661_PARAM_BUFFER_INDEX"),                         0xB111U},
    {("A661_PARENT_ENTRY_ARRAY"),                         0xB628U},
    {("A661_PARKING_LINE_END"),                           0xBBU},
    {("A661_PARKING_LINE_START"),                         0xBAU},
    {("A661_PICTURE_ARRAY"),                              0xB2E0U},
    {("A661_PICTURE_ENTRY_ARRAY"),                        0xB2E8U},
    {("A661_PICTURE_POSITION"),                           0xB2F1U},
    {("A661_PICTURE_REFERENCE"),                          0xB2F0U},
    {("A661_PIX_FMT_COLOR_INDEXED_8"),                    0xC2U},
    {("A661_PIX_FMT_LUMINANCE_ALPHA_8"),                  0xC1U},
    {("A661_PIX_FMT_RGBA_8"),                             0xC0U},
    {("A661_PLAY"),                                       0x01U},
    {("A661_POPUP_AND_HIGHLIGHT"),                        0x06U},
    {("A661_POPUP_ONLY"),                                 0x02U},
    {("A661_POPUP_POS_X"),                                0xB1C3U},
    {("A661_POPUP_POS_Y"),                                0xB1C4U},
    {("A661_POPUP_SIZE_X"),                               0xB1C5U},
    {("A661_POPUP_SIZE_Y"),                               0xB1C6U},
    {("A661_POSITIVE_STRING"),                            0xB4D3U},
    {("A661_POS_X"),                                      0xB300U},
    {("A661_POS_X2"),                                     0xB330U},
    {("A661_POS_X3"),                                     0xB360U},
    {("A661_POS_XY"),                                     0xB320U},
    {("A661_POS_XY2"),                                    0xB350U},
    {("A661_POS_XY3"),                                    0xB380U},
    {("A661_POS_Y"),                                      0xB310U},
    {("A661_POS_Y2"),                                     0xB340U},
    {("A661_POS_Y3"),                                     0xB370U},
    {("A661_PRESERVE_CHARACTER"),                         0x01U},
    {("A661_PRESERVE_WORD"),                              0x00U},
    {("A661_PROJECTION_REFERENCE"),                       0xB1D3U},
    {("A661_PRP_LAT"),                                    0xB390U},
    {("A661_PRP_LAT_LONG"),                               0xB3B0U},
    {("A661_PRP_LONG"),                                   0xB3A0U},
    {("A661_PRP_SCREEN_X"),                               0xB3C0U},
    {("A661_PRP_SCREEN_XY"),                              0xB3E0U},
    {("A661_PRP_SCREEN_Y"),                               0xB3D0U},
    {("A661_PRP_X"),                                      0xB3B2U},
    {("A661_PRP_XY"),                                     0xB3B4U},
    {("A661_PRP_Y"),                                      0xB3B3U},
    {("A661_RADIUS"),                                     0xB3F0U},
    {("A661_RANGE"),                                      0xB1D1U},
    {("A661_RANGE_X"),                                    0xB402U},
    {("A661_RANGE_XY"),                                   0xB404U},
    {("A661_RANGE_Y"),                                    0xB403U},
    {("A661_REFRESH"),                                    0xB670U},
    {("A661_REF_POS_X"),                                  0xB690U},
    {("A661_REF_POS_XY"),                                 0xB691U},
    {("A661_REF_POS_Y"),                                  0xB692U},
    {("A661_REORDER"),                                    0xB0ABU},
    {("A661_REPORT_ALL"),                                 0x61U},
    {("A661_REPORT_ALL_WITH_CANDIDATE"),                  0x64U},
    {("A661_REPORT_ON_CDS"),                              0x63U},
    {("A661_REPORT_ON_COMPLETION"),                       0x62U},
    {("A661_REPORT_ON_TRANSITION"),                       0x60U},
    {("A661_REPORT_ON_TRANSITION_WITH_CANDIDATE"),        0x65U},
    {("A661_REP_DEFAULT"),                                0x00U},
    {("A661_REP_FOCUS"),                                  0x02U},
    {("A661_REP_HIGHLIGHT"),                              0x03U},
    {("A661_REP_STANDARD"),                               0x01U},
    {("A661_RIGHT"),                                      0x14U},
    {("A661_RIGHT_TO_LEFT"),                              0x42U},
    {("A661_ROTATION_ANGLE"),                             0xB410U},
    {("A661_RUN"),                                        0x01U},
    {("A661_RUN_ONCE"),                                   0x02U},
    {("A661_SAME_LEVEL"),                                 0x02U},
    {("A661_SCALE_X"),                                    0xB0ACU},
    {("A661_SCALE_XY"),                                   0xB0AEU},
    {("A661_SCALE_Y"),                                    0xB0ADU},
    {("A661_SCREEN_RANGE"),                               0xB420U},
    {("A661_SELECTED"),                                   0x01U},
    {("A661_SELECTED_ENTRY"),                             0xB430U},
    {("A661_SELECTED_ENTRY_ARRAY"),                       0xB439U},
    {("A661_SELECTING_HEIGHT"),                           0xB431U},
    {("A661_SELECTING_WIDTH"),                            0xB432U},
    {("A661_SELECT_ALL"),                                 0xB438U},
    {("A661_SEQUENTIAL"),                                 0x01U},
    {("A661_SEQUENTIAL_FROM"),                            0x02U},
    {("A661_SHEET_SIZE_X"),                               0xB215U},
    {("A661_SHEET_SIZE_Y"),                               0xB216U},
    {("A661_SHIFT_FIRST_VISIBLE_ENTRY"),                  0xB440U},
    {("A661_SHOW_FAIL"),                                  0xB671U},
    {("A661_SHOW_MAJOR_TICKS"),                           0xB601U},
    {("A661_SHUFFLE_DOWN"),                               0x81U},
    {("A661_SHUFFLE_LEFT"),                               0x82U},
    {("A661_SHUFFLE_RIGHT"),                              0x83U},
    {("A661_SHUFFLE_TO_FIT_MODE"),                        0xB5E0U},
    {("A661_SHUFFLE_UP"),                                 0x80U},
    {("A661_SIZE_BOTTOM_UP"),                             0x71U},
    {("A661_SIZE_LEFT_TO_RIGHT"),                         0x72U},
    {("A661_SIZE_RIGHT_TO_LEFT"),                         0x73U},
    {("A661_SIZE_TOP_DOWN"),                              0x70U},
    {("A661_SIZE_TO_FIT_MODE"),                           0xB5C0U},
    {("A661_SIZE_X"),                                     0xB450U},
    {("A661_SIZE_XY"),                                    0xB470U},
    {("A661_SIZE_Y"),                                     0xB460U},
    {("A661_SOURCE_DX"),                                  0xB662U},
    {("A661_SOURCE_DXDY"),                                0xB665U},
    {("A661_SOURCE_DY"),                                  0xB663U},
    {("A661_SOURCE_REF"),                                 0xB666U},
    {("A661_SOURCE_WIDGET_PARAM"),                        0x00U},
    {("A661_SOURCE_X"),                                   0xB660U},
    {("A661_SOURCE_XY"),                                  0xB664U},
    {("A661_SOURCE_Y"),                                   0xB661U},
    {("A661_STAR"),                                       0x04U},
    {("A661_START_ANGLE"),                                0xB480U},
    {("A661_START_SCALED_VALUE"),                         0xB504U},
    {("A661_START_VALUE"),                                0xB502U},
    {("A661_STOP_BACKWARD"),                              0xB6B2U},
    {("A661_STOP_FORWARD"),                               0xB6B1U},
    {("A661_STRING"),                                     0xB490U},
    {("A661_STRING_ALTERNATE"),                           0xB498U},
    {("A661_STRING_ARRAY"),                               0xB4A0U},
    {("A661_STYLE_SET"),                                  0xB4B0U},
    {("A661_STYLE_SET_ARRAY"),                            0xB4B1U},
    {("A661_STYLE_SET_ARRAY_SIZE"),                       0xB4B2U},
    {("A661_STYLE_SET_DEFAULT"),                          0x00U},
    {("A661_SYMBOL_CIRCLE"),                              0x27U},
    {("A661_SYMBOL_CIRCLE_INTERACTIVE"),                  0xA7U},
    {("A661_SYMBOL_ENTRY_ARRAY"),                         0xB2E9U},
    {("A661_SYMBOL_GENERIC"),                             0x28U},
    {("A661_SYMBOL_GENERIC_INTERACTIVE"),                 0xA8U},
    {("A661_SYMBOL_OVAL"),                                0x2CU},
    {("A661_SYMBOL_OVAL_INTERACTIVE"),                    0xACU},
    {("A661_SYMBOL_PARKABLE"),                            0xB8U},
    {("A661_SYMBOL_PARKABLE_INTERACTIVE"),                0xB9U},
    {("A661_SYMBOL_POSITION"),                            0xB4C2U},
    {("A661_SYMBOL_RECTANGLE"),                           0x37U},
    {("A661_SYMBOL_RECTANGLE_INTERACTIVE"),               0xB7U},
    {("A661_SYMBOL_REFERENCE"),                           0xB4C0U},
    {("A661_SYMBOL_ROTATED"),                             0x29U},
    {("A661_SYMBOL_ROTATED_INTERACTIVE"),                 0xA9U},
    {("A661_SYMBOL_ROTATED_PARKABLE"),                    0xBEU},
    {("A661_SYMBOL_ROTATED_PARKABLE_INTERACTIVE"),        0xBFU},
    {("A661_SYMBOL_RUNWAY"),                              0x2AU},
    {("A661_SYMBOL_RUNWAY_INTERACTIVE"),                  0xAAU},
    {("A661_SYMBOL_TARGET"),                              0x2DU},
    {("A661_SYMBOL_TARGET_INTERACTIVE"),                  0xADU},
    {("A661_SYMBOL_TARGET_PARKABLE"),                     0x3DU},
    {("A661_SYMBOL_TARGET_PARKABLE_INTERACTIVE"),         0xBDU},
    {("A661_SYNC_FRAME"),                                 0xB4C7U},
    {("A661_TAB_DIRECTION_WIDGET_ID_ARRAY"),              0xB672U},
    {("A661_TAB_POSITION"),                               0xB052U},
    {("A661_TARGET_WIDGET_ID"),                           0xB650U},
    {("A661_TICS_COARSE"),                                0xB4D0U},
    {("A661_TICS_COARSE_8BYTE"),                          0xB4D2U},
    {("A661_TICS_COARSE_ARRAY"),                          0xB0A6U},
    {("A661_TICS_FINE"),                                  0xB4E0U},
    {("A661_TICS_FINE_8BYTE"),                            0xB4E2U},
    {("A661_TICS_FINE_ARRAY"),                            0xB0AAU},
    {("A661_TO"),                                         0x06U},
    {("A661_TOP"),                                        0x11U},
    {("A661_TOP_CENTER"),                                 0x33U},
    {("A661_TOP_LEFT"),                                   0x34U},
    {("A661_TOP_RIGHT"),                                  0x35U},
    {("A661_TOP_TO_BOTTOM"),                              0x43U},
    {("A661_TOUCH_ABORT"),                                0x04U},
    {("A661_TOUCH_DOWN"),                                 0x01U},
    {("A661_TOUCH_MOVE"),                                 0x02U},
    {("A661_TOUCH_MOVE_OUTSIDE"),                         0x05U},
    {("A661_TOUCH_UP"),                                   0x03U},
    {("A661_TO_ANGLE"),                                   0xB70DU},
    {("A661_TO_BACK"),                                    0x01U},
    {("A661_TO_FRONT"),                                   0x02U},
    {("A661_TO_SCALE_X"),                                 0xB70EU},
    {("A661_TO_SCALE_XY"),                                0xB716U},
    {("A661_TO_SCALE_Y"),                                 0xB70FU},
    {("A661_TO_VALUE"),                                   0xB710U},
    {("A661_TO_VALUE_X"),                                 0xB711U},
    {("A661_TO_VALUE_XY"),                                0xB718U},
    {("A661_TO_VALUE_Y"),                                 0xB712U},
    {("A661_TRANSLATION_X"),                              0xB4F0U},
    {("A661_TRANSLATION_XY"),                             0xB510U},
    {("A661_TRANSLATION_Y"),                              0xB500U},
    {("A661_TREE_NODE_STYLE_ARRAY"),                      0xB629U},
    {("A661_TRIANGLE_END"),                               0x32U},
    {("A661_TRIANGLE_END_DOUBLE"),                        0x33U},
    {("A661_TRIANGLE_END_DOUBLE_INTERACTIVE"),            0xB3U},
    {("A661_TRIANGLE_END_INTERACTIVE"),                   0xB2U},
    {("A661_TRIANGLE_FAN_START"),                         0x34U},
    {("A661_TRIANGLE_FAN_START_INTERACTIVE"),             0xB4U},
    {("A661_TRIANGLE_SEGMENT"),                           0x2FU},
    {("A661_TRIANGLE_SEGMENT_DOUBLE"),                    0x31U},
    {("A661_TRIANGLE_SEGMENT_DOUBLE_INTERACTIVE"),        0xB1U},
    {("A661_TRIANGLE_SEGMENT_INTERACTIVE"),               0xAFU},
    {("A661_TRIANGLE_STRIP_START"),                       0x2EU},
    {("A661_TRIANGLE_STRIP_START_INTERACTIVE"),           0xAEU},
    {("A661_TRUE"),                                       0x01U},
    {("A661_TRUE_WITH_VALIDATION"),                       0x02U},
    {("A661_UA_POSITION_FLAG"),                           0xB1C1U},
    {("A661_UNSELECTED"),                                 0x00U},
    {("A661_UNSELECT_ALL"),                               0xB43AU},
    {("A661_UP"),                                         0x19U},
    {("A661_UTF8"),                                       0x02U},
    {("A661_VALIDATION"),                                 0x01U},
    {("A661_VALIDATION_AND_WHEEL"),                       0x02U},
    {("A661_VALUE"),                                      0xB520U},
    {("A661_VALUE_8BYTE"),                                0xB522U},
    {("A661_VERTEX_RENDER_END"),                          0x3BU},
    {("A661_VERTEX_RENDER_LINES"),                        0x01U},
    {("A661_VERTEX_RENDER_LINE_LOOP"),                    0x02U},
    {("A661_VERTEX_RENDER_POLYLINE"),                     0x03U},
    {("A661_VERTEX_RENDER_SEGMENT"),                      0x3AU},
    {("A661_VERTEX_RENDER_START"),                        0x39U},
    {("A661_VERTEX_RENDER_TRIANGLE"),                     0x04U},
    {("A661_VERTEX_RENDER_TRIANGLE_FAN"),                 0x05U},
    {("A661_VERTEX_RENDER_TRIANGLE_STRIP"),               0x06U},
    {("A661_VERTICAL"),                                   0x00U},
    {("A661_VERTICAL_SCROLL"),                            0xB218U},
    {("A661_VERTICES_ARRAY"),                             0xB528U},
    {("A661_VERT_LINE"),                                  0x03U},
    {("A661_VISIBLE"),                                    0xB530U},
    {("A661_VISIBLE_CHILD"),                              0xB540U},
    {("A661_VISIBLE_CHILD_INDEX"),                        0xB680U},
    {("A661_WHEEL"),                                      0x03U},
    {("A661_WORD_WRAP"),                                  0xB4A8U},
    {("A661_WRAP_BOTH"),                                  0x01U},
    {("A661_WRAP_FIRST_TO_LAST"),                         0x02U},
    {("A661_WRAP_LAST_TO_FIRST"),                         0x03U},
    {("A661_WRAP_NONE"),                                  0x00U},
    {("A661_WRAP_STYLE"),                                 0xB4A9U}
};

static const char_t* const c_boundaryCheckTargets[] = {"A661_MAPHORZ_ITEMLIST"};
static const char_t* const c_childIndexTargets[] = {"A661_MUTUALLY_EXCLUSIVE_CONTAINER", "A661_PAGING_CONTAINER", "A661_TABBED_PANEL_GROUP"};
static const char_t* const c_cursorEntryEventsTargets[] = {"A661_COMBO_BOX", "A661_SCROLL_LIST", "A661_SELECTOR", "A661_TREE"};
static const char_t* const c_cursorEventsTargets[] = {"A661_ACTIVE_AREA", "A661_PICTURE_PUSH_BUTTON", "A661_PUSH_BUTTON", "A661_SYMBOL_PUSH_BUTTON"};
static const char_t* const c_cursorShapeTargets[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_CURSOR_OVER", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_INK_AREA", "A661_KEYBOARD_AREA", "A661_MAPHORZ_ITEMLIST", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SCROLL_WHEEL_AREA", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_directionalTabbingTargets[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_excludedRegionsTargets[] = {"A661_MAP_BOUNDARY"};
static const char_t* const c_focusStopTargets[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_INK_AREA", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_initialFocusTargets[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_INK_AREA", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_legendAlignTargets[] = {"A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_NUMERIC_READOUT"};
static const char_t* const c_maphorzCoordSystemEventTargets[] = {"A661_MAPHORZ", "A661_MAPHORZ_SOURCE", "A661_MAP_EXTERNAL_SOURCE"};
static const char_t* const c_mapvertCoordSystemEventTargets[] = {"A661_MAPVERT", "A661_MAPVERT_SOURCE", "A661_MAP_EXTERNAL_SOURCE"};
static const char_t* const c_multiSelectionTargets[] = {"A661_SCROLL_LIST", "A661_TREE"};
static const char_t* const c_pictureTargets[] = {"A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_MULTI_STATE_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_reorderTargets[] = {"A661_ACTIVE_AREA", "A661_BASIC_CONTAINER", "A661_BLINKING_CONTAINER", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_CONNECTOR", "A661_CURSOR_OVER", "A661_CURSOR_POS_OVERLAY", "A661_CURSOR_REF", "A661_DATA_CONNECTOR", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_EXTERNAL_SOURCE", "A661_GESTURE_AREA", "A661_GP_ARC_CIRCLE", "A661_GP_ARC_ELLIPSE", "A661_GP_CROWN", "A661_GP_LINE", "A661_GP_LINE_POLAR", "A661_GP_POLYLINE", "A661_GP_RECTANGLE", "A661_GP_TRIANGLE", "A661_INK_AREA", "A661_KEYBOARD_AREA", "A661_LABEL", "A661_LABEL_COMPLEX", "A661_MAPHORZ", "A661_MAPHORZ_CONTAINER", "A661_MAPHORZ_ITEMLIST", "A661_MAPHORZ_PANEL", "A661_MAPHORZ_SOURCE", "A661_MAPVERT", "A661_MAPVERT_CONTAINER", "A661_MAPVERT_ITEMLIST", "A661_MAPVERT_PANEL", "A661_MAPVERT_SOURCE", "A661_MAP_GRID", "A661_MASK_CONTAINER", "A661_MENU_BAR", "A661_MULTI_STATE_BUTTON", "A661_MUTUALLY_EXCLUSIVE_CONTAINER", "A661_NUMERIC_READOUT", "A661_PAGING_CONTAINER", "A661_PANEL", "A661_PICTURE", "A661_PICTURE_ANIMATED", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_RADIO_BOX", "A661_ROTATION_CONTAINER", "A661_SCALE_CONTAINER", "A661_SCROLL_LIST", "A661_SCROLL_PANEL", "A661_SCROLL_WHEEL_AREA", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SHUFFLE_TO_FIT_CONTAINER", "A661_SIZE_TO_FIT_CONTAINER", "A661_SLIDER", "A661_SYMBOL", "A661_SYMBOL_ANIMATED", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TABBED_PANEL_GROUP", "A661_TOGGLE_BUTTON", "A661_TOUCH_AREA", "A661_TRANSLATION_CONTAINER", "A661_TREE"};
static const char_t* const c_staticParamBufferTargets[] = {"A661_BUFFER_FORMAT"};
static const char_t* const c_styleSetTargets[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_CURSOR_OVER", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_EXTERNAL_SOURCE", "A661_GESTURE_AREA", "A661_GP_ARC_CIRCLE", "A661_GP_ARC_ELLIPSE", "A661_GP_CROWN", "A661_GP_LINE", "A661_GP_LINE_POLAR", "A661_GP_POLYLINE", "A661_GP_RECTANGLE", "A661_GP_TRIANGLE", "A661_INK_AREA", "A661_LABEL", "A661_LABEL_COMPLEX", "A661_MAPHORZ_PANEL", "A661_MULTI_STATE_BUTTON", "A661_NUMERIC_READOUT", "A661_PAGING_CONTAINER", "A661_PANEL", "A661_PICTURE", "A661_PICTURE_ANIMATED", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SCROLL_PANEL", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SHUFFLE_TO_FIT_CONTAINER", "A661_SIZE_TO_FIT_CONTAINER", "A661_SLIDER", "A661_SYMBOL", "A661_SYMBOL_ANIMATED", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TABBED_PANEL_GROUP", "A661_TOGGLE_BUTTON", "A661_TOUCH_AREA", "A661_TREE"};
static const char_t* const c_symbolTargets[] = {"A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_MULTI_STATE_BUTTON", "A661_POP_UP_MENU", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_ticsArrayTargets[] = {"A661_EDIT_BOX_NUMERIC"};
static const char_t* const c_wordWrapTargets[] = {"A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_LABEL", "A661_LABEL_COMPLEX", "A661_NUMERIC_READOUT", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};

static const ExtensionTargets_t c_extensionTargets[] =
{
    {"A661_BOUNDARY_CHECK_EXTENSION", ARRAY_AND_COUNT(c_boundaryCheckTargets)},
    {"A661_CHILD_INDEX_EXTENSION", ARRAY_AND_COUNT(c_childIndexTargets)},
    {"A661_CURSOR_ENTRY_EVENTS_EXTENSION", ARRAY_AND_COUNT(c_cursorEntryEventsTargets)},
    {"A661_CURSOR_EVENTS_EXTENSION", ARRAY_AND_COUNT(c_cursorEventsTargets)},
    {"A661_CURSOR_SHAPE_EXTENSION", ARRAY_AND_COUNT(c_cursorShapeTargets)},
    {"A661_DIRECTIONAL_TABBING_EXTENSION", ARRAY_AND_COUNT(c_directionalTabbingTargets)},
    {"A661_EXCLUDED_REGIONS_EXTENSION", ARRAY_AND_COUNT(c_excludedRegionsTargets)},
    {"A661_FOCUS_STOP_EXTENSION", ARRAY_AND_COUNT(c_focusStopTargets)},
    {"A661_INITIAL_FOCUS_EXTENSION", ARRAY_AND_COUNT(c_initialFocusTargets)},
    {"A661_LEGEND_ALIGN_EXTENSION", ARRAY_AND_COUNT(c_legendAlignTargets)},
    {"A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION", ARRAY_AND_COUNT(c_maphorzCoordSystemEventTargets)},
    {"A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION", ARRAY_AND_COUNT(c_mapvertCoordSystemEventTargets)},
    {"A661_MULTI_SELECTION_EXTENSION", ARRAY_AND_COUNT(c_multiSelectionTargets)},
    {"A661_PICTURE_EXTENSION", ARRAY_AND_COUNT(c_pictureTargets)},
    {"A661_REORDER_EXTENSION", ARRAY_AND_COUNT(c_reorderTargets)},
    {"A661_STATIC_PARAM_BUFFER_EXTENSION", ARRAY_AND_COUNT(c_staticParamBufferTargets)},
    {"A661_STYLE_SET_EXTENSION", ARRAY_AND_COUNT(c_styleSetTargets)},
    {"A661_SYMBOL_EXTENSION", ARRAY_AND_COUNT(c_symbolTargets)},
    {"A661_TICS_ARRAY_EXTENSION", ARRAY_AND_COUNT(c_ticsArrayTargets)},
    {"A661_WORD_WRAP_EXTENSION", ARRAY_AND_COUNT(c_wordWrapTargets)},
};

#define EXTENSION_TARGET_COUNT ((uint32_t)COUNT_OF(c_extensionTargets))

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Clears the published counts, block bytes, and charset of a Definition File.
 *
 * \param[in,out] definition   Definition File whose arrays and capacities are retained
 */
static void ResetDefinition(Arinc661_Definition_t* definition);

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

/*************************************************************************************************/
/**
 * \brief Stores an unsigned value in big-endian order when it fits in the buffer.
 *
 * \param[out] buffer      Output bytes
 * \param[in]  capacity    Output buffer capacity
 * \param[in]  offset      Byte offset of the value
 * \param[in]  value       Value to store
 * \param[in]  byteCount   Number of bytes to store, at most four
 *
 * \retval None
 */
static void PutUnsigned(byte_t* buffer, uint64_t capacity, uint64_t offset, uint32_t value, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Loads a big-endian unsigned value.
 *
 * \param[in] buffer      Input bytes
 * \param[in] offset      Byte offset of the value
 * \param[in] byteCount   Number of bytes to load, at most four
 *
 * \retval    uint32_t    Loaded value.
 */
static uint32_t GetUnsigned(const byte_t* buffer, uint64_t offset, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Converts a 32-bit two's complement pattern to its signed value.
 *
 * \param[in] value   Bit pattern
 *
 * \retval    int64_t   Signed value in the int32_t range.
 */
static int64_t ToSigned32(uint32_t value);

/*************************************************************************************************/
/**
 * \brief Stores zero bytes up to the next 32-bit boundary.
 *
 * \param[out]    buffer     Output bytes
 * \param[in]     capacity   Output buffer capacity
 * \param[in,out] offset     Output byte offset, advanced to the boundary
 *
 * \retval None
 */
static void PutAlignment(byte_t* buffer, uint64_t capacity, uint64_t* offset);

/*************************************************************************************************/
/**
 * \brief Stores bytes given as hexadecimal digits.
 *
 * \param[in]  text        Zero-terminated digits, two per byte
 * \param[in]  byteCount   Number of bytes the text must hold
 * \param[out] buffer      Output bytes
 * \param[in]  capacity    Output buffer capacity
 * \param[in]  offset      Byte offset of the first byte
 *
 * \retval     TRUE        if the text holds exactly byteCount bytes of valid digits.
 * \retval     FALSE       otherwise.
 */
static bool_t PutHexBytes(const char_t* text, uint64_t byteCount, byte_t* buffer, uint64_t capacity, uint64_t offset);

/*************************************************************************************************/
/**
 * \brief Finds a widget or extension type by name.
 *
 * \param[in] name          Zero-terminated type name
 * \param[in] isExtension   TRUE to search extension types, FALSE for widget types
 *
 * \retval    const BinaryWidget_t*   Type, or NULL when it is not supported.
 */
static const BinaryWidget_t* FindTypeByName(const char_t* name, bool_t isExtension);

/*************************************************************************************************/
/**
 * \brief Finds a widget or extension type by its code.
 *
 * \param[in] code          WidgetType or ExtensionType value
 * \param[in] isExtension   TRUE to search extension types, FALSE for widget types
 *
 * \retval    const BinaryWidget_t*   Type, or NULL when it is not supported.
 */
static const BinaryWidget_t* FindTypeByCode(uint32_t code, bool_t isExtension);

/*************************************************************************************************/
/**
 * \brief Finds a property of one model by name and position among properties of that name.
 *
 * \param[in] definition   Definition File values
 * \param[in] owner        Owning model kind
 * \param[in] ownerIndex   Layer or widget index, or zero for the Definition File
 * \param[in] name         Zero-terminated property name
 * \param[in] occurrence   Zero for the first property of this name, one for the second, and so on
 *
 * \retval    const Arinc661_Property_t*   Property, or NULL when the model has too few with this name.
 */
static const Arinc661_Property_t* FindProperty(const Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, uint32_t occurrence);

/*************************************************************************************************/
/**
 * \brief Converts an optional property to an integer within a range.
 *
 * \param[in]  property   Property to convert, or NULL for zero
 * \param[in]  minimum    Smallest accepted value
 * \param[in]  maximum    Largest accepted value
 * \param[out] value      Converted value
 *
 * \retval     TRUE       if the property is absent, or a number or constant name within the range.
 * \retval     FALSE      if the value is not numeric, not a known constant, or out of range.
 */
static bool_t ScanInteger(const Arinc661_Property_t* property, int64_t minimum, int64_t maximum, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Returns the byte count of an excluded region structure from its type byte.
 *
 * \param[in] regionType   ExcludedRegionType value
 *
 * \retval    uint32_t     Structure bytes, or zero for an unknown type.
 */
static uint32_t RegionByteCount(uint32_t regionType);

/*************************************************************************************************/
/**
 * \brief Appends one Creation Structure parameter to the output.
 *
 * \param[in]     field      Parameter encoding
 * \param[in]     property   Property holding the value, or NULL for zero or an empty string
 * \param[out]    buffer     Output bytes
 * \param[in]     capacity   Output buffer capacity
 * \param[in,out] offset     Output byte offset, advanced past the parameter
 *
 * \retval        TRUE       if the value was converted.
 * \retval        FALSE      if the value does not fit the parameter type.
 */
static bool_t WriteField(const BinaryField_t* field, const Arinc661_Property_t* property, byte_t* buffer, uint64_t capacity, uint64_t* offset);

/*************************************************************************************************/
/**
 * \brief Appends the A661_CMD_CREATE or A661_CMD_EXTEND command of one widget array entry.
 *
 * \param[in]     definition        Definition File values
 * \param[in]     widgetIndex       Widget array index
 * \param[in]     lastWidgetIndex   Index of the most recent non-extension widget in this layer block, or UINT32_MAX
 * \param[out]    buffer            Output bytes
 * \param[in]     capacity          Output buffer capacity
 * \param[in,out] offset            Output byte offset, advanced past the command
 *
 * \retval        TRUE              if the command was built.
 * \retval        FALSE             if the type, parent, or a property is not supported.
 */
static bool_t WriteCommand(const Arinc661_Definition_t* definition, uint32_t widgetIndex, uint32_t lastWidgetIndex, byte_t* buffer, uint64_t capacity, uint64_t* offset);

/*************************************************************************************************/
/**
 * \brief Formats a number without an exponent or unused fractional zeros.
 *
 * \param[out] text        Zero-terminated result
 * \param[in]  capacity    Result capacity in bytes
 * \param[in]  value       Finite value to format
 * \param[in]  precision   Fractional digits before zeros are removed
 *
 * \retval     uint32_t    Number of text bytes, excluding the zero terminator.
 */
static uint32_t FormatReal(char_t* text, uint32_t capacity, float64_t value, int precision);

/*************************************************************************************************/
/**
 * \brief Converts a value within a fr() range to its signed 32-bit LSB count.
 *
 * \param[in] value   Value between -range and range
 * \param[in] range   Half the full fr() range, 180 or 32768
 *
 * \retval    int64_t   LSB count, rounded half away from zero and clamped to int32_t.
 */
static int64_t EncodeFraction(float64_t value, float64_t range);

/*************************************************************************************************/
/**
 * \brief Formats an IEEE 754 single with the fewest fractional digits that read back as the same value.
 *
 * \param[out] text       Zero-terminated result
 * \param[in]  capacity   Result capacity in bytes
 * \param[in]  value      Finite value to format
 *
 * \retval     uint32_t   Number of text bytes, excluding the zero terminator.
 */
static uint32_t FormatFloat32(char_t* text, uint32_t capacity, float32_t value);

/*************************************************************************************************/
/**
 * \brief Formats a fr() LSB count with the fewest fractional digits that encode back to the same count.
 *
 * \param[out] text       Zero-terminated result
 * \param[in]  capacity   Result capacity in bytes
 * \param[in]  lsbCount   Signed 32-bit LSB count
 * \param[in]  range      Half the full fr() range, 180 or 32768
 *
 * \retval     uint32_t   Number of text bytes, excluding the zero terminator.
 */
static uint32_t FormatFraction(char_t* text, uint32_t capacity, int64_t lsbCount, float64_t range);

/*************************************************************************************************/
/**
 * \brief Appends a property with a counted value to the result.
 *
 * \param[in,out] definition       Caller-owned Definition File data
 * \param[in]     owner            Owning model kind
 * \param[in]     ownerIndex       Layer or widget index, or zero for the Definition File
 * \param[in]     name             Zero-terminated property name
 * \param[in]     value            Value bytes
 * \param[in]     valueByteCount   Number of value bytes
 *
 * \retval        TRUE             if the property was appended.
 * \retval        FALSE            if the property array is full or a string does not fit.
 */
static bool_t AddProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value, uint64_t valueByteCount);

/*************************************************************************************************/
/**
 * \brief Appends a property with a decimal integer value to the result.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     owner        Owning model kind
 * \param[in]     ownerIndex   Layer or widget index, or zero for the Definition File
 * \param[in]     name         Zero-terminated property name
 * \param[in]     value        Property value
 *
 * \retval        TRUE         if the property was appended.
 * \retval        FALSE        if the property array is full.
 */
static bool_t AddIntegerProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, int64_t value);

/*************************************************************************************************/
/**
 * \brief Appends a property holding bytes as uppercase hexadecimal digits to the newest widget.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     name         Zero-terminated property name
 * \param[in]     bytes        Bytes to encode
 * \param[in]     byteCount    Number of bytes
 *
 * \retval        TRUE         if the property was appended.
 * \retval        FALSE        if the property array is full or the digits do not fit a value.
 */
static bool_t AddHexProperty(Arinc661_Definition_t* definition, const char_t* name, const byte_t* bytes, uint64_t byteCount);

/*************************************************************************************************/
/**
 * \brief Reads one Creation Structure parameter into a property of the newest widget.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     field        Parameter encoding
 * \param[in]     buffer       Input bytes
 * \param[in]     endOffset    Byte offset following the command
 * \param[in,out] offset       Input byte offset, advanced past the parameter
 *
 * \retval        TRUE         if the parameter was read.
 * \retval        FALSE        if the parameter exceeds the command or result storage.
 */
static bool_t ReadField(Arinc661_Definition_t* definition, const BinaryField_t* field, const byte_t* buffer, uint64_t endOffset, uint64_t* offset);

/*************************************************************************************************/
/**
 * \brief Reads one A661_CMD_CREATE or A661_CMD_EXTEND command into the widget and property arrays.
 *
 * \param[in,out] definition           Caller-owned Definition File data
 * \param[in]     buffer               Input bytes
 * \param[in]     firstCommandOffset   Byte offset of the first command in this layer block
 * \param[in]     firstWidgetIndex     Widget array index of that first command
 * \param[in,out] lastWidgetIndex      Index of the most recent non-extension widget in this layer block, or UINT32_MAX
 * \param[in]     commandOffset        Byte offset of the command to read
 * \param[in]     commandSize          CommandSize of the command to read
 *
 * \retval        TRUE                 if the widget or extension was read.
 * \retval        FALSE                if the command, type, or parent is unknown, or storage is insufficient.
 */
static bool_t ReadCommand(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t firstCommandOffset, uint32_t firstWidgetIndex, uint32_t* lastWidgetIndex, uint64_t commandOffset, uint32_t commandSize);

/*************************************************************************************************/
/**
 * \brief Reads one layer block into the layer, widget, and property arrays.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     buffer       Input bytes
 * \param[in]     byteCount    Number of input bytes
 * \param[in,out] offset       Byte offset of the block, advanced past it
 *
 * \retval        TRUE         if the layer and its widgets were read.
 * \retval        FALSE        if the block is malformed or storage is insufficient.
 */
static bool_t ReadLayer(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount, uint64_t* offset);

/*************************************************************************************************/
/**
 * \brief Measures a picture block (Tables 7-1 and 7-2).
 *
 * \param[in]  buffer      Input bytes
 * \param[in]  byteCount   Number of input bytes
 * \param[in]  offset      Byte offset of the block
 * \param[out] blockSize   Number of bytes in the block
 *
 * \retval     TRUE        if the block is well formed and complete.
 * \retval     FALSE       if it is truncated or uses an unknown pixel format.
 */
static bool_t MeasurePictureBlock(const byte_t* buffer, uint64_t byteCount, uint64_t offset, uint64_t* blockSize);

/*************************************************************************************************/
/**
 * \brief Copies a symbol, picture, or animation law block into the caller's block storage.
 *
 * \param[in,out] definition   Caller-owned Definition File data
 * \param[in]     buffer       Input bytes
 * \param[in]     byteCount    Number of input bytes
 * \param[in,out] offset       Byte offset of the block, advanced past it
 *
 * \retval        TRUE         if the block was measured and copied.
 * \retval        FALSE        if the block is malformed or block storage is insufficient.
 */
static bool_t ReadOpaqueBlock(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount, uint64_t* offset);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Array entries and scalar fields are left as they are; with zero counts they are not published.
 */
static void ResetDefinition(Arinc661_Definition_t* definition)
{
    definition->layerCount      = 0;
    definition->widgetCount     = 0;
    definition->propertyCount   = 0;
    definition->blockByteCount  = 0;
    definition->charsetEncoding = 0;
}

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

/*************************************************************************************************/
/**
 * \par Details:
 *  Stores the most significant byte first, as required by section 4.5.1. A value
 *  that would extend past capacity is not stored; callers keep advancing their
 *  offset and compare it with capacity once the complete file has been sized.
 */
static void PutUnsigned(byte_t* buffer, uint64_t capacity, uint64_t offset, uint32_t value, uint32_t byteCount)
{
    uint32_t byte_index;

    LOGASSERT(BINARY_WORD_BYTES >= byteCount);
    if ((offset <= capacity) && (byteCount <= (capacity - offset)))
    {
        for (byte_index = 0; byte_index < byteCount; ++byte_index)
        {
            buffer[offset + byte_index] = (byte_t)(value >> (CHAR_BIT * (byteCount - 1U - byte_index)));
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Loads the most significant byte first. The caller has already checked that
 *  the bytes lie inside the input.
 */
static uint32_t GetUnsigned(const byte_t* buffer, uint64_t offset, uint32_t byteCount)
{
    uint32_t value;
    uint32_t byte_index;

    LOGASSERT(BINARY_WORD_BYTES >= byteCount);
    value = 0;
    for (byte_index = 0; byte_index < byteCount; ++byte_index)
    {
        value = (value << CHAR_BIT) | buffer[offset + byte_index];
    }

    return value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Avoids the implementation-defined conversion of an out-of-range unsigned
 *  value to int32_t: 0xFFFFFFFF becomes -1 and 0x80000000 becomes INT32_MIN.
 */
static int64_t ToSigned32(uint32_t value)
{
    int64_t result;

    result = (int64_t)value;
    if ((uint32_t)INT32_MAX < value)
    {
        result -= ((int64_t)UINT32_MAX + 1);
    }

    return result;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offsets of blocks and commands are multiples of four, so aligning the output
 *  offset aligns the position inside the command.
 */
static void PutAlignment(byte_t* buffer, uint64_t capacity, uint64_t* offset)
{
    uint32_t byte_index;

    for (byte_index = 0; byte_index < BINARY_WORD_BYTES; ++byte_index)
    {
        if (0 == ((*offset) % BINARY_WORD_BYTES))
        {
            break;
        }
        PutUnsigned(buffer, capacity, (*offset), 0, BINARY_UCHAR_BYTES);
        (*offset) += BINARY_UCHAR_BYTES;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts uppercase digits only, exactly two per byte and nothing else, so
 *  "1B4200" stores three bytes. The length check keeps the terminator, which
 *  strchr would also find, out of reach.
 */
static bool_t PutHexBytes(const char_t* text, uint64_t byteCount, byte_t* buffer, uint64_t capacity, uint64_t offset)
{
    const char_t* high_digit;
    const char_t* low_digit;
    uint64_t      byte_index;
    bool_t        is_valid;

    is_valid = (strlen(text) == (byteCount * HEX_DIGITS_PER_BYTE));
    for (byte_index = 0; is_valid && (byte_index < byteCount); ++byte_index)
    {
        high_digit = strchr(c_binaryHexDigits, text[byte_index * HEX_DIGITS_PER_BYTE]);
        low_digit  = strchr(c_binaryHexDigits, text[(byte_index * HEX_DIGITS_PER_BYTE) + 1U]);
        is_valid   = (NULL != high_digit) && (NULL != low_digit);
        if (is_valid)
        {
            PutUnsigned(buffer, capacity, offset + byte_index, (uint32_t)(((high_digit - c_binaryHexDigits) << HEX_DIGIT_BITS) | (low_digit - c_binaryHexDigits)), BINARY_UCHAR_BYTES);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Matching is case-sensitive, for example "A661_PANEL" among widget types or
 *  "A661_LEGEND_ALIGN_EXTENSION" among extension types.
 */
static const BinaryWidget_t* FindTypeByName(const char_t* name, bool_t isExtension)
{
    const BinaryWidget_t* table;
    const BinaryWidget_t* type;
    uint32_t              type_count;
    uint32_t              type_index;

    table      = c_binaryWidgets;
    type_count = (uint32_t)COUNT_OF(c_binaryWidgets);
    if (isExtension)
    {
        table      = c_binaryExtensions;
        type_count = (uint32_t)COUNT_OF(c_binaryExtensions);
    }
    type = NULL;
    for (type_index = 0; type_index < type_count; ++type_index)
    {
        if (0 == strcmp(name, table[type_index].name))
        {
            type = &table[type_index];
            break;
        }
    }

    return type;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, 0xA1F0 selects A661_PANEL and 0x8004 selects A661_LEGEND_ALIGN_EXTENSION.
 */
static const BinaryWidget_t* FindTypeByCode(uint32_t code, bool_t isExtension)
{
    const BinaryWidget_t* table;
    const BinaryWidget_t* type;
    uint32_t              type_count;
    uint32_t              type_index;

    table      = c_binaryWidgets;
    type_count = (uint32_t)COUNT_OF(c_binaryWidgets);
    if (isExtension)
    {
        table      = c_binaryExtensions;
        type_count = (uint32_t)COUNT_OF(c_binaryExtensions);
    }
    type = NULL;
    for (type_index = 0; type_index < type_count; ++type_index)
    {
        if (code == table[type_index].code)
        {
            type = &table[type_index];
            break;
        }
    }

    return type;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Scans the flat property array for entries whose owner kind, owner index, and
 *  name all match, and returns the one at the requested position among them.
 *  Array elements are properties sharing the array's name, in element order,
 *  so occurrence is the element index. Names are compared case-sensitively.
 */
static const Arinc661_Property_t* FindProperty(const Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, uint32_t occurrence)
{
    const Arinc661_Property_t* property;
    const Arinc661_Property_t* candidate;
    uint32_t                   property_index;
    uint32_t                   match_count;

    // ponytail: accept a linear scan per lookup; index properties by owner if large files become slow
    property    = NULL;
    match_count = 0;
    for (property_index = 0; property_index < definition->propertyCount; ++property_index)
    {
        candidate = &definition->properties[property_index];
        if ((owner == candidate->owner) && (ownerIndex == candidate->ownerIndex) && (0 == strcmp(name, candidate->name)))
        {
            if (occurrence == match_count)
            {
                property = candidate;
                break;
            }
            match_count += 1;
        }
    }

    return property;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An absent property yields zero. A present value is either a complete decimal
 *  integer such as "10000" or a constant name such as "A661_TRUE" or
 *  "A661_CENTER". Other text, for example a color name, returns FALSE because the
 *  binary form has no place for it.
 */
static bool_t ScanInteger(const Arinc661_Property_t* property, int64_t minimum, int64_t maximum, int64_t* value)
{
    int64_t  parsed_value;
    int32_t  consumed;
    uint32_t constant_index;
    bool_t   is_valid;

    parsed_value = 0;
    is_valid     = TRUE;
    if (NULL != property)
    {
        consumed = FACEOE_Utl_Scan_Int64(property->value, &parsed_value);
        // Require a nonempty scan that consumes the complete value
        is_valid = (0 < consumed) && ('\0' == property->value[consumed]);
        if (FALSE == is_valid)
        {
            for (constant_index = 0; constant_index < COUNT_OF(c_binaryConstants); ++constant_index)
            {
                if (0 == strcmp(property->value, c_binaryConstants[constant_index].name))
                {
                    parsed_value = c_binaryConstants[constant_index].value;
                    is_valid     = TRUE;
                    break;
                }
            }
        }
    }
    if (is_valid)
    {
        is_valid = (minimum <= parsed_value) && (maximum >= parsed_value);
    }
    if (is_valid)
    {
        (*value) = parsed_value;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Table 8.6.7-3: A661_NOT_USED (0) is 4 bytes, A661_EXCLUDED_RECTANGLE (1) is 20,
 *  A661_EXCLUDED_TRIANGLE (2) is 28, and A661_EXCLUDED_CIRCLE (3) is 24.
 */
static uint32_t RegionByteCount(uint32_t regionType)
{
    uint32_t byte_count;

    byte_count = 0;
    if (BINARY_REGION_NOT_USED == regionType)
    {
        byte_count = BINARY_REGION_NOT_USED_BYTES;
    }
    else if (BINARY_REGION_RECTANGLE == regionType)
    {
        byte_count = BINARY_REGION_RECTANGLE_BYTES;
    }
    else if (BINARY_REGION_TRIANGLE == regionType)
    {
        byte_count = BINARY_REGION_TRIANGLE_BYTES;
    }
    else if (BINARY_REGION_CIRCLE == regionType)
    {
        byte_count = BINARY_REGION_CIRCLE_BYTES;
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Converts the property text according to the parameter type:
 *
 * \verbatim
 * uchar, ushort, ulong, long   decimal integer or constant name, range-checked
 * float                        decimal number stored as IEEE 754 single precision
 * fr(180), fr(32768)           decimal number scaled by 2^31 / range and rounded
 * string                       value bytes followed by a zero terminator
 * bytes                        two hexadecimal digits per byte, such as "1B4200"
 * sized bytes                  hexadecimal digits; written as byte count, pad, bytes, and alignment
 * region                       hexadecimal digits of a complete excluded region structure
 * layer request                hexadecimal digits of a complete LayerReqResult entry
 * UnusedPad                    zero bytes
 * alignment                    zero bytes up to the next 32-bit boundary
 * \endverbatim
 *
 *  An absent property is written as zero or as an empty string. The fr() full
 *  positive range is one LSB short of representable and is clamped to it.
 */
static bool_t WriteField(const BinaryField_t* field, const Arinc661_Property_t* property, byte_t* buffer, uint64_t capacity, uint64_t* offset)
{
    const char_t* text;
    const char_t* type_digit;
    float64_t     real_value;
    float64_t     range;
    float32_t     float_value;
    int64_t       integer_value;
    int64_t       minimum;
    int64_t       maximum;
    uint64_t      byte_count;
    uint64_t      byte_index;
    uint32_t      encoded_value;
    uint32_t      request_size;
    int32_t       consumed;
    bool_t        is_valid;

    is_valid      = TRUE;
    encoded_value = 0;
    text          = "";
    if (NULL != property)
    {
        text = property->value;
    }
    if (BINARY_FIELD_ALIGN == field->type)
    {
        PutAlignment(buffer, capacity, offset);
    }
    else if (BINARY_FIELD_BYTES == field->type)
    {
        if (NULL != property)
        {
            is_valid = PutHexBytes(text, field->byteCount, buffer, capacity, (*offset));
        }
        else
        {
            for (byte_index = 0; byte_index < field->byteCount; ++byte_index)
            {
                PutUnsigned(buffer, capacity, (*offset) + byte_index, 0, BINARY_UCHAR_BYTES);
            }
        }
        (*offset) += field->byteCount;
    }
    else if (BINARY_FIELD_SIZED_BYTES == field->type)
    {
        // Write ParamBufferSize, the unused pad, the bytes, then 32-bit alignment (Table 8.6.5-3)
        byte_count = strlen(text) / HEX_DIGITS_PER_BYTE;
        is_valid   = (UINT16_MAX >= byte_count);
        if (is_valid)
        {
            PutUnsigned(buffer, capacity, (*offset), (uint32_t)byte_count, BINARY_USHORT_BYTES);
            PutUnsigned(buffer, capacity, (*offset) + BINARY_SIZED_PAD_OFFSET, 0, BINARY_USHORT_BYTES);
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset) + BINARY_WORD_BYTES);
        }
        (*offset) += (BINARY_WORD_BYTES + byte_count);
        PutAlignment(buffer, capacity, offset);
    }
    else if (BINARY_FIELD_REGION == field->type)
    {
        // Read the structure size from its second byte, requiring both digits of that byte to be valid
        byte_count = 0;
        is_valid   = ((BINARY_REGION_HEADER_BYTES * HEX_DIGITS_PER_BYTE) <= strlen(text));
        if (is_valid)
        {
            type_digit = strchr(c_binaryHexDigits, text[(BINARY_REGION_TYPE_OFFSET * HEX_DIGITS_PER_BYTE) + 1U]);
            is_valid   = ('0' == text[BINARY_REGION_TYPE_OFFSET * HEX_DIGITS_PER_BYTE]) && (NULL != type_digit);
        }
        if (is_valid)
        {
            byte_count = RegionByteCount((uint32_t)(type_digit - c_binaryHexDigits));
            is_valid   = (0 != byte_count);
        }
        if (is_valid)
        {
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset));
        }
        (*offset) += byte_count;
    }
    else if (BINARY_FIELD_LAYER_REQUEST == field->type)
    {
        // Require the CommandSize in the third and fourth bytes to match the request that follows (Table 3.9.17-6)
        byte_count = strlen(text) / HEX_DIGITS_PER_BYTE;
        is_valid   = (BINARY_REQUEST_HEADER_BYTES <= byte_count);
        if (is_valid)
        {
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset));
        }
        if (is_valid)
        {
            request_size = 0;
            for (byte_index = BINARY_REQUEST_SIZE_OFFSET * HEX_DIGITS_PER_BYTE; byte_index < ((BINARY_REQUEST_SIZE_OFFSET + BINARY_USHORT_BYTES) * HEX_DIGITS_PER_BYTE); ++byte_index)
            {
                request_size = (request_size << HEX_DIGIT_BITS) | (uint32_t)(strchr(c_binaryHexDigits, text[byte_index]) - c_binaryHexDigits);
            }
            is_valid = (byte_count == (BINARY_REQUEST_HEADER_BYTES + request_size)) && (0 == (request_size % BINARY_WORD_BYTES));
        }
        (*offset) += byte_count;
    }
    else if (BINARY_FIELD_STRING == field->type)
    {
        byte_count = strlen(text);
        for (byte_index = 0; byte_index < byte_count; ++byte_index)
        {
            PutUnsigned(buffer, capacity, (*offset) + byte_index, (byte_t)text[byte_index], BINARY_UCHAR_BYTES);
        }
        // Keep each string's terminator; pad the command after its last parameter
        PutUnsigned(buffer, capacity, (*offset) + byte_count, 0, BINARY_UCHAR_BYTES);
        (*offset) += (byte_count + 1);
    }
    else
    {
        if ((BINARY_FIELD_FLOAT == field->type) || (BINARY_FIELD_FR180 == field->type) || (BINARY_FIELD_FR32768 == field->type))
        {
            real_value = 0.0;
            if (NULL != property)
            {
                consumed = FACEOE_Utl_Scan_Float(text, &real_value);
                is_valid = (0 < consumed) && ('\0' == text[consumed]);
            }
            if (is_valid && (BINARY_FIELD_FLOAT == field->type))
            {
                is_valid = (real_value <= FLT_MAX) && (real_value >= -FLT_MAX);
                if (is_valid)
                {
                    float_value = (float32_t)real_value;
                    (void)memcpy(&encoded_value, &float_value, sizeof(encoded_value));
                }
            }
            else if (is_valid)
            {
                range = BINARY_FR32768_RANGE;
                if (BINARY_FIELD_FR180 == field->type)
                {
                    range = BINARY_FR180_RANGE;
                }
                is_valid = (real_value <= range) && (real_value >= -range);
                if (is_valid)
                {
                    encoded_value = (uint32_t)EncodeFraction(real_value, range);
                }
            }
        }
        else if (BINARY_FIELD_PAD != field->type)
        {
            minimum = 0;
            maximum = UINT32_MAX;
            if (BINARY_FIELD_UCHAR == field->type)
            {
                maximum = UINT8_MAX;
            }
            else if (BINARY_FIELD_USHORT == field->type)
            {
                maximum = UINT16_MAX;
            }
            else if (BINARY_FIELD_LONG == field->type)
            {
                minimum = INT32_MIN;
                maximum = INT32_MAX;
            }
            is_valid = ScanInteger(property, minimum, maximum, &integer_value);
            if (is_valid)
            {
                // Keep the 32-bit pattern of a negative long through two's complement truncation
                encoded_value = (uint32_t)integer_value;
            }
        }
        PutUnsigned(buffer, capacity, (*offset), encoded_value, field->byteCount);
        (*offset) += field->byteCount;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds one command as in Table 4.5.3.3.1, for example a panel under its layer:
 *
 * \verbatim
 * CA01      A661_CMD_CREATE
 * 0020      CommandSize (32 bytes, including this header)
 * A1F0      WidgetType (A661_PANEL)
 * 1221      WidgetIdent
 * 0000      ParentIdent (zero: the layer is the parent)
 * 01 01     Enable, Visible ...
 * \endverbatim
 *
 *  or an extension of the most recent widget as in Table 4.5.3.3.2:
 *
 * \verbatim
 * CA05      A661_CMD_EXTEND
 * 0008      CommandSize
 * 8004      ExtensionType (A661_LEGEND_ALIGN_EXTENSION)
 * 15 00     LegendAlignment, UnusedPad
 * \endverbatim
 *
 *  WidgetIdent comes from the widget's property of that name. ParentIdent is the
 *  WidgetIdent of the parent widget, which must precede this widget in the same
 *  layer and must not be an extension. An extension entry must name the most
 *  recently written widget as its parent, since section 8.1 binds an extension
 *  block to the widget instance it follows. Remaining parameters follow the
 *  Creation Structure of the type; a property that names none of them returns
 *  FALSE, so that no value is silently dropped. The command is zero-padded to
 *  32 bits.
 *
 *  An array takes its elements from the properties sharing its name, in order.
 *  Their number must equal the count parameter named by the table, for example
 *  NumberOfEntries, times the elements per entry:
 *
 * \verbatim
 * NumberOfEntries = 2
 * EntryList = "ON"         first entry
 * EntryList = "OFF"        second entry
 * \endverbatim
 */
static bool_t WriteCommand(const Arinc661_Definition_t* definition, uint32_t widgetIndex, uint32_t lastWidgetIndex, byte_t* buffer, uint64_t capacity, uint64_t* offset)
{
    const Arinc661_Widget_t*   widget;
    const Arinc661_Property_t* property;
    const BinaryWidget_t*      type;
    const BinaryField_t*       field;
    int64_t                    widget_ident;
    int64_t                    parent_ident;
    int64_t                    field_value;
    uint64_t                   cursor;
    uint32_t                   values[BINARY_MAX_FIELDS];
    uint32_t                   header_bytes;
    uint32_t                   command;
    uint32_t                   field_index;
    uint32_t                   element_index;
    uint32_t                   element_count;
    uint32_t                   property_index;
    uint32_t                   matched_count;
    uint32_t                   owned_count;
    bool_t                     is_valid;

    widget        = &definition->widgets[widgetIndex];
    type          = FindTypeByName(widget->type, widget->isExtension);
    header_bytes  = BINARY_COMMAND_BYTES;
    command       = BINARY_CMD_CREATE;
    if (widget->isExtension)
    {
        header_bytes = BINARY_EXTEND_BYTES;
        command      = BINARY_CMD_EXTEND;
    }
    cursor        = (*offset) + header_bytes;
    widget_ident  = 0;
    parent_ident  = 0;
    matched_count = 0;
    (void)memset(values, 0, sizeof(values));
    is_valid = (NULL != type);
    if (is_valid)
    {
        LOGASSERT(BINARY_MAX_FIELDS >= type->fieldCount);
    }
    if (is_valid && widget->isExtension)
    {
        // Require the extension to follow the widget instance it extends (section 8.1)
        is_valid = (lastWidgetIndex == widget->parentWidgetIndex);
    }
    if (is_valid && (FALSE == widget->isExtension))
    {
        property = FindProperty(definition, ARINC661_PROPERTY_WIDGET, widgetIndex, BINARY_PROPERTY_WIDGET_IDENT, 0);
        if (NULL != property)
        {
            matched_count += 1;
        }
        is_valid = ScanInteger(property, 0, UINT16_MAX, &widget_ident);
        if (is_valid && (UINT32_MAX != widget->parentWidgetIndex))
        {
            // Require the nested parent to precede the current widget in the same layer and to be a widget itself
            is_valid = (widget->parentWidgetIndex < widgetIndex);
            if (is_valid)
            {
                is_valid = (widget->layerIndex == definition->widgets[widget->parentWidgetIndex].layerIndex) && (FALSE == definition->widgets[widget->parentWidgetIndex].isExtension);
            }
            if (is_valid)
            {
                property = FindProperty(definition, ARINC661_PROPERTY_WIDGET, widget->parentWidgetIndex, BINARY_PROPERTY_WIDGET_IDENT, 0);
                is_valid = ScanInteger(property, 0, UINT16_MAX, &parent_ident);
            }
        }
    }
    if (is_valid)
    {
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            field = &type->fields[field_index];
            if (0 == field->repeat)
            {
                property = NULL;
                if (NULL != field->name)
                {
                    property = FindProperty(definition, ARINC661_PROPERTY_WIDGET, widgetIndex, field->name, 0);
                }
                if (NULL != property)
                {
                    matched_count += 1;
                }
                is_valid = WriteField(field, property, buffer, capacity, &cursor);
                if (is_valid && ((BINARY_FIELD_UCHAR == field->type) || (BINARY_FIELD_USHORT == field->type)))
                {
                    // Keep small integers so later arrays can find their entry counts
                    (void)ScanInteger(property, 0, UINT16_MAX, &field_value);
                    values[field_index] = (uint32_t)field_value;
                }
            }
            else
            {
                // Require a property for every counted element; let surplus ones fail the ownership check below
                element_count = field->repeat * values[field->countField];
                for (element_index = 0; element_index < element_count; ++element_index)
                {
                    property = FindProperty(definition, ARINC661_PROPERTY_WIDGET, widgetIndex, field->name, element_index);
                    is_valid = (NULL != property);
                    if (is_valid)
                    {
                        matched_count += 1;
                        is_valid       = WriteField(field, property, buffer, capacity, &cursor);
                    }
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                }
            }
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }
    if (is_valid)
    {
        // Require every property of this entry to have been placed in the command
        owned_count = 0;
        for (property_index = 0; property_index < definition->propertyCount; ++property_index)
        {
            if ((ARINC661_PROPERTY_WIDGET == definition->properties[property_index].owner) && (widgetIndex == definition->properties[property_index].ownerIndex))
            {
                owned_count += 1;
            }
        }
        is_valid = (owned_count == matched_count);
    }
    if (is_valid)
    {
        PutAlignment(buffer, capacity, &cursor);
        is_valid = ((cursor - (*offset)) <= UINT16_MAX);
    }
    if (is_valid)
    {
        PutUnsigned(buffer, capacity, (*offset), command, BINARY_USHORT_BYTES);
        PutUnsigned(buffer, capacity, (*offset) + BINARY_COMMAND_SIZE_OFFSET, (uint32_t)(cursor - (*offset)), BINARY_USHORT_BYTES);
        PutUnsigned(buffer, capacity, (*offset) + BINARY_COMMAND_TYPE_OFFSET, type->code, BINARY_USHORT_BYTES);
        if (FALSE == widget->isExtension)
        {
            PutUnsigned(buffer, capacity, (*offset) + BINARY_WIDGET_IDENT_OFFSET, (uint32_t)widget_ident, BINARY_USHORT_BYTES);
            PutUnsigned(buffer, capacity, (*offset) + BINARY_PARENT_IDENT_OFFSET, (uint32_t)parent_ident, BINARY_USHORT_BYTES);
        }
        (*offset) = cursor;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Formats in fixed notation, then removes trailing fractional zeros and an
 *  unused decimal point, so 90.0000000000 becomes "90" and 1.2500 becomes "1.25".
 */
static uint32_t FormatReal(char_t* text, uint32_t capacity, float64_t value, int precision)
{
    int      byte_count;
    uint32_t byte_index;

    byte_count = snprintf(text, capacity, "%.*f", precision, value);
    LOGASSERT((0 < byte_count) && ((uint32_t)byte_count < capacity));
    if (NULL != strchr(text, '.'))
    {
        // Strip fractional zeros only; an integer such as 100 keeps its zeros
        for (byte_index = 0; byte_index < capacity; ++byte_index)
        {
            if ('0' != text[byte_count - 1])
            {
                break;
            }
            byte_count -= 1;
        }
        if ('.' == text[byte_count - 1])
        {
            byte_count -= 1;
        }
    }
    text[byte_count] = '\0';

    return (uint32_t)byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Scales to LSBs, rounds half away from zero, and clamps so that the full positive range,
 *  which is one LSB past representable, becomes INT32_MAX.
 */
static int64_t EncodeFraction(float64_t value, float64_t range)
{
    float64_t scaled;

    scaled = (value * BINARY_FR_FULL_SCALE) / range;
    scaled = round(scaled);
    if (scaled > (float64_t)INT32_MAX)
    {
        scaled = (float64_t)INT32_MAX;
    }
    if (scaled < (float64_t)INT32_MIN)
    {
        scaled = (float64_t)INT32_MIN;
    }

    return (int64_t)scaled;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Tries each fractional digit count from zero up and keeps the first text that scans back
 *  to the identical single, so 0.1f becomes "0.1" rather than its 54-digit expansion while
 *  every bit of the value survives the round trip. The scanner reads xs:decimal, so no
 *  exponent form is produced.
 */
static uint32_t FormatFloat32(char_t* text, uint32_t capacity, float32_t value)
{
    float64_t parsed_value;
    uint32_t  byte_count;
    int32_t   consumed;
    int       precision;

    byte_count = 0;
    for (precision = 0; precision <= BINARY_FLOAT_PRECISION; ++precision)
    {
        byte_count = FormatReal(text, capacity, value, precision);
        consumed   = FACEOE_Utl_Scan_Float(text, &parsed_value);
        if ((0 < consumed) && ((float32_t)parsed_value == value))
        {
            break;
        }
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The exact value is lsbCount times range over 2^31; the text keeps only the digits
 *  needed for EncodeFraction to return lsbCount again, at most BINARY_FR_PRECISION.
 */
static uint32_t FormatFraction(char_t* text, uint32_t capacity, int64_t lsbCount, float64_t range)
{
    float64_t parsed_value;
    uint32_t  byte_count;
    int32_t   consumed;
    int       precision;

    byte_count = 0;
    for (precision = 0; precision <= BINARY_FR_PRECISION; ++precision)
    {
        byte_count = FormatReal(text, capacity, ((float64_t)lsbCount * range) / BINARY_FR_FULL_SCALE, precision);
        consumed   = FACEOE_Utl_Scan_Float(text, &parsed_value);
        if ((0 < consumed) && (EncodeFraction(parsed_value, range) == lsbCount))
        {
            break;
        }
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reserves the next property slot and copies both strings into it. The count is
 *  published only after the name and value fit with their zero terminators.
 */
static bool_t AddProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value, uint64_t valueByteCount)
{
    Arinc661_Property_t* property;
    uint64_t             name_byte_count;
    bool_t               is_valid;

    name_byte_count = strlen(name);
    is_valid        = (definition->propertyCount < definition->propertyCapacity);
    if (is_valid)
    {
        is_valid = (name_byte_count < sizeof(property->name)) && (valueByteCount < sizeof(property->value));
    }
    if (is_valid)
    {
        property = &definition->properties[definition->propertyCount];
        (void)memset(property, 0, sizeof((*property)));
        property->owner      = owner;
        property->ownerIndex = ownerIndex;
        (void)memcpy(property->name, name, name_byte_count);
        (void)memcpy(property->value, value, valueByteCount);
        definition->propertyCount += 1;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Formats the value as a decimal integer, for example -42 as "-42".
 */
static bool_t AddIntegerProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, int64_t value)
{
    char_t text[BINARY_INTEGER_TEXT_BYTES];
    int    byte_count;

    byte_count = snprintf(text, sizeof(text), "%" PRId64, value);
    LOGASSERT((0 < byte_count) && ((uint32_t)byte_count < sizeof(text)));

    return AddProperty(definition, owner, ownerIndex, name, text, (uint64_t)byte_count);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Encodes each byte as two uppercase digits, so 0x1B 0x42 0x00 becomes "1B4200".
 *  The widget being read has not been counted yet, so its index is the current count.
 */
static bool_t AddHexProperty(Arinc661_Definition_t* definition, const char_t* name, const byte_t* bytes, uint64_t byteCount)
{
    char_t   text[ARINC661_MAX_PROPERTY_BYTES];
    uint64_t byte_index;
    bool_t   is_valid;

    is_valid = ((byteCount * HEX_DIGITS_PER_BYTE) < sizeof(text));
    if (is_valid)
    {
        for (byte_index = 0; byte_index < byteCount; ++byte_index)
        {
            text[byte_index * HEX_DIGITS_PER_BYTE]        = c_binaryHexDigits[bytes[byte_index] >> HEX_DIGIT_BITS];
            text[(byte_index * HEX_DIGITS_PER_BYTE) + 1U] = c_binaryHexDigits[bytes[byte_index] & HEX_DIGIT_MASK];
        }
        is_valid = AddProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, name, text, byteCount * HEX_DIGITS_PER_BYTE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Converts one parameter to property text, the inverse of WriteField. Integers
 *  become decimal numbers; constant names such as A661_TRUE are not restored
 *  because the same number has different names for different parameters.
 *  Floats and fr() angles use the fewest decimal digits that reproduce the
 *  stored bits exactly. A
 *  string extends to its zero terminator, which must lie inside the command.
 *  Bytes without a numeric type, sized buffers, and excluded regions become
 *  uppercase hexadecimal digits. Unused pads and alignment bytes are skipped
 *  without checking their content.
 */
static bool_t ReadField(Arinc661_Definition_t* definition, const BinaryField_t* field, const byte_t* buffer, uint64_t endOffset, uint64_t* offset)
{
    char_t    text[ARINC661_MAX_PROPERTY_BYTES];
    float64_t range;
    float32_t float_value;
    uint64_t  byte_count;
    uint64_t  remaining;
    uint32_t  encoded_value;
    uint32_t  widget_index;
    bool_t    is_valid;

    // Use the current count as the index of the widget being read, which is not counted yet
    widget_index = definition->widgetCount;
    LOGASSERT((*offset) <= endOffset);
    remaining = endOffset - (*offset);
    if (BINARY_FIELD_ALIGN == field->type)
    {
        (*offset) = BINARY_ALIGN((*offset));
        is_valid  = ((*offset) <= endOffset);
    }
    else if (BINARY_FIELD_BYTES == field->type)
    {
        is_valid = (field->byteCount <= remaining);
        if (is_valid)
        {
            is_valid = AddHexProperty(definition, field->name, &buffer[(*offset)], field->byteCount);
            (*offset) += field->byteCount;
        }
    }
    else if (BINARY_FIELD_SIZED_BYTES == field->type)
    {
        is_valid = (BINARY_WORD_BYTES <= remaining);
        if (is_valid)
        {
            byte_count = GetUnsigned(buffer, (*offset), BINARY_USHORT_BYTES);
            is_valid   = (byte_count <= (remaining - BINARY_WORD_BYTES));
        }
        if (is_valid)
        {
            is_valid = AddHexProperty(definition, field->name, &buffer[(*offset) + BINARY_WORD_BYTES], byte_count);
            (*offset) = ((*offset) + BINARY_WORD_BYTES + byte_count + (BINARY_WORD_BYTES - 1U)) & ~(uint64_t)(BINARY_WORD_BYTES - 1U);
        }
        if (is_valid)
        {
            is_valid = ((*offset) <= endOffset);
        }
    }
    else if (BINARY_FIELD_REGION == field->type)
    {
        is_valid = (BINARY_REGION_HEADER_BYTES <= remaining);
        if (is_valid)
        {
            byte_count = RegionByteCount(buffer[(*offset) + BINARY_REGION_TYPE_OFFSET]);
            is_valid   = (0 != byte_count) && (byte_count <= remaining);
        }
        if (is_valid)
        {
            is_valid = AddHexProperty(definition, field->name, &buffer[(*offset)], byte_count);
            (*offset) += byte_count;
        }
    }
    else if (BINARY_FIELD_LAYER_REQUEST == field->type)
    {
        is_valid = (BINARY_REQUEST_HEADER_BYTES <= remaining);
        if (is_valid)
        {
            byte_count = BINARY_REQUEST_HEADER_BYTES + GetUnsigned(buffer, (*offset) + BINARY_REQUEST_SIZE_OFFSET, BINARY_USHORT_BYTES);
            is_valid   = (byte_count <= remaining) && (0 == (byte_count % BINARY_WORD_BYTES));
        }
        if (is_valid)
        {
            is_valid = AddHexProperty(definition, field->name, &buffer[(*offset)], byte_count);
            (*offset) += byte_count;
        }
    }
    else if (BINARY_FIELD_STRING == field->type)
    {
        // Locate the terminator without leaving the command
        for (byte_count = 0; byte_count < remaining; ++byte_count)
        {
            if (0 == buffer[(*offset) + byte_count])
            {
                break;
            }
        }
        is_valid = (byte_count < remaining);
        if (is_valid)
        {
            is_valid = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, (const char_t*)&buffer[(*offset)], byte_count);
            (*offset) += (byte_count + 1);
        }
    }
    else
    {
        is_valid = (field->byteCount <= remaining);
        if (is_valid && (BINARY_FIELD_PAD != field->type))
        {
            encoded_value = GetUnsigned(buffer, (*offset), field->byteCount);
            if (BINARY_FIELD_FLOAT == field->type)
            {
                (void)memcpy(&float_value, &encoded_value, sizeof(float_value));
                // Reject infinities and NaN, which have no decimal property text
                is_valid = (float_value <= FLT_MAX) && (float_value >= -FLT_MAX);
                if (is_valid)
                {
                    byte_count = FormatFloat32(text, sizeof(text), float_value);
                    is_valid   = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, text, byte_count);
                }
            }
            else if ((BINARY_FIELD_FR180 == field->type) || (BINARY_FIELD_FR32768 == field->type))
            {
                range = BINARY_FR32768_RANGE;
                if (BINARY_FIELD_FR180 == field->type)
                {
                    range = BINARY_FR180_RANGE;
                }
                byte_count = FormatFraction(text, sizeof(text), ToSigned32(encoded_value), range);
                is_valid   = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, text, byte_count);
            }
            else if (BINARY_FIELD_LONG == field->type)
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, ToSigned32(encoded_value));
            }
            else
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, encoded_value);
            }
        }
        if (is_valid)
        {
            (*offset) += field->byteCount;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds one widget array entry and its properties in Creation Structure order.
 *  A create command adds a WidgetIdent property first; an extend command adds an
 *  entry flagged isExtension whose parent is the most recent widget of the layer
 *  block, as section 8.1 requires. Names stay empty because the binary form does
 *  not carry names.
 *
 *  A zero ParentIdent places a widget directly under its layer. Any other value
 *  must equal the WidgetIdent of an earlier create command in the same layer
 *  block (section 4.5.3.3.3); earlier commands are walked by their CommandSize
 *  to map that identifier to a widget array index.
 *
 *  Each array element becomes its own property, all sharing the array's name,
 *  in element order. The element count comes from the parameter the table names
 *  as the count, read earlier in the same command.
 *
 *  The parameters must fill the command up to its 32-bit padding. Another
 *  length means the command follows a different supplement's structure, and
 *  returns FALSE.
 */
static bool_t ReadCommand(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t firstCommandOffset, uint32_t firstWidgetIndex, uint32_t* lastWidgetIndex, uint64_t commandOffset, uint32_t commandSize)
{
    Arinc661_Widget_t*    widget;
    const BinaryWidget_t* type;
    const BinaryField_t*  field;
    uint64_t              cursor;
    uint64_t              end_offset;
    uint32_t              values[BINARY_MAX_FIELDS];
    uint32_t              command;
    uint32_t              parent_ident;
    uint32_t              widget_index;
    uint32_t              field_index;
    uint32_t              element_index;
    uint32_t              element_count;
    bool_t                is_extension;
    bool_t                is_valid;

    command      = GetUnsigned(buffer, commandOffset, BINARY_USHORT_BYTES);
    is_extension = (BINARY_CMD_EXTEND == command);
    end_offset   = commandOffset + commandSize;
    cursor       = end_offset;
    type         = NULL;
    (void)memset(values, 0, sizeof(values));
    is_valid = is_extension || (BINARY_CMD_CREATE == command);
    if (is_valid)
    {
        type     = FindTypeByCode(GetUnsigned(buffer, commandOffset + BINARY_COMMAND_TYPE_OFFSET, BINARY_USHORT_BYTES), is_extension);
        is_valid = (NULL != type);
    }
    if (is_valid)
    {
        LOGASSERT(BINARY_MAX_FIELDS >= type->fieldCount);
        is_valid = (definition->widgetCount < definition->widgetCapacity);
    }
    if (is_valid)
    {
        widget = &definition->widgets[definition->widgetCount];
        (void)memset(widget, 0, sizeof((*widget)));
        (void)strcpy(widget->type, type->name);
        widget->layerIndex        = definition->layerCount - 1;
        widget->parentWidgetIndex = UINT32_MAX;
        widget->isExtension       = is_extension;
        cursor                    = commandOffset + BINARY_EXTEND_BYTES;
        if (is_extension)
        {
            // Require a widget to extend in this layer block
            widget->parentWidgetIndex = (*lastWidgetIndex);
            is_valid                  = (UINT32_MAX != (*lastWidgetIndex));
        }
        else
        {
            cursor       = commandOffset + BINARY_COMMAND_BYTES;
            parent_ident = GetUnsigned(buffer, commandOffset + BINARY_PARENT_IDENT_OFFSET, BINARY_USHORT_BYTES);
            if (0 != parent_ident)
            {
                cursor = firstCommandOffset;
                for (widget_index = firstWidgetIndex; widget_index < definition->widgetCount; ++widget_index)
                {
                    if ((BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, BINARY_USHORT_BYTES)) && (parent_ident == GetUnsigned(buffer, cursor + BINARY_WIDGET_IDENT_OFFSET, BINARY_USHORT_BYTES)))
                    {
                        widget->parentWidgetIndex = widget_index;
                        break;
                    }
                    cursor += GetUnsigned(buffer, cursor + BINARY_COMMAND_SIZE_OFFSET, BINARY_USHORT_BYTES);
                }
                cursor   = commandOffset + BINARY_COMMAND_BYTES;
                is_valid = (UINT32_MAX != widget->parentWidgetIndex);
            }
            if (is_valid)
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, BINARY_PROPERTY_WIDGET_IDENT, GetUnsigned(buffer, commandOffset + BINARY_WIDGET_IDENT_OFFSET, BINARY_USHORT_BYTES));
            }
        }
    }
    if (is_valid)
    {
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            field         = &type->fields[field_index];
            element_count = 1;
            if (0 != field->repeat)
            {
                element_count = field->repeat * values[field->countField];
            }
            else if (((BINARY_FIELD_UCHAR == field->type) || (BINARY_FIELD_USHORT == field->type)) && (field->byteCount <= (end_offset - cursor)))
            {
                // Keep small integers so later arrays can find their entry counts
                values[field_index] = GetUnsigned(buffer, cursor, field->byteCount);
            }
            // Let a count larger than the command holds fail in ReadField at the command end
            for (element_index = 0; element_index < element_count; ++element_index)
            {
                is_valid = ReadField(definition, field, buffer, end_offset, &cursor);
                if (FALSE == is_valid)
                {
                    break;
                }
            }
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }
    if (is_valid)
    {
        // Allow only alignment padding after the last parameter
        is_valid = ((end_offset - cursor) < BINARY_WORD_BYTES);
    }
    if (is_valid)
    {
        if (FALSE == is_extension)
        {
            (*lastWidgetIndex) = definition->widgetCount;
        }
        definition->widgetCount += 1;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads one block laid out as in Table 4.5.3.2-4:
 *
 * \verbatim
 * A0        A661_BEGIN_LAYER_BLOCK
 * 01        LayerIdent          -> property LayerId
 * 0000      ContextNumber       -> property ContextNumber
 * 0000007C  Block Size, including this header and the end code
 * ...       A661_CMD_CREATE and A661_CMD_EXTEND commands
 * C0        A661_END_LAYER_BLOCK
 * 000000    UnusedPad
 * \endverbatim
 *
 *  The layer name stays empty. Block Size and every CommandSize must be a
 *  multiple of four and stay inside the enclosing bytes.
 */
static bool_t ReadLayer(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount, uint64_t* offset)
{
    Arinc661_Layer_t* layer;
    uint64_t          cursor;
    uint64_t          first_command_offset;
    uint64_t          end_offset;
    uint64_t          command_index;
    uint32_t          block_size;
    uint32_t          command_size;
    uint32_t          first_widget_index;
    uint32_t          last_widget_index;
    uint32_t          layer_index;
    bool_t            is_valid;

    layer_index        = definition->layerCount;
    first_widget_index = definition->widgetCount;
    last_widget_index  = UINT32_MAX;
    end_offset         = 0;
    is_valid           = ((BINARY_LAYER_HEADER_BYTES + BINARY_BLOCK_END_BYTES) <= (byteCount - (*offset)));
    if (is_valid)
    {
        block_size = GetUnsigned(buffer, (*offset) + BINARY_BLOCK_SIZE_OFFSET, BINARY_ULONG_BYTES);
        is_valid   = ((BINARY_LAYER_HEADER_BYTES + BINARY_BLOCK_END_BYTES) <= block_size) && (0 == (block_size % BINARY_WORD_BYTES)) && (block_size <= (byteCount - (*offset)));
    }
    if (is_valid)
    {
        end_offset = (*offset) + block_size - BINARY_BLOCK_END_BYTES;
        is_valid   = (BINARY_END_LAYER_BLOCK == buffer[end_offset]);
    }
    if (is_valid)
    {
        is_valid = (definition->layerCount < definition->layerCapacity);
    }
    if (is_valid)
    {
        layer = &definition->layers[layer_index];
        (void)memset(layer, 0, sizeof((*layer)));
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_LAYER_ID, buffer[(*offset) + BINARY_LAYER_IDENT_OFFSET]);
    }
    if (is_valid)
    {
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_CONTEXT_NUMBER, GetUnsigned(buffer, (*offset) + BINARY_CONTEXT_NUMBER_OFFSET, BINARY_USHORT_BYTES));
    }
    if (is_valid)
    {
        definition->layerCount += 1;
        first_command_offset = (*offset) + BINARY_LAYER_HEADER_BYTES;
        cursor               = first_command_offset;
        // Bound the walk by the two-word minimum command size
        for (command_index = 0; command_index < byteCount; ++command_index)
        {
            if (cursor >= end_offset)
            {
                break;
            }
            is_valid = (BINARY_ALIGN(BINARY_EXTEND_BYTES) <= (end_offset - cursor));
            if (is_valid)
            {
                command_size = GetUnsigned(buffer, cursor + BINARY_COMMAND_SIZE_OFFSET, BINARY_USHORT_BYTES);
                is_valid     = (BINARY_ALIGN(BINARY_EXTEND_BYTES) <= command_size) && (0 == (command_size % BINARY_WORD_BYTES)) && (command_size <= (end_offset - cursor));
            }
            if (is_valid && (BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, BINARY_USHORT_BYTES)))
            {
                is_valid = (BINARY_ALIGN(BINARY_COMMAND_BYTES) <= command_size);
            }
            if (is_valid)
            {
                is_valid = ReadCommand(definition, buffer, first_command_offset, first_widget_index, &last_widget_index, cursor, command_size);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            cursor += command_size;
        }
    }
    if (is_valid)
    {
        (*offset) = end_offset + BINARY_BLOCK_END_BYTES;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks Table 7-1: the block header, an optional color table, each picture of
 *  Table 7-2 with its pixel data padded to 32 bits, an optional reserved word,
 *  and the end code. Bytes per pixel and per color table entry follow
 *  Table 7-3; an implementation-dependent format cannot be measured and
 *  returns FALSE. Appendix C shows the end code directly after the last
 *  picture, while Table 7-1 lists a reserved word before it, so both are accepted.
 */
static bool_t MeasurePictureBlock(const byte_t* buffer, uint64_t byteCount, uint64_t offset, uint64_t* blockSize)
{
    uint64_t cursor;
    uint64_t pixel_bytes;
    uint32_t pixel_format;
    uint32_t color_format;
    uint32_t picture_count;
    uint32_t color_count;
    uint32_t picture_index;
    bool_t   is_valid;

    cursor        = offset + BINARY_PICTURE_HEADER_BYTES;
    pixel_format  = 0;
    picture_count = 0;
    is_valid      = (BINARY_PICTURE_HEADER_BYTES <= (byteCount - offset));
    if (is_valid)
    {
        pixel_format  = buffer[offset + BINARY_PIXEL_FORMAT_OFFSET];
        picture_count = buffer[offset + BINARY_PICTURE_COUNT_OFFSET];
        color_count   = buffer[offset + BINARY_COLOR_COUNT_OFFSET];
        color_format  = buffer[offset + BINARY_COLOR_FORMAT_OFFSET];
        is_valid      = (BINARY_PIX_FMT_RGBA_8 <= pixel_format) && (BINARY_PIX_FMT_COLOR_INDEXED_8 >= pixel_format);
        if (is_valid && (0 != color_count))
        {
            is_valid = (BINARY_PIX_FMT_RGBA_8 <= color_format) && (BINARY_PIX_FMT_COLOR_INDEXED_8 >= color_format);
            if (is_valid)
            {
                // Measure color table entries in their own format and pad them as a whole
                cursor += BINARY_ALIGN((uint64_t)color_count * c_pixelFormatBytes[color_format - BINARY_PIX_FMT_RGBA_8]);
            }
        }
    }
    for (picture_index = 0; is_valid && (picture_index < picture_count); ++picture_index)
    {
        is_valid = (cursor <= byteCount) && (BINARY_PICTURE_BYTES <= (byteCount - cursor)) && (BINARY_BEGIN_PICTURE == buffer[cursor]);
        if (is_valid)
        {
            pixel_bytes = (uint64_t)GetUnsigned(buffer, cursor + BINARY_PICTURE_WIDTH_OFFSET, BINARY_USHORT_BYTES) * GetUnsigned(buffer, cursor + BINARY_PICTURE_HEIGHT_OFFSET, BINARY_USHORT_BYTES) * c_pixelFormatBytes[pixel_format - BINARY_PIX_FMT_RGBA_8];
            cursor     += BINARY_PICTURE_BYTES + BINARY_ALIGN(pixel_bytes);
            is_valid    = (cursor <= byteCount) && (BINARY_BLOCK_END_BYTES <= (byteCount - cursor)) && (BINARY_END_PICTURE == buffer[cursor]);
            cursor     += BINARY_BLOCK_END_BYTES;
        }
    }
    if (is_valid)
    {
        is_valid = (cursor <= byteCount) && (BINARY_BLOCK_END_BYTES <= (byteCount - cursor));
    }
    if (is_valid && (BINARY_END_PICTURE_BLOCK != buffer[cursor]))
    {
        // Skip the Table 7-1 reserved word before the end code
        cursor  += BINARY_WORD_BYTES;
        is_valid = (cursor <= byteCount) && (BINARY_BLOCK_END_BYTES <= (byteCount - cursor)) && (BINARY_END_PICTURE_BLOCK == buffer[cursor]);
    }
    if (is_valid)
    {
        (*blockSize) = (cursor + BINARY_BLOCK_END_BYTES) - offset;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Symbol and animation law blocks (Tables 4.5.3.2-5 and 9.2-1) carry a Block
 *  Size after a padded code and end with their end code; picture blocks are
 *  measured structurally. The block is appended unchanged to the caller's block
 *  storage, so Arinc661_Definition_WriteBinary can emit it again. The storage
 *  pointer may be NULL only while its capacity is zero.
 */
static bool_t ReadOpaqueBlock(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount, uint64_t* offset)
{
    uint64_t block_size;
    uint32_t end_code;
    bool_t   is_valid;

    block_size = 0;
    if (BINARY_BEGIN_PICTURE_BLOCK == buffer[(*offset)])
    {
        is_valid = MeasurePictureBlock(buffer, byteCount, (*offset), &block_size);
    }
    else
    {
        end_code = BINARY_END_ANIMATION_BLOCK;
        if (BINARY_BEGIN_SYMBOL_BLOCK == buffer[(*offset)])
        {
            end_code = BINARY_END_SYMBOL_BLOCK;
        }
        is_valid = (BINARY_SIZED_HEADER_BYTES <= (byteCount - (*offset)));
        if (is_valid)
        {
            block_size = GetUnsigned(buffer, (*offset) + BINARY_BLOCK_SIZE_OFFSET, BINARY_ULONG_BYTES);
            is_valid   = ((BINARY_SIZED_HEADER_BYTES + BINARY_BLOCK_END_BYTES) <= block_size) && (block_size <= (byteCount - (*offset)));
        }
        if (is_valid)
        {
            is_valid = (end_code == buffer[(*offset) + block_size - BINARY_BLOCK_END_BYTES]);
        }
    }
    if (is_valid)
    {
        is_valid = (block_size <= (definition->blockCapacity - definition->blockByteCount));
    }
    if (is_valid)
    {
        (void)memcpy(&definition->blocks[definition->blockByteCount], &buffer[(*offset)], (size_t)block_size);
        definition->blockByteCount += block_size;
        (*offset)                  += block_size;
    }

    return is_valid;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Storage pointers may be NULL only with a zero capacity.
 */
void Arinc661_Definition_Initialize(Arinc661_Definition_t* definition, Arinc661_Layer_t* layers, uint32_t layerCapacity, Arinc661_Widget_t* widgets, uint32_t widgetCapacity, Arinc661_Property_t* properties, uint32_t propertyCapacity, byte_t* blocks, uint64_t blockCapacity)
{
    LOGASSERT(NULL != definition);
    LOGASSERT((NULL != layers) || (0 == layerCapacity));
    LOGASSERT((NULL != widgets) || (0 == widgetCapacity));
    LOGASSERT((NULL != properties) || (0 == propertyCapacity));
    LOGASSERT((NULL != blocks) || (0 == blockCapacity));
    (void)memset(definition, 0, sizeof((*definition)));
    definition->layers           = layers;
    definition->widgets          = widgets;
    definition->properties       = properties;
    definition->blocks           = blocks;
    definition->layerCapacity    = layerCapacity;
    definition->widgetCapacity   = widgetCapacity;
    definition->propertyCapacity = propertyCapacity;
    definition->blockCapacity    = blockCapacity;
    ResetDefinition(definition);
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
 *  Versions must fit uint32_t. Failure clears layerCount, widgetCount, propertyCount,
 *  blockByteCount, and charsetEncoding; scalar fields and array entries may still
 *  contain partial data.
 */
bool_t Arinc661_Definition_Read(Arinc661_Definition_t* definition, byte_t* bufferToReadXmlFrom, uint64_t sizeOfXmlBuffer, void* workspace, uint64_t workspaceSize)
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
    LOGASSERT(NULL != bufferToReadXmlFrom);
    LOGASSERT(NULL != workspace);
    // Reset result counts while retaining the supplied arrays and capacities; XML carries no binary blocks
    ResetDefinition(definition);
    has_workspace = InitializeXmlWorkspace(&xml, workspace, workspaceSize);
    is_valid      = has_workspace;
    if (is_valid)
    {
        // Parse the source into the temporary XML workspace
        is_valid = (0 < FACEOE_Utl_Xml_ReadDocument(&xml.document, bufferToReadXmlFrom, sizeOfXmlBuffer, xml.nodes, ARINC661_XML_NODE_CAPACITY, xml.attributes, ARINC661_XML_ATTRIBUTE_CAPACITY));
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
        ResetDefinition(definition);
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
uint64_t Arinc661_Definition_Write(const Arinc661_Definition_t* definition, byte_t* bufferToWriteXmlTo, uint64_t capacityOfXmlBuffer, void* workspace, uint64_t workspaceSize)
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
    LOGASSERT(NULL != bufferToWriteXmlTo);
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
        byte_count = FACEOE_Utl_Xml_WriteDocument(&xml.document, bufferToWriteXmlTo, capacityOfXmlBuffer);
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the binary Definition File of Table 4.5.3.2-1, for example:
 *
 * \verbatim
 * A661      A661_DF_MAGIC_NUMBER
 * 01        LibraryVersion      -> libraryVersion
 * 04        SuppVersion         -> supplementVersion
 * 6788      AppliIdent          -> property ApplicationId
 * 0000      Size of the OEM free data, which is skipped
 * C8 ...    optional charset encoding block -> charsetEncoding
 * F0 ...    symbol, picture, and animation law blocks -> blocks, unchanged
 * A0 ...    layer blocks
 * E0        A661_DF_FOOTER
 * 000000    UnusedPad
 * \endverbatim
 *
 *  Blocks are accepted in any order. Bytes after the footer are ignored.
 *  Failure clears layerCount, widgetCount, propertyCount, blockByteCount, and
 *  charsetEncoding; scalar fields and array entries may still contain partial
 *  data.
 */
bool_t Arinc661_Definition_ReadBinary(Arinc661_Definition_t* definition, const byte_t* bufferToReadBinaryFrom, uint64_t sizeOfBinaryBuffer)
{
    uint64_t offset;
    uint64_t block_index;
    uint32_t block_code;
    bool_t   has_footer;
    bool_t   is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != bufferToReadBinaryFrom);
    LOGASSERT((NULL != definition->blocks) || (0 == definition->blockCapacity));
    // Reset result counts while retaining the supplied arrays and capacities
    ResetDefinition(definition);
    has_footer = FALSE;
    offset     = BINARY_HEADER_BYTES;
    is_valid   = (BINARY_HEADER_BYTES <= sizeOfBinaryBuffer);
    if (is_valid)
    {
        is_valid = (BINARY_DF_MAGIC_NUMBER == GetUnsigned(bufferToReadBinaryFrom, BINARY_HEADER_MAGIC_OFFSET, BINARY_USHORT_BYTES));
    }
    if (is_valid)
    {
        definition->name[0]           = '\0';
        definition->libraryVersion    = bufferToReadBinaryFrom[BINARY_HEADER_LIBRARY_VERSION_OFFSET];
        definition->supplementVersion = bufferToReadBinaryFrom[BINARY_HEADER_SUPPLEMENT_VERSION_OFFSET];
        is_valid                      = AddIntegerProperty(definition, ARINC661_PROPERTY_DEFINITION, 0, BINARY_PROPERTY_APPLICATION_ID, GetUnsigned(bufferToReadBinaryFrom, BINARY_HEADER_APPLI_IDENT_OFFSET, BINARY_USHORT_BYTES));
        // Skip the implementation-dependent OEM data, which occupies whole words
        offset += BINARY_ALIGN(GetUnsigned(bufferToReadBinaryFrom, BINARY_HEADER_OEM_DATA_SIZE_OFFSET, BINARY_USHORT_BYTES));
    }
    if (is_valid)
    {
        // Bound the walk by the one-word minimum block size
        for (block_index = 0; block_index < sizeOfBinaryBuffer; ++block_index)
        {
            is_valid = (offset <= sizeOfBinaryBuffer) && (BINARY_BLOCK_END_BYTES <= (sizeOfBinaryBuffer - offset));
            if (FALSE == is_valid)
            {
                break;
            }
            block_code = bufferToReadBinaryFrom[offset];
            if (BINARY_DF_FOOTER == block_code)
            {
                has_footer = TRUE;
                break;
            }
            if (BINARY_BEGIN_LAYER_BLOCK == block_code)
            {
                is_valid = ReadLayer(definition, bufferToReadBinaryFrom, sizeOfBinaryBuffer, &offset);
            }
            else if (BINARY_BEGIN_CHARSET_BLOCK == block_code)
            {
                // Read the block as code, pad, encoding, end code, pad (Table 4.5.3.2-6) and allow one per file
                is_valid = (BINARY_CHARSET_BLOCK_BYTES <= (sizeOfBinaryBuffer - offset)) && (BINARY_END_CHARSET_BLOCK == bufferToReadBinaryFrom[offset + BINARY_CHARSET_END_OFFSET]) && (0 == definition->charsetEncoding);
                if (is_valid)
                {
                    definition->charsetEncoding = GetUnsigned(bufferToReadBinaryFrom, offset + BINARY_CHARSET_ENCODING_OFFSET, BINARY_USHORT_BYTES);
                    is_valid                    = (0 != definition->charsetEncoding);
                    offset                     += BINARY_CHARSET_BLOCK_BYTES;
                }
            }
            else if ((BINARY_BEGIN_SYMBOL_BLOCK == block_code) || (BINARY_BEGIN_PICTURE_BLOCK == block_code) || (BINARY_BEGIN_ANIMATION_BLOCK == block_code))
            {
                is_valid = ReadOpaqueBlock(definition, bufferToReadBinaryFrom, sizeOfBinaryBuffer, &offset);
            }
            else
            {
                is_valid = FALSE;
            }
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }
    is_valid = is_valid && has_footer;
    if (FALSE == is_valid)
    {
        // Hide all partially read entries by clearing the published counts
        ResetDefinition(definition);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the header, the charset encoding block when charsetEncoding is set,
 *  the retained symbol, picture, and animation law blocks unchanged, one layer
 *  block per layer, and the footer. Each layer block holds the commands of its
 *  widget array entries in array order, so a parent always precedes its
 *  children and an extension follows the widget it extends. For example, one
 *  layer with one panel:
 *
 * \verbatim
 * A661 01 04 6788 0000        header: versions, ApplicationId, no OEM data
 * A0 01 0000 0000002C         layer: LayerId, ContextNumber, Block Size
 * CA01 0020 A1F0 1221 0000    command: size, A661_PANEL, WidgetIdent, ParentIdent
 * 01 01 ... 0000 00 00        panel Creation Structure parameters
 * C0 000000                   A661_END_LAYER_BLOCK
 * E0 000000                   A661_DF_FOOTER
 * \endverbatim
 *
 *  ApplicationId, LayerId, and ContextNumber come from the properties of those
 *  names and default to zero; other Definition File and layer properties, and
 *  all names, have no binary representation and are not written. The complete
 *  file is sized even when it exceeds capacityOfBinaryBuffer, in which case nothing past
 *  capacityOfBinaryBuffer is stored and zero is returned.
 */
uint64_t Arinc661_Definition_WriteBinary(const Arinc661_Definition_t* definition, byte_t* bufferToWriteBinaryTo, uint64_t capacityOfBinaryBuffer)
{
    const Arinc661_Property_t* property;
    int64_t                    application_id;
    int64_t                    layer_id;
    int64_t                    context_number;
    uint64_t                   byte_count;
    uint64_t                   byte_index;
    uint64_t                   offset;
    uint64_t                   layer_offset;
    uint32_t                   layer_index;
    uint32_t                   widget_index;
    uint32_t                   last_widget_index;
    bool_t                     is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != bufferToWriteBinaryTo);
    LOGASSERT((NULL != definition->blocks) || (0 == definition->blockByteCount));
    // Keep zero as the failure result until the complete file is written
    byte_count     = 0;
    application_id = 0;
    layer_id       = 0;
    context_number = 0;
    offset         = BINARY_HEADER_BYTES;
    is_valid       = (UINT8_MAX >= definition->libraryVersion) && (UINT8_MAX >= definition->supplementVersion) && (UINT16_MAX >= definition->charsetEncoding);
    if (is_valid)
    {
        property = FindProperty(definition, ARINC661_PROPERTY_DEFINITION, 0, BINARY_PROPERTY_APPLICATION_ID, 0);
        is_valid = ScanInteger(property, 0, UINT16_MAX, &application_id);
    }
    if (is_valid)
    {
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, BINARY_HEADER_MAGIC_OFFSET, BINARY_DF_MAGIC_NUMBER, BINARY_USHORT_BYTES);
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, BINARY_HEADER_LIBRARY_VERSION_OFFSET, definition->libraryVersion, BINARY_UCHAR_BYTES);
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, BINARY_HEADER_SUPPLEMENT_VERSION_OFFSET, definition->supplementVersion, BINARY_UCHAR_BYTES);
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, BINARY_HEADER_APPLI_IDENT_OFFSET, (uint32_t)application_id, BINARY_USHORT_BYTES);
        // Write no OEM free data
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, BINARY_HEADER_OEM_DATA_SIZE_OFFSET, 0, BINARY_USHORT_BYTES);
        if (0 != definition->charsetEncoding)
        {
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset, (BINARY_BEGIN_CHARSET_BLOCK << BINARY_BLOCK_CODE_SHIFT) | definition->charsetEncoding, BINARY_ULONG_BYTES);
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset + BINARY_CHARSET_END_OFFSET, BINARY_END_CHARSET_BLOCK << BINARY_BLOCK_CODE_SHIFT, BINARY_ULONG_BYTES);
            offset += BINARY_CHARSET_BLOCK_BYTES;
        }
        // Write symbol, picture, and animation law blocks before the layers (Table 4.5.3.2-1)
        for (byte_index = 0; byte_index < definition->blockByteCount; ++byte_index)
        {
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset + byte_index, definition->blocks[byte_index], BINARY_UCHAR_BYTES);
        }
        offset += definition->blockByteCount;
        for (layer_index = 0; layer_index < definition->layerCount; ++layer_index)
        {
            layer_offset      = offset;
            last_widget_index = UINT32_MAX;
            property          = FindProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_LAYER_ID, 0);
            is_valid          = ScanInteger(property, 0, UINT8_MAX, &layer_id);
            if (is_valid)
            {
                property = FindProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_CONTEXT_NUMBER, 0);
                is_valid = ScanInteger(property, 0, UINT16_MAX, &context_number);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset, BINARY_BEGIN_LAYER_BLOCK, BINARY_UCHAR_BYTES);
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset + BINARY_LAYER_IDENT_OFFSET, (uint32_t)layer_id, BINARY_UCHAR_BYTES);
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset + BINARY_CONTEXT_NUMBER_OFFSET, (uint32_t)context_number, BINARY_USHORT_BYTES);
            offset += BINARY_LAYER_HEADER_BYTES;
            for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
            {
                // Select entries belonging to this layer while preserving their array order
                if (layer_index == definition->widgets[widget_index].layerIndex)
                {
                    is_valid = WriteCommand(definition, widget_index, last_widget_index, bufferToWriteBinaryTo, capacityOfBinaryBuffer, &offset);
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                    if (FALSE == definition->widgets[widget_index].isExtension)
                    {
                        last_widget_index = widget_index;
                    }
                }
            }
            if (is_valid)
            {
                PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset, (uint32_t)BINARY_END_LAYER_BLOCK << BINARY_BLOCK_CODE_SHIFT, BINARY_BLOCK_END_BYTES);
                offset   += BINARY_BLOCK_END_BYTES;
                // Count the block header and the end code in Block Size
                is_valid  = ((offset - layer_offset) <= UINT32_MAX);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset + BINARY_BLOCK_SIZE_OFFSET, (uint32_t)(offset - layer_offset), BINARY_ULONG_BYTES);
        }
    }
    if (is_valid)
    {
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset, (uint32_t)BINARY_DF_FOOTER << BINARY_BLOCK_CODE_SHIFT, BINARY_BLOCK_END_BYTES);
        offset   += BINARY_BLOCK_END_BYTES;
        is_valid  = (offset <= capacityOfBinaryBuffer);
    }
    if (is_valid)
    {
        byte_count = offset;
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts the supported widget types or the supported extension types.
 */
uint32_t Arinc661_Definition_GetTypeCount(bool_t isExtension)
{
    uint32_t type_count;

    type_count = (uint32_t)COUNT_OF(c_binaryWidgets);
    if (isExtension)
    {
        type_count = (uint32_t)COUNT_OF(c_binaryExtensions);
    }

    return type_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Widget types are in name order; extension types follow Table 4.6-12 order.
 */
const char_t* Arinc661_Definition_GetTypeName(bool_t isExtension, uint32_t typeIndex)
{
    const char_t* name;

    LOGASSERT(typeIndex < Arinc661_Definition_GetTypeCount(isExtension));
    name = c_binaryWidgets[typeIndex].name;
    if (isExtension)
    {
        name = c_binaryExtensions[typeIndex].name;
    }

    return name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts the named parameters of the type, leaving out unused pads and alignment.
 */
bool_t Arinc661_Definition_FindType(const char_t* typeName, bool_t isExtension, uint32_t* parameterCount)
{
    const BinaryWidget_t* type;
    uint32_t              field_index;
    uint32_t              count;
    bool_t                is_found;

    LOGASSERT(NULL != typeName);
    LOGASSERT(NULL != parameterCount);
    type     = FindTypeByName(typeName, isExtension);
    is_found = (NULL != type);
    if (is_found)
    {
        count = 0;
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            if (NULL != type->fields[field_index].name)
            {
                count += 1;
            }
        }
        (*parameterCount) = count;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Describes the parameter at parameterIndex among the named parameters of the type, in
 *  Creation Structure order. An array names the parameter holding its entry count.
 */
bool_t Arinc661_Definition_GetParameter(const char_t* typeName, bool_t isExtension, uint32_t parameterIndex, Arinc661_Parameter_t* parameter)
{
    const BinaryWidget_t* type;
    const BinaryField_t*  field;
    uint32_t              field_index;
    uint32_t              named_index;
    bool_t                is_found;

    LOGASSERT(NULL != typeName);
    LOGASSERT(NULL != parameter);
    type     = FindTypeByName(typeName, isExtension);
    is_found = FALSE;
    if (NULL != type)
    {
        named_index = 0;
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            field = &type->fields[field_index];
            if (NULL == field->name)
            {
                continue;
            }
            if (named_index == parameterIndex)
            {
                is_found                    = TRUE;
                parameter->name             = field->name;
                parameter->byteCount        = field->byteCount;
                parameter->elementsPerEntry = field->repeat;
                parameter->countName        = NULL;
                parameter->values           = field->values;
                parameter->valueCount       = field->valueCount;
                if (0 != field->repeat)
                {
                    parameter->countName = type->fields[field->countField].name;
                }
                switch (field->type)
                {
                    case BINARY_FIELD_UCHAR:       parameter->kind = ARINC661_PARAMETER_UCHAR; break;
                    case BINARY_FIELD_USHORT:      parameter->kind = ARINC661_PARAMETER_USHORT; break;
                    case BINARY_FIELD_LONG:        parameter->kind = ARINC661_PARAMETER_LONG; break;
                    case BINARY_FIELD_ULONG:       parameter->kind = ARINC661_PARAMETER_ULONG; break;
                    case BINARY_FIELD_FLOAT:       parameter->kind = ARINC661_PARAMETER_FLOAT; break;
                    case BINARY_FIELD_FR180:       parameter->kind = ARINC661_PARAMETER_FR180; break;
                    case BINARY_FIELD_FR32768:     parameter->kind = ARINC661_PARAMETER_FR32768; break;
                    case BINARY_FIELD_STRING:      parameter->kind = ARINC661_PARAMETER_STRING; break;
                    case BINARY_FIELD_BYTES:       parameter->kind = ARINC661_PARAMETER_BYTES; break;
                    case BINARY_FIELD_SIZED_BYTES: parameter->kind = ARINC661_PARAMETER_SIZED_BYTES; break;
                    case BINARY_FIELD_REGION:      parameter->kind = ARINC661_PARAMETER_REGION; break;
                    case BINARY_FIELD_LAYER_REQUEST: parameter->kind = ARINC661_PARAMETER_LAYER_REQUEST; break;
                    case BINARY_FIELD_PAD:
                    case BINARY_FIELD_ALIGN:
                    default:                       LOGFATAL("Named parameter with a pad type!");
                }
                break;
            }
            named_index += 1;
        }
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts the constant names of section 4.6 that property values may use.
 */
uint32_t Arinc661_Definition_GetConstantCount(void)
{
    return (uint32_t)COUNT_OF(c_binaryConstants);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Constants are in name order.
 */
const char_t* Arinc661_Definition_GetConstantName(uint32_t constantIndex)
{
    LOGASSERT(constantIndex < Arinc661_Definition_GetConstantCount());

    return c_binaryConstants[constantIndex].name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the numeric value the binary form stores for the constant.
 */
uint32_t Arinc661_Definition_GetConstantValue(uint32_t constantIndex)
{
    LOGASSERT(constantIndex < Arinc661_Definition_GetConstantCount());

    return c_binaryConstants[constantIndex].value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An extension type the table does not list applies nowhere.
 */
bool_t Arinc661_Definition_ExtensionApplies(const char_t* extensionType, const char_t* widgetType)
{
    uint32_t extension_index;
    uint32_t widget_index;
    bool_t   applies;

    LOGASSERT(NULL != extensionType);
    LOGASSERT(NULL != widgetType);
    applies = FALSE;
    for (extension_index = 0; extension_index < EXTENSION_TARGET_COUNT; ++extension_index)
    {
        if (0 == strcmp(extensionType, c_extensionTargets[extension_index].extension))
        {
            for (widget_index = 0; widget_index < c_extensionTargets[extension_index].widgetCount; ++widget_index)
            {
                if (0 == strcmp(widgetType, c_extensionTargets[extension_index].widgets[widget_index]))
                {
                    applies = TRUE;
                    break;
                }
            }
            break;
        }
    }

    return applies;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Lists the widget types in table order.
 */
const char_t* Arinc661_Definition_GetExtensionTarget(const char_t* extensionType, uint32_t index)
{
    uint32_t      extension_index;
    const char_t* widget;

    LOGASSERT(NULL != extensionType);
    widget = NULL;
    for (extension_index = 0; extension_index < EXTENSION_TARGET_COUNT; ++extension_index)
    {
        if ((0 == strcmp(extensionType, c_extensionTargets[extension_index].extension)) && (index < c_extensionTargets[extension_index].widgetCount))
        {
            widget = c_extensionTargets[extension_index].widgets[index];
            break;
        }
    }

    return widget;
}
