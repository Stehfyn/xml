/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1, Supplement 7, Cockpit Display System Interfaces to User Systems:
 * section 4.5.3.2 (Definition File structure), section 4.5.3.3 (layer block commands),
 * section 4.6 (constants), and the widget Creation Structure tables in section 3.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"
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
#define BINARY_DF_MAGIC_NUMBER   (0xA661U) ///< A661_DF_MAGIC_NUMBER
#define BINARY_BEGIN_LAYER_BLOCK (0xA0U)   ///< A661_BEGIN_LAYER_BLOCK
#define BINARY_END_LAYER_BLOCK   (0xC0U)   ///< A661_END_LAYER_BLOCK
#define BINARY_DF_FOOTER         (0xE0U)   ///< A661_DF_FOOTER
#define BINARY_CMD_CREATE        (0xCA01U) ///< A661_CMD_CREATE

/* ARINC 661 Part 1-7, Tables 4.5.3.2-2, 4.5.3.2-4, and 4.5.3.3.1. */
#define BINARY_HEADER_BYTES       (8U)  ///< Magic number, versions, AppliIdent, and OEM data size
#define BINARY_LAYER_HEADER_BYTES (8U)  ///< Block code, LayerIdent, ContextNumber, and Block Size
#define BINARY_BLOCK_END_BYTES    (4U)  ///< End or footer code followed by its unused pad
#define BINARY_COMMAND_BYTES      (10U) ///< Command code, CommandSize, WidgetType, WidgetIdent, and ParentIdent
#define BINARY_WORD_BYTES         (4U)  ///< Blocks and commands are aligned to 32 bits

/* Property names shared with the XML Definition File form (section 6.5.1.2). */
#define BINARY_PROPERTY_APPLICATION_ID ("ApplicationId")
#define BINARY_PROPERTY_LAYER_ID       ("LayerId")
#define BINARY_PROPERTY_CONTEXT_NUMBER ("ContextNumber")
#define BINARY_PROPERTY_WIDGET_IDENT   ("WidgetIdent")

#define BINARY_INTEGER_TEXT_BYTES (32U)           ///< Integer formatting buffer bytes, including the zero terminator
#define BINARY_FR_FULL_SCALE      (2147483648.0)  ///< Two raised to the number of fr() bits minus one
#define BINARY_FR_PRECISION       (10)            ///< Fractional digits that resolve one 32-bit fr(180) LSB
#define BINARY_FLOAT_PRECISION    (54)            ///< Fractional digits that retain very small float32_t values

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Encoding of one Creation Structure parameter (Part 1-7, section 3.3 data types).
 */
typedef enum BinaryFieldType_e
{
    BINARY_FIELD_PAD,     ///< UnusedPad bytes, written as zero
    BINARY_FIELD_UCHAR,   ///< Unsigned 8-bit integer
    BINARY_FIELD_USHORT,  ///< Unsigned 16-bit integer
    BINARY_FIELD_LONG,    ///< Signed 32-bit integer
    BINARY_FIELD_ULONG,   ///< Unsigned 32-bit integer
    BINARY_FIELD_FLOAT,   ///< IEEE 754 single precision
    BINARY_FIELD_FR180,   ///< Signed 32-bit fraction of 180
    BINARY_FIELD_FR32768, ///< Signed 32-bit fraction of 32768
    BINARY_FIELD_STRING,  ///< Zero-terminated bytes
    BINARY_FIELD_BYTES,   ///< Fixed number of bytes with no numeric type, given as hexadecimal digits
    BINARY_FIELD_ALIGN    ///< Zero bytes up to the next 32-bit boundary
} BinaryFieldType_t;

/*************************************************************************************************/
/**
 * \brief One parameter of a widget Creation Structure, after ParentIdent.
 */
typedef struct BinaryField_s
{
    const char_t*     name;      ///< Property name, or NULL for an unused pad or alignment
    BinaryFieldType_t type;      ///< Binary encoding
    uint32_t          byteCount; ///< Encoded bytes of one value; zero for a string or alignment
    bool_t            isCount;   ///< TRUE when the value is the entry count of the arrays that follow
    uint32_t          repeat;    ///< Array elements per counted entry; zero for a single value
} BinaryField_t;

/*************************************************************************************************/
/**
 * \brief Widget type with its Creation Structure parameters.
 */
