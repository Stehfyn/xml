/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, sections 2.3.4, 3.1.3, 3.2.3, 3.2.6, and the widget definitions of
 * sections 3.3 to 3.10; see arinc661_scene.h.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_scene.h"

#include "faceoe_utl/faceoe_utl_scan.h"
#include "os/os_assert.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define SCENE_MAX_WIDGETS      (4096)
#define SCENE_MAX_PROPERTIES   (65536)
#define SCENE_MAX_POINTS       (256)
#define SCENE_TEXT_BYTES       (ARINC661_MAX_PROPERTY_BYTES + 64)
#define SCENE_DEFAULT_EXTENT   (10000.0f)
#define SCENE_DEGREES          (0.01745329252f)
#define SCENE_FULL_TURN        (6.28318530718f)
#define SCENE_HIDDEN_ALPHA     (0.3f)
#define SCENE_DISABLED_ALPHA   (0.55f)
#define SCENE_DASH_LENGTH      (150.0f)
#define SCENE_MARKER_RADIUS    (120.0f)
#define SCENE_THIN             (1.0f)
#define SCENE_THICK            (2.0f)
#define SCENE_CHIP_HEIGHT      (260.0f)
#define SCENE_CHIP_WIDTH       (3200.0f)
#define SCENE_CHIP_GAP         (60.0f)
#define SCENE_PAD              (40.0f)
#define SCENE_TEXT_FIT         (0.8f)
#define SCENE_MIN_FONT         (4.0f)
#define SCENE_NAME_FONT_SCALE  (0.85f)
#define SCENE_BOUNDARY_STEP    (5.0f)
#define SCENE_PALETTE_COUNT    (16)

#define COLOR_LAYER            FACEOE_GFX_RGBA(42, 50, 64, 255)
#define COLOR_LAYER_BORDER     FACEOE_GFX_RGBA(220, 230, 245, 255)
#define COLOR_CONTAINER        FACEOE_GFX_RGBA(42, 154, 188, 50)
#define COLOR_BORDER           FACEOE_GFX_RGBA(97, 221, 245, 255)
#define COLOR_BUTTON           FACEOE_GFX_RGBA(70, 110, 150, 200)
#define COLOR_BUTTON_BORDER    FACEOE_GFX_RGBA(170, 205, 240, 255)
#define COLOR_EDIT             FACEOE_GFX_RGBA(20, 24, 30, 230)
#define COLOR_EDIT_BORDER      FACEOE_GFX_RGBA(150, 170, 190, 255)
#define COLOR_TEXT             FACEOE_GFX_RGBA(240, 248, 255, 255)
#define COLOR_NAME             FACEOE_GFX_RGBA(255, 220, 120, 255)
#define COLOR_AREA             FACEOE_GFX_RGBA(255, 180, 60, 255)
#define COLOR_MARKER           FACEOE_GFX_RGBA(255, 120, 120, 255)
#define COLOR_SELECTION        FACEOE_GFX_RGBA(255, 255, 255, 60)
#define COLOR_ROW_SELECTED     FACEOE_GFX_RGBA(97, 160, 245, 120)
#define COLOR_CHIP             FACEOE_GFX_RGBA(90, 70, 120, 220)
#define COLOR_CHIP_BORDER      FACEOE_GFX_RGBA(190, 160, 240, 255)
#define COLOR_PICTURE          FACEOE_GFX_RGBA(120, 150, 90, 120)
#define COLOR_TRACK            FACEOE_GFX_RGBA(160, 170, 180, 255)
#define COLOR_SCROLLBAR        FACEOE_GFX_RGBA(120, 130, 140, 160)
#define COLOR_POPUP            FACEOE_GFX_RGBA(60, 60, 80, 160)
#define COLOR_GRID             FACEOE_GFX_RGBA(120, 200, 120, 160)
#define COLOR_RANGE            FACEOE_GFX_RGBA(120, 220, 160, 200)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/** Widget kinds, one per Creation Structure. */
typedef enum Kind_e
{
    KIND_UNKNOWN,
    KIND_ACTIVE_AREA, KIND_ANIMATION_GROUP, KIND_ANIMATION_ONPARAM, KIND_ANIMATION_ROTATION, KIND_ANIMATION_SCALE, KIND_ANIMATION_TRANSLATION,
    KIND_BASIC_CONTAINER, KIND_BLINKING_CONTAINER, KIND_BROADCAST_RECEIVER, KIND_BUFFER_FORMAT, KIND_CHECK_BUTTON, KIND_COMBO_BOX, KIND_COMBO_BOX_EDIT,
    KIND_CONNECTOR, KIND_CURSOR_OVER, KIND_CURSOR_POS_OVERLAY, KIND_CURSOR_REF, KIND_DATA_CONNECTOR, KIND_DATA_SCALING_FR180, KIND_DATA_SCALING_LONG, KIND_DATA_SCALING_ULONG,
    KIND_EDIT_BOX_MASKED, KIND_EDIT_BOX_MULTI_LINE, KIND_EDIT_BOX_NUMERIC, KIND_EDIT_BOX_NUMERIC_BCD, KIND_EDIT_BOX_TEXT, KIND_EVENT_HANDLER, KIND_EXTERNAL_SOURCE,
    KIND_FOCUS_IN, KIND_FOCUS_LINK, KIND_FOCUS_OUT, KIND_FRAMING_REFRESH_CONTAINER, KIND_GESTURE_AREA,
    KIND_GP_ARC_CIRCLE, KIND_GP_ARC_ELLIPSE, KIND_GP_CROWN, KIND_GP_LINE, KIND_GP_LINE_POLAR, KIND_GP_POLYLINE, KIND_GP_RECTANGLE, KIND_GP_TRIANGLE, KIND_GP_TRIANGLE_FAN, KIND_GP_TRIANGLE_STRIP,
    KIND_INK_AREA, KIND_KEYBOARD_AREA, KIND_LABEL, KIND_LABEL_COMPLEX,
    KIND_MAPHORZ, KIND_MAPHORZ_CONTAINER, KIND_MAPHORZ_ITEMLIST, KIND_MAPHORZ_PANEL, KIND_MAPHORZ_SOURCE, KIND_MAPHORZ_VERTEXBUFFER,
    KIND_MAPVERT, KIND_MAPVERT_CONTAINER, KIND_MAPVERT_ITEMLIST, KIND_MAPVERT_PANEL, KIND_MAPVERT_SOURCE,
    KIND_MAP_BOUNDARY, KIND_MAP_EXTERNAL_SOURCE, KIND_MAP_GRID, KIND_MASK_CONTAINER, KIND_MENU_BAR, KIND_MULTI_STATE_BUTTON, KIND_MUTUALLY_EXCLUSIVE_CONTAINER,
    KIND_NO_SERVICE_MONITOR, KIND_NUMERIC_READOUT, KIND_PAGING_CONTAINER, KIND_PANEL, KIND_PICTURE, KIND_PICTURE_ANIMATED, KIND_PICTURE_PUSH_BUTTON, KIND_PICTURE_TOGGLE_BUTTON,
    KIND_POP_UP_MENU, KIND_POP_UP_MENU_BUTTON, KIND_POP_UP_PANEL, KIND_POP_UP_PANEL_BUTTON, KIND_PROXY_BUTTON, KIND_PUSH_BUTTON, KIND_RADIO_BOX,
    KIND_ROTATION_CONTAINER, KIND_SCALE_CONTAINER, KIND_SCROLL_LIST, KIND_SCROLL_PANEL, KIND_SCROLL_WHEEL_AREA, KIND_SELECTION_LIST_BUTTON,
    KIND_SHUFFLE_TO_FIT_CONTAINER, KIND_SIZE_TO_FIT_CONTAINER, KIND_SLIDER, KIND_SYMBOL, KIND_SYMBOL_ANIMATED, KIND_SYMBOL_PUSH_BUTTON, KIND_SYMBOL_TOGGLE_BUTTON,
    KIND_TABBED_PANEL, KIND_TABBED_PANEL_GROUP, KIND_TOGGLE_BUTTON, KIND_TOUCH_AREA, KIND_TRANSLATION_CONTAINER, KIND_TREE, KIND_WATCHDOG_CONTAINER
} Kind_t;

/** Name to kind. */
typedef struct KindName_s
{
    const char_t* name;
    Kind_t        kind;
} KindName_t;

/** How a container presents its children. */
typedef enum ChildMode_e
{
    CHILD_ALL,      ///< Every child as it is
    CHILD_IDENT,    ///< Only the child whose WidgetIdent matches; the rest hidden
    CHILD_MENU_BAR, ///< Children laid along the bar
    CHILD_TABS      ///< Tabbed panels share the content area; only the active one is visible
} ChildMode_t;

/** Position and size handed to a child by its container's layout. */
typedef struct Layout_s
{
    bool_t    isSet;
    float32_t x;
    float32_t y;
    float32_t width;
    float32_t height;
} Layout_t;

/** Children policy produced by a container's handler. */
typedef struct Children_s
{
    ChildMode_t mode;
    int64_t     ident;
    bool_t      isHorizontal;
    float32_t   buttonPosition;
    float32_t   buttonSize;
    float32_t   offset;
    Layout_t    content;
    uint32_t    tabIndex;
} Children_t;

/** Output-space bounds accumulator. */
typedef struct Bounds_s
{
    bool_t             has;
    FACEOE_Gfx_Point_t minimum;
    FACEOE_Gfx_Point_t maximum;
} Bounds_t;

/** Drawing state for one layer. */
typedef struct Scene_s
{
    const Arinc661_Definition_t* definition;
    FACEOE_Gfx_DrawList_t*       drawList;
    Arinc661_Scene_Options_t*    options;
    uint32_t                     layerIndex;
    float32_t                    layerWidth;
    float32_t                    layerHeight;
    uint32_t                     chipCount;
    FACEOE_Gfx_Transform_t       base;
    uint32_t*                    first;
    uint32_t*                    count;
    uint32_t*                    order;
} Scene_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

static const KindName_t c_kindNames[] =
{
    {"A661_ACTIVE_AREA", KIND_ACTIVE_AREA}, {"A661_ANIMATION_GROUP", KIND_ANIMATION_GROUP}, {"A661_ANIMATION_ONPARAM", KIND_ANIMATION_ONPARAM},
    {"A661_ANIMATION_ROTATION", KIND_ANIMATION_ROTATION}, {"A661_ANIMATION_SCALE", KIND_ANIMATION_SCALE}, {"A661_ANIMATION_TRANSLATION", KIND_ANIMATION_TRANSLATION},
    {"A661_BASIC_CONTAINER", KIND_BASIC_CONTAINER}, {"A661_BLINKING_CONTAINER", KIND_BLINKING_CONTAINER}, {"A661_BROADCAST_RECEIVER", KIND_BROADCAST_RECEIVER},
    {"A661_BUFFER_FORMAT", KIND_BUFFER_FORMAT}, {"A661_CHECK_BUTTON", KIND_CHECK_BUTTON}, {"A661_COMBO_BOX", KIND_COMBO_BOX}, {"A661_COMBO_BOX_EDIT", KIND_COMBO_BOX_EDIT},
    {"A661_CONNECTOR", KIND_CONNECTOR}, {"A661_CURSOR_OVER", KIND_CURSOR_OVER}, {"A661_CURSOR_POS_OVERLAY", KIND_CURSOR_POS_OVERLAY}, {"A661_CURSOR_REF", KIND_CURSOR_REF},
    {"A661_DATA_CONNECTOR", KIND_DATA_CONNECTOR}, {"A661_DATA_SCALING_FR180", KIND_DATA_SCALING_FR180}, {"A661_DATA_SCALING_LONG", KIND_DATA_SCALING_LONG},
    {"A661_DATA_SCALING_ULONG", KIND_DATA_SCALING_ULONG}, {"A661_EDIT_BOX_MASKED", KIND_EDIT_BOX_MASKED}, {"A661_EDIT_BOX_MULTI_LINE", KIND_EDIT_BOX_MULTI_LINE},
    {"A661_EDIT_BOX_NUMERIC", KIND_EDIT_BOX_NUMERIC}, {"A661_EDIT_BOX_NUMERIC_BCD", KIND_EDIT_BOX_NUMERIC_BCD}, {"A661_EDIT_BOX_TEXT", KIND_EDIT_BOX_TEXT},
    {"A661_EVENT_HANDLER", KIND_EVENT_HANDLER}, {"A661_EXTERNAL_SOURCE", KIND_EXTERNAL_SOURCE}, {"A661_FOCUS_IN", KIND_FOCUS_IN}, {"A661_FOCUS_LINK", KIND_FOCUS_LINK},
    {"A661_FOCUS_OUT", KIND_FOCUS_OUT}, {"A661_FRAMING_REFRESH_CONTAINER", KIND_FRAMING_REFRESH_CONTAINER}, {"A661_GESTURE_AREA", KIND_GESTURE_AREA},
    {"A661_GP_ARC_CIRCLE", KIND_GP_ARC_CIRCLE}, {"A661_GP_ARC_ELLIPSE", KIND_GP_ARC_ELLIPSE}, {"A661_GP_CROWN", KIND_GP_CROWN}, {"A661_GP_LINE", KIND_GP_LINE},
    {"A661_GP_LINE_POLAR", KIND_GP_LINE_POLAR}, {"A661_GP_POLYLINE", KIND_GP_POLYLINE}, {"A661_GP_RECTANGLE", KIND_GP_RECTANGLE}, {"A661_GP_TRIANGLE", KIND_GP_TRIANGLE},
    {"A661_GP_TRIANGLE_FAN", KIND_GP_TRIANGLE_FAN}, {"A661_GP_TRIANGLE_STRIP", KIND_GP_TRIANGLE_STRIP}, {"A661_INK_AREA", KIND_INK_AREA}, {"A661_KEYBOARD_AREA", KIND_KEYBOARD_AREA},
    {"A661_LABEL", KIND_LABEL}, {"A661_LABEL_COMPLEX", KIND_LABEL_COMPLEX}, {"A661_MAPHORZ", KIND_MAPHORZ}, {"A661_MAPHORZ_CONTAINER", KIND_MAPHORZ_CONTAINER},
    {"A661_MAPHORZ_ITEMLIST", KIND_MAPHORZ_ITEMLIST}, {"A661_MAPHORZ_PANEL", KIND_MAPHORZ_PANEL}, {"A661_MAPHORZ_SOURCE", KIND_MAPHORZ_SOURCE},
    {"A661_MAPHORZ_VERTEXBUFFER", KIND_MAPHORZ_VERTEXBUFFER}, {"A661_MAPVERT", KIND_MAPVERT}, {"A661_MAPVERT_CONTAINER", KIND_MAPVERT_CONTAINER},
    {"A661_MAPVERT_ITEMLIST", KIND_MAPVERT_ITEMLIST}, {"A661_MAPVERT_PANEL", KIND_MAPVERT_PANEL}, {"A661_MAPVERT_SOURCE", KIND_MAPVERT_SOURCE},
    {"A661_MAP_BOUNDARY", KIND_MAP_BOUNDARY}, {"A661_MAP_EXTERNAL_SOURCE", KIND_MAP_EXTERNAL_SOURCE}, {"A661_MAP_GRID", KIND_MAP_GRID}, {"A661_MASK_CONTAINER", KIND_MASK_CONTAINER},
    {"A661_MENU_BAR", KIND_MENU_BAR}, {"A661_MULTI_STATE_BUTTON", KIND_MULTI_STATE_BUTTON}, {"A661_MUTUALLY_EXCLUSIVE_CONTAINER", KIND_MUTUALLY_EXCLUSIVE_CONTAINER},
    {"A661_NO_SERVICE_MONITOR", KIND_NO_SERVICE_MONITOR}, {"A661_NUMERIC_READOUT", KIND_NUMERIC_READOUT}, {"A661_PAGING_CONTAINER", KIND_PAGING_CONTAINER}, {"A661_PANEL", KIND_PANEL},
    {"A661_PICTURE", KIND_PICTURE}, {"A661_PICTURE_ANIMATED", KIND_PICTURE_ANIMATED}, {"A661_PICTURE_PUSH_BUTTON", KIND_PICTURE_PUSH_BUTTON},
    {"A661_PICTURE_TOGGLE_BUTTON", KIND_PICTURE_TOGGLE_BUTTON}, {"A661_POP_UP_MENU", KIND_POP_UP_MENU}, {"A661_POP_UP_MENU_BUTTON", KIND_POP_UP_MENU_BUTTON},
    {"A661_POP_UP_PANEL", KIND_POP_UP_PANEL}, {"A661_POP_UP_PANEL_BUTTON", KIND_POP_UP_PANEL_BUTTON}, {"A661_PROXY_BUTTON", KIND_PROXY_BUTTON}, {"A661_PUSH_BUTTON", KIND_PUSH_BUTTON},
    {"A661_RADIO_BOX", KIND_RADIO_BOX}, {"A661_ROTATION_CONTAINER", KIND_ROTATION_CONTAINER}, {"A661_SCALE_CONTAINER", KIND_SCALE_CONTAINER}, {"A661_SCROLL_LIST", KIND_SCROLL_LIST},
    {"A661_SCROLL_PANEL", KIND_SCROLL_PANEL}, {"A661_SCROLL_WHEEL_AREA", KIND_SCROLL_WHEEL_AREA}, {"A661_SELECTION_LIST_BUTTON", KIND_SELECTION_LIST_BUTTON},
    {"A661_SHUFFLE_TO_FIT_CONTAINER", KIND_SHUFFLE_TO_FIT_CONTAINER}, {"A661_SIZE_TO_FIT_CONTAINER", KIND_SIZE_TO_FIT_CONTAINER}, {"A661_SLIDER", KIND_SLIDER},
    {"A661_SYMBOL", KIND_SYMBOL}, {"A661_SYMBOL_ANIMATED", KIND_SYMBOL_ANIMATED}, {"A661_SYMBOL_PUSH_BUTTON", KIND_SYMBOL_PUSH_BUTTON}, {"A661_SYMBOL_TOGGLE_BUTTON", KIND_SYMBOL_TOGGLE_BUTTON},
    {"A661_TABBED_PANEL", KIND_TABBED_PANEL}, {"A661_TABBED_PANEL_GROUP", KIND_TABBED_PANEL_GROUP}, {"A661_TOGGLE_BUTTON", KIND_TOGGLE_BUTTON}, {"A661_TOUCH_AREA", KIND_TOUCH_AREA},
    {"A661_TRANSLATION_CONTAINER", KIND_TRANSLATION_CONTAINER}, {"A661_TREE", KIND_TREE}, {"A661_WATCHDOG_CONTAINER", KIND_WATCHDOG_CONTAINER}
};

