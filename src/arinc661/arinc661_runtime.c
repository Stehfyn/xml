/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, sections 3 and 8: the Runtime Modifiable Parameters table of every widget
 * and extension, and Table 4.6-8, Parameter Types. Tables 4.6-8, 4.6-10, 4.6-11, 4.6-13, 4.6-14,
 * and 4.6-15: the constant names a property value may be, and their values. Generated from the
 * specification text by tools/runtime.py. Rows carried by a structure
 * Arinc661_ParameterStructure_t does not list, such as arrays and buffers, and rows whose
 * ParameterIdent Table 4.6-8 does not list, are left out.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_runtime.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "os/os_assert.h"

#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/** One row of a Runtime Modifiable Parameters table. */
typedef struct RuntimeRow_s
{
    uint32_t                      ident;      ///< ParameterIdent
    Arinc661_ParameterStructure_t structure;  ///< Structure carrying the value
    Arinc661_ParameterKind_t      kind;       ///< Type of the value, or of the first value of a pair
    const char_t*                 name;       ///< Parameter set
    Arinc661_ParameterKind_t      secondKind; ///< Type of the second value of a pair; unused otherwise
    const char_t*                 secondName; ///< Second parameter set by a pair, or NULL
} RuntimeRow_t;

/** One widget or extension type's table. */
typedef struct RuntimeType_s
{
    const char_t* type;     ///< A661_ type name
    uint32_t      firstRow; ///< Index of the type's first row in c_runtimeRows
    uint32_t      rowCount; ///< Number of the type's rows
} RuntimeType_t;