typedef struct BinaryWidget_s
{
    const char_t* name;       ///< Widget type name, for example A661_PANEL
    uint16_t      code;       ///< WidgetType value in Table 4.6-7
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
 * \brief Digits of a BINARY_FIELD_BYTES property value; one array, so digit pointers can be subtracted.
 */
static const char_t c_binaryHexDigits[] = "0123456789ABCDEF";

/*************************************************************************************************/
/**
 * \brief Creation Structure parameters after ParentIdent, grouped by widget type.
 *
 * \par Details:
 *  Transcribed from the Part 1-7 Creation Structure tables named in each group.
 *  An array holds repeat elements for each entry counted by the preceding
 *  isCount parameter of the same widget; arrays of pairs, such as Vertices and
 *  BufferStructure, alternate the two members. Arrays are padded to 32 bits
 *  only where the table says so, which is recorded as an alignment entry.
 *  A661_SELECTOR is not listed because Table 4.6-7 gives it no WidgetType code.
 */
static const BinaryField_t c_binaryFields[] =
{
    /* A661_ACTIVE_AREA: Table 3.3.1-2, ActiveArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    /* A661_ANIMATION_GROUP: Table 3.9.12-2, AnimationGroup Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Type"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_ANIMATION_ONPARAM: Table 3.9.8-2, AnimationOnParam Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TargetParamID"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("FromValue"),                     BINARY_FIELD_BYTES,    4U, FALSE, 0U},
    {("ToValue"),                       BINARY_FIELD_BYTES,    4U, FALSE, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_ANIMATION_ROTATION: Table 3.9.9-2, AnimationRotation Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("CenterX"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("CenterY"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("FromAngle"),                     BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("ToAngle"),                       BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Direction"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NumberOfTurns"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_ANIMATION_SCALE: Table 3.9.10-2, AnimationScale Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("FromScaleX"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("FromScaleY"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("ToScaleX"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("ToScaleY"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_ANIMATION_TRANSLATION: Table 3.9.11-2, AnimationTranslation Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("FromValueX"),                    BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("FromValueY"),                    BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("ToValueX"),                      BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("ToValueY"),                      BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("AnimationLawRef"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Duration"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Delay"),                         BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AnimationRepetition"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_BASIC_CONTAINER: Table 3.3.2-2, BasicContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    /* A661_BLINKING_CONTAINER: Table 3.3.3-2, BlinkingContainer Creation Structure */
    {("BlinkingType"),                  BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_BROADCAST_RECEIVER: Table 3.8.9-2, BroadcastReceiver Creation Structure */
    {("NumberOfFields"),                BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("ParameterIdent"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("WidgetStructure"),               BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    /* A661_BUFFER_FORMAT: Table 3.3.4-2, BufferFormat Creation Structure */
    {("NumberOfFields"),                BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("BufferStructure"),               BINARY_FIELD_USHORT,   2U, FALSE, 2U},
    /* A661_CHECK_BUTTON: Table 3.3.5-2, CheckButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("CheckButtonState"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_COMBO_BOX: Table 3.3.6-2, ComboBox Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    {("EntryList"),                     BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_COMBO_BOX_EDIT: Table 3.4.7-2, ComboBoxEdit Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("EntryList"),                     BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_CONNECTOR: Table 3.3.7-2, Connector Creation Structure */
    {("ConnectorReference"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_CURSOR_OVER: Table 3.6.3-2, CursorOver Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("PositionReportMode"),            BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    /* A661_CURSOR_POS_OVERLAY: Table 3.3.8-2, CursorPosOverlay Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    /* A661_CURSOR_REF: Table 3.6.2-2, CursorRef Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    /* A661_DATA_CONNECTOR: Table 3.9.16-2, DataConnector Creation Structure */
    {("ConnectorReference"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("DataLength"),                    BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      4U, FALSE, 0U},
    /* A661_DATA_SCALING_FR180: Table 3.8.8-2, DataScalingFR180 Creation Structure */
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("StartScaledValue"),              BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("EndScaledValue"),                BINARY_FIELD_FR180,    4U, FALSE, 0U},
    /* A661_DATA_SCALING_LONG: Table 3.8.6-2, DataScalingLong Creation Structure */
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("StartScaledValue"),              BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("EndScaledValue"),                BINARY_FIELD_LONG,     4U, FALSE, 0U},
    /* A661_DATA_SCALING_ULONG: Table 3.8.7-2, DataScalingULong Creation Structure */
    {("TargetWidgetID"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TargetParameterID"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Clamping"),                      BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("StartValue"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("EndValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("StartScaledValue"),              BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("EndScaledValue"),                BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    /* A661_EDIT_BOX_MASKED: Table 3.3.9-2, EditBoxMasked Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AlphaMask"),                     BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("NumericMask"),                   BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_EDIT_BOX_MULTI_LINE: Table 3.4.6-2, EditBoxMultiLine Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_EDIT_BOX_NUMERIC: Table 3.3.10-2, EditBoxNumeric Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Value"),                         BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("TicsCoarse"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("TicsFine"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("MinValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("MaxValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StartCursorPosByte"),            BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LegendRemoved"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NumericKeyFlag"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("CyclicFlag"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("FormatString"),                  BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {("LegendString"),                  BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_EDIT_BOX_TEXT: Table 3.3.11-2, EditBoxText Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StartCursorPos"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ReportAllChanges"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_EXTERNAL_SOURCE: Table 3.4.2-2, ExternalSource Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SourceReference"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("SourceX"),                       BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SourceY"),                       BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SourceDX"),                      BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SourceDY"),                      BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_FRAMING_REFRESH_CONTAINER: Table 3.9.18-2, FramingRefreshContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("CachingEnabled"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    /* A661_GP_ARC_CIRCLE: Table 3.3.13-2, GpArcCircle Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("Radius"),                        BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_GP_ARC_ELLIPSE: Table 3.3.12-2, GpArcEllipse Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_GP_CROWN: Table 3.3.14-2, GpCrown Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("StartAngle"),                    BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("EndAngle"),                      BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("InnerRadius"),                   BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("OuterRadius"),                   BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_GP_LINE: Table 3.3.15-2, GpLine Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosXStart"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosYStart"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosXEnd"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosYEnd"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_GP_LINE_POLAR: Table 3.3.16-2, GpLinePolar Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosXStart"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosYStart"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("LineLength"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_GP_POLYLINE: Table 3.8.1-2, GpPolyline Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Closed"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("Vertices"),                      BINARY_FIELD_LONG,     4U, FALSE, 2U},
    /* A661_GP_RECTANGLE: Table 3.3.17-2, GpRectangle Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_GP_TRIANGLE: Table 3.3.18-2, GpTriangle Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosX2"),                         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY2"),                         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosX3"),                         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY3"),                         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_GP_TRIANGLE_FAN: Table 3.10.5-2, GpTriangleFan Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("Vertices"),                      BINARY_FIELD_LONG,     4U, FALSE, 2U},
    /* A661_GP_TRIANGLE_STRIP: Table 3.10.6-2, GpTriangleStrip Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Halo"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("Vertices"),                      BINARY_FIELD_LONG,     4U, FALSE, 2U},
    /* A661_INK_AREA: Table 3.9.7-2, InkArea Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Filled"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FillIndex"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("MaxBufferSize"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_LABEL: Table 3.3.20-2, Label Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Font"),                          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_LABEL_COMPLEX: Table 3.3.21-2, LabelComplex Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    {("DefaultStyleText"),              BINARY_FIELD_BYTES,   12U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_MAPHORZ: Table 3.3.25-2a, MapHorz Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("ScreenReferencePointX"),         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("ScreenReferencePointY"),         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("Range"),                         BINARY_FIELD_FR32768,  4U, FALSE, 0U},
    {("ScreenRange"),                   BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("ProjectionType"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    /* A661_MAPHORZ_CONTAINER: Table 3.8.4-2, MapHorz_Container Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_MAPHORZ_ITEMLIST: Table 3.3.22-2, MapHorz_ItemList Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxNumberOfItem"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxItemSize"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    /* A661_MAPHORZ_PANEL: Table 3.8.5-2, MapHorz_Panel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_MAPHORZ_SOURCE: Table 3.3.24-2a, MapHorz_Source Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MapDataFormat"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("EventFlag"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_MAPHORZ_VERTEXBUFFER: Table 3.9.4-2, MapHorz_VertexBuffer Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("MaxNumberOfVertices"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfVertices"),              BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("Vertices"),                      BINARY_FIELD_LONG,     4U, FALSE, 2U},
    /* A661_MAPVERT: Table 3.4.3-2a, MapVert Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("RangeX"),                        BINARY_FIELD_FR32768,  4U, FALSE, 0U},
    {("RangeY"),                        BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("RefPosX"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("RefPosY"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("RefGeoPosX"),                    BINARY_FIELD_FR32768,  4U, FALSE, 0U},
    {("RefGeoPosY"),                    BINARY_FIELD_LONG,     4U, FALSE, 0U},
    /* A661_MAPVERT_CONTAINER: Table 3.9.13-2, MapVert_Container Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_MAPVERT_ITEMLIST: Table 3.4.5-2a, MapVert_ItemList Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxNumberOfItem"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxItemSize"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    /* A661_MAPVERT_PANEL: Table 3.9.14-2, MapVert_Panel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_MAPVERT_SOURCE: Table 3.4.4-2a, MapVert_Source Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MapDataFormatX"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MapDataFormatY"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("EventFlag"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    /* A661_MAP_BOUNDARY: Table 3.9.15-2, MapBoundary Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("BeginAngleArray"),               BINARY_FIELD_FR180,    4U, FALSE, 1U},
    {("DistanceArray"),                 BINARY_FIELD_ULONG,    4U, FALSE, 1U},
    {("AlgorithmArray"),                BINARY_FIELD_ULONG,    4U, FALSE, 1U},
    /* A661_MAP_EXTERNAL_SOURCE: Table 3.10.4-2, MapExternalSource Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SourceReference"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_MAP_GRID: Table 3.4.1-2, MapGrid Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("OffsetX"),                       BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("OffsetY"),                       BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("IncrementX"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("IncrementY"),                    BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("CountX"),                        BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("CountY"),                        BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_MASK_CONTAINER: Table 3.3.26-2, MaskContainer Creation Structure */
    {("MaskEnabled"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("MaskReference"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    /* A661_MENU_BAR: Table 3.4.8-2, MenuBar Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Horizontal"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("ButtonPos"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("ButtonSize"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    /* A661_MULTI_STATE_BUTTON: Table 3.9.1-2, MultiStateButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ButtonState"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxNumberOfStates"),             BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NumberOfStates"),                BINARY_FIELD_UCHAR,    1U, TRUE,  0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_MUTUALLY_EXCLUSIVE_CONTAINER: Table 3.5.1-2, MutuallyExclusiveContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("VisibleChild"),                  BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_NO_SERVICE_MONITOR: Table 3.8.10-2, NoServiceMonitor Creation Structure */
    {("ShowNoServiceIdent"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      4U, FALSE, 0U},
    /* A661_NUMERIC_READOUT: Table 3.8.3-2, NumericReadout Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("Value"),                         BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("LegendAreaSizeX"),               BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("MaxFormatStringLength"),         BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxLegendStringLength"),         BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LegendPosition"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("FormatString"),                  BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {("LegendString"),                  BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_PAGING_CONTAINER: Table 3.8.2-2, PagingContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("WrappingType"),                  BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PagingControlPosition"),         BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FinePageDelta"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("CoarsePageDelta"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("VisibleChild"),                  BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ReportVisibleChild"),            BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_PANEL: Table 3.3.27-2, Panel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    /* A661_PICTURE: Table 3.3.19-2, Picture Creation Structure */
    {("Anonymous"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_PICTURE_ANIMATED: Table 3.5.5-2, PictureAnimated Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfPictures"),              BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("IndexOfFrequency"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LoopFlag"),                      BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AnimationFlag"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    /* A661_PICTURE_PUSH_BUTTON: Table 3.3.28-2, Picture PushButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_PICTURE_TOGGLE_BUTTON: Table 3.3.29-2, PictureToggleButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AlternatePictureReference"),     BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_POP_UP_MENU: Table 3.3.31-2, PopUpMenu Creation Structure */
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_UCHAR,    1U, TRUE,  0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PopUpIdentArray"),               BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 1U},
    {("StringArray"),                   BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_POP_UP_MENU_BUTTON: Table 3.3.32-2, PopUpMenuButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PopupPosX"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PopupPosY"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PopupSizeX"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("PopupSizeY"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLengthPopUp"),          BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_UCHAR,    1U, TRUE,  0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,    0U, FALSE, 0U},
    {("PopUpIdentArray"),               BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {("PictureArray"),                  BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 1U},
    {("StringArray"),                   BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_POP_UP_PANEL: Table 3.3.30-2, PopUpPanel Creation Structure */
    {("UAPositionFlag"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticClosure"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_POP_UP_PANEL_BUTTON: Table 3.7.3-2, PopUpPanelButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PopupPosX"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PopupPosY"),                     BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PopupSizeX"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("PopupSizeY"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("UAPositionFlag"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticClosure"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_PROXY_BUTTON: Table 3.5.2-2, ProxyButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("DedicatedKeyIdent"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TargetWidgetIdent"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    /* A661_PUSH_BUTTON: Table 3.3.33-2, PushButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_RADIO_BOX: Table 3.3.34-2, RadioBox Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_ROTATION_CONTAINER: Table 3.3.35-2, RotationContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("CenterX"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("CenterY"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,    4U, FALSE, 0U},
    /* A661_SCALE_CONTAINER: Table 3.10.1-2, ScaleContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("CenterX"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("CenterY"),                       BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("ScaleX"),                        BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("ScaleY"),                        BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    /* A661_SCROLL_LIST: Table 3.3.37-2, ScrollList Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("FirstAccessibleEntry"),          BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("FirstVisibleEntry"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FlagReportVisibleEntry"),        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("DefaultStyleText"),              BINARY_FIELD_BYTES,   12U, FALSE, 0U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 1U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_SCROLL_PANEL: Table 3.3.36-2, ScrollPanel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("LineDeltaX"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("LineDeltaY"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("PageDeltaX"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("PageDeltaY"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("HomeX"),                         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("HomeY"),                         BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("FrameX"),                        BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("FrameY"),                        BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeXsheet"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeYsheet"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("BoundX"),                        BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("BoundY"),                        BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeXbound"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeYbound"),                    BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("HorizontalScroll"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("VerticalScroll"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("FlagReportFramePos"),            BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    /* A661_SELECTION_LIST_BUTTON: Table 3.5.7-2, SelectionListButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SelectingAreaWidth"),            BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SelectingAreaHeight"),           BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("OpeningEntry"),                  BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("OpeningMode"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_ALIGN,    0U, FALSE, 0U},
    {("EntryList"),                     BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_SHUFFLE_TO_FIT_CONTAINER: Table 3.6.6-2, ShuffleToFitContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("NumberOfVisibleChildren"),       BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ShuffleToFitMode"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("ItemSpacing"),                   BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_SIZE_TO_FIT_CONTAINER: Table 3.6.5-2, SizeToFitContainer Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("NumberOfVisibleChildren"),       BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("SizeToFitMode"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("ItemSpacing"),                   BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    /* A661_SLIDER: Table 3.5.4-2, Slider Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MinValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("MaxValue"),                      BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("Value"),                         BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("MajorTickInterval"),             BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    {("ShowMajorLabels"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Orientation"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LsbMultiple"),                   BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MinorTickMultiple"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MajorTickReference"),            BINARY_FIELD_FLOAT,    4U, FALSE, 0U},
    /* A661_SYMBOL: Table 3.3.38-2, Symbol Creation Structure */
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("RotationAngle"),                 BINARY_FIELD_FR180,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      3U, FALSE, 0U},
    /* A661_SYMBOL_ANIMATED: Table 3.5.6-2, SymbolAnimated Creation Structure */
    {("MotionAllowed"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ColorIndex"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("LoopType"),                      BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NumberOfSymbols"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("IndexOfFrequency"),              BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AnimationType"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("SymbolArray"),                   BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {(NULL),                            BINARY_FIELD_ALIGN,    0U, FALSE, 0U},
    {("OrientationArray"),              BINARY_FIELD_FR180,    4U, FALSE, 1U},
    {("MovementXArray"),                BINARY_FIELD_LONG,     4U, FALSE, 1U},
    {("MovementYArray"),                BINARY_FIELD_LONG,     4U, FALSE, 1U},
    /* A661_SYMBOL_PUSH_BUTTON: Table 3.7.1-2, SymbolPushButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_SYMBOL_TOGGLE_BUTTON: Table 3.7.2-2, SymbolToggleButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("SymbolReference"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AlternateSymbolReference"),      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("SymbolPosition"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_TABBED_PANEL: Table 3.3.39-2, TabbedPanel Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PictureReference"),              BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("PicturePosition"),               BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("InsetSize"),                     BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_TABBED_PANEL_GROUP: Table 3.3.40-2, TabbedPanelGroup Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ActiveTabbedPanelID"),           BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("TabPosition"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticInsetSizeFlag"),        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    /* A661_TOGGLE_BUTTON: Table 3.3.41-2, ToggleButton Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ToggleState"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AlternateFlag"),                 BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("LabelString"),                   BINARY_FIELD_STRING,   0U, FALSE, 0U},
    {("AlternateLabelString"),          BINARY_FIELD_STRING,   0U, FALSE, 0U},
    /* A661_TOUCH_AREA: Table 3.9.5-2, TouchArea Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("ReportMode"),                    BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("AllowOutside"),                  BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("MaxTouchPoints"),                BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NumberOfTouchPoints"),           BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    /* A661_TRANSLATION_CONTAINER: Table 3.3.42-2, TranslationContainer Creation Structure */
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("TranslationX"),                  BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("TranslationY"),                  BINARY_FIELD_LONG,     4U, FALSE, 0U},
    /* A661_TREE: Table 3.10.3-2, Tree Creation Structure */
    {("Enable"),                        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Visible"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("PosX"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("PosY"),                          BINARY_FIELD_LONG,     4U, FALSE, 0U},
    {("SizeX"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("SizeY"),                         BINARY_FIELD_ULONG,    4U, FALSE, 0U},
    {("FirstVisibleEntry"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("FlagReportVisibleEntry"),        BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("StyleSet"),                      BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NextFocusedWidget"),             BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxNumberOfEntries"),            BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("NumberOfEntries"),               BINARY_FIELD_USHORT,   2U, TRUE,  0U},
    {("SelectedEntry"),                 BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("MaxStringLength"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("AutomaticFocusMotion"),          BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("Alignment"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {("NodeStyleArray"),                BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {("ParentArray"),                   BINARY_FIELD_USHORT,   2U, FALSE, 1U},
    {("EnableArray"),                   BINARY_FIELD_UCHAR,    1U, FALSE, 1U},
    {("OpenArray"),                     BINARY_FIELD_UCHAR,    1U, FALSE, 1U},
    {(NULL),                            BINARY_FIELD_ALIGN,    0U, FALSE, 0U},
    {("LabelStringArray"),              BINARY_FIELD_STRING,   0U, FALSE, 1U},
    /* A661_WATCHDOG_CONTAINER: Table 3.5.3-2, WatchdogContainer Creation Structure */
    {(NULL),                            BINARY_FIELD_PAD,      2U, FALSE, 0U},
    {("TimePeriod"),                    BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ValidCountLimit"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("FailCountLimit"),                BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("Refresh"),                       BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U},
    {("ShowIfFailIdent"),               BINARY_FIELD_USHORT,   2U, FALSE, 0U},
    {("ShowFail"),                      BINARY_FIELD_UCHAR,    1U, FALSE, 0U},
    {(NULL),                            BINARY_FIELD_PAD,      1U, FALSE, 0U}
};

/*************************************************************************************************/
/**
 * \brief Supported widget types in name order, with WidgetType codes from Table 4.6-7.
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
    {("A661_EDIT_BOX_TEXT"),                  0xA0D0U,  242U, 15U},
    {("A661_EXTERNAL_SOURCE"),                0xA188U,  257U, 14U},
    {("A661_FRAMING_REFRESH_CONTAINER"),      0xA380U,  271U,  4U},
    {("A661_GP_ARC_CIRCLE"),                  0xA0F0U,  275U, 13U},
    {("A661_GP_ARC_ELLIPSE"),                 0xA100U,  288U, 14U},
    {("A661_GP_CROWN"),                       0xA110U,  302U, 14U},
    {("A661_GP_LINE"),                        0xA120U,  316U,  9U},
    {("A661_GP_LINE_POLAR"),                  0xA130U,  325U,  9U},
    {("A661_GP_POLYLINE"),                    0xA138U,  334U, 10U},
    {("A661_GP_RECTANGLE"),                   0xA140U,  344U, 12U},
    {("A661_GP_TRIANGLE"),                    0xA150U,  356U, 14U},
    {("A661_GP_TRIANGLE_FAN"),                0xA151U,  370U, 11U},
    {("A661_GP_TRIANGLE_STRIP"),              0xA152U,  381U, 11U},
    {("A661_INK_AREA"),                       0xA502U,  392U, 14U},
    {("A661_LABEL"),                          0xA160U,  406U, 14U},
    {("A661_LABEL_COMPLEX"),                  0xA170U,  420U, 12U},
    {("A661_MAPHORZ"),                        0xA1B0U,  432U, 12U},
    {("A661_MAPHORZ_CONTAINER"),              0xA1B8U,  444U,  2U},
    {("A661_MAPHORZ_ITEMLIST"),               0xA180U,  446U,  5U},
    {("A661_MAPHORZ_PANEL"),                  0xA189U,  451U,  2U},
    {("A661_MAPHORZ_SOURCE"),                 0xA1A0U,  453U,  5U},
    {("A661_MAPHORZ_VERTEXBUFFER"),           0xA1A9U,  458U,  5U},
    {("A661_MAPVERT"),                        0xA1B2U,  463U, 12U},
    {("A661_MAPVERT_CONTAINER"),              0xA1B5U,  475U,  2U},
    {("A661_MAPVERT_ITEMLIST"),               0xA1B3U,  477U,  5U},
    {("A661_MAPVERT_PANEL"),                  0xA1B6U,  482U,  2U},
    {("A661_MAPVERT_SOURCE"),                 0xA1B4U,  484U,  6U},
    {("A661_MAP_BOUNDARY"),                   0xA1BAU,  490U,  7U},
    {("A661_MAP_EXTERNAL_SOURCE"),            0xA224U,  497U,  8U},
    {("A661_MAP_GRID"),                       0xA178U,  505U,  8U},
    {("A661_MASK_CONTAINER"),                 0xA1C0U,  513U,  7U},
    {("A661_MENU_BAR"),                       0xA1D0U,  520U,  8U},
    {("A661_MULTI_STATE_BUTTON"),             0xA348U,  528U, 16U},
    {("A661_MUTUALLY_EXCLUSIVE_CONTAINER"),   0xA400U,  544U,  6U},
    {("A661_NO_SERVICE_MONITOR"),             0xA4F0U,  550U,  2U},
    {("A661_NUMERIC_READOUT"),                0xA406U,  552U, 18U},
    {("A661_PAGING_CONTAINER"),               0xA408U,  570U, 14U},
    {("A661_PANEL"),                          0xA1F0U,  584U,  9U},
    {("A661_PICTURE"),                        0xA200U,  593U,  8U},
    {("A661_PICTURE_ANIMATED"),               0xA210U,  601U, 13U},
    {("A661_PICTURE_PUSH_BUTTON"),            0xA240U,  614U, 15U},
    {("A661_PICTURE_TOGGLE_BUTTON"),          0xA250U,  629U, 19U},
    {("A661_POP_UP_MENU"),                    0xA270U,  648U, 12U},
    {("A661_POP_UP_MENU_BUTTON"),             0xA280U,  660U, 27U},
    {("A661_POP_UP_PANEL"),                   0xA290U,  687U,  8U},
    {("A661_POP_UP_PANEL_BUTTON"),            0xA4E0U,  695U, 19U},
    {("A661_PROXY_BUTTON"),                   0xA420U,  714U,  4U},
    {("A661_PUSH_BUTTON"),                    0xA2A0U,  718U, 12U},
    {("A661_RADIO_BOX"),                      0xA2B0U,  730U,  2U},
    {("A661_ROTATION_CONTAINER"),             0xA2D0U,  732U,  5U},
    {("A661_SCALE_CONTAINER"),                0xA2F2U,  737U,  6U},
    {("A661_SCROLL_LIST"),                    0xA2F0U,  743U, 21U},
    {("A661_SCROLL_PANEL"),                   0xA300U,  764U, 26U},
    {("A661_SELECTION_LIST_BUTTON"),          0xA370U,  790U, 22U},
    {("A661_SHUFFLE_TO_FIT_CONTAINER"),       0xA4B0U,  812U, 12U},
    {("A661_SIZE_TO_FIT_CONTAINER"),          0xA4A0U,  824U, 10U},
    {("A661_SLIDER"),                         0xA440U,  834U, 19U},
    {("A661_SYMBOL"),                         0xA310U,  853U,  9U},
    {("A661_SYMBOL_ANIMATED"),                0xA450U,  862U, 15U},
    {("A661_SYMBOL_PUSH_BUTTON"),             0xA4C0U,  877U, 15U},
    {("A661_SYMBOL_TOGGLE_BUTTON"),           0xA4D0U,  892U, 19U},
    {("A661_TABBED_PANEL"),                   0xA320U,  911U, 12U},
    {("A661_TABBED_PANEL_GROUP"),             0xA330U,  923U, 11U},
    {("A661_TOGGLE_BUTTON"),                  0xA340U,  934U, 16U},
    {("A661_TOUCH_AREA"),                     0xA500U,  950U, 12U},
    {("A661_TRANSLATION_CONTAINER"),          0xA360U,  962U,  4U},
    {("A661_TREE"),                           0xA222U,  966U, 23U},
    {("A661_WATCHDOG_CONTAINER"),             0xA460U,  989U,  9U}
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
    {("A661_ARC"),                                        0x02U},
    {("A661_ASCII_EXTENDED"),                             0x00U},
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
    {("A661_CURSOR_CLICKED"),                             0xB1D4U},
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
    {("A661_EASING_IN"),                                  0x00U},
    {("A661_EASING_INOUT"),                               0x02U},
    {("A661_EASING_OUT"),                                 0x01U},
    {("A661_EDB_ALL_CHANGE"),                             0x01U},
    {("A661_EDB_CHANGE_CONFIRMED"),                       0x00U},
    {("A661_EDB_OPEN_CLOSE"),                             0x02U},
    {("A661_EDITING"),                                    0x1BU},
    {("A661_EFFECTIVITY_ARRAY"),                          0xB6C8U},
    {("A661_ENABLE"),                                     0xB180U},
    {("A661_ENABLE_ARRAY"),                               0xB1A0U},
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
    {("A661_GBK"),                                        0x02U},
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
    {("A661_HORZ_LINE"),                                  0x03U},
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
    {("A661_UTF8"),                                       0x01U},
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
 * \brief Finds a widget type by name.
 *
 * \param[in] name   Zero-terminated widget type name
 *
 * \retval    const BinaryWidget_t*   Widget type, or NULL when it is not supported.
 */
static const BinaryWidget_t* FindWidgetByName(const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds a widget type by its WidgetType code.
 *
 * \param[in] code   WidgetType value
 *
 * \retval    const BinaryWidget_t*   Widget type, or NULL when it is not supported.
 */
static const BinaryWidget_t* FindWidgetByCode(uint32_t code);

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
 * \brief Appends the A661_CMD_CREATE command of one widget to the output.
 *
 * \param[in]     definition    Definition File values
 * \param[in]     widgetIndex   Widget array index
 * \param[out]    buffer        Output bytes
 * \param[in]     capacity      Output buffer capacity
 * \param[in,out] offset        Output byte offset, advanced past the command
 *
 * \retval        TRUE          if the command was built.
 * \retval        FALSE         if the widget type, parent, or a property is not supported.
 */
static bool_t WriteWidget(const Arinc661_Definition_t* definition, uint32_t widgetIndex, byte_t* buffer, uint64_t capacity, uint64_t* offset);

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
 * \brief Reads one A661_CMD_CREATE command into the widget and property arrays.
 *
 * \param[in,out] definition           Caller-owned Definition File data
 * \param[in]     buffer               Input bytes
 * \param[in]     firstCommandOffset   Byte offset of the first command in this layer block
 * \param[in]     firstWidgetIndex     Widget array index of that first command
 * \param[in]     commandOffset        Byte offset of the command to read
 * \param[in]     commandSize          CommandSize of the command to read
 *
 * \retval        TRUE                 if the widget was read.
 * \retval        FALSE                if the widget type or parent is unknown, or storage is insufficient.
 */
static bool_t ReadWidget(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t firstCommandOffset, uint32_t firstWidgetIndex, uint64_t commandOffset, uint32_t commandSize);

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
    if ((offset + byteCount) <= capacity)
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
 *  Matching is case-sensitive, for example "A661_PANEL".
 */
static const BinaryWidget_t* FindWidgetByName(const char_t* name)
{
    const BinaryWidget_t* widget;
    uint32_t              widget_index;

    widget = NULL;
    for (widget_index = 0; widget_index < (sizeof(c_binaryWidgets) / sizeof(c_binaryWidgets[0])); ++widget_index)
    {
        if (0 == strcmp(name, c_binaryWidgets[widget_index].name))
        {
            widget = &c_binaryWidgets[widget_index];
            break;
        }
    }

    return widget;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, 0xA1F0 selects A661_PANEL.
 */
static const BinaryWidget_t* FindWidgetByCode(uint32_t code)
{
    const BinaryWidget_t* widget;
    uint32_t              widget_index;

    widget = NULL;
    for (widget_index = 0; widget_index < (sizeof(c_binaryWidgets) / sizeof(c_binaryWidgets[0])); ++widget_index)
    {
        if (code == c_binaryWidgets[widget_index].code)
        {
            widget = &c_binaryWidgets[widget_index];
            break;
        }
    }

    return widget;
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
    uint32_t                       property_index;
    uint32_t                       match_count;

    // ponytail: linear scan per lookup; index properties by owner if large files become slow
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
            for (constant_index = 0; constant_index < (sizeof(c_binaryConstants) / sizeof(c_binaryConstants[0])); ++constant_index)
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
 *  Converts the property text according to the parameter type:
 *
 * \verbatim
 * uchar, ushort, ulong, long   decimal integer or constant name, range-checked
 * float                        decimal number stored as IEEE 754 single precision
 * fr(180), fr(32768)           decimal number scaled by 2^31 / range and rounded
 * string                       value bytes followed by a zero terminator
 * bytes                        two hexadecimal digits per byte, such as "1B4200"
 * UnusedPad                    zero bytes
 * alignment                    zero bytes up to the next 32-bit boundary
 * \endverbatim
 *
 *  An absent property is written as zero or as an empty string. The fr() full
 *  positive range is one LSB short of representable and is clamped to it.
 *  Offsets of blocks and commands are multiples of four, so aligning the
 *  output offset aligns the position inside the command.
 */
static bool_t WriteField(const BinaryField_t* field, const Arinc661_Property_t* property, byte_t* buffer, uint64_t capacity, uint64_t* offset)
{
    float64_t real_value;
    float64_t range;
    float32_t float_value;
    int64_t   integer_value;
    int64_t   minimum;
    int64_t   maximum;
    const char_t* high_digit;
    const char_t* low_digit;
    uint64_t  byte_count;
    uint64_t  byte_index;
    uint32_t  encoded_value;
    int32_t   consumed;
    bool_t    is_valid;

    is_valid      = TRUE;
    encoded_value = 0;
    if (BINARY_FIELD_ALIGN == field->type)
    {
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
    else if (BINARY_FIELD_BYTES == field->type)
    {
        if (NULL != property)
        {
            // Exactly two digits per byte, with no separators
            is_valid = (strlen(property->value) == ((uint64_t)field->byteCount * 2U));
        }
        for (byte_index = 0; byte_index < field->byteCount; ++byte_index)
        {
            if (is_valid && (NULL != property))
            {
                high_digit = strchr(c_binaryHexDigits, property->value[byte_index * 2U]);
                low_digit  = strchr(c_binaryHexDigits, property->value[(byte_index * 2U) + 1U]);
                // The length check above keeps the terminator, which strchr also finds, out of reach
                is_valid   = (NULL != high_digit) && (NULL != low_digit);
                if (is_valid)
                {
                    encoded_value = (uint32_t)(((high_digit - c_binaryHexDigits) << 4) | (low_digit - c_binaryHexDigits));
                }
            }
            PutUnsigned(buffer, capacity, (*offset) + byte_index, encoded_value, 1);
        }
        (*offset) += field->byteCount;
    }
    else if (BINARY_FIELD_STRING == field->type)
    {
        byte_count = 0;
        if (NULL != property)
        {
            byte_count = strlen(property->value);
        }
        for (byte_index = 0; byte_index < byte_count; ++byte_index)
        {
            PutUnsigned(buffer, capacity, (*offset) + byte_index, (byte_t)property->value[byte_index], 1);
        }
        // Each string keeps its terminator; the command is padded after its last parameter
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
                consumed = FACEOE_Utl_Scan_Float(property->value, &real_value);
                is_valid = (0 < consumed) && ('\0' == property->value[consumed]);
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
                range    = (BINARY_FIELD_FR180 == field->type) ? 180.0 : 32768.0;
                is_valid = (real_value <= range) && (real_value >= -range);
                if (is_valid)
                {
                    // Scale to LSBs and round half away from zero
                    real_value = (real_value * BINARY_FR_FULL_SCALE) / range;
                    real_value = (0.0 <= real_value) ? (real_value + 0.5) : (real_value - 0.5);
                    if (real_value > (float64_t)INT32_MAX)
                    {
                        real_value = (float64_t)INT32_MAX;
                    }
                    if (real_value < (float64_t)INT32_MIN)
                    {
                        real_value = (float64_t)INT32_MIN;
                    }
                    encoded_value = (uint32_t)(int32_t)real_value;
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
                // Two's complement truncation keeps the 32-bit pattern of a negative long
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
 *  WidgetIdent comes from the widget's property of that name. ParentIdent is the
 *  WidgetIdent of the parent widget, which must precede this widget in the same
 *  layer. Remaining parameters follow the Creation Structure of the widget type;
 *  a widget property that names none of them returns FALSE, so that no value is
 *  silently dropped. The command is zero-padded to 32 bits.
 *
 *  An array takes its elements from the properties sharing its name, in order.
 *  Their number must equal the count parameter of the widget, for example
 *  NumberOfEntries, times the elements per entry:
 *
 * \verbatim
 * NumberOfEntries = 2
 * EntryList = "ON"         first entry
 * EntryList = "OFF"        second entry
 * \endverbatim
 */
static bool_t WriteWidget(const Arinc661_Definition_t* definition, uint32_t widgetIndex, byte_t* buffer, uint64_t capacity, uint64_t* offset)
{
    const Arinc661_Widget_t*   widget;
    const Arinc661_Property_t* property;
    const BinaryWidget_t*          type;
    const BinaryField_t*           field;
    int64_t                        widget_ident;
    int64_t                        parent_ident;
    int64_t                        entry_count;
    uint64_t                       cursor;
    uint32_t                       field_index;
    uint32_t                       element_index;
    uint32_t                       element_count;
    uint32_t                       property_index;
    uint32_t                       matched_count;
    uint32_t                       owned_count;
    bool_t                         is_valid;

    widget        = &definition->widgets[widgetIndex];
    widget_ident  = 0;
    type          = FindWidgetByName(widget->type);
    cursor        = (*offset) + BINARY_COMMAND_BYTES;
    parent_ident  = 0;
    entry_count   = 0;
    matched_count = 0;
    is_valid      = (NULL != type);
    if (is_valid)
    {
        property = FindProperty(definition, ARINC661_PROPERTY_WIDGET, widgetIndex, BINARY_PROPERTY_WIDGET_IDENT, 0);
        if (NULL != property)
        {
            matched_count += 1;
        }
        is_valid = ScanInteger(property, 0, UINT16_MAX, &widget_ident);
    }
    if (is_valid && (UINT32_MAX != widget->parentWidgetIndex))
    {
        // A nested parent must precede the current widget in the same layer
        is_valid = (widget->parentWidgetIndex < widgetIndex);
        if (is_valid)
        {
            is_valid = (widget->layerIndex == definition->widgets[widget->parentWidgetIndex].layerIndex);
        }
        if (is_valid)
        {
            property = FindProperty(definition, ARINC661_PROPERTY_WIDGET, widget->parentWidgetIndex, BINARY_PROPERTY_WIDGET_IDENT, 0);
            is_valid = ScanInteger(property, 0, UINT16_MAX, &parent_ident);
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
                if (is_valid && field->isCount)
                {
                    // WriteField accepted the value, so it converts again within the same range
                    (void)ScanInteger(property, 0, UINT16_MAX, &entry_count);
                }
            }
            else
            {
                // Every counted element needs its own property; surplus ones fail the ownership check below
                element_count = field->repeat * (uint32_t)entry_count;
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
        // Every property of this widget must have been placed in the command
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
        // Commands start on a 32-bit boundary, so padding the offset pads the command
        for (field_index = 0; field_index < BINARY_WORD_BYTES; ++field_index)
        {
            if (0 == (cursor % BINARY_WORD_BYTES))
            {
                break;
            }
            PutUnsigned(buffer, capacity, cursor, 0, 1);
            cursor += 1;
        }
        is_valid = ((cursor - (*offset)) <= UINT16_MAX);
    }
    if (is_valid)
    {
        PutUnsigned(buffer, capacity, (*offset), BINARY_CMD_CREATE, 2);
        PutUnsigned(buffer, capacity, (*offset) + 2, (uint32_t)(cursor - (*offset)), 2);
        PutUnsigned(buffer, capacity, (*offset) + 4, type->code, 2);
        PutUnsigned(buffer, capacity, (*offset) + 6, (uint32_t)widget_ident, 2);
        PutUnsigned(buffer, capacity, (*offset) + 8, (uint32_t)parent_ident, 2);
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
    text[byte_count] = '\0';

    return (uint32_t)byte_count;
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
    uint64_t                 name_byte_count;
    bool_t                   is_valid;

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
 *  Converts one parameter to property text, the inverse of WriteField. Integers
 *  become decimal numbers; constant names such as A661_TRUE are not restored
 *  because the same number has different names for different parameters. A
 *  string extends to its zero terminator, which must lie inside the command.
 *  Bytes without a numeric type become uppercase hexadecimal digits. Unused pads
 *  and alignment bytes are skipped without checking their content.
 */
static bool_t ReadField(Arinc661_Definition_t* definition, const BinaryField_t* field, const byte_t* buffer, uint64_t endOffset, uint64_t* offset)
{
    char_t    text[ARINC661_MAX_PROPERTY_BYTES];
    float64_t range;
    float32_t float_value;
    uint64_t  byte_count;
    uint32_t  byte_index;
    uint32_t  encoded_value;
    uint32_t  widget_index;
    bool_t    is_valid;

    // The widget being read has not been counted yet, so its index is the current count
    widget_index = definition->widgetCount;
    if (BINARY_FIELD_ALIGN == field->type)
    {
        (*offset) = ((*offset) + (BINARY_WORD_BYTES - 1U)) & ~(uint64_t)(BINARY_WORD_BYTES - 1U);
        is_valid  = ((*offset) <= endOffset);
    }
    else if (BINARY_FIELD_BYTES == field->type)
    {
        is_valid = (field->byteCount <= (endOffset - (*offset))) && (((uint64_t)field->byteCount * 2U) < sizeof(text));
        if (is_valid)
        {
            for (byte_index = 0; byte_index < field->byteCount; ++byte_index)
            {
                text[byte_index * 2U]        = c_binaryHexDigits[buffer[(*offset) + byte_index] >> 4];
                text[(byte_index * 2U) + 1U] = c_binaryHexDigits[buffer[(*offset) + byte_index] & 0x0FU];
            }
            is_valid = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, text, (uint64_t)field->byteCount * 2U);
            (*offset) += field->byteCount;
        }
    }
    else if (BINARY_FIELD_STRING == field->type)
    {
        // Locate the terminator without leaving the command
        for (byte_count = 0; ((*offset) + byte_count) < endOffset; ++byte_count)
        {
            if (0 == buffer[(*offset) + byte_count])
            {
                break;
            }
        }
        is_valid = (((*offset) + byte_count) < endOffset);
        if (is_valid)
        {
            is_valid = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, (const char_t*)&buffer[(*offset)], byte_count);
            (*offset) += (byte_count + 1);
        }
    }
    else
    {
        is_valid = ((*offset) <= endOffset) && (field->byteCount <= (endOffset - (*offset)));
        if (is_valid && (BINARY_FIELD_PAD != field->type))
        {
            encoded_value = GetUnsigned(buffer, (*offset), field->byteCount);
            if (BINARY_FIELD_FLOAT == field->type)
            {
                (void)memcpy(&float_value, &encoded_value, sizeof(float_value));
                // Infinities and NaN have no decimal property text
                is_valid = (float_value <= FLT_MAX) && (float_value >= -FLT_MAX);
                if (is_valid)
                {
                    byte_count = FormatReal(text, sizeof(text), float_value, BINARY_FLOAT_PRECISION);
                    is_valid   = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, text, byte_count);
                }
            }
            else if ((BINARY_FIELD_FR180 == field->type) || (BINARY_FIELD_FR32768 == field->type))
            {
                range      = (BINARY_FIELD_FR180 == field->type) ? 180.0 : 32768.0;
                byte_count = FormatReal(text, sizeof(text), ((float64_t)(int32_t)encoded_value * range) / BINARY_FR_FULL_SCALE, BINARY_FR_PRECISION);
                is_valid   = AddProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, text, byte_count);
            }
            else if (BINARY_FIELD_LONG == field->type)
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, (int32_t)encoded_value);
            }
            else
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, widget_index, field->name, encoded_value);
            }
        }
        (*offset) += field->byteCount;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds one widget and a property for WidgetIdent and for each parameter of its
 *  Creation Structure, in structure order. The widget name stays empty because
 *  the binary form does not carry names.
 *
 *  A zero ParentIdent places the widget directly under its layer. Any other
 *  value must equal the WidgetIdent of an earlier command in the same layer
 *  block (section 4.5.3.3.3); earlier commands are walked by their CommandSize
 *  to map that identifier to a widget array index.
 *
 *  Each array element becomes its own property, all sharing the array's name,
 *  in element order. The element count comes from the widget's count
 *  parameter, for example NumberOfEntries, read earlier in the same command.
 *
 *  The parameters must fill the command up to its 32-bit padding. Another
 *  length means the command follows a different supplement's structure, and
 *  returns FALSE.
 */
static bool_t ReadWidget(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t firstCommandOffset, uint32_t firstWidgetIndex, uint64_t commandOffset, uint32_t commandSize)
{
    Arinc661_Widget_t* widget;
    const BinaryWidget_t*  type;
    const BinaryField_t*   field;
    uint64_t               cursor;
    uint64_t               end_offset;
    uint32_t               parent_ident;
    uint32_t               widget_index;
    uint32_t               field_index;
    uint32_t               entry_count;
    uint32_t               element_index;
    uint32_t               element_count;
    bool_t                 is_valid;

    type       = FindWidgetByCode(GetUnsigned(buffer, commandOffset + 4, 2));
    end_offset = commandOffset + commandSize;
    is_valid   = (NULL != type);
    if (is_valid)
    {
        is_valid = (definition->widgetCount < definition->widgetCapacity);
    }
    if (is_valid)
    {
        widget = &definition->widgets[definition->widgetCount];
        (void)memset(widget, 0, sizeof((*widget)));
        (void)strcpy(widget->type, type->name);
        widget->layerIndex        = definition->layerCount - 1;
        widget->parentWidgetIndex = UINT32_MAX;
        parent_ident              = GetUnsigned(buffer, commandOffset + 8, 2);
        if (0 != parent_ident)
        {
            cursor = firstCommandOffset;
            for (widget_index = firstWidgetIndex; widget_index < definition->widgetCount; ++widget_index)
            {
                if (parent_ident == GetUnsigned(buffer, cursor + 6, 2))
                {
                    widget->parentWidgetIndex = widget_index;
                    break;
                }
                cursor += GetUnsigned(buffer, cursor + 2, 2);
            }
            is_valid = (UINT32_MAX != widget->parentWidgetIndex);
        }
    }
    if (is_valid)
    {
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, BINARY_PROPERTY_WIDGET_IDENT, GetUnsigned(buffer, commandOffset + 6, 2));
    }
    if (is_valid)
    {
        cursor      = commandOffset + BINARY_COMMAND_BYTES;
        entry_count = 0;
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            field         = &c_binaryFields[type->firstField + field_index];
            element_count = 1;
            if (0 != field->repeat)
            {
                element_count = field->repeat * entry_count;
            }
            else if (field->isCount && (cursor <= end_offset) && (field->byteCount <= (end_offset - cursor)))
            {
                entry_count = GetUnsigned(buffer, cursor, field->byteCount);
            }
            // A count larger than the command holds fails in ReadField at the command end
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
        // Only alignment padding may follow the last parameter
        is_valid = (cursor <= end_offset) && ((end_offset - cursor) < BINARY_WORD_BYTES);
    }
    if (is_valid)
    {
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
 * ...       A661_CMD_CREATE commands
 * C0        A661_END_LAYER_BLOCK
 * 000000    UnusedPad
 * \endverbatim
 *
 *  The layer name stays empty. Block Size and every CommandSize must be a
 *  multiple of four and stay inside the enclosing bytes. A command other than
 *  A661_CMD_CREATE, including A661_CMD_EXTEND, returns FALSE.
 */
static bool_t ReadLayer(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount, uint64_t* offset)
{
    Arinc661_Layer_t* layer;
    uint64_t              cursor;
    uint64_t              first_command_offset;
    uint64_t              end_offset;
    uint64_t              command_index;
    uint32_t              block_size;
    uint32_t              command_size;
    uint32_t              first_widget_index;
    uint32_t              layer_index;
    bool_t                is_valid;

    layer_index        = definition->layerCount;
    first_widget_index = definition->widgetCount;
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
        // Each command occupies at least three words, which bounds the walk
        for (command_index = 0; command_index < byteCount; ++command_index)
        {
            if (cursor >= end_offset)
            {
                break;
            }
            is_valid = ((BINARY_COMMAND_BYTES + 2U) <= (end_offset - cursor));
            if (is_valid)
            {
                command_size = GetUnsigned(buffer, cursor + 2, 2);
                is_valid     = (BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, 2)) && ((BINARY_COMMAND_BYTES + 2U) <= command_size) && (0 == (command_size % BINARY_WORD_BYTES)) && (command_size <= (end_offset - cursor));
            }
            if (is_valid)
            {
                is_valid = ReadWidget(definition, buffer, first_command_offset, first_widget_index, cursor, command_size);
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
 * A0 ...    layer blocks
 * E0        A661_DF_FOOTER
 * 000000    UnusedPad
 * \endverbatim
 *
 *  Charset encoding, symbol, picture, and animation law blocks are not
 *  supported and return FALSE. Bytes after the footer are ignored. Failure
 *  clears layerCount, widgetCount, and propertyCount; scalar fields and array
 *  entries may still contain partial data.
 */
bool_t Arinc661_ReadDefinitionBinary(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t byteCount)
{
    uint64_t offset;
    uint64_t block_index;
    bool_t   has_footer;
    bool_t   is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    // Reset result counts while retaining the supplied arrays and capacities
    definition->layerCount    = 0;
    definition->widgetCount   = 0;
    definition->propertyCount = 0;
    has_footer                = FALSE;
    offset                    = BINARY_HEADER_BYTES;
    is_valid                  = (BINARY_HEADER_BYTES <= byteCount);
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
        // Each block occupies at least one word, which bounds the walk
        for (block_index = 0; block_index < byteCount; ++block_index)
        {
            is_valid = (offset <= byteCount) && (BINARY_BLOCK_END_BYTES <= (byteCount - offset));
            if (FALSE == is_valid)
            {
                break;
            }
            if (BINARY_DF_FOOTER == buffer[offset])
            {
                has_footer = TRUE;
                break;
            }
            is_valid = (BINARY_BEGIN_LAYER_BLOCK == buffer[offset]);
            if (is_valid)
            {
                is_valid = ReadLayer(definition, buffer, byteCount, &offset);
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
        definition->layerCount    = 0;
        definition->widgetCount   = 0;
        definition->propertyCount = 0;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the header, one layer block per layer, and the footer. Each layer
 *  block holds the A661_CMD_CREATE commands of its widgets in array order, so a
 *  parent always precedes its children. For example, one layer with one panel:
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
    int64_t                        application_id;
    int64_t                        layer_id;
    int64_t                        context_number;
    uint64_t                       byte_count;
    uint64_t                       offset;
    uint64_t                       layer_offset;
    uint32_t                       layer_index;
    uint32_t                       widget_index;
    bool_t                         is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != buffer);
    // Keep zero as the failure result until the complete file is written
    byte_count     = 0;
    application_id = 0;
    layer_id       = 0;
    context_number = 0;
    offset         = BINARY_HEADER_BYTES;
    is_valid       = (UINT8_MAX >= definition->libraryVersion) && (UINT8_MAX >= definition->supplementVersion);
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
        // No OEM free data is written
        PutUnsigned(buffer, capacity, 6, 0, 2);
        for (layer_index = 0; layer_index < definition->layerCount; ++layer_index)
        {
            layer_offset = offset;
            property     = FindProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_LAYER_ID, 0);
            is_valid     = ScanInteger(property, 0, UINT8_MAX, &layer_id);
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
                // Select widgets belonging to this layer while preserving their array order
                if (layer_index == definition->widgets[widget_index].layerIndex)
                {
                    is_valid = WriteWidget(definition, widget_index, buffer, capacity, &offset);
                    if (FALSE == is_valid)
                    {
                        break;
                    }
                }
            }
            if (is_valid)
            {
                PutUnsigned(buffer, capacity, offset, (uint32_t)BINARY_END_LAYER_BLOCK << 24, BINARY_BLOCK_END_BYTES);
                offset   += BINARY_BLOCK_END_BYTES;
                // Block Size counts the block header and the end code
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