/** Colours for ColorIndex values; the CDS colour table is not part of the Definition File. */
static const FACEOE_Gfx_Color_t c_palette[SCENE_PALETTE_COUNT] =
{
    FACEOE_GFX_RGBA(255, 255, 255, 255), FACEOE_GFX_RGBA(0, 0, 0, 255),       FACEOE_GFX_RGBA(255, 60, 60, 255),   FACEOE_GFX_RGBA(60, 255, 60, 255),
    FACEOE_GFX_RGBA(80, 120, 255, 255),  FACEOE_GFX_RGBA(255, 255, 60, 255),  FACEOE_GFX_RGBA(60, 255, 255, 255),  FACEOE_GFX_RGBA(255, 60, 255, 255),
    FACEOE_GFX_RGBA(255, 160, 40, 255),  FACEOE_GFX_RGBA(160, 160, 160, 255), FACEOE_GFX_RGBA(120, 80, 40, 255),   FACEOE_GFX_RGBA(40, 120, 80, 255),
    FACEOE_GFX_RGBA(200, 200, 255, 255), FACEOE_GFX_RGBA(255, 200, 200, 255), FACEOE_GFX_RGBA(200, 255, 200, 255), FACEOE_GFX_RGBA(90, 90, 90, 255)
};

static uint32_t s_first[SCENE_MAX_WIDGETS + 1];
static uint32_t s_count[SCENE_MAX_WIDGETS + 1];
static uint32_t s_order[SCENE_MAX_PROPERTIES];

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Maps a widget type name to its kind.
 *
 * \param type   Widget or extension type name
 *
 * \retval Kind_t   Kind, or KIND_UNKNOWN.
 */
static Kind_t KindOf(const char_t* type);

/*************************************************************************************************/
/**
 * \brief Groups property indices by widget with a counting sort so lookups stay local.
 *
 * \param scene   Scene
 *
 * \retval None
 */
static void IndexProperties(Scene_t* scene);

/*************************************************************************************************/
/**
 * \brief Returns the n-th same-named property value of a widget, or NULL.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 * \param occurrence    Index among same-named properties
 *
 * \retval const char_t*   Value, or NULL.
 */
static const char_t* Value(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, uint32_t occurrence);

/*************************************************************************************************/
/**
 * \brief Counts the same-named properties of a widget.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 *
 * \retval uint32_t   Number of same-named properties.
 */