/** One constant name a property value may be. */
typedef struct RuntimeConstant_s
{
    const char_t* name;  ///< Constant name, such as A661_TRUE
    uint32_t      value; ///< Its value in Table 4.6-8, 4.6-10, 4.6-11, 4.6-13, 4.6-14, or 4.6-15
} RuntimeConstant_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/** Rows of every type, grouped by type, each group in table order. */
static const RuntimeRow_t c_runtimeRows[] =
{
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB0A8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "BlinkingType",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BLINKING_TYPE
    {0xB244, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "CheckButtonState",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INNER_STATE_CHECK
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB430, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SelectedEntry",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTED_ENTRY
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB4A0, ARINC661_PARAMETER_STRUCTURE_STRING_ARRAY, ARINC661_PARAMETER_STRING,  "EntryList",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ARRAY
    {0xB560, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "OpeningEntry",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_ENTRY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB432, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SelectingAreaWidth",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTING_WIDTH
    {0xB431, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SelectingAreaHeight",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTING_HEIGHT
    {0xB561, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "OpeningMode",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_MODE
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB170, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StartCursorPos",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CURSOR_POS
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB060, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "AlphaMask",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALPHA_MASK
    {0xB2B0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "NumericMask",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMERIC_MASK
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB520, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "Value",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VALUE
    {0xB580, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FLOAT,   "MinValue",                          ARINC661_PARAMETER_FLOAT, "MaxValue"},                          // A661_MINMAX_VALUES
    {0xB4D0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "TicsCoarse",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TICS_COARSE
    {0xB4E0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "TicsFine",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TICS_FINE
    {0xB172, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "StartCursorPosByte",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CURSOR_POS_BYTE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LegendString",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB550, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "FormatString",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FORMAT_STRING
    {0xB260, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "LegendPosition",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_POSITION
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB451, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "LegendAreaSizeX",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_AREA_SIZE_X
    {0xB452, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "LegendRemoved",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_REMOVED
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB170, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StartCursorPos",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CURSOR_POS
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB480, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "StartAngle",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_ANGLE
    {0xB1B0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "EndAngle",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_ANGLE
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB3F0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "Radius",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RADIUS
    {0xB480, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "StartAngle",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_ANGLE
    {0xB1B0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "EndAngle",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_ANGLE
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB240, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "InnerRadius",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INNER_RADIUS
    {0xB2D0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "OuterRadius",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OUTER_RADIUS
    {0xB480, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "StartAngle",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_ANGLE
    {0xB1B0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "EndAngle",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_ANGLE
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosXStart",                         ARINC661_PARAMETER_LONG,  "PosYStart"},                         // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosXStart",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosYStart",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB350, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosXEnd",                           ARINC661_PARAMETER_LONG,  "PosYEnd"},                           // A661_POS_XY2
    {0xB330, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosXEnd",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X2
    {0xB340, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosYEnd",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y2
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosXStart",                         ARINC661_PARAMETER_LONG,  "PosYStart"},                         // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosXStart",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosYStart",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB410, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "RotationAngle",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ROTATION_ANGLE
    {0xB270, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "LineLength",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LINE_LENGTH
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB350, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX2",                             ARINC661_PARAMETER_LONG,  "PosY2"},                             // A661_POS_XY2
    {0xB330, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX2",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X2
    {0xB340, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY2",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y2
    {0xB380, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX3",                             ARINC661_PARAMETER_LONG,  "PosY3"},                             // A661_POS_XY3
    {0xB360, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX3",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X3
    {0xB370, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY3",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y3
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB2F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "PictureReference",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_REFERENCE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB2C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "RotationAngle",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ORIENTATION
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB590, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Font",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FONT
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB1D0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EventFlag",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_EVENT_FLAG
    {0xB162, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "CoordinateReferencePointLatitude",  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CRP_LAT
    {0xB164, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "CoordinateReferencePointLongitude", ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CRP_LONG
    {0xB166, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FR180,   "CoordinateReferencePointLatitude",  ARINC661_PARAMETER_FR180, "CoordinateReferencePointLongitude"}, // A661_CRP_LAT_LONG
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB390, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "ProjectionReferencePointLatitude",  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_LAT
    {0xB3B0, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FR180,   "ProjectionReferencePointLatitude",  ARINC661_PARAMETER_FR180, "ProjectionReferencePointLongitude"}, // A661_PRP_LAT_LONG
    {0xB3A0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "ProjectionReferencePointLongitude", ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_LONG
    {0xB3C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "ScreenReferencePointX",             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_SCREEN_X
    {0xB3E0, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "ScreenReferencePointX",             ARINC661_PARAMETER_LONG,  "ScreenReferencePointY"},             // A661_PRP_SCREEN_XY
    {0xB3D0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "ScreenReferencePointY",             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_SCREEN_Y
    {0xB400, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR32768, "Range",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RANGE
    {0xB420, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "ScreenRange",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SCREEN_RANGE
    {0xB2C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "Orientation",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ORIENTATION
    {0xB010, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "AircraftLatitude",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AC_LAT
    {0xB020, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "AircraftLongitude",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AC_LONG
    {0xB030, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FR180,   "AircraftLatitude",                  ARINC661_PARAMETER_FR180, "AircraftLongitude"},                 // A661_AC_LAT_LONG
    {0xB040, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "AircraftOrientation",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AC_ORIENTATION
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB280, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MaskReference",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MASK_REFERENCE
    {0xB290, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "MaskEnabled",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MASK_ENABLED
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB2F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "PictureReference",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_REFERENCE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB258, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ToggleState",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INNER_STATE_TOGGLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB2F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "PictureReference",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_REFERENCE
    {0xB080, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AlternatePictureReference",         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALTERN_PICTURE_REFERENCE
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB498, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "AlternateLabelString",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ALTERNATE
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB088, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AlternateFlag",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALTERNATE_FLAG
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB1C1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "UAPositionFlag",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_UA_POSITION_FLAG
    {0xB1C2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticClosure",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_CLOSURE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB561, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "OpeningMode",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_MODE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB2F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "PictureReference",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_REFERENCE
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB561, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "OpeningMode",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_MODE
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB1C3, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PopupPosX",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_POS_X
    {0xB1C4, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PopupPosY",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_POS_Y
    {0xB1C5, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "PopupSizeX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_SIZE_X
    {0xB1C6, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "PopupSizeY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_SIZE_Y
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB150, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "CenterX",                           ARINC661_PARAMETER_LONG,  "CenterY"},                           // A661_CENTER_XY
    {0xB130, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "CenterX",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CENTER_X
    {0xB140, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "CenterY",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CENTER_Y
    {0xB410, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "RotationAngle",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ROTATION_ANGLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB219, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "HomeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HOME_X
    {0xB21A, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "HomeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HOME_Y
    {0xB210, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "FrameX",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FRAME_X
    {0xB220, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "FrameY",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FRAME_Y
    {0xB230, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "FrameX",                            ARINC661_PARAMETER_LONG,  "FrameY"},                            // A661_FRAME_XY
    {0xB0B0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "BoundX",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BOUND_X
    {0xB0C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "BoundY",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BOUND_Y
    {0xB0D0, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "BoundX",                            ARINC661_PARAMETER_LONG,  "BoundY"},                            // A661_BOUND_XY
    {0xB0E0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeXbound",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BOUND_SIZE_X
    {0xB0F0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeYbound",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BOUND_SIZE_Y
    {0xB100, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeXbound",                        ARINC661_PARAMETER_ULONG, "SizeYbound"},                        // A661_BOUND_SIZE_XY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB211, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "LineDeltaX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LINE_DELTA_X
    {0xB212, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "LineDeltaY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LINE_DELTA_Y
    {0xB213, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "PageDeltaX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PAGE_DELTA_X
    {0xB214, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "PageDeltaY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PAGE_DELTA_Y
    {0xB215, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeXsheet",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SHEET_SIZE_X
    {0xB216, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeYsheet",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SHEET_SIZE_Y
    {0xB217, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "HorizontalScroll",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HORIZONTAL_SCROLL
    {0xB218, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "VerticalScroll",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VERTICAL_SCROLL
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB2A8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfUAEntries",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_UA_ENTRIES
    {0xB1F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "FirstAccessibleEntry",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FIRST_ACCESS_ENTRY
    {0xB1F8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "FirstAccessibleUAEntry",            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FIRST_ACCESS_UA_ENTRY
    {0xB200, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "FirstVisibleEntry",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FIRST_VISIBLE_ENTRY
    {0xB440, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_SHORT,   "ShiftFirstVisibleEntry",            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SHIFT_FIRST_VISIBLE_ENTRY
    {0xB430, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SelectedEntry",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTED_ENTRY
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB2C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "RotationAngle",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ORIENTATION
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB4C0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SymbolReference",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYMBOL_REFERENCE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB2F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "PictureReference",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_REFERENCE
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB051, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "InsetSize",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INSET_SIZE
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB050, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "ActiveTabbedPanelID",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ACTIVE_TABBED_PANEL
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB052, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "TabPosition",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TAB_POSITION
    {0xB258, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ToggleState",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INNER_STATE_TOGGLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB498, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "AlternateLabelString",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ALTERNATE
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB088, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AlternateFlag",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALTERNATE_FLAG
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB510, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "TranslationX",                      ARINC661_PARAMETER_LONG,  "TranslationY"},                      // A661_TRANSLATION_XY
    {0xB4F0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "TranslationX",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TRANSLATION_X
    {0xB500, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "TranslationY",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TRANSLATION_Y
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB278, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FLOAT,   "OffsetX",                           ARINC661_PARAMETER_FLOAT, "OffsetY"},                           // A661_MAPGRID_OFFSET
    {0xB274, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FLOAT,   "IncrementX",                        ARINC661_PARAMETER_FLOAT, "IncrementY"},                        // A661_MAPGRID_CELLSIZE
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB660, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SourceX",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SOURCE_X
    {0xB661, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SourceY",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SOURCE_Y
    {0xB664, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SourceX",                           ARINC661_PARAMETER_ULONG, "SourceY"},                           // A661_SOURCE_XY
    {0xB662, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SourceDX",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SOURCE_DX
    {0xB663, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SourceDY",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SOURCE_DY
    {0xB665, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SourceDX",                          ARINC661_PARAMETER_ULONG, "SourceDY"},                          // A661_SOURCE_DXDY
    {0xB666, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SourceReference",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SOURCE_REF
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB402, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR32768, "RangeX",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RANGE_X
    {0xB403, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "RangeY",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RANGE_Y
    {0xB404, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FR32768, "RangeX",                            ARINC661_PARAMETER_LONG,  "RangeY"},                            // A661_RANGE_XY
    {0xB3C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "RefPosX",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_SCREEN_X
    {0xB3D0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "RefPosY",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_SCREEN_Y
    {0xB3E0, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "RefPosX",                           ARINC661_PARAMETER_LONG,  "RefPosY"},                           // A661_PRP_SCREEN_XY
    {0xB3B2, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR32768, "RefGeoPosX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_X
    {0xB3B3, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "RefGeoPosY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PRP_Y
    {0xB3B4, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FR32768, "RefGeoPosX",                        ARINC661_PARAMETER_LONG,  "RefGeoPosY"},                        // A661_PRP_XY
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB1D0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EventFlag",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_EVENT_FLAG
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB170, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StartCursorPos",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CURSOR_POS
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB218, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "VerticalScroll",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VERTICAL_SCROLL
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB430, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SelectedEntry",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTED_ENTRY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB170, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StartCursorPos",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CURSOR_POS
    {0xB4A0, ARINC661_PARAMETER_STRUCTURE_STRING_ARRAY, ARINC661_PARAMETER_STRING,  "EntryList",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ARRAY
    {0xB560, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "OpeningEntry",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_ENTRY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB561, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "OpeningMode",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_MODE
    {0xB432, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SelectingAreaWidth",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTING_WIDTH
    {0xB431, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SelectingAreaHeight",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTING_HEIGHT
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB640, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Horizontal",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MENU_HORIZONTAL
    {0xB641, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "ButtonPos",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BUTTON_POS
    {0xB642, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "ButtonSize",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BUTTON_SIZE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB540, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "VisibleChild",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE_CHILD
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB650, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "TargetWidgetIdent",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TARGET_WIDGET_ID
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB670, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Refresh",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_REFRESH
    {0xB671, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ShowFail",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SHOW_FAIL
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB520, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "Value",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VALUE
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB620, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "MinValue",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MIN_VALUE
    {0xB610, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "MaxValue",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAX_VALUE
    {0xB600, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "MajorTickInterval",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAJOR_TICK_INTERVAL
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB630, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MinorTickMultiple",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MINOR_TICK_MULTIPLE
    {0xB608, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "MajorTickReference",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAJOR_TICK_REFERENCE
    {0xB2C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_UCHAR,   "Orientation",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ORIENTATION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB631, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "LsbMultiple",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LSB_MULTIPLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB090, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationFlag",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_FLAG
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB091, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "IndexOfFrequency",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INDEX_OF_FREQUENCY
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB098, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationType",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_TYPE
    {0xB091, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "IndexOfFrequency",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INDEX_OF_FREQUENCY
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB430, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SelectedEntry",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTED_ENTRY
    {0xB560, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "OpeningEntry",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_ENTRY
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB4A0, ARINC661_PARAMETER_STRUCTURE_STRING_ARRAY, ARINC661_PARAMETER_STRING,  "EntryList",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ARRAY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB432, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SelectingAreaWidth",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTING_WIDTH
    {0xB431, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SelectingAreaHeight",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTING_HEIGHT
    {0xB561, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "OpeningMode",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_OPENING_MODE
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LegendString",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB550, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "FormatString",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FORMAT_STRING
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB4D3, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "PositiveString",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POSITIVE_STRING
    {0xB4D4, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "NegativeString",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEGATIVE_STRING
    {0xB451, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "LegendAreaSizeX",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_AREA_SIZE_X
    {0xB260, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "LegendPosition",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_POSITION
    {0xB452, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "LegendRemoved",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_REMOVED
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2BA, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusInAppId",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUS_IN_APP_ID
    {0xB2BB, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "NextFocusInLayerId",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUS_IN_LAYER_ID
    {0xB2BC, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusInId",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUS_IN_ID
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB5D0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfVisibleChildren",           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_VISIBLE_CHILDREN
    {0xB5C0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "SizeToFitMode",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_TO_FIT_MODE
    {0xB5C1, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "ItemSpacing",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ITEM_SPACING
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB5D0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfVisibleChildren",           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_VISIBLE_CHILDREN
    {0xB5E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ShuffleToFitMode",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SHUFFLE_TO_FIT_MODE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB5C1, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "ItemSpacing",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ITEM_SPACING
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB4C0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SymbolReference",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYMBOL_REFERENCE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB4C2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "SymbolPosition",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYMBOL_POSITION
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB258, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ToggleState",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INNER_STATE_TOGGLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB4C0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SymbolReference",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYMBOL_REFERENCE
    {0xB4C1, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AlternateSymbolReference",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALTERN_SYMBOL_REFERENCE
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB498, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "AlternateLabelString",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ALTERNATE
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB4C2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "SymbolPosition",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYMBOL_POSITION
    {0xB088, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AlternateFlag",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALTERNATE_FLAG
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LabelString",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB2F0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "PictureReference",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_REFERENCE
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB1C3, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PopupPosX",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_POS_X
    {0xB1C4, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PopupPosY",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_POS_Y
    {0xB1C5, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "PopupSizeX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_SIZE_X
    {0xB1C6, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "PopupSizeY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POPUP_SIZE_Y
    {0xB1C1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "UAPositionFlag",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_UA_POSITION_FLAG
    {0xB1C2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticClosure",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_CLOSURE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfVertices",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Closed",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CLOSED
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB540, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "VisibleChild",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE_CHILD
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB6E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "WrappingType",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PAGE_WRAP_TYPE
    {0xB6E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PagingControlPosition",             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PAGE_CONTROL_POSITION
    {0xB6E2, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "FinePageDelta",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FINE_PAGE_DELTA
    {0xB6E3, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "CoarsePageDelta",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COARSE_PAGE_DELTA
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB520, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "Value",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VALUE
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB490, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "LegendString",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING
    {0xB550, ARINC661_PARAMETER_STRUCTURE_STRING,       ARINC661_PARAMETER_STRING,  "FormatString",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FORMAT_STRING
    {0xB260, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "LegendPosition",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_POSITION
    {0xB2C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "RotationAngle",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ORIENTATION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB451, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "LegendAreaSizeX",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_AREA_SIZE_X
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB2C0, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "Orientation",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ORIENTATION
    {0xB400, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR32768, "Range",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RANGE
    {0xB420, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "ScreenRange",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SCREEN_RANGE
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB691, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "X",                                 ARINC661_PARAMETER_LONG,  "Y"},                                 // A661_REF_POS_XY
    {0xB690, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "X",                                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_REF_POS_X
    {0xB692, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "Y",                                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_REF_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB501, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Clamping",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CLAMPING
    {0xB520, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "Value",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VALUE
    {0xB502, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "StartValue",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_VALUE
    {0xB503, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "EndValue",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_VALUE
    {0xB504, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "StartScaledValue",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_SCALED_VALUE
    {0xB505, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "EndScaledValue",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_SCALED_VALUE
    {0xB501, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Clamping",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CLAMPING
    {0xB520, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "Value",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VALUE
    {0xB502, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "StartValue",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_VALUE
    {0xB503, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "EndValue",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_VALUE
    {0xB504, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "StartScaledValue",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_SCALED_VALUE
    {0xB505, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "EndScaledValue",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_SCALED_VALUE
    {0xB501, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Clamping",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CLAMPING
    {0xB520, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "Value",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VALUE
    {0xB502, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "StartValue",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_VALUE
    {0xB503, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "EndValue",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_VALUE
    {0xB504, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "StartScaledValue",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_START_SCALED_VALUE
    {0xB505, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "EndScaledValue",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_END_SCALED_VALUE
    {0xB5A0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ButtonState",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MULTI_STATE_VALUE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB5A1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "NumberOfStates",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_STATES
    {0xB4A0, ARINC661_PARAMETER_STRUCTURE_STRING_ARRAY, ARINC661_PARAMETER_STRING,  "LabelStringArray",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STRING_ARRAY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB2B9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AutomaticFocusMotion",              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_AUTO_FOCUS_MOTION
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB701, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "NumberOfKeys",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_KEYS
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfVertices",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB0A0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "NumberOfTouchPoints",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_TOUCH_POINTS
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB000, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Clear",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CLEAR
    {0xB2B8, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NextFocusedWidget",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NEXT_FOCUSED_WIDGET
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB703, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Duration",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DURATION
    {0xB702, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Delay",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DELAY
    {0xB706, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationState",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_STATE
    {0xB705, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationRepetition",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_REPETITION
    {0xB704, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationLawRef",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_LAW_REF
    {0xB707, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "FromAngle",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FROM_ANGLE
    {0xB70D, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR180,   "ToAngle",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TO_ANGLE
    {0xB150, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "CenterX",                           ARINC661_PARAMETER_LONG,  "CenterY"},                           // A661_CENTER_XY
    {0xB130, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "CenterX",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CENTER_X
    {0xB140, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "CenterY",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CENTER_Y
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB703, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Duration",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DURATION
    {0xB702, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Delay",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DELAY
    {0xB706, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationState",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_STATE
    {0xB705, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationRepetition",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_REPETITION
    {0xB704, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationLawRef",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_LAW_REF
    {0xB713, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Direction",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_DIRECTION
    {0xB714, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "NumberOfTurns",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_TURNS
    {0xB708, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "FromScaleX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FROM_SCALE_X
    {0xB709, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "FromScaleY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FROM_SCALE_Y
    {0xB715, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FLOAT,   "FromScaleX",                        ARINC661_PARAMETER_FLOAT, "FromScaleY"},                        // A661_FROM_SCALE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB70E, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "ToScaleX",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TO_SCALE_X
    {0xB70F, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "ToScaleY",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TO_SCALE_Y
    {0xB716, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FLOAT,   "ToScaleX",                          ARINC661_PARAMETER_FLOAT, "ToScaleY"},                          // A661_TO_SCALE_XY
    {0xB703, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Duration",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DURATION
    {0xB702, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Delay",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DELAY
    {0xB706, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationState",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_STATE
    {0xB705, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationRepetition",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_REPETITION
    {0xB704, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationLawRef",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_LAW_REF
    {0xB70B, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "FromValueX",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FROM_VALUE_X
    {0xB70C, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "FromValueY",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FROM_VALUE_Y
    {0xB717, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "FromValueX",                        ARINC661_PARAMETER_LONG,  "FromValueY"},                        // A661_FROM_VALUE_XY
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB711, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "ToValueX",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TO_VALUE_X
    {0xB712, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "ToValueY",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_TO_VALUE_Y
    {0xB718, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "ToValueX",                          ARINC661_PARAMETER_LONG,  "ToValueY"},                          // A661_TO_VALUE_XY
    {0xB703, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Duration",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DURATION
    {0xB702, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Delay",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DELAY
    {0xB706, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationState",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_STATE
    {0xB705, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationRepetition",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_REPETITION
    {0xB704, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationLawRef",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_LAW_REF
    {0xB702, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "Delay",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_DELAY
    {0xB706, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AnimationState",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_STATE
    {0xB705, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "AnimationRepetition",               ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ANIMATION_REPETITION
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB402, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FR32768, "RangeX",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RANGE_X
    {0xB403, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "RangeY",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_RANGE_Y
    {0xB404, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FR32768, "RangeX",                            ARINC661_PARAMETER_LONG,  "RangeY"},                            // A661_RANGE_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB691, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "X",                                 ARINC661_PARAMETER_LONG,  "Y"},                                 // A661_REF_POS_XY
    {0xB690, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "X",                                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_REF_POS_X
    {0xB692, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "Y",                                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_REF_POS_Y
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB277, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "MapSynchronizationNumber",          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_MAP_SYNCHRONIZATION_NUMBER
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB117, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "CachingEnabled",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CACHING_ENABLE
    {0xB4C7, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SyncFrame",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYNC_FRAME
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB0AE, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_FLOAT,   "ScaleX",                            ARINC661_PARAMETER_FLOAT, "ScaleY"},                            // A661_SCALE_XY
    {0xB0AC, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "ScaleX",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SCALE_X
    {0xB0AD, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_FLOAT,   "ScaleY",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SCALE_Y
    {0xB150, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "CenterX",                           ARINC661_PARAMETER_LONG,  "CenterY"},                           // A661_CENTER_XY
    {0xB130, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "CenterX",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CENTER_X
    {0xB140, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "CenterY",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CENTER_Y
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB430, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SelectedEntry",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTED_ENTRY
    {0xB5F0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Layout",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LAYOUT
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB180, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Enable",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENABLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfEntries",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB430, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SelectedEntry",                     ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SELECTED_ENTRY
    {0xB200, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "FirstVisibleEntry",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FIRST_VISIBLE_ENTRY
    {0xB570, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "EntryValidation",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ENTRY_VALID
    {0xB49A, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Alignment",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALIGNMENT
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB300, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_X
    {0xB310, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_LONG,    "PosY",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_POS_Y
    {0xB320, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_LONG,    "PosX",                              ARINC661_PARAMETER_LONG,  "PosY"},                              // A661_POS_XY
    {0xB450, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_X
    {0xB460, ARINC661_PARAMETER_STRUCTURE_4BYTES,       ARINC661_PARAMETER_ULONG,   "SizeY",                             ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SIZE_Y
    {0xB470, ARINC661_PARAMETER_STRUCTURE_8BYTES,       ARINC661_PARAMETER_ULONG,   "SizeX",                             ARINC661_PARAMETER_ULONG, "SizeY"},                             // A661_SIZE_XY
    {0xB666, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "SourceReference",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SOURCE_REF
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfVertices",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB530, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Visible",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE
    {0xB4B0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "StyleSet",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET
    {0xB160, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "ColorIndex",                        ARINC661_PARAMETER_UCHAR, NULL},                                // A661_COLOR_INDEX
    {0xB1E0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "FillIndex",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILL_INDEX
    {0xB1E8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Halo",                              ARINC661_PARAMETER_UCHAR, NULL},                                // A661_HALO
    {0xB1E1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Filled",                            ARINC661_PARAMETER_UCHAR, NULL},                                // A661_FILLED
    {0xB2A0, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfVertices",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_ENTRIES
    {0xB49B, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "LegendAlignment",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_LEGEND_ALIGNMENT
    {0xB680, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "VisibleChildIndex",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_VISIBLE_CHILD_INDEX
    {0xB6B0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "InitialFocus",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_INITIAL_FOCUS
    {0xB6B1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "StopForward",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STOP_FORWARD
    {0xB6B2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "StopBackward",                      ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STOP_BACKWARD
    {0xB6A1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "AllowChange",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_ALLOW_CHANGE
    {0xB6A0, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "CursorIndex",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_CURSOR_INDEX
    {0xB2A2, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfPictures",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_PICTURES
    {0xB2F1, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "PicturePosition",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PICTURE_POSITION
    {0xB2A4, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfSymbols",                   ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_SYMBOLS
    {0xB4C2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "SymbolPosition",                    ARINC661_PARAMETER_UCHAR, NULL},                                // A661_SYMBOL_POSITION
    {0xB0A2, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfTicsCoarse",                ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_TICS_COARSE
    {0xB0A4, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "NumberOfTicsFine",                  ARINC661_PARAMETER_UCHAR, NULL},                                // A661_NUMBER_OF_TICS_FINE
    {0xB4B2, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "StyleSetArraySize",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_STYLE_SET_ARRAY_SIZE
    {0xB111, ARINC661_PARAMETER_STRUCTURE_2BYTES,       ARINC661_PARAMETER_USHORT,  "BufferIndex",                       ARINC661_PARAMETER_UCHAR, NULL},                                // A661_PARAM_BUFFER_INDEX
    {0xB122, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "BoundaryCheckMode",                 ARINC661_PARAMETER_UCHAR, NULL},                                // A661_BOUNDARY_CHECK_MODE
    {0xB0AB, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "Reorder",                           ARINC661_PARAMETER_UCHAR, NULL},                                // A661_REORDER
    {0xB4A8, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "WordWrap",                          ARINC661_PARAMETER_UCHAR, NULL},                                // A661_WORD_WRAP
    {0xB4A9, ARINC661_PARAMETER_STRUCTURE_1BYTE,        ARINC661_PARAMETER_UCHAR,   "WrapStyle",                         ARINC661_PARAMETER_UCHAR, NULL},                                // A661_WRAP_STYLE
};

/** Types in name order, each with its group of rows. */
static const RuntimeType_t c_runtimeTypes[] =
{
    {"A661_ACTIVE_AREA",                   0,    12},
    {"A661_ANIMATION_GROUP",               920,  3},
    {"A661_ANIMATION_ONPARAM",             877,  6},
    {"A661_ANIMATION_ROTATION",            883,  13},
    {"A661_ANIMATION_SCALE",               896,  12},
    {"A661_ANIMATION_TRANSLATION",         908,  12},
    {"A661_BASIC_CONTAINER",               12,   5},
    {"A661_BLINKING_CONTAINER",            17,   2},
    {"A661_BOUNDARY_CHECK_EXTENSION",      1023, 1},
    {"A661_BROADCAST_RECEIVER",            811,  0},
    {"A661_BUFFER_FORMAT",                 19,   0},
    {"A661_CHECK_BUTTON",                  19,   16},
    {"A661_CHILD_INDEX_EXTENSION",         1009, 1},
    {"A661_COMBO_BOX",                     35,   20},
    {"A661_COMBO_BOX_EDIT",                529,  21},
    {"A661_CONNECTOR",                     55,   2},
    {"A661_CURSOR_OVER",                   649,  9},
    {"A661_CURSOR_POS_OVERLAY",            57,   7},
    {"A661_CURSOR_REF",                    647,  2},
    {"A661_CURSOR_SHAPE_EXTENSION",        1013, 2},
    {"A661_DATA_CONNECTOR",                944,  2},
    {"A661_DATA_SCALING_FR180",            805,  6},
    {"A661_DATA_SCALING_LONG",             793,  6},
    {"A661_DATA_SCALING_ULONG",            799,  6},
    {"A661_DIRECTIONAL_TABBING_EXTENSION", 1008, 0},
    {"A661_EDIT_BOX_MASKED",               64,   10},
    {"A661_EDIT_BOX_MULTI_LINE",           513,  16},
    {"A661_EDIT_BOX_NUMERIC",              74,   23},
    {"A661_EDIT_BOX_NUMERIC_BCD",          627,  20},
    {"A661_EDIT_BOX_TEXT",                 97,   15},
    {"A661_EXCLUDED_REGIONS_EXTENSION",    1023, 0},
    {"A661_EXTERNAL_SOURCE",               473,  15},
    {"A661_FOCUS_IN",                      659,  1},
    {"A661_FOCUS_LINK",                    658,  1},
    {"A661_FOCUS_OUT",                     660,  3},
    {"A661_FOCUS_STOP_EXTENSION",          1011, 2},
    {"A661_FRAMING_REFRESH_CONTAINER",     946,  4},
    {"A661_GESTURE_AREA",                  854,  9},
    {"A661_GP_ARC_CIRCLE",                 126,  12},
    {"A661_GP_ARC_ELLIPSE",                112,  14},
    {"A661_GP_CROWN",                      138,  13},
    {"A661_GP_LINE",                       151,  10},
    {"A661_GP_LINE_POLAR",                 161,  9},
    {"A661_GP_POLYLINE",                   742,  6},
    {"A661_GP_RECTANGLE",                  170,  12},
    {"A661_GP_TRIANGLE",                   182,  15},
    {"A661_GP_TRIANGLE_FAN",               994,  7},
    {"A661_GP_TRIANGLE_STRIP",             1001, 7},
    {"A661_INITIAL_FOCUS_EXTENSION",       1010, 1},
    {"A661_INK_AREA",                      863,  14},
    {"A661_KEYBOARD_AREA",                 827,  8},
    {"A661_LABEL",                         206,  13},
    {"A661_LABEL_COMPLEX",                 219,  10},
    {"A661_LEGEND_ALIGN_EXTENSION",        1008, 1},
    {"A661_MAPHORZ",                       239,  22},
    {"A661_MAPHORZ_CONTAINER",             778,  5},
    {"A661_MAPHORZ_ITEMLIST",              229,  4},
    {"A661_MAPHORZ_PANEL",                 783,  10},
    {"A661_MAPHORZ_SOURCE",                233,  6},
    {"A661_MAPHORZ_VERTEXBUFFER",          842,  2},
    {"A661_MAPVERT",                       488,  18},
    {"A661_MAPVERT_CONTAINER",             923,  8},
    {"A661_MAPVERT_ITEMLIST",              509,  4},
    {"A661_MAPVERT_PANEL",                 931,  10},
    {"A661_MAPVERT_SOURCE",                506,  3},
    {"A661_MAP_BOUNDARY",                  941,  3},
    {"A661_MAP_EXTERNAL_SOURCE",           985,  9},
    {"A661_MAP_GRID",                      469,  4},
    {"A661_MASK_CONTAINER",                261,  6},
    {"A661_MENU_BAR",                      550,  8},
    {"A661_MULTI_SELECTION_EXTENSION",     1023, 0},
    {"A661_MULTI_STATE_BUTTON",            811,  16},
    {"A661_MUTUALLY_EXCLUSIVE_CONTAINER",  558,  6},
    {"A661_NUMERIC_READOUT",               763,  15},
    {"A661_PAGING_CONTAINER",              748,  15},
    {"A661_PANEL",                         267,  9},
    {"A661_PICTURE",                       197,  9},
    {"A661_PICTURE_ANIMATED",              589,  10},
    {"A661_PICTURE_EXTENSION",             1015, 2},
    {"A661_PICTURE_PUSH_BUTTON",           276,  16},
    {"A661_PICTURE_TOGGLE_BUTTON",         292,  20},
    {"A661_POP_UP_MENU",                   322,  9},
    {"A661_POP_UP_MENU_BUTTON",            331,  21},
    {"A661_POP_UP_PANEL",                  312,  10},
    {"A661_POP_UP_PANEL_BUTTON",           722,  20},
    {"A661_PROXY_BUTTON",                  564,  2},
    {"A661_PUSH_BUTTON",                   352,  14},
    {"A661_RADIO_BOX",                     366,  2},
    {"A661_REORDER_EXTENSION",             1024, 1},
    {"A661_ROTATION_CONTAINER",            368,  5},
    {"A661_SCALE_CONTAINER",               950,  7},
    {"A661_SCROLL_LIST",                   402,  16},
    {"A661_SCROLL_PANEL",                  373,  29},
    {"A661_SCROLL_WHEEL_AREA",             835,  7},
    {"A661_SELECTION_LIST_BUTTON",         607,  20},
    {"A661_SELECTOR",                      957,  14},
    {"A661_SHUFFLE_TO_FIT_CONTAINER",      674,  12},
    {"A661_SIZE_TO_FIT_CONTAINER",         663,  11},
    {"A661_SLIDER",                        568,  21},
    {"A661_STATIC_PARAM_BUFFER_EXTENSION", 1022, 1},
    {"A661_STYLE_SET_EXTENSION",           1021, 1},
    {"A661_SYMBOL",                        418,  8},
    {"A661_SYMBOL_ANIMATED",               599,  8},
    {"A661_SYMBOL_EXTENSION",              1017, 2},
    {"A661_SYMBOL_PUSH_BUTTON",            686,  16},
    {"A661_SYMBOL_TOGGLE_BUTTON",          702,  20},
    {"A661_TABBED_PANEL",                  426,  10},
    {"A661_TABBED_PANEL_GROUP",            436,  12},
    {"A661_TICS_ARRAY_EXTENSION",          1019, 2},
    {"A661_TOGGLE_BUTTON",                 448,  17},
    {"A661_TOUCH_AREA",                    844,  10},
    {"A661_TRANSLATION_CONTAINER",         465,  4},
    {"A661_TREE",                          971,  14},
    {"A661_WATCHDOG_CONTAINER",            566,  2},
    {"A661_WORD_WRAP_EXTENSION",           1025, 2},
};

/** Constants in name order. A661_ORIENTATION and A661_RANGE have their Table 4.6-11 values, not their Table 4.6-8 ParameterIdents. */
static const RuntimeConstant_t c_runtimeConstants[] =
{
    {"A661_ABORT",                               0x02},
    {"A661_ABORTED",                             0x02},
    {"A661_ABSENT",                              0x10},
    {"A661_ACTIVE_TABBED_PANEL",                 0xB050},
    {"A661_AC_LAT",                              0xB010},
    {"A661_AC_LAT_LONG",                         0xB030},
    {"A661_AC_LONG",                             0xB020},
    {"A661_AC_ORIENTATION",                      0xB040},
    {"A661_ALIGNMENT",                           0xB49A},
    {"A661_ALLOW_CHANGE",                        0xB6A1},
    {"A661_ALPHA_MASK",                          0xB060},
    {"A661_ALTERNATE_FLAG",                      0xB088},
    {"A661_ALTERN_PICTURE_REFERENCE",            0xB080},
    {"A661_ALTERN_SYMBOL_REFERENCE",             0xB4C1},
    {"A661_ANIMATION_DELAY",                     0xB702},
    {"A661_ANIMATION_DURATION",                  0xB703},
    {"A661_ANIMATION_FLAG",                      0xB090},
    {"A661_ANIMATION_LAW_REF",                   0xB704},
    {"A661_ANIMATION_REPETITION",                0xB705},
    {"A661_ANIMATION_STATE",                     0xB706},
    {"A661_ANIMATION_TYPE",                      0xB098},
    {"A661_APP_DATA",                            0xB6D0},
    {"A661_ARC",                                 0x00000001},
    {"A661_ASCII_EXTENDED",                      0x0001},
    {"A661_AUTO_CLOSURE",                        0xB1C2},
    {"A661_AUTO_FOCUS_MOTION",                   0xB2B9},
    {"A661_BLINKING_TYPE",                       0xB0A8},
    {"A661_BLOCKED_BUFFER_OF_MAPITEM",           0xB121},
    {"A661_BLOCKED_BUFFER_OF_MAPVERT_ITEMS",     0xB126},
    {"A661_BOTH_CLAMPED",                        0x01},
    {"A661_BOTTOM",                              0x12},
    {"A661_BOTTOM_CENTER",                       0x30},
    {"A661_BOTTOM_LEFT",                         0x31},
    {"A661_BOTTOM_RIGHT",                        0x32},
    {"A661_BOTTOM_TO_TOP",                       0x40},
    {"A661_BOUNDARY_ARRAY",                      0xB6C1},
    {"A661_BOUNDARY_CHECK",                      0xBC},
    {"A661_BOUNDARY_CHECK_MODE",                 0xB122},
    {"A661_BOUND_SIZE_X",                        0xB0E0},
    {"A661_BOUND_SIZE_XY",                       0xB100},
    {"A661_BOUND_SIZE_Y",                        0xB0F0},
    {"A661_BOUND_X",                             0xB0B0},
    {"A661_BOUND_XY",                            0xB0D0},
    {"A661_BOUND_Y",                             0xB0C0},
    {"A661_BROADCAST_DATA",                      0xB0C2},
    {"A661_BUFFER_OF_FILL_STYLES",               0xB0F8},
    {"A661_BUFFER_OF_MAPITEM",                   0xB120},
    {"A661_BUFFER_OF_MAPVERT_ITEMS",             0xB125},
    {"A661_BUFFER_OF_PARAM",                     0xB110},
    {"A661_BUTTON_POS",                          0xB641},
    {"A661_BUTTON_SIZE",                         0xB642},
    {"A661_CACHING_ENABLE",                      0xB117},
    {"A661_CDS_DEPENDENT",                       0x1E},
    {"A661_CENTER",                              0x15},
    {"A661_CENTER_X",                            0xB130},
    {"A661_CENTER_XY",                           0xB150},
    {"A661_CENTER_Y",                            0xB140},
    {"A661_CIRCULAR",                            0x02},
    {"A661_CLAMPING",                            0xB501},
    {"A661_CLEAR",                               0xB000},
    {"A661_CLOCKWISE",                           0x00},
    {"A661_CLOSED",                              0xB1E2},
    {"A661_COARSE_PAGE_DELTA",                   0xB6E3},
    {"A661_COLOR_INDEX",                         0xB160},
    {"A661_COMPLETED",                           0x01},
    {"A661_COUNTER_CLOCKWISE",                   0x01},
    {"A661_CRP_LAT",                             0xB162},
    {"A661_CRP_LAT_LONG",                        0xB166},
    {"A661_CRP_LONG",                            0xB164},
    {"A661_CURSOR_CLICKED",                      0xB1D3},
    {"A661_CURSOR_DOUBLE_CLICKED",               0xB1D4},
    {"A661_CURSOR_INDEX",                        0xB6A0},
    {"A661_CURSOR_POS",                          0xB170},
    {"A661_CURSOR_POS_BYTE",                     0xB172},
    {"A661_CURSOR_PRESSED",                      0xB1D1},
    {"A661_CURSOR_RELEASED",                     0xB1D2},
    {"A661_CURSOR_RIGHT_CLICKED",                0xB1D5},
    {"A661_DEACTIVATE",                          0x04},
    {"A661_DIRECTION",                           0xB713},
    {"A661_DONT_RUN",                            0x00},
    {"A661_DOWN",                                0x1A},
    {"A661_DRAW_LINE_TO_CURSOR",                 0x36},
    {"A661_EASING_IN",                           0x0001},
    {"A661_EASING_INOUT",                        0x0003},
    {"A661_EASING_OUT",                          0x0002},
    {"A661_EDB_ALL_CHANGE",                      0x01},
    {"A661_EDB_CHANGE_CONFIRMED",                0x00},
    {"A661_EDB_OPEN_CLOSE",                      0x02},
    {"A661_EDITING",                             0x1B},
    {"A661_EFFECTIVITY_ARRAY",                   0xB6C8},
    {"A661_ENABLE",                              0xB180},
    {"A661_ENABLE_ENTRY_ARRAY",                  0xB1A8},
    {"A661_ENABLE_HANDLER",                      0xB1A1},
    {"A661_END",                                 0x03},
    {"A661_END_ANGLE",                           0xB1B0},
    {"A661_END_SCALED_VALUE",                    0xB505},
    {"A661_END_VALUE",                           0xB503},
    {"A661_ENTRY_ARRAY",                         0xB190},
    {"A661_ENTRY_POP_UP_ARRAY",                  0xB1C0},
    {"A661_ENTRY_VALID",                         0xB570},
    {"A661_ERROR",                               0x1C},
    {"A661_EVENT_FLAG",                          0xB1D0},
    {"A661_EVENT_NONE",                          0x00},
    {"A661_EXCLUDED_CIRCLE",                     0x03},
    {"A661_EXCLUDED_RECTANGLE",                  0x01},
    {"A661_EXCLUDED_REGIONS_LIST",               0xB6C9},
    {"A661_EXCLUDED_TRIANGLE",                   0x02},
    {"A661_FALSE",                               0x00},
    {"A661_FILLED",                              0xB1E1},
    {"A661_FILLED_POLY_START",                   0x2B},
    {"A661_FILLED_POLY_START_INTERACTIVE",       0xAB},
    {"A661_FILL_INDEX",                          0xB1E0},
    {"A661_FINE_PAGE_DELTA",                     0xB6E2},
    {"A661_FIRST_ACCESS_ENTRY",                  0xB1F0},
    {"A661_FIRST_ACCESS_UA_ENTRY",               0xB1F8},
    {"A661_FIRST_VISIBLE_ENTRY",                 0xB200},
    {"A661_FONT",                                0xB590},
    {"A661_FORMAT_STRING",                       0xB550},
    {"A661_FRAME_X",                             0xB210},
    {"A661_FRAME_XY",                            0xB230},
    {"A661_FRAME_Y",                             0xB220},
    {"A661_FROM",                                0x05},
    {"A661_FROM_ANGLE",                          0xB707},
    {"A661_FROM_SCALE_X",                        0xB708},
    {"A661_FROM_SCALE_XY",                       0xB715},
    {"A661_FROM_SCALE_Y",                        0xB709},
    {"A661_FROM_VALUE",                          0xB70A},
    {"A661_FROM_VALUE_X",                        0xB70B},
    {"A661_FROM_VALUE_XY",                       0xB717},
    {"A661_FROM_VALUE_Y",                        0xB70C},
    {"A661_GBK",                                 0x0003},
    {"A661_GESTURE_ABORT",                       0x04},
    {"A661_GESTURE_CANDIDATE",                   0x05},
    {"A661_GESTURE_DOUBLE_TAP",                  0x02},
    {"A661_GESTURE_DRAG",                        0x06},
    {"A661_GESTURE_END",                         0x03},
    {"A661_GESTURE_FLICK",                       0x07},
    {"A661_GESTURE_FLICK_DOWN",                  0x0B},
    {"A661_GESTURE_FLICK_LEFT",                  0x08},
    {"A661_GESTURE_FLICK_RIGHT",                 0x09},
    {"A661_GESTURE_FLICK_UP",                    0x0A},
    {"A661_GESTURE_HOLD",                        0x0C},
    {"A661_GESTURE_PINCH",                       0x04},
    {"A661_GESTURE_PRESS_AND_HOLD",              0x03},
    {"A661_GESTURE_ROTATE",                      0x05},
    {"A661_GESTURE_START",                       0x01},
    {"A661_GESTURE_TAP",                         0x01},
    {"A661_GESTURE_UPDATE",                      0x02},
    {"A661_GRID",                                0x03},
    {"A661_HALO",                                0xB1E8},
    {"A661_HIGHLIGHT_ONLY",                      0x04},
    {"A661_HOME_X",                              0xB219},
    {"A661_HOME_Y",                              0xB21A},
    {"A661_HORIZONTAL",                          0x01},
    {"A661_HORIZONTAL_SCROLL",                   0xB217},
    {"A661_HORZ_LINE",                           0x00000002},
    {"A661_HYPHENATE",                           0x02},
    {"A661_INDEX_OF_FREQUENCY",                  0xB091},
    {"A661_INITIAL_FOCUS",                       0xB6B0},
    {"A661_INNER_RADIUS",                        0xB240},
    {"A661_INNER_STATE_CHECK",                   0xB244},
    {"A661_INNER_STATE_TOGGLE",                  0xB258},
    {"A661_INSET_SIZE",                          0xB051},
    {"A661_IN_BOUNDS",                           0x01},
    {"A661_ITEM_SPACING",                        0xB5C1},
    {"A661_ITEM_STYLE",                          0x20},
    {"A661_ITEM_SYNCHRONIZATION",                0x51},
    {"A661_KEY_ARRAY",                           0xB700},
    {"A661_LAYOUT",                              0xB5F0},
    {"A661_LEFT",                                0x13},
    {"A661_LEFT_TO_RIGHT",                       0x41},
    {"A661_LEGEND",                              0x21},
    {"A661_LEGEND_ALIGNMENT",                    0xB49B},
    {"A661_LEGEND_ANCHOR",                       0x22},
    {"A661_LEGEND_ANCHOR_ROTATED",               0x35},
    {"A661_LEGEND_AND_HIGHLIGHT",                0x05},
    {"A661_LEGEND_AND_POPUP",                    0x03},
    {"A661_LEGEND_AND_POPUP_AND_HIGHLIGHT",      0x07},
    {"A661_LEGEND_AREA_SIZE_X",                  0xB451},
    {"A661_LEGEND_COMBO",                        0x38},
    {"A661_LEGEND_HIGHLIGHT",                    0x58},
    {"A661_LEGEND_ONLY",                         0x01},
    {"A661_LEGEND_POP_UP",                       0x23},
    {"A661_LEGEND_POSITION",                     0xB260},
    {"A661_LEGEND_READOUT",                      0x45},
    {"A661_LEGEND_READOUT_COMBO",                0x48},
    {"A661_LEGEND_READOUT_FORMAT_STRING",        0x44},
    {"A661_LEGEND_REMOVED",                      0xB452},
    {"A661_LINEAR",                              0x0000},
    {"A661_LINE_ARC",                            0x24},
    {"A661_LINE_ARC_INTERACTIVE",                0xA4},
    {"A661_LINE_DELTA_X",                        0xB211},
    {"A661_LINE_DELTA_Y",                        0xB212},
    {"A661_LINE_LENGTH",                         0xB270},
    {"A661_LINE_SEGMENT",                        0x25},
    {"A661_LINE_SEGMENT_INTERACTIVE",            0xA5},
    {"A661_LINE_START",                          0x26},
    {"A661_LINE_START_INTERACTIVE",              0xA6},
    {"A661_LOOP_FORWARD",                        0x00},
    {"A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK", 0x03},
    {"A661_LOOP_FORWARD_AND_BLANK",              0x02},
    {"A661_LOOP_FORWARD_AND_RESET",              0x01},
    {"A661_LSB_MULTIPLE",                        0xB631},
    {"A661_MAJOR_TICK_INTERVAL",                 0xB600},
    {"A661_MAJOR_TICK_REFERENCE",                0xB608},
    {"A661_MAPGRID_CELLSIZE",                    0xB274},
    {"A661_MAPGRID_OFFSET",                      0xB278},
    {"A661_MAP_SYNCHRONIZATION_NUMBER",          0xB277},
    {"A661_MASK_ENABLED",                        0xB290},
    {"A661_MASK_REFERENCE",                      0xB280},
    {"A661_MAX_VALUE",                           0xB610},
    {"A661_MDF_ABSOLUTE",                        0x65},
    {"A661_MDF_BRG_DIST_ACHDG",                  0x60},
    {"A661_MDF_DIST_DIST",                       0x67},
    {"A661_MDF_DIST_DIST_FIX",                   0x6A},
    {"A661_MDF_LAT_LONG",                        0x61},
    {"A661_MDF_LEGACY",                          0x62},
    {"A661_MDF_RELATIVE",                        0x66},
    {"A661_MDF_X_DIST",                          0x68},
    {"A661_MDF_Y_ALT",                           0x69},
    {"A661_MENU_HORIZONTAL",                     0xB640},
    {"A661_MINIMUM_SIZE",                        0x02},
    {"A661_MINMAX_VALUES",                       0xB580},
    {"A661_MINOR_TICK_MULTIPLE",                 0xB630},
    {"A661_MIN_VALUE",                           0xB620},
    {"A661_MULTI_STATE_VALUE",                   0xB5A0},
    {"A661_NEGATIVE_STRING",                     0xB4D4},
    {"A661_NEXT_FOCUSED_WIDGET",                 0xB2B8},
    {"A661_NEXT_FOCUS_IN_APP_ID",                0xB2BA},
    {"A661_NEXT_FOCUS_IN_ID",                    0xB2BC},
    {"A661_NEXT_FOCUS_IN_LAYER_ID",              0xB2BB},
    {"A661_NORMAL",                              0x1D},
    {"A661_NOT_CLAMPED",                         0x00},
    {"A661_NOT_USED",                            0x00},
    {"A661_NO_SHUFFLE",                          0x84},
    {"A661_NO_SIZING",                           0x74},
    {"A661_NUMBER_OF_ENTRIES",                   0xB2A0},
    {"A661_NUMBER_OF_KEYS",                      0xB701},
    {"A661_NUMBER_OF_PICTURES",                  0xB2A2},
    {"A661_NUMBER_OF_STATES",                    0xB5A1},
    {"A661_NUMBER_OF_SYMBOLS",                   0xB2A4},
    {"A661_NUMBER_OF_TICS_COARSE",               0xB0A2},
    {"A661_NUMBER_OF_TICS_FINE",                 0xB0A4},
    {"A661_NUMBER_OF_TOUCH_POINTS",              0xB0A0},
    {"A661_NUMBER_OF_TURNS",                     0xB714},
    {"A661_NUMBER_OF_UA_ENTRIES",                0xB2A8},
    {"A661_NUMBER_OF_VISIBLE_CHILDREN",          0xB5D0},
    {"A661_NUMERIC_MASK",                        0xB2B0},
    {"A661_OPENING_ENTRY",                       0xB560},
    {"A661_OPENING_MODE",                        0xB561},
    {"A661_OPEN_CENTERED",                       0x17},
    {"A661_OPEN_DOWN",                           0x18},
    {"A661_OPEN_ENTRY_ARRAY",                    0xB627},
    {"A661_OPEN_UP",                             0x16},
    {"A661_OPERATOR_ADD_ASSIGN_ARG",             0x0B},
    {"A661_OPERATOR_ADD_ASSIGN_SRC",             0x08},
    {"A661_OPERATOR_ASSIGNMENT",                 0x00},
    {"A661_OPERATOR_CONSTANT",                   0x02},
    {"A661_OPERATOR_INVERSE",                    0x01},
    {"A661_OPERATOR_LOGICAL_AND",                0x06},
    {"A661_OPERATOR_LOGICAL_OR",                 0x07},
    {"A661_OPERATOR_PRODUCT",                    0x05},
    {"A661_OPERATOR_PROD_ASSIGN_ARG",            0x0D},
    {"A661_OPERATOR_PROD_ASSIGN_SRC",            0x0A},
    {"A661_OPERATOR_SUB_ASSIGN_ARG",             0x0C},
    {"A661_OPERATOR_SUB_ASSIGN_SRC",             0x09},
    {"A661_OPERATOR_SUM",                        0x04},
    {"A661_ORIENTATION",                         0xB1D2},
    {"A661_OUTER_RADIUS",                        0xB2D0},
    {"A661_OUT_OF_BOUNDS",                       0x00},
    {"A661_PAGE_CONTROL_POSITION",               0xB6E1},
    {"A661_PAGE_DELTA_X",                        0xB213},
    {"A661_PAGE_DELTA_Y",                        0xB214},
    {"A661_PAGE_WRAP_TYPE",                      0xB6E0},
    {"A661_PARALLEL",                            0x00},
    {"A661_PARAM_BUFFER_INDEX",                  0xB111},
    {"A661_PARENT_ENTRY_ARRAY",                  0xB628},
    {"A661_PARKING_LINE_END",                    0xBB},
    {"A661_PARKING_LINE_START",                  0xBA},
    {"A661_PICTURE_ARRAY",                       0xB2E0},
    {"A661_PICTURE_ENTRY_ARRAY",                 0xB2E8},
    {"A661_PICTURE_POSITION",                    0xB2F1},
    {"A661_PICTURE_REFERENCE",                   0xB2F0},
    {"A661_PIX_FMT_COLOR_INDEXED_8",             0xC2},
    {"A661_PIX_FMT_LUMINANCE_ALPHA_8",           0xC1},
    {"A661_PIX_FMT_RGBA_8",                      0xC0},
    {"A661_PLAY",                                0x01},
    {"A661_POPUP_AND_HIGHLIGHT",                 0x06},
    {"A661_POPUP_ONLY",                          0x02},
    {"A661_POPUP_POS_X",                         0xB1C3},
    {"A661_POPUP_POS_Y",                         0xB1C4},
    {"A661_POPUP_SIZE_X",                        0xB1C5},
    {"A661_POPUP_SIZE_Y",                        0xB1C6},
    {"A661_POSITIVE_STRING",                     0xB4D3},
    {"A661_POS_X",                               0xB300},
    {"A661_POS_X2",                              0xB330},
    {"A661_POS_X3",                              0xB360},
    {"A661_POS_XY",                              0xB320},
    {"A661_POS_XY2",                             0xB350},
    {"A661_POS_XY3",                             0xB380},
    {"A661_POS_Y",                               0xB310},
    {"A661_POS_Y2",                              0xB340},
    {"A661_POS_Y3",                              0xB370},
    {"A661_PRESERVE_CHARACTER",                  0x01},
    {"A661_PRESERVE_WORD",                       0x00},
    {"A661_PROJECTION_REFERENCE",                0xB1D3},
    {"A661_PRP_LAT",                             0xB390},
    {"A661_PRP_LAT_LONG",                        0xB3B0},
    {"A661_PRP_LONG",                            0xB3A0},
    {"A661_PRP_SCREEN_X",                        0xB3C0},
    {"A661_PRP_SCREEN_XY",                       0xB3E0},
    {"A661_PRP_SCREEN_Y",                        0xB3D0},
    {"A661_PRP_X",                               0xB3B2},
    {"A661_PRP_XY",                              0xB3B4},
    {"A661_PRP_Y",                               0xB3B3},
    {"A661_RADIUS",                              0xB3F0},
    {"A661_RANGE",                               0xB1D1},
    {"A661_RANGE_X",                             0xB402},
    {"A661_RANGE_XY",                            0xB404},
    {"A661_RANGE_Y",                             0xB403},
    {"A661_REFRESH",                             0xB670},
    {"A661_REF_POS_X",                           0xB690},
    {"A661_REF_POS_XY",                          0xB691},
    {"A661_REF_POS_Y",                           0xB692},
    {"A661_REORDER",                             0xB0AB},
    {"A661_REPORT_ALL",                          0x61},
    {"A661_REPORT_ALL_WITH_CANDIDATE",           0x64},
    {"A661_REPORT_ON_CDS",                       0x63},
    {"A661_REPORT_ON_COMPLETION",                0x62},
    {"A661_REPORT_ON_TRANSITION",                0x60},
    {"A661_REPORT_ON_TRANSITION_WITH_CANDIDATE", 0x65},
    {"A661_REP_DEFAULT",                         0x00},
    {"A661_REP_FOCUS",                           0x02},
    {"A661_REP_HIGHLIGHT",                       0x03},
    {"A661_REP_STANDARD",                        0x01},
    {"A661_RIGHT",                               0x14},
    {"A661_RIGHT_TO_LEFT",                       0x42},
    {"A661_ROTATION_ANGLE",                      0xB410},
    {"A661_RUN",                                 0x01},
    {"A661_RUN_ONCE",                            0x02},
    {"A661_SAME_LEVEL",                          0x02},
    {"A661_SCALE_X",                             0xB0AC},
    {"A661_SCALE_XY",                            0xB0AE},
    {"A661_SCALE_Y",                             0xB0AD},
    {"A661_SCREEN_RANGE",                        0xB420},
    {"A661_SELECTED",                            0x01},
    {"A661_SELECTED_ENTRY",                      0xB430},
    {"A661_SELECTED_ENTRY_ARRAY",                0xB439},
    {"A661_SELECTING_HEIGHT",                    0xB431},
    {"A661_SELECTING_WIDTH",                     0xB432},
    {"A661_SELECT_ALL",                          0xB438},
    {"A661_SEQUENTIAL",                          0x01},
    {"A661_SEQUENTIAL_FROM",                     0x02},
    {"A661_SHEET_SIZE_X",                        0xB215},
    {"A661_SHEET_SIZE_Y",                        0xB216},
    {"A661_SHIFT_FIRST_VISIBLE_ENTRY",           0xB440},
    {"A661_SHOW_FAIL",                           0xB671},
    {"A661_SHOW_MAJOR_TICKS",                    0xB601},
    {"A661_SHUFFLE_DOWN",                        0x81},
    {"A661_SHUFFLE_LEFT",                        0x82},
    {"A661_SHUFFLE_RIGHT",                       0x83},
    {"A661_SHUFFLE_TO_FIT_MODE",                 0xB5E0},
    {"A661_SHUFFLE_UP",                          0x80},
    {"A661_SIZE_BOTTOM_UP",                      0x71},
    {"A661_SIZE_LEFT_TO_RIGHT",                  0x72},
    {"A661_SIZE_RIGHT_TO_LEFT",                  0x73},
    {"A661_SIZE_TOP_DOWN",                       0x70},
    {"A661_SIZE_TO_FIT_MODE",                    0xB5C0},
    {"A661_SIZE_X",                              0xB450},
    {"A661_SIZE_XY",                             0xB470},
    {"A661_SIZE_Y",                              0xB460},
    {"A661_SOURCE_DX",                           0xB662},
    {"A661_SOURCE_DXDY",                         0xB665},
    {"A661_SOURCE_DY",                           0xB663},
    {"A661_SOURCE_REF",                          0xB666},
    {"A661_SOURCE_WIDGET_PARAM",                 0x00},
    {"A661_SOURCE_X",                            0xB660},
    {"A661_SOURCE_XY",                           0xB664},
    {"A661_SOURCE_Y",                            0xB661},
    {"A661_STAR",                                0x04},
    {"A661_START_ANGLE",                         0xB480},
    {"A661_START_SCALED_VALUE",                  0xB504},
    {"A661_START_VALUE",                         0xB502},
    {"A661_STOP_BACKWARD",                       0xB6B2},
    {"A661_STOP_FORWARD",                        0xB6B1},
    {"A661_STRING",                              0xB490},
    {"A661_STRING_ALTERNATE",                    0xB498},
    {"A661_STRING_ARRAY",                        0xB4A0},
    {"A661_STYLE_SET",                           0xB4B0},
    {"A661_STYLE_SET_ARRAY",                     0xB4B1},
    {"A661_STYLE_SET_ARRAY_SIZE",                0xB4B2},
    {"A661_STYLE_SET_DEFAULT",                   0x0000},
    {"A661_SYMBOL_CIRCLE",                       0x27},
    {"A661_SYMBOL_CIRCLE_INTERACTIVE",           0xA7},
    {"A661_SYMBOL_ENTRY_ARRAY",                  0xB2E9},
    {"A661_SYMBOL_GENERIC",                      0x28},
    {"A661_SYMBOL_GENERIC_INTERACTIVE",          0xA8},
    {"A661_SYMBOL_OVAL",                         0x2C},
    {"A661_SYMBOL_OVAL_INTERACTIVE",             0xAC},
    {"A661_SYMBOL_PARKABLE",                     0xB8},
    {"A661_SYMBOL_PARKABLE_INTERACTIVE",         0xB9},
    {"A661_SYMBOL_POSITION",                     0xB4C2},
    {"A661_SYMBOL_RECTANGLE",                    0x37},
    {"A661_SYMBOL_RECTANGLE_INTERACTIVE",        0xB7},
    {"A661_SYMBOL_REFERENCE",                    0xB4C0},
    {"A661_SYMBOL_ROTATED",                      0x29},
    {"A661_SYMBOL_ROTATED_INTERACTIVE",          0xA9},
    {"A661_SYMBOL_ROTATED_PARKABLE",             0xBE},
    {"A661_SYMBOL_ROTATED_PARKABLE_INTERACTIVE", 0xBF},
    {"A661_SYMBOL_RUNWAY",                       0x2A},
    {"A661_SYMBOL_RUNWAY_INTERACTIVE",           0xAA},
    {"A661_SYMBOL_TARGET",                       0x2D},
    {"A661_SYMBOL_TARGET_INTERACTIVE",           0xAD},
    {"A661_SYMBOL_TARGET_PARKABLE",              0x3D},
    {"A661_SYMBOL_TARGET_PARKABLE_INTERACTIVE",  0xBD},
    {"A661_SYNC_FRAME",                          0xB4C7},
    {"A661_TAB_DIRECTION_WIDGET_ID_ARRAY",       0xB672},
    {"A661_TAB_POSITION",                        0xB052},
    {"A661_TARGET_WIDGET_ID",                    0xB650},
    {"A661_TICS_COARSE",                         0xB4D0},
    {"A661_TICS_COARSE_8BYTE",                   0xB4D2},
    {"A661_TICS_COARSE_ARRAY",                   0xB0A6},
    {"A661_TICS_FINE",                           0xB4E0},
    {"A661_TICS_FINE_8BYTE",                     0xB4E2},
    {"A661_TICS_FINE_ARRAY",                     0xB0AA},
    {"A661_TO",                                  0x06},
    {"A661_TOP",                                 0x11},
    {"A661_TOP_CENTER",                          0x33},
    {"A661_TOP_LEFT",                            0x34},
    {"A661_TOP_RIGHT",                           0x35},
    {"A661_TOP_TO_BOTTOM",                       0x43},
    {"A661_TOUCH_ABORT",                         0x04},
    {"A661_TOUCH_DOWN",                          0x01},
    {"A661_TOUCH_MOVE",                          0x02},
    {"A661_TOUCH_MOVE_OUTSIDE",                  0x05},
    {"A661_TOUCH_UP",                            0x03},
    {"A661_TO_ANGLE",                            0xB70D},
    {"A661_TO_BACK",                             0x01},
    {"A661_TO_FRONT",                            0x02},
    {"A661_TO_SCALE_X",                          0xB70E},
    {"A661_TO_SCALE_XY",                         0xB716},
    {"A661_TO_SCALE_Y",                          0xB70F},
    {"A661_TO_VALUE",                            0xB710},
    {"A661_TO_VALUE_X",                          0xB711},
    {"A661_TO_VALUE_XY",                         0xB718},
    {"A661_TO_VALUE_Y",                          0xB712},
    {"A661_TRANSLATION_X",                       0xB4F0},
    {"A661_TRANSLATION_XY",                      0xB510},
    {"A661_TRANSLATION_Y",                       0xB500},
    {"A661_TREE_NODE_STYLE_ARRAY",               0xB629},
    {"A661_TRIANGLE_END",                        0x32},
    {"A661_TRIANGLE_END_DOUBLE",                 0x33},
    {"A661_TRIANGLE_END_DOUBLE_INTERACTIVE",     0xB3},
    {"A661_TRIANGLE_END_INTERACTIVE",            0xB2},
    {"A661_TRIANGLE_FAN_START",                  0x34},
    {"A661_TRIANGLE_FAN_START_INTERACTIVE",      0xB4},
    {"A661_TRIANGLE_SEGMENT",                    0x2F},
    {"A661_TRIANGLE_SEGMENT_DOUBLE",             0x31},
    {"A661_TRIANGLE_SEGMENT_DOUBLE_INTERACTIVE", 0xB1},
    {"A661_TRIANGLE_SEGMENT_INTERACTIVE",        0xAF},
    {"A661_TRIANGLE_STRIP_START",                0x2E},
    {"A661_TRIANGLE_STRIP_START_INTERACTIVE",    0xAE},
    {"A661_TRUE",                                0x01},
    {"A661_TRUE_WITH_VALIDATION",                0x02},
    {"A661_UA_POSITION_FLAG",                    0xB1C1},
    {"A661_UNSELECTED",                          0x00},
    {"A661_UNSELECT_ALL",                        0xB43A},
    {"A661_UP",                                  0x19},
    {"A661_UTF8",                                0x0002},
    {"A661_VALIDATION",                          0x01},
    {"A661_VALIDATION_AND_WHEEL",                0x02},
    {"A661_VALUE",                               0xB520},
    {"A661_VALUE_8BYTE",                         0xB522},
    {"A661_VERTEX_RENDER_END",                   0x3B},
    {"A661_VERTEX_RENDER_LINES",                 0x01},
    {"A661_VERTEX_RENDER_LINE_LOOP",             0x02},
    {"A661_VERTEX_RENDER_POLYLINE",              0x03},
    {"A661_VERTEX_RENDER_SEGMENT",               0x3A},
    {"A661_VERTEX_RENDER_START",                 0x39},
    {"A661_VERTEX_RENDER_TRIANGLE",              0x04},
    {"A661_VERTEX_RENDER_TRIANGLE_FAN",          0x05},
    {"A661_VERTEX_RENDER_TRIANGLE_STRIP",        0x06},
    {"A661_VERTICAL",                            0x00},
    {"A661_VERTICAL_SCROLL",                     0xB218},
    {"A661_VERTICES_ARRAY",                      0xB528},
    {"A661_VERT_LINE",                           0x00000003},
    {"A661_VISIBLE",                             0xB530},
    {"A661_VISIBLE_CHILD",                       0xB540},
    {"A661_VISIBLE_CHILD_INDEX",                 0xB680},
    {"A661_WHEEL",                               0x03},
    {"A661_WORD_WRAP",                           0xB4A8},
    {"A661_WRAP_BOTH",                           0x01},
    {"A661_WRAP_FIRST_TO_LAST",                  0x02},
    {"A661_WRAP_LAST_TO_FIRST",                  0x03},
    {"A661_WRAP_NONE",                           0x00},
    {"A661_WRAP_STYLE",                          0xB4A9}
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds the type's table, then the row of the ParameterIdent in it. Compares at most
 *  ARINC661_MAX_NAME_BYTES characters of the type name.
 */
bool_t Arinc661_Runtime_GetParameter(const char_t* typeName, uint32_t parameterIdent, Arinc661_Runtime_Parameter_t* parameter)
{
    const RuntimeType_t* type;
    const RuntimeRow_t*  row;
    uint32_t             index;
    bool_t               names_match;
    bool_t               is_found;

    LOGASSERT(NULL != typeName);
    LOGASSERT(NULL != parameter);

    type = NULL;
    for (index = 0; index < COUNT_OF(c_runtimeTypes); ++index)
    {
        names_match = (0 == strncmp(typeName, c_runtimeTypes[index].type, ARINC661_MAX_NAME_BYTES));
        if (names_match)
        {
            type = &c_runtimeTypes[index];
            break;
        }
    }

    row = NULL;
    if (NULL != type)
    {
        for (index = type->firstRow; index < (type->firstRow + type->rowCount); ++index)
        {
            if (parameterIdent == c_runtimeRows[index].ident)
            {
                row = &c_runtimeRows[index];
                break;
            }
        }
    }

    is_found = (NULL != row);
    if (is_found)
    {
        parameter->ident      = row->ident;
        parameter->structure  = row->structure;
        parameter->kind       = row->kind;
        parameter->name       = row->name;
        parameter->secondKind = row->secondKind;
        parameter->secondName = row->secondName;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
uint32_t Arinc661_Runtime_GetConstantCount(void)
{
    return (uint32_t)COUNT_OF(c_runtimeConstants);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Constants are in name order.
 */
const char_t* Arinc661_Runtime_GetConstantName(uint32_t constantIndex)
{
    LOGASSERT(constantIndex < COUNT_OF(c_runtimeConstants));

    return c_runtimeConstants[constantIndex].name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
uint32_t Arinc661_Runtime_GetConstantValue(uint32_t constantIndex)
{
    LOGASSERT(constantIndex < COUNT_OF(c_runtimeConstants));

    return c_runtimeConstants[constantIndex].value;
}
