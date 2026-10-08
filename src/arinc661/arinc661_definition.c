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
#include "faceoe_utl/faceoe_utl_print.h"
#include "faceoe_utl/faceoe_utl_scan.h"
#include "faceoe_utl/faceoe_utl_xml.h"

#include "os/os_assert.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
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

/* ARINC 661 Part 1-7, section 4.5.3: 32-bit alignment of blocks and commands. */
#define BINARY_WORD_BYTES           (4U)  ///< Blocks and commands are aligned to 32 bits
#define BINARY_MAX_FIELDS           (40U) ///< Largest Creation Structure parameter count in the tables

/* ARINC 661 Part 1-7, section 3.3 data types: big-endian field widths. */
#define BINARY_UCHAR_BYTES    (1U) ///< uchar
#define BINARY_USHORT_BYTES   (2U) ///< ushort
#define BINARY_ULONG_BYTES    (4U) ///< ulong, long, float, and fr()
#define BINARY_VARIABLE_BYTES (0U) ///< Width of a field whose value sizes itself: string, alignment, sized bytes, region, or layer request

/* Elements per counted entry of a Creation Structure array. */
#define BINARY_SINGLE_VALUE   (0U) ///< elementsPerEntry of a field that is not an array
#define BINARY_ONE_PER_ENTRY  (1U) ///< elementsPerEntry of an array of single elements
#define BINARY_PAIR_PER_ENTRY (2U) ///< elementsPerEntry of an array of pairs, such as Vertices x and y

/* ARINC 661 Part 1-7, Tables 4.5.3.2-1 through 4.5.3.2-6: block codes. */
#define BINARY_BLOCK_CODE_SHIFT (24U) ///< A block code is the most significant byte of its 32-bit word

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

/* Widget and extension type names (Tables 4.6-7, 4.6-12, and 8.4). */
#define TYPE_A661_ACTIVE_AREA                          ("A661_ACTIVE_AREA")
#define TYPE_A661_ANIMATION_GROUP                      ("A661_ANIMATION_GROUP")
#define TYPE_A661_ANIMATION_ONPARAM                    ("A661_ANIMATION_ONPARAM")
#define TYPE_A661_ANIMATION_ROTATION                   ("A661_ANIMATION_ROTATION")
#define TYPE_A661_ANIMATION_SCALE                      ("A661_ANIMATION_SCALE")
#define TYPE_A661_ANIMATION_TRANSLATION                ("A661_ANIMATION_TRANSLATION")
#define TYPE_A661_BASIC_CONTAINER                      ("A661_BASIC_CONTAINER")
#define TYPE_A661_BLINKING_CONTAINER                   ("A661_BLINKING_CONTAINER")
#define TYPE_A661_BOUNDARY_CHECK_EXTENSION             ("A661_BOUNDARY_CHECK_EXTENSION")
#define TYPE_A661_BROADCAST_RECEIVER                   ("A661_BROADCAST_RECEIVER")
#define TYPE_A661_BUFFER_FORMAT                        ("A661_BUFFER_FORMAT")
#define TYPE_A661_CHECK_BUTTON                         ("A661_CHECK_BUTTON")
#define TYPE_A661_CHILD_INDEX_EXTENSION                ("A661_CHILD_INDEX_EXTENSION")
#define TYPE_A661_COMBO_BOX                            ("A661_COMBO_BOX")
#define TYPE_A661_COMBO_BOX_EDIT                       ("A661_COMBO_BOX_EDIT")
#define TYPE_A661_CONNECTOR                            ("A661_CONNECTOR")
#define TYPE_A661_CURSOR_ENTRY_EVENTS_EXTENSION        ("A661_CURSOR_ENTRY_EVENTS_EXTENSION")
#define TYPE_A661_CURSOR_EVENTS_EXTENSION              ("A661_CURSOR_EVENTS_EXTENSION")
#define TYPE_A661_CURSOR_OVER                          ("A661_CURSOR_OVER")
#define TYPE_A661_CURSOR_POS_OVERLAY                   ("A661_CURSOR_POS_OVERLAY")
#define TYPE_A661_CURSOR_REF                           ("A661_CURSOR_REF")
#define TYPE_A661_CURSOR_SHAPE_EXTENSION               ("A661_CURSOR_SHAPE_EXTENSION")
#define TYPE_A661_DATA_CONNECTOR                       ("A661_DATA_CONNECTOR")
#define TYPE_A661_DATA_SCALING_FR180                   ("A661_DATA_SCALING_FR180")
#define TYPE_A661_DATA_SCALING_LONG                    ("A661_DATA_SCALING_LONG")
#define TYPE_A661_DATA_SCALING_ULONG                   ("A661_DATA_SCALING_ULONG")
#define TYPE_A661_DIRECTIONAL_TABBING_EXTENSION        ("A661_DIRECTIONAL_TABBING_EXTENSION")
#define TYPE_A661_EDIT_BOX_MASKED                      ("A661_EDIT_BOX_MASKED")
#define TYPE_A661_EDIT_BOX_MULTI_LINE                  ("A661_EDIT_BOX_MULTI_LINE")
#define TYPE_A661_EDIT_BOX_NUMERIC                     ("A661_EDIT_BOX_NUMERIC")
#define TYPE_A661_EDIT_BOX_NUMERIC_BCD                 ("A661_EDIT_BOX_NUMERIC_BCD")
#define TYPE_A661_EDIT_BOX_TEXT                        ("A661_EDIT_BOX_TEXT")
#define TYPE_A661_EVENT_HANDLER                        ("A661_EVENT_HANDLER")
#define TYPE_A661_EXCLUDED_REGIONS_EXTENSION           ("A661_EXCLUDED_REGIONS_EXTENSION")
#define TYPE_A661_EXTERNAL_SOURCE                      ("A661_EXTERNAL_SOURCE")
#define TYPE_A661_FOCUS_IN                             ("A661_FOCUS_IN")
#define TYPE_A661_FOCUS_LINK                           ("A661_FOCUS_LINK")
#define TYPE_A661_FOCUS_OUT                            ("A661_FOCUS_OUT")
#define TYPE_A661_FOCUS_STOP_EXTENSION                 ("A661_FOCUS_STOP_EXTENSION")
#define TYPE_A661_FRAMING_REFRESH_CONTAINER            ("A661_FRAMING_REFRESH_CONTAINER")
#define TYPE_A661_GESTURE_AREA                         ("A661_GESTURE_AREA")
#define TYPE_A661_GP_ARC_CIRCLE                        ("A661_GP_ARC_CIRCLE")
#define TYPE_A661_GP_ARC_ELLIPSE                       ("A661_GP_ARC_ELLIPSE")
#define TYPE_A661_GP_CROWN                             ("A661_GP_CROWN")
#define TYPE_A661_GP_LINE                              ("A661_GP_LINE")
#define TYPE_A661_GP_LINE_POLAR                        ("A661_GP_LINE_POLAR")
#define TYPE_A661_GP_POLYLINE                          ("A661_GP_POLYLINE")
#define TYPE_A661_GP_RECTANGLE                         ("A661_GP_RECTANGLE")
#define TYPE_A661_GP_TRIANGLE                          ("A661_GP_TRIANGLE")
#define TYPE_A661_GP_TRIANGLE_FAN                      ("A661_GP_TRIANGLE_FAN")
#define TYPE_A661_GP_TRIANGLE_STRIP                    ("A661_GP_TRIANGLE_STRIP")
#define TYPE_A661_INITIAL_FOCUS_EXTENSION              ("A661_INITIAL_FOCUS_EXTENSION")
#define TYPE_A661_INK_AREA                             ("A661_INK_AREA")
#define TYPE_A661_KEYBOARD_AREA                        ("A661_KEYBOARD_AREA")
#define TYPE_A661_LABEL                                ("A661_LABEL")
#define TYPE_A661_LABEL_COMPLEX                        ("A661_LABEL_COMPLEX")
#define TYPE_A661_LEGEND_ALIGN_EXTENSION               ("A661_LEGEND_ALIGN_EXTENSION")
#define TYPE_A661_MAPHORZ                              ("A661_MAPHORZ")
#define TYPE_A661_MAPHORZ_CONTAINER                    ("A661_MAPHORZ_CONTAINER")
#define TYPE_A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION ("A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION")
#define TYPE_A661_MAPHORZ_ITEMLIST                     ("A661_MAPHORZ_ITEMLIST")
#define TYPE_A661_MAPHORZ_PANEL                        ("A661_MAPHORZ_PANEL")
#define TYPE_A661_MAPHORZ_SOURCE                       ("A661_MAPHORZ_SOURCE")
#define TYPE_A661_MAPHORZ_VERTEXBUFFER                 ("A661_MAPHORZ_VERTEXBUFFER")
#define TYPE_A661_MAPVERT                              ("A661_MAPVERT")
#define TYPE_A661_MAPVERT_CONTAINER                    ("A661_MAPVERT_CONTAINER")
#define TYPE_A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION ("A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION")
#define TYPE_A661_MAPVERT_ITEMLIST                     ("A661_MAPVERT_ITEMLIST")
#define TYPE_A661_MAPVERT_PANEL                        ("A661_MAPVERT_PANEL")
#define TYPE_A661_MAPVERT_SOURCE                       ("A661_MAPVERT_SOURCE")
#define TYPE_A661_MAP_BOUNDARY                         ("A661_MAP_BOUNDARY")
#define TYPE_A661_MAP_EXTERNAL_SOURCE                  ("A661_MAP_EXTERNAL_SOURCE")
#define TYPE_A661_MAP_GRID                             ("A661_MAP_GRID")
#define TYPE_A661_MASK_CONTAINER                       ("A661_MASK_CONTAINER")
#define TYPE_A661_MENU_BAR                             ("A661_MENU_BAR")
#define TYPE_A661_MULTI_SELECTION_EXTENSION            ("A661_MULTI_SELECTION_EXTENSION")
#define TYPE_A661_MULTI_STATE_BUTTON                   ("A661_MULTI_STATE_BUTTON")
#define TYPE_A661_MUTUALLY_EXCLUSIVE_CONTAINER         ("A661_MUTUALLY_EXCLUSIVE_CONTAINER")
#define TYPE_A661_NO_SERVICE_MONITOR                   ("A661_NO_SERVICE_MONITOR")
#define TYPE_A661_NUMERIC_READOUT                      ("A661_NUMERIC_READOUT")
#define TYPE_A661_PAGING_CONTAINER                     ("A661_PAGING_CONTAINER")
#define TYPE_A661_PANEL                                ("A661_PANEL")
#define TYPE_A661_PICTURE                              ("A661_PICTURE")
#define TYPE_A661_PICTURE_ANIMATED                     ("A661_PICTURE_ANIMATED")
#define TYPE_A661_PICTURE_EXTENSION                    ("A661_PICTURE_EXTENSION")
#define TYPE_A661_PICTURE_PUSH_BUTTON                  ("A661_PICTURE_PUSH_BUTTON")
#define TYPE_A661_PICTURE_TOGGLE_BUTTON                ("A661_PICTURE_TOGGLE_BUTTON")
#define TYPE_A661_POP_UP_MENU                          ("A661_POP_UP_MENU")
#define TYPE_A661_POP_UP_MENU_BUTTON                   ("A661_POP_UP_MENU_BUTTON")
#define TYPE_A661_POP_UP_PANEL                         ("A661_POP_UP_PANEL")
#define TYPE_A661_POP_UP_PANEL_BUTTON                  ("A661_POP_UP_PANEL_BUTTON")
#define TYPE_A661_PROXY_BUTTON                         ("A661_PROXY_BUTTON")
#define TYPE_A661_PUSH_BUTTON                          ("A661_PUSH_BUTTON")
#define TYPE_A661_RADIO_BOX                            ("A661_RADIO_BOX")
#define TYPE_A661_REORDER_EXTENSION                    ("A661_REORDER_EXTENSION")
#define TYPE_A661_ROTATION_CONTAINER                   ("A661_ROTATION_CONTAINER")
#define TYPE_A661_SCALE_CONTAINER                      ("A661_SCALE_CONTAINER")
#define TYPE_A661_SCROLL_LIST                          ("A661_SCROLL_LIST")
#define TYPE_A661_SCROLL_PANEL                         ("A661_SCROLL_PANEL")
#define TYPE_A661_SCROLL_WHEEL_AREA                    ("A661_SCROLL_WHEEL_AREA")
#define TYPE_A661_SELECTION_LIST_BUTTON                ("A661_SELECTION_LIST_BUTTON")
#define TYPE_A661_SELECTOR                             ("A661_SELECTOR")
#define TYPE_A661_SHUFFLE_TO_FIT_CONTAINER             ("A661_SHUFFLE_TO_FIT_CONTAINER")
#define TYPE_A661_SIZE_TO_FIT_CONTAINER                ("A661_SIZE_TO_FIT_CONTAINER")
#define TYPE_A661_SLIDER                               ("A661_SLIDER")
#define TYPE_A661_STATIC_PARAM_BUFFER_EXTENSION        ("A661_STATIC_PARAM_BUFFER_EXTENSION")
#define TYPE_A661_STYLE_SET_EXTENSION                  ("A661_STYLE_SET_EXTENSION")
#define TYPE_A661_SYMBOL                               ("A661_SYMBOL")
#define TYPE_A661_SYMBOL_ANIMATED                      ("A661_SYMBOL_ANIMATED")
#define TYPE_A661_SYMBOL_EXTENSION                     ("A661_SYMBOL_EXTENSION")
#define TYPE_A661_SYMBOL_PUSH_BUTTON                   ("A661_SYMBOL_PUSH_BUTTON")
#define TYPE_A661_SYMBOL_TOGGLE_BUTTON                 ("A661_SYMBOL_TOGGLE_BUTTON")
#define TYPE_A661_TABBED_PANEL                         ("A661_TABBED_PANEL")
#define TYPE_A661_TABBED_PANEL_GROUP                   ("A661_TABBED_PANEL_GROUP")
#define TYPE_A661_TICS_ARRAY_EXTENSION                 ("A661_TICS_ARRAY_EXTENSION")
#define TYPE_A661_TOGGLE_BUTTON                        ("A661_TOGGLE_BUTTON")
#define TYPE_A661_TOUCH_AREA                           ("A661_TOUCH_AREA")
#define TYPE_A661_TRANSLATION_CONTAINER                ("A661_TRANSLATION_CONTAINER")
#define TYPE_A661_TREE                                 ("A661_TREE")
#define TYPE_A661_WATCHDOG_CONTAINER                   ("A661_WATCHDOG_CONTAINER")
#define TYPE_A661_WORD_WRAP_EXTENSION                  ("A661_WORD_WRAP_EXTENSION")

/* Creation Structure parameter names. */
#define PARAMETER_ACTIVE_TABBED_PANEL_ID        ("ActiveTabbedPanelID")
#define PARAMETER_ALGORITHM_ARRAY               ("AlgorithmArray")
#define PARAMETER_ALIGNMENT                     ("Alignment")
#define PARAMETER_ALLOW_CHANGE                  ("AllowChange")
#define PARAMETER_ALLOW_OUTSIDE                 ("AllowOutside")
#define PARAMETER_ALPHA_MASK                    ("AlphaMask")
#define PARAMETER_ALTERNATE_FLAG                ("AlternateFlag")
#define PARAMETER_ALTERNATE_LABEL_STRING        ("AlternateLabelString")
#define PARAMETER_ALTERNATE_PICTURE_REFERENCE   ("AlternatePictureReference")
#define PARAMETER_ALTERNATE_SYMBOL_REFERENCE    ("AlternateSymbolReference")
#define PARAMETER_ANIMATION_FLAG                ("AnimationFlag")
#define PARAMETER_ANIMATION_LAW_REF             ("AnimationLawRef")
#define PARAMETER_ANIMATION_REPETITION          ("AnimationRepetition")
#define PARAMETER_ANIMATION_TYPE                ("AnimationType")
#define PARAMETER_ANONYMOUS                     ("Anonymous")
#define PARAMETER_AUTOMATIC_CLOSURE             ("AutomaticClosure")
#define PARAMETER_AUTOMATIC_FOCUS_MOTION        ("AutomaticFocusMotion")
#define PARAMETER_AUTOMATIC_INSET_SIZE_FLAG     ("AutomaticInsetSizeFlag")
#define PARAMETER_BEGIN_ANGLE_ARRAY             ("BeginAngleArray")
#define PARAMETER_BLINKING_TYPE                 ("BlinkingType")
#define PARAMETER_BOUNDARY_CHECK_MODE           ("BoundaryCheckMode")
#define PARAMETER_BOUND_X                       ("BoundX")
#define PARAMETER_BOUND_Y                       ("BoundY")
#define PARAMETER_BUFFER_ARRAY                  ("BufferArray")
#define PARAMETER_BUFFER_STRUCTURE              ("BufferStructure")
#define PARAMETER_BUTTON_POS                    ("ButtonPos")
#define PARAMETER_BUTTON_SIZE                   ("ButtonSize")
#define PARAMETER_BUTTON_STATE                  ("ButtonState")
#define PARAMETER_CACHING_ENABLED               ("CachingEnabled")
#define PARAMETER_CENTER_X                      ("CenterX")
#define PARAMETER_CENTER_Y                      ("CenterY")
#define PARAMETER_CHECK_BUTTON_STATE            ("CheckButtonState")
#define PARAMETER_CLAMPING                      ("Clamping")
#define PARAMETER_CLOSED                        ("Closed")
#define PARAMETER_COARSE_PAGE_DELTA             ("CoarsePageDelta")
#define PARAMETER_COLOR_INDEX                   ("ColorIndex")
#define PARAMETER_CONNECTOR_REFERENCE           ("ConnectorReference")
#define PARAMETER_COUNT_X                       ("CountX")
#define PARAMETER_COUNT_Y                       ("CountY")
#define PARAMETER_CURSOR_INDEX                  ("CursorIndex")
#define PARAMETER_CYCLIC_FLAG                   ("CyclicFlag")
#define PARAMETER_DATA                          ("Data")
#define PARAMETER_DATA_LENGTH                   ("DataLength")
#define PARAMETER_DEDICATED_KEY_IDENT           ("DedicatedKeyIdent")
#define PARAMETER_DEFAULT_STYLE_TEXT            ("DefaultStyleText")
#define PARAMETER_DELAY                         ("Delay")
#define PARAMETER_DIRECTION                     ("Direction")
#define PARAMETER_DISTANCE_ARRAY                ("DistanceArray")
#define PARAMETER_DURATION                      ("Duration")
#define PARAMETER_EFFECTIVITY_ARRAY             ("EffectivityArray")
#define PARAMETER_ENABLE                        ("Enable")
#define PARAMETER_ENABLE_ARRAY                  ("EnableArray")
#define PARAMETER_ENABLE_HANDLER                ("EnableHandler")
#define PARAMETER_END_ANGLE                     ("EndAngle")
#define PARAMETER_END_SCALED_VALUE              ("EndScaledValue")
#define PARAMETER_END_VALUE                     ("EndValue")
#define PARAMETER_ENTRY_LIST                    ("EntryList")
#define PARAMETER_EVENT_FLAG                    ("EventFlag")
#define PARAMETER_EVENT_ID_ARRAY                ("EventIdArray")
#define PARAMETER_EXCLUDED_REGIONS_LIST         ("ExcludedRegionsList")
#define PARAMETER_FAIL_COUNT_LIMIT              ("FailCountLimit")
#define PARAMETER_FILLED                        ("Filled")
#define PARAMETER_FILL_INDEX                    ("FillIndex")
#define PARAMETER_FINE_PAGE_DELTA               ("FinePageDelta")
#define PARAMETER_FIRST_ACCESSIBLE_ENTRY        ("FirstAccessibleEntry")
#define PARAMETER_FIRST_VISIBLE_ENTRY           ("FirstVisibleEntry")
#define PARAMETER_FLAG_REPORT_FRAME_POS         ("FlagReportFramePos")
#define PARAMETER_FLAG_REPORT_VISIBLE_ENTRY     ("FlagReportVisibleEntry")
#define PARAMETER_FONT                          ("Font")
#define PARAMETER_FORMAT_STRING                 ("FormatString")
#define PARAMETER_FRAME_X                       ("FrameX")
#define PARAMETER_FRAME_Y                       ("FrameY")
#define PARAMETER_FROM_ANGLE                    ("FromAngle")
#define PARAMETER_FROM_SCALE_X                  ("FromScaleX")
#define PARAMETER_FROM_SCALE_Y                  ("FromScaleY")
#define PARAMETER_FROM_VALUE                    ("FromValue")
#define PARAMETER_FROM_VALUE_X                  ("FromValueX")
#define PARAMETER_FROM_VALUE_Y                  ("FromValueY")
#define PARAMETER_HALO                          ("Halo")
#define PARAMETER_HOME_X                        ("HomeX")
#define PARAMETER_HOME_Y                        ("HomeY")
#define PARAMETER_HORIZONTAL                    ("Horizontal")
#define PARAMETER_HORIZONTAL_SCROLL             ("HorizontalScroll")
#define PARAMETER_INCREMENT_X                   ("IncrementX")
#define PARAMETER_INCREMENT_Y                   ("IncrementY")
#define PARAMETER_INDEX_OF_FREQUENCY            ("IndexOfFrequency")
#define PARAMETER_INITIAL_FOCUS                 ("InitialFocus")
#define PARAMETER_INNER_RADIUS                  ("InnerRadius")
#define PARAMETER_INSET_SIZE                    ("InsetSize")
#define PARAMETER_ITEM_SPACING                  ("ItemSpacing")
#define PARAMETER_KEY_ARRAY                     ("KeyArray")
#define PARAMETER_LABEL_STRING                  ("LabelString")
#define PARAMETER_LABEL_STRING_ARRAY            ("LabelStringArray")
#define PARAMETER_LAYER_REQ_RESULT_ARRAY        ("LayerReqResultArray")
#define PARAMETER_LEGEND_ALIGNMENT              ("LegendAlignment")
#define PARAMETER_LEGEND_AREA_SIZE_X            ("LegendAreaSizeX")
#define PARAMETER_LEGEND_POSITION               ("LegendPosition")
#define PARAMETER_LEGEND_REMOVED                ("LegendRemoved")
#define PARAMETER_LEGEND_STRING                 ("LegendString")
#define PARAMETER_LINE_DELTA_X                  ("LineDeltaX")
#define PARAMETER_LINE_DELTA_Y                  ("LineDeltaY")
#define PARAMETER_LINE_LENGTH                   ("LineLength")
#define PARAMETER_LOOP_FLAG                     ("LoopFlag")
#define PARAMETER_LOOP_TYPE                     ("LoopType")
#define PARAMETER_LSB_MULTIPLE                  ("LsbMultiple")
#define PARAMETER_MAJOR_TICK_INTERVAL           ("MajorTickInterval")
#define PARAMETER_MAJOR_TICK_REFERENCE          ("MajorTickReference")
#define PARAMETER_MAP_DATA_FORMAT               ("MapDataFormat")
#define PARAMETER_MAP_DATA_FORMAT_X             ("MapDataFormatX")
#define PARAMETER_MAP_DATA_FORMAT_Y             ("MapDataFormatY")
#define PARAMETER_MASK_ENABLED                  ("MaskEnabled")
#define PARAMETER_MASK_REFERENCE                ("MaskReference")
#define PARAMETER_MAXIMUM_SELECTABLE_ENTRIES    ("MaximumSelectableEntries")
#define PARAMETER_MAX_BUFFER_SIZE               ("MaxBufferSize")
#define PARAMETER_MAX_FIELDS_VALUES             ("MaxFieldsValues")
#define PARAMETER_MAX_FORMAT_STRING_LENGTH      ("MaxFormatStringLength")
#define PARAMETER_MAX_ITEM_SIZE                 ("MaxItemSize")
#define PARAMETER_MAX_LEGEND_STRING_LENGTH      ("MaxLegendStringLength")
#define PARAMETER_MAX_NUMBER_OF_ENTRIES         ("MaxNumberOfEntries")
#define PARAMETER_MAX_NUMBER_OF_ITEM            ("MaxNumberOfItem")
#define PARAMETER_MAX_NUMBER_OF_KEYS            ("MaxNumberOfKeys")
#define PARAMETER_MAX_NUMBER_OF_PICTURES        ("MaxNumberOfPictures")
#define PARAMETER_MAX_NUMBER_OF_REGIONS         ("MaxNumberOfRegions")
#define PARAMETER_MAX_NUMBER_OF_STATES          ("MaxNumberOfStates")
#define PARAMETER_MAX_NUMBER_OF_SYMBOLS         ("MaxNumberOfSymbols")
#define PARAMETER_MAX_NUMBER_OF_TICS_COARSE     ("MaxNumberOfTicsCoarse")
#define PARAMETER_MAX_NUMBER_OF_TICS_FINE       ("MaxNumberOfTicsFine")
#define PARAMETER_MAX_NUMBER_OF_VERTICES        ("MaxNumberOfVertices")
#define PARAMETER_MAX_STRING_LENGTH             ("MaxStringLength")
#define PARAMETER_MAX_STRING_LENGTH_POP_UP      ("MaxStringLengthPopUp")
#define PARAMETER_MAX_STYLE_SET_ARRAY_SIZE      ("MaxStyleSetArraySize")
#define PARAMETER_MAX_TOUCH_POINTS              ("MaxTouchPoints")
#define PARAMETER_MAX_VALUE                     ("MaxValue")
#define PARAMETER_MINOR_TICK_MULTIPLE           ("MinorTickMultiple")
#define PARAMETER_MIN_FIELDS_VALUES             ("MinFieldsValues")
#define PARAMETER_MIN_VALUE                     ("MinValue")
#define PARAMETER_MOTION_ALLOWED                ("MotionAllowed")
#define PARAMETER_MOVEMENT_X_ARRAY              ("MovementXArray")
#define PARAMETER_MOVEMENT_Y_ARRAY              ("MovementYArray")
#define PARAMETER_NEGATIVE_STRING               ("NegativeString")
#define PARAMETER_NEGATIVE_STRING_LENGTH        ("NegativeStringLength")
#define PARAMETER_NEXT_FOCUSED_WIDGET           ("NextFocusedWidget")
#define PARAMETER_NEXT_FOCUS_IN_APP_ID          ("NextFocusInAppId")
#define PARAMETER_NEXT_FOCUS_IN_ID              ("NextFocusInId")
#define PARAMETER_NEXT_FOCUS_IN_LAYER_ID        ("NextFocusInLayerId")
#define PARAMETER_NEXT_LAYER_CONNECTOR_REF      ("NextLayerConnectorRef")
#define PARAMETER_NODE_STYLE_ARRAY              ("NodeStyleArray")
#define PARAMETER_NUMBER_OF_BUFFERS             ("NumberOfBuffers")
#define PARAMETER_NUMBER_OF_ENTRIES             ("NumberOfEntries")
#define PARAMETER_NUMBER_OF_EVENTS              ("NumberOfEvents")
#define PARAMETER_NUMBER_OF_FIELDS              ("NumberOfFields")
#define PARAMETER_NUMBER_OF_KEYS                ("NumberOfKeys")
#define PARAMETER_NUMBER_OF_PICTURES            ("NumberOfPictures")
#define PARAMETER_NUMBER_OF_STATES              ("NumberOfStates")
#define PARAMETER_NUMBER_OF_SYMBOLS             ("NumberOfSymbols")
#define PARAMETER_NUMBER_OF_TICS_COARSE         ("NumberOfTicsCoarse")
#define PARAMETER_NUMBER_OF_TICS_FINE           ("NumberOfTicsFine")
#define PARAMETER_NUMBER_OF_TOUCH_POINTS        ("NumberOfTouchPoints")
#define PARAMETER_NUMBER_OF_TURNS               ("NumberOfTurns")
#define PARAMETER_NUMBER_OF_VERTICES            ("NumberOfVertices")
#define PARAMETER_NUMBER_OF_VISIBLE_CHILDREN    ("NumberOfVisibleChildren")
#define PARAMETER_NUMERIC_KEY_FLAG              ("NumericKeyFlag")
#define PARAMETER_NUMERIC_MASK                  ("NumericMask")
#define PARAMETER_NUM_DIRECTIONS                ("NumDirections")
#define PARAMETER_NUM_EVENTS                    ("NumEvents")
#define PARAMETER_NUM_LAYER_REQ_RESULTS         ("NumLayerReqResults")
#define PARAMETER_NUM_WIDGET_PARAM_RESULTS      ("NumWidgetParamResults")
#define PARAMETER_OFFSET_X                      ("OffsetX")
#define PARAMETER_OFFSET_Y                      ("OffsetY")
#define PARAMETER_OPENING_ENTRY                 ("OpeningEntry")
#define PARAMETER_OPENING_MODE                  ("OpeningMode")
#define PARAMETER_OPEN_ARRAY                    ("OpenArray")
#define PARAMETER_ORIENTATION                   ("Orientation")
#define PARAMETER_ORIENTATION_ARRAY             ("OrientationArray")
#define PARAMETER_OUTER_RADIUS                  ("OuterRadius")
#define PARAMETER_PAGE_DELTA_X                  ("PageDeltaX")
#define PARAMETER_PAGE_DELTA_Y                  ("PageDeltaY")
#define PARAMETER_PAGING_CONTROL_POSITION       ("PagingControlPosition")
#define PARAMETER_PARAMETER_IDENT               ("ParameterIdent")
#define PARAMETER_PARENT_ARRAY                  ("ParentArray")
#define PARAMETER_PICTURE_ARRAY                 ("PictureArray")
#define PARAMETER_PICTURE_POSITION              ("PicturePosition")
#define PARAMETER_PICTURE_REFERENCE             ("PictureReference")
#define PARAMETER_POPUP_POS_X                   ("PopupPosX")
#define PARAMETER_POPUP_POS_Y                   ("PopupPosY")
#define PARAMETER_POPUP_SIZE_X                  ("PopupSizeX")
#define PARAMETER_POPUP_SIZE_Y                  ("PopupSizeY")
#define PARAMETER_POP_UP_IDENT_ARRAY            ("PopUpIdentArray")
#define PARAMETER_POSITION_REPORT_MODE          ("PositionReportMode")
#define PARAMETER_POSITIVE_STRING               ("PositiveString")
#define PARAMETER_POSITIVE_STRING_LENGTH        ("PositiveStringLength")
#define PARAMETER_POS_X                         ("PosX")
#define PARAMETER_POS_X2                        ("PosX2")
#define PARAMETER_POS_X3                        ("PosX3")
#define PARAMETER_POS_X_END                     ("PosXEnd")
#define PARAMETER_POS_X_START                   ("PosXStart")
#define PARAMETER_POS_Y                         ("PosY")
#define PARAMETER_POS_Y2                        ("PosY2")
#define PARAMETER_POS_Y3                        ("PosY3")
#define PARAMETER_POS_Y_END                     ("PosYEnd")
#define PARAMETER_POS_Y_START                   ("PosYStart")
#define PARAMETER_PROJECTION_TYPE               ("ProjectionType")
#define PARAMETER_RADIUS                        ("Radius")
#define PARAMETER_RANGE                         ("Range")
#define PARAMETER_RANGE_X                       ("RangeX")
#define PARAMETER_RANGE_Y                       ("RangeY")
#define PARAMETER_REFRESH                       ("Refresh")
#define PARAMETER_REF_GEO_POS_X                 ("RefGeoPosX")
#define PARAMETER_REF_GEO_POS_Y                 ("RefGeoPosY")
#define PARAMETER_REF_POS_X                     ("RefPosX")
#define PARAMETER_REF_POS_Y                     ("RefPosY")
#define PARAMETER_REPORT_ALL_CHANGES            ("ReportAllChanges")
#define PARAMETER_REPORT_MODE                   ("ReportMode")
#define PARAMETER_REPORT_VISIBLE_CHILD          ("ReportVisibleChild")
#define PARAMETER_ROTATION_ANGLE                ("RotationAngle")
#define PARAMETER_SCALE_X                       ("ScaleX")
#define PARAMETER_SCALE_Y                       ("ScaleY")
#define PARAMETER_SCREEN_RANGE                  ("ScreenRange")
#define PARAMETER_SCREEN_REFERENCE_POINT_X      ("ScreenReferencePointX")
#define PARAMETER_SCREEN_REFERENCE_POINT_Y      ("ScreenReferencePointY")
#define PARAMETER_SELECTED_ENTRY                ("SelectedEntry")
#define PARAMETER_SELECTING_AREA_HEIGHT         ("SelectingAreaHeight")
#define PARAMETER_SELECTING_AREA_WIDTH          ("SelectingAreaWidth")
#define PARAMETER_SHOW_FAIL                     ("ShowFail")
#define PARAMETER_SHOW_IF_FAIL_IDENT            ("ShowIfFailIdent")
#define PARAMETER_SHOW_MAJOR_LABELS             ("ShowMajorLabels")
#define PARAMETER_SHOW_NO_SERVICE_IDENT         ("ShowNoServiceIdent")
#define PARAMETER_SHUFFLE_TO_FIT_MODE           ("ShuffleToFitMode")
#define PARAMETER_SIZE_OF_FIELDS                ("SizeOfFields")
#define PARAMETER_SIZE_TO_FIT_MODE              ("SizeToFitMode")
#define PARAMETER_SIZE_X                        ("SizeX")
#define PARAMETER_SIZE_XBOUND                   ("SizeXbound")
#define PARAMETER_SIZE_XSHEET                   ("SizeXsheet")
#define PARAMETER_SIZE_Y                        ("SizeY")
#define PARAMETER_SIZE_YBOUND                   ("SizeYbound")
#define PARAMETER_SIZE_YSHEET                   ("SizeYsheet")
#define PARAMETER_SOURCE_DX                     ("SourceDX")
#define PARAMETER_SOURCE_DY                     ("SourceDY")
#define PARAMETER_SOURCE_REFERENCE              ("SourceReference")
#define PARAMETER_SOURCE_X                      ("SourceX")
#define PARAMETER_SOURCE_Y                      ("SourceY")
#define PARAMETER_START_ANGLE                   ("StartAngle")
#define PARAMETER_START_CURSOR_POS              ("StartCursorPos")
#define PARAMETER_START_CURSOR_POS_BYTE         ("StartCursorPosByte")
#define PARAMETER_START_SCALED_VALUE            ("StartScaledValue")
#define PARAMETER_START_VALUE                   ("StartValue")
#define PARAMETER_STOP_BACKWARD                 ("StopBackward")
#define PARAMETER_STOP_FORWARD                  ("StopForward")
#define PARAMETER_STRING_ARRAY                  ("StringArray")
#define PARAMETER_STYLE_SET                     ("StyleSet")
#define PARAMETER_STYLE_SET_ARRAY               ("StyleSetArray")
#define PARAMETER_STYLE_SET_ARRAY_SIZE          ("StyleSetArraySize")
#define PARAMETER_SYMBOL_ARRAY                  ("SymbolArray")
#define PARAMETER_SYMBOL_POSITION               ("SymbolPosition")
#define PARAMETER_SYMBOL_REFERENCE              ("SymbolReference")
#define PARAMETER_TAB_DIRECTION_WIDGET_ID_ARRAY ("TabDirectionWidgetIdArray")
#define PARAMETER_TAB_POSITION                  ("TabPosition")
#define PARAMETER_TARGET_PARAMETER_ID           ("TargetParameterID")
#define PARAMETER_TARGET_PARAM_ID               ("TargetParamID")
#define PARAMETER_TARGET_WIDGET_ID              ("TargetWidgetID")
#define PARAMETER_TARGET_WIDGET_IDENT           ("TargetWidgetIdent")
#define PARAMETER_TICS_COARSE                   ("TicsCoarse")
#define PARAMETER_TICS_COARSE_ARRAY             ("TicsCoarseArray")
#define PARAMETER_TICS_FINE                     ("TicsFine")
#define PARAMETER_TICS_FINE_ARRAY               ("TicsFineArray")
#define PARAMETER_TIME_PERIOD                   ("TimePeriod")
#define PARAMETER_TOGGLE_STATE                  ("ToggleState")
#define PARAMETER_TO_ANGLE                      ("ToAngle")
#define PARAMETER_TO_SCALE_X                    ("ToScaleX")
#define PARAMETER_TO_SCALE_Y                    ("ToScaleY")
#define PARAMETER_TO_VALUE                      ("ToValue")
#define PARAMETER_TO_VALUE_X                    ("ToValueX")
#define PARAMETER_TO_VALUE_Y                    ("ToValueY")
#define PARAMETER_TRANSLATION_X                 ("TranslationX")
#define PARAMETER_TRANSLATION_Y                 ("TranslationY")
#define PARAMETER_TRIGGER_EVENT                 ("TriggerEvent")
#define PARAMETER_TRIGGER_LAYER_IDENT           ("TriggerLayerIdent")
#define PARAMETER_TRIGGER_WIDGET_IDENT          ("TriggerWidgetIdent")
#define PARAMETER_TYPE                          ("Type")
#define PARAMETER_UA_POSITION_FLAG              ("UAPositionFlag")
#define PARAMETER_VALID_COUNT_LIMIT             ("ValidCountLimit")
#define PARAMETER_VALUE                         ("Value")
#define PARAMETER_VERTICAL_SCROLL               ("VerticalScroll")
#define PARAMETER_VERTICES                      ("Vertices")
#define PARAMETER_VISIBLE                       ("Visible")
#define PARAMETER_VISIBLE_CHILD                 ("VisibleChild")
#define PARAMETER_VISIBLE_CHILD_INDEX           ("VisibleChildIndex")
#define PARAMETER_WIDGET_PARAM_RESULT_ARRAY     ("WidgetParamResultArray")
#define PARAMETER_WIDGET_STRUCTURE              ("WidgetStructure")
#define PARAMETER_WORD_WRAP                     ("WordWrap")
#define PARAMETER_WRAPPING_TYPE                 ("WrappingType")
#define PARAMETER_WRAP_STYLE                    ("WrapStyle")