static uint32_t Occurrences(const Scene_t* scene, uint32_t widgetIndex, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Returns a widget's first same-named property value, or an empty string.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 *
 * \retval const char_t*   Value, or an empty string.
 */
static const char_t* Text(const Scene_t* scene, uint32_t widgetIndex, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a decimal or a constant name as a number.
 *
 * \param text     Text
 * \param number   Number
 *
 * \retval TRUE   if the text is a decimal or a known constant.
 * \retval FALSE  otherwise.
 */
static bool_t ScanNumber(const char_t* text, float32_t* number);

/*************************************************************************************************/
/**
 * \brief Reads a widget's first same-named property as a number.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 * \param number        Number
 *
 * \retval TRUE   if the property exists and is numeric.
 * \retval FALSE  otherwise.
 */
static bool_t Number(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, float32_t* number);

/*************************************************************************************************/
/**
 * \brief Reads a number with a fallback.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 * \param fallback      Fallback
 *
 * \retval float32_t   Value, or the fallback.
 */
static float32_t NumberOr(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, float32_t fallback);

/*************************************************************************************************/
/**
 * \brief Reads the n-th same-named property as a number with a fallback.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 * \param occurrence    Index among same-named properties
 * \param fallback      Fallback
 *
 * \retval float32_t   Value, or the fallback.
 */
static float32_t NumberAt(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, uint32_t occurrence, float32_t fallback);

/*************************************************************************************************/
/**
 * \brief Tests the first same-named property against a constant.
 *
 * \param scene          Scene
 * \param widgetIndex    Widget array index
 * \param name           Property or element name
 * \param constantName   Constant name
 *
 * \retval TRUE   if the property equals the constant.
 * \retval FALSE  otherwise.
 */
static bool_t Is(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, const char_t* constantName);

/*************************************************************************************************/
/**
 * \brief Tests whether a property equals a constant, by name or by value.
 *
 * \param scene          Scene
 * \param widgetIndex    Widget array index
 * \param name           Property or element name
 * \param occurrence     Index among same-named properties
 * \param constantName   Constant name
 *
 * \retval TRUE   if the property equals the constant.
 * \retval FALSE  otherwise.
 */
static bool_t IsAt(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, uint32_t occurrence, const char_t* constantName);

/*************************************************************************************************/
/**
 * \brief Reads a widget's WidgetIdent, or -1.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 *
 * \retval int64_t   WidgetIdent, or -1 when missing.
 */
static int64_t Ident(const Scene_t* scene, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Applies the inherited alpha to a colour.
 *
 * \param color   Colour
 * \param alpha   Alpha
 *
 * \retval FACEOE_Gfx_Color_t   Colour with its alpha scaled.
 */
static FACEOE_Gfx_Color_t Fade(FACEOE_Gfx_Color_t color, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Resolves a widget's ColorIndex through the palette.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param alpha         Alpha
 *
 * \retval FACEOE_Gfx_Color_t   Palette colour for the ColorIndex, faded.
 */
static FACEOE_Gfx_Color_t IndexColor(const Scene_t* scene, uint32_t widgetIndex, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Reads a widget's reference point and size, or takes them from the container's layout.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param layout        Layout
 * \param x             X in layer units
 * \param y             Y in layer units
 * \param width         Width
 * \param height        Height
 *
 * \retval None
 */
static void Rect(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t* x, float32_t* y, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Grows output bounds by a mapped local point.
 *
 * \param scene    Scene
 * \param bounds   Bounds
 * \param x        X in layer units
 * \param y        Y in layer units
 *
 * \retval None
 */
static void Extend(const Scene_t* scene, Bounds_t* bounds, float32_t x, float32_t y);

/*************************************************************************************************/
/**
 * \brief Grows output bounds by a local box.
 *
 * \param scene    Scene
 * \param bounds   Bounds
 * \param x        X in layer units
 * \param y        Y in layer units
 * \param width    Width
 * \param height   Height
 *
 * \retval None
 */
static void ExtendBox(const Scene_t* scene, Bounds_t* bounds, float32_t x, float32_t y, float32_t width, float32_t height);

/*************************************************************************************************/
/**
 * \brief Picks a font size that fits a local height, in output units.
 *
 * \param scene    Scene
 * \param height   Height
 *
 * \retval float32_t   Font size in output units.
 */
static float32_t FontFor(const Scene_t* scene, float32_t height);

/*************************************************************************************************/
/**
 * \brief Converts an Alignment constant to box alignments.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 * \param horizontal    Horizontal
 * \param vertical      Vertical
 *
 * \retval None
 */
static void AlignOf(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, FACEOE_Gfx_Align_t* horizontal, FACEOE_Gfx_Align_t* vertical);

/*************************************************************************************************/
/**
 * \brief Records text in a local box with a font that fits the box.
 *
 * \param scene        Scene
 * \param x            X in layer units
 * \param y            Y in layer units
 * \param width        Width
 * \param height       Height
 * \param text         Text
 * \param horizontal   Horizontal
 * \param vertical     Vertical
 * \param radians      Radians
 * \param color        Colour
 *
 * \retval None
 */
static void BoxText(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* text, FACEOE_Gfx_Align_t horizontal, FACEOE_Gfx_Align_t vertical, float32_t radians, FACEOE_Gfx_Color_t color);

/*************************************************************************************************/
/**
 * \brief Records a filled, outlined local box and grows the bounds.
 *
 * \param scene    Scene
 * \param x        X in layer units
 * \param y        Y in layer units
 * \param width    Width
 * \param height   Height
 * \param fill     Fill
 * \param border   Border
 * \param alpha    Alpha
 * \param bounds   Bounds
 *
 * \retval None
 */
static void Frame(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t fill, FACEOE_Gfx_Color_t border, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a dashed local box and grows the bounds.
 *
 * \param scene    Scene
 * \param x        X in layer units
 * \param y        Y in layer units
 * \param width    Width
 * \param height   Height
 * \param color    Colour
 * \param alpha    Alpha
 * \param bounds   Bounds
 *
 * \retval None
 */
static void Dashed(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a reference-point marker: a circle, a cross, and a heading tick.
 *
 * \param scene     Scene
 * \param x         X in layer units
 * \param y         Y in layer units
 * \param radians   Radians
 * \param color     Colour
 * \param alpha     Alpha
 * \param bounds    Bounds
 *
 * \retval None
 */
static void Marker(const Scene_t* scene, float32_t x, float32_t y, float32_t radians, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a picture or source placeholder: a box with diagonals and a caption.
 *
 * \param scene     Scene
 * \param x         X in layer units
 * \param y         Y in layer units
 * \param width     Width
 * \param height    Height
 * \param caption   Caption
 * \param alpha     Alpha
 * \param bounds    Bounds
 *
 * \retval None
 */
static void Placeholder(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* caption, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a scrollbar on the side named by a Top/Bottom/Left/Right property.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param name          Property or element name
 * \param x             X in layer units
 * \param y             Y in layer units
 * \param width         Width
 * \param height        Height
 * \param fraction      Fraction
 * \param start         Start
 * \param alpha         Alpha
 *
 * \retval None
 */
static void Scrollbar(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, float32_t x, float32_t y, float32_t width, float32_t height, float32_t fraction, float32_t start, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Records a chip for a widget without geometry along the layer's left edge.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void Chip(Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Counts the extensions attached to a widget.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 *
 * \retval uint32_t   Number of extensions attached to the widget.
 */
static uint32_t ExtensionCount(const Scene_t* scene, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Reports the widget's bounds and labels it with its name and extension count.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param bounds        Bounds
 * \param alpha         Alpha
 *
 * \retval None
 */
static void Record(Scene_t* scene, uint32_t widgetIndex, const Bounds_t* bounds, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Records a button face with an optional picture or symbol box beside its label.
 *
 * \param scene          Scene
 * \param widgetIndex    Widget array index
 * \param layout         Layout
 * \param label          Row label
 * \param iconName       Icon name
 * \param iconPosition   Icon position
 * \param alpha          Alpha
 * \param bounds         Bounds
 * \param x              X in layer units
 * \param y              Y in layer units
 * \param width          Width
 * \param height         Height
 *
 * \retval None
 */
static void Button(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, const char_t* label, const char_t* iconName, const char_t* iconPosition, float32_t alpha, Bounds_t* bounds, float32_t* x, float32_t* y, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Records the rows of a list from a string array, highlighting the selected entry.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param arrayName     Array name
 * \param enableName    Enable name
 * \param first         First
 * \param count         Count
 * \param selected      Selected
 * \param x             X in layer units
 * \param y             Y in layer units
 * \param width         Width
 * \param height        Height
 * \param align         Align
 * \param alpha         Alpha
 *
 * \retval None
 */
static void EntryRows(const Scene_t* scene, uint32_t widgetIndex, const char_t* arrayName, const char_t* enableName, uint32_t first, uint32_t count, uint32_t selected, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Align_t align, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Records a formatted numeric field with its legend: EditBoxNumeric, BCD, and NumericReadout.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param layout        Layout
 * \param radians       Radians
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void NumericBox(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t radians, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a graphics primitive: GpLine, GpLinePolar, GpArcCircle, GpArcEllipse, GpCrown, GpRectangle, GpTriangle, GpPolyline, GpTriangleFan, GpTriangleStrip.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param kind          Kind
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void DrawGp(const Scene_t* scene, uint32_t widgetIndex, Kind_t kind, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a slider: track, major and minor ticks, labels, and the thumb at Value.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param layout        Layout
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void DrawSlider(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a tree: entries indented by their ParentArray depth, collapsed under closed parents.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param layout        Layout
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void DrawTree(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a MapBoundary: sections of arc, vertical, or horizontal lines around the origin.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void DrawMapBoundary(const Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a MapGrid: CountX by CountY cells of IncrementX by IncrementY, offset so the cell at (OffsetX, OffsetY) sits on the display reference point.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param alpha         Alpha
 * \param bounds        Bounds
 *
 * \retval None
 */
static void DrawMapGrid(const Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a TabbedPanelGroup: a tab strip on TabPosition with one tab per TabbedPanel child, and the content area the active child fills.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param layout        Layout
 * \param alpha         Alpha
 * \param bounds        Bounds
 * \param children      Children
 *
 * \retval None
 */
static void DrawTabbedGroup(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t alpha, Bounds_t* bounds, Children_t* children);

/*************************************************************************************************/
/**
 * \brief Records one widget by its Creation Structure, then its children through the policy its kind imposes.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param alpha         Alpha
 * \param layout        Layout
 *
 * \retval None
 */
static void DrawWidget(Scene_t* scene, uint32_t widgetIndex, float32_t alpha, const Layout_t* layout);

/*************************************************************************************************/
/**
 * \brief Records a container's children in array order under its policy.
 *
 * \param scene         Scene
 * \param widgetIndex   Widget array index
 * \param alpha         Alpha
 * \param children      Children
 *
 * \retval None
 */
static void DrawChildren(Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Children_t* children);

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Maps a widget type name to its kind.
 */
static Kind_t KindOf(const char_t* type)
{
    uint32_t index;
    Kind_t   kind;

    kind = KIND_UNKNOWN;
    for (index = 0; index < (sizeof(c_kindNames) / sizeof(c_kindNames[0])); ++index)
    {
        if (0 == strcmp(type, c_kindNames[index].name))
        {
            kind = c_kindNames[index].kind;
            break;
        }
    }

    return kind;
}

/*************************************************************************************************/
/**
 * \brief Groups property indices by widget with a counting sort so lookups stay local.
 */
static void IndexProperties(Scene_t* scene)
{
    const Arinc661_Definition_t* definition;
    uint32_t                     widget_index;
    uint32_t                     property_index;
    uint32_t                     running;

    definition = scene->definition;
    for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
    {
        s_count[widget_index] = 0;
    }
    for (property_index = 0; property_index < definition->propertyCount; ++property_index)
    {
        if ((ARINC661_PROPERTY_WIDGET == definition->properties[property_index].owner) && (definition->properties[property_index].ownerIndex < definition->widgetCount))
        {
            s_count[definition->properties[property_index].ownerIndex] += 1;
        }
    }
    running = 0;
    for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
    {
        s_first[widget_index] = running;
        running += s_count[widget_index];
        s_count[widget_index] = 0;
    }
    for (property_index = 0; property_index < definition->propertyCount; ++property_index)
    {
        if ((ARINC661_PROPERTY_WIDGET == definition->properties[property_index].owner) && (definition->properties[property_index].ownerIndex < definition->widgetCount))
        {
            widget_index                                              = definition->properties[property_index].ownerIndex;
            s_order[s_first[widget_index] + s_count[widget_index]]    = property_index;
            s_count[widget_index] += 1;
        }
    }
    scene->first = s_first;
    scene->count = s_count;
    scene->order = s_order;
}

/*************************************************************************************************/
/**
 * \brief Returns the n-th same-named property value of a widget, or NULL.
 */
static const char_t* Value(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, uint32_t occurrence)
{
    const Arinc661_Property_t* property;
    const char_t*              value;
    uint32_t                   slot;
    uint32_t                   seen;

    value = NULL;
    seen  = 0;
    for (slot = 0; slot < scene->count[widgetIndex]; ++slot)
    {
        property = &scene->definition->properties[scene->order[scene->first[widgetIndex] + slot]];
        if (0 == strcmp(name, property->name))
        {
            if (seen == occurrence)
            {
                value = property->value;
                break;
            }
            seen += 1;
        }
    }

    return value;
}

/*************************************************************************************************/
/**
 * \brief Counts the same-named properties of a widget.
 */
static uint32_t Occurrences(const Scene_t* scene, uint32_t widgetIndex, const char_t* name)
{
    uint32_t slot;
    uint32_t seen;

    seen = 0;
    for (slot = 0; slot < scene->count[widgetIndex]; ++slot)
    {
        if (0 == strcmp(name, scene->definition->properties[scene->order[scene->first[widgetIndex] + slot]].name))
        {
            seen += 1;
        }
    }

    return seen;
}

/*************************************************************************************************/
/**
 * \brief Returns a widget's first same-named property value, or an empty string.
 */
static const char_t* Text(const Scene_t* scene, uint32_t widgetIndex, const char_t* name)
{
    const char_t* value;

    value = Value(scene, widgetIndex, name, 0);
    if (NULL == value)
    {
        value = "";
    }

    return value;
}

/*************************************************************************************************/
/**
 * \brief Reads a decimal or a constant name as a number.
 */
static bool_t ScanNumber(const char_t* text, float32_t* number)
{
    float64_t real_value;
    int32_t   consumed;
    uint32_t  constant_index;
    bool_t    is_number;

    is_number = FALSE;
    consumed  = FACEOE_Utl_Scan_Float(text, &real_value);
    if ((0 < consumed) && ('\0' == text[consumed]))
    {
        (*number) = (float32_t)real_value;
        is_number = TRUE;
    }
    else
    {
        for (constant_index = 0; constant_index < Arinc661_ConstantCount(); ++constant_index)
        {
            if (0 == strcmp(text, Arinc661_ConstantName(constant_index)))
            {
                (*number) = (float32_t)Arinc661_ConstantValue(constant_index);
                is_number = TRUE;
                break;
            }
        }
    }

    return is_number;
}

/*************************************************************************************************/
/**
 * \brief Reads a widget's first same-named property as a number.
 */
static bool_t Number(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, float32_t* number)
{
    const char_t* value;
    bool_t        is_number;

    is_number = FALSE;
    value     = Value(scene, widgetIndex, name, 0);
    if (NULL != value)
    {
        is_number = ScanNumber(value, number);
    }

    return is_number;
}

/*************************************************************************************************/
/**
 * \brief Reads a number with a fallback.
 */
static float32_t NumberOr(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, float32_t fallback)
{
    float32_t number;

    number = fallback;
    (void)Number(scene, widgetIndex, name, &number);

    return number;
}

/*************************************************************************************************/
/**
 * \brief Reads the n-th same-named property as a number with a fallback.
 */
static float32_t NumberAt(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, uint32_t occurrence, float32_t fallback)
{
    const char_t* value;
    float32_t     number;

    number = fallback;
    value  = Value(scene, widgetIndex, name, occurrence);
    if (NULL != value)
    {
        (void)ScanNumber(value, &number);
    }

    return number;
}

/*************************************************************************************************/
/**
 * \brief Tests whether a property equals a constant, by name or by value.
 */
static bool_t IsAt(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, uint32_t occurrence, const char_t* constantName)
{
    const char_t* value;
    float32_t     number;
    float32_t     constant;
    bool_t        is_equal;

    is_equal = FALSE;
    value    = Value(scene, widgetIndex, name, occurrence);
    if (NULL != value)
    {
        if (0 == strcmp(value, constantName))
        {
            is_equal = TRUE;
        }
        else if (ScanNumber(value, &number) && ScanNumber(constantName, &constant))
        {
            is_equal = (number == constant);
        }
    }

    return is_equal;
}

/*************************************************************************************************/
/**
 * \brief Tests the first same-named property against a constant.
 */
static bool_t Is(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, const char_t* constantName)
{
    return IsAt(scene, widgetIndex, name, 0, constantName);
}

/*************************************************************************************************/
/**
 * \brief Reads a widget's WidgetIdent, or -1.
 */
static int64_t Ident(const Scene_t* scene, uint32_t widgetIndex)
{
    float32_t number;
    int64_t   ident;

    ident = -1;
    if (Number(scene, widgetIndex, "WidgetIdent", &number))
    {
        ident = (int64_t)number;
    }

    return ident;
}

/*************************************************************************************************/
/**
 * \brief Applies the inherited alpha to a colour.
 */
static FACEOE_Gfx_Color_t Fade(FACEOE_Gfx_Color_t color, float32_t alpha)
{
    return FACEOE_Gfx_Color_Fade(color, alpha);
}

/*************************************************************************************************/
/**
 * \brief Resolves a widget's ColorIndex through the palette.
 */
static FACEOE_Gfx_Color_t IndexColor(const Scene_t* scene, uint32_t widgetIndex, float32_t alpha)
{
    float32_t index;

    index = NumberOr(scene, widgetIndex, "ColorIndex", 0.0f);
    if (index < 0.0f)
    {
        index = 0.0f;
    }

    return Fade(c_palette[((uint32_t)index) % SCENE_PALETTE_COUNT], alpha);
}

/*************************************************************************************************/
/**
 * \brief Reads a widget's reference point and size, or takes them from the container's layout.
 */
static void Rect(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t* x, float32_t* y, float32_t* width, float32_t* height)
{
    (*x)      = NumberOr(scene, widgetIndex, "PosX", 0.0f);
    (*y)      = NumberOr(scene, widgetIndex, "PosY", 0.0f);
    (*width)  = NumberOr(scene, widgetIndex, "SizeX", 0.0f);
    (*height) = NumberOr(scene, widgetIndex, "SizeY", 0.0f);
    if ((NULL != layout) && layout->isSet)
    {
        (*x)      = layout->x;
        (*y)      = layout->y;
        (*width)  = layout->width;
        (*height) = layout->height;
    }
}

/*************************************************************************************************/
/**
 * \brief Grows output bounds by a mapped local point.
 */
static void Extend(const Scene_t* scene, Bounds_t* bounds, float32_t x, float32_t y)
{
    FACEOE_Gfx_Point_t point;

    point = FACEOE_Gfx_DrawList_Map(scene->drawList, x, y);
    if (FALSE == bounds->has)
    {
        bounds->has     = TRUE;
        bounds->minimum = point;
        bounds->maximum = point;
    }
    else
    {
        if (point.x < bounds->minimum.x)
        {
            bounds->minimum.x = point.x;
        }
        if (point.y < bounds->minimum.y)
        {
            bounds->minimum.y = point.y;
        }
        if (bounds->maximum.x < point.x)
        {
            bounds->maximum.x = point.x;
        }
        if (bounds->maximum.y < point.y)
        {
            bounds->maximum.y = point.y;
        }
    }
}

/*************************************************************************************************/
/**
 * \brief Grows output bounds by a local box.
 */
static void ExtendBox(const Scene_t* scene, Bounds_t* bounds, float32_t x, float32_t y, float32_t width, float32_t height)
{
    Extend(scene, bounds, x, y);
    Extend(scene, bounds, x + width, y);
    Extend(scene, bounds, x + width, y + height);
    Extend(scene, bounds, x, y + height);
}

/*************************************************************************************************/
/**
 * \brief Picks a font size that fits a local height, in output units.
 */
static float32_t FontFor(const Scene_t* scene, float32_t height)
{
    float32_t unit_x;
    float32_t unit_y;
    float32_t pixels;
    float32_t size;

    size = scene->options->fontSize;
    if (0.0f < height)
    {
        FACEOE_Gfx_DrawList_UnitLengths(scene->drawList, &unit_x, &unit_y);
        pixels = (height * unit_y * SCENE_TEXT_FIT);
        if (pixels < size)
        {
            size = pixels;
        }
    }
    if (size < SCENE_MIN_FONT)
    {
        size = SCENE_MIN_FONT;
    }

    return size;
}

/*************************************************************************************************/
/**
 * \brief Converts an Alignment constant to box alignments.
 */
static void AlignOf(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, FACEOE_Gfx_Align_t* horizontal, FACEOE_Gfx_Align_t* vertical)
{
    (*horizontal) = FACEOE_GFX_ALIGN_CENTER;
    (*vertical)   = FACEOE_GFX_ALIGN_CENTER;
    if (Is(scene, widgetIndex, name, "A661_LEFT") || Is(scene, widgetIndex, name, "A661_TOP_LEFT") || Is(scene, widgetIndex, name, "A661_BOTTOM_LEFT"))
    {
        (*horizontal) = FACEOE_GFX_ALIGN_START;
    }
    else if (Is(scene, widgetIndex, name, "A661_RIGHT") || Is(scene, widgetIndex, name, "A661_TOP_RIGHT") || Is(scene, widgetIndex, name, "A661_BOTTOM_RIGHT"))
    {
        (*horizontal) = FACEOE_GFX_ALIGN_END;
    }
    if (Is(scene, widgetIndex, name, "A661_TOP") || Is(scene, widgetIndex, name, "A661_TOP_LEFT") || Is(scene, widgetIndex, name, "A661_TOP_CENTER") || Is(scene, widgetIndex, name, "A661_TOP_RIGHT"))
    {
        (*vertical) = FACEOE_GFX_ALIGN_START;
    }
    else if (Is(scene, widgetIndex, name, "A661_BOTTOM") || Is(scene, widgetIndex, name, "A661_BOTTOM_LEFT") || Is(scene, widgetIndex, name, "A661_BOTTOM_CENTER") || Is(scene, widgetIndex, name, "A661_BOTTOM_RIGHT"))
    {
        (*vertical) = FACEOE_GFX_ALIGN_END;
    }
}

/*************************************************************************************************/
/**
 * \brief Records text in a local box with a font that fits the box.
 */
static void BoxText(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* text, FACEOE_Gfx_Align_t horizontal, FACEOE_Gfx_Align_t vertical, float32_t radians, FACEOE_Gfx_Color_t color)
{
    if ('\0' != text[0])
    {
        FACEOE_Gfx_DrawList_Text(scene->drawList, x, y, width, height, text, horizontal, vertical, FontFor(scene, height), radians, color);
    }
}

/*************************************************************************************************/
/**
 * \brief Records a filled, outlined local box and grows the bounds.
 */
static void Frame(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t fill, FACEOE_Gfx_Color_t border, float32_t alpha, Bounds_t* bounds)
{
    if (0 != FACEOE_GFX_COLOR_ALPHA(fill))
    {
        FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, x, y, width, height, Fade(fill, alpha));
    }
    FACEOE_Gfx_DrawList_StrokeRectangle(scene->drawList, x, y, width, height, Fade(border, alpha), SCENE_THIN);
    ExtendBox(scene, bounds, x, y, width, height);
}

/*************************************************************************************************/
/**
 * \brief Records a dashed local box and grows the bounds.
 */
static void Dashed(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_DrawList_DashedRectangle(scene->drawList, x, y, width, height, SCENE_DASH_LENGTH, Fade(color, alpha), SCENE_THIN);
    ExtendBox(scene, bounds, x, y, width, height);
}

/*************************************************************************************************/
/**
 * \brief Records a reference-point marker: a circle, a cross, and a heading tick.
 */
static void Marker(const Scene_t* scene, float32_t x, float32_t y, float32_t radians, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Color_t faded;

    faded = Fade(color, alpha);
    FACEOE_Gfx_DrawList_Arc(scene->drawList, x, y, SCENE_MARKER_RADIUS, SCENE_MARKER_RADIUS, 0.0f, SCENE_FULL_TURN, FALSE, faded, SCENE_THIN);
    FACEOE_Gfx_DrawList_Line(scene->drawList, x - SCENE_MARKER_RADIUS, y, x + SCENE_MARKER_RADIUS, y, faded, SCENE_THIN);
    FACEOE_Gfx_DrawList_Line(scene->drawList, x, y - SCENE_MARKER_RADIUS, x, y + SCENE_MARKER_RADIUS, faded, SCENE_THIN);
    FACEOE_Gfx_DrawList_Line(scene->drawList, x, y, x + (2.0f * SCENE_MARKER_RADIUS * cosf(radians)), y + (2.0f * SCENE_MARKER_RADIUS * sinf(radians)), faded, SCENE_THICK);
    ExtendBox(scene, bounds, x - SCENE_MARKER_RADIUS, y - SCENE_MARKER_RADIUS, 2.0f * SCENE_MARKER_RADIUS, 2.0f * SCENE_MARKER_RADIUS);
}

/*************************************************************************************************/
/**
 * \brief Records a picture or source placeholder: a box with diagonals and a caption.
 */
static void Placeholder(const Scene_t* scene, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* caption, float32_t alpha, Bounds_t* bounds)
{
    Frame(scene, x, y, width, height, COLOR_PICTURE, COLOR_BORDER, alpha, bounds);
    FACEOE_Gfx_DrawList_Line(scene->drawList, x, y, x + width, y + height, Fade(COLOR_BORDER, alpha), SCENE_THIN);
    FACEOE_Gfx_DrawList_Line(scene->drawList, x, y + height, x + width, y, Fade(COLOR_BORDER, alpha), SCENE_THIN);
    BoxText(scene, x, y, width, height, caption, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
}

/*************************************************************************************************/
/**
 * \brief Records a scrollbar on the side named by a Top/Bottom/Left/Right property.
 */
static void Scrollbar(const Scene_t* scene, uint32_t widgetIndex, const char_t* name, float32_t x, float32_t y, float32_t width, float32_t height, float32_t fraction, float32_t start, float32_t alpha)
{
    float32_t thickness;
    float32_t bar_x;
    float32_t bar_y;
    float32_t bar_width;
    float32_t bar_height;
    bool_t    is_vertical;
    bool_t    is_present;

    thickness   = SCENE_CHIP_HEIGHT * 0.5f;
    is_present  = TRUE;
    is_vertical = FALSE;
    bar_x       = x;
    bar_y       = y;
    bar_width   = width;
    bar_height  = thickness;
    if (Is(scene, widgetIndex, name, "A661_LEFT"))
    {
        is_vertical = TRUE;
        bar_width   = thickness;
        bar_height  = height;
    }
    else if (Is(scene, widgetIndex, name, "A661_RIGHT"))
    {
        is_vertical = TRUE;
        bar_x       = (x + width - thickness);
        bar_width   = thickness;
        bar_height  = height;
    }
    else if (Is(scene, widgetIndex, name, "A661_TOP"))
    {
        bar_y = (y + height - thickness);
    }
    else if (FALSE == Is(scene, widgetIndex, name, "A661_BOTTOM"))
    {
        is_present = FALSE;
    }
    if (is_present)
    {
        if (fraction < 0.05f)
        {
            fraction = 0.05f;
        }
        if (1.0f < fraction)
        {
            fraction = 1.0f;
        }
        if (start < 0.0f)
        {
            start = 0.0f;
        }
        if ((1.0f - fraction) < start)
        {
            start = (1.0f - fraction);
        }
        FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, bar_x, bar_y, bar_width, bar_height, Fade(COLOR_SCROLLBAR, alpha * 0.5f));
        if (is_vertical)
        {
            // Run the thumb from the top of the bar, where the first entry sits
            FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, bar_x, bar_y + (bar_height * (1.0f - start - fraction)), bar_width, bar_height * fraction, Fade(COLOR_SCROLLBAR, alpha));
        }
        else
        {
            FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, bar_x + (bar_width * start), bar_y, bar_width * fraction, bar_height, Fade(COLOR_SCROLLBAR, alpha));
        }
    }
}

/*************************************************************************************************/
/**
 * \brief Records a chip for a widget without geometry along the layer's left edge.
 */
static void Chip(Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Bounds_t* bounds)
{
    const Arinc661_Widget_t* widget;
    char_t                   text[SCENE_TEXT_BYTES];
    float32_t                x;
    float32_t                y;

    if (scene->options->showsNonGraphical)
    {
        widget = &scene->definition->widgets[widgetIndex];
        x      = SCENE_PAD;
        y      = scene->layerHeight - SCENE_PAD - ((float32_t)(scene->chipCount + 1) * (SCENE_CHIP_HEIGHT + SCENE_CHIP_GAP));
        scene->chipCount += 1;
        if (FACEOE_Gfx_DrawList_PushTransform(scene->drawList))
        {
            // Place chips in layer space regardless of the container's transform
            FACEOE_Gfx_DrawList_SetTransform(scene->drawList, &scene->base);
            (void)snprintf(text, sizeof(text), "%s  %s", widget->name, widget->type);
            Frame(scene, x, y, SCENE_CHIP_WIDTH, SCENE_CHIP_HEIGHT, COLOR_CHIP, COLOR_CHIP_BORDER, alpha, bounds);
            BoxText(scene, x + SCENE_PAD, y, SCENE_CHIP_WIDTH - (2.0f * SCENE_PAD), SCENE_CHIP_HEIGHT, text, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
            FACEOE_Gfx_DrawList_PopTransform(scene->drawList);
        }
    }
}

/*************************************************************************************************/
/**
 * \brief Counts the extensions attached to a widget.
 */
static uint32_t ExtensionCount(const Scene_t* scene, uint32_t widgetIndex)
{
    uint32_t index;
    uint32_t count;

    count = 0;
    for (index = 0; index < scene->definition->widgetCount; ++index)
    {
        if (scene->definition->widgets[index].isExtension && (widgetIndex == scene->definition->widgets[index].parentWidgetIndex))
        {
            count += 1;
        }
    }

    return count;
}

/*************************************************************************************************/
/**
 * \brief Reports the widget's bounds and labels it with its name and extension count.
 */
static void Record(Scene_t* scene, uint32_t widgetIndex, const Bounds_t* bounds, float32_t alpha)
{
    Arinc661_Scene_Options_t* options;
    FACEOE_Gfx_Transform_t    identity;
    char_t                    text[SCENE_TEXT_BYTES];
    uint32_t                  extension_count;
    float32_t                 font_size;

    options = scene->options;
    if (bounds->has)
    {
        if ((NULL != options->bounds) && (options->boundsCount < options->boundsCapacity))
        {
            options->bounds[options->boundsCount].widgetIndex = widgetIndex;
            options->bounds[options->boundsCount].minimum     = bounds->minimum;
            options->bounds[options->boundsCount].maximum     = bounds->maximum;
            options->boundsCount += 1;
        }
        if (options->showsNames && FACEOE_Gfx_DrawList_PushTransform(scene->drawList))
        {
            extension_count = ExtensionCount(scene, widgetIndex);
            if (0 < extension_count)
            {
                (void)snprintf(text, sizeof(text), "%s +%u", scene->definition->widgets[widgetIndex].name, (unsigned)extension_count);
            }
            else
            {
                (void)snprintf(text, sizeof(text), "%s", scene->definition->widgets[widgetIndex].name);
            }
            // Draw names upright in output space at the bounds' top-left corner
            (void)memset(&identity, 0, sizeof(identity));
            identity.a = 1.0f;
            identity.d = 1.0f;
            FACEOE_Gfx_DrawList_SetTransform(scene->drawList, &identity);
            font_size = (options->fontSize * SCENE_NAME_FONT_SCALE);
            FACEOE_Gfx_DrawList_Text(scene->drawList, bounds->minimum.x + 2.0f, bounds->minimum.y + 1.0f, bounds->maximum.x - bounds->minimum.x, font_size + 2.0f, text, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_START, font_size, 0.0f, Fade(COLOR_NAME, alpha));
            FACEOE_Gfx_DrawList_PopTransform(scene->drawList);
        }
    }
}

/*************************************************************************************************/
/**
 * \brief Records a button face with an optional picture or symbol box beside its label.
 */
static void Button(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, const char_t* label, const char_t* iconName, const char_t* iconPosition, float32_t alpha, Bounds_t* bounds, float32_t* x, float32_t* y, float32_t* width, float32_t* height)
{
    FACEOE_Gfx_Align_t horizontal;
    FACEOE_Gfx_Align_t vertical;
    char_t             caption[SCENE_TEXT_BYTES];
    float32_t          label_x;
    float32_t          label_y;
    float32_t          label_width;
    float32_t          label_height;
    float32_t          icon_size;

    Rect(scene, widgetIndex, layout, x, y, width, height);
    Frame(scene, (*x), (*y), (*width), (*height), COLOR_BUTTON, COLOR_BUTTON_BORDER, alpha, bounds);
    label_x      = (*x);
    label_y      = (*y);
    label_width  = (*width);
    label_height = (*height);
    if ((NULL != iconName) && (NULL != Value(scene, widgetIndex, iconName, 0)))
    {
        // Reserve a square for the picture or symbol on the side the position names
        icon_size = (*height);
        if ((*width) < icon_size)
        {
            icon_size = (*width);
        }
        icon_size *= 0.6f;
        (void)snprintf(caption, sizeof(caption), "%s", Text(scene, widgetIndex, iconName));
        if (Is(scene, widgetIndex, iconPosition, "A661_LEFT"))
        {
            Placeholder(scene, (*x) + SCENE_PAD, (*y) + (((*height) - icon_size) * 0.5f), icon_size, icon_size, caption, alpha, bounds);
            label_x += (icon_size + (2.0f * SCENE_PAD));
            label_width -= (icon_size + (2.0f * SCENE_PAD));
        }
        else if (Is(scene, widgetIndex, iconPosition, "A661_RIGHT"))
        {
            Placeholder(scene, (*x) + (*width) - icon_size - SCENE_PAD, (*y) + (((*height) - icon_size) * 0.5f), icon_size, icon_size, caption, alpha, bounds);
            label_width -= (icon_size + (2.0f * SCENE_PAD));
        }
        else if (Is(scene, widgetIndex, iconPosition, "A661_TOP"))
        {
            Placeholder(scene, (*x) + (((*width) - icon_size) * 0.5f), (*y) + (*height) - icon_size - SCENE_PAD, icon_size, icon_size, caption, alpha, bounds);
            label_height -= (icon_size + (2.0f * SCENE_PAD));
        }
        else if (Is(scene, widgetIndex, iconPosition, "A661_BOTTOM"))
        {
            Placeholder(scene, (*x) + (((*width) - icon_size) * 0.5f), (*y) + SCENE_PAD, icon_size, icon_size, caption, alpha, bounds);
            label_y += (icon_size + (2.0f * SCENE_PAD));
            label_height -= (icon_size + (2.0f * SCENE_PAD));
        }
        else
        {
            Placeholder(scene, (*x) + (((*width) - icon_size) * 0.5f), (*y) + (((*height) - icon_size) * 0.5f), icon_size, icon_size, caption, alpha * 0.6f, bounds);
        }
    }
    if (NULL != label)
    {
        AlignOf(scene, widgetIndex, "Alignment", &horizontal, &vertical);
        BoxText(scene, label_x + SCENE_PAD, label_y, label_width - (2.0f * SCENE_PAD), label_height, label, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
    }
}

/*************************************************************************************************/
/**
 * \brief Records the rows of a list from a string array, highlighting the selected entry.
 */
static void EntryRows(const Scene_t* scene, uint32_t widgetIndex, const char_t* arrayName, const char_t* enableName, uint32_t first, uint32_t count, uint32_t selected, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Align_t align, float32_t alpha)
{
    const char_t* text;
    float32_t     unit_x;
    float32_t     unit_y;
    float32_t     row_height;
    float32_t     row_y;
    float32_t     row_alpha;
    uint32_t      entry;

    FACEOE_Gfx_DrawList_UnitLengths(scene->drawList, &unit_x, &unit_y);
    row_height = ((scene->options->fontSize * 1.3f) / unit_y);
    if (row_height <= 0.0f)
    {
        row_height = SCENE_CHIP_HEIGHT;
    }
    FACEOE_Gfx_DrawList_PushClip(scene->drawList, x, y, width, height);
    row_y = (y + height - row_height);
    for (entry = first; (entry < count) && ((y - row_height) < row_y); ++entry)
    {
        text = Value(scene, widgetIndex, arrayName, entry);
        if (NULL == text)
        {
            text = "";
        }
        row_alpha = alpha;
        if ((NULL != enableName) && IsAt(scene, widgetIndex, enableName, entry, "A661_FALSE"))
        {
            row_alpha *= SCENE_DISABLED_ALPHA;
        }
        if ((entry + 1) == selected)
        {
            FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, x, row_y, width, row_height, Fade(COLOR_ROW_SELECTED, alpha));
        }
        BoxText(scene, x + SCENE_PAD, row_y, width - (2.0f * SCENE_PAD), row_height, text, align, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, row_alpha));
        row_y -= row_height;
    }
    FACEOE_Gfx_DrawList_PopClip(scene->drawList);
}

/*************************************************************************************************/
/**
 * \brief Records a formatted numeric field with its legend: EditBoxNumeric, BCD, and NumericReadout.
 */
static void NumericBox(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t radians, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Align_t horizontal;
    FACEOE_Gfx_Align_t vertical;
    const char_t*      format;
    const char_t*      legend;
    char_t             text[SCENE_TEXT_BYTES];
    float32_t          x;
    float32_t          y;
    float32_t          width;
    float32_t          height;
    float32_t          field_x;
    float32_t          field_y;
    float32_t          field_width;
    float32_t          field_height;
    float32_t          legend_size;
    float32_t          unit_x;
    float32_t          unit_y;
    float32_t          legend_height;
    bool_t             has_legend;

    Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
    if (0.0f != radians)
    {
        FACEOE_Gfx_DrawList_Rotate(scene->drawList, x, y, radians);
    }
    Frame(scene, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, bounds);
    format = Value(scene, widgetIndex, "FormatString", 0);
    if (NULL == format)
    {
        format = "";
    }
    Arinc661_Scene_FormatNumber(format, (float64_t)NumberOr(scene, widgetIndex, "Value", 0.0f), text, sizeof(text));
    legend     = Value(scene, widgetIndex, "LegendString", 0);
    has_legend = ((NULL != legend) && ('\0' != legend[0]) && (FALSE == Is(scene, widgetIndex, "LegendRemoved", "A661_TRUE")));
    field_x      = x;
    field_y      = y;
    field_width  = width;
    field_height = height;
    if (has_legend)
    {
        legend_size = NumberOr(scene, widgetIndex, "LegendAreaSizeX", width * 0.3f);
        FACEOE_Gfx_DrawList_UnitLengths(scene->drawList, &unit_x, &unit_y);
        legend_height = ((scene->options->fontSize * 1.2f) / unit_y);
        if ((height * 0.5f) < legend_height)
        {
            legend_height = (height * 0.5f);
        }
        if (Is(scene, widgetIndex, "LegendPosition", "A661_RIGHT"))
        {
            field_width -= legend_size;
            BoxText(scene, x + width - legend_size, y, legend_size, height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        else if (Is(scene, widgetIndex, "LegendPosition", "A661_TOP"))
        {
            field_height -= legend_height;
            BoxText(scene, x, y + height - legend_height, width, legend_height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        else if (Is(scene, widgetIndex, "LegendPosition", "A661_BOTTOM"))
        {
            field_y += legend_height;
            field_height -= legend_height;
            BoxText(scene, x, y, width, legend_height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        else
        {
            field_x += legend_size;
            field_width -= legend_size;
            BoxText(scene, x, y, legend_size, height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        FACEOE_Gfx_DrawList_Line(scene->drawList, field_x, field_y, field_x, field_y + field_height, Fade(COLOR_EDIT_BORDER, alpha * 0.5f), SCENE_THIN);
    }
    AlignOf(scene, widgetIndex, "Alignment", &horizontal, &vertical);
    BoxText(scene, field_x + SCENE_PAD, field_y, field_width - (2.0f * SCENE_PAD), field_height, text, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
}

/*************************************************************************************************/
/**
 * \brief Records a graphics primitive: GpLine, GpLinePolar, GpArcCircle, GpArcEllipse, GpCrown,
 *  GpRectangle, GpTriangle, GpPolyline, GpTriangleFan, GpTriangleStrip.
 */
static void DrawGp(const Scene_t* scene, uint32_t widgetIndex, Kind_t kind, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Point_t points[SCENE_MAX_POINTS];
    FACEOE_Gfx_Point_t triangle[3];
    FACEOE_Gfx_Color_t color;
    FACEOE_Gfx_Color_t halo;
    float32_t          x;
    float32_t          y;
    float32_t          width;
    float32_t          height;
    float32_t          x2;
    float32_t          y2;
    float32_t          start;
    float32_t          end;
    float32_t          radius;
    float32_t          inner;
    float32_t          thickness;
    uint32_t           count;
    uint32_t           index;
    bool_t             is_filled;
    bool_t             has_halo;

    color     = IndexColor(scene, widgetIndex, alpha);
    halo      = Fade(FACEOE_GFX_RGBA(0, 0, 0, 255), alpha);
    is_filled = Is(scene, widgetIndex, "Filled", "A661_TRUE");
    has_halo  = (FALSE == Is(scene, widgetIndex, "Halo", "A661_FALSE")) && (NULL != Value(scene, widgetIndex, "Halo", 0));
    thickness = SCENE_THICK;
    x         = NumberOr(scene, widgetIndex, "PosX", 0.0f);
    y         = NumberOr(scene, widgetIndex, "PosY", 0.0f);
    width     = NumberOr(scene, widgetIndex, "SizeX", 0.0f);
    height    = NumberOr(scene, widgetIndex, "SizeY", 0.0f);
    start     = (NumberOr(scene, widgetIndex, "StartAngle", 0.0f) * SCENE_DEGREES);
    end       = (NumberOr(scene, widgetIndex, "EndAngle", 0.0f) * SCENE_DEGREES);
    switch (kind)
    {
        case KIND_GP_LINE:
            x  = NumberOr(scene, widgetIndex, "PosXStart", 0.0f);
            y  = NumberOr(scene, widgetIndex, "PosYStart", 0.0f);
            x2 = NumberOr(scene, widgetIndex, "PosXEnd", 0.0f);
            y2 = NumberOr(scene, widgetIndex, "PosYEnd", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Line(scene->drawList, x, y, x2, y2, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Line(scene->drawList, x, y, x2, y2, color, thickness);
            Extend(scene, bounds, x, y);
            Extend(scene, bounds, x2, y2);
            break;
        case KIND_GP_LINE_POLAR:
            x      = NumberOr(scene, widgetIndex, "PosXStart", 0.0f);
            y      = NumberOr(scene, widgetIndex, "PosYStart", 0.0f);
            radius = NumberOr(scene, widgetIndex, "LineLength", 0.0f);
            start  = (NumberOr(scene, widgetIndex, "RotationAngle", 0.0f) * SCENE_DEGREES);
            x2     = (x + (radius * cosf(start)));
            y2     = (y + (radius * sinf(start)));
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Line(scene->drawList, x, y, x2, y2, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Line(scene->drawList, x, y, x2, y2, color, thickness);
            Extend(scene, bounds, x, y);
            Extend(scene, bounds, x2, y2);
            break;
        case KIND_GP_ARC_CIRCLE:
            radius = NumberOr(scene, widgetIndex, "Radius", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Arc(scene->drawList, x, y, radius, radius, start, end, FALSE, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Arc(scene->drawList, x, y, radius, radius, start, end, is_filled, color, thickness);
            ExtendBox(scene, bounds, x - radius, y - radius, 2.0f * radius, 2.0f * radius);
            break;
        case KIND_GP_ARC_ELLIPSE:
            // Take SizeX and SizeY as the full axes of the ellipse centred on the reference point
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Arc(scene->drawList, x, y, width * 0.5f, height * 0.5f, start, end, FALSE, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Arc(scene->drawList, x, y, width * 0.5f, height * 0.5f, start, end, is_filled, color, thickness);
            ExtendBox(scene, bounds, x - (width * 0.5f), y - (height * 0.5f), width, height);
            break;
        case KIND_GP_CROWN:
            inner  = NumberOr(scene, widgetIndex, "InnerRadius", 0.0f);
            radius = NumberOr(scene, widgetIndex, "OuterRadius", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Crown(scene->drawList, x, y, inner, radius, start, end, FALSE, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Crown(scene->drawList, x, y, inner, radius, start, end, is_filled, color, thickness);
            ExtendBox(scene, bounds, x - radius, y - radius, 2.0f * radius, 2.0f * radius);
            break;
        case KIND_GP_RECTANGLE:
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_StrokeRectangle(scene->drawList, x, y, width, height, halo, thickness + 2.0f);
            }
            if (is_filled)
            {
                FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, x, y, width, height, color);
            }
            else
            {
                FACEOE_Gfx_DrawList_StrokeRectangle(scene->drawList, x, y, width, height, color, thickness);
            }
            ExtendBox(scene, bounds, x, y, width, height);
            break;
        case KIND_GP_TRIANGLE:
            triangle[0].x = x;
            triangle[0].y = y;
            triangle[1].x = NumberOr(scene, widgetIndex, "PosX2", 0.0f);
            triangle[1].y = NumberOr(scene, widgetIndex, "PosY2", 0.0f);
            triangle[2].x = NumberOr(scene, widgetIndex, "PosX3", 0.0f);
            triangle[2].y = NumberOr(scene, widgetIndex, "PosY3", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, triangle, 3, TRUE, halo, thickness + 2.0f);
            }
            if (is_filled)
            {
                FACEOE_Gfx_DrawList_FillPolygon(scene->drawList, triangle, 3, color);
            }
            else
            {
                FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, triangle, 3, TRUE, color, thickness);
            }
            for (index = 0; index < 3; ++index)
            {
                Extend(scene, bounds, triangle[index].x, triangle[index].y);
            }
            break;
        case KIND_GP_POLYLINE:
        case KIND_GP_TRIANGLE_FAN:
        case KIND_GP_TRIANGLE_STRIP:
            // Read vertices as alternating X and Y; bound the pairs by NumberOfVertices
            count = (uint32_t)NumberOr(scene, widgetIndex, "NumberOfVertices", 0.0f);
            if ((Occurrences(scene, widgetIndex, "Vertices") / 2) < count)
            {
                count = (Occurrences(scene, widgetIndex, "Vertices") / 2);
            }
            if (SCENE_MAX_POINTS < count)
            {
                count = SCENE_MAX_POINTS;
            }
            for (index = 0; index < count; ++index)
            {
                points[index].x = NumberAt(scene, widgetIndex, "Vertices", 2 * index, 0.0f);
                points[index].y = NumberAt(scene, widgetIndex, "Vertices", (2 * index) + 1, 0.0f);
                Extend(scene, bounds, points[index].x, points[index].y);
            }
            if (KIND_GP_POLYLINE == kind)
            {
                if (has_halo)
                {
                    FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, points, count, Is(scene, widgetIndex, "Closed", "A661_TRUE"), halo, thickness + 2.0f);
                }
                FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, points, count, Is(scene, widgetIndex, "Closed", "A661_TRUE"), color, thickness);
            }
            else
            {
                for (index = 2; index < count; ++index)
                {
                    if (KIND_GP_TRIANGLE_FAN == kind)
                    {
                        triangle[0] = points[0];
                    }
                    else
                    {
                        triangle[0] = points[index - 2];
                    }
                    triangle[1] = points[index - 1];
                    triangle[2] = points[index];
                    if (has_halo)
                    {
                        FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, triangle, 3, TRUE, halo, thickness + 2.0f);
                    }
                    if (is_filled)
                    {
                        FACEOE_Gfx_DrawList_FillPolygon(scene->drawList, triangle, 3, color);
                    }
                    else
                    {
                        FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, triangle, 3, TRUE, color, thickness);
                    }
                }
            }
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/**
 * \brief Records a slider: track, major and minor ticks, labels, and the thumb at Value.
 */
static void DrawSlider(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t alpha, Bounds_t* bounds)
{
    char_t    label[SCENE_TEXT_BYTES];
    float32_t x;
    float32_t y;
    float32_t width;
    float32_t height;
    float32_t minimum;
    float32_t maximum;
    float32_t value;
    float32_t interval;
    float32_t reference;
    float32_t minor_multiple;
    float32_t tick;
    float32_t fraction;
    float32_t along;
    float32_t track_x;
    float32_t track_y;
    float32_t tick_length;
    float32_t minor_length;
    float32_t thumb;
    float32_t side;
    float32_t unit_x;
    float32_t unit_y;
    float32_t label_height;
    uint32_t  tick_index;
    uint32_t  minor_index;
    bool_t    is_vertical;
    bool_t    is_reversed;
    bool_t    shows_labels;

    Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
    Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, bounds);
    track_x        = (x + (width * 0.5f));
    track_y        = (y + (height * 0.5f));
    minimum        = NumberOr(scene, widgetIndex, "MinValue", 0.0f);
    maximum        = NumberOr(scene, widgetIndex, "MaxValue", 100.0f);
    value          = NumberOr(scene, widgetIndex, "Value", minimum);
    interval       = NumberOr(scene, widgetIndex, "MajorTickInterval", 0.0f);
    reference      = NumberOr(scene, widgetIndex, "MajorTickReference", minimum);
    minor_multiple = NumberOr(scene, widgetIndex, "MinorTickMultiple", 0.0f);
    shows_labels   = Is(scene, widgetIndex, "ShowMajorLabels", "A661_TRUE");
    is_vertical    = (Is(scene, widgetIndex, "Orientation", "A661_BOTTOM_TO_TOP") || Is(scene, widgetIndex, "Orientation", "A661_TOP_TO_BOTTOM"));
    is_reversed    = (Is(scene, widgetIndex, "Orientation", "A661_RIGHT_TO_LEFT") || Is(scene, widgetIndex, "Orientation", "A661_TOP_TO_BOTTOM"));
    if (maximum <= minimum)
    {
        maximum = (minimum + 1.0f);
    }
    // Put ticks and labels on the side the Alignment names; give the track the rest
    side = 0.0f;
    if (Is(scene, widgetIndex, "Alignment", "A661_LEFT") || Is(scene, widgetIndex, "Alignment", "A661_BOTTOM"))
    {
        side = -1.0f;
    }
    else if (Is(scene, widgetIndex, "Alignment", "A661_RIGHT") || Is(scene, widgetIndex, "Alignment", "A661_TOP"))
    {
        side = 1.0f;
    }
    FACEOE_Gfx_DrawList_UnitLengths(scene->drawList, &unit_x, &unit_y);
    label_height = ((scene->options->fontSize * 1.1f) / unit_y);
    if (is_vertical)
    {
        track_x      = (x + (width * 0.5f) - (side * width * 0.25f));
        tick_length  = (width * 0.2f);
        minor_length = (tick_length * 0.5f);
        thumb        = (width * 0.3f);
        FACEOE_Gfx_DrawList_Line(scene->drawList, track_x, y + SCENE_PAD, track_x, y + height - SCENE_PAD, Fade(COLOR_TRACK, alpha), SCENE_THICK);
    }
    else
    {
        track_y      = (y + (height * 0.5f) - (side * height * 0.25f));
        tick_length  = (height * 0.2f);
        minor_length = (tick_length * 0.5f);
        thumb        = (height * 0.3f);
        FACEOE_Gfx_DrawList_Line(scene->drawList, x + SCENE_PAD, track_y, x + width - SCENE_PAD, track_y, Fade(COLOR_TRACK, alpha), SCENE_THICK);
    }
    if (0.0f < interval)
    {
        // Start from the reference and walk both ways across the range
        tick = reference;
        while (minimum < tick)
        {
            tick -= interval;
        }
        for (tick_index = 0; (tick <= maximum) && (tick_index < 1000); ++tick_index)
        {
            if (minimum <= tick)
            {
                fraction = ((tick - minimum) / (maximum - minimum));
                if (is_reversed)
                {
                    fraction = (1.0f - fraction);
                }
                if (is_vertical)
                {
                    along = (y + SCENE_PAD + (fraction * (height - (2.0f * SCENE_PAD))));
                    FACEOE_Gfx_DrawList_Line(scene->drawList, track_x, along, track_x + (side * tick_length), along, Fade(COLOR_TRACK, alpha), SCENE_THIN);
                    if (shows_labels)
                    {
                        (void)snprintf(label, sizeof(label), "%g", (float64_t)tick);
                        if (0.0f <= side)
                        {
                            BoxText(scene, track_x + tick_length, along - (label_height * 0.5f), (x + width) - (track_x + tick_length), label_height, label, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                        else
                        {
                            BoxText(scene, x, along - (label_height * 0.5f), (track_x - tick_length) - x, label_height, label, FACEOE_GFX_ALIGN_END, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                    }
                }
                else
                {
                    along = (x + SCENE_PAD + (fraction * (width - (2.0f * SCENE_PAD))));
                    FACEOE_Gfx_DrawList_Line(scene->drawList, along, track_y, along, track_y + (side * tick_length), Fade(COLOR_TRACK, alpha), SCENE_THIN);
                    if (shows_labels)
                    {
                        (void)snprintf(label, sizeof(label), "%g", (float64_t)tick);
                        if (0.0f <= side)
                        {
                            BoxText(scene, along - (width * 0.5f), track_y + tick_length, width, label_height, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_START, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                        else
                        {
                            BoxText(scene, along - (width * 0.5f), track_y - tick_length - label_height, width, label_height, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_END, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                    }
                }
            }
            if (1.0f < minor_multiple)
            {
                for (minor_index = 1; minor_index < (uint32_t)minor_multiple; ++minor_index)
                {
                    fraction = (((tick + (interval * ((float32_t)minor_index / minor_multiple))) - minimum) / (maximum - minimum));
                    if ((0.0f <= fraction) && (fraction <= 1.0f))
                    {
                        if (is_reversed)
                        {
                            fraction = (1.0f - fraction);
                        }
                        if (is_vertical)
                        {
                            along = (y + SCENE_PAD + (fraction * (height - (2.0f * SCENE_PAD))));
                            FACEOE_Gfx_DrawList_Line(scene->drawList, track_x, along, track_x + (side * minor_length), along, Fade(COLOR_TRACK, alpha), SCENE_THIN);
                        }
                        else
                        {
                            along = (x + SCENE_PAD + (fraction * (width - (2.0f * SCENE_PAD))));
                            FACEOE_Gfx_DrawList_Line(scene->drawList, along, track_y, along, track_y + (side * minor_length), Fade(COLOR_TRACK, alpha), SCENE_THIN);
                        }
                    }
                }
            }
            tick += interval;
        }
    }
    // Draw the thumb at Value
    fraction = ((value - minimum) / (maximum - minimum));
    if (fraction < 0.0f)
    {
        fraction = 0.0f;
    }
    if (1.0f < fraction)
    {
        fraction = 1.0f;
    }
    if (is_reversed)
    {
        fraction = (1.0f - fraction);
    }
    if (is_vertical)
    {
        along = (y + SCENE_PAD + (fraction * (height - (2.0f * SCENE_PAD))));
        FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, track_x - (thumb * 0.5f), along - (thumb * 0.3f), thumb, thumb * 0.6f, Fade(COLOR_BUTTON_BORDER, alpha));
    }
    else
    {
        along = (x + SCENE_PAD + (fraction * (width - (2.0f * SCENE_PAD))));
        FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, along - (thumb * 0.3f), track_y - (thumb * 0.5f), thumb * 0.6f, thumb, Fade(COLOR_BUTTON_BORDER, alpha));
    }
}

/*************************************************************************************************/
/**
 * \brief Records a tree: entries indented by their ParentArray depth, collapsed under closed parents.
 */
static void DrawTree(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Align_t horizontal;
    FACEOE_Gfx_Align_t vertical;
    const char_t*      text;
    float32_t          x;
    float32_t          y;
    float32_t          width;
    float32_t          height;
    float32_t          unit_x;
    float32_t          unit_y;
    float32_t          row_height;
    float32_t          row_y;
    float32_t          indent;
    float32_t          row_alpha;
    uint32_t           count;
    uint32_t           entry;
    uint32_t           parent;
    uint32_t           depth;
    uint32_t           first;
    uint32_t           selected;
    uint32_t           other;
    uint32_t           shown;
    uint32_t           guard;
    bool_t             is_shown;
    bool_t             has_children;

    Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
    Frame(scene, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, bounds);
    AlignOf(scene, widgetIndex, "Alignment", &horizontal, &vertical);
    count    = (uint32_t)NumberOr(scene, widgetIndex, "NumberOfEntries", 0.0f);
    first    = (uint32_t)NumberOr(scene, widgetIndex, "FirstVisibleEntry", 1.0f);
    selected = (uint32_t)NumberOr(scene, widgetIndex, "SelectedEntry", 0.0f);
    if (Occurrences(scene, widgetIndex, "LabelStringArray") < count)
    {
        count = Occurrences(scene, widgetIndex, "LabelStringArray");
    }
    FACEOE_Gfx_DrawList_UnitLengths(scene->drawList, &unit_x, &unit_y);
    row_height = ((scene->options->fontSize * 1.3f) / unit_y);
    indent     = (row_height * 1.2f);
    FACEOE_Gfx_DrawList_PushClip(scene->drawList, x, y, width, height);
    row_y = (y + height - row_height);
    shown = 0;
    for (entry = 0; (entry < count) && ((y - row_height) < row_y); ++entry)
    {
        // Show an entry only when every ancestor is open; read ParentArray as 1-based with 0 for roots
        is_shown = TRUE;
        depth    = 0;
        parent   = (uint32_t)NumberAt(scene, widgetIndex, "ParentArray", entry, 0.0f);
        for (guard = 0; (0 != parent) && (parent <= count) && (guard < count); ++guard)
        {
            depth += 1;
            if (IsAt(scene, widgetIndex, "OpenArray", parent - 1, "A661_FALSE"))
            {
                is_shown = FALSE;
            }
            parent = (uint32_t)NumberAt(scene, widgetIndex, "ParentArray", parent - 1, 0.0f);
        }
        if (is_shown)
        {
            shown += 1;
            if (first <= shown)
            {
                has_children = FALSE;
                for (other = 0; other < count; ++other)
                {
                    if ((entry + 1) == (uint32_t)NumberAt(scene, widgetIndex, "ParentArray", other, 0.0f))
                    {
                        has_children = TRUE;
                        break;
                    }
                }
                row_alpha = alpha;
                if (IsAt(scene, widgetIndex, "EnableArray", entry, "A661_FALSE"))
                {
                    row_alpha *= SCENE_DISABLED_ALPHA;
                }
                if ((entry + 1) == selected)
                {
                    FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, x, row_y, width, row_height, Fade(COLOR_ROW_SELECTED, alpha));
                }
                if (has_children)
                {
                    if (IsAt(scene, widgetIndex, "OpenArray", entry, "A661_FALSE"))
                    {
                        text = "+";
                    }
                    else
                    {
                        text = "-";
                    }
                    BoxText(scene, x + SCENE_PAD + ((float32_t)depth * indent), row_y, indent, row_height, text, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, row_alpha));
                }
                text = Value(scene, widgetIndex, "LabelStringArray", entry);
                if (NULL == text)
                {
                    text = "";
                }
                BoxText(scene, x + SCENE_PAD + ((float32_t)(depth + 1) * indent), row_y, width - (2.0f * SCENE_PAD) - ((float32_t)(depth + 1) * indent), row_height, text, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, row_alpha));
                row_y -= row_height;
            }
        }
    }
    FACEOE_Gfx_DrawList_PopClip(scene->drawList);
}

/*************************************************************************************************/
/**
 * \brief Records a MapBoundary: sections of arc, vertical, or horizontal lines around the origin.
 */
static void DrawMapBoundary(const Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Point_t points[SCENE_MAX_POINTS];
    float32_t          begin;
    float32_t          next;
    float32_t          distance;
    float32_t          angle;
    float32_t          radius;
    float32_t          sweep;
    uint32_t           count;
    uint32_t           section;
    uint32_t           point_count;
    uint32_t           step;
    uint32_t           step_count;
    bool_t             is_vertical;
    bool_t             is_horizontal;

    count = (uint32_t)NumberOr(scene, widgetIndex, "NumberOfEntries", 0.0f);
    if (Occurrences(scene, widgetIndex, "BeginAngleArray") < count)
    {
        count = Occurrences(scene, widgetIndex, "BeginAngleArray");
    }
    for (section = 0; section < count; ++section)
    {
        begin         = NumberAt(scene, widgetIndex, "BeginAngleArray", section, 0.0f);
        next          = NumberAt(scene, widgetIndex, "BeginAngleArray", (section + 1) % count, 0.0f);
        distance      = NumberAt(scene, widgetIndex, "DistanceArray", section, 0.0f);
        is_vertical   = IsAt(scene, widgetIndex, "AlgorithmArray", section, "A661_VERT_LINE");
        is_horizontal = IsAt(scene, widgetIndex, "AlgorithmArray", section, "A661_HORZ_LINE");
        sweep         = (next - begin);
        if (sweep <= 0.0f)
        {
            sweep += 360.0f;
        }
        step_count = (uint32_t)(sweep / SCENE_BOUNDARY_STEP);
        if (step_count < 1)
        {
            step_count = 1;
        }
        if ((SCENE_MAX_POINTS - 1) < step_count)
        {
            step_count = (SCENE_MAX_POINTS - 1);
        }
        point_count = 0;
        for (step = 0; step <= step_count; ++step)
        {
            angle  = ((begin + (sweep * ((float32_t)step / (float32_t)step_count))) * SCENE_DEGREES);
            radius = distance;
            // Meet a line at perpendicular distance D along the ray at D over the cosine of the angle to its normal
            if (is_vertical && (0.001f < fabsf(cosf(angle))))
            {
                radius = (distance / fabsf(cosf(angle)));
            }
            else if (is_horizontal && (0.001f < fabsf(sinf(angle))))
            {
                radius = (distance / fabsf(sinf(angle)));
            }
            if ((4.0f * distance) < radius)
            {
                radius = (4.0f * distance);
            }
            points[point_count].x = (radius * cosf(angle));
            points[point_count].y = (radius * sinf(angle));
            Extend(scene, bounds, points[point_count].x, points[point_count].y);
            point_count += 1;
        }
        FACEOE_Gfx_DrawList_StrokePolyline(scene->drawList, points, point_count, FALSE, Fade(COLOR_RANGE, alpha), SCENE_THIN);
    }
    if (0 == count)
    {
        Marker(scene, 0.0f, 0.0f, 0.0f, COLOR_RANGE, alpha, bounds);
    }
}

/*************************************************************************************************/
/**
 * \brief Records a MapGrid: CountX by CountY cells of IncrementX by IncrementY, offset so the
 *  cell at (OffsetX, OffsetY) sits on the display reference point.
 */
static void DrawMapGrid(const Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Bounds_t* bounds)
{
    float32_t offset_x;
    float32_t offset_y;
    float32_t increment_x;
    float32_t increment_y;
    float32_t origin_x;
    float32_t origin_y;
    float32_t count_x;
    float32_t count_y;
    uint32_t  line;

    offset_x    = NumberOr(scene, widgetIndex, "OffsetX", 0.0f);
    offset_y    = NumberOr(scene, widgetIndex, "OffsetY", 0.0f);
    increment_x = NumberOr(scene, widgetIndex, "IncrementX", 100.0f);
    increment_y = NumberOr(scene, widgetIndex, "IncrementY", 100.0f);
    count_x     = NumberOr(scene, widgetIndex, "CountX", 0.0f);
    count_y     = NumberOr(scene, widgetIndex, "CountY", 0.0f);
    if (200.0f < count_x)
    {
        count_x = 200.0f;
    }
    if (200.0f < count_y)
    {
        count_y = 200.0f;
    }
    origin_x = (-offset_x * increment_x);
    origin_y = (-offset_y * increment_y);
    for (line = 0; line <= (uint32_t)count_x; ++line)
    {
        FACEOE_Gfx_DrawList_Line(scene->drawList, origin_x + ((float32_t)line * increment_x), origin_y, origin_x + ((float32_t)line * increment_x), origin_y + (count_y * increment_y), Fade(COLOR_GRID, alpha), SCENE_THIN);
    }
    for (line = 0; line <= (uint32_t)count_y; ++line)
    {
        FACEOE_Gfx_DrawList_Line(scene->drawList, origin_x, origin_y + ((float32_t)line * increment_y), origin_x + (count_x * increment_x), origin_y + ((float32_t)line * increment_y), Fade(COLOR_GRID, alpha), SCENE_THIN);
    }
    ExtendBox(scene, bounds, origin_x, origin_y, count_x * increment_x, count_y * increment_y);
}

/*************************************************************************************************/
/**
 * \brief Records a TabbedPanelGroup: a tab strip on TabPosition with one tab per TabbedPanel
 *  child, and the content area the active child fills.
 */
static void DrawTabbedGroup(const Scene_t* scene, uint32_t widgetIndex, const Layout_t* layout, float32_t alpha, Bounds_t* bounds, Children_t* children)
{
    const char_t* label;
    float32_t     x;
    float32_t     y;
    float32_t     width;
    float32_t     height;
    float32_t     strip;
    float32_t     unit_x;
    float32_t     unit_y;
    float32_t     along;
    float32_t     inset;
    float32_t     tab_alpha;
    uint32_t      child;
    uint32_t      tab_count;
    bool_t        is_automatic;
    bool_t        is_active;
    int32_t       position;

    Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
    Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, bounds);
    FACEOE_Gfx_DrawList_UnitLengths(scene->drawList, &unit_x, &unit_y);
    strip = ((scene->options->fontSize * 1.6f) / unit_y);
    if ((height * 0.3f) < strip)
    {
        strip = (height * 0.3f);
    }
    // Encode the tab position as 0 bottom, 1 top, 2 left, 3 right, -1 absent
    position = 1;
    if (Is(scene, widgetIndex, "TabPosition", "A661_BOTTOM"))
    {
        position = 0;
    }
    else if (Is(scene, widgetIndex, "TabPosition", "A661_LEFT"))
    {
        position = 2;
    }
    else if (Is(scene, widgetIndex, "TabPosition", "A661_RIGHT"))
    {
        position = 3;
    }
    else if (Is(scene, widgetIndex, "TabPosition", "A661_ABSENT"))
    {
        position = -1;
    }
    children->mode          = CHILD_TABS;
    children->ident         = (int64_t)NumberOr(scene, widgetIndex, "ActiveTabbedPanelID", 0.0f);
    children->content.isSet = TRUE;
    children->content.x     = x;
    children->content.y     = y;
    children->content.width = width;
    children->content.height = height;
    if (0 == position)
    {
        children->content.y += strip;
        children->content.height -= strip;
    }
    else if (1 == position)
    {
        children->content.height -= strip;
    }
    else if (2 == position)
    {
        children->content.x += strip;
        children->content.width -= strip;
    }
    else if (3 == position)
    {
        children->content.width -= strip;
    }
    if (0 <= position)
    {
        is_automatic = Is(scene, widgetIndex, "AutomaticInsetSizeFlag", "A661_TRUE");
        tab_count    = 0;
        for (child = 0; child < scene->definition->widgetCount; ++child)
        {
            if ((widgetIndex == scene->definition->widgets[child].parentWidgetIndex) && (FALSE == scene->definition->widgets[child].isExtension))
            {
                tab_count += 1;
            }
        }
        along = 0.0f;
        for (child = 0; (child < scene->definition->widgetCount) && (0 < tab_count); ++child)
        {
            if ((widgetIndex == scene->definition->widgets[child].parentWidgetIndex) && (FALSE == scene->definition->widgets[child].isExtension))
            {
                inset = NumberOr(scene, child, "InsetSize", 0.0f);
                if (is_automatic || (inset <= 0.0f))
                {
                    if (position < 2)
                    {
                        inset = (width / (float32_t)tab_count);
                    }
                    else
                    {
                        inset = (height / (float32_t)tab_count);
                    }
                }
                is_active = (Ident(scene, child) == children->ident);
                tab_alpha = alpha;
                if (FALSE == is_active)
                {
                    tab_alpha *= 0.7f;
                }
                label = Value(scene, child, "LabelString", 0);
                if (NULL == label)
                {
                    label = scene->definition->widgets[child].name;
                }
                if (0 == position)
                {
                    Frame(scene, x + along, y, inset, strip, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(scene, x + along, y, inset, strip, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                else if (1 == position)
                {
                    Frame(scene, x + along, y + height - strip, inset, strip, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(scene, x + along, y + height - strip, inset, strip, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                else if (2 == position)
                {
                    Frame(scene, x, y + height - along - inset, strip, inset, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(scene, x, y + height - along - inset, strip, inset, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                else
                {
                    Frame(scene, x + width - strip, y + height - along - inset, strip, inset, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(scene, x + width - strip, y + height - along - inset, strip, inset, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                along += inset;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \brief Records one widget by its Creation Structure, then its children through the policy
 *  its kind imposes.
 */
static void DrawWidget(Scene_t* scene, uint32_t widgetIndex, float32_t alpha, const Layout_t* layout)
{
    const Arinc661_Widget_t* widget;
    const char_t*            text;
    const char_t*            value;
    FACEOE_Gfx_Align_t       horizontal;
    FACEOE_Gfx_Align_t       vertical;
    Children_t               children;
    Bounds_t                 bounds;
    Kind_t                   kind;
    char_t                   caption[SCENE_TEXT_BYTES];
    float32_t                x;
    float32_t                y;
    float32_t                width;
    float32_t                height;
    float32_t                angle;
    float32_t                area_x;
    float32_t                area_y;
    float32_t                area_width;
    float32_t                area_height;
    float32_t                box;
    float32_t                fraction;
    float32_t                start;
    uint32_t                 count;
    uint32_t                 selected;
    uint32_t                 first;
    uint32_t                 state;
    uint32_t                 index;
    bool_t                   is_visible;
    bool_t                   has_clip;
    bool_t                   is_drawn;

    widget = &scene->definition->widgets[widgetIndex];
    kind   = KindOf(widget->type);
    // Propagate Visible and Enable to children; skip hidden subtrees unless they are shown faded
    is_visible = (FALSE == Is(scene, widgetIndex, "Visible", "A661_FALSE"));
    is_drawn   = TRUE;
    if (FALSE == is_visible)
    {
        if (scene->options->showsHidden)
        {
            alpha *= SCENE_HIDDEN_ALPHA;
        }
        else
        {
            is_drawn = FALSE;
        }
    }
    if (is_drawn && Is(scene, widgetIndex, "Enable", "A661_FALSE"))
    {
        alpha *= SCENE_DISABLED_ALPHA;
    }
    if (is_drawn && (FALSE == FACEOE_Gfx_DrawList_PushTransform(scene->drawList)))
    {
        is_drawn = FALSE;
    }
    if (is_drawn)
    {
        (void)memset(&children, 0, sizeof(children));
        (void)memset(&bounds, 0, sizeof(bounds));
        children.mode = CHILD_ALL;
        has_clip      = FALSE;
        switch (kind)
        {
            case KIND_PANEL:
            case KIND_SHUFFLE_TO_FIT_CONTAINER:
            case KIND_SIZE_TO_FIT_CONTAINER:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_PAGING_CONTAINER:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                if (FALSE == Is(scene, widgetIndex, "PagingControlPosition", "A661_ABSENT"))
                {
                    // Draw the paging control as a small strip of page marks at the named position
                    AlignOf(scene, widgetIndex, "PagingControlPosition", &horizontal, &vertical);
                    BoxText(scene, x + SCENE_PAD, y + SCENE_PAD, width - (2.0f * SCENE_PAD), height - (2.0f * SCENE_PAD), "< o o o >", horizontal, vertical, 0.0f, Fade(COLOR_TEXT, alpha * 0.8f));
                }
                children.mode  = CHILD_IDENT;
                children.ident = (int64_t)NumberOr(scene, widgetIndex, "VisibleChild", 0.0f);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_MUTUALLY_EXCLUSIVE_CONTAINER:
                x = NumberOr(scene, widgetIndex, "PosX", 0.0f);
                y = NumberOr(scene, widgetIndex, "PosY", 0.0f);
                Marker(scene, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                children.mode  = CHILD_IDENT;
                children.ident = (int64_t)NumberOr(scene, widgetIndex, "VisibleChild", 0.0f);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_BASIC_CONTAINER:
            case KIND_MASK_CONTAINER:
            case KIND_CURSOR_REF:
                x = NumberOr(scene, widgetIndex, "PosX", 0.0f);
                y = NumberOr(scene, widgetIndex, "PosY", 0.0f);
                Marker(scene, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_TRANSLATION_CONTAINER:
                x = NumberOr(scene, widgetIndex, "TranslationX", 0.0f);
                y = NumberOr(scene, widgetIndex, "TranslationY", 0.0f);
                Marker(scene, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_ROTATION_CONTAINER:
                x     = NumberOr(scene, widgetIndex, "CenterX", 0.0f);
                y     = NumberOr(scene, widgetIndex, "CenterY", 0.0f);
                angle = (NumberOr(scene, widgetIndex, "RotationAngle", 0.0f) * SCENE_DEGREES);
                Marker(scene, x, y, angle, COLOR_MARKER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Rotate(scene->drawList, x, y, angle);
                break;
            case KIND_SCALE_CONTAINER:
                x = NumberOr(scene, widgetIndex, "CenterX", 0.0f);
                y = NumberOr(scene, widgetIndex, "CenterY", 0.0f);
                Marker(scene, x, y, 0.0f, COLOR_MARKER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Scale(scene->drawList, x, y, NumberOr(scene, widgetIndex, "ScaleX", 1.0f), NumberOr(scene, widgetIndex, "ScaleY", 1.0f));
                break;
            case KIND_SCROLL_PANEL:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                area_width  = NumberOr(scene, widgetIndex, "SizeXsheet", width);
                area_height = NumberOr(scene, widgetIndex, "SizeYsheet", height);
                area_x      = NumberOr(scene, widgetIndex, "FrameX", 0.0f);
                area_y      = NumberOr(scene, widgetIndex, "FrameY", 0.0f);
                if (0.0f < area_width)
                {
                    Scrollbar(scene, widgetIndex, "HorizontalScroll", x, y, width, height, width / area_width, area_x / area_width, alpha);
                }
                if (0.0f < area_height)
                {
                    Scrollbar(scene, widgetIndex, "VerticalScroll", x, y, width, height, height / area_height, 1.0f - ((area_y + height) / area_height), alpha);
                }
                // Place children on the sheet and show the sheet through the frame from (FrameX, FrameY)
                FACEOE_Gfx_DrawList_PushClip(scene->drawList, x, y, width, height);
                has_clip = TRUE;
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x - area_x, y - area_y);
                if (scene->options->showsHidden)
                {
                    FACEOE_Gfx_DrawList_DashedRectangle(scene->drawList, 0.0f, 0.0f, area_width, area_height, SCENE_DASH_LENGTH, Fade(COLOR_BORDER, alpha * 0.5f), SCENE_THIN);
                }
                break;
            case KIND_TABBED_PANEL_GROUP:
                DrawTabbedGroup(scene, widgetIndex, layout, alpha, &bounds, &children);
                break;
            case KIND_TABBED_PANEL:
                // Take the content area the group hands down and position children from it
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_MENU_BAR:
                x                       = NumberOr(scene, widgetIndex, "PosX", 0.0f);
                y                       = NumberOr(scene, widgetIndex, "PosY", 0.0f);
                children.mode           = CHILD_MENU_BAR;
                children.isHorizontal   = (FALSE == Is(scene, widgetIndex, "Horizontal", "A661_FALSE"));
                children.buttonPosition = NumberOr(scene, widgetIndex, "ButtonPos", 0.0f);
                children.buttonSize     = NumberOr(scene, widgetIndex, "ButtonSize", 0.0f);
                Marker(scene, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_MAPHORZ:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                area_x = NumberOr(scene, widgetIndex, "ScreenReferencePointX", 0.0f);
                area_y = NumberOr(scene, widgetIndex, "ScreenReferencePointY", 0.0f);
                box    = NumberOr(scene, widgetIndex, "ScreenRange", 0.0f);
                FACEOE_Gfx_DrawList_PushClip(scene->drawList, x, y, width, height);
                has_clip = TRUE;
                if (0.0f < box)
                {
                    FACEOE_Gfx_DrawList_Arc(scene->drawList, x + area_x, y + area_y, box, box, 0.0f, SCENE_FULL_TURN, FALSE, Fade(COLOR_RANGE, alpha), SCENE_THIN);
                }
                Marker(scene, x + area_x, y + area_y, (90.0f * SCENE_DEGREES), COLOR_RANGE, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x + area_x, y + area_y);
                break;
            case KIND_MAPVERT:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                area_x = NumberOr(scene, widgetIndex, "RefPosX", 0.0f);
                area_y = NumberOr(scene, widgetIndex, "RefPosY", 0.0f);
                FACEOE_Gfx_DrawList_PushClip(scene->drawList, x, y, width, height);
                has_clip = TRUE;
                Marker(scene, x + area_x, y + area_y, 0.0f, COLOR_RANGE, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x + area_x, y + area_y);
                break;
            case KIND_MAP_GRID:
                DrawMapGrid(scene, widgetIndex, alpha, &bounds);
                break;
            case KIND_MAP_BOUNDARY:
                DrawMapBoundary(scene, widgetIndex, alpha, &bounds);
                break;
            case KIND_MAPHORZ_VERTEXBUFFER:
                DrawGp(scene, widgetIndex, KIND_GP_POLYLINE, alpha, &bounds);
                break;
            case KIND_ACTIVE_AREA:
            case KIND_CURSOR_OVER:
            case KIND_CURSOR_POS_OVERLAY:
            case KIND_TOUCH_AREA:
            case KIND_GESTURE_AREA:
            case KIND_SCROLL_WHEEL_AREA:
            case KIND_KEYBOARD_AREA:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Dashed(scene, x, y, width, height, COLOR_AREA, alpha, &bounds);
                if (KIND_KEYBOARD_AREA == kind)
                {
                    (void)snprintf(caption, sizeof(caption), "%u keys", (unsigned)NumberOr(scene, widgetIndex, "NumberOfKeys", 0.0f));
                    BoxText(scene, x, y, width, height, caption, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_AREA, alpha));
                }
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_INK_AREA:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                if (Is(scene, widgetIndex, "Filled", "A661_TRUE"))
                {
                    FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, x, y, width, height, IndexColor(scene, widgetIndex, alpha * 0.5f));
                }
                Dashed(scene, x, y, width, height, COLOR_AREA, alpha, &bounds);
                break;
            case KIND_EXTERNAL_SOURCE:
            case KIND_MAP_EXTERNAL_SOURCE:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                (void)snprintf(caption, sizeof(caption), "Source %s", Text(scene, widgetIndex, "SourceReference"));
                Placeholder(scene, x, y, width, height, caption, alpha, &bounds);
                break;
            case KIND_PICTURE:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                (void)snprintf(caption, sizeof(caption), "Picture %s", Text(scene, widgetIndex, "PictureReference"));
                Placeholder(scene, x, y, width, height, caption, alpha, &bounds);
                break;
            case KIND_PICTURE_ANIMATED:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                (void)snprintf(caption, sizeof(caption), "%u pictures", (unsigned)NumberOr(scene, widgetIndex, "NumberOfPictures", 0.0f));
                Placeholder(scene, x, y, width, height, caption, alpha, &bounds);
                break;
            case KIND_SYMBOL:
                x     = NumberOr(scene, widgetIndex, "PosX", 0.0f);
                y     = NumberOr(scene, widgetIndex, "PosY", 0.0f);
                angle = (NumberOr(scene, widgetIndex, "RotationAngle", 0.0f) * SCENE_DEGREES);
                Marker(scene, x, y, angle, IndexColor(scene, widgetIndex, 1.0f), alpha, &bounds);
                (void)snprintf(caption, sizeof(caption), "Symbol %s", Text(scene, widgetIndex, "SymbolReference"));
                BoxText(scene, x + SCENE_MARKER_RADIUS, y - SCENE_MARKER_RADIUS, SCENE_CHIP_WIDTH, SCENE_MARKER_RADIUS, caption, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                break;
            case KIND_SYMBOL_ANIMATED:
                x = NumberOr(scene, widgetIndex, "PosX", 0.0f);
                y = NumberOr(scene, widgetIndex, "PosY", 0.0f);
                Marker(scene, x, y, (NumberAt(scene, widgetIndex, "OrientationArray", 0, 0.0f) * SCENE_DEGREES), IndexColor(scene, widgetIndex, 1.0f), alpha, &bounds);
                (void)snprintf(caption, sizeof(caption), "%u symbols", (unsigned)NumberOr(scene, widgetIndex, "NumberOfSymbols", 0.0f));
                BoxText(scene, x + SCENE_MARKER_RADIUS, y - SCENE_MARKER_RADIUS, SCENE_CHIP_WIDTH, SCENE_MARKER_RADIUS, caption, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                break;
            case KIND_LABEL:
            case KIND_LABEL_COMPLEX:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                angle = (NumberOr(scene, widgetIndex, "RotationAngle", 0.0f) * SCENE_DEGREES);
                if (0.0f != angle)
                {
                    FACEOE_Gfx_DrawList_Rotate(scene->drawList, x, y, angle);
                }
                text = Value(scene, widgetIndex, "LabelString", 0);
                if (NULL == text)
                {
                    text = "";
                }
                AlignOf(scene, widgetIndex, "Alignment", &horizontal, &vertical);
                if (scene->options->showsHidden)
                {
                    Dashed(scene, x, y, width, height, COLOR_BORDER, alpha * 0.5f, &bounds);
                }
                else
                {
                    ExtendBox(scene, &bounds, x, y, width, height);
                }
                BoxText(scene, x, y, width, height, text, horizontal, vertical, 0.0f, IndexColor(scene, widgetIndex, alpha));
                break;
            case KIND_EDIT_BOX_TEXT:
            case KIND_EDIT_BOX_MASKED:
            case KIND_EDIT_BOX_MULTI_LINE:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, &bounds);
                text = Value(scene, widgetIndex, "LabelString", 0);
                if (NULL == text)
                {
                    text = "";
                }
                AlignOf(scene, widgetIndex, "Alignment", &horizontal, &vertical);
                if (KIND_EDIT_BOX_MULTI_LINE == kind)
                {
                    Scrollbar(scene, widgetIndex, "VerticalScroll", x, y, width, height, 1.0f, 0.0f, alpha);
                    BoxText(scene, x + SCENE_PAD, y + SCENE_PAD, width - (2.0f * SCENE_PAD), height - (2.0f * SCENE_PAD), text, horizontal, FACEOE_GFX_ALIGN_START, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                else
                {
                    BoxText(scene, x + SCENE_PAD, y, width - (2.0f * SCENE_PAD), height, text, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                break;
            case KIND_EDIT_BOX_NUMERIC:
            case KIND_EDIT_BOX_NUMERIC_BCD:
                NumericBox(scene, widgetIndex, layout, 0.0f, alpha, &bounds);
                break;
            case KIND_NUMERIC_READOUT:
                NumericBox(scene, widgetIndex, layout, (NumberOr(scene, widgetIndex, "RotationAngle", 0.0f) * SCENE_DEGREES), alpha, &bounds);
                break;
            case KIND_PUSH_BUTTON:
                Button(scene, widgetIndex, layout, Value(scene, widgetIndex, "LabelString", 0), NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_TOGGLE_BUTTON:
            case KIND_PICTURE_TOGGLE_BUTTON:
            case KIND_SYMBOL_TOGGLE_BUTTON:
                // Show the alternate label and picture of a selected toggle with AlternateFlag
                text = Value(scene, widgetIndex, "LabelString", 0);
                if (Is(scene, widgetIndex, "ToggleState", "A661_SELECTED") && Is(scene, widgetIndex, "AlternateFlag", "A661_TRUE") && (NULL != Value(scene, widgetIndex, "AlternateLabelString", 0)))
                {
                    text = Value(scene, widgetIndex, "AlternateLabelString", 0);
                }
                if (KIND_PICTURE_TOGGLE_BUTTON == kind)
                {
                    Button(scene, widgetIndex, layout, text, "PictureReference", "PicturePosition", alpha, &bounds, &x, &y, &width, &height);
                }
                else if (KIND_SYMBOL_TOGGLE_BUTTON == kind)
                {
                    Button(scene, widgetIndex, layout, text, "SymbolReference", "SymbolPosition", alpha, &bounds, &x, &y, &width, &height);
                }
                else
                {
                    Button(scene, widgetIndex, layout, text, NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                }
                if (Is(scene, widgetIndex, "ToggleState", "A661_SELECTED"))
                {
                    FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, x, y, width, height, Fade(COLOR_ROW_SELECTED, alpha * 0.6f));
                }
                break;
            case KIND_PICTURE_PUSH_BUTTON:
                Button(scene, widgetIndex, layout, Value(scene, widgetIndex, "LabelString", 0), "PictureReference", "PicturePosition", alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_SYMBOL_PUSH_BUTTON:
                Button(scene, widgetIndex, layout, Value(scene, widgetIndex, "LabelString", 0), "SymbolReference", "SymbolPosition", alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_CHECK_BUTTON:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                box = (height * 0.6f);
                if (width < box)
                {
                    box = width;
                }
                // Put the check box on the side PicturePosition names and the label beside it
                if (Is(scene, widgetIndex, "PicturePosition", "A661_RIGHT"))
                {
                    area_x = (x + width - box - SCENE_PAD);
                    BoxText(scene, x + SCENE_PAD, y, width - box - (3.0f * SCENE_PAD), height, Text(scene, widgetIndex, "LabelString"), FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                else
                {
                    area_x = (x + SCENE_PAD);
                    BoxText(scene, x + box + (2.0f * SCENE_PAD), y, width - box - (3.0f * SCENE_PAD), height, Text(scene, widgetIndex, "LabelString"), FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                FACEOE_Gfx_DrawList_StrokeRectangle(scene->drawList, area_x, y + ((height - box) * 0.5f), box, box, Fade(COLOR_BUTTON_BORDER, alpha), SCENE_THIN);
                if (Is(scene, widgetIndex, "CheckButtonState", "A661_SELECTED"))
                {
                    FACEOE_Gfx_DrawList_FillRectangle(scene->drawList, area_x + (box * 0.2f), y + ((height - box) * 0.5f) + (box * 0.2f), box * 0.6f, box * 0.6f, Fade(COLOR_BUTTON_BORDER, alpha));
                }
                break;
            case KIND_MULTI_STATE_BUTTON:
                state = (uint32_t)NumberOr(scene, widgetIndex, "ButtonState", 1.0f);
                text  = NULL;
                if (0 < state)
                {
                    text = Value(scene, widgetIndex, "LabelStringArray", state - 1);
                }
                Button(scene, widgetIndex, layout, text, NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_COMBO_BOX:
            case KIND_COMBO_BOX_EDIT:
            case KIND_SELECTION_LIST_BUTTON:
                selected = (uint32_t)NumberOr(scene, widgetIndex, "SelectedEntry", 0.0f);
                text     = NULL;
                if (KIND_SELECTION_LIST_BUTTON == kind)
                {
                    text = Value(scene, widgetIndex, "LabelString", 0);
                }
                else if (0 < selected)
                {
                    text = Value(scene, widgetIndex, "EntryList", selected - 1);
                }
                Button(scene, widgetIndex, layout, text, NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                // Draw the drop arrow at the right edge
                box = (height * 0.3f);
                FACEOE_Gfx_DrawList_Line(scene->drawList, x + width - (2.0f * box) - SCENE_PAD, y + (height * 0.5f) + (box * 0.4f), x + width - box - SCENE_PAD, y + (height * 0.5f) - (box * 0.4f), Fade(COLOR_TEXT, alpha), SCENE_THICK);
                FACEOE_Gfx_DrawList_Line(scene->drawList, x + width - box - SCENE_PAD, y + (height * 0.5f) - (box * 0.4f), x + width - SCENE_PAD, y + (height * 0.5f) + (box * 0.4f), Fade(COLOR_TEXT, alpha), SCENE_THICK);
                if (scene->options->showsHidden)
                {
                    // Show the selecting area closed and dashed where OpeningMode opens it
                    area_width  = NumberOr(scene, widgetIndex, "SelectingAreaWidth", width);
                    area_height = NumberOr(scene, widgetIndex, "SelectingAreaHeight", height * 3.0f);
                    area_x      = x;
                    area_y      = (y - area_height);
                    if (Is(scene, widgetIndex, "OpeningMode", "A661_OPEN_UP"))
                    {
                        area_y = (y + height);
                    }
                    else if (Is(scene, widgetIndex, "OpeningMode", "A661_OPEN_CENTERED"))
                    {
                        area_y = (y + (height * 0.5f) - (area_height * 0.5f));
                    }
                    Dashed(scene, area_x, area_y, area_width, area_height, COLOR_BORDER, alpha * 0.5f, &bounds);
                    EntryRows(scene, widgetIndex, "EntryList", NULL, 0, (uint32_t)NumberOr(scene, widgetIndex, "NumberOfEntries", 0.0f), selected, area_x, area_y, area_width, area_height, FACEOE_GFX_ALIGN_START, alpha * 0.5f);
                }
                break;
            case KIND_SCROLL_LIST:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, &bounds);
                count    = (uint32_t)NumberOr(scene, widgetIndex, "NumberOfEntries", 0.0f);
                first    = (uint32_t)NumberOr(scene, widgetIndex, "FirstVisibleEntry", 1.0f);
                selected = (uint32_t)NumberOr(scene, widgetIndex, "SelectedEntry", 0.0f);
                if (first < 1)
                {
                    first = 1;
                }
                AlignOf(scene, widgetIndex, "Alignment", &horizontal, &vertical);
                EntryRows(scene, widgetIndex, "LabelStringArray", "EnableArray", first - 1, count, selected, x, y, width, height, horizontal, alpha);
                fraction = 1.0f;
                start    = 0.0f;
                if (0 < count)
                {
                    fraction = (0.3f);
                    start    = ((float32_t)(first - 1) / (float32_t)count);
                }
                Scrollbar(scene, widgetIndex, "VerticalScroll", x, y, width, height, fraction, start, alpha);
                break;
            case KIND_TREE:
                DrawTree(scene, widgetIndex, layout, alpha, &bounds);
                break;
            case KIND_POP_UP_MENU:
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                Frame(scene, x, y, width, height, COLOR_POPUP, COLOR_BORDER, alpha, &bounds);
                EntryRows(scene, widgetIndex, "StringArray", "EnableArray", 0, (uint32_t)NumberOr(scene, widgetIndex, "NumberOfEntries", 0.0f), 0, x, y, width, height, FACEOE_GFX_ALIGN_START, alpha);
                break;
            case KIND_POP_UP_MENU_BUTTON:
            case KIND_POP_UP_PANEL_BUTTON:
                Button(scene, widgetIndex, layout, Value(scene, widgetIndex, "LabelString", 0), "PictureReference", "PicturePosition", alpha, &bounds, &x, &y, &width, &height);
                if (scene->options->showsHidden)
                {
                    // Outline the closed pop-up part where PopupPos places it
                    area_x      = NumberOr(scene, widgetIndex, "PopupPosX", x);
                    area_y      = NumberOr(scene, widgetIndex, "PopupPosY", y - height);
                    area_width  = NumberOr(scene, widgetIndex, "PopupSizeX", width);
                    area_height = NumberOr(scene, widgetIndex, "PopupSizeY", height);
                    Dashed(scene, area_x, area_y, area_width, area_height, COLOR_BORDER, alpha * 0.5f, &bounds);
                    if (KIND_POP_UP_MENU_BUTTON == kind)
                    {
                        EntryRows(scene, widgetIndex, "StringArray", "EnableArray", 0, (uint32_t)NumberOr(scene, widgetIndex, "NumberOfEntries", 0.0f), 0, area_x, area_y, area_width, area_height, FACEOE_GFX_ALIGN_START, alpha * 0.5f);
                    }
                }
                break;
            case KIND_POP_UP_PANEL:
                // Keep the panel closed until its button or the UA opens it
                Rect(scene, widgetIndex, layout, &x, &y, &width, &height);
                if (scene->options->showsHidden)
                {
                    Dashed(scene, x, y, width, height, COLOR_BORDER, alpha * 0.6f, &bounds);
                    alpha *= 0.6f;
                }
                else
                {
                    is_drawn = FALSE;
                }
                FACEOE_Gfx_DrawList_Translate(scene->drawList, x, y);
                break;
            case KIND_SLIDER:
                DrawSlider(scene, widgetIndex, layout, alpha, &bounds);
                break;
            case KIND_GP_ARC_CIRCLE:
            case KIND_GP_ARC_ELLIPSE:
            case KIND_GP_CROWN:
            case KIND_GP_LINE:
            case KIND_GP_LINE_POLAR:
            case KIND_GP_POLYLINE:
            case KIND_GP_RECTANGLE:
            case KIND_GP_TRIANGLE:
            case KIND_GP_TRIANGLE_FAN:
            case KIND_GP_TRIANGLE_STRIP:
                DrawGp(scene, widgetIndex, kind, alpha, &bounds);
                break;
            case KIND_BLINKING_CONTAINER:
            case KIND_FRAMING_REFRESH_CONTAINER:
            case KIND_WATCHDOG_CONTAINER:
            case KIND_RADIO_BOX:
            case KIND_MAPHORZ_CONTAINER:
            case KIND_MAPHORZ_PANEL:
            case KIND_MAPVERT_CONTAINER:
            case KIND_MAPVERT_PANEL:
            case KIND_ANIMATION_GROUP:
                // Keep the parent's origin for containers without a reference point
                Chip(scene, widgetIndex, alpha, &bounds);
                break;
            case KIND_ANIMATION_ROTATION:
                Marker(scene, NumberOr(scene, widgetIndex, "CenterX", 0.0f), NumberOr(scene, widgetIndex, "CenterY", 0.0f), (NumberOr(scene, widgetIndex, "ToAngle", 0.0f) * SCENE_DEGREES), COLOR_MARKER, alpha * 0.6f, &bounds);
                Chip(scene, widgetIndex, alpha, &bounds);
                break;
            default:
                // Draw connectors, data scaling, animations, focus, event, buffer, receiver, monitor, proxy, map sources and item lists as chips
                Chip(scene, widgetIndex, alpha, &bounds);
                break;
        }
        if (is_drawn)
        {
            DrawChildren(scene, widgetIndex, alpha, &children);
        }
        if (has_clip)
        {
            FACEOE_Gfx_DrawList_PopClip(scene->drawList);
        }
        FACEOE_Gfx_DrawList_PopTransform(scene->drawList);
        Record(scene, widgetIndex, &bounds, alpha);
        (void)value;
        (void)index;
    }
}

/*************************************************************************************************/
/**
 * \brief Records a container's children in array order under its policy.
 */
static void DrawChildren(Scene_t* scene, uint32_t widgetIndex, float32_t alpha, Children_t* children)
{
    const Arinc661_Widget_t* child;
    Layout_t                 layout;
    float32_t                child_alpha;
    float32_t                size;
    uint32_t                 child_index;
    bool_t                   is_shown;

    for (child_index = 0; child_index < scene->definition->widgetCount; ++child_index)
    {
        child = &scene->definition->widgets[child_index];
        if ((widgetIndex != child->parentWidgetIndex) || child->isExtension || (scene->layerIndex != child->layerIndex))
        {
            continue;
        }
        (void)memset(&layout, 0, sizeof(layout));
        child_alpha = alpha;
        is_shown    = TRUE;
        switch (children->mode)
        {
            case CHILD_IDENT:
            case CHILD_TABS:
                if (Ident(scene, child_index) != children->ident)
                {
                    if (scene->options->showsHidden)
                    {
                        child_alpha *= SCENE_HIDDEN_ALPHA;
                    }
                    else
                    {
                        is_shown = FALSE;
                    }
                }
                if (CHILD_TABS == children->mode)
                {
                    layout = children->content;
                }
                break;
            case CHILD_MENU_BAR:
                // Lay buttons one after another along the bar and fix their cross position and size from the bar
                layout.isSet = TRUE;
                if (children->isHorizontal)
                {
                    size          = NumberOr(scene, child_index, "SizeX", children->buttonSize);
                    layout.x      = children->offset;
                    layout.y      = children->buttonPosition;
                    layout.width  = size;
                    layout.height = children->buttonSize;
                }
                else
                {
                    size          = NumberOr(scene, child_index, "SizeY", children->buttonSize);
                    layout.x      = children->buttonPosition;
                    layout.y      = (-children->offset - size);
                    layout.width  = children->buttonSize;
                    layout.height = size;
                }
                children->offset += size;
                break;
            default:
                break;
        }
        if (is_shown)
        {
            DrawWidget(scene, child_index, child_alpha, &layout);
        }
    }
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Indexes the properties by widget once, fills the layer area when asked, then records every
 *  direct child of the layer and, through it, the whole widget tree.
 */
void Arinc661_Scene_DrawLayer(const Arinc661_Definition_t* definition, uint32_t layerIndex, FACEOE_Gfx_DrawList_t* drawList, Arinc661_Scene_Options_t* options)
{
    Scene_t  scene;
    uint32_t widget_index;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != options);
    LOGASSERT(layerIndex < definition->layerCount);
    LOGASSERT(definition->widgetCount <= SCENE_MAX_WIDGETS);
    LOGASSERT(definition->propertyCount <= SCENE_MAX_PROPERTIES);
    (void)memset(&scene, 0, sizeof(scene));
    scene.definition  = definition;
    scene.drawList    = drawList;
    scene.options     = options;
    scene.layerIndex  = layerIndex;
    options->boundsCount = 0;
    scene.base           = (*FACEOE_Gfx_DrawList_Transform(drawList));
    IndexProperties(&scene);
    Arinc661_Scene_LayerSize(definition, layerIndex, &scene.layerWidth, &scene.layerHeight);
    if (options->fillsLayerArea)
    {
        FACEOE_Gfx_DrawList_FillRectangle(drawList, 0.0f, 0.0f, scene.layerWidth, scene.layerHeight, COLOR_LAYER);
        FACEOE_Gfx_DrawList_StrokeRectangle(drawList, 0.0f, 0.0f, scene.layerWidth, scene.layerHeight, COLOR_LAYER_BORDER, SCENE_THIN);
    }
    for (widget_index = 0; widget_index < definition->widgetCount; ++widget_index)
    {
        if ((layerIndex == definition->widgets[widget_index].layerIndex) && (UINT32_MAX == definition->widgets[widget_index].parentWidgetIndex) && (FALSE == definition->widgets[widget_index].isExtension))
        {
            DrawWidget(&scene, widget_index, 1.0f, NULL);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Ties go to the later entry, which was drawn on top.
 */
uint32_t Arinc661_Scene_Pick(const Arinc661_Scene_Bounds_t* bounds, uint32_t count, FACEOE_Gfx_Point_t point)
{
    float32_t area;
    float32_t best_area;
    uint32_t  index;
    uint32_t  best;

    LOGASSERT((NULL != bounds) || (0 == count));
    best      = ARINC661_SCENE_NO_WIDGET;
    best_area = 0.0f;
    for (index = 0; index < count; ++index)
    {
        if ((bounds[index].minimum.x <= point.x) && (point.x <= bounds[index].maximum.x) && (bounds[index].minimum.y <= point.y) && (point.y <= bounds[index].maximum.y))
        {
            area = ((bounds[index].maximum.x - bounds[index].minimum.x) * (bounds[index].maximum.y - bounds[index].minimum.y));
            if ((ARINC661_SCENE_NO_WIDGET == best) || (area <= best_area))
            {
                best      = bounds[index].widgetIndex;
                best_area = area;
            }
        }
    }

    return best;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts the digit slots on each side of the point, rounds the magnitude to the decimal
 *  slots, and walks the format emitting digits, the sign, and mask characters. Integer digits
 *  beyond the slots are emitted in the first slot so no magnitude is lost. A negative value
 *  without a `+` slot gets a leading minus.
 */
void Arinc661_Scene_FormatNumber(const char_t* format, float64_t value, char_t* text, uint32_t capacity)
{
    char_t    integer_digits[64];
    char_t    decimal_digits[64];
    float64_t magnitude;
    float64_t rounded;
    uint64_t  integer_part;
    uint64_t  decimal_part;
    float64_t scale;
    uint32_t  integer_slots;
    uint32_t  decimal_slots;
    uint32_t  integer_length;
    uint32_t  decimal_length;
    uint32_t  out;
    uint32_t  index;
    uint32_t  digit_index;
    uint32_t  consumed_integer;
    uint32_t  consumed_decimal;
    uint32_t  last_significant_decimal;
    bool_t    has_point;
    bool_t    has_sign_slot;
    bool_t    is_negative;
    bool_t    has_nonzero_integer;
    bool_t    is_past_point;
    char_t    digit;
    char_t    symbol;

    LOGASSERT(NULL != format);
    LOGASSERT(NULL != text);
    LOGASSERT(0 < capacity);
    integer_slots = 0;
    decimal_slots = 0;
    has_point     = FALSE;
    has_sign_slot = FALSE;
    for (index = 0; '\0' != format[index]; ++index)
    {
        if ('.' == format[index])
        {
            has_point = TRUE;
        }
        else if (('#' == format[index]) || ('_' == format[index]) || ('~' == format[index]))
        {
            if (has_point)
            {
                decimal_slots += 1;
            }
            else
            {
                integer_slots += 1;
            }
        }
        else if ('+' == format[index])
        {
            has_sign_slot = TRUE;
        }
    }
    if (20 < decimal_slots)
    {
        decimal_slots = 20;
    }
    is_negative = (value < 0.0);
    magnitude   = fabs(value);
    scale       = 1.0;
    for (index = 0; index < decimal_slots; ++index)
    {
        scale *= 10.0;
    }
    rounded      = floor((magnitude * scale) + 0.5);
    integer_part = (uint64_t)(rounded / scale);
    decimal_part = (uint64_t)(rounded - ((float64_t)integer_part * scale));
    // Zero-pad the integer digits to the slots and keep extra digits in front
    integer_length = (uint32_t)snprintf(integer_digits, sizeof(integer_digits), "%llu", (unsigned long long)integer_part);
    if ((0 == integer_slots) && (0 == integer_part))
    {
        integer_length = 0;
    }
    while (integer_length < integer_slots)
    {
        (void)memmove(&integer_digits[1], &integer_digits[0], integer_length + 1);
        integer_digits[0] = '0';
        integer_length += 1;
    }
    decimal_length = 0;
    for (index = 0; index < decimal_slots; ++index)
    {
        decimal_digits[decimal_slots - 1 - index] = (char_t)('0' + (decimal_part % 10));
        decimal_part /= 10;
        decimal_length += 1;
    }
    decimal_digits[decimal_length] = '\0';
    last_significant_decimal       = 0;
    for (index = 0; index < decimal_length; ++index)
    {
        if ('0' != decimal_digits[index])
        {
            last_significant_decimal = (index + 1);
        }
    }
    out = 0;
    if (is_negative && (FALSE == has_sign_slot) && (out < (capacity - 1)))
    {
        text[out] = '-';
        out += 1;
    }
    consumed_integer    = 0;
    consumed_decimal    = 0;
    has_nonzero_integer = FALSE;
    is_past_point       = FALSE;
    for (index = 0; ('\0' != format[index]) && (out < (capacity - 1)); ++index)
    {
        symbol = format[index];
        if ('.' == symbol)
        {
            // Emit integer digits that found no slot before the point
            for (digit_index = 0; (0 == integer_slots) && (digit_index < integer_length) && (out < (capacity - 1)); ++digit_index)
            {
                text[out] = integer_digits[digit_index];
                out += 1;
            }
            is_past_point = TRUE;
            text[out]     = '.';
            out += 1;
        }
        else if ('+' == symbol)
        {
            if (is_negative)
            {
                text[out] = '-';
            }
            else
            {
                text[out] = '+';
            }
            out += 1;
        }
        else if (('#' == symbol) || ('_' == symbol) || ('~' == symbol))
        {
            if (is_past_point)
            {
                digit_index = consumed_decimal;
                consumed_decimal += 1;
                digit = '0';
                if (digit_index < decimal_length)
                {
                    digit = decimal_digits[digit_index];
                }
                // Treat trailing zeros of the decimal part as insignificant
                if (('#' == symbol) || (digit_index < last_significant_decimal))
                {
                    text[out] = digit;
                    out += 1;
                }
                else if ('~' == symbol)
                {
                    text[out] = ' ';
                    out += 1;
                }
            }
            else
            {
                // Emit every digit beyond the slot count in the first slot
                digit_index = consumed_integer;
                consumed_integer += 1;
                if ((0 == digit_index) && (integer_slots < integer_length))
                {
                    while ((digit_index < (integer_length - integer_slots)) && (out < (capacity - 1)))
                    {
                        text[out] = integer_digits[digit_index];
                        has_nonzero_integer = TRUE;
                        out += 1;
                        digit_index += 1;
                    }
                }
                digit_index = (integer_length - integer_slots) + (consumed_integer - 1);
                digit       = '0';
                if (digit_index < integer_length)
                {
                    digit = integer_digits[digit_index];
                }
                if ('0' != digit)
                {
                    has_nonzero_integer = TRUE;
                }
                // Treat leading zeros of the integer part as insignificant
                if (('#' == symbol) || has_nonzero_integer)
                {
                    text[out] = digit;
                    out += 1;
                }
                else if ('~' == symbol)
                {
                    text[out] = ' ';
                    out += 1;
                }
            }
        }
        else
        {
            text[out] = symbol;
            out += 1;
        }
    }
    if ((0 == integer_slots) && (FALSE == has_point) && (0 < integer_length) && (out < (capacity - 1)))
    {
        // Emit the integer digits a slot-less format left out
        for (digit_index = 0; (digit_index < integer_length) && (out < (capacity - 1)); ++digit_index)
        {
            text[out] = integer_digits[digit_index];
            out += 1;
        }
    }
    text[out] = '\0';
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Missing or non-positive values fall back to 100 mm.
 */
void Arinc661_Scene_LayerSize(const Arinc661_Definition_t* definition, uint32_t layerIndex, float32_t* width, float32_t* height)
{
    uint32_t  property_index;
    float32_t number;

    LOGASSERT(NULL != definition);
    LOGASSERT(NULL != width);
    LOGASSERT(NULL != height);
    (*width)  = SCENE_DEFAULT_EXTENT;
    (*height) = SCENE_DEFAULT_EXTENT;
    for (property_index = 0; property_index < definition->propertyCount; ++property_index)
    {
        if ((ARINC661_PROPERTY_LAYER == definition->properties[property_index].owner) && (layerIndex == definition->properties[property_index].ownerIndex))
        {
            if ((0 == strcmp("Width", definition->properties[property_index].name)) && ScanNumber(definition->properties[property_index].value, &number) && (1.0f <= number))
            {
                (*width) = number;
            }
            else if ((0 == strcmp("Height", definition->properties[property_index].name)) && ScanNumber(definition->properties[property_index].value, &number) && (1.0f <= number))
            {
                (*height) = number;
            }
        }
    }
}
