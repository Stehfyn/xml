/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1, Supplement 7, Cockpit Display System Interfaces to User Systems:
 * section 4.5.3.2 (Definition File structure), section 4.5.3.3 (layer block commands),
 * section 4.6 (constants), section 7 (picture blocks), section 8 (widget extensions),
 * section 9 (animation law blocks), and the Creation Structure tables in sections 3 and 8.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_scan.h"

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
#define BINARY_MAX_FIELDS           (40U) ///< Largest Creation Structure parameter count in the tables

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
#define BINARY_FR_PRECISION       (10)            ///< Most fractional digits tried when formatting a fr() value; one fr(180) LSB is 8.4e-8
#define BINARY_FLOAT_PRECISION    (60)            ///< Most fractional digits tried when formatting a float; the smallest subnormal needs 45

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Encoding of one Creation Structure parameter (Part 1-7, section 3.3 data types).
 */
typedef enum BinaryFieldType_e
{
    BINARY_FIELD_PAD,         ///< UnusedPad bytes, written as zero
    BINARY_FIELD_UCHAR,       ///< Unsigned 8-bit integer
    BINARY_FIELD_USHORT,      ///< Unsigned 16-bit integer
    BINARY_FIELD_LONG,        ///< Signed 32-bit integer
    BINARY_FIELD_ULONG,       ///< Unsigned 32-bit integer
    BINARY_FIELD_FLOAT,       ///< IEEE 754 single precision
    BINARY_FIELD_FR180,       ///< Signed 32-bit fraction of 180
    BINARY_FIELD_FR32768,     ///< Signed 32-bit fraction of 32768
    BINARY_FIELD_STRING,      ///< Zero-terminated bytes
    BINARY_FIELD_BYTES,       ///< Fixed number of bytes with no numeric type, given as hexadecimal digits
    BINARY_FIELD_ALIGN,       ///< Zero bytes up to the next 32-bit boundary
    BINARY_FIELD_SIZED_BYTES, ///< Table 8.6.5-3 StaticParamBuffer: byte count, pad, bytes, and alignment, given as hexadecimal digits
    BINARY_FIELD_REGION,      ///< Tables 8.6.7-4 to 8.6.7-7 excluded region structure, sized by its type byte, given as hexadecimal digits
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
    const char_t* name;       ///< Type name, for example A661_PANEL or A661_LEGEND_ALIGN_EXTENSION
    uint16_t      code;       ///< WidgetType value in Table 4.6-7, or ExtensionType value in Table 4.6-12
    uint16_t      firstField; ///< Index of the first parameter in c_binaryFields
    uint16_t      fieldCount; ///< Number of parameters, including unused pads
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

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Digits of a hexadecimal property value; one array, so digit pointers can be subtracted.
 */
static const char_t c_binaryHexDigits[] = "0123456789ABCDEF";

/*************************************************************************************************/
/**
 * \brief Creation Structure parameters, grouped by widget type and then by extension type.
 *
 * \par Details:
 *  Transcribed from the Part 1-7 Creation Structure tables named in each group.
 *  Widget groups start after ParentIdent; extension groups start after
 *  ExtensionType. An array holds repeat elements for each entry counted by the
 *  parameter at countField; arrays of pairs, such as Vertices and
 *  BufferStructure, alternate the two members. Arrays are padded to 32 bits
 *  only where the table says so, which is recorded as an alignment entry.
 *  A661_SELECTOR is not listed because Table 4.6-7 gives it no WidgetType code.
 */
/*************************************************************************************************/
/**
 * rief Constant names each Creation Structure table lists for a parameter, shared where equal.
 */
static const char_t* const c_values0[] = {"A661_FALSE", "A661_TRUE", "A661_TRUE_WITH_VALIDATION"};
static const char_t* const c_values1[] = {"A661_FALSE", "A661_TRUE"};
static const char_t* const c_values2[] = {"A661_CLOCKWISE", "A661_COUNTER_CLOCKWISE"};
static const char_t* const c_values3[] = {"A661_SELECTED", "A661_UNSELECTED"};
static const char_t* const c_values4[] = {"A661_LEFT", "A661_CENTER", "A661_RIGHT"};
static const char_t* const c_values5[] = {"A661_LEFT", "A661_RIGHT"};
static const char_t* const c_values6[] = {"A661_OPEN_UP", "A661_OPEN_CENTERED", "A661_OPEN_DOWN"};
static const char_t* const c_values7[] = {"A661_EDB_CHANGE_CONFIRMED", "A661_EDB_ALL_CHANGE", "A661_EDB_OPEN_CLOSE"};
static const char_t* const c_values8[] = {"A661_CENTER", "A661_LEFT", "A661_RIGHT"};
static const char_t* const c_values9[] = {"A661_REPORT_ON_TRANSITION", "A661_REPORT_ALL"};
static const char_t* const c_values10[] = {"A661_NOT_CLAMPED", "A661_BOTH_CLAMPED"};
static const char_t* const c_values11[] = {"A661_TOP", "A661_BOTTOM", "A661_LEFT", "A661_RIGHT", "A661_ABSENT"};
static const char_t* const c_values12[] = {"A661_LEFT", "A661_RIGHT", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_values13[] = {"A661_EDB_CHANGE_CONFIRMED", "A661_EDB_OPEN_CLOSE", "A661_EDB_ALL_CHANGE"};
static const char_t* const c_values14[] = {"A661_FALSE", "A661_TRUE", "A661_SAME_LEVEL"};
static const char_t* const c_values15[] = {"A661_BOTTOM_CENTER", "A661_BOTTOM_LEFT", "A661_BOTTOM_RIGHT", "A661_CENTER", "A661_LEFT", "A661_RIGHT", "A661_TOP_CENTER", "A661_TOP_LEFT", "A661_TOP_RIGHT"};
static const char_t* const c_values16[] = {"A661_MDF_BRG_DIST_ACHDG", "A661_MDF_DIST_DIST", "A661_MDF_LAT_LONG", "A661_MDF_DIST_DIST_FIX", "A661_MDF_LEGACY"};
static const char_t* const c_values17[] = {"A661_EVENT_NONE", "A661_VALIDATION", "A661_VALIDATION_AND_WHEEL", "A661_WHEEL"};
static const char_t* const c_values18[] = {"A661_MDF_ABSOLUTE", "A661_MDF_RELATIVE", "A661_MDF_X_DIST"};
static const char_t* const c_values19[] = {"A661_MDF_ABSOLUTE", "A661_MDF_RELATIVE", "A661_MDF_Y_ALT"};
static const char_t* const c_values20[] = {"A661_ARC", "A661_HORZ_LINE", "A661_VERT_LINE"};
static const char_t* const c_values21[] = {"A661_LEFT", "A661_RIGHT", "A661_CENTER"};
static const char_t* const c_values22[] = {"A661_WRAP_NONE", "A661_WRAP_BOTH", "A661_WRAP_FIRST_TO_LAST", "A661_WRAP_LAST_TO_FIRST"};
static const char_t* const c_values23[] = {"A661_ABSENT", "A661_BOTTOM_CENTER", "A661_BOTTOM_LEFT", "A661_BOTTOM_RIGHT", "A661_CENTER", "A661_LEFT", "A661_RIGHT", "A661_TOP_CENTER", "A661_TOP_LEFT", "A661_TOP_RIGHT"};
static const char_t* const c_values24[] = {"A661_LEFT", "A661_CENTER", "A661_RIGHT", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_values25[] = {"A661_UNSELECTED", "A661_SELECTED"};
static const char_t* const c_values26[] = {"A661_OPEN_UP", "A661_OPEN_DOWN", "A661_CDS_DEPENDENT"};
static const char_t* const c_values27[] = {"A661_CENTER", "A661_LEFT", "A661_RIGHT", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_values28[] = {"A661_OPEN_UP", "A661_OPEN_DOWN"};
static const char_t* const c_values29[] = {"A661_SHUFFLE_UP", "A661_SHUFFLE_DOWN", "A661_SHUFFLE_LEFT", "A661_SHUFFLE_RIGHT", "A661_NO_SHUFFLE"};
static const char_t* const c_values30[] = {"A661_TRUE", "A661_FALSE"};
static const char_t* const c_values31[] = {"A661_BOTTOM_TO_TOP", "A661_TOP_TO_BOTTOM", "A661_LEFT_TO_RIGHT", "A661_RIGHT_TO_LEFT"};
static const char_t* const c_values32[] = {"A661_LEFT", "A661_RIGHT", "A661_CENTER", "A661_TOP", "A661_BOTTOM"};
static const char_t* const c_values33[] = {"A661_LOOP_FORWARD", "A661_LOOP_FORWARD_AND_RESET", "A661_LOOP_FORWARD_AND_BLANK", "A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK"};
static const char_t* const c_values34[] = {"A661_DONT_RUN", "A661_RUN", "A661_RUN_ONCE"};
static const char_t* const c_values35[] = {"A661_ABSENT", "A661_TOP", "A661_BOTTOM", "A661_LEFT", "A661_RIGHT"};
static const char_t* const c_values36[] = {"A661_OUT_OF_BOUNDS"};
static const char_t* const c_values37[] = {"A661_REPORT_ON_CDS", "A661_REPORT_ALL"};

static const BinaryField_t c_binaryFields[] =
{
    /* A661_ACTIVE_AREA: Table 3.3.1-2, ActiveArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    /* A661_ANIMATION_GROUP: Table 3.9.12-2, AnimationGroup Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Type"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_ANIMATION_ONPARAM: Table 3.9.8-2, AnimationOnParam Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TargetParamID"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("FromValue"),                     BINARY_FIELD_BYTES,        4U,  0U, 0U, NULL,       0U},
    {("ToValue"),                       BINARY_FIELD_BYTES,        4U,  0U, 0U, NULL,       0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_ANIMATION_ROTATION: Table 3.9.9-2, AnimationRotation Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("CenterX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("CenterY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("FromAngle"),                     BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("ToAngle"),                       BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Direction"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values2,  2U},
    {("NumberOfTurns"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_ANIMATION_SCALE: Table 3.9.10-2, AnimationScale Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("FromScaleX"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("FromScaleY"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("ToScaleX"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("ToScaleY"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_ANIMATION_TRANSLATION: Table 3.9.11-2, AnimationTranslation Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("FromValueX"),                    BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("FromValueY"),                    BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("ToValueX"),                      BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("ToValueY"),                      BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Duration"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Delay"),                         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_BASIC_CONTAINER: Table 3.3.2-2, BasicContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    /* A661_BLINKING_CONTAINER: Table 3.3.3-2, BlinkingContainer Creation Structure */
    {("BlinkingType"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_BROADCAST_RECEIVER: Table 3.8.9-2, BroadcastReceiver Creation Structure */
    {("NumberOfFields"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ParameterIdent"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("WidgetStructure"),               BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL,       0U},
    /* A661_BUFFER_FORMAT: Table 3.3.4-2, BufferFormat Creation Structure */
    {("NumberOfFields"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("BufferStructure"),               BINARY_FIELD_USHORT,       2U,  0U, 2U, NULL,       0U},
    /* A661_CHECK_BUTTON: Table 3.3.5-2, CheckButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("CheckButtonState"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values3,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values5,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_COMBO_BOX: Table 3.3.6-2, ComboBox Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values6,  3U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("EntryList"),                     BINARY_FIELD_STRING,       0U, 11U, 1U, NULL,       0U},
    /* A661_COMBO_BOX_EDIT: Table 3.4.7-2, ComboBoxEdit Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values6,  3U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values7,  3U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values8,  3U},
    {("EntryList"),                     BINARY_FIELD_STRING,       0U, 11U, 1U, NULL,       0U},
    /* A661_CONNECTOR: Table 3.3.7-2, Connector Creation Structure */
    {("ConnectorReference"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_CURSOR_OVER: Table 3.6.3-2, CursorOver Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("PositionReportMode"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values9,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    /* A661_CURSOR_POS_OVERLAY: Table 3.3.8-2, CursorPosOverlay Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    /* A661_CURSOR_REF: Table 3.6.2-2, CursorRef Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    /* A661_DATA_CONNECTOR: Table 3.9.16-2, DataConnector Creation Structure */
    {("ConnectorReference"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("DataLength"),                    BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Data"),                          BINARY_FIELD_ULONG,        4U,  3U, 1U, NULL,       0U},
    /* A661_DATA_SCALING_FR180: Table 3.8.8-2, DataScalingFR180 Creation Structure */
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values10, 2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("StartScaledValue"),              BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("EndScaledValue"),                BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    /* A661_DATA_SCALING_LONG: Table 3.8.6-2, DataScalingLong Creation Structure */
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values10, 2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("StartScaledValue"),              BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("EndScaledValue"),                BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    /* A661_DATA_SCALING_ULONG: Table 3.8.7-2, DataScalingULong Creation Structure */
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values10, 2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("StartScaledValue"),              BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("EndScaledValue"),                BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    /* A661_EDIT_BOX_MASKED: Table 3.3.9-2, EditBoxMasked Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AlphaMask"),                     BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("NumericMask"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values7,  3U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_EDIT_BOX_MULTI_LINE: Table 3.4.6-2, EditBoxMultiLine Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values7,  3U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values8,  3U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values11, 5U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_EDIT_BOX_NUMERIC: Table 3.3.10-2, EditBoxNumeric Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Value"),                         BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("TicsCoarse"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("TicsFine"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("MinValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("MaxValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StartCursorPosByte"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values12, 4U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values13, 3U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values8,  3U},
    {("LegendRemoved"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("NumericKeyFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("CyclicFlag"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("FormatString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("LegendString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_EDIT_BOX_NUMERIC_BCD: Table 3.6.1-5, EditBoxNumericBCD Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Value"),                         BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("MinValue"),                      BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("MaxValue"),                      BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("MinFieldsValues"),               BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("MaxFieldsValues"),               BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("TicsCoarse"),                    BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("TicsFine"),                      BINARY_FIELD_BYTES,        8U,  0U, 0U, NULL,       0U},
    {("NumberOfFields"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("LegendRemoved"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("PositiveStringLength"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NegativeStringLength"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NumericKeyFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("CyclicFlag"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("SizeOfFields"),                  BINARY_FIELD_UCHAR,        1U, 16U, 1U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL,       0U},
    {("FormatString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("LegendString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("PositiveString"),                BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("NegativeString"),                BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_EDIT_BOX_TEXT: Table 3.3.11-2, EditBoxText Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values7,  3U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values8,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_EVENT_HANDLER: Table 3.9.17-3, EventHandler Creation Structure */
    {("EnableHandler"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("TriggerLayerIdent"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("TriggerWidgetIdent"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TriggerEvent"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("NumWidgetParamResults"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NumLayerReqResults"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("WidgetParamResultArray"),        BINARY_FIELD_BYTES,       20U,  5U, 1U, NULL,       0U},
    {("LayerReqResultArray"),           BINARY_FIELD_LAYER_REQUEST,  0U,  6U, 1U, NULL,       0U},
    /* A661_EXTERNAL_SOURCE: Table 3.4.2-2, ExternalSource Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SourceReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("SourceX"),                       BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SourceY"),                       BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SourceDX"),                      BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SourceDY"),                      BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_FOCUS_IN: Table 3.6.4.2-2, FocusIn Creation Structure */
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_FOCUS_LINK: Table 3.6.4.1-2, FocusLink Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          4U,  0U, 0U, NULL,       0U},
    {("NextLayerConnectorRef"),         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_FOCUS_OUT: Table 3.6.4.3-2, FocusOut Creation Structure */
    {("NextFocusInAppId"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusInLayerId"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("NextFocusInId"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_FRAMING_REFRESH_CONTAINER: Table 3.9.18-2, FramingRefreshContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("CachingEnabled"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    /* A661_GESTURE_AREA: Table 3.9.6-4, GestureArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("AllowOutside"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NumberOfEvents"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("EventIdArray"),                  BINARY_FIELD_UCHAR,        1U, 10U, 1U, NULL,       0U},
    /* A661_GP_ARC_CIRCLE: Table 3.3.13-2, GpArcCircle Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("Radius"),                        BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_GP_ARC_ELLIPSE: Table 3.3.12-2, GpArcEllipse Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_GP_CROWN: Table 3.3.14-2, GpCrown Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("InnerRadius"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("OuterRadius"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_GP_LINE: Table 3.3.15-2, GpLine Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosXStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosYStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosXEnd"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosYEnd"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    /* A661_GP_LINE_POLAR: Table 3.3.16-2, GpLinePolar Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosXStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosYStart"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("LineLength"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    /* A661_GP_POLYLINE: Table 3.8.1-2, GpPolyline Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {("Closed"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  8U, 2U, NULL,       0U},
    /* A661_GP_RECTANGLE: Table 3.3.17-2, GpRectangle Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_GP_TRIANGLE: Table 3.3.18-2, GpTriangle Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosX2"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY2"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosX3"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY3"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_GP_TRIANGLE_FAN: Table 3.10.5-2, GpTriangleFan Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  9U, 2U, NULL,       0U},
    /* A661_GP_TRIANGLE_STRIP: Table 3.10.6-2, GpTriangleStrip Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values14, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  9U, 2U, NULL,       0U},
    /* A661_INK_AREA: Table 3.9.7-2, InkArea Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("MaxBufferSize"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_KEYBOARD_AREA: Table 3.9.2-2, KeyboardArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NumberOfKeys"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfKeys"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("KeyArray"),                      BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL,       0U},
    /* A661_LABEL: Table 3.3.20-2, Label Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Font"),                          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values15, 9U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_LABEL_COMPLEX: Table 3.3.21-2, LabelComplex Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values15, 9U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("DefaultStyleText"),              BINARY_FIELD_BYTES,       12U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_MAPHORZ: Table 3.3.25-2a, MapHorz Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("ScreenReferencePointX"),         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("ScreenReferencePointY"),         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("Range"),                         BINARY_FIELD_FR32768,      4U,  0U, 0U, NULL,       0U},
    {("ScreenRange"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("ProjectionType"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    /* A661_MAPHORZ_CONTAINER: Table 3.8.4-2, MapHorz_Container Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_MAPHORZ_ITEMLIST: Table 3.3.22-2, MapHorz_ItemList Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MaxNumberOfItem"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxItemSize"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_MAPHORZ_PANEL: Table 3.8.5-2, MapHorz_Panel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_MAPHORZ_SOURCE: Table 3.3.24-2a, MapHorz_Source Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MapDataFormat"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values16, 5U},
    {("EventFlag"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values17, 4U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_MAPHORZ_VERTEXBUFFER: Table 3.9.4-2, MapHorz_VertexBuffer Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Vertices"),                      BINARY_FIELD_LONG,         4U,  3U, 2U, NULL,       0U},
    /* A661_MAPVERT: Table 3.4.3-2a, MapVert Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("RangeX"),                        BINARY_FIELD_FR32768,      4U,  0U, 0U, NULL,       0U},
    {("RangeY"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("RefPosX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("RefPosY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("RefGeoPosX"),                    BINARY_FIELD_FR32768,      4U,  0U, 0U, NULL,       0U},
    {("RefGeoPosY"),                    BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    /* A661_MAPVERT_CONTAINER: Table 3.9.13-2, MapVert_Container Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_MAPVERT_ITEMLIST: Table 3.4.5-2a, MapVert_ItemList Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MaxNumberOfItem"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxItemSize"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_MAPVERT_PANEL: Table 3.9.14-2, MapVert_Panel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_MAPVERT_SOURCE: Table 3.4.4-2a, MapVert_Source Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MapDataFormatX"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values18, 3U},
    {("MapDataFormatY"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values19, 3U},
    {("EventFlag"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_MAP_BOUNDARY: Table 3.9.15-2, MapBoundary Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("BeginAngleArray"),               BINARY_FIELD_FR180,        4U,  3U, 1U, NULL,       0U},
    {("DistanceArray"),                 BINARY_FIELD_ULONG,        4U,  3U, 1U, NULL,       0U},
    {("AlgorithmArray"),                BINARY_FIELD_ULONG,        4U,  3U, 1U, c_values20, 3U},
    /* A661_MAP_EXTERNAL_SOURCE: Table 3.10.4-2, MapExternalSource Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SourceReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_MAP_GRID: Table 3.4.1-2, MapGrid Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("OffsetX"),                       BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("OffsetY"),                       BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("IncrementX"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("IncrementY"),                    BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("CountX"),                        BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("CountY"),                        BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_MASK_CONTAINER: Table 3.3.26-2, MaskContainer Creation Structure */
    {("MaskEnabled"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("MaskReference"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_MENU_BAR: Table 3.4.8-2, MenuBar Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Horizontal"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("ButtonPos"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("ButtonSize"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    /* A661_MULTI_STATE_BUTTON: Table 3.9.1-2, MultiStateButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ButtonState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values21, 3U},
    {("MaxNumberOfStates"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NumberOfStates"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,       0U, 13U, 1U, NULL,       0U},
    /* A661_MUTUALLY_EXCLUSIVE_CONTAINER: Table 3.5.1-2, MutuallyExclusiveContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("VisibleChild"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_NO_SERVICE_MONITOR: Table 3.8.10-2, NoServiceMonitor Creation Structure */
    {("ShowNoServiceIdent"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          4U,  0U, 0U, NULL,       0U},
    /* A661_NUMERIC_READOUT: Table 3.8.3-2, NumericReadout Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values15, 9U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("Value"),                         BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values12, 4U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("FormatString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("LegendString"),                  BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_PAGING_CONTAINER: Table 3.8.2-2, PagingContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("WrappingType"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values22, 4U},
    {("PagingControlPosition"),         BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values23, 10U},
    {("FinePageDelta"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("CoarsePageDelta"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("VisibleChild"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ReportVisibleChild"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_PANEL: Table 3.3.27-2, Panel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_PICTURE: Table 3.3.19-2, Picture Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_PICTURE_ANIMATED: Table 3.5.5-2, PictureAnimated Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfPictures"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("IndexOfFrequency"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("LoopFlag"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AnimationFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U,  7U, 1U, NULL,       0U},
    /* A661_PICTURE_PUSH_BUTTON: Table 3.3.28-2, Picture PushButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values24, 5U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_PICTURE_TOGGLE_BUTTON: Table 3.3.29-2, PictureToggleButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AlternatePictureReference"),     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values24, 5U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values25, 2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_POP_UP_MENU: Table 3.3.31-2, PopUpMenu Creation Structure */
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values26, 3U},
    {("NumberOfEntries"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PopUpIdentArray"),               BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL,       0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL,       0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U,  1U, 1U, NULL,       0U},
    {("StringArray"),                   BINARY_FIELD_STRING,       0U,  1U, 1U, NULL,       0U},
    /* A661_POP_UP_MENU_BUTTON: Table 3.3.32-2, PopUpMenuButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PopupPosX"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PopupPosY"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PopupSizeX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("PopupSizeY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLengthPopUp"),          BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values27, 5U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values28, 2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL,       0U},
    {("PopUpIdentArray"),               BINARY_FIELD_USHORT,       2U, 15U, 1U, NULL,       0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U, 15U, 1U, NULL,       0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U, 15U, 1U, NULL,       0U},
    {("StringArray"),                   BINARY_FIELD_STRING,       0U, 15U, 1U, NULL,       0U},
    /* A661_POP_UP_PANEL: Table 3.3.30-2, PopUpPanel Creation Structure */
    {("UAPositionFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AutomaticClosure"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_POP_UP_PANEL_BUTTON: Table 3.7.3-2, PopUpPanelButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PopupPosX"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PopupPosY"),                     BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PopupSizeX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("PopupSizeY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values27, 5U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {("UAPositionFlag"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AutomaticClosure"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_PROXY_BUTTON: Table 3.5.2-2, ProxyButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("DedicatedKeyIdent"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TargetWidgetIdent"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_PUSH_BUTTON: Table 3.3.33-2, PushButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values21, 3U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_RADIO_BOX: Table 3.3.34-2, RadioBox Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_ROTATION_CONTAINER: Table 3.3.35-2, RotationContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("CenterX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("CenterY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    /* A661_SCALE_CONTAINER: Table 3.10.1-2, ScaleContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("CenterX"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("CenterY"),                       BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("ScaleX"),                        BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("ScaleY"),                        BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    /* A661_SCROLL_LIST: Table 3.3.37-2, ScrollList Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("FirstAccessibleEntry"),          BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("FirstVisibleEntry"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values11, 5U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values21, 3U},
    {("FlagReportVisibleEntry"),        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("DefaultStyleText"),              BINARY_FIELD_BYTES,       12U,  0U, 0U, NULL,       0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U,  9U, 1U, NULL,       0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,       0U,  9U, 1U, NULL,       0U},
    /* A661_SCROLL_PANEL: Table 3.3.36-2, ScrollPanel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("LineDeltaX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("LineDeltaY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("PageDeltaX"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("PageDeltaY"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("HomeX"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("HomeY"),                         BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("FrameX"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("FrameY"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeXsheet"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeYsheet"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("BoundX"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("BoundY"),                        BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeXbound"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeYbound"),                    BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("HorizontalScroll"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values11, 5U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values11, 5U},
    {("FlagReportFramePos"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_SCROLL_WHEEL_AREA: Table 3.9.3-2, ScrollWheelArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    /* A661_SELECTION_LIST_BUTTON: Table 3.5.7-2, SelectionListButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values6,  3U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL,       0U},
    {("EntryList"),                     BINARY_FIELD_STRING,       0U, 11U, 1U, NULL,       0U},
    /* A661_SHUFFLE_TO_FIT_CONTAINER: Table 3.6.6-2, ShuffleToFitContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("NumberOfVisibleChildren"),       BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ShuffleToFitMode"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values29, 5U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("ItemSpacing"),                   BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_SIZE_TO_FIT_CONTAINER: Table 3.6.5-2, SizeToFitContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("NumberOfVisibleChildren"),       BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SizeToFitMode"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("ItemSpacing"),                   BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    /* A661_SLIDER: Table 3.5.4-2, Slider Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MinValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("MaxValue"),                      BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("Value"),                         BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("MajorTickInterval"),             BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    {("ShowMajorLabels"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values30, 2U},
    {("Orientation"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values31, 4U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values32, 5U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("LsbMultiple"),                   BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MinorTickMultiple"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MajorTickReference"),            BINARY_FIELD_FLOAT,        4U,  0U, 0U, NULL,       0U},
    /* A661_SYMBOL: Table 3.3.38-2, Symbol Creation Structure */
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    /* A661_SYMBOL_ANIMATED: Table 3.5.6-2, SymbolAnimated Creation Structure */
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("LoopType"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values33, 4U},
    {("NumberOfSymbols"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("IndexOfFrequency"),              BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("AnimationType"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values34, 3U},
    {("SymbolArray"),                   BINARY_FIELD_USHORT,       2U,  7U, 1U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL,       0U},
    {("OrientationArray"),              BINARY_FIELD_FR180,        4U,  7U, 1U, NULL,       0U},
    {("MovementXArray"),                BINARY_FIELD_LONG,         4U,  7U, 1U, NULL,       0U},
    {("MovementYArray"),                BINARY_FIELD_LONG,         4U,  7U, 1U, NULL,       0U},
    /* A661_SYMBOL_PUSH_BUTTON: Table 3.7.1-2, SymbolPushButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values24, 5U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_SYMBOL_TOGGLE_BUTTON: Table 3.7.2-2, SymbolToggleButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AlternateSymbolReference"),      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values24, 5U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values25, 2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values4,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_TABBED_PANEL: Table 3.3.39-2, TabbedPanel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values27, 5U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values21, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("InsetSize"),                     BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_TABBED_PANEL_GROUP: Table 3.3.40-2, TabbedPanelGroup Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ActiveTabbedPanelID"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TabPosition"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values35, 5U},
    {("AutomaticInsetSizeFlag"),        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_TOGGLE_BUTTON: Table 3.3.41-2, ToggleButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values25, 2U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values21, 3U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("LabelString"),                   BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,       0U,  0U, 0U, NULL,       0U},
    /* A661_TOUCH_AREA: Table 3.9.5-2, TouchArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values9,  2U},
    {("AllowOutside"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("MaxTouchPoints"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("NumberOfTouchPoints"),           BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    /* A661_TRANSLATION_CONTAINER: Table 3.3.42-2, TranslationContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("TranslationX"),                  BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("TranslationY"),                  BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    /* A661_TREE: Table 3.10.3-2, Tree Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values0,  3U},
    {("Visible"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("PosX"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("PosY"),                          BINARY_FIELD_LONG,         4U,  0U, 0U, NULL,       0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,        4U,  0U, 0U, NULL,       0U},
    {("FirstVisibleEntry"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("FlagReportVisibleEntry"),        BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values8,  3U},
    {("NodeStyleArray"),                BINARY_FIELD_USHORT,       2U, 12U, 1U, NULL,       0U},
    {("ParentArray"),                   BINARY_FIELD_USHORT,       2U, 12U, 1U, NULL,       0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,        1U, 12U, 1U, NULL,       0U},
    {("OpenArray"),                     BINARY_FIELD_UCHAR,        1U, 12U, 1U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL,       0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,       0U, 12U, 1U, NULL,       0U},
    /* A661_WATCHDOG_CONTAINER: Table 3.5.3-2, WatchdogContainer Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("TimePeriod"),                    BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ValidCountLimit"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("FailCountLimit"),                BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("Refresh"),                       BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("ShowIfFailIdent"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ShowFail"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values30, 2U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_DIRECTIONAL_TABBING_EXTENSION: Table 8.5.1-2, DirectionalTabbingExtension Creation Structure */
    {("NumDirections"),                 BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("TabDirectionWidgetIdArray"),     BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL,       0U},
    /* A661_CURSOR_EVENTS_EXTENSION: Table 8.5.2-2, CursorEventsExtension Creation Structure */
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL,       0U},
    /* A661_LEGEND_ALIGN_EXTENSION: Table 8.5.3-2, LegendStringAlignment Creation Structure */
    {("LegendAlignment"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values8,  3U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_CHILD_INDEX_EXTENSION: Table 8.5.4-2, VisibleChildIndexExtension Creation Structure */
    {("VisibleChildIndex"),             BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_INITIAL_FOCUS_EXTENSION: Table 8.5.5-2, InitialFocusExtension Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("InitialFocus"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_FOCUS_STOP_EXTENSION: Table 8.5.6-2, FocusStopExtension Creation Structure */
    {("StopForward"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("StopBackward"),                  BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    /* A661_CURSOR_SHAPE_EXTENSION: Table 8.5.7-2, CursorShapeExtension Creation Structure */
    {("AllowChange"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values1,  2U},
    {("CursorIndex"),                   BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    /* A661_PICTURE_EXTENSION: Table 8.6.1-2, PictureExtension Creation Structure */
    {("MaxNumberOfPictures"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfPictures"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values24, 5U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL,       0U},
    /* A661_SYMBOL_EXTENSION: Table 8.6.2-2, SymbolExtension Creation Structure */
    {("MaxNumberOfSymbols"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfSymbols"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values24, 5U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("SymbolArray"),                   BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL,       0U},
    /* A661_TICS_ARRAY_EXTENSION: Table 8.6.3-2, TicsArrayExtension Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfTicsCoarse"),         BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfTicsCoarse"),            BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("MaxNumberOfTicsFine"),           BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("NumberOfTicsFine"),              BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("TicsCoarseArray"),               BINARY_FIELD_FLOAT,        4U,  2U, 2U, NULL,       0U},
    {("TicsFineArray"),                 BINARY_FIELD_FLOAT,        4U,  4U, 2U, NULL,       0U},
    /* A661_STYLE_SET_EXTENSION: Table 8.6.4-2, StyleSetExtension Creation Structure */
    {("MaxStyleSetArraySize"),          BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("StyleSetArraySize"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("StyleSetArray"),                 BINARY_FIELD_USHORT,       2U,  1U, 1U, NULL,       0U},
    /* A661_STATIC_PARAM_BUFFER_EXTENSION: Table 8.6.5-2, StaticParamBufferExtension Creation Structure */
    {("NumberOfBuffers"),               BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("BufferArray"),                   BINARY_FIELD_SIZED_BYTES,  0U,  0U, 1U, NULL,       0U},
    /* A661_MULTI_SELECTION_EXTENSION: Table 8.6.6-2, MultiSelectionExtension Creation Structure */
    {("MaximumSelectableEntries"),      BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    /* A661_EXCLUDED_REGIONS_EXTENSION: Table 8.6.7-2, ExcludedRegionsExtension Creation Structure */
    {("MaxNumberOfRegions"),            BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    {("EffectivityArray"),              BINARY_FIELD_UCHAR,        1U,  0U, 1U, NULL,       0U},
    {(NULL),                            BINARY_FIELD_ALIGN,        0U,  0U, 0U, NULL,       0U},
    {("ExcludedRegionsList"),           BINARY_FIELD_REGION,       0U,  0U, 1U, NULL,       0U},
    /* A661_BOUNDARY_CHECK_EXTENSION: Table 8.6.8-2, BoundaryCheckExtension Creation Structure */
    {("BoundaryCheckMode"),             BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values36, 1U},
    {(NULL),                            BINARY_FIELD_PAD,          1U,  0U, 0U, NULL,       0U},
    /* A661_REORDER_EXTENSION: Table 8.7.1-2, ReorderExtension Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,          2U,  0U, 0U, NULL,       0U},
    /* A661_WORD_WRAP_EXTENSION: Table 8.7.2-2, WordWrapExtension Creation Structure */
    {("WordWrap"),                      BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    {("WrapStyle"),                     BINARY_FIELD_UCHAR,        1U,  0U, 0U, NULL,       0U},
    /* A661_CURSOR_ENTRY_EVENTS_EXTENSION: Table 8.7.3-2, CursorEntryEventsExtension Creation Structure */
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL,       0U},
    /* A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION: Table 8.7.4-2, MapHorzCoordSystemEventExtension Creation Structure */
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values37, 2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL,       0U},
    /* A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION: Table 8.7.5-2, MapVertCoordSystemEventExtension Creation Structure */
    {("NumEvents"),                     BINARY_FIELD_USHORT,       2U,  0U, 0U, NULL,       0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,        1U,  0U, 0U, c_values37, 2U},
    {(NULL),                            BINARY_FIELD_PAD,          3U,  0U, 0U, NULL,       0U},
    {("EventIdArray"),                  BINARY_FIELD_USHORT,       2U,  0U, 1U, NULL,       0U}
};

/*************************************************************************************************/
/**
 * rief Supported widget types in name order, with WidgetType codes from Table 4.6-7.
 */
static const BinaryWidget_t c_binaryWidgets[] =
{
    {("A661_ACTIVE_AREA"),                    0xA010U,    0U, 10U},
    {("A661_ANIMATION_GROUP"),                0xA1E3U,   10U,  4U},
    {("A661_ANIMATION_ONPARAM"),              0xA1E4U,   14U, 12U},
    {("A661_ANIMATION_ROTATION"),             0xA1E5U,   26U, 15U},
    {("A661_ANIMATION_SCALE"),                0xA1E6U,   41U, 12U},
    {("A661_ANIMATION_TRANSLATION"),          0xA1E7U,   53U, 12U},
    {("A661_BASIC_CONTAINER"),                0xA020U,   65U,  4U},
    {("A661_BLINKING_CONTAINER"),             0xA030U,   69U,  2U},
    {("A661_BROADCAST_RECEIVER"),             0xA032U,   71U,  3U},
    {("A661_BUFFER_FORMAT"),                  0xA040U,   74U,  2U},
    {("A661_CHECK_BUTTON"),                   0xA050U,   76U, 15U},
    {("A661_COMBO_BOX"),                      0xA070U,   91U, 20U},
    {("A661_COMBO_BOX_EDIT"),                 0xA0E0U,  111U, 21U},
    {("A661_CONNECTOR"),                      0xA080U,  132U,  4U},
    {("A661_CURSOR_OVER"),                    0xA480U,  136U, 10U},
    {("A661_CURSOR_POS_OVERLAY"),             0xA090U,  146U,  6U},
    {("A661_CURSOR_REF"),                     0xA470U,  152U,  3U},
    {("A661_DATA_CONNECTOR"),                 0xA088U,  155U,  5U},
    {("A661_DATA_SCALING_FR180"),             0xA2E0U,  160U,  8U},
    {("A661_DATA_SCALING_LONG"),              0xA2D8U,  168U,  8U},
    {("A661_DATA_SCALING_ULONG"),             0xA2D9U,  176U,  8U},
    {("A661_EDIT_BOX_MASKED"),                0xA0A0U,  184U, 16U},
    {("A661_EDIT_BOX_MULTI_LINE"),            0xA0B0U,  200U, 15U},
    {("A661_EDIT_BOX_NUMERIC"),               0xA0C0U,  215U, 27U},
    {("A661_EDIT_BOX_NUMERIC_BCD"),           0xA0C2U,  242U, 34U},
    {("A661_EDIT_BOX_TEXT"),                  0xA0D0U,  276U, 15U},
    {("A661_EVENT_HANDLER"),                  0xA600U,  291U,  9U},
    {("A661_EXTERNAL_SOURCE"),                0xA188U,  300U, 14U},
    {("A661_FOCUS_IN"),                       0xA498U,  314U,  1U},
    {("A661_FOCUS_LINK"),                     0xA490U,  315U,  4U},
    {("A661_FOCUS_OUT"),                      0xA499U,  319U,  4U},
    {("A661_FRAMING_REFRESH_CONTAINER"),      0xA380U,  323U,  4U},
    {("A661_GESTURE_AREA"),                   0xA501U,  327U, 13U},
    {("A661_GP_ARC_CIRCLE"),                  0xA0F0U,  340U, 13U},
    {("A661_GP_ARC_ELLIPSE"),                 0xA100U,  353U, 14U},
    {("A661_GP_CROWN"),                       0xA110U,  367U, 14U},
    {("A661_GP_LINE"),                        0xA120U,  381U,  9U},
    {("A661_GP_LINE_POLAR"),                  0xA130U,  390U,  9U},
    {("A661_GP_POLYLINE"),                    0xA138U,  399U, 10U},
    {("A661_GP_RECTANGLE"),                   0xA140U,  409U, 12U},
    {("A661_GP_TRIANGLE"),                    0xA150U,  421U, 14U},
    {("A661_GP_TRIANGLE_FAN"),                0xA151U,  435U, 11U},
    {("A661_GP_TRIANGLE_STRIP"),              0xA152U,  446U, 11U},
    {("A661_INK_AREA"),                       0xA502U,  457U, 14U},
    {("A661_KEYBOARD_AREA"),                  0xA1E1U,  471U,  9U},
    {("A661_LABEL"),                          0xA160U,  480U, 14U},
    {("A661_LABEL_COMPLEX"),                  0xA170U,  494U, 12U},
    {("A661_MAPHORZ"),                        0xA1B0U,  506U, 12U},
    {("A661_MAPHORZ_CONTAINER"),              0xA1B8U,  518U,  2U},
    {("A661_MAPHORZ_ITEMLIST"),               0xA180U,  520U,  5U},
    {("A661_MAPHORZ_PANEL"),                  0xA189U,  525U,  2U},
    {("A661_MAPHORZ_SOURCE"),                 0xA1A0U,  527U,  5U},
    {("A661_MAPHORZ_VERTEXBUFFER"),           0xA1A9U,  532U,  5U},
    {("A661_MAPVERT"),                        0xA1B2U,  537U, 12U},
    {("A661_MAPVERT_CONTAINER"),              0xA1B5U,  549U,  2U},
    {("A661_MAPVERT_ITEMLIST"),               0xA1B3U,  551U,  5U},
    {("A661_MAPVERT_PANEL"),                  0xA1B6U,  556U,  2U},
    {("A661_MAPVERT_SOURCE"),                 0xA1B4U,  558U,  6U},
    {("A661_MAP_BOUNDARY"),                   0xA1BAU,  564U,  7U},
    {("A661_MAP_EXTERNAL_SOURCE"),            0xA224U,  571U,  8U},
    {("A661_MAP_GRID"),                       0xA178U,  579U,  8U},
    {("A661_MASK_CONTAINER"),                 0xA1C0U,  587U,  7U},
    {("A661_MENU_BAR"),                       0xA1D0U,  594U,  8U},
    {("A661_MULTI_STATE_BUTTON"),             0xA348U,  602U, 16U},
    {("A661_MUTUALLY_EXCLUSIVE_CONTAINER"),   0xA400U,  618U,  6U},
    {("A661_NO_SERVICE_MONITOR"),             0xA4F0U,  624U,  2U},
    {("A661_NUMERIC_READOUT"),                0xA406U,  626U, 18U},
    {("A661_PAGING_CONTAINER"),               0xA408U,  644U, 14U},
    {("A661_PANEL"),                          0xA1F0U,  658U,  9U},
    {("A661_PICTURE"),                        0xA200U,  667U,  8U},
    {("A661_PICTURE_ANIMATED"),               0xA210U,  675U, 13U},
    {("A661_PICTURE_PUSH_BUTTON"),            0xA240U,  688U, 15U},
    {("A661_PICTURE_TOGGLE_BUTTON"),          0xA250U,  703U, 19U},
    {("A661_POP_UP_MENU"),                    0xA270U,  722U, 12U},
    {("A661_POP_UP_MENU_BUTTON"),             0xA280U,  734U, 27U},
    {("A661_POP_UP_PANEL"),                   0xA290U,  761U,  8U},
    {("A661_POP_UP_PANEL_BUTTON"),            0xA4E0U,  769U, 19U},
    {("A661_PROXY_BUTTON"),                   0xA420U,  788U,  4U},
    {("A661_PUSH_BUTTON"),                    0xA2A0U,  792U, 12U},
    {("A661_RADIO_BOX"),                      0xA2B0U,  804U,  2U},
    {("A661_ROTATION_CONTAINER"),             0xA2D0U,  806U,  5U},
    {("A661_SCALE_CONTAINER"),                0xA2F2U,  811U,  6U},
    {("A661_SCROLL_LIST"),                    0xA2F0U,  817U, 21U},
    {("A661_SCROLL_PANEL"),                   0xA300U,  838U, 26U},
    {("A661_SCROLL_WHEEL_AREA"),              0xA1E2U,  864U,  6U},
    {("A661_SELECTION_LIST_BUTTON"),          0xA370U,  870U, 22U},
    {("A661_SHUFFLE_TO_FIT_CONTAINER"),       0xA4B0U,  892U, 12U},
    {("A661_SIZE_TO_FIT_CONTAINER"),          0xA4A0U,  904U, 10U},
    {("A661_SLIDER"),                         0xA440U,  914U, 19U},
    {("A661_SYMBOL"),                         0xA310U,  933U,  9U},
    {("A661_SYMBOL_ANIMATED"),                0xA450U,  942U, 15U},
    {("A661_SYMBOL_PUSH_BUTTON"),             0xA4C0U,  957U, 15U},
    {("A661_SYMBOL_TOGGLE_BUTTON"),           0xA4D0U,  972U, 19U},
    {("A661_TABBED_PANEL"),                   0xA320U,  991U, 12U},
    {("A661_TABBED_PANEL_GROUP"),             0xA330U, 1003U, 11U},
    {("A661_TOGGLE_BUTTON"),                  0xA340U, 1014U, 16U},
    {("A661_TOUCH_AREA"),                     0xA500U, 1030U, 12U},
    {("A661_TRANSLATION_CONTAINER"),          0xA360U, 1042U,  4U},
    {("A661_TREE"),                           0xA222U, 1046U, 23U},
    {("A661_WATCHDOG_CONTAINER"),             0xA460U, 1069U,  9U}
};

/*************************************************************************************************/
/**
 * rief Supported widget extension types, with ExtensionType codes from Table 4.6-12.
 */
static const BinaryWidget_t c_binaryExtensions[] =
{
    {("A661_DIRECTIONAL_TABBING_EXTENSION"),        0x8003U, 1078U,  3U},
    {("A661_CURSOR_EVENTS_EXTENSION"),              0x8002U, 1081U,  2U},
    {("A661_LEGEND_ALIGN_EXTENSION"),               0x8004U, 1083U,  2U},
    {("A661_CHILD_INDEX_EXTENSION"),                0x8001U, 1085U,  1U},
    {("A661_INITIAL_FOCUS_EXTENSION"),              0x8006U, 1086U,  2U},
    {("A661_FOCUS_STOP_EXTENSION"),                 0x8007U, 1088U,  2U},
    {("A661_CURSOR_SHAPE_EXTENSION"),               0x8008U, 1090U,  2U},
    {("A661_PICTURE_EXTENSION"),                    0x8009U, 1092U,  5U},
    {("A661_SYMBOL_EXTENSION"),                     0x800AU, 1097U,  5U},
    {("A661_TICS_ARRAY_EXTENSION"),                 0x800BU, 1102U,  7U},
    {("A661_STYLE_SET_EXTENSION"),                  0x800CU, 1109U,  3U},
    {("A661_STATIC_PARAM_BUFFER_EXTENSION"),        0x800DU, 1112U,  2U},
    {("A661_MULTI_SELECTION_EXTENSION"),            0x800FU, 1114U,  1U},
    {("A661_EXCLUDED_REGIONS_EXTENSION"),           0x800EU, 1115U,  5U},
    {("A661_BOUNDARY_CHECK_EXTENSION"),             0x8010U, 1120U,  2U},
    {("A661_REORDER_EXTENSION"),                    0x8011U, 1122U,  1U},
    {("A661_WORD_WRAP_EXTENSION"),                  0x8012U, 1123U,  2U},
    {("A661_CURSOR_ENTRY_EVENTS_EXTENSION"),        0x8013U, 1125U,  2U},
    {("A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION"), 0x8014U, 1127U,  4U},
    {("A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION"), 0x8015U, 1131U,  4U}
};

/*************************************************************************************************/
/**
 * rief Constant names accepted as property values, from Tables 4.6-8, 4.6-10, 4.6-11, 4.6-13, 4.6-14, and 4.6-15.
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

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

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
    if (0x80000000U <= value)
    {
        result -= ((int64_t)1 << 32);
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
        PutUnsigned(buffer, capacity, (*offset), 0, 1);
        (*offset) += 1;
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

    is_valid = (strlen(text) == (byteCount * 2U));
    for (byte_index = 0; is_valid && (byte_index < byteCount); ++byte_index)
    {
        high_digit = strchr(c_binaryHexDigits, text[byte_index * 2U]);
        low_digit  = strchr(c_binaryHexDigits, text[(byte_index * 2U) + 1U]);
        is_valid   = (NULL != high_digit) && (NULL != low_digit);
        if (is_valid)
        {
            PutUnsigned(buffer, capacity, offset + byte_index, (uint32_t)(((high_digit - c_binaryHexDigits) << 4) | (low_digit - c_binaryHexDigits)), 1);
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
    if (0U == regionType)
    {
        byte_count = BINARY_REGION_NOT_USED_BYTES;
    }
    else if (1U == regionType)
    {
        byte_count = BINARY_REGION_RECTANGLE_BYTES;
    }
    else if (2U == regionType)
    {
        byte_count = BINARY_REGION_TRIANGLE_BYTES;
    }
    else if (3U == regionType)
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
                PutUnsigned(buffer, capacity, (*offset) + byte_index, 0, 1);
            }
        }
        (*offset) += field->byteCount;
    }
    else if (BINARY_FIELD_SIZED_BYTES == field->type)
    {
        // Write ParamBufferSize, the unused pad, the bytes, then 32-bit alignment (Table 8.6.5-3)
        byte_count = strlen(text) / 2U;
        is_valid   = (UINT16_MAX >= byte_count);
        if (is_valid)
        {
            PutUnsigned(buffer, capacity, (*offset), (uint32_t)byte_count, 2);
            PutUnsigned(buffer, capacity, (*offset) + 2U, 0, 2);
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset) + BINARY_WORD_BYTES);
        }
        (*offset) += (BINARY_WORD_BYTES + byte_count);
        PutAlignment(buffer, capacity, offset);
    }
    else if (BINARY_FIELD_REGION == field->type)
    {
        // Read the structure size from its second byte, requiring both digits of that byte to be valid
        byte_count = 0;
        is_valid   = ((BINARY_REGION_HEADER_BYTES * 2U) <= strlen(text));
        if (is_valid)
        {
            type_digit = strchr(c_binaryHexDigits, text[3]);
            is_valid   = ('0' == text[2]) && (NULL != type_digit);
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
        byte_count = strlen(text) / 2U;
        is_valid   = (BINARY_REQUEST_HEADER_BYTES <= byte_count);
        if (is_valid)
        {
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset));
        }
        if (is_valid)
        {
            request_size = 0;
            for (byte_index = 4; byte_index < 8; ++byte_index)
            {
                request_size = (request_size << 4) | (uint32_t)(strchr(c_binaryHexDigits, text[byte_index]) - c_binaryHexDigits);
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
            PutUnsigned(buffer, capacity, (*offset) + byte_index, (byte_t)text[byte_index], 1);
        }
        // Keep each string's terminator; pad the command after its last parameter
        PutUnsigned(buffer, capacity, (*offset) + byte_count, 0, 1);
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
                range = 32768.0;
                if (BINARY_FIELD_FR180 == field->type)
                {
                    range = 180.0;
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
            field = &c_binaryFields[type->firstField + field_index];
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
        PutUnsigned(buffer, capacity, (*offset), command, 2);
        PutUnsigned(buffer, capacity, (*offset) + 2, (uint32_t)(cursor - (*offset)), 2);
        PutUnsigned(buffer, capacity, (*offset) + 4, type->code, 2);
        if (FALSE == widget->isExtension)
        {
            PutUnsigned(buffer, capacity, (*offset) + 6, (uint32_t)widget_ident, 2);
            PutUnsigned(buffer, capacity, (*offset) + 8, (uint32_t)parent_ident, 2);
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
    if (0.0 <= scaled)
    {
        scaled += 0.5;
    }
    else
    {
        scaled -= 0.5;
    }
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

    is_valid = ((byteCount * 2U) < sizeof(text));
    if (is_valid)
    {
        for (byte_index = 0; byte_index < byteCount; ++byte_index)
        {
            text[byte_index * 2U]        = c_binaryHexDigits[bytes[byte_index] >> 4];
            text[(byte_index * 2U) + 1U] = c_binaryHexDigits[bytes[byte_index] & 0x0FU];
        }
        is_valid = AddProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, name, text, byteCount * 2U);
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
        (*offset) = ((*offset) + (BINARY_WORD_BYTES - 1U)) & ~(uint64_t)(BINARY_WORD_BYTES - 1U);
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
            byte_count = GetUnsigned(buffer, (*offset), 2);
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
            byte_count = RegionByteCount(buffer[(*offset) + 1U]);
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
            byte_count = BINARY_REQUEST_HEADER_BYTES + GetUnsigned(buffer, (*offset) + 2U, 2);
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
                range = 32768.0;
                if (BINARY_FIELD_FR180 == field->type)
                {
                    range = 180.0;
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

    command      = GetUnsigned(buffer, commandOffset, 2);
    is_extension = (BINARY_CMD_EXTEND == command);
    end_offset   = commandOffset + commandSize;
    cursor       = end_offset;
    type         = NULL;
    (void)memset(values, 0, sizeof(values));
    is_valid = is_extension || (BINARY_CMD_CREATE == command);
    if (is_valid)
    {
        type     = FindTypeByCode(GetUnsigned(buffer, commandOffset + 4, 2), is_extension);
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
            parent_ident = GetUnsigned(buffer, commandOffset + 8, 2);
            if (0 != parent_ident)
            {
                cursor = firstCommandOffset;
                for (widget_index = firstWidgetIndex; widget_index < definition->widgetCount; ++widget_index)
                {
                    if ((BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, 2)) && (parent_ident == GetUnsigned(buffer, cursor + 6, 2)))
                    {
                        widget->parentWidgetIndex = widget_index;
                        break;
                    }
                    cursor += GetUnsigned(buffer, cursor + 2, 2);
                }
                cursor   = commandOffset + BINARY_COMMAND_BYTES;
                is_valid = (UINT32_MAX != widget->parentWidgetIndex);
            }
            if (is_valid)
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, BINARY_PROPERTY_WIDGET_IDENT, GetUnsigned(buffer, commandOffset + 6, 2));
            }
        }
    }
    if (is_valid)
    {
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            field         = &c_binaryFields[type->firstField + field_index];
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
        block_size = GetUnsigned(buffer, (*offset) + 4, 4);
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
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_LAYER_ID, buffer[(*offset) + 1]);
    }
    if (is_valid)
    {
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_CONTEXT_NUMBER, GetUnsigned(buffer, (*offset) + 2, 2));
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
            is_valid = ((BINARY_EXTEND_BYTES + 2U) <= (end_offset - cursor));
            if (is_valid)
            {
                command_size = GetUnsigned(buffer, cursor + 2, 2);
                is_valid     = ((BINARY_EXTEND_BYTES + 2U) <= command_size) && (0 == (command_size % BINARY_WORD_BYTES)) && (command_size <= (end_offset - cursor));
            }
            if (is_valid && (BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, 2)))
            {
                is_valid = ((BINARY_COMMAND_BYTES + 2U) <= command_size);
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
        pixel_format  = buffer[offset + 1U];
        picture_count = buffer[offset + 2U];
        color_count   = buffer[offset + 3U];
        color_format  = buffer[offset + 4U];
        is_valid      = (BINARY_PIX_FMT_RGBA_8 <= pixel_format) && (BINARY_PIX_FMT_COLOR_INDEXED_8 >= pixel_format);
        if (is_valid && (0 != color_count))
        {
            is_valid = (BINARY_PIX_FMT_RGBA_8 <= color_format) && (BINARY_PIX_FMT_COLOR_INDEXED_8 >= color_format);
            if (is_valid)
            {
                // Measure color table entries in their own format and pad them as a whole
                cursor += ((uint64_t)color_count * (BINARY_WORD_BYTES >> (color_format - BINARY_PIX_FMT_RGBA_8)) + (BINARY_WORD_BYTES - 1U)) & ~(uint64_t)(BINARY_WORD_BYTES - 1U);
            }
        }
    }
    for (picture_index = 0; is_valid && (picture_index < picture_count); ++picture_index)
    {
        is_valid = (cursor <= byteCount) && (BINARY_PICTURE_BYTES <= (byteCount - cursor)) && (BINARY_BEGIN_PICTURE == buffer[cursor]);
        if (is_valid)
        {
            pixel_bytes = (uint64_t)GetUnsigned(buffer, cursor + 4U, 2) * GetUnsigned(buffer, cursor + 6U, 2) * (BINARY_WORD_BYTES >> (pixel_format - BINARY_PIX_FMT_RGBA_8));
            cursor     += BINARY_PICTURE_BYTES + ((pixel_bytes + (BINARY_WORD_BYTES - 1U)) & ~(uint64_t)(BINARY_WORD_BYTES - 1U));
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
 *  storage, so Arinc661_WriteDefinitionBinary can emit it again. The storage
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
            block_size = GetUnsigned(buffer, (*offset) + 4U, 4);
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
bool_t Arinc661_ReadDefinitionBinary(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount)
{
    uint64_t offset;
    uint64_t block_index;
    uint32_t block_code;
    bool_t   has_footer;
    bool_t   is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    LOGASSERT((NULL != definition->blocks) || (0 == definition->blockCapacity));
    // Reset result counts while retaining the supplied arrays and capacities
    definition->layerCount      = 0;
    definition->widgetCount     = 0;
    definition->propertyCount   = 0;
    definition->blockByteCount  = 0;
    definition->charsetEncoding = 0;
    has_footer                  = FALSE;
    offset                      = BINARY_HEADER_BYTES;
    is_valid                    = (BINARY_HEADER_BYTES <= byteCount);
    if (is_valid)
    {
        is_valid = (BINARY_DF_MAGIC_NUMBER == GetUnsigned(buffer, 0, 2));
    }
    if (is_valid)
    {
        definition->name[0]           = '\0';
        definition->libraryVersion    = buffer[2];
        definition->supplementVersion = buffer[3];
        is_valid                      = AddIntegerProperty(definition, ARINC661_PROPERTY_DEFINITION, 0, BINARY_PROPERTY_APPLICATION_ID, GetUnsigned(buffer, 4, 2));
        // Skip the implementation-dependent OEM data, which occupies whole words
        offset += ((uint64_t)GetUnsigned(buffer, 6, 2) + (BINARY_WORD_BYTES - 1U)) & ~(uint64_t)(BINARY_WORD_BYTES - 1U);
    }
    if (is_valid)
    {
        // Bound the walk by the one-word minimum block size
        for (block_index = 0; block_index < byteCount; ++block_index)
        {
            is_valid = (offset <= byteCount) && (BINARY_BLOCK_END_BYTES <= (byteCount - offset));
            if (FALSE == is_valid)
            {
                break;
            }
            block_code = buffer[offset];
            if (BINARY_DF_FOOTER == block_code)
            {
                has_footer = TRUE;
                break;
            }
            if (BINARY_BEGIN_LAYER_BLOCK == block_code)
            {
                is_valid = ReadLayer(definition, buffer, byteCount, &offset);
            }
            else if (BINARY_BEGIN_CHARSET_BLOCK == block_code)
            {
                // Read the block as code, pad, encoding, end code, pad (Table 4.5.3.2-6) and allow one per file
                is_valid = (BINARY_CHARSET_BLOCK_BYTES <= (byteCount - offset)) && (BINARY_END_CHARSET_BLOCK == buffer[offset + 4U]) && (0 == definition->charsetEncoding);
                if (is_valid)
                {
                    definition->charsetEncoding = GetUnsigned(buffer, offset + 2U, 2);
                    is_valid                    = (0 != definition->charsetEncoding);
                    offset                     += BINARY_CHARSET_BLOCK_BYTES;
                }
            }
            else if ((BINARY_BEGIN_SYMBOL_BLOCK == block_code) || (BINARY_BEGIN_PICTURE_BLOCK == block_code) || (BINARY_BEGIN_ANIMATION_BLOCK == block_code))
            {
                is_valid = ReadOpaqueBlock(definition, buffer, byteCount, &offset);
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
        definition->layerCount      = 0;
        definition->widgetCount     = 0;
        definition->propertyCount   = 0;
        definition->blockByteCount  = 0;
        definition->charsetEncoding = 0;
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
 *  file is sized even when it exceeds capacity, in which case nothing past
 *  capacity is stored and zero is returned.
 */
uint64_t Arinc661_WriteDefinitionBinary(const Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity)
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
    LOGASSERT(NULL != buffer);
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
        PutUnsigned(buffer, capacity, 0, BINARY_DF_MAGIC_NUMBER, 2);
        PutUnsigned(buffer, capacity, 2, definition->libraryVersion, 1);
        PutUnsigned(buffer, capacity, 3, definition->supplementVersion, 1);
        PutUnsigned(buffer, capacity, 4, (uint32_t)application_id, 2);
        // Write no OEM free data
        PutUnsigned(buffer, capacity, 6, 0, 2);
        if (0 != definition->charsetEncoding)
        {
            PutUnsigned(buffer, capacity, offset, (BINARY_BEGIN_CHARSET_BLOCK << 24) | definition->charsetEncoding, 4);
            PutUnsigned(buffer, capacity, offset + 4U, BINARY_END_CHARSET_BLOCK << 24, 4);
            offset += BINARY_CHARSET_BLOCK_BYTES;
        }
        // Write symbol, picture, and animation law blocks before the layers (Table 4.5.3.2-1)
        for (byte_index = 0; byte_index < definition->blockByteCount; ++byte_index)
        {
            PutUnsigned(buffer, capacity, offset + byte_index, definition->blocks[byte_index], 1);
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
            PutUnsigned(buffer, capacity, layer_offset, BINARY_BEGIN_LAYER_BLOCK, 1);
            PutUnsigned(buffer, capacity, layer_offset + 1, (uint32_t)layer_id, 1);
            PutUnsigned(buffer, capacity, layer_offset + 2, (uint32_t)context_number, 2);
            offset += BINARY_LAYER_HEADER_BYTES;
            for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
            {
                // Select entries belonging to this layer while preserving their array order
                if (layer_index == definition->widgets[widget_index].layerIndex)
                {
                    is_valid = WriteCommand(definition, widget_index, last_widget_index, buffer, capacity, &offset);
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
                PutUnsigned(buffer, capacity, offset, (uint32_t)BINARY_END_LAYER_BLOCK << 24, BINARY_BLOCK_END_BYTES);
                offset   += BINARY_BLOCK_END_BYTES;
                // Count the block header and the end code in Block Size
                is_valid  = ((offset - layer_offset) <= UINT32_MAX);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            PutUnsigned(buffer, capacity, layer_offset + 4, (uint32_t)(offset - layer_offset), 4);
        }
    }
    if (is_valid)
    {
        PutUnsigned(buffer, capacity, offset, (uint32_t)BINARY_DF_FOOTER << 24, BINARY_BLOCK_END_BYTES);
        offset   += BINARY_BLOCK_END_BYTES;
        is_valid  = (offset <= capacity);
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
uint32_t Arinc661_TypeCount(bool_t isExtension)
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
const char_t* Arinc661_TypeName(bool_t isExtension, uint32_t typeIndex)
{
    const char_t* name;

    LOGASSERT(typeIndex < Arinc661_TypeCount(isExtension));
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
bool_t Arinc661_FindType(const char_t* typeName, bool_t isExtension, uint32_t* parameterCount)
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
            if (NULL != c_binaryFields[type->firstField + field_index].name)
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
bool_t Arinc661_Parameter(const char_t* typeName, bool_t isExtension, uint32_t parameterIndex, Arinc661_Parameter_t* parameter)
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
            field = &c_binaryFields[type->firstField + field_index];
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
                    parameter->countName = c_binaryFields[type->firstField + field->countField].name;
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
uint32_t Arinc661_ConstantCount(void)
{
    return (uint32_t)COUNT_OF(c_binaryConstants);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Constants are in name order.
 */
const char_t* Arinc661_ConstantName(uint32_t constantIndex)
{
    LOGASSERT(constantIndex < Arinc661_ConstantCount());

    return c_binaryConstants[constantIndex].name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the numeric value the binary form stores for the constant.
 */
uint32_t Arinc661_ConstantValue(uint32_t constantIndex)
{
    LOGASSERT(constantIndex < Arinc661_ConstantCount());

    return c_binaryConstants[constantIndex].value;
}