/* Constant names of section 4.6, for c_binaryConstants and the value lists. */
#define CONSTANT_A661_ABORT                               ("A661_ABORT")
#define CONSTANT_A661_ABORTED                             ("A661_ABORTED")
#define CONSTANT_A661_ABSENT                              ("A661_ABSENT")
#define CONSTANT_A661_ACTIVE_TABBED_PANEL                 ("A661_ACTIVE_TABBED_PANEL")
#define CONSTANT_A661_AC_LAT                              ("A661_AC_LAT")
#define CONSTANT_A661_AC_LAT_LONG                         ("A661_AC_LAT_LONG")
#define CONSTANT_A661_AC_LONG                             ("A661_AC_LONG")
#define CONSTANT_A661_AC_ORIENTATION                      ("A661_AC_ORIENTATION")
#define CONSTANT_A661_ALIGNMENT                           ("A661_ALIGNMENT")
#define CONSTANT_A661_ALLOW_CHANGE                        ("A661_ALLOW_CHANGE")
#define CONSTANT_A661_ALPHA_MASK                          ("A661_ALPHA_MASK")
#define CONSTANT_A661_ALTERNATE_FLAG                      ("A661_ALTERNATE_FLAG")
#define CONSTANT_A661_ALTERN_PICTURE_REFERENCE            ("A661_ALTERN_PICTURE_REFERENCE")
#define CONSTANT_A661_ALTERN_SYMBOL_REFERENCE             ("A661_ALTERN_SYMBOL_REFERENCE")
#define CONSTANT_A661_ANIMATION_DELAY                     ("A661_ANIMATION_DELAY")
#define CONSTANT_A661_ANIMATION_DURATION                  ("A661_ANIMATION_DURATION")
#define CONSTANT_A661_ANIMATION_FLAG                      ("A661_ANIMATION_FLAG")
#define CONSTANT_A661_ANIMATION_LAW_REF                   ("A661_ANIMATION_LAW_REF")
#define CONSTANT_A661_ANIMATION_REPETITION                ("A661_ANIMATION_REPETITION")
#define CONSTANT_A661_ANIMATION_STATE                     ("A661_ANIMATION_STATE")
#define CONSTANT_A661_ANIMATION_TYPE                      ("A661_ANIMATION_TYPE")
#define CONSTANT_A661_APP_DATA                            ("A661_APP_DATA")
#define CONSTANT_A661_ARC                                 ("A661_ARC")
#define CONSTANT_A661_ASCII_EXTENDED                      ("A661_ASCII_EXTENDED")
#define CONSTANT_A661_AUTO_CLOSURE                        ("A661_AUTO_CLOSURE")
#define CONSTANT_A661_AUTO_FOCUS_MOTION                   ("A661_AUTO_FOCUS_MOTION")
#define CONSTANT_A661_BLINKING_TYPE                       ("A661_BLINKING_TYPE")
#define CONSTANT_A661_BLOCKED_BUFFER_OF_MAPITEM           ("A661_BLOCKED_BUFFER_OF_MAPITEM")
#define CONSTANT_A661_BLOCKED_BUFFER_OF_MAPVERT_ITEMS     ("A661_BLOCKED_BUFFER_OF_MAPVERT_ITEMS")
#define CONSTANT_A661_BOTH_CLAMPED                        ("A661_BOTH_CLAMPED")
#define CONSTANT_A661_BOTTOM                              ("A661_BOTTOM")
#define CONSTANT_A661_BOTTOM_CENTER                       ("A661_BOTTOM_CENTER")
#define CONSTANT_A661_BOTTOM_LEFT                         ("A661_BOTTOM_LEFT")
#define CONSTANT_A661_BOTTOM_RIGHT                        ("A661_BOTTOM_RIGHT")
#define CONSTANT_A661_BOTTOM_TO_TOP                       ("A661_BOTTOM_TO_TOP")
#define CONSTANT_A661_BOUNDARY_ARRAY                      ("A661_BOUNDARY_ARRAY")
#define CONSTANT_A661_BOUNDARY_CHECK                      ("A661_BOUNDARY_CHECK")
#define CONSTANT_A661_BOUNDARY_CHECK_MODE                 ("A661_BOUNDARY_CHECK_MODE")
#define CONSTANT_A661_BOUND_SIZE_X                        ("A661_BOUND_SIZE_X")
#define CONSTANT_A661_BOUND_SIZE_XY                       ("A661_BOUND_SIZE_XY")
#define CONSTANT_A661_BOUND_SIZE_Y                        ("A661_BOUND_SIZE_Y")
#define CONSTANT_A661_BOUND_X                             ("A661_BOUND_X")
#define CONSTANT_A661_BOUND_XY                            ("A661_BOUND_XY")
#define CONSTANT_A661_BOUND_Y                             ("A661_BOUND_Y")
#define CONSTANT_A661_BROADCAST_DATA                      ("A661_BROADCAST_DATA")
#define CONSTANT_A661_BUFFER_OF_FILL_STYLES               ("A661_BUFFER_OF_FILL_STYLES")
#define CONSTANT_A661_BUFFER_OF_MAPITEM                   ("A661_BUFFER_OF_MAPITEM")
#define CONSTANT_A661_BUFFER_OF_MAPVERT_ITEMS             ("A661_BUFFER_OF_MAPVERT_ITEMS")
#define CONSTANT_A661_BUFFER_OF_PARAM                     ("A661_BUFFER_OF_PARAM")
#define CONSTANT_A661_BUTTON_POS                          ("A661_BUTTON_POS")
#define CONSTANT_A661_BUTTON_SIZE                         ("A661_BUTTON_SIZE")
#define CONSTANT_A661_CACHING_ENABLE                      ("A661_CACHING_ENABLE")
#define CONSTANT_A661_CDS_DEPENDENT                       ("A661_CDS_DEPENDENT")
#define CONSTANT_A661_CENTER                              ("A661_CENTER")
#define CONSTANT_A661_CENTER_X                            ("A661_CENTER_X")
#define CONSTANT_A661_CENTER_XY                           ("A661_CENTER_XY")
#define CONSTANT_A661_CENTER_Y                            ("A661_CENTER_Y")
#define CONSTANT_A661_CIRCULAR                            ("A661_CIRCULAR")
#define CONSTANT_A661_CLAMPING                            ("A661_CLAMPING")
#define CONSTANT_A661_CLEAR                               ("A661_CLEAR")
#define CONSTANT_A661_CLOCKWISE                           ("A661_CLOCKWISE")
#define CONSTANT_A661_CLOSED                              ("A661_CLOSED")
#define CONSTANT_A661_COARSE_PAGE_DELTA                   ("A661_COARSE_PAGE_DELTA")
#define CONSTANT_A661_COLOR_INDEX                         ("A661_COLOR_INDEX")
#define CONSTANT_A661_COMPLETED                           ("A661_COMPLETED")
#define CONSTANT_A661_COUNTER_CLOCKWISE                   ("A661_COUNTER_CLOCKWISE")
#define CONSTANT_A661_CRP_LAT                             ("A661_CRP_LAT")
#define CONSTANT_A661_CRP_LAT_LONG                        ("A661_CRP_LAT_LONG")
#define CONSTANT_A661_CRP_LONG                            ("A661_CRP_LONG")
#define CONSTANT_A661_CURSOR_CLICKED                      ("A661_CURSOR_CLICKED")
#define CONSTANT_A661_CURSOR_DOUBLE_CLICKED               ("A661_CURSOR_DOUBLE_CLICKED")
#define CONSTANT_A661_CURSOR_INDEX                        ("A661_CURSOR_INDEX")
#define CONSTANT_A661_CURSOR_POS                          ("A661_CURSOR_POS")
#define CONSTANT_A661_CURSOR_POS_BYTE                     ("A661_CURSOR_POS_BYTE")
#define CONSTANT_A661_CURSOR_PRESSED                      ("A661_CURSOR_PRESSED")
#define CONSTANT_A661_CURSOR_RELEASED                     ("A661_CURSOR_RELEASED")
#define CONSTANT_A661_CURSOR_RIGHT_CLICKED                ("A661_CURSOR_RIGHT_CLICKED")
#define CONSTANT_A661_DEACTIVATE                          ("A661_DEACTIVATE")
#define CONSTANT_A661_DIRECTION                           ("A661_DIRECTION")
#define CONSTANT_A661_DONT_RUN                            ("A661_DONT_RUN")
#define CONSTANT_A661_DOWN                                ("A661_DOWN")
#define CONSTANT_A661_DRAW_LINE_TO_CURSOR                 ("A661_DRAW_LINE_TO_CURSOR")
#define CONSTANT_A661_EASING_IN                           ("A661_EASING_IN")
#define CONSTANT_A661_EASING_INOUT                        ("A661_EASING_INOUT")
#define CONSTANT_A661_EASING_OUT                          ("A661_EASING_OUT")
#define CONSTANT_A661_EDB_ALL_CHANGE                      ("A661_EDB_ALL_CHANGE")
#define CONSTANT_A661_EDB_CHANGE_CONFIRMED                ("A661_EDB_CHANGE_CONFIRMED")
#define CONSTANT_A661_EDB_OPEN_CLOSE                      ("A661_EDB_OPEN_CLOSE")
#define CONSTANT_A661_EDITING                             ("A661_EDITING")
#define CONSTANT_A661_EFFECTIVITY_ARRAY                   ("A661_EFFECTIVITY_ARRAY")
#define CONSTANT_A661_ENABLE                              ("A661_ENABLE")
#define CONSTANT_A661_ENABLE_ENTRY_ARRAY                  ("A661_ENABLE_ENTRY_ARRAY")
#define CONSTANT_A661_ENABLE_HANDLER                      ("A661_ENABLE_HANDLER")
#define CONSTANT_A661_END                                 ("A661_END")
#define CONSTANT_A661_END_ANGLE                           ("A661_END_ANGLE")
#define CONSTANT_A661_END_SCALED_VALUE                    ("A661_END_SCALED_VALUE")
#define CONSTANT_A661_END_VALUE                           ("A661_END_VALUE")
#define CONSTANT_A661_ENTRY_ARRAY                         ("A661_ENTRY_ARRAY")
#define CONSTANT_A661_ENTRY_POP_UP_ARRAY                  ("A661_ENTRY_POP_UP_ARRAY")
#define CONSTANT_A661_ENTRY_VALID                         ("A661_ENTRY_VALID")
#define CONSTANT_A661_ERROR                               ("A661_ERROR")
#define CONSTANT_A661_EVENT_FLAG                          ("A661_EVENT_FLAG")
#define CONSTANT_A661_EVENT_NONE                          ("A661_EVENT_NONE")
#define CONSTANT_A661_EXCLUDED_CIRCLE                     ("A661_EXCLUDED_CIRCLE")
#define CONSTANT_A661_EXCLUDED_RECTANGLE                  ("A661_EXCLUDED_RECTANGLE")
#define CONSTANT_A661_EXCLUDED_REGIONS_LIST               ("A661_EXCLUDED_REGIONS_LIST")
#define CONSTANT_A661_EXCLUDED_TRIANGLE                   ("A661_EXCLUDED_TRIANGLE")
#define CONSTANT_A661_FALSE                               ("A661_FALSE")
#define CONSTANT_A661_FILLED                              ("A661_FILLED")
#define CONSTANT_A661_FILLED_POLY_START                   ("A661_FILLED_POLY_START")
#define CONSTANT_A661_FILLED_POLY_START_INTERACTIVE       ("A661_FILLED_POLY_START_INTERACTIVE")
#define CONSTANT_A661_FILL_INDEX                          ("A661_FILL_INDEX")
#define CONSTANT_A661_FINE_PAGE_DELTA                     ("A661_FINE_PAGE_DELTA")
#define CONSTANT_A661_FIRST_ACCESS_ENTRY                  ("A661_FIRST_ACCESS_ENTRY")
#define CONSTANT_A661_FIRST_ACCESS_UA_ENTRY               ("A661_FIRST_ACCESS_UA_ENTRY")
#define CONSTANT_A661_FIRST_VISIBLE_ENTRY                 ("A661_FIRST_VISIBLE_ENTRY")
#define CONSTANT_A661_FONT                                ("A661_FONT")
#define CONSTANT_A661_FORMAT_STRING                       ("A661_FORMAT_STRING")
#define CONSTANT_A661_FRAME_X                             ("A661_FRAME_X")
#define CONSTANT_A661_FRAME_XY                            ("A661_FRAME_XY")
#define CONSTANT_A661_FRAME_Y                             ("A661_FRAME_Y")
#define CONSTANT_A661_FROM                                ("A661_FROM")
#define CONSTANT_A661_FROM_ANGLE                          ("A661_FROM_ANGLE")
#define CONSTANT_A661_FROM_SCALE_X                        ("A661_FROM_SCALE_X")
#define CONSTANT_A661_FROM_SCALE_XY                       ("A661_FROM_SCALE_XY")
#define CONSTANT_A661_FROM_SCALE_Y                        ("A661_FROM_SCALE_Y")
#define CONSTANT_A661_FROM_VALUE                          ("A661_FROM_VALUE")
#define CONSTANT_A661_FROM_VALUE_X                        ("A661_FROM_VALUE_X")
#define CONSTANT_A661_FROM_VALUE_XY                       ("A661_FROM_VALUE_XY")
#define CONSTANT_A661_FROM_VALUE_Y                        ("A661_FROM_VALUE_Y")
#define CONSTANT_A661_GBK                                 ("A661_GBK")
#define CONSTANT_A661_GESTURE_ABORT                       ("A661_GESTURE_ABORT")
#define CONSTANT_A661_GESTURE_CANDIDATE                   ("A661_GESTURE_CANDIDATE")
#define CONSTANT_A661_GESTURE_DOUBLE_TAP                  ("A661_GESTURE_DOUBLE_TAP")
#define CONSTANT_A661_GESTURE_DRAG                        ("A661_GESTURE_DRAG")
#define CONSTANT_A661_GESTURE_END                         ("A661_GESTURE_END")
#define CONSTANT_A661_GESTURE_FLICK                       ("A661_GESTURE_FLICK")
#define CONSTANT_A661_GESTURE_FLICK_DOWN                  ("A661_GESTURE_FLICK_DOWN")
#define CONSTANT_A661_GESTURE_FLICK_LEFT                  ("A661_GESTURE_FLICK_LEFT")
#define CONSTANT_A661_GESTURE_FLICK_RIGHT                 ("A661_GESTURE_FLICK_RIGHT")
#define CONSTANT_A661_GESTURE_FLICK_UP                    ("A661_GESTURE_FLICK_UP")
#define CONSTANT_A661_GESTURE_HOLD                        ("A661_GESTURE_HOLD")
#define CONSTANT_A661_GESTURE_PINCH                       ("A661_GESTURE_PINCH")
#define CONSTANT_A661_GESTURE_PRESS_AND_HOLD              ("A661_GESTURE_PRESS_AND_HOLD")
#define CONSTANT_A661_GESTURE_ROTATE                      ("A661_GESTURE_ROTATE")
#define CONSTANT_A661_GESTURE_START                       ("A661_GESTURE_START")
#define CONSTANT_A661_GESTURE_TAP                         ("A661_GESTURE_TAP")
#define CONSTANT_A661_GESTURE_UPDATE                      ("A661_GESTURE_UPDATE")
#define CONSTANT_A661_GRID                                ("A661_GRID")
#define CONSTANT_A661_HALO                                ("A661_HALO")
#define CONSTANT_A661_HIGHLIGHT_ONLY                      ("A661_HIGHLIGHT_ONLY")
#define CONSTANT_A661_HOME_X                              ("A661_HOME_X")
#define CONSTANT_A661_HOME_Y                              ("A661_HOME_Y")
#define CONSTANT_A661_HORIZONTAL                          ("A661_HORIZONTAL")
#define CONSTANT_A661_HORIZONTAL_SCROLL                   ("A661_HORIZONTAL_SCROLL")
#define CONSTANT_A661_HORZ_LINE                           ("A661_HORZ_LINE")
#define CONSTANT_A661_HYPHENATE                           ("A661_HYPHENATE")
#define CONSTANT_A661_INDEX_OF_FREQUENCY                  ("A661_INDEX_OF_FREQUENCY")
#define CONSTANT_A661_INITIAL_FOCUS                       ("A661_INITIAL_FOCUS")
#define CONSTANT_A661_INNER_RADIUS                        ("A661_INNER_RADIUS")
#define CONSTANT_A661_INNER_STATE_CHECK                   ("A661_INNER_STATE_CHECK")
#define CONSTANT_A661_INNER_STATE_TOGGLE                  ("A661_INNER_STATE_TOGGLE")
#define CONSTANT_A661_INSET_SIZE                          ("A661_INSET_SIZE")
#define CONSTANT_A661_IN_BOUNDS                           ("A661_IN_BOUNDS")
#define CONSTANT_A661_ITEM_SPACING                        ("A661_ITEM_SPACING")
#define CONSTANT_A661_ITEM_STYLE                          ("A661_ITEM_STYLE")
#define CONSTANT_A661_ITEM_SYNCHRONIZATION                ("A661_ITEM_SYNCHRONIZATION")
#define CONSTANT_A661_KEY_ARRAY                           ("A661_KEY_ARRAY")
#define CONSTANT_A661_LAYOUT                              ("A661_LAYOUT")
#define CONSTANT_A661_LEFT                                ("A661_LEFT")
#define CONSTANT_A661_LEFT_TO_RIGHT                       ("A661_LEFT_TO_RIGHT")
#define CONSTANT_A661_LEGEND                              ("A661_LEGEND")
#define CONSTANT_A661_LEGEND_ALIGNMENT                    ("A661_LEGEND_ALIGNMENT")
#define CONSTANT_A661_LEGEND_ANCHOR                       ("A661_LEGEND_ANCHOR")
#define CONSTANT_A661_LEGEND_ANCHOR_ROTATED               ("A661_LEGEND_ANCHOR_ROTATED")
#define CONSTANT_A661_LEGEND_AND_HIGHLIGHT                ("A661_LEGEND_AND_HIGHLIGHT")
#define CONSTANT_A661_LEGEND_AND_POPUP                    ("A661_LEGEND_AND_POPUP")
#define CONSTANT_A661_LEGEND_AND_POPUP_AND_HIGHLIGHT      ("A661_LEGEND_AND_POPUP_AND_HIGHLIGHT")
#define CONSTANT_A661_LEGEND_AREA_SIZE_X                  ("A661_LEGEND_AREA_SIZE_X")
#define CONSTANT_A661_LEGEND_COMBO                        ("A661_LEGEND_COMBO")
#define CONSTANT_A661_LEGEND_HIGHLIGHT                    ("A661_LEGEND_HIGHLIGHT")
#define CONSTANT_A661_LEGEND_ONLY                         ("A661_LEGEND_ONLY")
#define CONSTANT_A661_LEGEND_POP_UP                       ("A661_LEGEND_POP_UP")
#define CONSTANT_A661_LEGEND_POSITION                     ("A661_LEGEND_POSITION")
#define CONSTANT_A661_LEGEND_READOUT                      ("A661_LEGEND_READOUT")
#define CONSTANT_A661_LEGEND_READOUT_COMBO                ("A661_LEGEND_READOUT_COMBO")
#define CONSTANT_A661_LEGEND_READOUT_FORMAT_STRING        ("A661_LEGEND_READOUT_FORMAT_STRING")
#define CONSTANT_A661_LEGEND_REMOVED                      ("A661_LEGEND_REMOVED")
#define CONSTANT_A661_LINEAR                              ("A661_LINEAR")
#define CONSTANT_A661_LINE_ARC                            ("A661_LINE_ARC")
#define CONSTANT_A661_LINE_ARC_INTERACTIVE                ("A661_LINE_ARC_INTERACTIVE")
#define CONSTANT_A661_LINE_DELTA_X                        ("A661_LINE_DELTA_X")
#define CONSTANT_A661_LINE_DELTA_Y                        ("A661_LINE_DELTA_Y")
#define CONSTANT_A661_LINE_LENGTH                         ("A661_LINE_LENGTH")
#define CONSTANT_A661_LINE_SEGMENT                        ("A661_LINE_SEGMENT")
#define CONSTANT_A661_LINE_SEGMENT_INTERACTIVE            ("A661_LINE_SEGMENT_INTERACTIVE")
#define CONSTANT_A661_LINE_START                          ("A661_LINE_START")
#define CONSTANT_A661_LINE_START_INTERACTIVE              ("A661_LINE_START_INTERACTIVE")
#define CONSTANT_A661_LOOP_FORWARD                        ("A661_LOOP_FORWARD")
#define CONSTANT_A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK ("A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK")
#define CONSTANT_A661_LOOP_FORWARD_AND_BLANK              ("A661_LOOP_FORWARD_AND_BLANK")
#define CONSTANT_A661_LOOP_FORWARD_AND_RESET              ("A661_LOOP_FORWARD_AND_RESET")
#define CONSTANT_A661_LSB_MULTIPLE                        ("A661_LSB_MULTIPLE")
#define CONSTANT_A661_MAJOR_TICK_INTERVAL                 ("A661_MAJOR_TICK_INTERVAL")
#define CONSTANT_A661_MAJOR_TICK_REFERENCE                ("A661_MAJOR_TICK_REFERENCE")
#define CONSTANT_A661_MAPGRID_CELLSIZE                    ("A661_MAPGRID_CELLSIZE")
#define CONSTANT_A661_MAPGRID_OFFSET                      ("A661_MAPGRID_OFFSET")
#define CONSTANT_A661_MAP_SYNCHRONIZATION_NUMBER          ("A661_MAP_SYNCHRONIZATION_NUMBER")
#define CONSTANT_A661_MASK_ENABLED                        ("A661_MASK_ENABLED")
#define CONSTANT_A661_MASK_REFERENCE                      ("A661_MASK_REFERENCE")
#define CONSTANT_A661_MAX_VALUE                           ("A661_MAX_VALUE")
#define CONSTANT_A661_MDF_ABSOLUTE                        ("A661_MDF_ABSOLUTE")
#define CONSTANT_A661_MDF_BRG_DIST_ACHDG                  ("A661_MDF_BRG_DIST_ACHDG")
#define CONSTANT_A661_MDF_DIST_DIST                       ("A661_MDF_DIST_DIST")
#define CONSTANT_A661_MDF_DIST_DIST_FIX                   ("A661_MDF_DIST_DIST_FIX")
#define CONSTANT_A661_MDF_LAT_LONG                        ("A661_MDF_LAT_LONG")
#define CONSTANT_A661_MDF_LEGACY                          ("A661_MDF_LEGACY")
#define CONSTANT_A661_MDF_RELATIVE                        ("A661_MDF_RELATIVE")
#define CONSTANT_A661_MDF_X_DIST                          ("A661_MDF_X_DIST")
#define CONSTANT_A661_MDF_Y_ALT                           ("A661_MDF_Y_ALT")
#define CONSTANT_A661_MENU_HORIZONTAL                     ("A661_MENU_HORIZONTAL")
#define CONSTANT_A661_MINIMUM_SIZE                        ("A661_MINIMUM_SIZE")
#define CONSTANT_A661_MINMAX_VALUES                       ("A661_MINMAX_VALUES")
#define CONSTANT_A661_MINOR_TICK_MULTIPLE                 ("A661_MINOR_TICK_MULTIPLE")
#define CONSTANT_A661_MIN_VALUE                           ("A661_MIN_VALUE")
#define CONSTANT_A661_MULTI_STATE_VALUE                   ("A661_MULTI_STATE_VALUE")
#define CONSTANT_A661_NEGATIVE_STRING                     ("A661_NEGATIVE_STRING")
#define CONSTANT_A661_NEXT_FOCUSED_WIDGET                 ("A661_NEXT_FOCUSED_WIDGET")
#define CONSTANT_A661_NEXT_FOCUS_IN_APP_ID                ("A661_NEXT_FOCUS_IN_APP_ID")
#define CONSTANT_A661_NEXT_FOCUS_IN_ID                    ("A661_NEXT_FOCUS_IN_ID")
#define CONSTANT_A661_NEXT_FOCUS_IN_LAYER_ID              ("A661_NEXT_FOCUS_IN_LAYER_ID")
#define CONSTANT_A661_NORMAL                              ("A661_NORMAL")
#define CONSTANT_A661_NOT_CLAMPED                         ("A661_NOT_CLAMPED")
#define CONSTANT_A661_NOT_USED                            ("A661_NOT_USED")
#define CONSTANT_A661_NO_SHUFFLE                          ("A661_NO_SHUFFLE")
#define CONSTANT_A661_NO_SIZING                           ("A661_NO_SIZING")
#define CONSTANT_A661_NUMBER_OF_ENTRIES                   ("A661_NUMBER_OF_ENTRIES")
#define CONSTANT_A661_NUMBER_OF_KEYS                      ("A661_NUMBER_OF_KEYS")
#define CONSTANT_A661_NUMBER_OF_PICTURES                  ("A661_NUMBER_OF_PICTURES")
#define CONSTANT_A661_NUMBER_OF_STATES                    ("A661_NUMBER_OF_STATES")
#define CONSTANT_A661_NUMBER_OF_SYMBOLS                   ("A661_NUMBER_OF_SYMBOLS")
#define CONSTANT_A661_NUMBER_OF_TICS_COARSE               ("A661_NUMBER_OF_TICS_COARSE")
#define CONSTANT_A661_NUMBER_OF_TICS_FINE                 ("A661_NUMBER_OF_TICS_FINE")
#define CONSTANT_A661_NUMBER_OF_TOUCH_POINTS              ("A661_NUMBER_OF_TOUCH_POINTS")
#define CONSTANT_A661_NUMBER_OF_TURNS                     ("A661_NUMBER_OF_TURNS")
#define CONSTANT_A661_NUMBER_OF_UA_ENTRIES                ("A661_NUMBER_OF_UA_ENTRIES")
#define CONSTANT_A661_NUMBER_OF_VISIBLE_CHILDREN          ("A661_NUMBER_OF_VISIBLE_CHILDREN")
#define CONSTANT_A661_NUMERIC_MASK                        ("A661_NUMERIC_MASK")
#define CONSTANT_A661_OPENING_ENTRY                       ("A661_OPENING_ENTRY")
#define CONSTANT_A661_OPENING_MODE                        ("A661_OPENING_MODE")
#define CONSTANT_A661_OPEN_CENTERED                       ("A661_OPEN_CENTERED")
#define CONSTANT_A661_OPEN_DOWN                           ("A661_OPEN_DOWN")
#define CONSTANT_A661_OPEN_ENTRY_ARRAY                    ("A661_OPEN_ENTRY_ARRAY")
#define CONSTANT_A661_OPEN_UP                             ("A661_OPEN_UP")
#define CONSTANT_A661_OPERATOR_ADD_ASSIGN_ARG             ("A661_OPERATOR_ADD_ASSIGN_ARG")
#define CONSTANT_A661_OPERATOR_ADD_ASSIGN_SRC             ("A661_OPERATOR_ADD_ASSIGN_SRC")
#define CONSTANT_A661_OPERATOR_ASSIGNMENT                 ("A661_OPERATOR_ASSIGNMENT")
#define CONSTANT_A661_OPERATOR_CONSTANT                   ("A661_OPERATOR_CONSTANT")
#define CONSTANT_A661_OPERATOR_INVERSE                    ("A661_OPERATOR_INVERSE")
#define CONSTANT_A661_OPERATOR_LOGICAL_AND                ("A661_OPERATOR_LOGICAL_AND")
#define CONSTANT_A661_OPERATOR_LOGICAL_OR                 ("A661_OPERATOR_LOGICAL_OR")
#define CONSTANT_A661_OPERATOR_PRODUCT                    ("A661_OPERATOR_PRODUCT")
#define CONSTANT_A661_OPERATOR_PROD_ASSIGN_ARG            ("A661_OPERATOR_PROD_ASSIGN_ARG")
#define CONSTANT_A661_OPERATOR_PROD_ASSIGN_SRC            ("A661_OPERATOR_PROD_ASSIGN_SRC")
#define CONSTANT_A661_OPERATOR_SUB_ASSIGN_ARG             ("A661_OPERATOR_SUB_ASSIGN_ARG")
#define CONSTANT_A661_OPERATOR_SUB_ASSIGN_SRC             ("A661_OPERATOR_SUB_ASSIGN_SRC")
#define CONSTANT_A661_OPERATOR_SUM                        ("A661_OPERATOR_SUM")
#define CONSTANT_A661_ORIENTATION                         ("A661_ORIENTATION")
#define CONSTANT_A661_OUTER_RADIUS                        ("A661_OUTER_RADIUS")
#define CONSTANT_A661_OUT_OF_BOUNDS                       ("A661_OUT_OF_BOUNDS")
#define CONSTANT_A661_PAGE_CONTROL_POSITION               ("A661_PAGE_CONTROL_POSITION")
#define CONSTANT_A661_PAGE_DELTA_X                        ("A661_PAGE_DELTA_X")
#define CONSTANT_A661_PAGE_DELTA_Y                        ("A661_PAGE_DELTA_Y")
#define CONSTANT_A661_PAGE_WRAP_TYPE                      ("A661_PAGE_WRAP_TYPE")
#define CONSTANT_A661_PARALLEL                            ("A661_PARALLEL")
#define CONSTANT_A661_PARAM_BUFFER_INDEX                  ("A661_PARAM_BUFFER_INDEX")
#define CONSTANT_A661_PARENT_ENTRY_ARRAY                  ("A661_PARENT_ENTRY_ARRAY")
#define CONSTANT_A661_PARKING_LINE_END                    ("A661_PARKING_LINE_END")
#define CONSTANT_A661_PARKING_LINE_START                  ("A661_PARKING_LINE_START")
#define CONSTANT_A661_PICTURE_ARRAY                       ("A661_PICTURE_ARRAY")
#define CONSTANT_A661_PICTURE_ENTRY_ARRAY                 ("A661_PICTURE_ENTRY_ARRAY")
#define CONSTANT_A661_PICTURE_POSITION                    ("A661_PICTURE_POSITION")
#define CONSTANT_A661_PICTURE_REFERENCE                   ("A661_PICTURE_REFERENCE")
#define CONSTANT_A661_PIX_FMT_COLOR_INDEXED_8             ("A661_PIX_FMT_COLOR_INDEXED_8")
#define CONSTANT_A661_PIX_FMT_LUMINANCE_ALPHA_8           ("A661_PIX_FMT_LUMINANCE_ALPHA_8")
#define CONSTANT_A661_PIX_FMT_RGBA_8                      ("A661_PIX_FMT_RGBA_8")
#define CONSTANT_A661_PLAY                                ("A661_PLAY")
#define CONSTANT_A661_POPUP_AND_HIGHLIGHT                 ("A661_POPUP_AND_HIGHLIGHT")
#define CONSTANT_A661_POPUP_ONLY                          ("A661_POPUP_ONLY")
#define CONSTANT_A661_POPUP_POS_X                         ("A661_POPUP_POS_X")
#define CONSTANT_A661_POPUP_POS_Y                         ("A661_POPUP_POS_Y")
#define CONSTANT_A661_POPUP_SIZE_X                        ("A661_POPUP_SIZE_X")
#define CONSTANT_A661_POPUP_SIZE_Y                        ("A661_POPUP_SIZE_Y")
#define CONSTANT_A661_POSITIVE_STRING                     ("A661_POSITIVE_STRING")
#define CONSTANT_A661_POS_X                               ("A661_POS_X")
#define CONSTANT_A661_POS_X2                              ("A661_POS_X2")
#define CONSTANT_A661_POS_X3                              ("A661_POS_X3")
#define CONSTANT_A661_POS_XY                              ("A661_POS_XY")
#define CONSTANT_A661_POS_XY2                             ("A661_POS_XY2")
#define CONSTANT_A661_POS_XY3                             ("A661_POS_XY3")
#define CONSTANT_A661_POS_Y                               ("A661_POS_Y")
#define CONSTANT_A661_POS_Y2                              ("A661_POS_Y2")
#define CONSTANT_A661_POS_Y3                              ("A661_POS_Y3")
#define CONSTANT_A661_PRESERVE_CHARACTER                  ("A661_PRESERVE_CHARACTER")
#define CONSTANT_A661_PRESERVE_WORD                       ("A661_PRESERVE_WORD")
#define CONSTANT_A661_PROJECTION_REFERENCE                ("A661_PROJECTION_REFERENCE")
#define CONSTANT_A661_PRP_LAT                             ("A661_PRP_LAT")
#define CONSTANT_A661_PRP_LAT_LONG                        ("A661_PRP_LAT_LONG")
#define CONSTANT_A661_PRP_LONG                            ("A661_PRP_LONG")
#define CONSTANT_A661_PRP_SCREEN_X                        ("A661_PRP_SCREEN_X")
#define CONSTANT_A661_PRP_SCREEN_XY                       ("A661_PRP_SCREEN_XY")
#define CONSTANT_A661_PRP_SCREEN_Y                        ("A661_PRP_SCREEN_Y")
#define CONSTANT_A661_PRP_X                               ("A661_PRP_X")
#define CONSTANT_A661_PRP_XY                              ("A661_PRP_XY")
#define CONSTANT_A661_PRP_Y                               ("A661_PRP_Y")
#define CONSTANT_A661_RADIUS                              ("A661_RADIUS")
#define CONSTANT_A661_RANGE                               ("A661_RANGE")
#define CONSTANT_A661_RANGE_X                             ("A661_RANGE_X")
#define CONSTANT_A661_RANGE_XY                            ("A661_RANGE_XY")
#define CONSTANT_A661_RANGE_Y                             ("A661_RANGE_Y")
#define CONSTANT_A661_REFRESH                             ("A661_REFRESH")
#define CONSTANT_A661_REF_POS_X                           ("A661_REF_POS_X")
#define CONSTANT_A661_REF_POS_XY                          ("A661_REF_POS_XY")
#define CONSTANT_A661_REF_POS_Y                           ("A661_REF_POS_Y")
#define CONSTANT_A661_REORDER                             ("A661_REORDER")
#define CONSTANT_A661_REPORT_ALL                          ("A661_REPORT_ALL")
#define CONSTANT_A661_REPORT_ALL_WITH_CANDIDATE           ("A661_REPORT_ALL_WITH_CANDIDATE")
#define CONSTANT_A661_REPORT_ON_CDS                       ("A661_REPORT_ON_CDS")
#define CONSTANT_A661_REPORT_ON_COMPLETION                ("A661_REPORT_ON_COMPLETION")
#define CONSTANT_A661_REPORT_ON_TRANSITION                ("A661_REPORT_ON_TRANSITION")
#define CONSTANT_A661_REPORT_ON_TRANSITION_WITH_CANDIDATE ("A661_REPORT_ON_TRANSITION_WITH_CANDIDATE")
#define CONSTANT_A661_REP_DEFAULT                         ("A661_REP_DEFAULT")
#define CONSTANT_A661_REP_FOCUS                           ("A661_REP_FOCUS")
#define CONSTANT_A661_REP_HIGHLIGHT                       ("A661_REP_HIGHLIGHT")
#define CONSTANT_A661_REP_STANDARD                        ("A661_REP_STANDARD")
#define CONSTANT_A661_RIGHT                               ("A661_RIGHT")
#define CONSTANT_A661_RIGHT_TO_LEFT                       ("A661_RIGHT_TO_LEFT")
#define CONSTANT_A661_ROTATION_ANGLE                      ("A661_ROTATION_ANGLE")
#define CONSTANT_A661_RUN                                 ("A661_RUN")
#define CONSTANT_A661_RUN_ONCE                            ("A661_RUN_ONCE")
#define CONSTANT_A661_SAME_LEVEL                          ("A661_SAME_LEVEL")
#define CONSTANT_A661_SCALE_X                             ("A661_SCALE_X")
#define CONSTANT_A661_SCALE_XY                            ("A661_SCALE_XY")
#define CONSTANT_A661_SCALE_Y                             ("A661_SCALE_Y")
#define CONSTANT_A661_SCREEN_RANGE                        ("A661_SCREEN_RANGE")
#define CONSTANT_A661_SELECTED                            ("A661_SELECTED")
#define CONSTANT_A661_SELECTED_ENTRY                      ("A661_SELECTED_ENTRY")
#define CONSTANT_A661_SELECTED_ENTRY_ARRAY                ("A661_SELECTED_ENTRY_ARRAY")
#define CONSTANT_A661_SELECTING_HEIGHT                    ("A661_SELECTING_HEIGHT")
#define CONSTANT_A661_SELECTING_WIDTH                     ("A661_SELECTING_WIDTH")
#define CONSTANT_A661_SELECT_ALL                          ("A661_SELECT_ALL")
#define CONSTANT_A661_SEQUENTIAL                          ("A661_SEQUENTIAL")
#define CONSTANT_A661_SEQUENTIAL_FROM                     ("A661_SEQUENTIAL_FROM")
#define CONSTANT_A661_SHEET_SIZE_X                        ("A661_SHEET_SIZE_X")
#define CONSTANT_A661_SHEET_SIZE_Y                        ("A661_SHEET_SIZE_Y")
#define CONSTANT_A661_SHIFT_FIRST_VISIBLE_ENTRY           ("A661_SHIFT_FIRST_VISIBLE_ENTRY")
#define CONSTANT_A661_SHOW_FAIL                           ("A661_SHOW_FAIL")
#define CONSTANT_A661_SHOW_MAJOR_TICKS                    ("A661_SHOW_MAJOR_TICKS")
#define CONSTANT_A661_SHUFFLE_DOWN                        ("A661_SHUFFLE_DOWN")
#define CONSTANT_A661_SHUFFLE_LEFT                        ("A661_SHUFFLE_LEFT")
#define CONSTANT_A661_SHUFFLE_RIGHT                       ("A661_SHUFFLE_RIGHT")
#define CONSTANT_A661_SHUFFLE_TO_FIT_MODE                 ("A661_SHUFFLE_TO_FIT_MODE")
#define CONSTANT_A661_SHUFFLE_UP                          ("A661_SHUFFLE_UP")
#define CONSTANT_A661_SIZE_BOTTOM_UP                      ("A661_SIZE_BOTTOM_UP")
#define CONSTANT_A661_SIZE_LEFT_TO_RIGHT                  ("A661_SIZE_LEFT_TO_RIGHT")
#define CONSTANT_A661_SIZE_RIGHT_TO_LEFT                  ("A661_SIZE_RIGHT_TO_LEFT")
#define CONSTANT_A661_SIZE_TOP_DOWN                       ("A661_SIZE_TOP_DOWN")
#define CONSTANT_A661_SIZE_TO_FIT_MODE                    ("A661_SIZE_TO_FIT_MODE")
#define CONSTANT_A661_SIZE_X                              ("A661_SIZE_X")
#define CONSTANT_A661_SIZE_XY                             ("A661_SIZE_XY")
#define CONSTANT_A661_SIZE_Y                              ("A661_SIZE_Y")
#define CONSTANT_A661_SOURCE_DX                           ("A661_SOURCE_DX")
#define CONSTANT_A661_SOURCE_DXDY                         ("A661_SOURCE_DXDY")
#define CONSTANT_A661_SOURCE_DY                           ("A661_SOURCE_DY")
#define CONSTANT_A661_SOURCE_REF                          ("A661_SOURCE_REF")
#define CONSTANT_A661_SOURCE_WIDGET_PARAM                 ("A661_SOURCE_WIDGET_PARAM")
#define CONSTANT_A661_SOURCE_X                            ("A661_SOURCE_X")
#define CONSTANT_A661_SOURCE_XY                           ("A661_SOURCE_XY")
#define CONSTANT_A661_SOURCE_Y                            ("A661_SOURCE_Y")
#define CONSTANT_A661_STAR                                ("A661_STAR")
#define CONSTANT_A661_START_ANGLE                         ("A661_START_ANGLE")
#define CONSTANT_A661_START_SCALED_VALUE                  ("A661_START_SCALED_VALUE")
#define CONSTANT_A661_START_VALUE                         ("A661_START_VALUE")
#define CONSTANT_A661_STOP_BACKWARD                       ("A661_STOP_BACKWARD")
#define CONSTANT_A661_STOP_FORWARD                        ("A661_STOP_FORWARD")
#define CONSTANT_A661_STRING                              ("A661_STRING")
#define CONSTANT_A661_STRING_ALTERNATE                    ("A661_STRING_ALTERNATE")
#define CONSTANT_A661_STRING_ARRAY                        ("A661_STRING_ARRAY")
#define CONSTANT_A661_STYLE_SET                           ("A661_STYLE_SET")
#define CONSTANT_A661_STYLE_SET_ARRAY                     ("A661_STYLE_SET_ARRAY")
#define CONSTANT_A661_STYLE_SET_ARRAY_SIZE                ("A661_STYLE_SET_ARRAY_SIZE")
#define CONSTANT_A661_STYLE_SET_DEFAULT                   ("A661_STYLE_SET_DEFAULT")
#define CONSTANT_A661_SYMBOL_CIRCLE                       ("A661_SYMBOL_CIRCLE")
#define CONSTANT_A661_SYMBOL_CIRCLE_INTERACTIVE           ("A661_SYMBOL_CIRCLE_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_ENTRY_ARRAY                  ("A661_SYMBOL_ENTRY_ARRAY")
#define CONSTANT_A661_SYMBOL_GENERIC                      ("A661_SYMBOL_GENERIC")
#define CONSTANT_A661_SYMBOL_GENERIC_INTERACTIVE          ("A661_SYMBOL_GENERIC_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_OVAL                         ("A661_SYMBOL_OVAL")
#define CONSTANT_A661_SYMBOL_OVAL_INTERACTIVE             ("A661_SYMBOL_OVAL_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_PARKABLE                     ("A661_SYMBOL_PARKABLE")
#define CONSTANT_A661_SYMBOL_PARKABLE_INTERACTIVE         ("A661_SYMBOL_PARKABLE_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_POSITION                     ("A661_SYMBOL_POSITION")
#define CONSTANT_A661_SYMBOL_RECTANGLE                    ("A661_SYMBOL_RECTANGLE")
#define CONSTANT_A661_SYMBOL_RECTANGLE_INTERACTIVE        ("A661_SYMBOL_RECTANGLE_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_REFERENCE                    ("A661_SYMBOL_REFERENCE")
#define CONSTANT_A661_SYMBOL_ROTATED                      ("A661_SYMBOL_ROTATED")
#define CONSTANT_A661_SYMBOL_ROTATED_INTERACTIVE          ("A661_SYMBOL_ROTATED_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_ROTATED_PARKABLE             ("A661_SYMBOL_ROTATED_PARKABLE")
#define CONSTANT_A661_SYMBOL_ROTATED_PARKABLE_INTERACTIVE ("A661_SYMBOL_ROTATED_PARKABLE_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_RUNWAY                       ("A661_SYMBOL_RUNWAY")
#define CONSTANT_A661_SYMBOL_RUNWAY_INTERACTIVE           ("A661_SYMBOL_RUNWAY_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_TARGET                       ("A661_SYMBOL_TARGET")
#define CONSTANT_A661_SYMBOL_TARGET_INTERACTIVE           ("A661_SYMBOL_TARGET_INTERACTIVE")
#define CONSTANT_A661_SYMBOL_TARGET_PARKABLE              ("A661_SYMBOL_TARGET_PARKABLE")
#define CONSTANT_A661_SYMBOL_TARGET_PARKABLE_INTERACTIVE  ("A661_SYMBOL_TARGET_PARKABLE_INTERACTIVE")
#define CONSTANT_A661_SYNC_FRAME                          ("A661_SYNC_FRAME")
#define CONSTANT_A661_TAB_DIRECTION_WIDGET_ID_ARRAY       ("A661_TAB_DIRECTION_WIDGET_ID_ARRAY")
#define CONSTANT_A661_TAB_POSITION                        ("A661_TAB_POSITION")
#define CONSTANT_A661_TARGET_WIDGET_ID                    ("A661_TARGET_WIDGET_ID")
#define CONSTANT_A661_TICS_COARSE                         ("A661_TICS_COARSE")
#define CONSTANT_A661_TICS_COARSE_8BYTE                   ("A661_TICS_COARSE_8BYTE")
#define CONSTANT_A661_TICS_COARSE_ARRAY                   ("A661_TICS_COARSE_ARRAY")
#define CONSTANT_A661_TICS_FINE                           ("A661_TICS_FINE")
#define CONSTANT_A661_TICS_FINE_8BYTE                     ("A661_TICS_FINE_8BYTE")
#define CONSTANT_A661_TICS_FINE_ARRAY                     ("A661_TICS_FINE_ARRAY")
#define CONSTANT_A661_TO                                  ("A661_TO")
#define CONSTANT_A661_TOP                                 ("A661_TOP")
#define CONSTANT_A661_TOP_CENTER                          ("A661_TOP_CENTER")
#define CONSTANT_A661_TOP_LEFT                            ("A661_TOP_LEFT")
#define CONSTANT_A661_TOP_RIGHT                           ("A661_TOP_RIGHT")
#define CONSTANT_A661_TOP_TO_BOTTOM                       ("A661_TOP_TO_BOTTOM")
#define CONSTANT_A661_TOUCH_ABORT                         ("A661_TOUCH_ABORT")
#define CONSTANT_A661_TOUCH_DOWN                          ("A661_TOUCH_DOWN")
#define CONSTANT_A661_TOUCH_MOVE                          ("A661_TOUCH_MOVE")
#define CONSTANT_A661_TOUCH_MOVE_OUTSIDE                  ("A661_TOUCH_MOVE_OUTSIDE")
#define CONSTANT_A661_TOUCH_UP                            ("A661_TOUCH_UP")
#define CONSTANT_A661_TO_ANGLE                            ("A661_TO_ANGLE")
#define CONSTANT_A661_TO_BACK                             ("A661_TO_BACK")
#define CONSTANT_A661_TO_FRONT                            ("A661_TO_FRONT")
#define CONSTANT_A661_TO_SCALE_X                          ("A661_TO_SCALE_X")
#define CONSTANT_A661_TO_SCALE_XY                         ("A661_TO_SCALE_XY")
#define CONSTANT_A661_TO_SCALE_Y                          ("A661_TO_SCALE_Y")
#define CONSTANT_A661_TO_VALUE                            ("A661_TO_VALUE")
#define CONSTANT_A661_TO_VALUE_X                          ("A661_TO_VALUE_X")
#define CONSTANT_A661_TO_VALUE_XY                         ("A661_TO_VALUE_XY")
#define CONSTANT_A661_TO_VALUE_Y                          ("A661_TO_VALUE_Y")
#define CONSTANT_A661_TRANSLATION_X                       ("A661_TRANSLATION_X")
#define CONSTANT_A661_TRANSLATION_XY                      ("A661_TRANSLATION_XY")
#define CONSTANT_A661_TRANSLATION_Y                       ("A661_TRANSLATION_Y")
#define CONSTANT_A661_TREE_NODE_STYLE_ARRAY               ("A661_TREE_NODE_STYLE_ARRAY")
#define CONSTANT_A661_TRIANGLE_END                        ("A661_TRIANGLE_END")
#define CONSTANT_A661_TRIANGLE_END_DOUBLE                 ("A661_TRIANGLE_END_DOUBLE")
#define CONSTANT_A661_TRIANGLE_END_DOUBLE_INTERACTIVE     ("A661_TRIANGLE_END_DOUBLE_INTERACTIVE")
#define CONSTANT_A661_TRIANGLE_END_INTERACTIVE            ("A661_TRIANGLE_END_INTERACTIVE")
#define CONSTANT_A661_TRIANGLE_FAN_START                  ("A661_TRIANGLE_FAN_START")
#define CONSTANT_A661_TRIANGLE_FAN_START_INTERACTIVE      ("A661_TRIANGLE_FAN_START_INTERACTIVE")
#define CONSTANT_A661_TRIANGLE_SEGMENT                    ("A661_TRIANGLE_SEGMENT")
#define CONSTANT_A661_TRIANGLE_SEGMENT_DOUBLE             ("A661_TRIANGLE_SEGMENT_DOUBLE")
#define CONSTANT_A661_TRIANGLE_SEGMENT_DOUBLE_INTERACTIVE ("A661_TRIANGLE_SEGMENT_DOUBLE_INTERACTIVE")
#define CONSTANT_A661_TRIANGLE_SEGMENT_INTERACTIVE        ("A661_TRIANGLE_SEGMENT_INTERACTIVE")
#define CONSTANT_A661_TRIANGLE_STRIP_START                ("A661_TRIANGLE_STRIP_START")
#define CONSTANT_A661_TRIANGLE_STRIP_START_INTERACTIVE    ("A661_TRIANGLE_STRIP_START_INTERACTIVE")
#define CONSTANT_A661_TRUE                                ("A661_TRUE")
#define CONSTANT_A661_TRUE_WITH_VALIDATION                ("A661_TRUE_WITH_VALIDATION")
#define CONSTANT_A661_UA_POSITION_FLAG                    ("A661_UA_POSITION_FLAG")
#define CONSTANT_A661_UNSELECTED                          ("A661_UNSELECTED")
#define CONSTANT_A661_UNSELECT_ALL                        ("A661_UNSELECT_ALL")
#define CONSTANT_A661_UP                                  ("A661_UP")
#define CONSTANT_A661_UTF8                                ("A661_UTF8")
#define CONSTANT_A661_VALIDATION                          ("A661_VALIDATION")
#define CONSTANT_A661_VALIDATION_AND_WHEEL                ("A661_VALIDATION_AND_WHEEL")
#define CONSTANT_A661_VALUE                               ("A661_VALUE")
#define CONSTANT_A661_VALUE_8BYTE                         ("A661_VALUE_8BYTE")
#define CONSTANT_A661_VERTEX_RENDER_END                   ("A661_VERTEX_RENDER_END")
#define CONSTANT_A661_VERTEX_RENDER_LINES                 ("A661_VERTEX_RENDER_LINES")
#define CONSTANT_A661_VERTEX_RENDER_LINE_LOOP             ("A661_VERTEX_RENDER_LINE_LOOP")
#define CONSTANT_A661_VERTEX_RENDER_POLYLINE              ("A661_VERTEX_RENDER_POLYLINE")
#define CONSTANT_A661_VERTEX_RENDER_SEGMENT               ("A661_VERTEX_RENDER_SEGMENT")
#define CONSTANT_A661_VERTEX_RENDER_START                 ("A661_VERTEX_RENDER_START")
#define CONSTANT_A661_VERTEX_RENDER_TRIANGLE              ("A661_VERTEX_RENDER_TRIANGLE")
#define CONSTANT_A661_VERTEX_RENDER_TRIANGLE_FAN          ("A661_VERTEX_RENDER_TRIANGLE_FAN")
#define CONSTANT_A661_VERTEX_RENDER_TRIANGLE_STRIP        ("A661_VERTEX_RENDER_TRIANGLE_STRIP")
#define CONSTANT_A661_VERTICAL                            ("A661_VERTICAL")
#define CONSTANT_A661_VERTICAL_SCROLL                     ("A661_VERTICAL_SCROLL")
#define CONSTANT_A661_VERTICES_ARRAY                      ("A661_VERTICES_ARRAY")
#define CONSTANT_A661_VERT_LINE                           ("A661_VERT_LINE")
#define CONSTANT_A661_VISIBLE                             ("A661_VISIBLE")
#define CONSTANT_A661_VISIBLE_CHILD                       ("A661_VISIBLE_CHILD")
#define CONSTANT_A661_VISIBLE_CHILD_INDEX                 ("A661_VISIBLE_CHILD_INDEX")
#define CONSTANT_A661_WHEEL                               ("A661_WHEEL")
#define CONSTANT_A661_WORD_WRAP                           ("A661_WORD_WRAP")
#define CONSTANT_A661_WRAP_BOTH                           ("A661_WRAP_BOTH")
#define CONSTANT_A661_WRAP_FIRST_TO_LAST                  ("A661_WRAP_FIRST_TO_LAST")
#define CONSTANT_A661_WRAP_LAST_TO_FIRST                  ("A661_WRAP_LAST_TO_FIRST")
#define CONSTANT_A661_WRAP_NONE                           ("A661_WRAP_NONE")
#define CONSTANT_A661_WRAP_STYLE                          ("A661_WRAP_STYLE")

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
    const char_t*        name;             ///< Property name, or NULL for an unused pad or alignment
    BinaryFieldType_t    type;             ///< Binary encoding
    uint32_t             byteCount;        ///< Encoded bytes of one value; zero when the value sizes itself
    const char_t*        countName;        ///< For an array, name of the parameter holding its entry count; NULL for a single value
    uint32_t             elementsPerEntry; ///< BINARY_SINGLE_VALUE, BINARY_ONE_PER_ENTRY, or BINARY_PAIR_PER_ENTRY
    const char_t* const* values;           ///< Constant names the table lists for this parameter, or NULL
    uint32_t             valueCount;       ///< Number of listed constant names
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

/*************************************************************************************************/
/**
 * \brief Definition File header (Table 4.5.3.2-2).
 */
#pragma pack(push, 1)
typedef struct BinaryHeader_s
{
    uint16_t magicNumber;       ///< A661_DF_MAGIC_NUMBER
    byte_t   libraryVersion;    ///< LibraryVersion
    byte_t   supplementVersion; ///< SuppVersion
    uint16_t appliIdent;        ///< AppliIdent
    uint16_t oemDataSize;       ///< Bytes of OEM data after the header
} BinaryHeader_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of a layer block (Table 4.5.3.2-4); its commands and end code follow.
 */
#pragma pack(push, 1)
typedef struct BinaryLayerBlock_s
{
    byte_t   blockCode;     ///< A661_BEGIN_LAYER_BLOCK
    byte_t   layerIdent;    ///< LayerIdent
    uint16_t contextNumber; ///< ContextNumber
    uint32_t blockSize;     ///< Block Size, including this header and the end code
} BinaryLayerBlock_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of a symbol or animation law block (Tables 4.5.3.2-5 and 9.2-1).
 */
#pragma pack(push, 1)
typedef struct BinarySizedBlock_s
{
    byte_t   blockCode;    ///< A661_BEGIN_SYMBOL_BLOCK or A661_BEGIN_ANIMATION_LAW_BLOCK
    byte_t   unusedPad[3]; ///< UnusedPad
    uint32_t blockSize;    ///< Block Size, including this header and the end code
} BinarySizedBlock_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Complete charset encoding block (Table 4.5.3.2-6).
 */
#pragma pack(push, 1)
typedef struct BinaryCharsetBlock_s
{
    byte_t   blockCode; ///< A661_BEGIN_CHARSET_ENCODING_BLOCK
    byte_t   unusedPad; ///< UnusedPad
    uint16_t encoding;  ///< A661_Charset_Encoding value (Table 4.6-13)
    byte_t   endCode;   ///< A661_END_CHARSET_ENCODING_BLOCK
    byte_t   endPad[3]; ///< UnusedPad
} BinaryCharsetBlock_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief End code that closes a block, a picture, or the file.
 */
#pragma pack(push, 1)
typedef struct BinaryBlockEnd_s
{
    byte_t endCode;      ///< Block end code or A661_DF_FOOTER
    byte_t unusedPad[3]; ///< UnusedPad
} BinaryBlockEnd_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of a picture block (Table 7-1); the color table and pictures follow.
 */
#pragma pack(push, 1)
typedef struct BinaryPictureBlock_s
{
    byte_t blockCode;    ///< A661_BEGIN_PICTURE_BLOCK
    byte_t pixelFormat;  ///< Table 7-3 format of the pictures
    byte_t pictureCount; ///< Number of pictures
    byte_t colorCount;   ///< Number of color table entries
    byte_t colorFormat;  ///< Table 7-3 format of the color table entries
    byte_t unusedPad[3]; ///< UnusedPad
} BinaryPictureBlock_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of one picture in a picture block (Table 7-2); its pixels and end code follow.
 */
#pragma pack(push, 1)
typedef struct BinaryPicture_s
{
    byte_t   pictureCode; ///< A661_BEGIN_PICTURE
    byte_t   unusedPad;   ///< UnusedPad
    uint16_t reference;   ///< Picture reference
    uint16_t width;       ///< Width in pixels
    uint16_t height;      ///< Height in pixels
} BinaryPicture_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Start of every command, and all of A661_CMD_EXTEND (Table 4.5.3.3.2); parameters follow.
 */
#pragma pack(push, 1)
typedef struct BinaryCommand_s
{
    uint16_t commandCode; ///< A661_CMD_CREATE or A661_CMD_EXTEND
    uint16_t commandSize; ///< CommandSize, including this header and the padding
    uint16_t type;        ///< WidgetType or ExtensionType
} BinaryCommand_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of A661_CMD_CREATE (Table 4.5.3.3.1); parameters follow.
 */
#pragma pack(push, 1)
typedef struct BinaryCreateCommand_s
{
    BinaryCommand_t command;     ///< Command code, CommandSize, and WidgetType
    uint16_t        widgetIdent; ///< WidgetIdent
    uint16_t        parentIdent; ///< ParentIdent, or zero for a widget directly under its layer
} BinaryCreateCommand_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of one StaticParamBuffer entry (Table 8.6.5-3); the bytes and their alignment follow.
 */
#pragma pack(push, 1)
typedef struct BinarySizedBytes_s
{
    uint16_t paramBufferSize; ///< ParamBufferSize, the bytes that follow before alignment
    uint16_t unusedPad;       ///< UnusedPad
} BinarySizedBytes_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of one LayerReqResult entry (Table 3.9.17-6); the request follows.
 */
#pragma pack(push, 1)
typedef struct BinaryLayerRequest_s
{
    byte_t   layerIdent;  ///< LayerIdent
    byte_t   unusedPad;   ///< UnusedPad
    uint16_t commandSize; ///< CommandSize of the request that follows
} BinaryLayerRequest_t;
#pragma pack(pop)

/*************************************************************************************************/
/**
 * \brief Header of an excluded region structure (Table 8.6.7-3); the region follows.
 */
#pragma pack(push, 1)
typedef struct BinaryRegionHeader_s
{
    byte_t   excludedItemIndex; ///< ExcludedItemIndex
    byte_t   regionType;        ///< ExcludedRegionType
    uint16_t unusedPad;         ///< UnusedPad
} BinaryRegionHeader_t;
#pragma pack(pop)

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
    {CONSTANT_A661_ASCII_EXTENDED, 1},
    {CONSTANT_A661_UTF8,           2},
    {CONSTANT_A661_GBK,            3}
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
static const uint32_t c_pixelFormatBytes[] = {4, 2, 1};

/*************************************************************************************************/
/**
 * \brief Constant names an enumerated parameter accepts, as its Creation Structure table lists them.
 *
 * \par Details:
 *  For example, Enable (Table 3.3.1-2) accepts A661_FALSE, A661_TRUE, and
 *  A661_TRUE_WITH_VALIDATION. A field points at its list through values and
 *  valueCount; fields whose lists match in content and order share one array.
 *  The library only hands the list out through Arinc661_Definition_GetParameter,
 *  for the editor's drop-down and default value. The numbers behind the names
 *  come from c_binaryConstants.
 */
static const char_t* const c_enableValues[] =
{
    CONSTANT_A661_FALSE,
    CONSTANT_A661_TRUE,
    CONSTANT_A661_TRUE_WITH_VALIDATION
};

static const char_t* const c_booleanValues[] =
{
    CONSTANT_A661_FALSE,
    CONSTANT_A661_TRUE
};

static const char_t* const c_rotationDirectionValues[] =
{
    CONSTANT_A661_CLOCKWISE,
    CONSTANT_A661_COUNTER_CLOCKWISE
};

static const char_t* const c_checkButtonStateValues[] =
{
    CONSTANT_A661_SELECTED,
    CONSTANT_A661_UNSELECTED
};

static const char_t* const c_buttonAlignmentValues[] =
{
    CONSTANT_A661_LEFT,
    CONSTANT_A661_CENTER,
    CONSTANT_A661_RIGHT
};

static const char_t* const c_checkButtonPicturePositionValues[] =
{
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT
};

static const char_t* const c_comboBoxOpeningModeValues[] =
{
    CONSTANT_A661_OPEN_UP,
    CONSTANT_A661_OPEN_CENTERED,
    CONSTANT_A661_OPEN_DOWN
};

static const char_t* const c_editBoxReportAllChangesValues[] =
{
    CONSTANT_A661_EDB_CHANGE_CONFIRMED,
    CONSTANT_A661_EDB_ALL_CHANGE,
    CONSTANT_A661_EDB_OPEN_CLOSE
};

static const char_t* const c_editBoxAlignmentValues[] =
{
    CONSTANT_A661_CENTER,
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT
};

static const char_t* const c_positionReportModeValues[] =
{
    CONSTANT_A661_REPORT_ON_TRANSITION,
    CONSTANT_A661_REPORT_ALL
};

static const char_t* const c_clampingValues[] =
{
    CONSTANT_A661_NOT_CLAMPED,
    CONSTANT_A661_BOTH_CLAMPED
};

static const char_t* const c_scrollBarPositionValues[] =
{
    CONSTANT_A661_TOP,
    CONSTANT_A661_BOTTOM,
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_ABSENT
};

static const char_t* const c_legendPositionValues[] =
{
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_TOP,
    CONSTANT_A661_BOTTOM
};

static const char_t* const c_editBoxNumericReportAllChangesValues[] =
{
    CONSTANT_A661_EDB_CHANGE_CONFIRMED,
    CONSTANT_A661_EDB_OPEN_CLOSE,
    CONSTANT_A661_EDB_ALL_CHANGE
};

static const char_t* const c_haloValues[] =
{
    CONSTANT_A661_FALSE,
    CONSTANT_A661_TRUE,
    CONSTANT_A661_SAME_LEVEL
};

static const char_t* const c_labelAlignmentValues[] =
{
    CONSTANT_A661_BOTTOM_CENTER,
    CONSTANT_A661_BOTTOM_LEFT,
    CONSTANT_A661_BOTTOM_RIGHT,
    CONSTANT_A661_CENTER,
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_TOP_CENTER,
    CONSTANT_A661_TOP_LEFT,
    CONSTANT_A661_TOP_RIGHT
};

static const char_t* const c_mapDataFormatValues[] =
{
    CONSTANT_A661_MDF_BRG_DIST_ACHDG,
    CONSTANT_A661_MDF_DIST_DIST,
    CONSTANT_A661_MDF_LAT_LONG,
    CONSTANT_A661_MDF_DIST_DIST_FIX,
    CONSTANT_A661_MDF_LEGACY
};

static const char_t* const c_eventFlagValues[] =
{
    CONSTANT_A661_EVENT_NONE,
    CONSTANT_A661_VALIDATION,
    CONSTANT_A661_VALIDATION_AND_WHEEL,
    CONSTANT_A661_WHEEL
};

static const char_t* const c_mapDataFormatXValues[] =
{
    CONSTANT_A661_MDF_ABSOLUTE,
    CONSTANT_A661_MDF_RELATIVE,
    CONSTANT_A661_MDF_X_DIST
};

static const char_t* const c_mapDataFormatYValues[] =
{
    CONSTANT_A661_MDF_ABSOLUTE,
    CONSTANT_A661_MDF_RELATIVE,
    CONSTANT_A661_MDF_Y_ALT
};

static const char_t* const c_mapBoundaryAlgorithmValues[] =
{
    CONSTANT_A661_ARC,
    CONSTANT_A661_HORZ_LINE,
    CONSTANT_A661_VERT_LINE
};

static const char_t* const c_pushButtonAlignmentValues[] =
{
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_CENTER
};

static const char_t* const c_wrappingTypeValues[] =
{
    CONSTANT_A661_WRAP_NONE,
    CONSTANT_A661_WRAP_BOTH,
    CONSTANT_A661_WRAP_FIRST_TO_LAST,
    CONSTANT_A661_WRAP_LAST_TO_FIRST
};

static const char_t* const c_pagingControlPositionValues[] =
{
    CONSTANT_A661_ABSENT,
    CONSTANT_A661_BOTTOM_CENTER,
    CONSTANT_A661_BOTTOM_LEFT,
    CONSTANT_A661_BOTTOM_RIGHT,
    CONSTANT_A661_CENTER,
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_TOP_CENTER,
    CONSTANT_A661_TOP_LEFT,
    CONSTANT_A661_TOP_RIGHT
};

static const char_t* const c_picturePositionValues[] =
{
    CONSTANT_A661_LEFT,
    CONSTANT_A661_CENTER,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_TOP,
    CONSTANT_A661_BOTTOM
};

static const char_t* const c_toggleStateValues[] =
{
    CONSTANT_A661_UNSELECTED,
    CONSTANT_A661_SELECTED
};

static const char_t* const c_popUpMenuOpeningModeValues[] =
{
    CONSTANT_A661_OPEN_UP,
    CONSTANT_A661_OPEN_DOWN,
    CONSTANT_A661_CDS_DEPENDENT
};

static const char_t* const c_popUpButtonPicturePositionValues[] =
{
    CONSTANT_A661_CENTER,
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_TOP,
    CONSTANT_A661_BOTTOM
};

static const char_t* const c_popUpMenuButtonOpeningModeValues[] =
{
    CONSTANT_A661_OPEN_UP,
    CONSTANT_A661_OPEN_DOWN
};

static const char_t* const c_shuffleToFitModeValues[] =
{
    CONSTANT_A661_SHUFFLE_UP,
    CONSTANT_A661_SHUFFLE_DOWN,
    CONSTANT_A661_SHUFFLE_LEFT,
    CONSTANT_A661_SHUFFLE_RIGHT,
    CONSTANT_A661_NO_SHUFFLE
};

static const char_t* const c_trueFalseValues[] =
{
    CONSTANT_A661_TRUE,
    CONSTANT_A661_FALSE
};

static const char_t* const c_sliderOrientationValues[] =
{
    CONSTANT_A661_BOTTOM_TO_TOP,
    CONSTANT_A661_TOP_TO_BOTTOM,
    CONSTANT_A661_LEFT_TO_RIGHT,
    CONSTANT_A661_RIGHT_TO_LEFT
};

static const char_t* const c_sliderAlignmentValues[] =
{
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT,
    CONSTANT_A661_CENTER,
    CONSTANT_A661_TOP,
    CONSTANT_A661_BOTTOM
};

static const char_t* const c_loopTypeValues[] =
{
    CONSTANT_A661_LOOP_FORWARD,
    CONSTANT_A661_LOOP_FORWARD_AND_RESET,
    CONSTANT_A661_LOOP_FORWARD_AND_BLANK,
    CONSTANT_A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK
};

static const char_t* const c_animationTypeValues[] =
{
    CONSTANT_A661_DONT_RUN,
    CONSTANT_A661_RUN,
    CONSTANT_A661_RUN_ONCE
};

static const char_t* const c_tabPositionValues[] =
{
    CONSTANT_A661_ABSENT,
    CONSTANT_A661_TOP,
    CONSTANT_A661_BOTTOM,
    CONSTANT_A661_LEFT,
    CONSTANT_A661_RIGHT
};

static const char_t* const c_boundaryCheckModeValues[] =
{
    CONSTANT_A661_OUT_OF_BOUNDS
};

static const char_t* const c_coordSystemReportModeValues[] =
{
    CONSTANT_A661_REPORT_ON_CDS,
    CONSTANT_A661_REPORT_ALL
};

/*************************************************************************************************/
/**
 * \brief Creation Structure parameters, one array per widget type and then per extension type.
 *
 * \par Details:
 *  Transcribed from the Part 1-7 Creation Structure tables named in each group.
 *  Widget groups start after ParentIdent; extension groups start after
 *  ExtensionType. An array holds elementsPerEntry elements for each entry counted by the
 *  parameter named by countName; arrays of pairs, such as Vertices and
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
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                             BINARY_FIELD_PAD,    3,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_GROUP parameters: Table 3.9.12-2, AnimationGroup Creation Structure.
 */
static const BinaryField_t c_animationGroupFields[] =
{
    {PARAMETER_ANONYMOUS,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_TYPE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DELAY,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_REPETITION, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_ONPARAM parameters: Table 3.9.8-2, AnimationOnParam Creation Structure.
 */
static const BinaryField_t c_animationOnparamFields[] =
{
    {PARAMETER_ANONYMOUS,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                           BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TARGET_WIDGET_ID,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TARGET_PARAM_ID,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                           BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FROM_VALUE,           BINARY_FIELD_BYTES,  4,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TO_VALUE,             BINARY_FIELD_BYTES,  4,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_LAW_REF,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DURATION,             BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DELAY,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_REPETITION, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_ROTATION parameters: Table 3.9.9-2, AnimationRotation Creation Structure.
 */
static const BinaryField_t c_animationRotationFields[] =
{
    {PARAMETER_ANONYMOUS,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,           COUNT_OF(c_booleanValues)},
    {NULL,                           BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_TARGET_WIDGET_ID,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_STYLE_SET,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_CENTER_X,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_CENTER_Y,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_FROM_ANGLE,           BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_TO_ANGLE,             BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_ANIMATION_LAW_REF,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_DURATION,             BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_DELAY,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_DIRECTION,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_rotationDirectionValues, COUNT_OF(c_rotationDirectionValues)},
    {PARAMETER_NUMBER_OF_TURNS,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_ANIMATION_REPETITION, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {NULL,                           BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,                      0}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_SCALE parameters: Table 3.9.10-2, AnimationScale Creation Structure.
 */
static const BinaryField_t c_animationScaleFields[] =
{
    {PARAMETER_ANONYMOUS,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                           BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TARGET_WIDGET_ID,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FROM_SCALE_X,         BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FROM_SCALE_Y,         BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TO_SCALE_X,           BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TO_SCALE_Y,           BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_LAW_REF,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DURATION,             BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DELAY,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_REPETITION, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_ANIMATION_TRANSLATION parameters: Table 3.9.11-2, AnimationTranslation Creation Structure.
 */
static const BinaryField_t c_animationTranslationFields[] =
{
    {PARAMETER_ANONYMOUS,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                           BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TARGET_WIDGET_ID,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FROM_VALUE_X,         BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FROM_VALUE_Y,         BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TO_VALUE_X,           BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TO_VALUE_Y,           BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_LAW_REF,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DURATION,             BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_DELAY,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ANIMATION_REPETITION, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_BASIC_CONTAINER parameters: Table 3.3.2-2, BasicContainer Creation Structure.
 */
static const BinaryField_t c_basicContainerFields[] =
{
    {PARAMETER_ENABLE,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,   BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,   BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_BLINKING_CONTAINER parameters: Table 3.3.3-2, BlinkingContainer Creation Structure.
 */
static const BinaryField_t c_blinkingContainerFields[] =
{
    {PARAMETER_BLINKING_TYPE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_VISIBLE,       BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_BROADCAST_RECEIVER parameters: Table 3.8.9-2, BroadcastReceiver Creation Structure.
 */
static const BinaryField_t c_broadcastReceiverFields[] =
{
    {PARAMETER_NUMBER_OF_FIELDS, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_PARAMETER_IDENT,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_WIDGET_STRUCTURE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUMBER_OF_FIELDS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_BUFFER_FORMAT parameters: Table 3.3.4-2, BufferFormat Creation Structure.
 */
static const BinaryField_t c_bufferFormatFields[] =
{
    {PARAMETER_NUMBER_OF_FIELDS, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                       BINARY_SINGLE_VALUE,   NULL, 0},
    {PARAMETER_BUFFER_STRUCTURE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUMBER_OF_FIELDS, BINARY_PAIR_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_CHECK_BUTTON parameters: Table 3.3.5-2, CheckButton Creation Structure.
 */
static const BinaryField_t c_checkButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                     COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_CHECK_BUTTON_STATE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_checkButtonStateValues,           COUNT_OF(c_checkButtonStateValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues,            COUNT_OF(c_buttonAlignmentValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_PICTURE_POSITION,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_checkButtonPicturePositionValues, COUNT_OF(c_checkButtonPicturePositionValues)},
    {NULL,                             BINARY_FIELD_PAD,    2,                     NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                               0}
};

/*************************************************************************************************/
/**
 * \brief A661_COMBO_BOX parameters: Table 3.3.6-2, ComboBox Creation Structure.
 */
static const BinaryField_t c_comboBoxFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,              COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTING_AREA_WIDTH,   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTING_AREA_HEIGHT,  BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_NUMBER_OF_ENTRIES,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NUMBER_OF_ENTRIES,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTED_ENTRY,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_OPENING_ENTRY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_buttonAlignmentValues,     COUNT_OF(c_buttonAlignmentValues)},
    {PARAMETER_OPENING_MODE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_comboBoxOpeningModeValues, COUNT_OF(c_comboBoxOpeningModeValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {NULL,                             BINARY_FIELD_PAD,    3,                     NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_ENTRY_LIST,             BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                        0}
};

/*************************************************************************************************/
/**
 * \brief A661_COMBO_BOX_EDIT parameters: Table 3.4.7-2, ComboBoxEdit Creation Structure.
 */
static const BinaryField_t c_comboBoxEditFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,                  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_SELECTING_AREA_WIDTH,   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_SELECTING_AREA_HEIGHT,  BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_MAX_NUMBER_OF_ENTRIES,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_NUMBER_OF_ENTRIES,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_SELECTED_ENTRY,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_OPENING_ENTRY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_OPENING_MODE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_comboBoxOpeningModeValues,     COUNT_OF(c_comboBoxOpeningModeValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_START_CURSOR_POS,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                            0},
    {PARAMETER_REPORT_ALL_CHANGES,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_editBoxReportAllChangesValues, COUNT_OF(c_editBoxReportAllChangesValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_editBoxAlignmentValues,        COUNT_OF(c_editBoxAlignmentValues)},
    {PARAMETER_ENTRY_LIST,             BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                            0}
};

/*************************************************************************************************/
/**
 * \brief A661_CONNECTOR parameters: Table 3.3.7-2, Connector Creation Structure.
 */
static const BinaryField_t c_connectorFields[] =
{
    {PARAMETER_CONNECTOR_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_VISIBLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_ENABLE,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {NULL,                          BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_OVER parameters: Table 3.6.3-2, CursorOver Creation Structure.
 */
static const BinaryField_t c_cursorOverFields[] =
{
    {PARAMETER_VISIBLE,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,            COUNT_OF(c_booleanValues)},
    {PARAMETER_ENABLE,               BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,             COUNT_OF(c_enableValues)},
    {PARAMETER_POS_X,                BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_POS_Y,                BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_SIZE_X,               BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_SIZE_Y,               BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_STYLE_SET,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {NULL,                           BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_POSITION_REPORT_MODE, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_positionReportModeValues, COUNT_OF(c_positionReportModeValues)},
    {NULL,                           BINARY_FIELD_PAD,    3,                   NULL, BINARY_SINGLE_VALUE, NULL,                       0}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_POS_OVERLAY parameters: Table 3.3.8-2, CursorPosOverlay Creation Structure.
 */
static const BinaryField_t c_cursorPosOverlayFields[] =
{
    {PARAMETER_ENABLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues, COUNT_OF(c_enableValues)},
    {NULL,             BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,           0},
    {PARAMETER_POS_X,  BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,           0},
    {PARAMETER_POS_Y,  BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,           0},
    {PARAMETER_SIZE_X, BINARY_FIELD_ULONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,           0},
    {PARAMETER_SIZE_Y, BINARY_FIELD_ULONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,           0}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_REF parameters: Table 3.6.2-2, CursorRef Creation Structure.
 */
static const BinaryField_t c_cursorRefFields[] =
{
    {NULL,            BINARY_FIELD_PAD,  2,                  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_POS_X, BINARY_FIELD_LONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_POS_Y, BINARY_FIELD_LONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_CONNECTOR parameters: Table 3.9.16-2, DataConnector Creation Structure.
 */
static const BinaryField_t c_dataConnectorFields[] =
{
    {PARAMETER_CONNECTOR_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                  BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_VISIBLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                  BINARY_SINGLE_VALUE,  c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_ENABLE,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                  BINARY_SINGLE_VALUE,  c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_DATA_LENGTH,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                  BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_DATA,                BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  PARAMETER_DATA_LENGTH, BINARY_ONE_PER_ENTRY, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_SCALING_FR180 parameters: Table 3.8.8-2, DataScalingFR180 Creation Structure.
 */
static const BinaryField_t c_dataScalingFr180Fields[] =
{
    {PARAMETER_TARGET_WIDGET_ID,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_TARGET_PARAMETER_ID, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_CLAMPING,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_clampingValues, COUNT_OF(c_clampingValues)},
    {NULL,                          BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_START_VALUE,         BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_END_VALUE,           BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_START_SCALED_VALUE,  BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_END_SCALED_VALUE,    BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_SCALING_LONG parameters: Table 3.8.6-2, DataScalingLong Creation Structure.
 */
static const BinaryField_t c_dataScalingLongFields[] =
{
    {PARAMETER_TARGET_WIDGET_ID,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_TARGET_PARAMETER_ID, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_CLAMPING,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_clampingValues, COUNT_OF(c_clampingValues)},
    {NULL,                          BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_START_VALUE,         BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_END_VALUE,           BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_START_SCALED_VALUE,  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_END_SCALED_VALUE,    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0}
};

/*************************************************************************************************/
/**
 * \brief A661_DATA_SCALING_ULONG parameters: Table 3.8.7-2, DataScalingULong Creation Structure.
 */
static const BinaryField_t c_dataScalingUlongFields[] =
{
    {PARAMETER_TARGET_WIDGET_ID,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_TARGET_PARAMETER_ID, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_CLAMPING,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_clampingValues, COUNT_OF(c_clampingValues)},
    {NULL,                          BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_START_VALUE,         BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_END_VALUE,           BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_START_SCALED_VALUE,  BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0},
    {PARAMETER_END_SCALED_VALUE,    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,             0}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_MASKED parameters: Table 3.3.9-2, EditBoxMasked Creation Structure.
 */
static const BinaryField_t c_editBoxMaskedFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_ALPHA_MASK,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_NUMERIC_MASK,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_START_CURSOR_POS,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_REPORT_ALL_CHANGES,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxReportAllChangesValues, COUNT_OF(c_editBoxReportAllChangesValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues,         COUNT_OF(c_buttonAlignmentValues)},
    {NULL,                             BINARY_FIELD_PAD,    3,                     NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                            0}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_MULTI_LINE parameters: Table 3.4.6-2, EditBoxMultiLine Creation Structure.
 */
static const BinaryField_t c_editBoxMultiLineFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_START_CURSOR_POS,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_REPORT_ALL_CHANGES,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxReportAllChangesValues, COUNT_OF(c_editBoxReportAllChangesValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxAlignmentValues,        COUNT_OF(c_editBoxAlignmentValues)},
    {PARAMETER_VERTICAL_SCROLL,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_scrollBarPositionValues,       COUNT_OF(c_scrollBarPositionValues)},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                            0}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_NUMERIC parameters: Table 3.3.10-2, EditBoxNumeric Creation Structure.
 */
static const BinaryField_t c_editBoxNumericFields[] =
{
    {PARAMETER_ENABLE,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                         COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                        COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_POS_Y,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_SIZE_X,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_SIZE_Y,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_STYLE_SET,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_VALUE,                    BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_TICS_COARSE,              BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_TICS_FINE,                BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_MIN_VALUE,                BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_MAX_VALUE,                BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_LEGEND_AREA_SIZE_X,       BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_START_CURSOR_POS_BYTE,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_MAX_FORMAT_STRING_LENGTH, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_MAX_LEGEND_STRING_LENGTH, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_LEGEND_POSITION,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_legendPositionValues,                 COUNT_OF(c_legendPositionValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                        COUNT_OF(c_booleanValues)},
    {PARAMETER_REPORT_ALL_CHANGES,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxNumericReportAllChangesValues, COUNT_OF(c_editBoxNumericReportAllChangesValues)},
    {PARAMETER_ALIGNMENT,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxAlignmentValues,               COUNT_OF(c_editBoxAlignmentValues)},
    {PARAMETER_LEGEND_REMOVED,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                        COUNT_OF(c_booleanValues)},
    {PARAMETER_NUMERIC_KEY_FLAG,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_CYCLIC_FLAG,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {NULL,                               BINARY_FIELD_PAD,    2,                     NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_FORMAT_STRING,            BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                                   0},
    {PARAMETER_LEGEND_STRING,            BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                                   0}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_NUMERIC_BCD parameters: Table 3.6.1-5, EditBoxNumericBCD Creation Structure.
 */
static const BinaryField_t c_editBoxNumericBcdFields[] =
{
    {PARAMETER_ENABLE,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_VISIBLE,                  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POS_X,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POS_Y,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_X,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_Y,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_LEGEND_AREA_SIZE_X,       BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_STYLE_SET,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_VALUE,                    BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MIN_VALUE,                BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MAX_VALUE,                BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MIN_FIELDS_VALUES,        BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MAX_FIELDS_VALUES,        BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_TICS_COARSE,              BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_TICS_FINE,                BINARY_FIELD_BYTES,  8,                     NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NUMBER_OF_FIELDS,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_ALIGNMENT,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_LEGEND_POSITION,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_LEGEND_REMOVED,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MAX_FORMAT_STRING_LENGTH, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MAX_LEGEND_STRING_LENGTH, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POSITIVE_STRING_LENGTH,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NEGATIVE_STRING_LENGTH,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_REPORT_ALL_CHANGES,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NUMERIC_KEY_FLAG,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_CYCLIC_FLAG,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_OF_FIELDS,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_NUMBER_OF_FIELDS, BINARY_ONE_PER_ENTRY, NULL, 0},
    {NULL,                               BINARY_FIELD_ALIGN,  BINARY_VARIABLE_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_FORMAT_STRING,            BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_LEGEND_STRING,            BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POSITIVE_STRING,          BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NEGATIVE_STRING,          BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_EDIT_BOX_TEXT parameters: Table 3.3.11-2, EditBoxText Creation Structure.
 */
static const BinaryField_t c_editBoxTextFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_START_CURSOR_POS,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                 COUNT_OF(c_booleanValues)},
    {PARAMETER_REPORT_ALL_CHANGES,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxReportAllChangesValues, COUNT_OF(c_editBoxReportAllChangesValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_editBoxAlignmentValues,        COUNT_OF(c_editBoxAlignmentValues)},
    {NULL,                             BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                            0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                            0}
};

/*************************************************************************************************/
/**
 * \brief A661_EVENT_HANDLER parameters: Table 3.9.17-3, EventHandler Creation Structure.
 */
static const BinaryField_t c_eventHandlerFields[] =
{
    {PARAMETER_ENABLE_HANDLER,            BINARY_FIELD_UCHAR,         BINARY_UCHAR_BYTES,    NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_TRIGGER_LAYER_IDENT,       BINARY_FIELD_UCHAR,         BINARY_UCHAR_BYTES,    NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_TRIGGER_WIDGET_IDENT,      BINARY_FIELD_USHORT,        BINARY_USHORT_BYTES,   NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_TRIGGER_EVENT,             BINARY_FIELD_USHORT,        BINARY_USHORT_BYTES,   NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {NULL,                                BINARY_FIELD_PAD,           2,                     NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NUM_WIDGET_PARAM_RESULTS,  BINARY_FIELD_UCHAR,         BINARY_UCHAR_BYTES,    NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NUM_LAYER_REQ_RESULTS,     BINARY_FIELD_UCHAR,         BINARY_UCHAR_BYTES,    NULL,                               BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_WIDGET_PARAM_RESULT_ARRAY, BINARY_FIELD_BYTES,         20,                    PARAMETER_NUM_WIDGET_PARAM_RESULTS, BINARY_ONE_PER_ENTRY, NULL, 0},
    {PARAMETER_LAYER_REQ_RESULT_ARRAY,    BINARY_FIELD_LAYER_REQUEST, BINARY_VARIABLE_BYTES, PARAMETER_NUM_LAYER_REQ_RESULTS,    BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_EXTERNAL_SOURCE parameters: Table 3.4.2-2, ExternalSource Creation Structure.
 */
static const BinaryField_t c_externalSourceFields[] =
{
    {PARAMETER_VISIBLE,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_MOTION_ALLOWED,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SOURCE_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                       BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SOURCE_X,         BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SOURCE_Y,         BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SOURCE_DX,        BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SOURCE_DY,        BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                       BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_IN parameters: Table 3.6.4.2-2, FocusIn Creation Structure.
 */
static const BinaryField_t c_focusInFields[] =
{
    {PARAMETER_NEXT_FOCUSED_WIDGET, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_LINK parameters: Table 3.6.4.1-2, FocusLink Creation Structure.
 */
static const BinaryField_t c_focusLinkFields[] =
{
    {NULL,                               BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {NULL,                               BINARY_FIELD_PAD,    4,                   NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_NEXT_LAYER_CONNECTOR_REF, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_OUT parameters: Table 3.6.4.3-2, FocusOut Creation Structure.
 */
static const BinaryField_t c_focusOutFields[] =
{
    {PARAMETER_NEXT_FOCUS_IN_APP_ID,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_NEXT_FOCUS_IN_LAYER_ID, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {NULL,                             BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_NEXT_FOCUS_IN_ID,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_FRAMING_REFRESH_CONTAINER parameters: Table 3.9.18-2, FramingRefreshContainer Creation Structure.
 */
static const BinaryField_t c_framingRefreshContainerFields[] =
{
    {PARAMETER_VISIBLE,         BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_ENABLE,          BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_CACHING_ENABLED, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                      BINARY_FIELD_PAD,   3,                  NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GESTURE_AREA parameters: Table 3.9.6-4, GestureArea Creation Structure.
 */
static const BinaryField_t c_gestureAreaFields[] =
{
    {PARAMETER_ENABLE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_VISIBLE,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_STYLE_SET,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {NULL,                       BINARY_FIELD_PAD,    2,                   NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POS_X,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POS_Y,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_X,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_Y,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_REPORT_MODE,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_ALLOW_OUTSIDE,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NUMBER_OF_EVENTS, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {NULL,                       BINARY_FIELD_PAD,    1,                   NULL,                       BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_EVENT_ID_ARRAY,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  PARAMETER_NUMBER_OF_EVENTS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_ARC_CIRCLE parameters: Table 3.3.13-2, GpArcCircle Creation Structure.
 */
static const BinaryField_t c_gpArcCircleFields[] =
{
    {PARAMETER_ANONYMOUS,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_START_ANGLE, BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_END_ANGLE,   BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_RADIUS,      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FILLED,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                  BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_ARC_ELLIPSE parameters: Table 3.3.12-2, GpArcEllipse Creation Structure.
 */
static const BinaryField_t c_gpArcEllipseFields[] =
{
    {PARAMETER_ANONYMOUS,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_START_ANGLE, BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_END_ANGLE,   BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FILLED,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                  BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_CROWN parameters: Table 3.3.14-2, GpCrown Creation Structure.
 */
static const BinaryField_t c_gpCrownFields[] =
{
    {PARAMETER_ANONYMOUS,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,        BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,        BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_START_ANGLE,  BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_END_ANGLE,    BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_INNER_RADIUS, BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_OUTER_RADIUS, BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FILLED,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                   BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_LINE parameters: Table 3.3.15-2, GpLine Creation Structure.
 */
static const BinaryField_t c_gpLineFields[] =
{
    {PARAMETER_ANONYMOUS,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X_START, BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y_START, BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_X_END,   BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y_END,   BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_LINE_POLAR parameters: Table 3.3.16-2, GpLinePolar Creation Structure.
 */
static const BinaryField_t c_gpLinePolarFields[] =
{
    {PARAMETER_ANONYMOUS,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X_START,    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y_START,    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ROTATION_ANGLE, BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_LINE_LENGTH,    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_POLYLINE parameters: Table 3.8.1-2, GpPolyline Creation Structure.
 */
static const BinaryField_t c_gpPolylineFields[] =
{
    {PARAMETER_ANONYMOUS,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_COLOR_INDEX,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_HALO,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_haloValues,    COUNT_OF(c_haloValues)},
    {PARAMETER_CLOSED,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                             BINARY_FIELD_PAD,    3,                   NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_MAX_NUMBER_OF_VERTICES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_NUMBER_OF_VERTICES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_VERTICES,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_VERTICES, BINARY_PAIR_PER_ENTRY, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_RECTANGLE parameters: Table 3.3.17-2, GpRectangle Creation Structure.
 */
static const BinaryField_t c_gpRectangleFields[] =
{
    {PARAMETER_ANONYMOUS,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FILLED,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                  BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_TRIANGLE parameters: Table 3.3.18-2, GpTriangle Creation Structure.
 */
static const BinaryField_t c_gpTriangleFields[] =
{
    {PARAMETER_ANONYMOUS,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_X2,      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y2,      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_X3,      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y3,      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FILLED,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_HALO,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                  BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_TRIANGLE_FAN parameters: Table 3.10.5-2, GpTriangleFan Creation Structure.
 */
static const BinaryField_t c_gpTriangleFanFields[] =
{
    {PARAMETER_ANONYMOUS,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_COLOR_INDEX,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_FILLED,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_HALO,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                             BINARY_FIELD_PAD,    2,                   NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_MAX_NUMBER_OF_VERTICES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_NUMBER_OF_VERTICES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_VERTICES,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_VERTICES, BINARY_PAIR_PER_ENTRY, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_GP_TRIANGLE_STRIP parameters: Table 3.10.6-2, GpTriangleStrip Creation Structure.
 */
static const BinaryField_t c_gpTriangleStripFields[] =
{
    {PARAMETER_ANONYMOUS,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_COLOR_INDEX,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_FILLED,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_HALO,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_haloValues,    COUNT_OF(c_haloValues)},
    {NULL,                             BINARY_FIELD_PAD,    2,                   NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_MAX_NUMBER_OF_VERTICES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_NUMBER_OF_VERTICES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_VERTICES,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_VERTICES, BINARY_PAIR_PER_ENTRY, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_INK_AREA parameters: Table 3.9.7-2, InkArea Creation Structure.
 */
static const BinaryField_t c_inkAreaFields[] =
{
    {PARAMETER_VISIBLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_ENABLE,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_STYLE_SET,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                          BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_X,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,              BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,              BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_FILLED,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_FILL_INDEX,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                          BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_MAX_BUFFER_SIZE,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_NEXT_FOCUSED_WIDGET, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_KEYBOARD_AREA parameters: Table 3.9.2-2, KeyboardArea Creation Structure.
 */
static const BinaryField_t c_keyboardAreaFields[] =
{
    {PARAMETER_ENABLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_NUMBER_OF_KEYS,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_MAX_NUMBER_OF_KEYS, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {NULL,                         BINARY_FIELD_PAD,    3,                   NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POS_X,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_POS_Y,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_X,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_SIZE_Y,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_KEY_ARRAY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUMBER_OF_KEYS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_LABEL parameters: Table 3.3.20-2, Label Creation Structure.
 */
static const BinaryField_t c_labelFields[] =
{
    {PARAMETER_ANONYMOUS,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_POS_Y,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_SIZE_X,            BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_SIZE_Y,            BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_ROTATION_ANGLE,    BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_STYLE_SET,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_MAX_STRING_LENGTH, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_MOTION_ALLOWED,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_FONT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_COLOR_INDEX,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_ALIGNMENT,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_labelAlignmentValues, COUNT_OF(c_labelAlignmentValues)},
    {PARAMETER_LABEL_STRING,      BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                   0}
};

/*************************************************************************************************/
/**
 * \brief A661_LABEL_COMPLEX parameters: Table 3.3.21-2, LabelComplex Creation Structure.
 */
static const BinaryField_t c_labelComplexFields[] =
{
    {PARAMETER_ANONYMOUS,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_POS_Y,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_SIZE_X,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_SIZE_Y,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_STYLE_SET,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_MAX_STRING_LENGTH,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_ALIGNMENT,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_labelAlignmentValues, COUNT_OF(c_labelAlignmentValues)},
    {NULL,                         BINARY_FIELD_PAD,    3,                     NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_DEFAULT_STYLE_TEXT, BINARY_FIELD_BYTES,  12,                    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_LABEL_STRING,       BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                   0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ parameters: Table 3.3.25-2a, MapHorz Creation Structure.
 */
static const BinaryField_t c_maphorzFields[] =
{
    {PARAMETER_ENABLE,                   BINARY_FIELD_UCHAR,   BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                  BINARY_FIELD_UCHAR,   BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                    BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,                    BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,                   BINARY_FIELD_ULONG,   BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,                   BINARY_FIELD_ULONG,   BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SCREEN_REFERENCE_POINT_X, BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SCREEN_REFERENCE_POINT_Y, BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_RANGE,                    BINARY_FIELD_FR32768, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SCREEN_RANGE,             BINARY_FIELD_ULONG,   BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_PROJECTION_TYPE,          BINARY_FIELD_UCHAR,   BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                               BINARY_FIELD_PAD,     3,                  NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_CONTAINER parameters: Table 3.8.4-2, MapHorz_Container Creation Structure.
 */
static const BinaryField_t c_maphorzContainerFields[] =
{
    {NULL,              BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_VISIBLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_ITEMLIST parameters: Table 3.3.22-2, MapHorz_ItemList Creation Structure.
 */
static const BinaryField_t c_maphorzItemlistFields[] =
{
    {PARAMETER_ENABLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_MAX_NUMBER_OF_ITEM, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_MAX_ITEM_SIZE,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                         BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_PANEL parameters: Table 3.8.5-2, MapHorz_Panel Creation Structure.
 */
static const BinaryField_t c_maphorzPanelFields[] =
{
    {PARAMETER_ENABLE,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_SOURCE parameters: Table 3.3.24-2a, MapHorz_Source Creation Structure.
 */
static const BinaryField_t c_maphorzSourceFields[] =
{
    {PARAMETER_ENABLE,          BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,        COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,         BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues,       COUNT_OF(c_booleanValues)},
    {PARAMETER_MAP_DATA_FORMAT, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_mapDataFormatValues, COUNT_OF(c_mapDataFormatValues)},
    {PARAMETER_EVENT_FLAG,      BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_eventFlagValues,     COUNT_OF(c_eventFlagValues)},
    {NULL,                      BINARY_FIELD_PAD,   2,                  NULL, BINARY_SINGLE_VALUE, NULL,                  0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_VERTEXBUFFER parameters: Table 3.9.4-2, MapHorz_VertexBuffer Creation Structure.
 */
static const BinaryField_t c_maphorzVertexbufferFields[] =
{
    {PARAMETER_ANONYMOUS,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,   c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                             BINARY_FIELD_PAD,    1,                   NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_MAX_NUMBER_OF_VERTICES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_NUMBER_OF_VERTICES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,   NULL,            0},
    {PARAMETER_VERTICES,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_VERTICES, BINARY_PAIR_PER_ENTRY, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT parameters: Table 3.4.3-2a, MapVert Creation Structure.
 */
static const BinaryField_t c_mapvertFields[] =
{
    {PARAMETER_ENABLE,        BINARY_FIELD_UCHAR,   BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,       BINARY_FIELD_UCHAR,   BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,         BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,         BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,        BINARY_FIELD_ULONG,   BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,        BINARY_FIELD_ULONG,   BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_RANGE_X,       BINARY_FIELD_FR32768, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_RANGE_Y,       BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_REF_POS_X,     BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_REF_POS_Y,     BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_REF_GEO_POS_X, BINARY_FIELD_FR32768, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_REF_GEO_POS_Y, BINARY_FIELD_LONG,    BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_CONTAINER parameters: Table 3.9.13-2, MapVert_Container Creation Structure.
 */
static const BinaryField_t c_mapvertContainerFields[] =
{
    {NULL,              BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_VISIBLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_ITEMLIST parameters: Table 3.4.5-2a, MapVert_ItemList Creation Structure.
 */
static const BinaryField_t c_mapvertItemlistFields[] =
{
    {PARAMETER_ENABLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_MAX_NUMBER_OF_ITEM, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_MAX_ITEM_SIZE,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                         BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_PANEL parameters: Table 3.9.14-2, MapVert_Panel Creation Structure.
 */
static const BinaryField_t c_mapvertPanelFields[] =
{
    {PARAMETER_ENABLE,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_SOURCE parameters: Table 3.4.4-2a, MapVert_Source Creation Structure.
 */
static const BinaryField_t c_mapvertSourceFields[] =
{
    {PARAMETER_ENABLE,            BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,         COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,           BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_MAP_DATA_FORMAT_X, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_mapDataFormatXValues, COUNT_OF(c_mapDataFormatXValues)},
    {PARAMETER_MAP_DATA_FORMAT_Y, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_mapDataFormatYValues, COUNT_OF(c_mapDataFormatYValues)},
    {PARAMETER_EVENT_FLAG,        BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {NULL,                        BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,                   0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAP_BOUNDARY parameters: Table 3.9.15-2, MapBoundary Creation Structure.
 */
static const BinaryField_t c_mapBoundaryFields[] =
{
    {PARAMETER_ENABLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,               COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,               BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,              COUNT_OF(c_booleanValues)},
    {PARAMETER_MAX_NUMBER_OF_ENTRIES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_NUMBER_OF_ENTRIES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_BEGIN_ANGLE_ARRAY,     BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                         0},
    {PARAMETER_DISTANCE_ARRAY,        BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                         0},
    {PARAMETER_ALGORITHM_ARRAY,       BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, c_mapBoundaryAlgorithmValues, COUNT_OF(c_mapBoundaryAlgorithmValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_MAP_EXTERNAL_SOURCE parameters: Table 3.10.4-2, MapExternalSource Creation Structure.
 */
static const BinaryField_t c_mapExternalSourceFields[] =
{
    {PARAMETER_VISIBLE,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                       BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_X,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SOURCE_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAP_GRID parameters: Table 3.4.1-2, MapGrid Creation Structure.
 */
static const BinaryField_t c_mapGridFields[] =
{
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                  BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_OFFSET_X,    BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_OFFSET_Y,    BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_INCREMENT_X, BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_INCREMENT_Y, BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COUNT_X,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COUNT_Y,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MASK_CONTAINER parameters: Table 3.3.26-2, MaskContainer Creation Structure.
 */
static const BinaryField_t c_maskContainerFields[] =
{
    {PARAMETER_MASK_ENABLED,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,          BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,          BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_MASK_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_MOTION_ALLOWED, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                     BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MENU_BAR parameters: Table 3.4.8-2, MenuBar Creation Structure.
 */
static const BinaryField_t c_menuBarFields[] =
{
    {PARAMETER_ENABLE,      BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,     BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_HORIZONTAL,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                  BINARY_FIELD_PAD,   3,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_X,       BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,       BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_BUTTON_POS,  BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_BUTTON_SIZE, BINARY_FIELD_ULONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_MULTI_STATE_BUTTON parameters: Table 3.9.1-2, MultiStateButton Creation Structure.
 */
static const BinaryField_t c_multiStateButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  c_enableValues,              COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_BUTTON_STATE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  c_pushButtonAlignmentValues, COUNT_OF(c_pushButtonAlignmentValues)},
    {PARAMETER_MAX_NUMBER_OF_STATES,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NUMBER_OF_STATES,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {NULL,                             BINARY_FIELD_PAD,    1,                     NULL,                       BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_LABEL_STRING_ARRAY,     BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_STATES, BINARY_ONE_PER_ENTRY, NULL,                        0}
};

/*************************************************************************************************/
/**
 * \brief A661_MUTUALLY_EXCLUSIVE_CONTAINER parameters: Table 3.5.1-2, MutuallyExclusiveContainer Creation Structure.
 */
static const BinaryField_t c_mutuallyExclusiveContainerFields[] =
{
    {PARAMETER_ENABLE,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_VISIBLE,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_POS_X,         BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_POS_Y,         BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_VISIBLE_CHILD, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {NULL,                    BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_NO_SERVICE_MONITOR parameters: Table 3.8.10-2, NoServiceMonitor Creation Structure.
 */
static const BinaryField_t c_noServiceMonitorFields[] =
{
    {PARAMETER_SHOW_NO_SERVICE_IDENT, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {NULL,                            BINARY_FIELD_PAD,    4,                   NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_NUMERIC_READOUT parameters: Table 3.8.3-2, NumericReadout Creation Structure.
 */
static const BinaryField_t c_numericReadoutFields[] =
{
    {PARAMETER_VISIBLE,                  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_MOTION_ALLOWED,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,        COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_POS_Y,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_SIZE_X,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_SIZE_Y,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_ROTATION_ANGLE,           BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_STYLE_SET,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_ALIGNMENT,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_labelAlignmentValues, COUNT_OF(c_labelAlignmentValues)},
    {NULL,                               BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_VALUE,                    BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_LEGEND_AREA_SIZE_X,       BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_MAX_FORMAT_STRING_LENGTH, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_MAX_LEGEND_STRING_LENGTH, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_LEGEND_POSITION,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_legendPositionValues, COUNT_OF(c_legendPositionValues)},
    {NULL,                               BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_FORMAT_STRING,            BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                   0},
    {PARAMETER_LEGEND_STRING,            BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                   0}
};

/*************************************************************************************************/
/**
 * \brief A661_PAGING_CONTAINER parameters: Table 3.8.2-2, PagingContainer Creation Structure.
 */
static const BinaryField_t c_pagingContainerFields[] =
{
    {PARAMETER_ENABLE,                  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,                COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,               COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                   BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_POS_Y,                   BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_SIZE_X,                  BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_SIZE_Y,                  BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_WRAPPING_TYPE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_wrappingTypeValues,          COUNT_OF(c_wrappingTypeValues)},
    {PARAMETER_PAGING_CONTROL_POSITION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_pagingControlPositionValues, COUNT_OF(c_pagingControlPositionValues)},
    {PARAMETER_FINE_PAGE_DELTA,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_COARSE_PAGE_DELTA,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_VISIBLE_CHILD,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_REPORT_VISIBLE_CHILD,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,               COUNT_OF(c_booleanValues)},
    {PARAMETER_STYLE_SET,               BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                          0},
    {PARAMETER_ANONYMOUS,               BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,               COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_PANEL parameters: Table 3.3.27-2, Panel Creation Structure.
 */
static const BinaryField_t c_panelFields[] =
{
    {PARAMETER_ENABLE,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,          BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,          BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,         BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,         BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_MOTION_ALLOWED, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                     BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE parameters: Table 3.3.19-2, Picture Creation Structure.
 */
static const BinaryField_t c_pictureFields[] =
{
    {PARAMETER_ANONYMOUS,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,            BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,            BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_PICTURE_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_ANIMATED parameters: Table 3.5.5-2, PictureAnimated Creation Structure.
 */
static const BinaryField_t c_pictureAnimatedFields[] =
{
    {PARAMETER_VISIBLE,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                         BINARY_FIELD_PAD,    1,                   NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_POS_X,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_POS_Y,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_SIZE_X,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_SIZE_Y,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_STYLE_SET,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_NUMBER_OF_PICTURES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_INDEX_OF_FREQUENCY, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_LOOP_FLAG,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_ANIMATION_FLAG,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                         BINARY_FIELD_PAD,    1,                   NULL,                         BINARY_SINGLE_VALUE,  NULL,            0},
    {PARAMETER_PICTURE_ARRAY,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUMBER_OF_PICTURES, BINARY_ONE_PER_ENTRY, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_PUSH_BUTTON parameters: Table 3.3.28-2, Picture PushButton Creation Structure.
 */
static const BinaryField_t c_picturePushButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,          COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_PICTURE_REFERENCE,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_PICTURE_POSITION,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_picturePositionValues, COUNT_OF(c_picturePositionValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues, COUNT_OF(c_buttonAlignmentValues)},
    {NULL,                             BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                    0}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_TOGGLE_BUTTON parameters: Table 3.3.29-2, PictureToggleButton Creation Structure.
 */
static const BinaryField_t c_pictureToggleButtonFields[] =
{
    {PARAMETER_ENABLE,                      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,          COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_POS_Y,                       BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_X,                      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_Y,                      BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_STYLE_SET,                   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_MAX_STRING_LENGTH,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_ALTERNATE_FLAG,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_PICTURE_REFERENCE,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_ALTERNATE_PICTURE_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_PICTURE_POSITION,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_picturePositionValues, COUNT_OF(c_picturePositionValues)},
    {PARAMETER_TOGGLE_STATE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_toggleStateValues,     COUNT_OF(c_toggleStateValues)},
    {PARAMETER_ALIGNMENT,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues, COUNT_OF(c_buttonAlignmentValues)},
    {NULL,                                  BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_LABEL_STRING,                BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_ALTERNATE_LABEL_STRING,      BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                    0}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_MENU parameters: Table 3.3.31-2, PopUpMenu Creation Structure.
 */
static const BinaryField_t c_popUpMenuFields[] =
{
    {PARAMETER_OPENING_MODE,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_popUpMenuOpeningModeValues, COUNT_OF(c_popUpMenuOpeningModeValues)},
    {PARAMETER_NUMBER_OF_ENTRIES,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_POS_X,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_POS_Y,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_SIZE_X,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_SIZE_Y,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_MAX_STRING_LENGTH,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_STYLE_SET,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                         0},
    {PARAMETER_POP_UP_IDENT_ARRAY, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                         0},
    {PARAMETER_PICTURE_ARRAY,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                         0},
    {PARAMETER_ENABLE_ARRAY,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                         0},
    {PARAMETER_STRING_ARRAY,       BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                         0}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_MENU_BUTTON parameters: Table 3.3.32-2, PopUpMenuButton Creation Structure.
 */
static const BinaryField_t c_popUpMenuButtonFields[] =
{
    {PARAMETER_ENABLE,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,                     COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_POS_Y,                    BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_SIZE_X,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_SIZE_Y,                   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_STYLE_SET,                BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_POPUP_POS_X,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_POPUP_POS_Y,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_POPUP_SIZE_X,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_POPUP_SIZE_Y,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_MAX_STRING_LENGTH,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_MAX_STRING_LENGTH_POP_UP, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_PICTURE_REFERENCE,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_NUMBER_OF_ENTRIES,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_PICTURE_POSITION,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_popUpButtonPicturePositionValues, COUNT_OF(c_popUpButtonPicturePositionValues)},
    {PARAMETER_OPENING_MODE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_popUpMenuButtonOpeningModeValues, COUNT_OF(c_popUpMenuButtonOpeningModeValues)},
    {PARAMETER_ALIGNMENT,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_buttonAlignmentValues,            COUNT_OF(c_buttonAlignmentValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {NULL,                               BINARY_FIELD_PAD,    1,                     NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_LABEL_STRING,             BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {NULL,                               BINARY_FIELD_ALIGN,  BINARY_VARIABLE_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                               0},
    {PARAMETER_POP_UP_IDENT_ARRAY,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                               0},
    {PARAMETER_PICTURE_ARRAY,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                               0},
    {PARAMETER_ENABLE_ARRAY,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                               0},
    {PARAMETER_STRING_ARRAY,             BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                               0}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_PANEL parameters: Table 3.3.30-2, PopUpPanel Creation Structure.
 */
static const BinaryField_t c_popUpPanelFields[] =
{
    {PARAMETER_UA_POSITION_FLAG,  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_AUTOMATIC_CLOSURE, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,             BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,            BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,            BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                        BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_POP_UP_PANEL_BUTTON parameters: Table 3.7.3-2, PopUpPanelButton Creation Structure.
 */
static const BinaryField_t c_popUpPanelButtonFields[] =
{
    {PARAMETER_ENABLE,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                     COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_POS_Y,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_SIZE_X,              BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_SIZE_Y,              BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_STYLE_SET,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_NEXT_FOCUSED_WIDGET, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_POPUP_POS_X,         BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_POPUP_POS_Y,         BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_POPUP_SIZE_X,        BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_POPUP_SIZE_Y,        BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_MAX_STRING_LENGTH,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_PICTURE_REFERENCE,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_PICTURE_POSITION,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_popUpButtonPicturePositionValues, COUNT_OF(c_popUpButtonPicturePositionValues)},
    {PARAMETER_ALIGNMENT,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues,            COUNT_OF(c_buttonAlignmentValues)},
    {PARAMETER_UA_POSITION_FLAG,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_AUTOMATIC_CLOSURE,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_LABEL_STRING,        BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                               0}
};

/*************************************************************************************************/
/**
 * \brief A661_PROXY_BUTTON parameters: Table 3.5.2-2, ProxyButton Creation Structure.
 */
static const BinaryField_t c_proxyButtonFields[] =
{
    {PARAMETER_ENABLE,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues, COUNT_OF(c_enableValues)},
    {NULL,                          BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,           0},
    {PARAMETER_DEDICATED_KEY_IDENT, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,           0},
    {PARAMETER_TARGET_WIDGET_IDENT, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,           0}
};

/*************************************************************************************************/
/**
 * \brief A661_PUSH_BUTTON parameters: Table 3.3.33-2, PushButton Creation Structure.
 */
static const BinaryField_t c_pushButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,              COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_pushButtonAlignmentValues, COUNT_OF(c_pushButtonAlignmentValues)},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                        0}
};

/*************************************************************************************************/
/**
 * \brief A661_RADIO_BOX parameters: Table 3.3.34-2, RadioBox Creation Structure.
 */
static const BinaryField_t c_radioBoxFields[] =
{
    {PARAMETER_ENABLE,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_ROTATION_CONTAINER parameters: Table 3.3.35-2, RotationContainer Creation Structure.
 */
static const BinaryField_t c_rotationContainerFields[] =
{
    {PARAMETER_VISIBLE,        BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                     BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_CENTER_X,       BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_CENTER_Y,       BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ROTATION_ANGLE, BINARY_FIELD_FR180, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_SCALE_CONTAINER parameters: Table 3.10.1-2, ScaleContainer Creation Structure.
 */
static const BinaryField_t c_scaleContainerFields[] =
{
    {PARAMETER_VISIBLE,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,               BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_CENTER_X, BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_CENTER_Y, BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SCALE_X,  BINARY_FIELD_FLOAT, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SCALE_Y,  BINARY_FIELD_FLOAT, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_SCROLL_LIST parameters: Table 3.3.37-2, ScrollList Creation Structure.
 */
static const BinaryField_t c_scrollListFields[] =
{
    {PARAMETER_ENABLE,                    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,              COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                     BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_POS_Y,                     BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_X,                    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_Y,                    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_STYLE_SET,                 BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_NUMBER_OF_ENTRIES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NUMBER_OF_ENTRIES,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTED_ENTRY,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_STRING_LENGTH,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_FIRST_ACCESSIBLE_ENTRY,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_FIRST_VISIBLE_ENTRY,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_VERTICAL_SCROLL,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_scrollBarPositionValues,   COUNT_OF(c_scrollBarPositionValues)},
    {PARAMETER_ALIGNMENT,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_pushButtonAlignmentValues, COUNT_OF(c_pushButtonAlignmentValues)},
    {PARAMETER_FLAG_REPORT_VISIBLE_ENTRY, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_DEFAULT_STYLE_TEXT,        BINARY_FIELD_BYTES,  12,                    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_ENABLE_ARRAY,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                        0},
    {PARAMETER_LABEL_STRING_ARRAY,        BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                        0}
};

/*************************************************************************************************/
/**
 * \brief A661_SCROLL_PANEL parameters: Table 3.3.36-2, ScrollPanel Creation Structure.
 */
static const BinaryField_t c_scrollPanelFields[] =
{
    {PARAMETER_ENABLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,            COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,               BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,           COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                 BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_POS_Y,                 BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_X,                BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_Y,                BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_LINE_DELTA_X,          BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_LINE_DELTA_Y,          BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_PAGE_DELTA_X,          BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_PAGE_DELTA_Y,          BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_HOME_X,                BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_HOME_Y,                BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_FRAME_X,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_FRAME_Y,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_XSHEET,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_YSHEET,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_BOUND_X,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_BOUND_Y,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_XBOUND,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_YBOUND,           BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_STYLE_SET,             BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {NULL,                            BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_HORIZONTAL_SCROLL,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_scrollBarPositionValues, COUNT_OF(c_scrollBarPositionValues)},
    {PARAMETER_VERTICAL_SCROLL,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_scrollBarPositionValues, COUNT_OF(c_scrollBarPositionValues)},
    {PARAMETER_FLAG_REPORT_FRAME_POS, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,           COUNT_OF(c_booleanValues)},
    {NULL,                            BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,                      0}
};

/*************************************************************************************************/
/**
 * \brief A661_SCROLL_WHEEL_AREA parameters: Table 3.9.3-2, ScrollWheelArea Creation Structure.
 */
static const BinaryField_t c_scrollWheelAreaFields[] =
{
    {PARAMETER_ENABLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {NULL,             BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_POS_X,  BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_POS_Y,  BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_SIZE_X, BINARY_FIELD_ULONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_SIZE_Y, BINARY_FIELD_ULONG, BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_SELECTION_LIST_BUTTON parameters: Table 3.5.7-2, SelectionListButton Creation Structure.
 */
static const BinaryField_t c_selectionListButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,              COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTING_AREA_WIDTH,   BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTING_AREA_HEIGHT,  BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_NUMBER_OF_ENTRIES,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_NUMBER_OF_ENTRIES,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_SELECTED_ENTRY,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_OPENING_ENTRY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_buttonAlignmentValues,     COUNT_OF(c_buttonAlignmentValues)},
    {PARAMETER_OPENING_MODE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_comboBoxOpeningModeValues, COUNT_OF(c_comboBoxOpeningModeValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,             COUNT_OF(c_booleanValues)},
    {NULL,                             BINARY_FIELD_PAD,    3,                     NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {NULL,                             BINARY_FIELD_ALIGN,  BINARY_VARIABLE_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                        0},
    {PARAMETER_ENTRY_LIST,             BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                        0}
};

/*************************************************************************************************/
/**
 * \brief A661_SHUFFLE_TO_FIT_CONTAINER parameters: Table 3.6.6-2, ShuffleToFitContainer Creation Structure.
 */
static const BinaryField_t c_shuffleToFitContainerFields[] =
{
    {PARAMETER_ENABLE,                     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,           COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,          COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_POS_Y,                      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_SIZE_X,                     BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_SIZE_Y,                     BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_NUMBER_OF_VISIBLE_CHILDREN, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_SHUFFLE_TO_FIT_MODE,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_shuffleToFitModeValues, COUNT_OF(c_shuffleToFitModeValues)},
    {NULL,                                 BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_ITEM_SPACING,               BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {PARAMETER_STYLE_SET,                  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                     0},
    {NULL,                                 BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,                     0}
};

/*************************************************************************************************/
/**
 * \brief A661_SIZE_TO_FIT_CONTAINER parameters: Table 3.6.5-2, SizeToFitContainer Creation Structure.
 */
static const BinaryField_t c_sizeToFitContainerFields[] =
{
    {PARAMETER_ENABLE,                     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,  COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,                      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_X,                     BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_Y,                     BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_NUMBER_OF_VISIBLE_CHILDREN, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SIZE_TO_FIT_MODE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                                 BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ITEM_SPACING,               BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_SLIDER parameters: Table 3.5.4-2, Slider Creation Structure.
 */
static const BinaryField_t c_sliderFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,            COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,           COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_MIN_VALUE,              BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_MAX_VALUE,              BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_VALUE,                  BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_MAJOR_TICK_INTERVAL,    BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_SHOW_MAJOR_LABELS,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_trueFalseValues,         COUNT_OF(c_trueFalseValues)},
    {PARAMETER_ORIENTATION,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_sliderOrientationValues, COUNT_OF(c_sliderOrientationValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_sliderAlignmentValues,   COUNT_OF(c_sliderAlignmentValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,           COUNT_OF(c_booleanValues)},
    {PARAMETER_LSB_MULTIPLE,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_MINOR_TICK_MULTIPLE,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                      0},
    {PARAMETER_MAJOR_TICK_REFERENCE,   BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                      0}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL parameters: Table 3.3.38-2, Symbol Creation Structure.
 */
static const BinaryField_t c_symbolFields[] =
{
    {PARAMETER_MOTION_ALLOWED,   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_POS_Y,            BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_ROTATION_ANGLE,   BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_STYLE_SET,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_SYMBOL_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_COLOR_INDEX,      BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {NULL,                       BINARY_FIELD_PAD,    3,                   NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_ANIMATED parameters: Table 3.5.6-2, SymbolAnimated Creation Structure.
 */
static const BinaryField_t c_symbolAnimatedFields[] =
{
    {PARAMETER_MOTION_ALLOWED,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,       COUNT_OF(c_booleanValues)},
    {PARAMETER_VISIBLE,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,       COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_POS_Y,              BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_STYLE_SET,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_COLOR_INDEX,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_LOOP_TYPE,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_loopTypeValues,      COUNT_OF(c_loopTypeValues)},
    {PARAMETER_NUMBER_OF_SYMBOLS,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_INDEX_OF_FREQUENCY, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_ANIMATION_TYPE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_animationTypeValues, COUNT_OF(c_animationTypeValues)},
    {PARAMETER_SYMBOL_ARRAY,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_SYMBOLS, BINARY_ONE_PER_ENTRY, NULL,                  0},
    {NULL,                         BINARY_FIELD_ALIGN,  BINARY_VARIABLE_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                  0},
    {PARAMETER_ORIENTATION_ARRAY,  BINARY_FIELD_FR180,  BINARY_ULONG_BYTES,    PARAMETER_NUMBER_OF_SYMBOLS, BINARY_ONE_PER_ENTRY, NULL,                  0},
    {PARAMETER_MOVEMENT_X_ARRAY,   BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    PARAMETER_NUMBER_OF_SYMBOLS, BINARY_ONE_PER_ENTRY, NULL,                  0},
    {PARAMETER_MOVEMENT_Y_ARRAY,   BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    PARAMETER_NUMBER_OF_SYMBOLS, BINARY_ONE_PER_ENTRY, NULL,                  0}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_PUSH_BUTTON parameters: Table 3.7.1-2, SymbolPushButton Creation Structure.
 */
static const BinaryField_t c_symbolPushButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,          COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SYMBOL_REFERENCE,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SYMBOL_POSITION,        BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_picturePositionValues, COUNT_OF(c_picturePositionValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues, COUNT_OF(c_buttonAlignmentValues)},
    {NULL,                             BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                    0}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_TOGGLE_BUTTON parameters: Table 3.7.2-2, SymbolToggleButton Creation Structure.
 */
static const BinaryField_t c_symbolToggleButtonFields[] =
{
    {PARAMETER_ENABLE,                     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,          COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_POS_Y,                      BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_X,                     BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SIZE_Y,                     BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_STYLE_SET,                  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_MAX_STRING_LENGTH,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_ALTERNATE_FLAG,             BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,         COUNT_OF(c_booleanValues)},
    {PARAMETER_SYMBOL_REFERENCE,           BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_ALTERNATE_SYMBOL_REFERENCE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_SYMBOL_POSITION,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_picturePositionValues, COUNT_OF(c_picturePositionValues)},
    {PARAMETER_TOGGLE_STATE,               BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_toggleStateValues,     COUNT_OF(c_toggleStateValues)},
    {PARAMETER_ALIGNMENT,                  BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_buttonAlignmentValues, COUNT_OF(c_buttonAlignmentValues)},
    {NULL,                                 BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_LABEL_STRING,               BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                    0},
    {PARAMETER_ALTERNATE_LABEL_STRING,     BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                    0}
};

/*************************************************************************************************/
/**
 * \brief A661_TABBED_PANEL parameters: Table 3.3.39-2, TabbedPanel Creation Structure.
 */
static const BinaryField_t c_tabbedPanelFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,                     COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_PICTURE_REFERENCE,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_PICTURE_POSITION,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_popUpButtonPicturePositionValues, COUNT_OF(c_popUpButtonPicturePositionValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,                    COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_pushButtonAlignmentValues,        COUNT_OF(c_pushButtonAlignmentValues)},
    {NULL,                             BINARY_FIELD_PAD,    1,                     NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_INSET_SIZE,             BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                               0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                               0}
};

/*************************************************************************************************/
/**
 * \brief A661_TABBED_PANEL_GROUP parameters: Table 3.3.40-2, TabbedPanelGroup Creation Structure.
 */
static const BinaryField_t c_tabbedPanelGroupFields[] =
{
    {PARAMETER_ENABLE,                    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,      COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,     COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                     BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                0},
    {PARAMETER_POS_Y,                     BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                0},
    {PARAMETER_SIZE_X,                    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                0},
    {PARAMETER_SIZE_Y,                    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                0},
    {PARAMETER_STYLE_SET,                 BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                0},
    {PARAMETER_ACTIVE_TABBED_PANEL_ID,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                0},
    {PARAMETER_TAB_POSITION,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_tabPositionValues, COUNT_OF(c_tabPositionValues)},
    {PARAMETER_AUTOMATIC_INSET_SIZE_FLAG, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,     COUNT_OF(c_booleanValues)},
    {NULL,                                BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,                0}
};

/*************************************************************************************************/
/**
 * \brief A661_TOGGLE_BUTTON parameters: Table 3.3.41-2, ToggleButton Creation Structure.
 */
static const BinaryField_t c_toggleButtonFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_enableValues,              COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,    BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_MAX_STRING_LENGTH,      BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_TOGGLE_STATE,           BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_toggleStateValues,         COUNT_OF(c_toggleStateValues)},
    {PARAMETER_ALTERNATE_FLAG,         BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_booleanValues,             COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL, BINARY_SINGLE_VALUE, c_pushButtonAlignmentValues, COUNT_OF(c_pushButtonAlignmentValues)},
    {NULL,                             BINARY_FIELD_PAD,    2,                     NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_LABEL_STRING,           BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                        0},
    {PARAMETER_ALTERNATE_LABEL_STRING, BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                        0}
};

/*************************************************************************************************/
/**
 * \brief A661_TOUCH_AREA parameters: Table 3.9.5-2, TouchArea Creation Structure.
 */
static const BinaryField_t c_touchAreaFields[] =
{
    {PARAMETER_ENABLE,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_enableValues,             COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,            COUNT_OF(c_booleanValues)},
    {PARAMETER_STYLE_SET,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {NULL,                             BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_POS_X,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_POS_Y,                  BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_SIZE_X,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_SIZE_Y,                 BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_REPORT_MODE,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_positionReportModeValues, COUNT_OF(c_positionReportModeValues)},
    {PARAMETER_ALLOW_OUTSIDE,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_booleanValues,            COUNT_OF(c_booleanValues)},
    {PARAMETER_MAX_TOUCH_POINTS,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0},
    {PARAMETER_NUMBER_OF_TOUCH_POINTS, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,                       0}
};

/*************************************************************************************************/
/**
 * \brief A661_TRANSLATION_CONTAINER parameters: Table 3.3.42-2, TranslationContainer Creation Structure.
 */
static const BinaryField_t c_translationContainerFields[] =
{
    {PARAMETER_VISIBLE,       BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {NULL,                    BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TRANSLATION_X, BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_TRANSLATION_Y, BINARY_FIELD_LONG,  BINARY_ULONG_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_TREE parameters: Table 3.10.3-2, Tree Creation Structure.
 */
static const BinaryField_t c_treeFields[] =
{
    {PARAMETER_ENABLE,                    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_enableValues,           COUNT_OF(c_enableValues)},
    {PARAMETER_VISIBLE,                   BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,          COUNT_OF(c_booleanValues)},
    {PARAMETER_POS_X,                     BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_POS_Y,                     BINARY_FIELD_LONG,   BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_SIZE_X,                    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_SIZE_Y,                    BINARY_FIELD_ULONG,  BINARY_ULONG_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_FIRST_VISIBLE_ENTRY,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_FLAG_REPORT_VISIBLE_ENTRY, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,          COUNT_OF(c_booleanValues)},
    {NULL,                                BINARY_FIELD_PAD,    1,                     NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_STYLE_SET,                 BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_NEXT_FOCUSED_WIDGET,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_MAX_NUMBER_OF_ENTRIES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_NUMBER_OF_ENTRIES,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_SELECTED_ENTRY,            BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_MAX_STRING_LENGTH,         BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_AUTOMATIC_FOCUS_MOTION,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_booleanValues,          COUNT_OF(c_booleanValues)},
    {PARAMETER_ALIGNMENT,                 BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                        BINARY_SINGLE_VALUE,  c_editBoxAlignmentValues, COUNT_OF(c_editBoxAlignmentValues)},
    {PARAMETER_NODE_STYLE_ARRAY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                     0},
    {PARAMETER_PARENT_ARRAY,              BINARY_FIELD_USHORT, BINARY_USHORT_BYTES,   PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                     0},
    {PARAMETER_ENABLE_ARRAY,              BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                     0},
    {PARAMETER_OPEN_ARRAY,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                     0},
    {NULL,                                BINARY_FIELD_ALIGN,  BINARY_VARIABLE_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                     0},
    {PARAMETER_LABEL_STRING_ARRAY,        BINARY_FIELD_STRING, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_ENTRIES, BINARY_ONE_PER_ENTRY, NULL,                     0}
};

/*************************************************************************************************/
/**
 * \brief A661_WATCHDOG_CONTAINER parameters: Table 3.5.3-2, WatchdogContainer Creation Structure.
 */
static const BinaryField_t c_watchdogContainerFields[] =
{
    {NULL,                         BINARY_FIELD_PAD,    2,                   NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {PARAMETER_TIME_PERIOD,        BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {PARAMETER_VALID_COUNT_LIMIT,  BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {PARAMETER_FAIL_COUNT_LIMIT,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {PARAMETER_REFRESH,            BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {NULL,                         BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {PARAMETER_SHOW_IF_FAIL_IDENT, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,              0},
    {PARAMETER_SHOW_FAIL,          BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL, BINARY_SINGLE_VALUE, c_trueFalseValues, COUNT_OF(c_trueFalseValues)},
    {NULL,                         BINARY_FIELD_PAD,    1,                   NULL, BINARY_SINGLE_VALUE, NULL,              0}
};

/*************************************************************************************************/
/**
 * \brief A661_DIRECTIONAL_TABBING_EXTENSION parameters: Table 8.5.1-2, DirectionalTabbingExtension Creation Structure.
 */
static const BinaryField_t c_directionalTabbingExtensionFields[] =
{
    {PARAMETER_NUM_DIRECTIONS,                BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {NULL,                                    BINARY_FIELD_PAD,    1,                   NULL,                     BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_TAB_DIRECTION_WIDGET_ID_ARRAY, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUM_DIRECTIONS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_EVENTS_EXTENSION parameters: Table 8.5.2-2, CursorEventsExtension Creation Structure.
 */
static const BinaryField_t c_cursorEventsExtensionFields[] =
{
    {PARAMETER_NUM_EVENTS,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                 BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_EVENT_ID_ARRAY, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUM_EVENTS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_LEGEND_ALIGN_EXTENSION parameters: Table 8.5.3-2, LegendStringAlignment Creation Structure.
 */
static const BinaryField_t c_legendAlignExtensionFields[] =
{
    {PARAMETER_LEGEND_ALIGNMENT, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_editBoxAlignmentValues, COUNT_OF(c_editBoxAlignmentValues)},
    {NULL,                       BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,                     0}
};

/*************************************************************************************************/
/**
 * \brief A661_CHILD_INDEX_EXTENSION parameters: Table 8.5.4-2, VisibleChildIndexExtension Creation Structure.
 */
static const BinaryField_t c_childIndexExtensionFields[] =
{
    {PARAMETER_VISIBLE_CHILD_INDEX, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_INITIAL_FOCUS_EXTENSION parameters: Table 8.5.5-2, InitialFocusExtension Creation Structure.
 */
static const BinaryField_t c_initialFocusExtensionFields[] =
{
    {NULL,                    BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,            0},
    {PARAMETER_INITIAL_FOCUS, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_FOCUS_STOP_EXTENSION parameters: Table 8.5.6-2, FocusStopExtension Creation Structure.
 */
static const BinaryField_t c_focusStopExtensionFields[] =
{
    {PARAMETER_STOP_FORWARD,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_STOP_BACKWARD, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_SHAPE_EXTENSION parameters: Table 8.5.7-2, CursorShapeExtension Creation Structure.
 */
static const BinaryField_t c_cursorShapeExtensionFields[] =
{
    {PARAMETER_ALLOW_CHANGE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_booleanValues, COUNT_OF(c_booleanValues)},
    {PARAMETER_CURSOR_INDEX, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, NULL,            0}
};

/*************************************************************************************************/
/**
 * \brief A661_PICTURE_EXTENSION parameters: Table 8.6.1-2, PictureExtension Creation Structure.
 */
static const BinaryField_t c_pictureExtensionFields[] =
{
    {PARAMETER_MAX_NUMBER_OF_PICTURES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,  NULL,                    0},
    {PARAMETER_NUMBER_OF_PICTURES,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                         BINARY_SINGLE_VALUE,  NULL,                    0},
    {PARAMETER_PICTURE_POSITION,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                         BINARY_SINGLE_VALUE,  c_picturePositionValues, COUNT_OF(c_picturePositionValues)},
    {NULL,                             BINARY_FIELD_PAD,    1,                   NULL,                         BINARY_SINGLE_VALUE,  NULL,                    0},
    {PARAMETER_PICTURE_ARRAY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUMBER_OF_PICTURES, BINARY_ONE_PER_ENTRY, NULL,                    0}
};

/*************************************************************************************************/
/**
 * \brief A661_SYMBOL_EXTENSION parameters: Table 8.6.2-2, SymbolExtension Creation Structure.
 */
static const BinaryField_t c_symbolExtensionFields[] =
{
    {PARAMETER_MAX_NUMBER_OF_SYMBOLS, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                    0},
    {PARAMETER_NUMBER_OF_SYMBOLS,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                        BINARY_SINGLE_VALUE,  NULL,                    0},
    {PARAMETER_SYMBOL_POSITION,       BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                        BINARY_SINGLE_VALUE,  c_picturePositionValues, COUNT_OF(c_picturePositionValues)},
    {NULL,                            BINARY_FIELD_PAD,    1,                   NULL,                        BINARY_SINGLE_VALUE,  NULL,                    0},
    {PARAMETER_SYMBOL_ARRAY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUMBER_OF_SYMBOLS, BINARY_ONE_PER_ENTRY, NULL,                    0}
};

/*************************************************************************************************/
/**
 * \brief A661_TICS_ARRAY_EXTENSION parameters: Table 8.6.3-2, TicsArrayExtension Creation Structure.
 */
static const BinaryField_t c_ticsArrayExtensionFields[] =
{
    {NULL,                                BINARY_FIELD_PAD,    2,                   NULL,                            BINARY_SINGLE_VALUE,   NULL, 0},
    {PARAMETER_MAX_NUMBER_OF_TICS_COARSE, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                            BINARY_SINGLE_VALUE,   NULL, 0},
    {PARAMETER_NUMBER_OF_TICS_COARSE,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                            BINARY_SINGLE_VALUE,   NULL, 0},
    {PARAMETER_MAX_NUMBER_OF_TICS_FINE,   BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                            BINARY_SINGLE_VALUE,   NULL, 0},
    {PARAMETER_NUMBER_OF_TICS_FINE,       BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                            BINARY_SINGLE_VALUE,   NULL, 0},
    {PARAMETER_TICS_COARSE_ARRAY,         BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_TICS_COARSE, BINARY_PAIR_PER_ENTRY, NULL, 0},
    {PARAMETER_TICS_FINE_ARRAY,           BINARY_FIELD_FLOAT,  BINARY_ULONG_BYTES,  PARAMETER_NUMBER_OF_TICS_FINE,   BINARY_PAIR_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_STYLE_SET_EXTENSION parameters: Table 8.6.4-2, StyleSetExtension Creation Structure.
 */
static const BinaryField_t c_styleSetExtensionFields[] =
{
    {PARAMETER_MAX_STYLE_SET_ARRAY_SIZE, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                           BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_STYLE_SET_ARRAY_SIZE,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                           BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_STYLE_SET_ARRAY,          BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_STYLE_SET_ARRAY_SIZE, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_STATIC_PARAM_BUFFER_EXTENSION parameters: Table 8.6.5-2, StaticParamBufferExtension Creation Structure.
 */
static const BinaryField_t c_staticParamBufferExtensionFields[] =
{
    {PARAMETER_NUMBER_OF_BUFFERS, BINARY_FIELD_USHORT,      BINARY_USHORT_BYTES,   NULL,                        BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_BUFFER_ARRAY,      BINARY_FIELD_SIZED_BYTES, BINARY_VARIABLE_BYTES, PARAMETER_NUMBER_OF_BUFFERS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_MULTI_SELECTION_EXTENSION parameters: Table 8.6.6-2, MultiSelectionExtension Creation Structure.
 */
static const BinaryField_t c_multiSelectionExtensionFields[] =
{
    {PARAMETER_MAXIMUM_SELECTABLE_ENTRIES, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_EXCLUDED_REGIONS_EXTENSION parameters: Table 8.6.7-2, ExcludedRegionsExtension Creation Structure.
 */
static const BinaryField_t c_excludedRegionsExtensionFields[] =
{
    {PARAMETER_MAX_NUMBER_OF_REGIONS, BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    NULL,                            BINARY_SINGLE_VALUE,  NULL, 0},
    {NULL,                            BINARY_FIELD_PAD,    1,                     NULL,                            BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_EFFECTIVITY_ARRAY,     BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,    PARAMETER_MAX_NUMBER_OF_REGIONS, BINARY_ONE_PER_ENTRY, NULL, 0},
    {NULL,                            BINARY_FIELD_ALIGN,  BINARY_VARIABLE_BYTES, NULL,                            BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_EXCLUDED_REGIONS_LIST, BINARY_FIELD_REGION, BINARY_VARIABLE_BYTES, PARAMETER_MAX_NUMBER_OF_REGIONS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_BOUNDARY_CHECK_EXTENSION parameters: Table 8.6.8-2, BoundaryCheckExtension Creation Structure.
 */
static const BinaryField_t c_boundaryCheckExtensionFields[] =
{
    {PARAMETER_BOUNDARY_CHECK_MODE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, c_boundaryCheckModeValues, COUNT_OF(c_boundaryCheckModeValues)},
    {NULL,                          BINARY_FIELD_PAD,   1,                  NULL, BINARY_SINGLE_VALUE, NULL,                      0}
};

/*************************************************************************************************/
/**
 * \brief A661_REORDER_EXTENSION parameters: Table 8.7.1-2, ReorderExtension Creation Structure.
 */
static const BinaryField_t c_reorderExtensionFields[] =
{
    {NULL, BINARY_FIELD_PAD, 2, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_WORD_WRAP_EXTENSION parameters: Table 8.7.2-2, WordWrapExtension Creation Structure.
 */
static const BinaryField_t c_wordWrapExtensionFields[] =
{
    {PARAMETER_WORD_WRAP,  BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0},
    {PARAMETER_WRAP_STYLE, BINARY_FIELD_UCHAR, BINARY_UCHAR_BYTES, NULL, BINARY_SINGLE_VALUE, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_CURSOR_ENTRY_EVENTS_EXTENSION parameters: Table 8.7.3-2, CursorEntryEventsExtension Creation Structure.
 */
static const BinaryField_t c_cursorEntryEventsExtensionFields[] =
{
    {PARAMETER_NUM_EVENTS,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                 BINARY_SINGLE_VALUE,  NULL, 0},
    {PARAMETER_EVENT_ID_ARRAY, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUM_EVENTS, BINARY_ONE_PER_ENTRY, NULL, 0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION parameters: Table 8.7.4-2, MapHorzCoordSystemEventExtension Creation Structure.
 */
static const BinaryField_t c_maphorzCoordSystemEventExtensionFields[] =
{
    {PARAMETER_NUM_EVENTS,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                 BINARY_SINGLE_VALUE,  NULL,                          0},
    {PARAMETER_REPORT_MODE,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                 BINARY_SINGLE_VALUE,  c_coordSystemReportModeValues, COUNT_OF(c_coordSystemReportModeValues)},
    {NULL,                     BINARY_FIELD_PAD,    3,                   NULL,                 BINARY_SINGLE_VALUE,  NULL,                          0},
    {PARAMETER_EVENT_ID_ARRAY, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUM_EVENTS, BINARY_ONE_PER_ENTRY, NULL,                          0}
};

/*************************************************************************************************/
/**
 * \brief A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION parameters: Table 8.7.5-2, MapVertCoordSystemEventExtension Creation Structure.
 */
static const BinaryField_t c_mapvertCoordSystemEventExtensionFields[] =
{
    {PARAMETER_NUM_EVENTS,     BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, NULL,                 BINARY_SINGLE_VALUE,  NULL,                          0},
    {PARAMETER_REPORT_MODE,    BINARY_FIELD_UCHAR,  BINARY_UCHAR_BYTES,  NULL,                 BINARY_SINGLE_VALUE,  c_coordSystemReportModeValues, COUNT_OF(c_coordSystemReportModeValues)},
    {NULL,                     BINARY_FIELD_PAD,    3,                   NULL,                 BINARY_SINGLE_VALUE,  NULL,                          0},
    {PARAMETER_EVENT_ID_ARRAY, BINARY_FIELD_USHORT, BINARY_USHORT_BYTES, PARAMETER_NUM_EVENTS, BINARY_ONE_PER_ENTRY, NULL,                          0}
};

/*************************************************************************************************/
/**
 * \brief Supported widget types in name order, with WidgetType codes from Table 4.6-7.
 */
static const BinaryWidget_t c_binaryWidgets[] =
{
    {TYPE_A661_ACTIVE_AREA,                  0xA010, c_activeAreaFields, COUNT_OF(c_activeAreaFields)},
    {TYPE_A661_ANIMATION_GROUP,              0xA1E3, c_animationGroupFields, COUNT_OF(c_animationGroupFields)},
    {TYPE_A661_ANIMATION_ONPARAM,            0xA1E4, c_animationOnparamFields, COUNT_OF(c_animationOnparamFields)},
    {TYPE_A661_ANIMATION_ROTATION,           0xA1E5, c_animationRotationFields, COUNT_OF(c_animationRotationFields)},
    {TYPE_A661_ANIMATION_SCALE,              0xA1E6, c_animationScaleFields, COUNT_OF(c_animationScaleFields)},
    {TYPE_A661_ANIMATION_TRANSLATION,        0xA1E7, c_animationTranslationFields, COUNT_OF(c_animationTranslationFields)},
    {TYPE_A661_BASIC_CONTAINER,              0xA020, c_basicContainerFields, COUNT_OF(c_basicContainerFields)},
    {TYPE_A661_BLINKING_CONTAINER,           0xA030, c_blinkingContainerFields, COUNT_OF(c_blinkingContainerFields)},
    {TYPE_A661_BROADCAST_RECEIVER,           0xA032, c_broadcastReceiverFields, COUNT_OF(c_broadcastReceiverFields)},
    {TYPE_A661_BUFFER_FORMAT,                0xA040, c_bufferFormatFields, COUNT_OF(c_bufferFormatFields)},
    {TYPE_A661_CHECK_BUTTON,                 0xA050, c_checkButtonFields, COUNT_OF(c_checkButtonFields)},
    {TYPE_A661_COMBO_BOX,                    0xA070, c_comboBoxFields, COUNT_OF(c_comboBoxFields)},
    {TYPE_A661_COMBO_BOX_EDIT,               0xA0E0, c_comboBoxEditFields, COUNT_OF(c_comboBoxEditFields)},
    {TYPE_A661_CONNECTOR,                    0xA080, c_connectorFields, COUNT_OF(c_connectorFields)},
    {TYPE_A661_CURSOR_OVER,                  0xA480, c_cursorOverFields, COUNT_OF(c_cursorOverFields)},
    {TYPE_A661_CURSOR_POS_OVERLAY,           0xA090, c_cursorPosOverlayFields, COUNT_OF(c_cursorPosOverlayFields)},
    {TYPE_A661_CURSOR_REF,                   0xA470, c_cursorRefFields, COUNT_OF(c_cursorRefFields)},
    {TYPE_A661_DATA_CONNECTOR,               0xA088, c_dataConnectorFields, COUNT_OF(c_dataConnectorFields)},
    {TYPE_A661_DATA_SCALING_FR180,           0xA2E0, c_dataScalingFr180Fields, COUNT_OF(c_dataScalingFr180Fields)},
    {TYPE_A661_DATA_SCALING_LONG,            0xA2D8, c_dataScalingLongFields, COUNT_OF(c_dataScalingLongFields)},
    {TYPE_A661_DATA_SCALING_ULONG,           0xA2D9, c_dataScalingUlongFields, COUNT_OF(c_dataScalingUlongFields)},
    {TYPE_A661_EDIT_BOX_MASKED,              0xA0A0, c_editBoxMaskedFields, COUNT_OF(c_editBoxMaskedFields)},
    {TYPE_A661_EDIT_BOX_MULTI_LINE,          0xA0B0, c_editBoxMultiLineFields, COUNT_OF(c_editBoxMultiLineFields)},
    {TYPE_A661_EDIT_BOX_NUMERIC,             0xA0C0, c_editBoxNumericFields, COUNT_OF(c_editBoxNumericFields)},
    {TYPE_A661_EDIT_BOX_NUMERIC_BCD,         0xA0C2, c_editBoxNumericBcdFields, COUNT_OF(c_editBoxNumericBcdFields)},
    {TYPE_A661_EDIT_BOX_TEXT,                0xA0D0, c_editBoxTextFields, COUNT_OF(c_editBoxTextFields)},
    {TYPE_A661_EVENT_HANDLER,                0xA600, c_eventHandlerFields, COUNT_OF(c_eventHandlerFields)},
    {TYPE_A661_EXTERNAL_SOURCE,              0xA188, c_externalSourceFields, COUNT_OF(c_externalSourceFields)},
    {TYPE_A661_FOCUS_IN,                     0xA498, c_focusInFields, COUNT_OF(c_focusInFields)},
    {TYPE_A661_FOCUS_LINK,                   0xA490, c_focusLinkFields, COUNT_OF(c_focusLinkFields)},
    {TYPE_A661_FOCUS_OUT,                    0xA499, c_focusOutFields, COUNT_OF(c_focusOutFields)},
    {TYPE_A661_FRAMING_REFRESH_CONTAINER,    0xA380, c_framingRefreshContainerFields, COUNT_OF(c_framingRefreshContainerFields)},
    {TYPE_A661_GESTURE_AREA,                 0xA501, c_gestureAreaFields, COUNT_OF(c_gestureAreaFields)},
    {TYPE_A661_GP_ARC_CIRCLE,                0xA0F0, c_gpArcCircleFields, COUNT_OF(c_gpArcCircleFields)},
    {TYPE_A661_GP_ARC_ELLIPSE,               0xA100, c_gpArcEllipseFields, COUNT_OF(c_gpArcEllipseFields)},
    {TYPE_A661_GP_CROWN,                     0xA110, c_gpCrownFields, COUNT_OF(c_gpCrownFields)},
    {TYPE_A661_GP_LINE,                      0xA120, c_gpLineFields, COUNT_OF(c_gpLineFields)},
    {TYPE_A661_GP_LINE_POLAR,                0xA130, c_gpLinePolarFields, COUNT_OF(c_gpLinePolarFields)},
    {TYPE_A661_GP_POLYLINE,                  0xA138, c_gpPolylineFields, COUNT_OF(c_gpPolylineFields)},
    {TYPE_A661_GP_RECTANGLE,                 0xA140, c_gpRectangleFields, COUNT_OF(c_gpRectangleFields)},
    {TYPE_A661_GP_TRIANGLE,                  0xA150, c_gpTriangleFields, COUNT_OF(c_gpTriangleFields)},
    {TYPE_A661_GP_TRIANGLE_FAN,              0xA151, c_gpTriangleFanFields, COUNT_OF(c_gpTriangleFanFields)},
    {TYPE_A661_GP_TRIANGLE_STRIP,            0xA152, c_gpTriangleStripFields, COUNT_OF(c_gpTriangleStripFields)},
    {TYPE_A661_INK_AREA,                     0xA502, c_inkAreaFields, COUNT_OF(c_inkAreaFields)},
    {TYPE_A661_KEYBOARD_AREA,                0xA1E1, c_keyboardAreaFields, COUNT_OF(c_keyboardAreaFields)},
    {TYPE_A661_LABEL,                        0xA160, c_labelFields, COUNT_OF(c_labelFields)},
    {TYPE_A661_LABEL_COMPLEX,                0xA170, c_labelComplexFields, COUNT_OF(c_labelComplexFields)},
    {TYPE_A661_MAPHORZ,                      0xA1B0, c_maphorzFields, COUNT_OF(c_maphorzFields)},
    {TYPE_A661_MAPHORZ_CONTAINER,            0xA1B8, c_maphorzContainerFields, COUNT_OF(c_maphorzContainerFields)},
    {TYPE_A661_MAPHORZ_ITEMLIST,             0xA180, c_maphorzItemlistFields, COUNT_OF(c_maphorzItemlistFields)},
    {TYPE_A661_MAPHORZ_PANEL,                0xA189, c_maphorzPanelFields, COUNT_OF(c_maphorzPanelFields)},
    {TYPE_A661_MAPHORZ_SOURCE,               0xA1A0, c_maphorzSourceFields, COUNT_OF(c_maphorzSourceFields)},
    {TYPE_A661_MAPHORZ_VERTEXBUFFER,         0xA1A9, c_maphorzVertexbufferFields, COUNT_OF(c_maphorzVertexbufferFields)},
    {TYPE_A661_MAPVERT,                      0xA1B2, c_mapvertFields, COUNT_OF(c_mapvertFields)},
    {TYPE_A661_MAPVERT_CONTAINER,            0xA1B5, c_mapvertContainerFields, COUNT_OF(c_mapvertContainerFields)},
    {TYPE_A661_MAPVERT_ITEMLIST,             0xA1B3, c_mapvertItemlistFields, COUNT_OF(c_mapvertItemlistFields)},
    {TYPE_A661_MAPVERT_PANEL,                0xA1B6, c_mapvertPanelFields, COUNT_OF(c_mapvertPanelFields)},
    {TYPE_A661_MAPVERT_SOURCE,               0xA1B4, c_mapvertSourceFields, COUNT_OF(c_mapvertSourceFields)},
    {TYPE_A661_MAP_BOUNDARY,                 0xA1BA, c_mapBoundaryFields, COUNT_OF(c_mapBoundaryFields)},
    {TYPE_A661_MAP_EXTERNAL_SOURCE,          0xA224, c_mapExternalSourceFields, COUNT_OF(c_mapExternalSourceFields)},
    {TYPE_A661_MAP_GRID,                     0xA178, c_mapGridFields, COUNT_OF(c_mapGridFields)},
    {TYPE_A661_MASK_CONTAINER,               0xA1C0, c_maskContainerFields, COUNT_OF(c_maskContainerFields)},
    {TYPE_A661_MENU_BAR,                     0xA1D0, c_menuBarFields, COUNT_OF(c_menuBarFields)},
    {TYPE_A661_MULTI_STATE_BUTTON,           0xA348, c_multiStateButtonFields, COUNT_OF(c_multiStateButtonFields)},
    {TYPE_A661_MUTUALLY_EXCLUSIVE_CONTAINER, 0xA400, c_mutuallyExclusiveContainerFields, COUNT_OF(c_mutuallyExclusiveContainerFields)},
    {TYPE_A661_NO_SERVICE_MONITOR,           0xA4F0, c_noServiceMonitorFields, COUNT_OF(c_noServiceMonitorFields)},
    {TYPE_A661_NUMERIC_READOUT,              0xA406, c_numericReadoutFields, COUNT_OF(c_numericReadoutFields)},
    {TYPE_A661_PAGING_CONTAINER,             0xA408, c_pagingContainerFields, COUNT_OF(c_pagingContainerFields)},
    {TYPE_A661_PANEL,                        0xA1F0, c_panelFields, COUNT_OF(c_panelFields)},
    {TYPE_A661_PICTURE,                      0xA200, c_pictureFields, COUNT_OF(c_pictureFields)},
    {TYPE_A661_PICTURE_ANIMATED,             0xA210, c_pictureAnimatedFields, COUNT_OF(c_pictureAnimatedFields)},
    {TYPE_A661_PICTURE_PUSH_BUTTON,          0xA240, c_picturePushButtonFields, COUNT_OF(c_picturePushButtonFields)},
    {TYPE_A661_PICTURE_TOGGLE_BUTTON,        0xA250, c_pictureToggleButtonFields, COUNT_OF(c_pictureToggleButtonFields)},
    {TYPE_A661_POP_UP_MENU,                  0xA270, c_popUpMenuFields, COUNT_OF(c_popUpMenuFields)},
    {TYPE_A661_POP_UP_MENU_BUTTON,           0xA280, c_popUpMenuButtonFields, COUNT_OF(c_popUpMenuButtonFields)},
    {TYPE_A661_POP_UP_PANEL,                 0xA290, c_popUpPanelFields, COUNT_OF(c_popUpPanelFields)},
    {TYPE_A661_POP_UP_PANEL_BUTTON,          0xA4E0, c_popUpPanelButtonFields, COUNT_OF(c_popUpPanelButtonFields)},
    {TYPE_A661_PROXY_BUTTON,                 0xA420, c_proxyButtonFields, COUNT_OF(c_proxyButtonFields)},
    {TYPE_A661_PUSH_BUTTON,                  0xA2A0, c_pushButtonFields, COUNT_OF(c_pushButtonFields)},
    {TYPE_A661_RADIO_BOX,                    0xA2B0, c_radioBoxFields, COUNT_OF(c_radioBoxFields)},
    {TYPE_A661_ROTATION_CONTAINER,           0xA2D0, c_rotationContainerFields, COUNT_OF(c_rotationContainerFields)},
    {TYPE_A661_SCALE_CONTAINER,              0xA2F2, c_scaleContainerFields, COUNT_OF(c_scaleContainerFields)},
    {TYPE_A661_SCROLL_LIST,                  0xA2F0, c_scrollListFields, COUNT_OF(c_scrollListFields)},
    {TYPE_A661_SCROLL_PANEL,                 0xA300, c_scrollPanelFields, COUNT_OF(c_scrollPanelFields)},
    {TYPE_A661_SCROLL_WHEEL_AREA,            0xA1E2, c_scrollWheelAreaFields, COUNT_OF(c_scrollWheelAreaFields)},
    {TYPE_A661_SELECTION_LIST_BUTTON,        0xA370, c_selectionListButtonFields, COUNT_OF(c_selectionListButtonFields)},
    {TYPE_A661_SHUFFLE_TO_FIT_CONTAINER,     0xA4B0, c_shuffleToFitContainerFields, COUNT_OF(c_shuffleToFitContainerFields)},
    {TYPE_A661_SIZE_TO_FIT_CONTAINER,        0xA4A0, c_sizeToFitContainerFields, COUNT_OF(c_sizeToFitContainerFields)},
    {TYPE_A661_SLIDER,                       0xA440, c_sliderFields, COUNT_OF(c_sliderFields)},
    {TYPE_A661_SYMBOL,                       0xA310, c_symbolFields, COUNT_OF(c_symbolFields)},
    {TYPE_A661_SYMBOL_ANIMATED,              0xA450, c_symbolAnimatedFields, COUNT_OF(c_symbolAnimatedFields)},
    {TYPE_A661_SYMBOL_PUSH_BUTTON,           0xA4C0, c_symbolPushButtonFields, COUNT_OF(c_symbolPushButtonFields)},
    {TYPE_A661_SYMBOL_TOGGLE_BUTTON,         0xA4D0, c_symbolToggleButtonFields, COUNT_OF(c_symbolToggleButtonFields)},
    {TYPE_A661_TABBED_PANEL,                 0xA320, c_tabbedPanelFields, COUNT_OF(c_tabbedPanelFields)},
    {TYPE_A661_TABBED_PANEL_GROUP,           0xA330, c_tabbedPanelGroupFields, COUNT_OF(c_tabbedPanelGroupFields)},
    {TYPE_A661_TOGGLE_BUTTON,                0xA340, c_toggleButtonFields, COUNT_OF(c_toggleButtonFields)},
    {TYPE_A661_TOUCH_AREA,                   0xA500, c_touchAreaFields, COUNT_OF(c_touchAreaFields)},
    {TYPE_A661_TRANSLATION_CONTAINER,        0xA360, c_translationContainerFields, COUNT_OF(c_translationContainerFields)},
    {TYPE_A661_TREE,                         0xA222, c_treeFields, COUNT_OF(c_treeFields)},
    {TYPE_A661_WATCHDOG_CONTAINER,           0xA460, c_watchdogContainerFields, COUNT_OF(c_watchdogContainerFields)}
};

/*************************************************************************************************/
/**
 * \brief Supported widget extension types, with ExtensionType codes from Table 4.6-12.
 */
static const BinaryWidget_t c_binaryExtensions[] =
{
    {TYPE_A661_DIRECTIONAL_TABBING_EXTENSION,        0x8003, c_directionalTabbingExtensionFields, COUNT_OF(c_directionalTabbingExtensionFields)},
    {TYPE_A661_CURSOR_EVENTS_EXTENSION,              0x8002, c_cursorEventsExtensionFields, COUNT_OF(c_cursorEventsExtensionFields)},
    {TYPE_A661_LEGEND_ALIGN_EXTENSION,               0x8004, c_legendAlignExtensionFields, COUNT_OF(c_legendAlignExtensionFields)},
    {TYPE_A661_CHILD_INDEX_EXTENSION,                0x8001, c_childIndexExtensionFields, COUNT_OF(c_childIndexExtensionFields)},
    {TYPE_A661_INITIAL_FOCUS_EXTENSION,              0x8006, c_initialFocusExtensionFields, COUNT_OF(c_initialFocusExtensionFields)},
    {TYPE_A661_FOCUS_STOP_EXTENSION,                 0x8007, c_focusStopExtensionFields, COUNT_OF(c_focusStopExtensionFields)},
    {TYPE_A661_CURSOR_SHAPE_EXTENSION,               0x8008, c_cursorShapeExtensionFields, COUNT_OF(c_cursorShapeExtensionFields)},
    {TYPE_A661_PICTURE_EXTENSION,                    0x8009, c_pictureExtensionFields, COUNT_OF(c_pictureExtensionFields)},
    {TYPE_A661_SYMBOL_EXTENSION,                     0x800A, c_symbolExtensionFields, COUNT_OF(c_symbolExtensionFields)},
    {TYPE_A661_TICS_ARRAY_EXTENSION,                 0x800B, c_ticsArrayExtensionFields, COUNT_OF(c_ticsArrayExtensionFields)},
    {TYPE_A661_STYLE_SET_EXTENSION,                  0x800C, c_styleSetExtensionFields, COUNT_OF(c_styleSetExtensionFields)},
    {TYPE_A661_STATIC_PARAM_BUFFER_EXTENSION,        0x800D, c_staticParamBufferExtensionFields, COUNT_OF(c_staticParamBufferExtensionFields)},
    {TYPE_A661_MULTI_SELECTION_EXTENSION,            0x800F, c_multiSelectionExtensionFields, COUNT_OF(c_multiSelectionExtensionFields)},
    {TYPE_A661_EXCLUDED_REGIONS_EXTENSION,           0x800E, c_excludedRegionsExtensionFields, COUNT_OF(c_excludedRegionsExtensionFields)},
    {TYPE_A661_BOUNDARY_CHECK_EXTENSION,             0x8010, c_boundaryCheckExtensionFields, COUNT_OF(c_boundaryCheckExtensionFields)},
    {TYPE_A661_REORDER_EXTENSION,                    0x8011, c_reorderExtensionFields, COUNT_OF(c_reorderExtensionFields)},
    {TYPE_A661_WORD_WRAP_EXTENSION,                  0x8012, c_wordWrapExtensionFields, COUNT_OF(c_wordWrapExtensionFields)},
    {TYPE_A661_CURSOR_ENTRY_EVENTS_EXTENSION,        0x8013, c_cursorEntryEventsExtensionFields, COUNT_OF(c_cursorEntryEventsExtensionFields)},
    {TYPE_A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION, 0x8014, c_maphorzCoordSystemEventExtensionFields, COUNT_OF(c_maphorzCoordSystemEventExtensionFields)},
    {TYPE_A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION, 0x8015, c_mapvertCoordSystemEventExtensionFields, COUNT_OF(c_mapvertCoordSystemEventExtensionFields)}
};

/*************************************************************************************************/
/**
 * \brief Constant names accepted as property values, from Tables 4.6-8, 4.6-10, 4.6-11, 4.6-13, 4.6-14, and 4.6-15.
 */
static const BinaryConstant_t c_binaryConstants[] =
{
    {CONSTANT_A661_ABORT,                               0x02},
    {CONSTANT_A661_ABORTED,                             0x02},
    {CONSTANT_A661_ABSENT,                              0x10},
    {CONSTANT_A661_ACTIVE_TABBED_PANEL,                 0xB050},
    {CONSTANT_A661_AC_LAT,                              0xB010},
    {CONSTANT_A661_AC_LAT_LONG,                         0xB030},
    {CONSTANT_A661_AC_LONG,                             0xB020},
    {CONSTANT_A661_AC_ORIENTATION,                      0xB040},
    {CONSTANT_A661_ALIGNMENT,                           0xB49A},
    {CONSTANT_A661_ALLOW_CHANGE,                        0xB6A1},
    {CONSTANT_A661_ALPHA_MASK,                          0xB060},
    {CONSTANT_A661_ALTERNATE_FLAG,                      0xB088},
    {CONSTANT_A661_ALTERN_PICTURE_REFERENCE,            0xB080},
    {CONSTANT_A661_ALTERN_SYMBOL_REFERENCE,             0xB4C1},
    {CONSTANT_A661_ANIMATION_DELAY,                     0xB702},
    {CONSTANT_A661_ANIMATION_DURATION,                  0xB703},
    {CONSTANT_A661_ANIMATION_FLAG,                      0xB090},
    {CONSTANT_A661_ANIMATION_LAW_REF,                   0xB704},
    {CONSTANT_A661_ANIMATION_REPETITION,                0xB705},
    {CONSTANT_A661_ANIMATION_STATE,                     0xB706},
    {CONSTANT_A661_ANIMATION_TYPE,                      0xB098},
    {CONSTANT_A661_APP_DATA,                            0xB6D0},
    {CONSTANT_A661_ARC,                                 0x01},
    {CONSTANT_A661_ASCII_EXTENDED,                      0x01},
    {CONSTANT_A661_AUTO_CLOSURE,                        0xB1C2},
    {CONSTANT_A661_AUTO_FOCUS_MOTION,                   0xB2B9},
    {CONSTANT_A661_BLINKING_TYPE,                       0xB0A8},
    {CONSTANT_A661_BLOCKED_BUFFER_OF_MAPITEM,           0xB121},
    {CONSTANT_A661_BLOCKED_BUFFER_OF_MAPVERT_ITEMS,     0xB126},
    {CONSTANT_A661_BOTH_CLAMPED,                        0x01},
    {CONSTANT_A661_BOTTOM,                              0x12},
    {CONSTANT_A661_BOTTOM_CENTER,                       0x30},
    {CONSTANT_A661_BOTTOM_LEFT,                         0x31},
    {CONSTANT_A661_BOTTOM_RIGHT,                        0x32},
    {CONSTANT_A661_BOTTOM_TO_TOP,                       0x40},
    {CONSTANT_A661_BOUNDARY_ARRAY,                      0xB6C1},
    {CONSTANT_A661_BOUNDARY_CHECK,                      0xBC},
    {CONSTANT_A661_BOUNDARY_CHECK_MODE,                 0xB122},
    {CONSTANT_A661_BOUND_SIZE_X,                        0xB0E0},
    {CONSTANT_A661_BOUND_SIZE_XY,                       0xB100},
    {CONSTANT_A661_BOUND_SIZE_Y,                        0xB0F0},
    {CONSTANT_A661_BOUND_X,                             0xB0B0},
    {CONSTANT_A661_BOUND_XY,                            0xB0D0},
    {CONSTANT_A661_BOUND_Y,                             0xB0C0},
    {CONSTANT_A661_BROADCAST_DATA,                      0xB0C2},
    {CONSTANT_A661_BUFFER_OF_FILL_STYLES,               0xB0F8},
    {CONSTANT_A661_BUFFER_OF_MAPITEM,                   0xB120},
    {CONSTANT_A661_BUFFER_OF_MAPVERT_ITEMS,             0xB125},
    {CONSTANT_A661_BUFFER_OF_PARAM,                     0xB110},
    {CONSTANT_A661_BUTTON_POS,                          0xB641},
    {CONSTANT_A661_BUTTON_SIZE,                         0xB642},
    {CONSTANT_A661_CACHING_ENABLE,                      0xB117},
    {CONSTANT_A661_CDS_DEPENDENT,                       0x1E},
    {CONSTANT_A661_CENTER,                              0x15},
    {CONSTANT_A661_CENTER_X,                            0xB130},
    {CONSTANT_A661_CENTER_XY,                           0xB150},
    {CONSTANT_A661_CENTER_Y,                            0xB140},
    {CONSTANT_A661_CIRCULAR,                            0x02},
    {CONSTANT_A661_CLAMPING,                            0xB501},
    {CONSTANT_A661_CLEAR,                               0xB000},
    {CONSTANT_A661_CLOCKWISE,                           0x00},
    {CONSTANT_A661_CLOSED,                              0xB1E2},
    {CONSTANT_A661_COARSE_PAGE_DELTA,                   0xB6E3},
    {CONSTANT_A661_COLOR_INDEX,                         0xB160},
    {CONSTANT_A661_COMPLETED,                           0x01},
    {CONSTANT_A661_COUNTER_CLOCKWISE,                   0x01},
    {CONSTANT_A661_CRP_LAT,                             0xB162},
    {CONSTANT_A661_CRP_LAT_LONG,                        0xB166},
    {CONSTANT_A661_CRP_LONG,                            0xB164},
    {CONSTANT_A661_CURSOR_CLICKED,                      0xB1D3},
    {CONSTANT_A661_CURSOR_DOUBLE_CLICKED,               0xB1D4},
    {CONSTANT_A661_CURSOR_INDEX,                        0xB6A0},
    {CONSTANT_A661_CURSOR_POS,                          0xB170},
    {CONSTANT_A661_CURSOR_POS_BYTE,                     0xB172},
    {CONSTANT_A661_CURSOR_PRESSED,                      0xB1D1},
    {CONSTANT_A661_CURSOR_RELEASED,                     0xB1D2},
    {CONSTANT_A661_CURSOR_RIGHT_CLICKED,                0xB1D5},
    {CONSTANT_A661_DEACTIVATE,                          0x04},
    {CONSTANT_A661_DIRECTION,                           0xB713},
    {CONSTANT_A661_DONT_RUN,                            0x00},
    {CONSTANT_A661_DOWN,                                0x1A},
    {CONSTANT_A661_DRAW_LINE_TO_CURSOR,                 0x36},
    {CONSTANT_A661_EASING_IN,                           0x01},
    {CONSTANT_A661_EASING_INOUT,                        0x03},
    {CONSTANT_A661_EASING_OUT,                          0x02},
    {CONSTANT_A661_EDB_ALL_CHANGE,                      0x01},
    {CONSTANT_A661_EDB_CHANGE_CONFIRMED,                0x00},
    {CONSTANT_A661_EDB_OPEN_CLOSE,                      0x02},
    {CONSTANT_A661_EDITING,                             0x1B},
    {CONSTANT_A661_EFFECTIVITY_ARRAY,                   0xB6C8},
    {CONSTANT_A661_ENABLE,                              0xB180},
    {CONSTANT_A661_ENABLE_ENTRY_ARRAY,                  0xB1A8},
    {CONSTANT_A661_ENABLE_HANDLER,                      0xB1A1},
    {CONSTANT_A661_END,                                 0x03},
    {CONSTANT_A661_END_ANGLE,                           0xB1B0},
    {CONSTANT_A661_END_SCALED_VALUE,                    0xB505},
    {CONSTANT_A661_END_VALUE,                           0xB503},
    {CONSTANT_A661_ENTRY_ARRAY,                         0xB190},
    {CONSTANT_A661_ENTRY_POP_UP_ARRAY,                  0xB1C0},
    {CONSTANT_A661_ENTRY_VALID,                         0xB570},
    {CONSTANT_A661_ERROR,                               0x1C},
    {CONSTANT_A661_EVENT_FLAG,                          0xB1D0},
    {CONSTANT_A661_EVENT_NONE,                          0x00},
    {CONSTANT_A661_EXCLUDED_CIRCLE,                     0x03},
    {CONSTANT_A661_EXCLUDED_RECTANGLE,                  0x01},
    {CONSTANT_A661_EXCLUDED_REGIONS_LIST,               0xB6C9},
    {CONSTANT_A661_EXCLUDED_TRIANGLE,                   0x02},
    {CONSTANT_A661_FALSE,                               0x00},
    {CONSTANT_A661_FILLED,                              0xB1E1},
    {CONSTANT_A661_FILLED_POLY_START,                   0x2B},
    {CONSTANT_A661_FILLED_POLY_START_INTERACTIVE,       0xAB},
    {CONSTANT_A661_FILL_INDEX,                          0xB1E0},
    {CONSTANT_A661_FINE_PAGE_DELTA,                     0xB6E2},
    {CONSTANT_A661_FIRST_ACCESS_ENTRY,                  0xB1F0},
    {CONSTANT_A661_FIRST_ACCESS_UA_ENTRY,               0xB1F8},
    {CONSTANT_A661_FIRST_VISIBLE_ENTRY,                 0xB200},
    {CONSTANT_A661_FONT,                                0xB590},
    {CONSTANT_A661_FORMAT_STRING,                       0xB550},
    {CONSTANT_A661_FRAME_X,                             0xB210},
    {CONSTANT_A661_FRAME_XY,                            0xB230},
    {CONSTANT_A661_FRAME_Y,                             0xB220},
    {CONSTANT_A661_FROM,                                0x05},
    {CONSTANT_A661_FROM_ANGLE,                          0xB707},
    {CONSTANT_A661_FROM_SCALE_X,                        0xB708},
    {CONSTANT_A661_FROM_SCALE_XY,                       0xB715},
    {CONSTANT_A661_FROM_SCALE_Y,                        0xB709},
    {CONSTANT_A661_FROM_VALUE,                          0xB70A},
    {CONSTANT_A661_FROM_VALUE_X,                        0xB70B},
    {CONSTANT_A661_FROM_VALUE_XY,                       0xB717},
    {CONSTANT_A661_FROM_VALUE_Y,                        0xB70C},
    {CONSTANT_A661_GBK,                                 0x03},
    {CONSTANT_A661_GESTURE_ABORT,                       0x04},
    {CONSTANT_A661_GESTURE_CANDIDATE,                   0x05},
    {CONSTANT_A661_GESTURE_DOUBLE_TAP,                  0x02},
    {CONSTANT_A661_GESTURE_DRAG,                        0x06},
    {CONSTANT_A661_GESTURE_END,                         0x03},
    {CONSTANT_A661_GESTURE_FLICK,                       0x07},
    {CONSTANT_A661_GESTURE_FLICK_DOWN,                  0x0B},
    {CONSTANT_A661_GESTURE_FLICK_LEFT,                  0x08},
    {CONSTANT_A661_GESTURE_FLICK_RIGHT,                 0x09},
    {CONSTANT_A661_GESTURE_FLICK_UP,                    0x0A},
    {CONSTANT_A661_GESTURE_HOLD,                        0x0C},
    {CONSTANT_A661_GESTURE_PINCH,                       0x04},
    {CONSTANT_A661_GESTURE_PRESS_AND_HOLD,              0x03},
    {CONSTANT_A661_GESTURE_ROTATE,                      0x05},
    {CONSTANT_A661_GESTURE_START,                       0x01},
    {CONSTANT_A661_GESTURE_TAP,                         0x01},
    {CONSTANT_A661_GESTURE_UPDATE,                      0x02},
    {CONSTANT_A661_GRID,                                0x03},
    {CONSTANT_A661_HALO,                                0xB1E8},
    {CONSTANT_A661_HIGHLIGHT_ONLY,                      0x04},
    {CONSTANT_A661_HOME_X,                              0xB219},
    {CONSTANT_A661_HOME_Y,                              0xB21A},
    {CONSTANT_A661_HORIZONTAL,                          0x01},
    {CONSTANT_A661_HORIZONTAL_SCROLL,                   0xB217},
    {CONSTANT_A661_HORZ_LINE,                           0x02},
    {CONSTANT_A661_HYPHENATE,                           0x02},
    {CONSTANT_A661_INDEX_OF_FREQUENCY,                  0xB091},
    {CONSTANT_A661_INITIAL_FOCUS,                       0xB6B0},
    {CONSTANT_A661_INNER_RADIUS,                        0xB240},
    {CONSTANT_A661_INNER_STATE_CHECK,                   0xB244},
    {CONSTANT_A661_INNER_STATE_TOGGLE,                  0xB258},
    {CONSTANT_A661_INSET_SIZE,                          0xB051},
    {CONSTANT_A661_IN_BOUNDS,                           0x01},
    {CONSTANT_A661_ITEM_SPACING,                        0xB5C1},
    {CONSTANT_A661_ITEM_STYLE,                          0x20},
    {CONSTANT_A661_ITEM_SYNCHRONIZATION,                0x51},
    {CONSTANT_A661_KEY_ARRAY,                           0xB700},
    {CONSTANT_A661_LAYOUT,                              0xB5F0},
    {CONSTANT_A661_LEFT,                                0x13},
    {CONSTANT_A661_LEFT_TO_RIGHT,                       0x41},
    {CONSTANT_A661_LEGEND,                              0x21},
    {CONSTANT_A661_LEGEND_ALIGNMENT,                    0xB49B},
    {CONSTANT_A661_LEGEND_ANCHOR,                       0x22},
    {CONSTANT_A661_LEGEND_ANCHOR_ROTATED,               0x35},
    {CONSTANT_A661_LEGEND_AND_HIGHLIGHT,                0x05},
    {CONSTANT_A661_LEGEND_AND_POPUP,                    0x03},
    {CONSTANT_A661_LEGEND_AND_POPUP_AND_HIGHLIGHT,      0x07},
    {CONSTANT_A661_LEGEND_AREA_SIZE_X,                  0xB451},
    {CONSTANT_A661_LEGEND_COMBO,                        0x38},
    {CONSTANT_A661_LEGEND_HIGHLIGHT,                    0x58},
    {CONSTANT_A661_LEGEND_ONLY,                         0x01},
    {CONSTANT_A661_LEGEND_POP_UP,                       0x23},
    {CONSTANT_A661_LEGEND_POSITION,                     0xB260},
    {CONSTANT_A661_LEGEND_READOUT,                      0x45},
    {CONSTANT_A661_LEGEND_READOUT_COMBO,                0x48},
    {CONSTANT_A661_LEGEND_READOUT_FORMAT_STRING,        0x44},
    {CONSTANT_A661_LEGEND_REMOVED,                      0xB452},
    {CONSTANT_A661_LINEAR,                              0x00},
    {CONSTANT_A661_LINE_ARC,                            0x24},
    {CONSTANT_A661_LINE_ARC_INTERACTIVE,                0xA4},
    {CONSTANT_A661_LINE_DELTA_X,                        0xB211},
    {CONSTANT_A661_LINE_DELTA_Y,                        0xB212},
    {CONSTANT_A661_LINE_LENGTH,                         0xB270},
    {CONSTANT_A661_LINE_SEGMENT,                        0x25},
    {CONSTANT_A661_LINE_SEGMENT_INTERACTIVE,            0xA5},
    {CONSTANT_A661_LINE_START,                          0x26},
    {CONSTANT_A661_LINE_START_INTERACTIVE,              0xA6},
    {CONSTANT_A661_LOOP_FORWARD,                        0x00},
    {CONSTANT_A661_LOOP_FORWARD_AND_BACKWARD_AND_BLANK, 0x03},
    {CONSTANT_A661_LOOP_FORWARD_AND_BLANK,              0x02},
    {CONSTANT_A661_LOOP_FORWARD_AND_RESET,              0x01},
    {CONSTANT_A661_LSB_MULTIPLE,                        0xB631},
    {CONSTANT_A661_MAJOR_TICK_INTERVAL,                 0xB600},
    {CONSTANT_A661_MAJOR_TICK_REFERENCE,                0xB608},
    {CONSTANT_A661_MAPGRID_CELLSIZE,                    0xB274},
    {CONSTANT_A661_MAPGRID_OFFSET,                      0xB278},
    {CONSTANT_A661_MAP_SYNCHRONIZATION_NUMBER,          0xB277},
    {CONSTANT_A661_MASK_ENABLED,                        0xB290},
    {CONSTANT_A661_MASK_REFERENCE,                      0xB280},
    {CONSTANT_A661_MAX_VALUE,                           0xB610},
    {CONSTANT_A661_MDF_ABSOLUTE,                        0x65},
    {CONSTANT_A661_MDF_BRG_DIST_ACHDG,                  0x60},
    {CONSTANT_A661_MDF_DIST_DIST,                       0x67},
    {CONSTANT_A661_MDF_DIST_DIST_FIX,                   0x6A},
    {CONSTANT_A661_MDF_LAT_LONG,                        0x61},
    {CONSTANT_A661_MDF_LEGACY,                          0x62},
    {CONSTANT_A661_MDF_RELATIVE,                        0x66},
    {CONSTANT_A661_MDF_X_DIST,                          0x68},
    {CONSTANT_A661_MDF_Y_ALT,                           0x69},
    {CONSTANT_A661_MENU_HORIZONTAL,                     0xB640},
    {CONSTANT_A661_MINIMUM_SIZE,                        0x02},
    {CONSTANT_A661_MINMAX_VALUES,                       0xB580},
    {CONSTANT_A661_MINOR_TICK_MULTIPLE,                 0xB630},
    {CONSTANT_A661_MIN_VALUE,                           0xB620},
    {CONSTANT_A661_MULTI_STATE_VALUE,                   0xB5A0},
    {CONSTANT_A661_NEGATIVE_STRING,                     0xB4D4},
    {CONSTANT_A661_NEXT_FOCUSED_WIDGET,                 0xB2B8},
    {CONSTANT_A661_NEXT_FOCUS_IN_APP_ID,                0xB2BA},
    {CONSTANT_A661_NEXT_FOCUS_IN_ID,                    0xB2BC},
    {CONSTANT_A661_NEXT_FOCUS_IN_LAYER_ID,              0xB2BB},
    {CONSTANT_A661_NORMAL,                              0x1D},
    {CONSTANT_A661_NOT_CLAMPED,                         0x00},
    {CONSTANT_A661_NOT_USED,                            0x00},
    {CONSTANT_A661_NO_SHUFFLE,                          0x84},
    {CONSTANT_A661_NO_SIZING,                           0x74},
    {CONSTANT_A661_NUMBER_OF_ENTRIES,                   0xB2A0},
    {CONSTANT_A661_NUMBER_OF_KEYS,                      0xB701},
    {CONSTANT_A661_NUMBER_OF_PICTURES,                  0xB2A2},
    {CONSTANT_A661_NUMBER_OF_STATES,                    0xB5A1},
    {CONSTANT_A661_NUMBER_OF_SYMBOLS,                   0xB2A4},
    {CONSTANT_A661_NUMBER_OF_TICS_COARSE,               0xB0A2},
    {CONSTANT_A661_NUMBER_OF_TICS_FINE,                 0xB0A4},
    {CONSTANT_A661_NUMBER_OF_TOUCH_POINTS,              0xB0A0},
    {CONSTANT_A661_NUMBER_OF_TURNS,                     0xB714},
    {CONSTANT_A661_NUMBER_OF_UA_ENTRIES,                0xB2A8},
    {CONSTANT_A661_NUMBER_OF_VISIBLE_CHILDREN,          0xB5D0},
    {CONSTANT_A661_NUMERIC_MASK,                        0xB2B0},
    {CONSTANT_A661_OPENING_ENTRY,                       0xB560},
    {CONSTANT_A661_OPENING_MODE,                        0xB561},
    {CONSTANT_A661_OPEN_CENTERED,                       0x17},
    {CONSTANT_A661_OPEN_DOWN,                           0x18},
    {CONSTANT_A661_OPEN_ENTRY_ARRAY,                    0xB627},
    {CONSTANT_A661_OPEN_UP,                             0x16},
    {CONSTANT_A661_OPERATOR_ADD_ASSIGN_ARG,             0x0B},
    {CONSTANT_A661_OPERATOR_ADD_ASSIGN_SRC,             0x08},
    {CONSTANT_A661_OPERATOR_ASSIGNMENT,                 0x00},
    {CONSTANT_A661_OPERATOR_CONSTANT,                   0x02},
    {CONSTANT_A661_OPERATOR_INVERSE,                    0x01},
    {CONSTANT_A661_OPERATOR_LOGICAL_AND,                0x06},
    {CONSTANT_A661_OPERATOR_LOGICAL_OR,                 0x07},
    {CONSTANT_A661_OPERATOR_PRODUCT,                    0x05},
    {CONSTANT_A661_OPERATOR_PROD_ASSIGN_ARG,            0x0D},
    {CONSTANT_A661_OPERATOR_PROD_ASSIGN_SRC,            0x0A},
    {CONSTANT_A661_OPERATOR_SUB_ASSIGN_ARG,             0x0C},
    {CONSTANT_A661_OPERATOR_SUB_ASSIGN_SRC,             0x09},
    {CONSTANT_A661_OPERATOR_SUM,                        0x04},
    {CONSTANT_A661_ORIENTATION,                         0xB1D2},
    {CONSTANT_A661_OUTER_RADIUS,                        0xB2D0},
    {CONSTANT_A661_OUT_OF_BOUNDS,                       0x00},
    {CONSTANT_A661_PAGE_CONTROL_POSITION,               0xB6E1},
    {CONSTANT_A661_PAGE_DELTA_X,                        0xB213},
    {CONSTANT_A661_PAGE_DELTA_Y,                        0xB214},
    {CONSTANT_A661_PAGE_WRAP_TYPE,                      0xB6E0},
    {CONSTANT_A661_PARALLEL,                            0x00},
    {CONSTANT_A661_PARAM_BUFFER_INDEX,                  0xB111},
    {CONSTANT_A661_PARENT_ENTRY_ARRAY,                  0xB628},
    {CONSTANT_A661_PARKING_LINE_END,                    0xBB},
    {CONSTANT_A661_PARKING_LINE_START,                  0xBA},
    {CONSTANT_A661_PICTURE_ARRAY,                       0xB2E0},
    {CONSTANT_A661_PICTURE_ENTRY_ARRAY,                 0xB2E8},
    {CONSTANT_A661_PICTURE_POSITION,                    0xB2F1},
    {CONSTANT_A661_PICTURE_REFERENCE,                   0xB2F0},
    {CONSTANT_A661_PIX_FMT_COLOR_INDEXED_8,             0xC2},
    {CONSTANT_A661_PIX_FMT_LUMINANCE_ALPHA_8,           0xC1},
    {CONSTANT_A661_PIX_FMT_RGBA_8,                      0xC0},
    {CONSTANT_A661_PLAY,                                0x01},
    {CONSTANT_A661_POPUP_AND_HIGHLIGHT,                 0x06},
    {CONSTANT_A661_POPUP_ONLY,                          0x02},
    {CONSTANT_A661_POPUP_POS_X,                         0xB1C3},
    {CONSTANT_A661_POPUP_POS_Y,                         0xB1C4},
    {CONSTANT_A661_POPUP_SIZE_X,                        0xB1C5},
    {CONSTANT_A661_POPUP_SIZE_Y,                        0xB1C6},
    {CONSTANT_A661_POSITIVE_STRING,                     0xB4D3},
    {CONSTANT_A661_POS_X,                               0xB300},
    {CONSTANT_A661_POS_X2,                              0xB330},
    {CONSTANT_A661_POS_X3,                              0xB360},
    {CONSTANT_A661_POS_XY,                              0xB320},
    {CONSTANT_A661_POS_XY2,                             0xB350},
    {CONSTANT_A661_POS_XY3,                             0xB380},
    {CONSTANT_A661_POS_Y,                               0xB310},
    {CONSTANT_A661_POS_Y2,                              0xB340},
    {CONSTANT_A661_POS_Y3,                              0xB370},
    {CONSTANT_A661_PRESERVE_CHARACTER,                  0x01},
    {CONSTANT_A661_PRESERVE_WORD,                       0x00},
    {CONSTANT_A661_PROJECTION_REFERENCE,                0xB1D3},
    {CONSTANT_A661_PRP_LAT,                             0xB390},
    {CONSTANT_A661_PRP_LAT_LONG,                        0xB3B0},
    {CONSTANT_A661_PRP_LONG,                            0xB3A0},
    {CONSTANT_A661_PRP_SCREEN_X,                        0xB3C0},
    {CONSTANT_A661_PRP_SCREEN_XY,                       0xB3E0},
    {CONSTANT_A661_PRP_SCREEN_Y,                        0xB3D0},
    {CONSTANT_A661_PRP_X,                               0xB3B2},
    {CONSTANT_A661_PRP_XY,                              0xB3B4},
    {CONSTANT_A661_PRP_Y,                               0xB3B3},
    {CONSTANT_A661_RADIUS,                              0xB3F0},
    {CONSTANT_A661_RANGE,                               0xB1D1},
    {CONSTANT_A661_RANGE_X,                             0xB402},
    {CONSTANT_A661_RANGE_XY,                            0xB404},
    {CONSTANT_A661_RANGE_Y,                             0xB403},
    {CONSTANT_A661_REFRESH,                             0xB670},
    {CONSTANT_A661_REF_POS_X,                           0xB690},
    {CONSTANT_A661_REF_POS_XY,                          0xB691},
    {CONSTANT_A661_REF_POS_Y,                           0xB692},
    {CONSTANT_A661_REORDER,                             0xB0AB},
    {CONSTANT_A661_REPORT_ALL,                          0x61},
    {CONSTANT_A661_REPORT_ALL_WITH_CANDIDATE,           0x64},
    {CONSTANT_A661_REPORT_ON_CDS,                       0x63},
    {CONSTANT_A661_REPORT_ON_COMPLETION,                0x62},
    {CONSTANT_A661_REPORT_ON_TRANSITION,                0x60},
    {CONSTANT_A661_REPORT_ON_TRANSITION_WITH_CANDIDATE, 0x65},
    {CONSTANT_A661_REP_DEFAULT,                         0x00},
    {CONSTANT_A661_REP_FOCUS,                           0x02},
    {CONSTANT_A661_REP_HIGHLIGHT,                       0x03},
    {CONSTANT_A661_REP_STANDARD,                        0x01},
    {CONSTANT_A661_RIGHT,                               0x14},
    {CONSTANT_A661_RIGHT_TO_LEFT,                       0x42},
    {CONSTANT_A661_ROTATION_ANGLE,                      0xB410},
    {CONSTANT_A661_RUN,                                 0x01},
    {CONSTANT_A661_RUN_ONCE,                            0x02},
    {CONSTANT_A661_SAME_LEVEL,                          0x02},
    {CONSTANT_A661_SCALE_X,                             0xB0AC},
    {CONSTANT_A661_SCALE_XY,                            0xB0AE},
    {CONSTANT_A661_SCALE_Y,                             0xB0AD},
    {CONSTANT_A661_SCREEN_RANGE,                        0xB420},
    {CONSTANT_A661_SELECTED,                            0x01},
    {CONSTANT_A661_SELECTED_ENTRY,                      0xB430},
    {CONSTANT_A661_SELECTED_ENTRY_ARRAY,                0xB439},
    {CONSTANT_A661_SELECTING_HEIGHT,                    0xB431},
    {CONSTANT_A661_SELECTING_WIDTH,                     0xB432},
    {CONSTANT_A661_SELECT_ALL,                          0xB438},
    {CONSTANT_A661_SEQUENTIAL,                          0x01},
    {CONSTANT_A661_SEQUENTIAL_FROM,                     0x02},
    {CONSTANT_A661_SHEET_SIZE_X,                        0xB215},
    {CONSTANT_A661_SHEET_SIZE_Y,                        0xB216},
    {CONSTANT_A661_SHIFT_FIRST_VISIBLE_ENTRY,           0xB440},
    {CONSTANT_A661_SHOW_FAIL,                           0xB671},
    {CONSTANT_A661_SHOW_MAJOR_TICKS,                    0xB601},
    {CONSTANT_A661_SHUFFLE_DOWN,                        0x81},
    {CONSTANT_A661_SHUFFLE_LEFT,                        0x82},
    {CONSTANT_A661_SHUFFLE_RIGHT,                       0x83},
    {CONSTANT_A661_SHUFFLE_TO_FIT_MODE,                 0xB5E0},
    {CONSTANT_A661_SHUFFLE_UP,                          0x80},
    {CONSTANT_A661_SIZE_BOTTOM_UP,                      0x71},
    {CONSTANT_A661_SIZE_LEFT_TO_RIGHT,                  0x72},
    {CONSTANT_A661_SIZE_RIGHT_TO_LEFT,                  0x73},
    {CONSTANT_A661_SIZE_TOP_DOWN,                       0x70},
    {CONSTANT_A661_SIZE_TO_FIT_MODE,                    0xB5C0},
    {CONSTANT_A661_SIZE_X,                              0xB450},
    {CONSTANT_A661_SIZE_XY,                             0xB470},
    {CONSTANT_A661_SIZE_Y,                              0xB460},
    {CONSTANT_A661_SOURCE_DX,                           0xB662},
    {CONSTANT_A661_SOURCE_DXDY,                         0xB665},
    {CONSTANT_A661_SOURCE_DY,                           0xB663},
    {CONSTANT_A661_SOURCE_REF,                          0xB666},
    {CONSTANT_A661_SOURCE_WIDGET_PARAM,                 0x00},
    {CONSTANT_A661_SOURCE_X,                            0xB660},
    {CONSTANT_A661_SOURCE_XY,                           0xB664},
    {CONSTANT_A661_SOURCE_Y,                            0xB661},
    {CONSTANT_A661_STAR,                                0x04},
    {CONSTANT_A661_START_ANGLE,                         0xB480},
    {CONSTANT_A661_START_SCALED_VALUE,                  0xB504},
    {CONSTANT_A661_START_VALUE,                         0xB502},
    {CONSTANT_A661_STOP_BACKWARD,                       0xB6B2},
    {CONSTANT_A661_STOP_FORWARD,                        0xB6B1},
    {CONSTANT_A661_STRING,                              0xB490},
    {CONSTANT_A661_STRING_ALTERNATE,                    0xB498},
    {CONSTANT_A661_STRING_ARRAY,                        0xB4A0},
    {CONSTANT_A661_STYLE_SET,                           0xB4B0},
    {CONSTANT_A661_STYLE_SET_ARRAY,                     0xB4B1},
    {CONSTANT_A661_STYLE_SET_ARRAY_SIZE,                0xB4B2},
    {CONSTANT_A661_STYLE_SET_DEFAULT,                   0x00},
    {CONSTANT_A661_SYMBOL_CIRCLE,                       0x27},
    {CONSTANT_A661_SYMBOL_CIRCLE_INTERACTIVE,           0xA7},
    {CONSTANT_A661_SYMBOL_ENTRY_ARRAY,                  0xB2E9},
    {CONSTANT_A661_SYMBOL_GENERIC,                      0x28},
    {CONSTANT_A661_SYMBOL_GENERIC_INTERACTIVE,          0xA8},
    {CONSTANT_A661_SYMBOL_OVAL,                         0x2C},
    {CONSTANT_A661_SYMBOL_OVAL_INTERACTIVE,             0xAC},
    {CONSTANT_A661_SYMBOL_PARKABLE,                     0xB8},
    {CONSTANT_A661_SYMBOL_PARKABLE_INTERACTIVE,         0xB9},
    {CONSTANT_A661_SYMBOL_POSITION,                     0xB4C2},
    {CONSTANT_A661_SYMBOL_RECTANGLE,                    0x37},
    {CONSTANT_A661_SYMBOL_RECTANGLE_INTERACTIVE,        0xB7},
    {CONSTANT_A661_SYMBOL_REFERENCE,                    0xB4C0},
    {CONSTANT_A661_SYMBOL_ROTATED,                      0x29},
    {CONSTANT_A661_SYMBOL_ROTATED_INTERACTIVE,          0xA9},
    {CONSTANT_A661_SYMBOL_ROTATED_PARKABLE,             0xBE},
    {CONSTANT_A661_SYMBOL_ROTATED_PARKABLE_INTERACTIVE, 0xBF},
    {CONSTANT_A661_SYMBOL_RUNWAY,                       0x2A},
    {CONSTANT_A661_SYMBOL_RUNWAY_INTERACTIVE,           0xAA},
    {CONSTANT_A661_SYMBOL_TARGET,                       0x2D},
    {CONSTANT_A661_SYMBOL_TARGET_INTERACTIVE,           0xAD},
    {CONSTANT_A661_SYMBOL_TARGET_PARKABLE,              0x3D},
    {CONSTANT_A661_SYMBOL_TARGET_PARKABLE_INTERACTIVE,  0xBD},
    {CONSTANT_A661_SYNC_FRAME,                          0xB4C7},
    {CONSTANT_A661_TAB_DIRECTION_WIDGET_ID_ARRAY,       0xB672},
    {CONSTANT_A661_TAB_POSITION,                        0xB052},
    {CONSTANT_A661_TARGET_WIDGET_ID,                    0xB650},
    {CONSTANT_A661_TICS_COARSE,                         0xB4D0},
    {CONSTANT_A661_TICS_COARSE_8BYTE,                   0xB4D2},
    {CONSTANT_A661_TICS_COARSE_ARRAY,                   0xB0A6},
    {CONSTANT_A661_TICS_FINE,                           0xB4E0},
    {CONSTANT_A661_TICS_FINE_8BYTE,                     0xB4E2},
    {CONSTANT_A661_TICS_FINE_ARRAY,                     0xB0AA},
    {CONSTANT_A661_TO,                                  0x06},
    {CONSTANT_A661_TOP,                                 0x11},
    {CONSTANT_A661_TOP_CENTER,                          0x33},
    {CONSTANT_A661_TOP_LEFT,                            0x34},
    {CONSTANT_A661_TOP_RIGHT,                           0x35},
    {CONSTANT_A661_TOP_TO_BOTTOM,                       0x43},
    {CONSTANT_A661_TOUCH_ABORT,                         0x04},
    {CONSTANT_A661_TOUCH_DOWN,                          0x01},
    {CONSTANT_A661_TOUCH_MOVE,                          0x02},
    {CONSTANT_A661_TOUCH_MOVE_OUTSIDE,                  0x05},
    {CONSTANT_A661_TOUCH_UP,                            0x03},
    {CONSTANT_A661_TO_ANGLE,                            0xB70D},
    {CONSTANT_A661_TO_BACK,                             0x01},
    {CONSTANT_A661_TO_FRONT,                            0x02},
    {CONSTANT_A661_TO_SCALE_X,                          0xB70E},
    {CONSTANT_A661_TO_SCALE_XY,                         0xB716},
    {CONSTANT_A661_TO_SCALE_Y,                          0xB70F},
    {CONSTANT_A661_TO_VALUE,                            0xB710},
    {CONSTANT_A661_TO_VALUE_X,                          0xB711},
    {CONSTANT_A661_TO_VALUE_XY,                         0xB718},
    {CONSTANT_A661_TO_VALUE_Y,                          0xB712},
    {CONSTANT_A661_TRANSLATION_X,                       0xB4F0},
    {CONSTANT_A661_TRANSLATION_XY,                      0xB510},
    {CONSTANT_A661_TRANSLATION_Y,                       0xB500},
    {CONSTANT_A661_TREE_NODE_STYLE_ARRAY,               0xB629},
    {CONSTANT_A661_TRIANGLE_END,                        0x32},
    {CONSTANT_A661_TRIANGLE_END_DOUBLE,                 0x33},
    {CONSTANT_A661_TRIANGLE_END_DOUBLE_INTERACTIVE,     0xB3},
    {CONSTANT_A661_TRIANGLE_END_INTERACTIVE,            0xB2},
    {CONSTANT_A661_TRIANGLE_FAN_START,                  0x34},
    {CONSTANT_A661_TRIANGLE_FAN_START_INTERACTIVE,      0xB4},
    {CONSTANT_A661_TRIANGLE_SEGMENT,                    0x2F},
    {CONSTANT_A661_TRIANGLE_SEGMENT_DOUBLE,             0x31},
    {CONSTANT_A661_TRIANGLE_SEGMENT_DOUBLE_INTERACTIVE, 0xB1},
    {CONSTANT_A661_TRIANGLE_SEGMENT_INTERACTIVE,        0xAF},
    {CONSTANT_A661_TRIANGLE_STRIP_START,                0x2E},
    {CONSTANT_A661_TRIANGLE_STRIP_START_INTERACTIVE,    0xAE},
    {CONSTANT_A661_TRUE,                                0x01},
    {CONSTANT_A661_TRUE_WITH_VALIDATION,                0x02},
    {CONSTANT_A661_UA_POSITION_FLAG,                    0xB1C1},
    {CONSTANT_A661_UNSELECTED,                          0x00},
    {CONSTANT_A661_UNSELECT_ALL,                        0xB43A},
    {CONSTANT_A661_UP,                                  0x19},
    {CONSTANT_A661_UTF8,                                0x02},
    {CONSTANT_A661_VALIDATION,                          0x01},
    {CONSTANT_A661_VALIDATION_AND_WHEEL,                0x02},
    {CONSTANT_A661_VALUE,                               0xB520},
    {CONSTANT_A661_VALUE_8BYTE,                         0xB522},
    {CONSTANT_A661_VERTEX_RENDER_END,                   0x3B},
    {CONSTANT_A661_VERTEX_RENDER_LINES,                 0x01},
    {CONSTANT_A661_VERTEX_RENDER_LINE_LOOP,             0x02},
    {CONSTANT_A661_VERTEX_RENDER_POLYLINE,              0x03},
    {CONSTANT_A661_VERTEX_RENDER_SEGMENT,               0x3A},
    {CONSTANT_A661_VERTEX_RENDER_START,                 0x39},
    {CONSTANT_A661_VERTEX_RENDER_TRIANGLE,              0x04},
    {CONSTANT_A661_VERTEX_RENDER_TRIANGLE_FAN,          0x05},
    {CONSTANT_A661_VERTEX_RENDER_TRIANGLE_STRIP,        0x06},
    {CONSTANT_A661_VERTICAL,                            0x00},
    {CONSTANT_A661_VERTICAL_SCROLL,                     0xB218},
    {CONSTANT_A661_VERTICES_ARRAY,                      0xB528},
    {CONSTANT_A661_VERT_LINE,                           0x03},
    {CONSTANT_A661_VISIBLE,                             0xB530},
    {CONSTANT_A661_VISIBLE_CHILD,                       0xB540},
    {CONSTANT_A661_VISIBLE_CHILD_INDEX,                 0xB680},
    {CONSTANT_A661_WHEEL,                               0x03},
    {CONSTANT_A661_WORD_WRAP,                           0xB4A8},
    {CONSTANT_A661_WRAP_BOTH,                           0x01},
    {CONSTANT_A661_WRAP_FIRST_TO_LAST,                  0x02},
    {CONSTANT_A661_WRAP_LAST_TO_FIRST,                  0x03},
    {CONSTANT_A661_WRAP_NONE,                           0x00},
    {CONSTANT_A661_WRAP_STYLE,                          0xB4A9}
};

/*************************************************************************************************/
/**
 * \brief Widget types each extension may be attached to, from Table 8.4, Possible Widgets for Extension.
 */
static const char_t* const c_boundaryCheckTargets[] =
{
    TYPE_A661_MAPHORZ_ITEMLIST
};

static const char_t* const c_childIndexTargets[] =
{
    TYPE_A661_MUTUALLY_EXCLUSIVE_CONTAINER,
    TYPE_A661_PAGING_CONTAINER,
    TYPE_A661_TABBED_PANEL_GROUP
};

static const char_t* const c_cursorEntryEventsTargets[] =
{
    TYPE_A661_COMBO_BOX,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTOR,
    TYPE_A661_TREE
};

static const char_t* const c_cursorEventsTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SYMBOL_PUSH_BUTTON
};

static const char_t* const c_cursorShapeTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_CURSOR_OVER,
    TYPE_A661_EDIT_BOX_MASKED,
    TYPE_A661_EDIT_BOX_MULTI_LINE,
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_EDIT_BOX_TEXT,
    TYPE_A661_INK_AREA,
    TYPE_A661_KEYBOARD_AREA,
    TYPE_A661_MAPHORZ_ITEMLIST,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SCROLL_WHEEL_AREA,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_SLIDER,
    TYPE_A661_SYMBOL_PUSH_BUTTON,
    TYPE_A661_SYMBOL_TOGGLE_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

static const char_t* const c_directionalTabbingTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_EDIT_BOX_MASKED,
    TYPE_A661_EDIT_BOX_MULTI_LINE,
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_EDIT_BOX_TEXT,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_SLIDER,
    TYPE_A661_SYMBOL_PUSH_BUTTON,
    TYPE_A661_SYMBOL_TOGGLE_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

static const char_t* const c_excludedRegionsTargets[] =
{
    TYPE_A661_MAP_BOUNDARY
};

static const char_t* const c_focusStopTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_EDIT_BOX_MASKED,
    TYPE_A661_EDIT_BOX_MULTI_LINE,
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_EDIT_BOX_TEXT,
    TYPE_A661_INK_AREA,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_SLIDER,
    TYPE_A661_SYMBOL_PUSH_BUTTON,
    TYPE_A661_SYMBOL_TOGGLE_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

static const char_t* const c_initialFocusTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_EDIT_BOX_MASKED,
    TYPE_A661_EDIT_BOX_MULTI_LINE,
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_EDIT_BOX_TEXT,
    TYPE_A661_INK_AREA,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_SLIDER,
    TYPE_A661_SYMBOL_PUSH_BUTTON,
    TYPE_A661_SYMBOL_TOGGLE_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

static const char_t* const c_legendAlignTargets[] =
{
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_NUMERIC_READOUT
};

static const char_t* const c_maphorzCoordSystemEventTargets[] =
{
    TYPE_A661_MAPHORZ,
    TYPE_A661_MAPHORZ_SOURCE,
    TYPE_A661_MAP_EXTERNAL_SOURCE
};

static const char_t* const c_mapvertCoordSystemEventTargets[] =
{
    TYPE_A661_MAPVERT,
    TYPE_A661_MAPVERT_SOURCE,
    TYPE_A661_MAP_EXTERNAL_SOURCE
};

static const char_t* const c_multiSelectionTargets[] =
{
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_TREE
};

static const char_t* const c_pictureTargets[] =
{
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

static const char_t* const c_reorderTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_BASIC_CONTAINER,
    TYPE_A661_BLINKING_CONTAINER,
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_CONNECTOR,
    TYPE_A661_CURSOR_OVER,
    TYPE_A661_CURSOR_POS_OVERLAY,
    TYPE_A661_CURSOR_REF,
    TYPE_A661_DATA_CONNECTOR,
    TYPE_A661_EDIT_BOX_MASKED,
    TYPE_A661_EDIT_BOX_MULTI_LINE,
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_EDIT_BOX_TEXT,
    TYPE_A661_EXTERNAL_SOURCE,
    TYPE_A661_GESTURE_AREA,
    TYPE_A661_GP_ARC_CIRCLE,
    TYPE_A661_GP_ARC_ELLIPSE,
    TYPE_A661_GP_CROWN,
    TYPE_A661_GP_LINE,
    TYPE_A661_GP_LINE_POLAR,
    TYPE_A661_GP_POLYLINE,
    TYPE_A661_GP_RECTANGLE,
    TYPE_A661_GP_TRIANGLE,
    TYPE_A661_INK_AREA,
    TYPE_A661_KEYBOARD_AREA,
    TYPE_A661_LABEL,
    TYPE_A661_LABEL_COMPLEX,
    TYPE_A661_MAPHORZ,
    TYPE_A661_MAPHORZ_CONTAINER,
    TYPE_A661_MAPHORZ_ITEMLIST,
    TYPE_A661_MAPHORZ_PANEL,
    TYPE_A661_MAPHORZ_SOURCE,
    TYPE_A661_MAPVERT,
    TYPE_A661_MAPVERT_CONTAINER,
    TYPE_A661_MAPVERT_ITEMLIST,
    TYPE_A661_MAPVERT_PANEL,
    TYPE_A661_MAPVERT_SOURCE,
    TYPE_A661_MAP_GRID,
    TYPE_A661_MASK_CONTAINER,
    TYPE_A661_MENU_BAR,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_MUTUALLY_EXCLUSIVE_CONTAINER,
    TYPE_A661_NUMERIC_READOUT,
    TYPE_A661_PAGING_CONTAINER,
    TYPE_A661_PANEL,
    TYPE_A661_PICTURE,
    TYPE_A661_PICTURE_ANIMATED,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_RADIO_BOX,
    TYPE_A661_ROTATION_CONTAINER,
    TYPE_A661_SCALE_CONTAINER,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SCROLL_PANEL,
    TYPE_A661_SCROLL_WHEEL_AREA,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_SHUFFLE_TO_FIT_CONTAINER,
    TYPE_A661_SIZE_TO_FIT_CONTAINER,
    TYPE_A661_SLIDER,
    TYPE_A661_SYMBOL,
    TYPE_A661_SYMBOL_ANIMATED,
    TYPE_A661_SYMBOL_PUSH_BUTTON,
    TYPE_A661_SYMBOL_TOGGLE_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TABBED_PANEL_GROUP,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TOUCH_AREA,
    TYPE_A661_TRANSLATION_CONTAINER,
    TYPE_A661_TREE
};

static const char_t* const c_staticParamBufferTargets[] =
{
    TYPE_A661_BUFFER_FORMAT
};

static const char_t* const c_styleSetTargets[] =
{
    TYPE_A661_ACTIVE_AREA,
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_CURSOR_OVER,
    TYPE_A661_EDIT_BOX_MASKED,
    TYPE_A661_EDIT_BOX_MULTI_LINE,
    TYPE_A661_EDIT_BOX_NUMERIC,
    TYPE_A661_EDIT_BOX_NUMERIC_BCD,
    TYPE_A661_EDIT_BOX_TEXT,
    TYPE_A661_EXTERNAL_SOURCE,
    TYPE_A661_GESTURE_AREA,
    TYPE_A661_GP_ARC_CIRCLE,
    TYPE_A661_GP_ARC_ELLIPSE,
    TYPE_A661_GP_CROWN,
    TYPE_A661_GP_LINE,
    TYPE_A661_GP_LINE_POLAR,
    TYPE_A661_GP_POLYLINE,
    TYPE_A661_GP_RECTANGLE,
    TYPE_A661_GP_TRIANGLE,
    TYPE_A661_INK_AREA,
    TYPE_A661_LABEL,
    TYPE_A661_LABEL_COMPLEX,
    TYPE_A661_MAPHORZ_PANEL,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_NUMERIC_READOUT,
    TYPE_A661_PAGING_CONTAINER,
    TYPE_A661_PANEL,
    TYPE_A661_PICTURE,
    TYPE_A661_PICTURE_ANIMATED,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SCROLL_PANEL,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_SHUFFLE_TO_FIT_CONTAINER,
    TYPE_A661_SIZE_TO_FIT_CONTAINER,
    TYPE_A661_SLIDER,
    TYPE_A661_SYMBOL,
    TYPE_A661_SYMBOL_ANIMATED,
    TYPE_A661_SYMBOL_PUSH_BUTTON,
    TYPE_A661_SYMBOL_TOGGLE_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TABBED_PANEL_GROUP,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TOUCH_AREA,
    TYPE_A661_TREE
};

static const char_t* const c_symbolTargets[] =
{
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_MULTI_STATE_BUTTON,
    TYPE_A661_POP_UP_MENU,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_SELECTOR,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

static const char_t* const c_ticsArrayTargets[] =
{
    TYPE_A661_EDIT_BOX_NUMERIC
};

static const char_t* const c_wordWrapTargets[] =
{
    TYPE_A661_CHECK_BUTTON,
    TYPE_A661_COMBO_BOX,
    TYPE_A661_COMBO_BOX_EDIT,
    TYPE_A661_LABEL,
    TYPE_A661_LABEL_COMPLEX,
    TYPE_A661_NUMERIC_READOUT,
    TYPE_A661_PICTURE_PUSH_BUTTON,
    TYPE_A661_PICTURE_TOGGLE_BUTTON,
    TYPE_A661_POP_UP_MENU_BUTTON,
    TYPE_A661_POP_UP_PANEL,
    TYPE_A661_POP_UP_PANEL_BUTTON,
    TYPE_A661_PUSH_BUTTON,
    TYPE_A661_SCROLL_LIST,
    TYPE_A661_SELECTION_LIST_BUTTON,
    TYPE_A661_TABBED_PANEL,
    TYPE_A661_TOGGLE_BUTTON,
    TYPE_A661_TREE
};

/*************************************************************************************************/
/**
 * \brief Each extension type with its list of widget types, in type name order.
 */
static const ExtensionTargets_t c_extensionTargets[] =
{
    {TYPE_A661_BOUNDARY_CHECK_EXTENSION, c_boundaryCheckTargets, COUNT_OF(c_boundaryCheckTargets)},
    {TYPE_A661_CHILD_INDEX_EXTENSION, c_childIndexTargets, COUNT_OF(c_childIndexTargets)},
    {TYPE_A661_CURSOR_ENTRY_EVENTS_EXTENSION, c_cursorEntryEventsTargets, COUNT_OF(c_cursorEntryEventsTargets)},
    {TYPE_A661_CURSOR_EVENTS_EXTENSION, c_cursorEventsTargets, COUNT_OF(c_cursorEventsTargets)},
    {TYPE_A661_CURSOR_SHAPE_EXTENSION, c_cursorShapeTargets, COUNT_OF(c_cursorShapeTargets)},
    {TYPE_A661_DIRECTIONAL_TABBING_EXTENSION, c_directionalTabbingTargets, COUNT_OF(c_directionalTabbingTargets)},
    {TYPE_A661_EXCLUDED_REGIONS_EXTENSION, c_excludedRegionsTargets, COUNT_OF(c_excludedRegionsTargets)},
    {TYPE_A661_FOCUS_STOP_EXTENSION, c_focusStopTargets, COUNT_OF(c_focusStopTargets)},
    {TYPE_A661_INITIAL_FOCUS_EXTENSION, c_initialFocusTargets, COUNT_OF(c_initialFocusTargets)},
    {TYPE_A661_LEGEND_ALIGN_EXTENSION, c_legendAlignTargets, COUNT_OF(c_legendAlignTargets)},
    {TYPE_A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION, c_maphorzCoordSystemEventTargets, COUNT_OF(c_maphorzCoordSystemEventTargets)},
    {TYPE_A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION, c_mapvertCoordSystemEventTargets, COUNT_OF(c_mapvertCoordSystemEventTargets)},
    {TYPE_A661_MULTI_SELECTION_EXTENSION, c_multiSelectionTargets, COUNT_OF(c_multiSelectionTargets)},
    {TYPE_A661_PICTURE_EXTENSION, c_pictureTargets, COUNT_OF(c_pictureTargets)},
    {TYPE_A661_REORDER_EXTENSION, c_reorderTargets, COUNT_OF(c_reorderTargets)},
    {TYPE_A661_STATIC_PARAM_BUFFER_EXTENSION, c_staticParamBufferTargets, COUNT_OF(c_staticParamBufferTargets)},
    {TYPE_A661_STYLE_SET_EXTENSION, c_styleSetTargets, COUNT_OF(c_styleSetTargets)},
    {TYPE_A661_SYMBOL_EXTENSION, c_symbolTargets, COUNT_OF(c_symbolTargets)},
    {TYPE_A661_TICS_ARRAY_EXTENSION, c_ticsArrayTargets, COUNT_OF(c_ticsArrayTargets)},
    {TYPE_A661_WORD_WRAP_EXTENSION, c_wordWrapTargets, COUNT_OF(c_wordWrapTargets)},
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
 * \param[out] xml               Partitioned temporary storage
 * \param[in]  workspace         Eight-byte-aligned caller storage
 * \param[in]  sizeOfWorkspace   Workspace size
 *
 * \retval     TRUE              if the workspace has enough bytes.
 * \retval     FALSE             if the workspace is too small or misaligned.
 */
static bool_t InitializeXmlWorkspace(XmlWorkspace_t* xml, void* workspace, uint64_t sizeOfWorkspace);

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
 * \param[out] buffer        Output bytes
 * \param[in]  capacity      Output buffer capacity
 * \param[in]  offset        Byte offset of the value
 * \param[in]  value         Value to store
 * \param[in]  sizeOfValue   Number of bytes to store, at most four
 *
 * \retval     None
 */
static void PutUnsigned(byte_t* buffer, uint64_t capacity, uint64_t offset, uint32_t value, uint32_t sizeOfValue);

/*************************************************************************************************/
/**
 * \brief Loads a big-endian unsigned value.
 *
 * \param[in] buffer        Input bytes
 * \param[in] offset        Byte offset of the value
 * \param[in] sizeOfValue   Number of bytes to load, at most four
 *
 * \retval    uint32_t      Loaded value.
 */
static uint32_t GetUnsigned(const byte_t* buffer, uint64_t offset, uint32_t sizeOfValue);

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
 * \brief Rounds a byte count up to the 32-bit alignment of blocks and commands.
 *
 * \param[in] size   Byte count
 *
 * \retval    uint64_t   Smallest multiple of BINARY_WORD_BYTES that is at least size.
 */
static uint64_t AlignToWord(uint64_t size);

/*************************************************************************************************/
/**
 * \brief Stores bytes given as hexadecimal digits.
 *
 * \param[in]  text          Zero-terminated digits, two per byte
 * \param[in]  sizeOfValue   Number of bytes the text must hold
 * \param[out] buffer        Output bytes
 * \param[in]  capacity      Output buffer capacity
 * \param[in]  offset        Byte offset of the first byte
 *
 * \retval     TRUE          if the text holds exactly sizeOfValue bytes of valid digits.
 * \retval     FALSE         otherwise.
 */
static bool_t PutHexBytes(const char_t* text, uint64_t sizeOfValue, byte_t* buffer, uint64_t capacity, uint64_t offset);

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
 * \brief Returns the entry count of an array field, read earlier in the same command.
 *
 * \param[in] type     Widget or extension type of the command
 * \param[in] field    Array field; its countName names a uchar or ushort field before it
 * \param[in] values   Values of the type's earlier uchar and ushort fields, by field index
 *
 * \retval    uint32_t   Value of the count parameter.
 */
static uint32_t GetEntryCount(const BinaryWidget_t* type, const BinaryField_t* field, const uint32_t* values);

/*************************************************************************************************/
/**
 * \brief Returns the public parameter kind of a Creation Structure field type.
 *
 * \param[in] fieldType   Type of a named field; never BINARY_FIELD_PAD or BINARY_FIELD_ALIGN
 *
 * \retval    Arinc661_ParameterKind_t   Matching parameter kind.
 */
static Arinc661_ParameterKind_t FieldTypeToParameterKind(BinaryFieldType_t fieldType);

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
 * \param[in,out] definition    Caller-owned Definition File data
 * \param[in]     owner         Owning model kind
 * \param[in]     ownerIndex    Layer or widget index, or zero for the Definition File
 * \param[in]     name          Zero-terminated property name
 * \param[in]     value         Value bytes
 * \param[in]     sizeOfValue   Number of value bytes
 *
 * \retval        TRUE          if the property was appended.
 * \retval        FALSE         if the property array is full or a string does not fit.
 */
static bool_t AddProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value, uint64_t sizeOfValue);

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
 * \param[in,out] definition    Caller-owned Definition File data
 * \param[in]     name          Zero-terminated property name
 * \param[in]     value         Bytes to encode
 * \param[in]     sizeOfValue   Number of bytes in value
 *
 * \retval        TRUE          if the property was appended.
 * \retval        FALSE         if the property array is full or the digits do not fit a value.
 */
static bool_t AddHexProperty(Arinc661_Definition_t* definition, const char_t* name, const byte_t* value, uint64_t sizeOfValue);

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
 * \param[in,out] definition           Caller-owned Definition File data
 * \param[in]     buffer               Input bytes
 * \param[in]     sizeOfBinaryBuffer   Number of input bytes
 * \param[in,out] offset               Byte offset of the block, advanced past it
 *
 * \retval        TRUE                 if the layer and its widgets were read.
 * \retval        FALSE                if the block is malformed or storage is insufficient.
 */
static bool_t ReadLayer(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint64_t* offset);

/*************************************************************************************************/
/**
 * \brief Moves past one picture of a picture block (Table 7-2).
 *
 * \param[in]     buffer               Input bytes
 * \param[in]     sizeOfBinaryBuffer   Number of input bytes
 * \param[in]     pixelFormat          Table 7-3 pixel format of the block's pictures
 * \param[in,out] cursor               Offset of the picture code; on success, the offset after its end code
 *
 * \retval        TRUE                 if the picture lies inside the input and ends with A661_END_PICTURE.
 * \retval        FALSE                otherwise.
 */
static bool_t MeasurePicture(const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint32_t pixelFormat, uint64_t* cursor);

/*************************************************************************************************/
/**
 * \brief Measures a picture block (Tables 7-1 and 7-2).
 *
 * \param[in]  buffer               Input bytes
 * \param[in]  sizeOfBinaryBuffer   Number of input bytes
 * \param[in]  offset               Byte offset of the block
 * \param[out] blockSize            Number of bytes in the block
 *
 * \retval     TRUE                 if the block is well formed and complete.
 * \retval     FALSE                if it is truncated or uses an unknown pixel format.
 */
static bool_t MeasurePictureBlock(const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint64_t offset, uint64_t* blockSize);

/*************************************************************************************************/
/**
 * \brief Copies a symbol, picture, or animation law block into the caller's block storage.
 *
 * \param[in,out] definition           Caller-owned Definition File data
 * \param[in]     buffer               Input bytes
 * \param[in]     sizeOfBinaryBuffer   Number of input bytes
 * \param[in,out] offset               Byte offset of the block, advanced past it
 *
 * \retval        TRUE                 if the block was measured and copied.
 * \retval        FALSE                if the block is malformed or block storage is insufficient.
 */
static bool_t ReadOpaqueBlock(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint64_t* offset);

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
 * workspace -> [ XML nodes ][ XML attributes ][ widget node pointers ][ text bytes ]
 *              xml.nodes    xml.attributes    xml.widgetNodes         xml.text
 * \endverbatim
 *
 *  The text area stores zero-terminated numeric attribute values for writers.
 *  At least one text byte must remain after the fixed arrays. Misalignment or
 *  insufficient space returns FALSE without initializing the workspace.
 *  Successful initialization clears the counts and document links; array contents
 *  are initialized as entries are used.
 */
static bool_t InitializeXmlWorkspace(XmlWorkspace_t* xml, void* workspace, uint64_t sizeOfWorkspace)
{
    uint64_t node_bytes;
    uint64_t attribute_bytes;
    uint64_t widget_bytes;
    uint64_t reserved_bytes;
    bool_t   is_aligned;
    bool_t   has_capacity;

    // Reserve the fixed arrays before assigning the remaining workspace to text
    node_bytes      = (uint64_t)ARINC661_XML_NODE_CAPACITY * sizeof(FACEOE_Utl_Xml_Node_t);
    attribute_bytes = (uint64_t)ARINC661_XML_ATTRIBUTE_CAPACITY * sizeof(FACEOE_Utl_Xml_Attribute_t);
    widget_bytes    = (uint64_t)ARINC661_XML_WIDGET_CAPACITY * sizeof(FACEOE_Utl_Xml_Node_t*);
    reserved_bytes  = ((node_bytes + attribute_bytes) + widget_bytes);
    is_aligned      = (0 == ((uintptr_t)workspace % sizeof(uint64_t)));
    has_capacity    = (reserved_bytes < sizeOfWorkspace);
    if (is_aligned && has_capacity)
    {
        // Each region points into the supplied workspace; no separate allocation is made
        xml->nodes          = (FACEOE_Utl_Xml_Node_t*)workspace;
        xml->attributes     = (FACEOE_Utl_Xml_Attribute_t*)((byte_t*)workspace + node_bytes);
        xml->widgetNodes    = (FACEOE_Utl_Xml_Node_t**)((byte_t*)workspace + (node_bytes + attribute_bytes));
        xml->text           = (byte_t*)workspace + reserved_bytes;
        xml->textCapacity   = sizeOfWorkspace - reserved_bytes;
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
    uint32_t               node_index;

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
    uint32_t                          byte_count;
    uint32_t                          attribute_index;
    bool_t                            has_length;
    bool_t                            has_name;

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
    bool_t                            fits;

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
    int64_t                           parsed_value;
    int32_t                           consumed;
    bool_t                            is_valid;

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
    uint32_t                    byte_count;
    bool_t                      has_storage;
    bool_t                      has_value;
    bool_t                      is_valid;

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
        byte_count = FACEOE_Utl_Print_Int64(text, XML_INTEGER_TEXT_BYTES, value);
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
    uint32_t               node_index;

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
    uint32_t               ancestor_index;

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
    bool_t               is_valid;

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
    uint32_t               entry_index;
    uint32_t               field_index;
    bool_t                 is_valid;

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
    uint32_t               node_index;
    bool_t                 is_valid;

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
    uint32_t                   match_index;
    uint32_t                   candidate_index;

    property    = &definition->properties[propertyIndex];
    match_index = UINT32_MAX;
    for (candidate_index = startIndex; candidate_index < definition->propertyCount; ++candidate_index)
    {
        candidate = &definition->properties[candidate_index];
        if ((property->owner == candidate->owner) && (property->ownerIndex == candidate->ownerIndex) && (0 == strncmp(property->name, candidate->name, sizeof(property->name))))
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
    FACEOE_Utl_Xml_Node_t*     model;
    FACEOE_Utl_Xml_Node_t*     property_node;
    FACEOE_Utl_Xml_Node_t*     entry_node;
    const Arinc661_Property_t* property;
    const char_t*              element_name;
    uint32_t                   property_index;
    uint32_t                   entry_index;
    uint32_t                   element_index;
    bool_t                     is_owner;
    bool_t                     has_index;
    bool_t                     is_array;
    bool_t                     is_valid;

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
static void PutUnsigned(byte_t* buffer, uint64_t capacity, uint64_t offset, uint32_t value, uint32_t sizeOfValue)
{
    uint32_t byte_index;
    bool_t   has_room;

    LOGASSERT(BINARY_WORD_BYTES >= sizeOfValue);
    has_room = (offset <= capacity) && (sizeOfValue <= (capacity - offset));
    if (has_room)
    {
        for (byte_index = 0; byte_index < sizeOfValue; ++byte_index)
        {
            buffer[offset + byte_index] = (byte_t)(value >> (CHAR_BIT * (sizeOfValue - 1U - byte_index)));
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Loads the most significant byte first. The caller has already checked that
 *  the bytes lie inside the input.
 */
static uint32_t GetUnsigned(const byte_t* buffer, uint64_t offset, uint32_t sizeOfValue)
{
    uint32_t value;
    uint32_t byte_index;

    LOGASSERT(BINARY_WORD_BYTES >= sizeOfValue);
    value = 0;
    for (byte_index = 0; byte_index < sizeOfValue; ++byte_index)
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
 *  Adds the bytes missing to the next multiple of four, for example 6 becomes 8
 *  and 8 stays 8.
 */
static uint64_t AlignToWord(uint64_t size)
{
    uint64_t remainder;
    uint64_t aligned_size;

    remainder    = size % BINARY_WORD_BYTES;
    aligned_size = size;
    if (0 != remainder)
    {
        aligned_size += (BINARY_WORD_BYTES - remainder);
    }

    return aligned_size;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts uppercase digits only, exactly two per byte and nothing else, so
 *  "1B4200" stores three bytes. The length check keeps the terminator, which
 *  strchr would also find, out of reach.
 */
static bool_t PutHexBytes(const char_t* text, uint64_t sizeOfValue, byte_t* buffer, uint64_t capacity, uint64_t offset)
{
    const char_t* high_digit;
    const char_t* low_digit;
    uint64_t      byte_index;
    bool_t        is_valid;

    is_valid = (strlen(text) == (sizeOfValue * HEX_DIGITS_PER_BYTE));
    if (is_valid)
    {
        for (byte_index = 0; byte_index < sizeOfValue; ++byte_index)
        {
            high_digit = strchr(c_binaryHexDigits, text[byte_index * HEX_DIGITS_PER_BYTE]);
            low_digit  = strchr(c_binaryHexDigits, text[(byte_index * HEX_DIGITS_PER_BYTE) + 1U]);
            is_valid   = (NULL != high_digit) && (NULL != low_digit);
            if (FALSE == is_valid)
            {
                break;
            }
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
        if (0 == strncmp(name, table[type_index].name, ARINC661_MAX_NAME_BYTES))
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
        if ((owner == candidate->owner) && (ownerIndex == candidate->ownerIndex) && (0 == strncmp(name, candidate->name, sizeof(candidate->name))))
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
                if (0 == strncmp(property->value, c_binaryConstants[constant_index].name, sizeof(property->value)))
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
 *  The count parameter always precedes its array in the Creation Structure, so
 *  its value is in values by the time the array is reached; for example,
 *  EntryList is counted by NumberOfEntries.
 */
static uint32_t GetEntryCount(const BinaryWidget_t* type, const BinaryField_t* field, const uint32_t* values)
{
    uint32_t field_index;

    LOGASSERT(NULL != field->countName);
    for (field_index = 0; field_index < type->fieldCount; ++field_index)
    {
        if ((NULL != type->fields[field_index].name) && (0 == strncmp(field->countName, type->fields[field_index].name, ARINC661_MAX_NAME_BYTES)))
        {
            break;
        }
    }
    LOGASSERT(field_index < type->fieldCount);

    return values[field_index];
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Pads and alignment entries have no name, so they never reach a caller
 *  through Arinc661_Definition_GetParameter; reaching one here is a fatal error.
 */
static Arinc661_ParameterKind_t FieldTypeToParameterKind(BinaryFieldType_t fieldType)
{
    Arinc661_ParameterKind_t kind;

    switch (fieldType)
    {
        case BINARY_FIELD_UCHAR:         kind = ARINC661_PARAMETER_UCHAR;         break;
        case BINARY_FIELD_USHORT:        kind = ARINC661_PARAMETER_USHORT;        break;
        case BINARY_FIELD_LONG:          kind = ARINC661_PARAMETER_LONG;          break;
        case BINARY_FIELD_ULONG:         kind = ARINC661_PARAMETER_ULONG;         break;
        case BINARY_FIELD_FLOAT:         kind = ARINC661_PARAMETER_FLOAT;         break;
        case BINARY_FIELD_FR180:         kind = ARINC661_PARAMETER_FR180;         break;
        case BINARY_FIELD_FR32768:       kind = ARINC661_PARAMETER_FR32768;       break;
        case BINARY_FIELD_STRING:        kind = ARINC661_PARAMETER_STRING;        break;
        case BINARY_FIELD_BYTES:         kind = ARINC661_PARAMETER_BYTES;         break;
        case BINARY_FIELD_SIZED_BYTES:   kind = ARINC661_PARAMETER_SIZED_BYTES;   break;
        case BINARY_FIELD_REGION:        kind = ARINC661_PARAMETER_REGION;        break;
        case BINARY_FIELD_LAYER_REQUEST: kind = ARINC661_PARAMETER_LAYER_REQUEST; break;
        case BINARY_FIELD_PAD:
        case BINARY_FIELD_ALIGN:
        default:                         LOGFATAL("Named parameter with a pad type!");
    }

    return kind;
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
            PutUnsigned(buffer, capacity, (*offset) + offsetof(BinarySizedBytes_t, unusedPad), 0, BINARY_USHORT_BYTES);
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset) + sizeof(BinarySizedBytes_t));
        }
        (*offset) += (sizeof(BinarySizedBytes_t) + byte_count);
        PutAlignment(buffer, capacity, offset);
    }
    else if (BINARY_FIELD_REGION == field->type)
    {
        // Read the structure size from its second byte, requiring both digits of that byte to be valid
        byte_count = 0;
        is_valid   = ((sizeof(BinaryRegionHeader_t) * HEX_DIGITS_PER_BYTE) <= strlen(text));
        if (is_valid)
        {
            type_digit = strchr(c_binaryHexDigits, text[(offsetof(BinaryRegionHeader_t, regionType) * HEX_DIGITS_PER_BYTE) + 1U]);
            is_valid   = ('0' == text[offsetof(BinaryRegionHeader_t, regionType) * HEX_DIGITS_PER_BYTE]) && (NULL != type_digit);
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
        is_valid   = (sizeof(BinaryLayerRequest_t) <= byte_count);
        if (is_valid)
        {
            is_valid = PutHexBytes(text, byte_count, buffer, capacity, (*offset));
        }
        if (is_valid)
        {
            request_size = 0;
            for (byte_index = offsetof(BinaryLayerRequest_t, commandSize) * HEX_DIGITS_PER_BYTE; byte_index < ((offsetof(BinaryLayerRequest_t, commandSize) + BINARY_USHORT_BYTES) * HEX_DIGITS_PER_BYTE); ++byte_index)
            {
                request_size = (request_size << HEX_DIGIT_BITS) | (uint32_t)(strchr(c_binaryHexDigits, text[byte_index]) - c_binaryHexDigits);
            }
            is_valid = (byte_count == (sizeof(BinaryLayerRequest_t) + request_size)) && (0 == (request_size % BINARY_WORD_BYTES));
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
    header_bytes  = sizeof(BinaryCreateCommand_t);
    command       = BINARY_CMD_CREATE;
    if (widget->isExtension)
    {
        header_bytes = sizeof(BinaryCommand_t);
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
            if (NULL == field->countName)
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
                element_count = field->elementsPerEntry * GetEntryCount(type, field, values);
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
        PutUnsigned(buffer, capacity, (*offset) + offsetof(BinaryCommand_t, commandSize), (uint32_t)(cursor - (*offset)), BINARY_USHORT_BYTES);
        PutUnsigned(buffer, capacity, (*offset) + offsetof(BinaryCommand_t, type), type->code, BINARY_USHORT_BYTES);
        if (FALSE == widget->isExtension)
        {
            PutUnsigned(buffer, capacity, (*offset) + offsetof(BinaryCreateCommand_t, widgetIdent), (uint32_t)widget_ident, BINARY_USHORT_BYTES);
            PutUnsigned(buffer, capacity, (*offset) + offsetof(BinaryCreateCommand_t, parentIdent), (uint32_t)parent_ident, BINARY_USHORT_BYTES);
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

    byte_count = FACEOE_Utl_Print_Float(text, capacity, value, (uint32_t)precision);
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
static bool_t AddProperty(Arinc661_Definition_t* definition, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value, uint64_t sizeOfValue)
{
    Arinc661_Property_t* property;
    uint64_t             name_byte_count;
    bool_t               is_valid;

    name_byte_count = strlen(name);
    is_valid        = (definition->propertyCount < definition->propertyCapacity);
    if (is_valid)
    {
        is_valid = (name_byte_count < sizeof(property->name)) && (sizeOfValue < sizeof(property->value));
    }
    if (is_valid)
    {
        property = &definition->properties[definition->propertyCount];
        (void)memset(property, 0, sizeof((*property)));
        property->owner      = owner;
        property->ownerIndex = ownerIndex;
        (void)memcpy(property->name, name, name_byte_count);
        (void)memcpy(property->value, value, sizeOfValue);
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

    byte_count = FACEOE_Utl_Print_Int64(text, sizeof(text), value);
    LOGASSERT((0 < byte_count) && ((uint32_t)byte_count < sizeof(text)));

    return AddProperty(definition, owner, ownerIndex, name, text, (uint64_t)byte_count);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Encodes each byte as two uppercase digits, so 0x1B 0x42 0x00 becomes "1B4200".
 *  The widget being read has not been counted yet, so its index is the current count.
 */
static bool_t AddHexProperty(Arinc661_Definition_t* definition, const char_t* name, const byte_t* value, uint64_t sizeOfValue)
{
    char_t   text[ARINC661_MAX_PROPERTY_BYTES];
    uint64_t byte_index;
    bool_t   is_valid;

    is_valid = ((sizeOfValue * HEX_DIGITS_PER_BYTE) < sizeof(text));
    if (is_valid)
    {
        for (byte_index = 0; byte_index < sizeOfValue; ++byte_index)
        {
            text[byte_index * HEX_DIGITS_PER_BYTE]        = c_binaryHexDigits[value[byte_index] >> HEX_DIGIT_BITS];
            text[(byte_index * HEX_DIGITS_PER_BYTE) + 1U] = c_binaryHexDigits[value[byte_index] & HEX_DIGIT_MASK];
        }
        is_valid = AddProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, name, text, sizeOfValue * HEX_DIGITS_PER_BYTE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Converts one parameter to property text, the inverse of WriteField:
 *
 * \verbatim
 * Parameter     Bytes                                      Property value
 * uchar         15                                         "21"
 * ushort        1221                                       "4641"
 * long          FFFFFFFF                                   "-1"
 * ulong         00002710                                   "10000"
 * float         3FC00000                                   "1.5"
 * fr(180)       40000000                                   "90"
 * string        59 4F 55 00                                "YOU"
 * sized bytes   0003 0000 0A0B0C 00                        "0A0B0C"
 * region        0001 0000 00000000 00000000 00000064 ...   "0001000000000000000000000000006400000064"
 * UnusedPad     00                                         none; skipped
 * \endverbatim
 *
 *  Integers become decimal numbers; constant names such as A661_TRUE are not
 *  restored because the same number has different names for different
 *  parameters. Floats and fr() angles use the fewest decimal digits that
 *  reproduce the stored bits exactly. A string extends to its zero terminator,
 *  which must lie inside the command. Bytes without a numeric type, sized
 *  buffers, and excluded regions become uppercase hexadecimal digits. Unused
 *  pads and alignment bytes are skipped without checking their content.
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
    bool_t    has_buffer_bytes;
    bool_t    has_region_bytes;
    bool_t    has_request_bytes;
    bool_t    is_valid;

    // Use the current count as the index of the widget being read, which is not counted yet
    widget_index = definition->widgetCount;
    LOGASSERT((*offset) <= endOffset);
    remaining = endOffset - (*offset);
    if (BINARY_FIELD_ALIGN == field->type)
    {
        (*offset) = AlignToWord(*offset);
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
        is_valid = (sizeof(BinarySizedBytes_t) <= remaining);
        if (is_valid)
        {
            byte_count       = GetUnsigned(buffer, (*offset), BINARY_USHORT_BYTES);
            has_buffer_bytes = (byte_count <= (remaining - sizeof(BinarySizedBytes_t)));
            is_valid         = has_buffer_bytes;
        }
        if (is_valid)
        {
            is_valid = AddHexProperty(definition, field->name, &buffer[(*offset) + sizeof(BinarySizedBytes_t)], byte_count);
            (*offset) = AlignToWord((*offset) + sizeof(BinarySizedBytes_t) + byte_count);
        }
        if (is_valid)
        {
            is_valid = ((*offset) <= endOffset);
        }
    }
    else if (BINARY_FIELD_REGION == field->type)
    {
        is_valid = (sizeof(BinaryRegionHeader_t) <= remaining);
        if (is_valid)
        {
            byte_count       = RegionByteCount(buffer[(*offset) + offsetof(BinaryRegionHeader_t, regionType)]);
            has_region_bytes = (byte_count <= remaining);
            is_valid         = (0 != byte_count) && has_region_bytes;
        }
        if (is_valid)
        {
            is_valid = AddHexProperty(definition, field->name, &buffer[(*offset)], byte_count);
            (*offset) += byte_count;
        }
    }
    else if (BINARY_FIELD_LAYER_REQUEST == field->type)
    {
        is_valid = (sizeof(BinaryLayerRequest_t) <= remaining);
        if (is_valid)
        {
            byte_count        = sizeof(BinaryLayerRequest_t) + GetUnsigned(buffer, (*offset) + offsetof(BinaryLayerRequest_t, commandSize), BINARY_USHORT_BYTES);
            has_request_bytes = (byte_count <= remaining);
            is_valid          = has_request_bytes && (0 == (byte_count % BINARY_WORD_BYTES));
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
 *  Adds one widget array entry and its properties in Creation Structure order,
 *  for example a label under the panel whose WidgetIdent is 4641 (Table 4.5.3.3.1):
 *
 * \verbatim
 * CA01      A661_CMD_CREATE
 * 002C      CommandSize (44 bytes, including this header)
 * A160      WidgetType          -> widget type A661_LABEL
 * 0002      WidgetIdent         -> property WidgetIdent = "2"
 * 1221      ParentIdent         -> parentWidgetIndex of the earlier widget with WidgetIdent 4641
 * 00        Anonymous           -> property Anonymous = "0"
 * 01        Visible             -> property Visible = "1"
 * FFFFFFFF  PosX                -> property PosX = "-1"
 * ...       remaining parameters, each read by ReadField
 * 594F5500  LabelString         -> property LabelString = "YOU"
 * \endverbatim
 *
 *  or an extension of the widget before it (Table 4.5.3.3.2):
 *
 * \verbatim
 * CA05      A661_CMD_EXTEND
 * 0008      CommandSize
 * 8004      ExtensionType       -> extension entry A661_LEGEND_ALIGN_EXTENSION
 * 15        LegendAlignment     -> property LegendAlignment = "21"
 * 00        UnusedPad           -> skipped
 * \endverbatim
 *
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
    bool_t                has_field_bytes;
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
        type     = FindTypeByCode(GetUnsigned(buffer, commandOffset + offsetof(BinaryCommand_t, type), BINARY_USHORT_BYTES), is_extension);
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
        cursor                    = commandOffset + sizeof(BinaryCommand_t);
        if (is_extension)
        {
            // Require a widget to extend in this layer block
            widget->parentWidgetIndex = (*lastWidgetIndex);
            is_valid                  = (UINT32_MAX != (*lastWidgetIndex));
        }
        else
        {
            cursor       = commandOffset + sizeof(BinaryCreateCommand_t);
            parent_ident = GetUnsigned(buffer, commandOffset + offsetof(BinaryCreateCommand_t, parentIdent), BINARY_USHORT_BYTES);
            if (0 != parent_ident)
            {
                cursor = firstCommandOffset;
                for (widget_index = firstWidgetIndex; widget_index < definition->widgetCount; ++widget_index)
                {
                    if ((BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, BINARY_USHORT_BYTES)) && (parent_ident == GetUnsigned(buffer, cursor + offsetof(BinaryCreateCommand_t, widgetIdent), BINARY_USHORT_BYTES)))
                    {
                        widget->parentWidgetIndex = widget_index;
                        break;
                    }
                    cursor += GetUnsigned(buffer, cursor + offsetof(BinaryCommand_t, commandSize), BINARY_USHORT_BYTES);
                }
                cursor   = commandOffset + sizeof(BinaryCreateCommand_t);
                is_valid = (UINT32_MAX != widget->parentWidgetIndex);
            }
            if (is_valid)
            {
                is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_WIDGET, definition->widgetCount, BINARY_PROPERTY_WIDGET_IDENT, GetUnsigned(buffer, commandOffset + offsetof(BinaryCreateCommand_t, widgetIdent), BINARY_USHORT_BYTES));
            }
        }
    }
    if (is_valid)
    {
        for (field_index = 0; field_index < type->fieldCount; ++field_index)
        {
            field           = &type->fields[field_index];
            element_count   = 1;
            has_field_bytes = (field->byteCount <= (end_offset - cursor));
            if (NULL != field->countName)
            {
                element_count = field->elementsPerEntry * GetEntryCount(type, field, values);
            }
            else if (((BINARY_FIELD_UCHAR == field->type) || (BINARY_FIELD_USHORT == field->type)) && has_field_bytes)
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
static bool_t ReadLayer(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint64_t* offset)
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
    bool_t            has_minimum_bytes;
    bool_t            is_block_size_valid;
    bool_t            has_block_bytes;
    bool_t            has_command_header;
    bool_t            is_command_size_valid;
    bool_t            has_command_bytes;
    bool_t            is_valid;

    layer_index        = definition->layerCount;
    first_widget_index = definition->widgetCount;
    last_widget_index  = UINT32_MAX;
    end_offset         = 0;
    has_minimum_bytes  = ((sizeof(BinaryLayerBlock_t) + sizeof(BinaryBlockEnd_t)) <= (sizeOfBinaryBuffer - (*offset)));
    is_valid           = has_minimum_bytes;
    if (is_valid)
    {
        block_size          = GetUnsigned(buffer, (*offset) + offsetof(BinaryLayerBlock_t, blockSize), BINARY_ULONG_BYTES);
        is_block_size_valid = ((sizeof(BinaryLayerBlock_t) + sizeof(BinaryBlockEnd_t)) <= block_size) && (0 == (block_size % BINARY_WORD_BYTES));
        has_block_bytes     = (block_size <= (sizeOfBinaryBuffer - (*offset)));
        is_valid            = is_block_size_valid && has_block_bytes;
    }
    if (is_valid)
    {
        end_offset = (*offset) + block_size - sizeof(BinaryBlockEnd_t);
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
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_LAYER_ID, buffer[(*offset) + offsetof(BinaryLayerBlock_t, layerIdent)]);
    }
    if (is_valid)
    {
        is_valid = AddIntegerProperty(definition, ARINC661_PROPERTY_LAYER, layer_index, BINARY_PROPERTY_CONTEXT_NUMBER, GetUnsigned(buffer, (*offset) + offsetof(BinaryLayerBlock_t, contextNumber), BINARY_USHORT_BYTES));
    }
    if (is_valid)
    {
        definition->layerCount += 1;
        first_command_offset = (*offset) + sizeof(BinaryLayerBlock_t);
        cursor               = first_command_offset;
        // Bound the walk by the two-word minimum command size
        for (command_index = 0; command_index < sizeOfBinaryBuffer; ++command_index)
        {
            if (cursor >= end_offset)
            {
                break;
            }
            has_command_header = (AlignToWord(sizeof(BinaryCommand_t)) <= (end_offset - cursor));
            is_valid           = has_command_header;
            if (is_valid)
            {
                command_size          = GetUnsigned(buffer, cursor + offsetof(BinaryCommand_t, commandSize), BINARY_USHORT_BYTES);
                is_command_size_valid = (AlignToWord(sizeof(BinaryCommand_t)) <= command_size) && (0 == (command_size % BINARY_WORD_BYTES));
                has_command_bytes     = (command_size <= (end_offset - cursor));
                is_valid              = is_command_size_valid && has_command_bytes;
            }
            if (is_valid && (BINARY_CMD_CREATE == GetUnsigned(buffer, cursor, BINARY_USHORT_BYTES)))
            {
                is_valid = (AlignToWord(sizeof(BinaryCreateCommand_t)) <= command_size);
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
        (*offset) = end_offset + sizeof(BinaryBlockEnd_t);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads one picture laid out as in Table 7-2, for example two by three RGBA pixels:
 *
 * \verbatim
 * BC        A661_BEGIN_PICTURE
 * 00        UnusedPad
 * 0001      Picture reference
 * 0002      Width in pixels
 * 0003      Height in pixels
 * ...       Width x height pixels in the block's pixel format, 24 bytes here, padded to 32 bits
 * BD        A661_END_PICTURE
 * 000000    UnusedPad
 * \endverbatim
 */
static bool_t MeasurePicture(const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint32_t pixelFormat, uint64_t* cursor)
{
    uint64_t pixel_bytes;
    uint32_t width;
    uint32_t height;
    bool_t   has_picture_bytes;
    bool_t   has_end_code_bytes;
    bool_t   is_valid;

    has_picture_bytes = ((*cursor) <= sizeOfBinaryBuffer) && (sizeof(BinaryPicture_t) <= (sizeOfBinaryBuffer - (*cursor)));
    is_valid          = has_picture_bytes && (BINARY_BEGIN_PICTURE == buffer[(*cursor)]);
    if (is_valid)
    {
        width              = GetUnsigned(buffer, (*cursor) + offsetof(BinaryPicture_t, width), BINARY_USHORT_BYTES);
        height             = GetUnsigned(buffer, (*cursor) + offsetof(BinaryPicture_t, height), BINARY_USHORT_BYTES);
        pixel_bytes        = (uint64_t)width * height * c_pixelFormatBytes[pixelFormat - BINARY_PIX_FMT_RGBA_8];
        (*cursor)         += sizeof(BinaryPicture_t) + AlignToWord(pixel_bytes);
        has_end_code_bytes = ((*cursor) <= sizeOfBinaryBuffer) && (sizeof(BinaryBlockEnd_t) <= (sizeOfBinaryBuffer - (*cursor)));
        is_valid           = has_end_code_bytes && (BINARY_END_PICTURE == buffer[(*cursor)]);
        (*cursor)         += sizeof(BinaryBlockEnd_t);
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
static bool_t MeasurePictureBlock(const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint64_t offset, uint64_t* blockSize)
{
    uint64_t cursor;
    uint32_t pixel_format;
    uint32_t color_format;
    uint32_t picture_count;
    uint32_t color_count;
    uint32_t picture_index;
    bool_t   has_end_code_bytes;
    bool_t   has_header_bytes;
    bool_t   is_valid;

    cursor           = offset + sizeof(BinaryPictureBlock_t);
    pixel_format     = 0;
    picture_count    = 0;
    has_header_bytes = (sizeof(BinaryPictureBlock_t) <= (sizeOfBinaryBuffer - offset));
    is_valid         = has_header_bytes;
    if (is_valid)
    {
        pixel_format  = buffer[offset + offsetof(BinaryPictureBlock_t, pixelFormat)];
        picture_count = buffer[offset + offsetof(BinaryPictureBlock_t, pictureCount)];
        color_count   = buffer[offset + offsetof(BinaryPictureBlock_t, colorCount)];
        color_format  = buffer[offset + offsetof(BinaryPictureBlock_t, colorFormat)];
        is_valid      = (BINARY_PIX_FMT_RGBA_8 <= pixel_format) && (BINARY_PIX_FMT_COLOR_INDEXED_8 >= pixel_format);
        if (is_valid && (0 != color_count))
        {
            is_valid = (BINARY_PIX_FMT_RGBA_8 <= color_format) && (BINARY_PIX_FMT_COLOR_INDEXED_8 >= color_format);
            if (is_valid)
            {
                // Measure color table entries in their own format and pad them as a whole
                cursor += AlignToWord((uint64_t)color_count * c_pixelFormatBytes[color_format - BINARY_PIX_FMT_RGBA_8]);
            }
        }
    }
    if (is_valid)
    {
        for (picture_index = 0; picture_index < picture_count; ++picture_index)
        {
            is_valid = MeasurePicture(buffer, sizeOfBinaryBuffer, pixel_format, &cursor);
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }
    if (is_valid)
    {
        has_end_code_bytes = (cursor <= sizeOfBinaryBuffer) && (sizeof(BinaryBlockEnd_t) <= (sizeOfBinaryBuffer - cursor));
        is_valid           = has_end_code_bytes;
    }
    if (is_valid && (BINARY_END_PICTURE_BLOCK != buffer[cursor]))
    {
        // Skip the Table 7-1 reserved word before the end code
        cursor            += BINARY_WORD_BYTES;
        has_end_code_bytes = (cursor <= sizeOfBinaryBuffer) && (sizeof(BinaryBlockEnd_t) <= (sizeOfBinaryBuffer - cursor));
        is_valid           = has_end_code_bytes && (BINARY_END_PICTURE_BLOCK == buffer[cursor]);
    }
    if (is_valid)
    {
        (*blockSize) = (cursor + sizeof(BinaryBlockEnd_t)) - offset;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Symbol and animation law blocks (Tables 4.5.3.2-5 and 9.2-1) carry a Block
 *  Size after a padded code and end with their end code, for example:
 *
 * \verbatim
 * F0        A661_BEGIN_SYMBOL_BLOCK (CA for A661_BEGIN_ANIMATION_LAW_BLOCK)
 * 000000    UnusedPad
 * 00000014  Block Size, including this header and the end code
 * ...       symbol definitions, kept byte for byte
 * F8        A661_END_SYMBOL_BLOCK (CB for A661_END_ANIMATION_LAW_BLOCK)
 * 000000    UnusedPad
 * \endverbatim
 *
 *  Picture blocks have no Block Size and are measured by MeasurePictureBlock
 *  instead. The block is appended unchanged to the caller's block
 *  storage, so Arinc661_Definition_WriteBinary can emit it again. The storage
 *  pointer may be NULL only while its capacity is zero.
 */
static bool_t ReadOpaqueBlock(Arinc661_Definition_t* definition, const byte_t* buffer, uint64_t sizeOfBinaryBuffer, uint64_t* offset)
{
    uint64_t block_size;
    uint32_t end_code;
    bool_t   has_header_bytes;
    bool_t   is_block_size_valid;
    bool_t   has_block_bytes;
    bool_t   is_valid;

    block_size = 0;
    if (BINARY_BEGIN_PICTURE_BLOCK == buffer[(*offset)])
    {
        is_valid = MeasurePictureBlock(buffer, sizeOfBinaryBuffer, (*offset), &block_size);
    }
    else
    {
        end_code = BINARY_END_ANIMATION_BLOCK;
        if (BINARY_BEGIN_SYMBOL_BLOCK == buffer[(*offset)])
        {
            end_code = BINARY_END_SYMBOL_BLOCK;
        }
        has_header_bytes = (sizeof(BinarySizedBlock_t) <= (sizeOfBinaryBuffer - (*offset)));
        is_valid         = has_header_bytes;
        if (is_valid)
        {
            block_size          = GetUnsigned(buffer, (*offset) + offsetof(BinarySizedBlock_t, blockSize), BINARY_ULONG_BYTES);
            is_block_size_valid = ((sizeof(BinarySizedBlock_t) + sizeof(BinaryBlockEnd_t)) <= block_size);
            has_block_bytes     = (block_size <= (sizeOfBinaryBuffer - (*offset)));
            is_valid            = is_block_size_valid && has_block_bytes;
        }
        if (is_valid)
        {
            is_valid = (end_code == buffer[(*offset) + block_size - sizeof(BinaryBlockEnd_t)]);
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
 *  Counts the supported widget types or the supported extension types.
 */
uint32_t Arinc661_Definition_GetTypeCount(bool_t isExtension)
{
    uint32_t type_count;

    if (isExtension)
    {
        type_count = (uint32_t)COUNT_OF(c_binaryExtensions);
    }
    else
    {
        type_count = (uint32_t)COUNT_OF(c_binaryWidgets);
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
    if (isExtension)
    {
        name = c_binaryExtensions[typeIndex].name;
    }
    else
    {
        name = c_binaryWidgets[typeIndex].name;
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
                parameter->elementsPerEntry = field->elementsPerEntry;
                parameter->countName        = field->countName;
                parameter->values           = field->values;
                parameter->valueCount       = field->valueCount;
                parameter->kind = FieldTypeToParameterKind(field->type);
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
        if (0 == strncmp(extensionType, c_extensionTargets[extension_index].extension, ARINC661_MAX_NAME_BYTES))
        {
            for (widget_index = 0; widget_index < c_extensionTargets[extension_index].widgetCount; ++widget_index)
            {
                if (0 == strncmp(widgetType, c_extensionTargets[extension_index].widgets[widget_index], ARINC661_MAX_NAME_BYTES))
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
        if ((0 == strncmp(extensionType, c_extensionTargets[extension_index].extension, ARINC661_MAX_NAME_BYTES)) && (index < c_extensionTargets[extension_index].widgetCount))
        {
            widget = c_extensionTargets[extension_index].widgets[index];
            break;
        }
    }

    return widget;
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
    FACEOE_Utl_Xml_Node_t* root;
    FACEOE_Utl_Xml_Node_t* layer_node;
    FACEOE_Utl_Xml_Node_t* current_node;
    FACEOE_Utl_Xml_Node_t* model_node;
    Arinc661_Layer_t*      layer;
    Arinc661_Widget_t*     widget;
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
                        if (0 == strncmp(charset_name, c_charsetEncodings[charset_index].name, sizeof(charset_name)))
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
    XmlWorkspace_t           xml;
    FACEOE_Utl_Xml_Node_t*   root;
    FACEOE_Utl_Xml_Node_t*   layer_node;
    FACEOE_Utl_Xml_Node_t*   widget_node;
    FACEOE_Utl_Xml_Node_t*   parent_node;
    FACEOE_Utl_Xml_Node_t*   charset_node;
    const Arinc661_Layer_t*  layer;
    const Arinc661_Widget_t* widget;
    const char_t*            element_name;
    uint64_t                 byte_count;
    uint32_t                 layer_index;
    uint32_t                 widget_index;
    uint32_t                 charset_index;
    bool_t                   has_workspace;
    bool_t                   has_parent;
    bool_t                   is_valid;

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
    bool_t   has_block_word;
    bool_t   has_charset_bytes;
    bool_t   is_valid;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != bufferToReadBinaryFrom);
    LOGASSERT((NULL != definition->blocks) || (0 == definition->blockCapacity));
    // Reset result counts while retaining the supplied arrays and capacities
    ResetDefinition(definition);
    has_footer = FALSE;
    offset     = sizeof(BinaryHeader_t);
    is_valid   = (sizeof(BinaryHeader_t) <= sizeOfBinaryBuffer);
    if (is_valid)
    {
        is_valid = (BINARY_DF_MAGIC_NUMBER == GetUnsigned(bufferToReadBinaryFrom, offsetof(BinaryHeader_t, magicNumber), BINARY_USHORT_BYTES));
    }
    if (is_valid)
    {
        definition->name[0]           = '\0';
        definition->libraryVersion    = bufferToReadBinaryFrom[offsetof(BinaryHeader_t, libraryVersion)];
        definition->supplementVersion = bufferToReadBinaryFrom[offsetof(BinaryHeader_t, supplementVersion)];
        is_valid                      = AddIntegerProperty(definition, ARINC661_PROPERTY_DEFINITION, 0, BINARY_PROPERTY_APPLICATION_ID, GetUnsigned(bufferToReadBinaryFrom, offsetof(BinaryHeader_t, appliIdent), BINARY_USHORT_BYTES));
        // Skip the implementation-dependent OEM data, which occupies whole words
        offset += AlignToWord(GetUnsigned(bufferToReadBinaryFrom, offsetof(BinaryHeader_t, oemDataSize), BINARY_USHORT_BYTES));
    }
    if (is_valid)
    {
        // Bound the walk by the one-word minimum block size
        for (block_index = 0; block_index < sizeOfBinaryBuffer; ++block_index)
        {
            has_block_word = (offset <= sizeOfBinaryBuffer) && (sizeof(BinaryBlockEnd_t) <= (sizeOfBinaryBuffer - offset));
            is_valid       = has_block_word;
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
                has_charset_bytes = (sizeof(BinaryCharsetBlock_t) <= (sizeOfBinaryBuffer - offset));
                is_valid          = has_charset_bytes && (BINARY_END_CHARSET_BLOCK == bufferToReadBinaryFrom[offset + offsetof(BinaryCharsetBlock_t, endCode)]) && (0 == definition->charsetEncoding);
                if (is_valid)
                {
                    definition->charsetEncoding = GetUnsigned(bufferToReadBinaryFrom, offset + offsetof(BinaryCharsetBlock_t, encoding), BINARY_USHORT_BYTES);
                    is_valid                    = (0 != definition->charsetEncoding);
                    offset                     += sizeof(BinaryCharsetBlock_t);
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
    offset         = sizeof(BinaryHeader_t);
    is_valid       = (UINT8_MAX >= definition->libraryVersion) && (UINT8_MAX >= definition->supplementVersion) && (UINT16_MAX >= definition->charsetEncoding);
    if (is_valid)
    {
        property = FindProperty(definition, ARINC661_PROPERTY_DEFINITION, 0, BINARY_PROPERTY_APPLICATION_ID, 0);
        is_valid = ScanInteger(property, 0, UINT16_MAX, &application_id);
    }
    if (is_valid)
    {
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offsetof(BinaryHeader_t, magicNumber), BINARY_DF_MAGIC_NUMBER, BINARY_USHORT_BYTES);
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offsetof(BinaryHeader_t, libraryVersion), definition->libraryVersion, BINARY_UCHAR_BYTES);
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offsetof(BinaryHeader_t, supplementVersion), definition->supplementVersion, BINARY_UCHAR_BYTES);
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offsetof(BinaryHeader_t, appliIdent), (uint32_t)application_id, BINARY_USHORT_BYTES);
        // Write no OEM free data
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offsetof(BinaryHeader_t, oemDataSize), 0, BINARY_USHORT_BYTES);
        if (0 != definition->charsetEncoding)
        {
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset, (BINARY_BEGIN_CHARSET_BLOCK << BINARY_BLOCK_CODE_SHIFT) | definition->charsetEncoding, BINARY_ULONG_BYTES);
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset + offsetof(BinaryCharsetBlock_t, endCode), BINARY_END_CHARSET_BLOCK << BINARY_BLOCK_CODE_SHIFT, BINARY_ULONG_BYTES);
            offset += sizeof(BinaryCharsetBlock_t);
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
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset + offsetof(BinaryLayerBlock_t, layerIdent), (uint32_t)layer_id, BINARY_UCHAR_BYTES);
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset + offsetof(BinaryLayerBlock_t, contextNumber), (uint32_t)context_number, BINARY_USHORT_BYTES);
            offset += sizeof(BinaryLayerBlock_t);
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
                PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset, (uint32_t)BINARY_END_LAYER_BLOCK << BINARY_BLOCK_CODE_SHIFT, sizeof(BinaryBlockEnd_t));
                offset   += sizeof(BinaryBlockEnd_t);
                // Count the block header and the end code in Block Size
                is_valid  = ((offset - layer_offset) <= UINT32_MAX);
            }
            if (FALSE == is_valid)
            {
                break;
            }
            PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, layer_offset + offsetof(BinaryLayerBlock_t, blockSize), (uint32_t)(offset - layer_offset), BINARY_ULONG_BYTES);
        }
    }
    if (is_valid)
    {
        PutUnsigned(bufferToWriteBinaryTo, capacityOfBinaryBuffer, offset, (uint32_t)BINARY_DF_FOOTER << BINARY_BLOCK_CODE_SHIFT, sizeof(BinaryBlockEnd_t));
        offset   += sizeof(BinaryBlockEnd_t);
        is_valid  = (offset <= capacityOfBinaryBuffer);
    }
    if (is_valid)
    {
        byte_count = offset;
    }

    return byte_count;
}
