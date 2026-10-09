/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, sections 2.3.4, 3.1.3, 3.2.3, 3.2.6, and the widget definitions of
 * sections 3.3 to 3.10; see arinc661_renderer.h.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_renderer.h"
#include "arinc661/arinc661_definition.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_print.h"

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

#define RENDERER_MAX_ANIMATIONS  (256)
#define RENDERER_MAX_POINTS      (256)
#define RENDERER_MAX_TICKS       (1000)
#define RENDERER_TEXT_BYTES      (ARINC661_MAX_PROPERTY_BYTES + 64)
#define RENDERER_DEFAULT_EXTENT  (10000.0f)
#define RENDERER_DEGREES         (0.01745329252f)
#define RENDERER_FULL_TURN       (6.28318530718f)
#define RENDERER_HIDDEN_ALPHA    (0.3f)
#define RENDERER_DISABLED_ALPHA  (0.55f)
#define RENDERER_DASH_LENGTH     (150.0f)
#define RENDERER_MARKER_RADIUS   (120.0f)
#define RENDERER_THIN            (1.0f)
#define RENDERER_THICK           (2.0f)
#define RENDERER_CHIP_HEIGHT     (260.0f)
#define RENDERER_CHIP_WIDTH      (3200.0f)
#define RENDERER_PAD             (40.0f)
#define RENDERER_TEXT_FIT        (0.8f)
#define RENDERER_MIN_FONT        (4.0f)
#define RENDERER_NAME_FONT_SCALE (0.85f)
#define RENDERER_BOUNDARY_STEP   (5.0f)
#define RENDERER_PALETTE_COUNT   (16)

#define COLOR_LAYER              FACEOE_GFX_RGBA(42, 50, 64, 255)
#define COLOR_LAYER_BORDER       FACEOE_GFX_RGBA(220, 230, 245, 255)
#define COLOR_CONTAINER          FACEOE_GFX_RGBA(42, 154, 188, 50)
#define COLOR_BORDER             FACEOE_GFX_RGBA(97, 221, 245, 255)
#define COLOR_BUTTON             FACEOE_GFX_RGBA(70, 110, 150, 200)
#define COLOR_BUTTON_BORDER      FACEOE_GFX_RGBA(170, 205, 240, 255)
#define COLOR_EDIT               FACEOE_GFX_RGBA(20, 24, 30, 230)
#define COLOR_EDIT_BORDER        FACEOE_GFX_RGBA(150, 170, 190, 255)
#define COLOR_TEXT               FACEOE_GFX_RGBA(240, 248, 255, 255)
#define COLOR_NAME               FACEOE_GFX_RGBA(255, 220, 120, 255)
#define COLOR_AREA               FACEOE_GFX_RGBA(255, 180, 60, 255)
#define COLOR_MARKER             FACEOE_GFX_RGBA(255, 120, 120, 255)
#define COLOR_SELECTION          FACEOE_GFX_RGBA(255, 255, 255, 60)
#define COLOR_ROW_SELECTED       FACEOE_GFX_RGBA(97, 160, 245, 120)
#define COLOR_CHIP               FACEOE_GFX_RGBA(90, 70, 120, 220)
#define COLOR_CHIP_BORDER        FACEOE_GFX_RGBA(190, 160, 240, 255)
#define COLOR_PICTURE            FACEOE_GFX_RGBA(120, 150, 90, 120)
#define COLOR_TRACK              FACEOE_GFX_RGBA(160, 170, 180, 255)
#define COLOR_SCROLLBAR          FACEOE_GFX_RGBA(120, 130, 140, 160)
#define COLOR_POPUP              FACEOE_GFX_RGBA(60, 60, 80, 160)
#define COLOR_GRID               FACEOE_GFX_RGBA(120, 200, 120, 160)
#define COLOR_RANGE              FACEOE_GFX_RGBA(120, 220, 160, 200)

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
    CHILD_IDENT,    ///< Only the child whose WidgetIdent matches; the rest hidden, or faded when showsHidden is set
    CHILD_MENU_BAR, ///< Children laid along the bar
    CHILD_TABS      ///< Tabbed panels share the content area; the inactive ones are hidden, or faded when showsHidden is set
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

/** How DrawChildren presents a container's children; set while the container is drawn. */
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
    bool_t             isChip;
    FACEOE_Gfx_Point_t minimum;
    FACEOE_Gfx_Point_t maximum;
} Bounds_t;

/** Drawing state for one layer. */
typedef struct Renderer_s
{
    FACEOE_Gfx_DrawList_t*       drawList;
    Arinc661_Renderer_Options_t* options;
    float32_t                    layerWidth;
    float32_t                    layerHeight;
    uint32_t                     chipCount;
    uint32_t                     animationCount;
    const FACEOE_Utl_Node_t*     animations[RENDERER_MAX_ANIMATIONS];
    FACEOE_Gfx_Transform_t       base;
} Renderer_t;

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

/** Colors for ColorIndex values; the CDS color table is not part of the Definition File. */
static const FACEOE_Gfx_Color_t c_palette[RENDERER_PALETTE_COUNT] =
{
    FACEOE_GFX_RGBA(255, 255, 255, 255), FACEOE_GFX_RGBA(0, 0, 0, 255),       FACEOE_GFX_RGBA(255, 60, 60, 255),   FACEOE_GFX_RGBA(60, 255, 60, 255),
    FACEOE_GFX_RGBA(80, 120, 255, 255),  FACEOE_GFX_RGBA(255, 255, 60, 255),  FACEOE_GFX_RGBA(60, 255, 255, 255),  FACEOE_GFX_RGBA(255, 60, 255, 255),
    FACEOE_GFX_RGBA(255, 160, 40, 255),  FACEOE_GFX_RGBA(160, 160, 160, 255), FACEOE_GFX_RGBA(120, 80, 40, 255),   FACEOE_GFX_RGBA(40, 120, 80, 255),
    FACEOE_GFX_RGBA(200, 200, 255, 255), FACEOE_GFX_RGBA(255, 200, 200, 255), FACEOE_GFX_RGBA(200, 255, 200, 255), FACEOE_GFX_RGBA(90, 90, 90, 255)
};


/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Returns the widget a widget node stands for.
 *
 * \param[in] node   Widget node in the scene
 *
 * \retval    const Arinc661_Widget_t*   The widget.
 */
static const Arinc661_Widget_t* WidgetOf(const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Returns the first property node of a layer or widget node.
 *
 * \param[in] node   Layer or widget node in the scene
 *
 * \retval    const FACEOE_Utl_Node_t*   First property node, or NULL.
 */
static const FACEOE_Utl_Node_t* GetFirstProperty(const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Returns a node's first child after its model node: a layer below the root, or a widget
 *  or extension below a layer or widget.
 *
 * \param[in] node   Root, layer, or widget node in the scene
 *
 * \retval    const FACEOE_Utl_Node_t*   First child after the model node, or NULL.
 */
static const FACEOE_Utl_Node_t* GetFirstChild(const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Lists the rotation, scale, and translation animations below a node, in drawing order.
 *
 * \param[in,out] renderer   Drawing state
 * \param[in]     node       Layer or widget node in the scene
 *
 * \retval        None
 */
static void CollectAnimations(Renderer_t* renderer, const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Maps a widget type name to its kind.
 *
 * \param[in] type   Widget or extension type name
 *
 * \retval    Kind_t   Kind, or KIND_UNKNOWN.
 */
static Kind_t KindOf(const char_t* type);

/*************************************************************************************************/
/**
 * \brief Returns the n-th same-named property value of a widget, or NULL.
 *
 * \param[in] node         Widget node in the scene
 * \param[in] name         Property name
 * \param[in] occurrence   Index among same-named properties
 *
 * \retval    const char_t*   Value, or NULL.
 */
static const char_t* Value(const FACEOE_Utl_Node_t* node, const char_t* name, uint32_t occurrence);

/*************************************************************************************************/
/**
 * \brief Counts the same-named properties of a widget.
 *
 * \param[in] node   Widget node in the scene
 * \param[in] name   Property name
 *
 * \retval    uint32_t   Number of same-named properties.
 */
static uint32_t Occurrences(const FACEOE_Utl_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Returns a widget's first same-named property value, or an empty string.
 *
 * \param[in] node   Widget node in the scene
 * \param[in] name   Property name
 *
 * \retval    const char_t*   Value, or an empty string.
 */
static const char_t* Text(const FACEOE_Utl_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a decimal or a constant name as a number.
 *
 * \param[in]  text     Text
 * \param[out] number   Receives the number
 *
 * \retval     TRUE    if the text is a decimal or a known constant.
 * \retval     FALSE   otherwise.
 */
static bool_t ScanNumber(const char_t* text, float32_t* number);

/*************************************************************************************************/
/**
 * \brief Reads a widget's first same-named property as a number.
 *
 * \param[in]  node     Widget node in the scene
 * \param[in]  name     Property name
 * \param[out] number   Receives the number
 *
 * \retval     TRUE    if the property exists and is numeric.
 * \retval     FALSE   otherwise.
 */
static bool_t Number(const FACEOE_Utl_Node_t* node, const char_t* name, float32_t* number);

/*************************************************************************************************/
/**
 * \brief Reads a number with a fallback.
 *
 * \param[in] node       Widget node in the scene
 * \param[in] name       Property name
 * \param[in] fallback   Fallback
 *
 * \retval    float32_t   Value, or the fallback.
 */
static float32_t NumberOr(const FACEOE_Utl_Node_t* node, const char_t* name, float32_t fallback);

/*************************************************************************************************/
/**
 * \brief Reads the n-th same-named property as a number with a fallback.
 *
 * \param[in] node         Widget node in the scene
 * \param[in] name         Property name
 * \param[in] occurrence   Index among same-named properties
 * \param[in] fallback     Fallback
 *
 * \retval    float32_t   Value, or the fallback.
 */
static float32_t NumberAt(const FACEOE_Utl_Node_t* node, const char_t* name, uint32_t occurrence, float32_t fallback);

/*************************************************************************************************/
/**
 * \brief Tests the first same-named property against a constant.
 *
 * \param[in] node           Widget node in the scene
 * \param[in] name           Property name
 * \param[in] constantName   Constant name
 *
 * \retval    TRUE    if the property equals the constant.
 * \retval    FALSE   otherwise.
 */
static bool_t Is(const FACEOE_Utl_Node_t* node, const char_t* name, const char_t* constantName);

/*************************************************************************************************/
/**
 * \brief Tests whether a property equals a constant, by name or by value.
 *
 * \param[in] node           Widget node in the scene
 * \param[in] name           Property name
 * \param[in] occurrence     Index among same-named properties
 * \param[in] constantName   Constant name
 *
 * \retval    TRUE    if the property equals the constant.
 * \retval    FALSE   otherwise.
 */
static bool_t IsAt(const FACEOE_Utl_Node_t* node, const char_t* name, uint32_t occurrence, const char_t* constantName);

/*************************************************************************************************/
/**
 * \brief Reads a widget's WidgetIdent, or -1.
 *
 * \param[in] node   Widget node in the scene
 *
 * \retval    int64_t   WidgetIdent, or -1 when missing.
 */
static int64_t Ident(const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Applies the inherited alpha to a color.
 *
 * \param[in] color   Color
 * \param[in] alpha   Alpha
 *
 * \retval    FACEOE_Gfx_Color_t   Color with its alpha scaled.
 */
static FACEOE_Gfx_Color_t Fade(FACEOE_Gfx_Color_t color, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Resolves a widget's ColorIndex through the palette.
 *
 * \param[in] node    Widget node in the scene
 * \param[in] alpha   Alpha
 *
 * \retval    FACEOE_Gfx_Color_t   Palette color for the ColorIndex, faded.
 */
static FACEOE_Gfx_Color_t IndexColor(const FACEOE_Utl_Node_t* node, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Reads a widget's reference point and size, or takes them from the container's layout.
 *
 * \param[in]  node     Widget node in the scene
 * \param[in]  layout   Container's layout, or NULL
 * \param[out] x        Receives X in layer units
 * \param[out] y        Receives Y in layer units
 * \param[out] width    Receives the width
 * \param[out] height   Receives the height
 *
 * \retval     None
 */
static void Rect(const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t* x, float32_t* y, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Grows output bounds by a mapped local point.
 *
 * \param[in]     renderer   Drawing state
 * \param[in,out] bounds     Bounds
 * \param[in]     x          X in layer units
 * \param[in]     y          Y in layer units
 *
 * \retval        None
 */
static void Extend(const Renderer_t* renderer, Bounds_t* bounds, float32_t x, float32_t y);

/*************************************************************************************************/
/**
 * \brief Grows output bounds by a local box.
 *
 * \param[in]     renderer   Drawing state
 * \param[in,out] bounds     Bounds
 * \param[in]     x          X in layer units
 * \param[in]     y          Y in layer units
 * \param[in]     width      Width
 * \param[in]     height     Height
 *
 * \retval        None
 */
static void ExtendBox(const Renderer_t* renderer, Bounds_t* bounds, float32_t x, float32_t y, float32_t width, float32_t height);

/*************************************************************************************************/
/**
 * \brief Picks a font size that fits a local height, in output units.
 *
 * \param[in] renderer   Drawing state
 * \param[in] height     Height
 *
 * \retval    float32_t   Font size in output units.
 */
static float32_t FontFor(const Renderer_t* renderer, float32_t height);

/*************************************************************************************************/
/**
 * \brief Converts an Alignment constant to box alignments.
 *
 * \param[in]  node         Widget node in the scene
 * \param[in]  name         Name of the property holding the constant
 * \param[out] horizontal   Receives the horizontal alignment
 * \param[out] vertical     Receives the vertical alignment
 *
 * \retval     None
 */
static void AlignOf(const FACEOE_Utl_Node_t* node, const char_t* name, FACEOE_Gfx_Align_t* horizontal, FACEOE_Gfx_Align_t* vertical);

/*************************************************************************************************/
/**
 * \brief Records text in a local box with a font that fits the box.
 *
 * \param[in] renderer     Drawing state
 * \param[in] x            X in layer units
 * \param[in] y            Y in layer units
 * \param[in] width        Width
 * \param[in] height       Height
 * \param[in] text         Text; nothing is recorded when empty
 * \param[in] horizontal   Horizontal alignment
 * \param[in] vertical     Vertical alignment
 * \param[in] radians      Rotation in radians
 * \param[in] color        Color
 *
 * \retval    None
 */
static void BoxText(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* text, FACEOE_Gfx_Align_t horizontal, FACEOE_Gfx_Align_t vertical, float32_t radians, FACEOE_Gfx_Color_t color);

/*************************************************************************************************/
/**
 * \brief Records a filled, outlined local box and grows the bounds.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     x          X in layer units
 * \param[in]     y          Y in layer units
 * \param[in]     width      Width
 * \param[in]     height     Height
 * \param[in]     fill       Fill color; not filled when its alpha is zero
 * \param[in]     border     Border color
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void Frame(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t fill, FACEOE_Gfx_Color_t border, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a dashed local box and grows the bounds.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     x          X in layer units
 * \param[in]     y          Y in layer units
 * \param[in]     width      Width
 * \param[in]     height     Height
 * \param[in]     color      Color
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void Dashed(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a reference-point marker: a circle, a cross, and a heading tick.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     x          X in layer units
 * \param[in]     y          Y in layer units
 * \param[in]     radians    Heading tick direction in radians
 * \param[in]     color      Color
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void Marker(const Renderer_t* renderer, float32_t x, float32_t y, float32_t radians, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a picture or source placeholder: a box with diagonals and a caption.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     x          X in layer units
 * \param[in]     y          Y in layer units
 * \param[in]     width      Width
 * \param[in]     height     Height
 * \param[in]     caption    Caption
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void Placeholder(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* caption, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a scrollbar on the side a property names: A661_LEFT, A661_RIGHT, A661_TOP, or
 *  A661_BOTTOM; any other value records nothing.
 *
 * \param[in] renderer   Drawing state
 * \param[in] node       Widget node in the scene
 * \param[in] name       Name of the property naming the side
 * \param[in] x          X in layer units
 * \param[in] y          Y in layer units
 * \param[in] width      Width
 * \param[in] height     Height
 * \param[in] fraction   Visible fraction of the content, clamped to 0.05 to 1
 * \param[in] start      Fraction of the content before the visible part
 * \param[in] alpha      Alpha
 *
 * \retval    None
 */
static void Scrollbar(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const char_t* name, float32_t x, float32_t y, float32_t width, float32_t height, float32_t fraction, float32_t start, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Records a chip for a widget without geometry along the layer's left edge, when
 *  showsNonGraphical is set.
 *
 * \param[in,out] renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void Chip(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Counts the extensions attached to a widget.
 *
 * \param[in] node   Widget node in the scene
 *
 * \retval    uint32_t   Number of extensions attached to the widget.
 */
static uint32_t ExtensionCount(const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Reports a widget's bounds and, when names are shown, labels it with its name and
 *  extension count.
 *
 * \param[in,out] renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     bounds     Bounds; nothing is reported when empty
 * \param[in]     alpha      Alpha
 *
 * \retval        None
 */
static void Record(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Bounds_t* bounds, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Records a button face with an optional picture or symbol box beside its label.
 *
 * \param[in]     renderer       Drawing state
 * \param[in]     node           Widget node in the scene
 * \param[in]     layout         Container's layout, or NULL
 * \param[in]     label          Label text, or NULL
 * \param[in]     iconName       Name of the property naming the picture or symbol, or NULL
 * \param[in]     iconPosition   Name of the property naming the icon's side
 * \param[in]     alpha          Alpha
 * \param[in,out] bounds         Bounds
 * \param[out]    x              Receives the face's X in layer units
 * \param[out]    y              Receives the face's Y in layer units
 * \param[out]    width          Receives the face's width
 * \param[out]    height         Receives the face's height
 *
 * \retval        None
 */
static void Button(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, const char_t* label, const char_t* iconName, const char_t* iconPosition, float32_t alpha, Bounds_t* bounds, float32_t* x, float32_t* y, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Records the rows of a list from a string array, highlighting the selected entry.
 *
 * \param[in] renderer     Drawing state
 * \param[in] node         Widget node in the scene
 * \param[in] arrayName    Name of the string array property
 * \param[in] enableName   Name of the per-entry enable array property, or NULL
 * \param[in] first        Zero-based first entry to record
 * \param[in] count        Number of entries in the list
 * \param[in] selected     One-based selected entry, or zero for none
 * \param[in] x            X in layer units
 * \param[in] y            Y in layer units
 * \param[in] width        Width
 * \param[in] height       Height
 * \param[in] align        Horizontal alignment
 * \param[in] alpha        Alpha
 *
 * \retval    None
 */
static void EntryRows(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const char_t* arrayName, const char_t* enableName, uint32_t first, uint32_t count, uint32_t selected, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Align_t align, float32_t alpha);

/*************************************************************************************************/
/**
 * \brief Records a formatted numeric field with its legend: EditBoxNumeric, EditBoxNumericBCD, and NumericReadout.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     layout     Container's layout, or NULL
 * \param[in]     radians    Rotation about the reference point in radians
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void NumericBox(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t radians, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a graphics primitive: GpLine, GpLinePolar, GpArcCircle, GpArcEllipse, GpCrown, GpRectangle, GpTriangle, GpPolyline, GpTriangleFan, GpTriangleStrip.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     kind       Primitive kind
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void DrawGp(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, Kind_t kind, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a slider: track, major and minor ticks, labels, and the thumb at Value.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     layout     Container's layout, or NULL
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void DrawSlider(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a tree: entries indented by their ParentArray depth, collapsed under closed parents.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     layout     Container's layout, or NULL
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void DrawTree(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a MapBoundary: sections of arc, vertical, or horizontal lines around the origin.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void DrawMapBoundary(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a MapGrid: CountX by CountY cells of IncrementX by IncrementY, offset so the grid point (OffsetX, OffsetY), in cells, sits on the display reference point.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 *
 * \retval        None
 */
static void DrawMapGrid(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Bounds_t* bounds);

/*************************************************************************************************/
/**
 * \brief Records a TabbedPanelGroup: a tab strip on TabPosition with one tab per TabbedPanel child, and the content area the active child fills.
 *
 * \param[in]     renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     layout     Container's layout, or NULL
 * \param[in]     alpha      Alpha
 * \param[in,out] bounds     Bounds
 * \param[out]    children   Receives the tab policy and the content area
 *
 * \retval        None
 */
static void DrawTabbedGroup(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t alpha, Bounds_t* bounds, Children_t* children);

/*************************************************************************************************/
/**
 * \brief Applies the layer's transform animations that target a widget at the options' animation time.
 *
 * \param[in,out] renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 *
 * \retval        None
 */
static void ApplyAnimations(Renderer_t* renderer, const FACEOE_Utl_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Records one widget by its Creation Structure, then its children through the policy its kind imposes.
 *
 * \param[in,out] renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     alpha      Alpha inherited from the container
 * \param[in]     layout     Container's layout, or NULL
 *
 * \retval        None
 */
static void DrawWidget(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, const Layout_t* layout);

/*************************************************************************************************/
/**
 * \brief Records a container's children, extensions excluded, in scene order under its policy.
 *
 * \param[in,out] renderer   Drawing state
 * \param[in]     node       Widget node in the scene
 * \param[in]     alpha      Container's alpha
 * \param[in,out] children   Policy set by the container; a menu bar's offset advances per child
 *
 * \retval        None
 */
static void DrawChildren(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Children_t* children);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static const Arinc661_Widget_t* WidgetOf(const FACEOE_Utl_Node_t* node)
{
    return ((const Arinc661_Scene_Node_t*)node->item)->widget;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The properties are the children of the node's model node, its first child.
 */
static const FACEOE_Utl_Node_t* GetFirstProperty(const FACEOE_Utl_Node_t* node)
{
    return node->firstChild->firstChild;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static const FACEOE_Utl_Node_t* GetFirstChild(const FACEOE_Utl_Node_t* node)
{
    return node->firstChild->nextSibling;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Parents come before their children.
 */
static void CollectAnimations(Renderer_t* renderer, const FACEOE_Utl_Node_t* node)
{
    const FACEOE_Utl_Node_t* child;
    Kind_t                   kind;
    bool_t                   is_animation;
    bool_t                   has_room;

    for (child = GetFirstChild(node); NULL != child; child = child->nextSibling)
    {
        kind         = KindOf(WidgetOf(child)->type);
        is_animation = (KIND_ANIMATION_TRANSLATION == kind) || (KIND_ANIMATION_ROTATION == kind) || (KIND_ANIMATION_SCALE == kind);
        has_room     = (renderer->animationCount < COUNT_OF(renderer->animations));
        if (is_animation && has_room)
        {
            renderer->animations[renderer->animationCount] = child;
            renderer->animationCount += 1;
        }
        CollectAnimations(renderer, child);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static Kind_t KindOf(const char_t* type)
{
    uint32_t index;
    Kind_t   kind;

    kind = KIND_UNKNOWN;
    for (index = 0; index < COUNT_OF(c_kindNames); ++index)
    {
        if (0 == strncmp(type, c_kindNames[index].name, ARINC661_MAX_NAME_BYTES))
        {
            kind = c_kindNames[index].kind;
            break;
        }
    }

    return kind;
}


/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static const char_t* Value(const FACEOE_Utl_Node_t* node, const char_t* name, uint32_t occurrence)
{
    const FACEOE_Utl_Node_t*   entry;
    const Arinc661_Property_t* property;
    const char_t*              value;
    uint32_t                   seen;

    value = NULL;
    seen  = 0;
    for (entry = GetFirstProperty(node); NULL != entry; entry = entry->nextSibling)
    {
        property = ((const Arinc661_Scene_Node_t*)entry->item)->property;
        if (0 == strncmp(name, property->name, sizeof(property->name)))
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
 * \par Details:
 *  None
 */
static uint32_t Occurrences(const FACEOE_Utl_Node_t* node, const char_t* name)
{
    const FACEOE_Utl_Node_t*   entry;
    const Arinc661_Property_t* property;
    uint32_t                   seen;

    seen = 0;
    for (entry = GetFirstProperty(node); NULL != entry; entry = entry->nextSibling)
    {
        property = ((const Arinc661_Scene_Node_t*)entry->item)->property;
        if (0 == strncmp(name, property->name, sizeof(property->name)))
        {
            seen += 1;
        }
    }

    return seen;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static const char_t* Text(const FACEOE_Utl_Node_t* node, const char_t* name)
{
    const char_t* value;

    value = Value(node, name, 0);
    if (NULL == value)
    {
        value = "";
    }

    return value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
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
        for (constant_index = 0; constant_index < Arinc661_Definition_GetConstantCount(); ++constant_index)
        {
            if (0 == strncmp(text, Arinc661_Definition_GetConstantName(constant_index), ARINC661_MAX_NAME_BYTES))
            {
                (*number) = (float32_t)Arinc661_Definition_GetConstantValue(constant_index);
                is_number = TRUE;
                break;
            }
        }
    }

    return is_number;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t Number(const FACEOE_Utl_Node_t* node, const char_t* name, float32_t* number)
{
    const char_t* value;
    bool_t        is_number;

    is_number = FALSE;
    value     = Value(node, name, 0);
    if (NULL != value)
    {
        is_number = ScanNumber(value, number);
    }

    return is_number;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static float32_t NumberOr(const FACEOE_Utl_Node_t* node, const char_t* name, float32_t fallback)
{
    float32_t number;

    number = fallback;
    (void)Number(node, name, &number);

    return number;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static float32_t NumberAt(const FACEOE_Utl_Node_t* node, const char_t* name, uint32_t occurrence, float32_t fallback)
{
    const char_t* value;
    float32_t     number;

    number = fallback;
    value  = Value(node, name, occurrence);
    if (NULL != value)
    {
        (void)ScanNumber(value, &number);
    }

    return number;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t IsAt(const FACEOE_Utl_Node_t* node, const char_t* name, uint32_t occurrence, const char_t* constantName)
{
    const char_t* value;
    float32_t     number;
    float32_t     constant;
    bool_t        is_equal;

    is_equal = FALSE;
    value    = Value(node, name, occurrence);
    if (NULL != value)
    {
        if (0 == strncmp(value, constantName, ARINC661_MAX_NAME_BYTES))
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
 * \par Details:
 *  None
 */
static bool_t Is(const FACEOE_Utl_Node_t* node, const char_t* name, const char_t* constantName)
{
    return IsAt(node, name, 0, constantName);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static int64_t Ident(const FACEOE_Utl_Node_t* node)
{
    float32_t number;
    int64_t   ident;

    ident = -1;
    if (Number(node, "WidgetIdent", &number))
    {
        ident = (int64_t)number;
    }

    return ident;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static FACEOE_Gfx_Color_t Fade(FACEOE_Gfx_Color_t color, float32_t alpha)
{
    return FACEOE_Gfx_Color_Fade(color, alpha);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static FACEOE_Gfx_Color_t IndexColor(const FACEOE_Utl_Node_t* node, float32_t alpha)
{
    float32_t index;

    index = NumberOr(node, "ColorIndex", 0.0f);
    if (index < 0.0f)
    {
        index = 0.0f;
    }

    return Fade(c_palette[((uint32_t)index) % RENDERER_PALETTE_COUNT], alpha);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Rect(const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t* x, float32_t* y, float32_t* width, float32_t* height)
{
    (*x)      = NumberOr(node, "PosX", 0.0f);
    (*y)      = NumberOr(node, "PosY", 0.0f);
    (*width)  = NumberOr(node, "SizeX", 0.0f);
    (*height) = NumberOr(node, "SizeY", 0.0f);
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
 * \par Details:
 *  None
 */
static void Extend(const Renderer_t* renderer, Bounds_t* bounds, float32_t x, float32_t y)
{
    FACEOE_Gfx_Point_t point;

    point = FACEOE_Gfx_DrawList_Map(renderer->drawList, x, y);
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
 * \par Details:
 *  None
 */
static void ExtendBox(const Renderer_t* renderer, Bounds_t* bounds, float32_t x, float32_t y, float32_t width, float32_t height)
{
    Extend(renderer, bounds, x, y);
    Extend(renderer, bounds, x + width, y);
    Extend(renderer, bounds, x + width, y + height);
    Extend(renderer, bounds, x, y + height);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static float32_t FontFor(const Renderer_t* renderer, float32_t height)
{
    float32_t unit_x;
    float32_t unit_y;
    float32_t pixels;
    float32_t size;

    size = renderer->options->fontSize;
    if (0.0f < height)
    {
        FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
        pixels = (height * unit_y * RENDERER_TEXT_FIT);
        if (pixels < size)
        {
            size = pixels;
        }
    }
    if (size < RENDERER_MIN_FONT)
    {
        size = RENDERER_MIN_FONT;
    }

    return size;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void AlignOf(const FACEOE_Utl_Node_t* node, const char_t* name, FACEOE_Gfx_Align_t* horizontal, FACEOE_Gfx_Align_t* vertical)
{
    (*horizontal) = FACEOE_GFX_ALIGN_CENTER;
    (*vertical)   = FACEOE_GFX_ALIGN_CENTER;
    if (Is(node, name, "A661_LEFT") || Is(node, name, "A661_TOP_LEFT") || Is(node, name, "A661_BOTTOM_LEFT"))
    {
        (*horizontal) = FACEOE_GFX_ALIGN_START;
    }
    else if (Is(node, name, "A661_RIGHT") || Is(node, name, "A661_TOP_RIGHT") || Is(node, name, "A661_BOTTOM_RIGHT"))
    {
        (*horizontal) = FACEOE_GFX_ALIGN_END;
    }
    if (Is(node, name, "A661_TOP") || Is(node, name, "A661_TOP_LEFT") || Is(node, name, "A661_TOP_CENTER") || Is(node, name, "A661_TOP_RIGHT"))
    {
        (*vertical) = FACEOE_GFX_ALIGN_START;
    }
    else if (Is(node, name, "A661_BOTTOM") || Is(node, name, "A661_BOTTOM_LEFT") || Is(node, name, "A661_BOTTOM_CENTER") || Is(node, name, "A661_BOTTOM_RIGHT"))
    {
        (*vertical) = FACEOE_GFX_ALIGN_END;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void BoxText(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* text, FACEOE_Gfx_Align_t horizontal, FACEOE_Gfx_Align_t vertical, float32_t radians, FACEOE_Gfx_Color_t color)
{
    if ('\0' != text[0])
    {
        FACEOE_Gfx_DrawList_Text(renderer->drawList, x, y, width, height, text, horizontal, vertical, FontFor(renderer, height), radians, color);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Frame(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t fill, FACEOE_Gfx_Color_t border, float32_t alpha, Bounds_t* bounds)
{
    if (0 != FACEOE_GFX_COLOR_ALPHA(fill))
    {
        FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, x, y, width, height, Fade(fill, alpha));
    }
    FACEOE_Gfx_DrawList_StrokeRectangle(renderer->drawList, x, y, width, height, Fade(border, alpha), RENDERER_THIN);
    ExtendBox(renderer, bounds, x, y, width, height);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Dashed(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_DrawList_DashedRectangle(renderer->drawList, x, y, width, height, RENDERER_DASH_LENGTH, Fade(color, alpha), RENDERER_THIN);
    ExtendBox(renderer, bounds, x, y, width, height);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Marker(const Renderer_t* renderer, float32_t x, float32_t y, float32_t radians, FACEOE_Gfx_Color_t color, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Color_t faded;

    faded = Fade(color, alpha);
    FACEOE_Gfx_DrawList_Arc(renderer->drawList, x, y, RENDERER_MARKER_RADIUS, RENDERER_MARKER_RADIUS, 0.0f, RENDERER_FULL_TURN, FALSE, faded, RENDERER_THIN);
    FACEOE_Gfx_DrawList_Line(renderer->drawList, x - RENDERER_MARKER_RADIUS, y, x + RENDERER_MARKER_RADIUS, y, faded, RENDERER_THIN);
    FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y - RENDERER_MARKER_RADIUS, x, y + RENDERER_MARKER_RADIUS, faded, RENDERER_THIN);
    FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y, x + (2.0f * RENDERER_MARKER_RADIUS * cosf(radians)), y + (2.0f * RENDERER_MARKER_RADIUS * sinf(radians)), faded, RENDERER_THICK);
    ExtendBox(renderer, bounds, x - RENDERER_MARKER_RADIUS, y - RENDERER_MARKER_RADIUS, 2.0f * RENDERER_MARKER_RADIUS, 2.0f * RENDERER_MARKER_RADIUS);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Placeholder(const Renderer_t* renderer, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* caption, float32_t alpha, Bounds_t* bounds)
{
    Frame(renderer, x, y, width, height, COLOR_PICTURE, COLOR_BORDER, alpha, bounds);
    FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y, x + width, y + height, Fade(COLOR_BORDER, alpha), RENDERER_THIN);
    FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y + height, x + width, y, Fade(COLOR_BORDER, alpha), RENDERER_THIN);
    BoxText(renderer, x, y, width, height, caption, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Scrollbar(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const char_t* name, float32_t x, float32_t y, float32_t width, float32_t height, float32_t fraction, float32_t start, float32_t alpha)
{
    float32_t thickness;
    float32_t bar_x;
    float32_t bar_y;
    float32_t bar_width;
    float32_t bar_height;
    bool_t    is_vertical;
    bool_t    is_present;

    thickness   = RENDERER_CHIP_HEIGHT * 0.5f;
    is_present  = TRUE;
    is_vertical = FALSE;
    bar_x       = x;
    bar_y       = y;
    bar_width   = width;
    bar_height  = thickness;
    if (Is(node, name, "A661_LEFT"))
    {
        is_vertical = TRUE;
        bar_width   = thickness;
        bar_height  = height;
    }
    else if (Is(node, name, "A661_RIGHT"))
    {
        is_vertical = TRUE;
        bar_x       = (x + width - thickness);
        bar_width   = thickness;
        bar_height  = height;
    }
    else if (Is(node, name, "A661_TOP"))
    {
        bar_y = (y + height - thickness);
    }
    else if (FALSE == Is(node, name, "A661_BOTTOM"))
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
        FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, bar_x, bar_y, bar_width, bar_height, Fade(COLOR_SCROLLBAR, alpha * 0.5f));
        if (is_vertical)
        {
            // Run the thumb from the top of the bar, where the first entry sits
            FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, bar_x, bar_y + (bar_height * (1.0f - start - fraction)), bar_width, bar_height * fraction, Fade(COLOR_SCROLLBAR, alpha));
        }
        else
        {
            FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, bar_x + (bar_width * start), bar_y, bar_width * fraction, bar_height, Fade(COLOR_SCROLLBAR, alpha));
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Chip(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Bounds_t* bounds)
{
    const Arinc661_Widget_t* widget;
    char_t                   text[RENDERER_TEXT_BYTES];
    float32_t                x;
    float32_t                y;
    float32_t                unit_x;
    float32_t                unit_y;
    float32_t                height;
    float32_t                width;
    float32_t                gap;

    if (renderer->options->showsNonGraphical)
    {
        widget = WidgetOf(node);
        if (FACEOE_Gfx_DrawList_PushTransform(renderer->drawList))
        {
            // Place chips in layer space regardless of the container's transform, sized from the font so text always fits
            FACEOE_Gfx_DrawList_SetTransform(renderer->drawList, &renderer->base);
            FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
            height = ((renderer->options->fontSize * 1.4f) / unit_y);
            gap    = (height * 0.25f);
            width  = ((renderer->options->fontSize * (float32_t)(strnlen(widget->name, sizeof(widget->name)) + strnlen(widget->type, sizeof(widget->type)) + 2) * 0.6f) / unit_x);
            x      = RENDERER_PAD;
            y      = renderer->layerHeight - RENDERER_PAD - ((float32_t)(renderer->chipCount + 1) * (height + gap));
            renderer->chipCount += 1;
            bounds->isChip = TRUE;
            (void)snprintf(text, sizeof(text), "%s  %s", widget->name, widget->type);
            Frame(renderer, x, y, width, height, COLOR_CHIP, COLOR_CHIP_BORDER, alpha, bounds);
            BoxText(renderer, x + (gap * 0.5f), y, width - gap, height, text, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
            FACEOE_Gfx_DrawList_PopTransform(renderer->drawList);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static uint32_t ExtensionCount(const FACEOE_Utl_Node_t* node)
{
    const FACEOE_Utl_Node_t* child;
    uint32_t                 count;

    count = 0;
    for (child = GetFirstChild(node); NULL != child; child = child->nextSibling)
    {
        if (WidgetOf(child)->isExtension)
        {
            count += 1;
        }
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Record(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Bounds_t* bounds, float32_t alpha)
{
    Arinc661_Renderer_Options_t* options;
    const Arinc661_Widget_t*     widget;
    FACEOE_Gfx_Transform_t       identity;
    char_t                       text[RENDERER_TEXT_BYTES];
    uint32_t                     extension_count;
    float32_t                    font_size;

    options = renderer->options;
    widget  = WidgetOf(node);
    if (bounds->has)
    {
        if ((NULL != options->bounds) && (options->boundsCount < options->boundsCapacity))
        {
            options->bounds[options->boundsCount].widget  = widget;
            options->bounds[options->boundsCount].minimum = bounds->minimum;
            options->bounds[options->boundsCount].maximum = bounds->maximum;
            options->boundsCount += 1;
        }
        // A chip already carries the name
        if (options->showsNames && (FALSE == bounds->isChip) && FACEOE_Gfx_DrawList_PushTransform(renderer->drawList))
        {
            extension_count = ExtensionCount(node);
            if (0 < extension_count)
            {
                (void)snprintf(text, sizeof(text), "%s +%u", widget->name, (unsigned)extension_count);
            }
            else
            {
                (void)snprintf(text, sizeof(text), "%s", widget->name);
            }
            // Draw names upright in output space at the bounds' top-left corner
            (void)memset(&identity, 0, sizeof(identity));
            identity.a = 1.0f;
            identity.d = 1.0f;
            FACEOE_Gfx_DrawList_SetTransform(renderer->drawList, &identity);
            font_size = (options->fontSize * RENDERER_NAME_FONT_SCALE);
            FACEOE_Gfx_DrawList_Text(renderer->drawList, bounds->minimum.x + 2.0f, bounds->minimum.y + 1.0f, bounds->maximum.x - bounds->minimum.x, font_size + 2.0f, text, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_START, font_size, 0.0f, Fade(COLOR_NAME, alpha));
            FACEOE_Gfx_DrawList_PopTransform(renderer->drawList);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void Button(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, const char_t* label, const char_t* iconName, const char_t* iconPosition, float32_t alpha, Bounds_t* bounds, float32_t* x, float32_t* y, float32_t* width, float32_t* height)
{
    FACEOE_Gfx_Align_t horizontal;
    FACEOE_Gfx_Align_t vertical;
    char_t             caption[RENDERER_TEXT_BYTES];
    float32_t          label_x;
    float32_t          label_y;
    float32_t          label_width;
    float32_t          label_height;
    float32_t          icon_size;

    Rect(node, layout, x, y, width, height);
    Frame(renderer, (*x), (*y), (*width), (*height), COLOR_BUTTON, COLOR_BUTTON_BORDER, alpha, bounds);
    label_x      = (*x);
    label_y      = (*y);
    label_width  = (*width);
    label_height = (*height);
    if ((NULL != iconName) && (NULL != Value(node, iconName, 0)))
    {
        // Reserve a square for the picture or symbol on the side the position names
        icon_size = (*height);
        if ((*width) < icon_size)
        {
            icon_size = (*width);
        }
        icon_size *= 0.6f;
        (void)snprintf(caption, sizeof(caption), "%s", Text(node, iconName));
        if (Is(node, iconPosition, "A661_LEFT"))
        {
            Placeholder(renderer, (*x) + RENDERER_PAD, (*y) + (((*height) - icon_size) * 0.5f), icon_size, icon_size, caption, alpha, bounds);
            label_x += (icon_size + (2.0f * RENDERER_PAD));
            label_width -= (icon_size + (2.0f * RENDERER_PAD));
        }
        else if (Is(node, iconPosition, "A661_RIGHT"))
        {
            Placeholder(renderer, (*x) + (*width) - icon_size - RENDERER_PAD, (*y) + (((*height) - icon_size) * 0.5f), icon_size, icon_size, caption, alpha, bounds);
            label_width -= (icon_size + (2.0f * RENDERER_PAD));
        }
        else if (Is(node, iconPosition, "A661_TOP"))
        {
            Placeholder(renderer, (*x) + (((*width) - icon_size) * 0.5f), (*y) + (*height) - icon_size - RENDERER_PAD, icon_size, icon_size, caption, alpha, bounds);
            label_height -= (icon_size + (2.0f * RENDERER_PAD));
        }
        else if (Is(node, iconPosition, "A661_BOTTOM"))
        {
            Placeholder(renderer, (*x) + (((*width) - icon_size) * 0.5f), (*y) + RENDERER_PAD, icon_size, icon_size, caption, alpha, bounds);
            label_y += (icon_size + (2.0f * RENDERER_PAD));
            label_height -= (icon_size + (2.0f * RENDERER_PAD));
        }
        else
        {
            Placeholder(renderer, (*x) + (((*width) - icon_size) * 0.5f), (*y) + (((*height) - icon_size) * 0.5f), icon_size, icon_size, caption, alpha * 0.6f, bounds);
        }
    }
    if (NULL != label)
    {
        AlignOf(node, "Alignment", &horizontal, &vertical);
        BoxText(renderer, label_x + RENDERER_PAD, label_y, label_width - (2.0f * RENDERER_PAD), label_height, label, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void EntryRows(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const char_t* arrayName, const char_t* enableName, uint32_t first, uint32_t count, uint32_t selected, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Align_t align, float32_t alpha)
{
    const char_t* text;
    float32_t     unit_x;
    float32_t     unit_y;
    float32_t     row_height;
    float32_t     row_y;
    float32_t     row_alpha;
    uint32_t      entry;
    bool_t        is_row_inside;

    FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
    row_height = ((renderer->options->fontSize * 1.3f) / unit_y);
    if (row_height <= 0.0f)
    {
        row_height = RENDERER_CHIP_HEIGHT;
    }
    FACEOE_Gfx_DrawList_PushClip(renderer->drawList, x, y, width, height);
    row_y = (y + height - row_height);
    for (entry = first; entry < count; ++entry)
    {
        is_row_inside = ((y - row_height) < row_y);
        if (FALSE == is_row_inside)
        {
            break;
        }
        text = Value(node, arrayName, entry);
        if (NULL == text)
        {
            text = "";
        }
        row_alpha = alpha;
        if ((NULL != enableName) && IsAt(node, enableName, entry, "A661_FALSE"))
        {
            row_alpha *= RENDERER_DISABLED_ALPHA;
        }
        if ((entry + 1) == selected)
        {
            FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, x, row_y, width, row_height, Fade(COLOR_ROW_SELECTED, alpha));
        }
        BoxText(renderer, x + RENDERER_PAD, row_y, width - (2.0f * RENDERER_PAD), row_height, text, align, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, row_alpha));
        row_y -= row_height;
    }
    FACEOE_Gfx_DrawList_PopClip(renderer->drawList);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void NumericBox(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t radians, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Align_t horizontal;
    FACEOE_Gfx_Align_t vertical;
    const char_t*      format;
    const char_t*      legend;
    char_t             text[RENDERER_TEXT_BYTES];
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

    Rect(node, layout, &x, &y, &width, &height);
    if (0.0f != radians)
    {
        FACEOE_Gfx_DrawList_Rotate(renderer->drawList, x, y, radians);
    }
    Frame(renderer, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, bounds);
    format = Value(node, "FormatString", 0);
    if (NULL == format)
    {
        format = "";
    }
    Arinc661_Renderer_FormatNumber(format, (float64_t)NumberOr(node, "Value", 0.0f), text, sizeof(text));
    legend     = Value(node, "LegendString", 0);
    has_legend = ((NULL != legend) && ('\0' != legend[0]) && (FALSE == Is(node, "LegendRemoved", "A661_TRUE")));
    field_x      = x;
    field_y      = y;
    field_width  = width;
    field_height = height;
    if (has_legend)
    {
        legend_size = NumberOr(node, "LegendAreaSizeX", width * 0.3f);
        FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
        legend_height = ((renderer->options->fontSize * 1.2f) / unit_y);
        if ((height * 0.5f) < legend_height)
        {
            legend_height = (height * 0.5f);
        }
        if (Is(node, "LegendPosition", "A661_RIGHT"))
        {
            field_width -= legend_size;
            BoxText(renderer, x + width - legend_size, y, legend_size, height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        else if (Is(node, "LegendPosition", "A661_TOP"))
        {
            field_height -= legend_height;
            BoxText(renderer, x, y + height - legend_height, width, legend_height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        else if (Is(node, "LegendPosition", "A661_BOTTOM"))
        {
            field_y += legend_height;
            field_height -= legend_height;
            BoxText(renderer, x, y, width, legend_height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        else
        {
            field_x += legend_size;
            field_width -= legend_size;
            BoxText(renderer, x, y, legend_size, height, legend, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
        }
        FACEOE_Gfx_DrawList_Line(renderer->drawList, field_x, field_y, field_x, field_y + field_height, Fade(COLOR_EDIT_BORDER, alpha * 0.5f), RENDERER_THIN);
    }
    AlignOf(node, "Alignment", &horizontal, &vertical);
    BoxText(renderer, field_x + RENDERER_PAD, field_y, field_width - (2.0f * RENDERER_PAD), field_height, text, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawGp(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, Kind_t kind, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Point_t points[RENDERER_MAX_POINTS];
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

    color     = IndexColor(node, alpha);
    halo      = Fade(FACEOE_GFX_RGBA(0, 0, 0, 255), alpha);
    is_filled = Is(node, "Filled", "A661_TRUE");
    has_halo  = (FALSE == Is(node, "Halo", "A661_FALSE")) && (NULL != Value(node, "Halo", 0));
    thickness = RENDERER_THICK;
    x         = NumberOr(node, "PosX", 0.0f);
    y         = NumberOr(node, "PosY", 0.0f);
    width     = NumberOr(node, "SizeX", 0.0f);
    height    = NumberOr(node, "SizeY", 0.0f);
    start     = (NumberOr(node, "StartAngle", 0.0f) * RENDERER_DEGREES);
    end       = (NumberOr(node, "EndAngle", 0.0f) * RENDERER_DEGREES);
    switch (kind)
    {
        case KIND_GP_LINE:
            x  = NumberOr(node, "PosXStart", 0.0f);
            y  = NumberOr(node, "PosYStart", 0.0f);
            x2 = NumberOr(node, "PosXEnd", 0.0f);
            y2 = NumberOr(node, "PosYEnd", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y, x2, y2, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y, x2, y2, color, thickness);
            Extend(renderer, bounds, x, y);
            Extend(renderer, bounds, x2, y2);
            break;
        case KIND_GP_LINE_POLAR:
            x      = NumberOr(node, "PosXStart", 0.0f);
            y      = NumberOr(node, "PosYStart", 0.0f);
            radius = NumberOr(node, "LineLength", 0.0f);
            start  = (NumberOr(node, "RotationAngle", 0.0f) * RENDERER_DEGREES);
            x2     = (x + (radius * cosf(start)));
            y2     = (y + (radius * sinf(start)));
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y, x2, y2, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Line(renderer->drawList, x, y, x2, y2, color, thickness);
            Extend(renderer, bounds, x, y);
            Extend(renderer, bounds, x2, y2);
            break;
        case KIND_GP_ARC_CIRCLE:
            radius = NumberOr(node, "Radius", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Arc(renderer->drawList, x, y, radius, radius, start, end, FALSE, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Arc(renderer->drawList, x, y, radius, radius, start, end, is_filled, color, thickness);
            ExtendBox(renderer, bounds, x - radius, y - radius, 2.0f * radius, 2.0f * radius);
            break;
        case KIND_GP_ARC_ELLIPSE:
            // Take SizeX and SizeY as the full axes of the ellipse centered on the reference point
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Arc(renderer->drawList, x, y, width * 0.5f, height * 0.5f, start, end, FALSE, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Arc(renderer->drawList, x, y, width * 0.5f, height * 0.5f, start, end, is_filled, color, thickness);
            ExtendBox(renderer, bounds, x - (width * 0.5f), y - (height * 0.5f), width, height);
            break;
        case KIND_GP_CROWN:
            inner  = NumberOr(node, "InnerRadius", 0.0f);
            radius = NumberOr(node, "OuterRadius", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_Crown(renderer->drawList, x, y, inner, radius, start, end, FALSE, halo, thickness + 2.0f);
            }
            FACEOE_Gfx_DrawList_Crown(renderer->drawList, x, y, inner, radius, start, end, is_filled, color, thickness);
            ExtendBox(renderer, bounds, x - radius, y - radius, 2.0f * radius, 2.0f * radius);
            break;
        case KIND_GP_RECTANGLE:
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_StrokeRectangle(renderer->drawList, x, y, width, height, halo, thickness + 2.0f);
            }
            if (is_filled)
            {
                FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, x, y, width, height, color);
            }
            else
            {
                FACEOE_Gfx_DrawList_StrokeRectangle(renderer->drawList, x, y, width, height, color, thickness);
            }
            ExtendBox(renderer, bounds, x, y, width, height);
            break;
        case KIND_GP_TRIANGLE:
            triangle[0].x = x;
            triangle[0].y = y;
            triangle[1].x = NumberOr(node, "PosX2", 0.0f);
            triangle[1].y = NumberOr(node, "PosY2", 0.0f);
            triangle[2].x = NumberOr(node, "PosX3", 0.0f);
            triangle[2].y = NumberOr(node, "PosY3", 0.0f);
            if (has_halo)
            {
                FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, triangle, 3, TRUE, halo, thickness + 2.0f);
            }
            if (is_filled)
            {
                FACEOE_Gfx_DrawList_FillPolygon(renderer->drawList, triangle, 3, color);
            }
            else
            {
                FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, triangle, 3, TRUE, color, thickness);
            }
            for (index = 0; index < 3; ++index)
            {
                Extend(renderer, bounds, triangle[index].x, triangle[index].y);
            }
            break;
        case KIND_GP_POLYLINE:
        case KIND_GP_TRIANGLE_FAN:
        case KIND_GP_TRIANGLE_STRIP:
            // Read vertices as alternating X and Y; bound the pairs by NumberOfVertices, the values present, and RENDERER_MAX_POINTS
            count = (uint32_t)NumberOr(node, "NumberOfVertices", 0.0f);
            if ((Occurrences(node, "Vertices") / 2) < count)
            {
                count = (Occurrences(node, "Vertices") / 2);
            }
            if (RENDERER_MAX_POINTS < count)
            {
                count = RENDERER_MAX_POINTS;
            }
            for (index = 0; index < count; ++index)
            {
                points[index].x = NumberAt(node, "Vertices", 2 * index, 0.0f);
                points[index].y = NumberAt(node, "Vertices", (2 * index) + 1, 0.0f);
                Extend(renderer, bounds, points[index].x, points[index].y);
            }
            if (KIND_GP_POLYLINE == kind)
            {
                if (has_halo)
                {
                    FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, points, count, Is(node, "Closed", "A661_TRUE"), halo, thickness + 2.0f);
                }
                FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, points, count, Is(node, "Closed", "A661_TRUE"), color, thickness);
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
                        FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, triangle, 3, TRUE, halo, thickness + 2.0f);
                    }
                    if (is_filled)
                    {
                        FACEOE_Gfx_DrawList_FillPolygon(renderer->drawList, triangle, 3, color);
                    }
                    else
                    {
                        FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, triangle, 3, TRUE, color, thickness);
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
 * \par Details:
 *  None
 */
static void DrawSlider(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t alpha, Bounds_t* bounds)
{
    char_t    label[RENDERER_TEXT_BYTES];
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
    bool_t    is_tick_in_range;
    bool_t    shows_labels;

    Rect(node, layout, &x, &y, &width, &height);
    Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, bounds);
    track_x        = (x + (width * 0.5f));
    track_y        = (y + (height * 0.5f));
    minimum        = NumberOr(node, "MinValue", 0.0f);
    maximum        = NumberOr(node, "MaxValue", 100.0f);
    value          = NumberOr(node, "Value", minimum);
    interval       = NumberOr(node, "MajorTickInterval", 0.0f);
    reference      = NumberOr(node, "MajorTickReference", minimum);
    minor_multiple = NumberOr(node, "MinorTickMultiple", 0.0f);
    shows_labels   = Is(node, "ShowMajorLabels", "A661_TRUE");
    is_vertical    = (Is(node, "Orientation", "A661_BOTTOM_TO_TOP") || Is(node, "Orientation", "A661_TOP_TO_BOTTOM"));
    is_reversed    = (Is(node, "Orientation", "A661_RIGHT_TO_LEFT") || Is(node, "Orientation", "A661_TOP_TO_BOTTOM"));
    if (maximum <= minimum)
    {
        maximum = (minimum + 1.0f);
    }
    // Put ticks and labels on the side the Alignment names; give the track the rest
    side = 0.0f;
    if (Is(node, "Alignment", "A661_LEFT") || Is(node, "Alignment", "A661_BOTTOM"))
    {
        side = -1.0f;
    }
    else if (Is(node, "Alignment", "A661_RIGHT") || Is(node, "Alignment", "A661_TOP"))
    {
        side = 1.0f;
    }
    FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
    label_height = ((renderer->options->fontSize * 1.1f) / unit_y);
    if (is_vertical)
    {
        track_x      = (x + (width * 0.5f) - (side * width * 0.25f));
        tick_length  = (width * 0.2f);
        minor_length = (tick_length * 0.5f);
        thumb        = (width * 0.3f);
        FACEOE_Gfx_DrawList_Line(renderer->drawList, track_x, y + RENDERER_PAD, track_x, y + height - RENDERER_PAD, Fade(COLOR_TRACK, alpha), RENDERER_THICK);
    }
    else
    {
        track_y      = (y + (height * 0.5f) - (side * height * 0.25f));
        tick_length  = (height * 0.2f);
        minor_length = (tick_length * 0.5f);
        thumb        = (height * 0.3f);
        FACEOE_Gfx_DrawList_Line(renderer->drawList, x + RENDERER_PAD, track_y, x + width - RENDERER_PAD, track_y, Fade(COLOR_TRACK, alpha), RENDERER_THICK);
    }
    if (0.0f < interval)
    {
        // Step down from the reference to the minimum or below, then walk up across the range
        tick = reference;
        while (minimum < tick)
        {
            tick -= interval;
        }
        for (tick_index = 0; tick_index < RENDERER_MAX_TICKS; ++tick_index)
        {
            is_tick_in_range = (tick <= maximum);
            if (FALSE == is_tick_in_range)
            {
                break;
            }
            if (minimum <= tick)
            {
                fraction = ((tick - minimum) / (maximum - minimum));
                if (is_reversed)
                {
                    fraction = (1.0f - fraction);
                }
                if (is_vertical)
                {
                    along = (y + RENDERER_PAD + (fraction * (height - (2.0f * RENDERER_PAD))));
                    FACEOE_Gfx_DrawList_Line(renderer->drawList, track_x, along, track_x + (side * tick_length), along, Fade(COLOR_TRACK, alpha), RENDERER_THIN);
                    if (shows_labels)
                    {
                        (void)snprintf(label, sizeof(label), "%g", (float64_t)tick);
                        if (0.0f <= side)
                        {
                            BoxText(renderer, track_x + tick_length, along - (label_height * 0.5f), (x + width) - (track_x + tick_length), label_height, label, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                        else
                        {
                            BoxText(renderer, x, along - (label_height * 0.5f), (track_x - tick_length) - x, label_height, label, FACEOE_GFX_ALIGN_END, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                    }
                }
                else
                {
                    along = (x + RENDERER_PAD + (fraction * (width - (2.0f * RENDERER_PAD))));
                    FACEOE_Gfx_DrawList_Line(renderer->drawList, along, track_y, along, track_y + (side * tick_length), Fade(COLOR_TRACK, alpha), RENDERER_THIN);
                    if (shows_labels)
                    {
                        (void)snprintf(label, sizeof(label), "%g", (float64_t)tick);
                        if (0.0f <= side)
                        {
                            BoxText(renderer, along - (width * 0.5f), track_y + tick_length, width, label_height, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_START, 0.0f, Fade(COLOR_TEXT, alpha));
                        }
                        else
                        {
                            BoxText(renderer, along - (width * 0.5f), track_y - tick_length - label_height, width, label_height, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_END, 0.0f, Fade(COLOR_TEXT, alpha));
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
                            along = (y + RENDERER_PAD + (fraction * (height - (2.0f * RENDERER_PAD))));
                            FACEOE_Gfx_DrawList_Line(renderer->drawList, track_x, along, track_x + (side * minor_length), along, Fade(COLOR_TRACK, alpha), RENDERER_THIN);
                        }
                        else
                        {
                            along = (x + RENDERER_PAD + (fraction * (width - (2.0f * RENDERER_PAD))));
                            FACEOE_Gfx_DrawList_Line(renderer->drawList, along, track_y, along, track_y + (side * minor_length), Fade(COLOR_TRACK, alpha), RENDERER_THIN);
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
        along = (y + RENDERER_PAD + (fraction * (height - (2.0f * RENDERER_PAD))));
        FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, track_x - (thumb * 0.5f), along - (thumb * 0.3f), thumb, thumb * 0.6f, Fade(COLOR_BUTTON_BORDER, alpha));
    }
    else
    {
        along = (x + RENDERER_PAD + (fraction * (width - (2.0f * RENDERER_PAD))));
        FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, along - (thumb * 0.3f), track_y - (thumb * 0.5f), thumb * 0.6f, thumb, Fade(COLOR_BUTTON_BORDER, alpha));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawTree(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t alpha, Bounds_t* bounds)
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
    bool_t             is_row_inside;
    bool_t             has_children;

    Rect(node, layout, &x, &y, &width, &height);
    Frame(renderer, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, bounds);
    AlignOf(node, "Alignment", &horizontal, &vertical);
    count    = (uint32_t)NumberOr(node, "NumberOfEntries", 0.0f);
    first    = (uint32_t)NumberOr(node, "FirstVisibleEntry", 1.0f);
    selected = (uint32_t)NumberOr(node, "SelectedEntry", 0.0f);
    if (Occurrences(node, "LabelStringArray") < count)
    {
        count = Occurrences(node, "LabelStringArray");
    }
    FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
    row_height = ((renderer->options->fontSize * 1.3f) / unit_y);
    indent     = (row_height * 1.2f);
    FACEOE_Gfx_DrawList_PushClip(renderer->drawList, x, y, width, height);
    row_y = (y + height - row_height);
    shown = 0;
    for (entry = 0; entry < count; ++entry)
    {
        is_row_inside = ((y - row_height) < row_y);
        if (FALSE == is_row_inside)
        {
            break;
        }
        // Show an entry only when every ancestor is open; read ParentArray as 1-based with 0 for roots
        is_shown = TRUE;
        depth    = 0;
        parent   = (uint32_t)NumberAt(node, "ParentArray", entry, 0.0f);
        for (guard = 0; guard < count; ++guard)
        {
            if (0 == parent)
            {
                break;
            }
            if (count < parent)
            {
                break;
            }
            depth += 1;
            if (IsAt(node, "OpenArray", parent - 1, "A661_FALSE"))
            {
                is_shown = FALSE;
            }
            parent = (uint32_t)NumberAt(node, "ParentArray", parent - 1, 0.0f);
        }
        if (is_shown)
        {
            shown += 1;
            if (first <= shown)
            {
                has_children = FALSE;
                for (other = 0; other < count; ++other)
                {
                    if ((entry + 1) == (uint32_t)NumberAt(node, "ParentArray", other, 0.0f))
                    {
                        has_children = TRUE;
                        break;
                    }
                }
                row_alpha = alpha;
                if (IsAt(node, "EnableArray", entry, "A661_FALSE"))
                {
                    row_alpha *= RENDERER_DISABLED_ALPHA;
                }
                if ((entry + 1) == selected)
                {
                    FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, x, row_y, width, row_height, Fade(COLOR_ROW_SELECTED, alpha));
                }
                if (has_children)
                {
                    if (IsAt(node, "OpenArray", entry, "A661_FALSE"))
                    {
                        text = "+";
                    }
                    else
                    {
                        text = "-";
                    }
                    BoxText(renderer, x + RENDERER_PAD + ((float32_t)depth * indent), row_y, indent, row_height, text, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, row_alpha));
                }
                text = Value(node, "LabelStringArray", entry);
                if (NULL == text)
                {
                    text = "";
                }
                BoxText(renderer, x + RENDERER_PAD + ((float32_t)(depth + 1) * indent), row_y, width - (2.0f * RENDERER_PAD) - ((float32_t)(depth + 1) * indent), row_height, text, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, row_alpha));
                row_y -= row_height;
            }
        }
    }
    FACEOE_Gfx_DrawList_PopClip(renderer->drawList);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawMapBoundary(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Bounds_t* bounds)
{
    FACEOE_Gfx_Point_t points[RENDERER_MAX_POINTS];
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

    count = (uint32_t)NumberOr(node, "NumberOfEntries", 0.0f);
    if (Occurrences(node, "BeginAngleArray") < count)
    {
        count = Occurrences(node, "BeginAngleArray");
    }
    for (section = 0; section < count; ++section)
    {
        begin         = NumberAt(node, "BeginAngleArray", section, 0.0f);
        next          = NumberAt(node, "BeginAngleArray", (section + 1) % count, 0.0f);
        distance      = NumberAt(node, "DistanceArray", section, 0.0f);
        is_vertical   = IsAt(node, "AlgorithmArray", section, "A661_VERT_LINE");
        is_horizontal = IsAt(node, "AlgorithmArray", section, "A661_HORZ_LINE");
        sweep         = (next - begin);
        if (sweep <= 0.0f)
        {
            sweep += 360.0f;
        }
        step_count = (uint32_t)(sweep / RENDERER_BOUNDARY_STEP);
        if (step_count < 1)
        {
            step_count = 1;
        }
        if ((RENDERER_MAX_POINTS - 1) < step_count)
        {
            step_count = (RENDERER_MAX_POINTS - 1);
        }
        point_count = 0;
        for (step = 0; step <= step_count; ++step)
        {
            angle  = ((begin + (sweep * ((float32_t)step / (float32_t)step_count))) * RENDERER_DEGREES);
            radius = distance;
            // A line at perpendicular distance D crosses the ray at D divided by the cosine of the angle to its normal; cap at 4 D
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
            Extend(renderer, bounds, points[point_count].x, points[point_count].y);
            point_count += 1;
        }
        FACEOE_Gfx_DrawList_StrokePolyline(renderer->drawList, points, point_count, FALSE, Fade(COLOR_RANGE, alpha), RENDERER_THIN);
    }
    if (0 == count)
    {
        Marker(renderer, 0.0f, 0.0f, 0.0f, COLOR_RANGE, alpha, bounds);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawMapGrid(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Bounds_t* bounds)
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

    offset_x    = NumberOr(node, "OffsetX", 0.0f);
    offset_y    = NumberOr(node, "OffsetY", 0.0f);
    increment_x = NumberOr(node, "IncrementX", 100.0f);
    increment_y = NumberOr(node, "IncrementY", 100.0f);
    count_x     = NumberOr(node, "CountX", 0.0f);
    count_y     = NumberOr(node, "CountY", 0.0f);
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
        FACEOE_Gfx_DrawList_Line(renderer->drawList, origin_x + ((float32_t)line * increment_x), origin_y, origin_x + ((float32_t)line * increment_x), origin_y + (count_y * increment_y), Fade(COLOR_GRID, alpha), RENDERER_THIN);
    }
    for (line = 0; line <= (uint32_t)count_y; ++line)
    {
        FACEOE_Gfx_DrawList_Line(renderer->drawList, origin_x, origin_y + ((float32_t)line * increment_y), origin_x + (count_x * increment_x), origin_y + ((float32_t)line * increment_y), Fade(COLOR_GRID, alpha), RENDERER_THIN);
    }
    ExtendBox(renderer, bounds, origin_x, origin_y, count_x * increment_x, count_y * increment_y);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawTabbedGroup(const Renderer_t* renderer, const FACEOE_Utl_Node_t* node, const Layout_t* layout, float32_t alpha, Bounds_t* bounds, Children_t* children)
{
    const char_t*            label;
    const FACEOE_Utl_Node_t* child;
    float32_t                x;
    float32_t                y;
    float32_t                width;
    float32_t                height;
    float32_t                strip;
    float32_t                unit_x;
    float32_t                unit_y;
    float32_t                along;
    float32_t                inset;
    float32_t                tab_alpha;
    uint32_t                 tab_count;
    bool_t                   is_automatic;
    bool_t                   is_active;
    int32_t                  position;

    Rect(node, layout, &x, &y, &width, &height);
    Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, bounds);
    FACEOE_Gfx_DrawList_UnitLengths(renderer->drawList, &unit_x, &unit_y);
    strip = ((renderer->options->fontSize * 1.6f) / unit_y);
    if ((height * 0.3f) < strip)
    {
        strip = (height * 0.3f);
    }
    // Encode the tab position as 0 bottom, 1 top, 2 left, 3 right, -1 absent
    position = 1;
    if (Is(node, "TabPosition", "A661_BOTTOM"))
    {
        position = 0;
    }
    else if (Is(node, "TabPosition", "A661_LEFT"))
    {
        position = 2;
    }
    else if (Is(node, "TabPosition", "A661_RIGHT"))
    {
        position = 3;
    }
    else if (Is(node, "TabPosition", "A661_ABSENT"))
    {
        position = -1;
    }
    children->mode          = CHILD_TABS;
    children->ident         = (int64_t)NumberOr(node, "ActiveTabbedPanelID", 0.0f);
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
        is_automatic = Is(node, "AutomaticInsetSizeFlag", "A661_TRUE");
        tab_count    = 0;
        for (child = GetFirstChild(node); NULL != child; child = child->nextSibling)
        {
            if (FALSE == WidgetOf(child)->isExtension)
            {
                tab_count += 1;
            }
        }
        along = 0.0f;
        for (child = GetFirstChild(node); NULL != child; child = child->nextSibling)
        {
            if (FALSE == WidgetOf(child)->isExtension)
            {
                inset = NumberOr(child, "InsetSize", 0.0f);
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
                is_active = (Ident(child) == children->ident);
                tab_alpha = alpha;
                if (FALSE == is_active)
                {
                    tab_alpha *= 0.7f;
                }
                label = Value(child, "LabelString", 0);
                if (NULL == label)
                {
                    label = WidgetOf(child)->name;
                }
                if (0 == position)
                {
                    Frame(renderer, x + along, y, inset, strip, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(renderer, x + along, y, inset, strip, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                else if (1 == position)
                {
                    Frame(renderer, x + along, y + height - strip, inset, strip, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(renderer, x + along, y + height - strip, inset, strip, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                else if (2 == position)
                {
                    Frame(renderer, x, y + height - along - inset, strip, inset, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(renderer, x, y + height - along - inset, strip, inset, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                else
                {
                    Frame(renderer, x + width - strip, y + height - along - inset, strip, inset, COLOR_BUTTON, COLOR_BUTTON_BORDER, tab_alpha, bounds);
                    BoxText(renderer, x + width - strip, y + height - along - inset, strip, inset, label, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, tab_alpha));
                }
                along += inset;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Interpolates linearly; Delay and Duration are in milliseconds. The sweep repeats
 *  AnimationRepetition times, or forever when it is zero, then holds at its end.
 */
static void ApplyAnimations(Renderer_t* renderer, const FACEOE_Utl_Node_t* node)
{
    const FACEOE_Utl_Node_t* animation;
    uint32_t                 slot;
    float32_t                ident;
    float32_t                target;
    float32_t                delay;
    float32_t                duration;
    float32_t                repetition;
    float32_t                time_ms;
    float32_t                progress;
    float32_t                from_a;
    float32_t                to_a;
    float32_t                from_b;
    float32_t                to_b;
    Kind_t                   kind;

    if ((renderer->options->animationTime < 0.0f) || (FALSE == Number(node, "WidgetIdent", &ident)))
    {
        return;
    }
    time_ms = (renderer->options->animationTime * 1000.0f);
    for (slot = 0; slot < renderer->animationCount; ++slot)
    {
        animation = renderer->animations[slot];
        if ((FALSE == Number(animation, "TargetWidgetID", &target)) || (target != ident))
        {
            continue;
        }
        delay      = NumberOr(animation, "Delay", 0.0f);
        duration   = NumberOr(animation, "Duration", 0.0f);
        repetition = NumberOr(animation, "AnimationRepetition", 1.0f);
        progress   = 0.0f;
        if (0.0f < duration)
        {
            progress = ((time_ms - delay) / duration);
            if (progress < 0.0f)
            {
                progress = 0.0f;
            }
            else if ((1.0f < progress) && ((0.0f == repetition) || (progress < repetition)))
            {
                // Repeat the sweep; zero repetitions means forever
                progress = (progress - floorf(progress));
            }
            else if (1.0f < progress)
            {
                progress = 1.0f;
            }
        }
        kind = KindOf(WidgetOf(animation)->type);
        if (KIND_ANIMATION_TRANSLATION == kind)
        {
            from_a = NumberOr(animation, "FromValueX", 0.0f);
            to_a   = NumberOr(animation, "ToValueX", 0.0f);
            from_b = NumberOr(animation, "FromValueY", 0.0f);
            to_b   = NumberOr(animation, "ToValueY", 0.0f);
            FACEOE_Gfx_DrawList_Translate(renderer->drawList, from_a + ((to_a - from_a) * progress), from_b + ((to_b - from_b) * progress));
        }
        else if (KIND_ANIMATION_ROTATION == kind)
        {
            from_a = NumberOr(animation, "FromAngle", 0.0f);
            to_a   = NumberOr(animation, "ToAngle", 0.0f);
            FACEOE_Gfx_DrawList_Rotate(renderer->drawList, NumberOr(animation, "CenterX", 0.0f), NumberOr(animation, "CenterY", 0.0f), (from_a + ((to_a - from_a) * progress)) * RENDERER_DEGREES);
        }
        else if (KIND_ANIMATION_SCALE == kind)
        {
            from_a = NumberOr(animation, "FromScaleX", 1.0f);
            to_a   = NumberOr(animation, "ToScaleX", 1.0f);
            from_b = NumberOr(animation, "FromScaleY", 1.0f);
            to_b   = NumberOr(animation, "ToScaleY", 1.0f);
            FACEOE_Gfx_DrawList_Scale(renderer->drawList, NumberOr(animation, "CenterX", 0.0f), NumberOr(animation, "CenterY", 0.0f), from_a + ((to_a - from_a) * progress), from_b + ((to_b - from_b) * progress));
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawWidget(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, const Layout_t* layout)
{
    const Arinc661_Widget_t* widget;
    const char_t*            text;
    const char_t*            value;
    FACEOE_Gfx_Align_t       horizontal;
    FACEOE_Gfx_Align_t       vertical;
    Children_t               children;
    Bounds_t                 bounds;
    Kind_t                   kind;
    char_t                   caption[RENDERER_TEXT_BYTES];
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

    widget = WidgetOf(node);
    kind   = KindOf(widget->type);
    // Propagate Visible and Enable to children; skip hidden subtrees unless they are shown faded
    is_visible = (FALSE == Is(node, "Visible", "A661_FALSE"));
    is_drawn   = TRUE;
    if (FALSE == is_visible)
    {
        if (renderer->options->showsHidden)
        {
            alpha *= RENDERER_HIDDEN_ALPHA;
        }
        else
        {
            is_drawn = FALSE;
        }
    }
    if (is_drawn && Is(node, "Enable", "A661_FALSE"))
    {
        alpha *= RENDERER_DISABLED_ALPHA;
    }
    if (is_drawn && (FALSE == FACEOE_Gfx_DrawList_PushTransform(renderer->drawList)))
    {
        is_drawn = FALSE;
    }
    if (is_drawn)
    {
        (void)memset(&children, 0, sizeof(children));
        (void)memset(&bounds, 0, sizeof(bounds));
        children.mode = CHILD_ALL;
        has_clip      = FALSE;
        ApplyAnimations(renderer, node);
        switch (kind)
        {
            case KIND_PANEL:
            case KIND_SHUFFLE_TO_FIT_CONTAINER:
            case KIND_SIZE_TO_FIT_CONTAINER:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_PAGING_CONTAINER:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                if (FALSE == Is(node, "PagingControlPosition", "A661_ABSENT"))
                {
                    // Draw the paging control as a small strip of page marks at the named position
                    AlignOf(node, "PagingControlPosition", &horizontal, &vertical);
                    BoxText(renderer, x + RENDERER_PAD, y + RENDERER_PAD, width - (2.0f * RENDERER_PAD), height - (2.0f * RENDERER_PAD), "< o o o >", horizontal, vertical, 0.0f, Fade(COLOR_TEXT, alpha * 0.8f));
                }
                children.mode  = CHILD_IDENT;
                children.ident = (int64_t)NumberOr(node, "VisibleChild", 0.0f);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_MUTUALLY_EXCLUSIVE_CONTAINER:
                x = NumberOr(node, "PosX", 0.0f);
                y = NumberOr(node, "PosY", 0.0f);
                Marker(renderer, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                children.mode  = CHILD_IDENT;
                children.ident = (int64_t)NumberOr(node, "VisibleChild", 0.0f);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_BASIC_CONTAINER:
            case KIND_MASK_CONTAINER:
            case KIND_CURSOR_REF:
                x = NumberOr(node, "PosX", 0.0f);
                y = NumberOr(node, "PosY", 0.0f);
                Marker(renderer, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_TRANSLATION_CONTAINER:
                x = NumberOr(node, "TranslationX", 0.0f);
                y = NumberOr(node, "TranslationY", 0.0f);
                Marker(renderer, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_ROTATION_CONTAINER:
                x     = NumberOr(node, "CenterX", 0.0f);
                y     = NumberOr(node, "CenterY", 0.0f);
                angle = (NumberOr(node, "RotationAngle", 0.0f) * RENDERER_DEGREES);
                Marker(renderer, x, y, angle, COLOR_MARKER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Rotate(renderer->drawList, x, y, angle);
                break;
            case KIND_SCALE_CONTAINER:
                x = NumberOr(node, "CenterX", 0.0f);
                y = NumberOr(node, "CenterY", 0.0f);
                Marker(renderer, x, y, 0.0f, COLOR_MARKER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Scale(renderer->drawList, x, y, NumberOr(node, "ScaleX", 1.0f), NumberOr(node, "ScaleY", 1.0f));
                break;
            case KIND_SCROLL_PANEL:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                area_width  = NumberOr(node, "SizeXsheet", width);
                area_height = NumberOr(node, "SizeYsheet", height);
                area_x      = NumberOr(node, "FrameX", 0.0f);
                area_y      = NumberOr(node, "FrameY", 0.0f);
                if (0.0f < area_width)
                {
                    Scrollbar(renderer, node, "HorizontalScroll", x, y, width, height, width / area_width, area_x / area_width, alpha);
                }
                if (0.0f < area_height)
                {
                    Scrollbar(renderer, node, "VerticalScroll", x, y, width, height, height / area_height, 1.0f - ((area_y + height) / area_height), alpha);
                }
                // Place children on the sheet and show the sheet through the frame from (FrameX, FrameY)
                FACEOE_Gfx_DrawList_PushClip(renderer->drawList, x, y, width, height);
                has_clip = TRUE;
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x - area_x, y - area_y);
                if (renderer->options->showsHidden)
                {
                    FACEOE_Gfx_DrawList_DashedRectangle(renderer->drawList, 0.0f, 0.0f, area_width, area_height, RENDERER_DASH_LENGTH, Fade(COLOR_BORDER, alpha * 0.5f), RENDERER_THIN);
                }
                break;
            case KIND_TABBED_PANEL_GROUP:
                DrawTabbedGroup(renderer, node, layout, alpha, &bounds, &children);
                break;
            case KIND_TABBED_PANEL:
                // Take the content area the group hands down and position children from it
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_MENU_BAR:
                x                       = NumberOr(node, "PosX", 0.0f);
                y                       = NumberOr(node, "PosY", 0.0f);
                children.mode           = CHILD_MENU_BAR;
                children.isHorizontal   = (FALSE == Is(node, "Horizontal", "A661_FALSE"));
                children.buttonPosition = NumberOr(node, "ButtonPos", 0.0f);
                children.buttonSize     = NumberOr(node, "ButtonSize", 0.0f);
                Marker(renderer, x, y, 0.0f, COLOR_BORDER, alpha * 0.6f, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_MAPHORZ:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                area_x = NumberOr(node, "ScreenReferencePointX", 0.0f);
                area_y = NumberOr(node, "ScreenReferencePointY", 0.0f);
                box    = NumberOr(node, "ScreenRange", 0.0f);
                FACEOE_Gfx_DrawList_PushClip(renderer->drawList, x, y, width, height);
                has_clip = TRUE;
                if (0.0f < box)
                {
                    FACEOE_Gfx_DrawList_Arc(renderer->drawList, x + area_x, y + area_y, box, box, 0.0f, RENDERER_FULL_TURN, FALSE, Fade(COLOR_RANGE, alpha), RENDERER_THIN);
                }
                Marker(renderer, x + area_x, y + area_y, (90.0f * RENDERER_DEGREES), COLOR_RANGE, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x + area_x, y + area_y);
                break;
            case KIND_MAPVERT:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                area_x = NumberOr(node, "RefPosX", 0.0f);
                area_y = NumberOr(node, "RefPosY", 0.0f);
                FACEOE_Gfx_DrawList_PushClip(renderer->drawList, x, y, width, height);
                has_clip = TRUE;
                Marker(renderer, x + area_x, y + area_y, 0.0f, COLOR_RANGE, alpha, &bounds);
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x + area_x, y + area_y);
                break;
            case KIND_MAP_GRID:
                DrawMapGrid(renderer, node, alpha, &bounds);
                break;
            case KIND_MAP_BOUNDARY:
                DrawMapBoundary(renderer, node, alpha, &bounds);
                break;
            case KIND_MAPHORZ_VERTEXBUFFER:
                DrawGp(renderer, node, KIND_GP_POLYLINE, alpha, &bounds);
                break;
            case KIND_ACTIVE_AREA:
            case KIND_CURSOR_OVER:
            case KIND_CURSOR_POS_OVERLAY:
            case KIND_TOUCH_AREA:
            case KIND_GESTURE_AREA:
            case KIND_SCROLL_WHEEL_AREA:
            case KIND_KEYBOARD_AREA:
                Rect(node, layout, &x, &y, &width, &height);
                Dashed(renderer, x, y, width, height, COLOR_AREA, alpha, &bounds);
                if (KIND_KEYBOARD_AREA == kind)
                {
                    (void)snprintf(caption, sizeof(caption), "%u keys", (unsigned)NumberOr(node, "NumberOfKeys", 0.0f));
                    BoxText(renderer, x, y, width, height, caption, FACEOE_GFX_ALIGN_CENTER, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_AREA, alpha));
                }
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_INK_AREA:
                Rect(node, layout, &x, &y, &width, &height);
                if (Is(node, "Filled", "A661_TRUE"))
                {
                    FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, x, y, width, height, IndexColor(node, alpha * 0.5f));
                }
                Dashed(renderer, x, y, width, height, COLOR_AREA, alpha, &bounds);
                break;
            case KIND_EXTERNAL_SOURCE:
            case KIND_MAP_EXTERNAL_SOURCE:
                Rect(node, layout, &x, &y, &width, &height);
                (void)snprintf(caption, sizeof(caption), "Source %s", Text(node, "SourceReference"));
                Placeholder(renderer, x, y, width, height, caption, alpha, &bounds);
                break;
            case KIND_PICTURE:
                Rect(node, layout, &x, &y, &width, &height);
                (void)snprintf(caption, sizeof(caption), "Picture %s", Text(node, "PictureReference"));
                Placeholder(renderer, x, y, width, height, caption, alpha, &bounds);
                break;
            case KIND_PICTURE_ANIMATED:
                Rect(node, layout, &x, &y, &width, &height);
                (void)snprintf(caption, sizeof(caption), "%u pictures", (unsigned)NumberOr(node, "NumberOfPictures", 0.0f));
                Placeholder(renderer, x, y, width, height, caption, alpha, &bounds);
                break;
            case KIND_SYMBOL:
                x     = NumberOr(node, "PosX", 0.0f);
                y     = NumberOr(node, "PosY", 0.0f);
                angle = (NumberOr(node, "RotationAngle", 0.0f) * RENDERER_DEGREES);
                Marker(renderer, x, y, angle, IndexColor(node, 1.0f), alpha, &bounds);
                (void)snprintf(caption, sizeof(caption), "Symbol %s", Text(node, "SymbolReference"));
                BoxText(renderer, x + RENDERER_MARKER_RADIUS, y - RENDERER_MARKER_RADIUS, RENDERER_CHIP_WIDTH, RENDERER_MARKER_RADIUS, caption, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                break;
            case KIND_SYMBOL_ANIMATED:
                x = NumberOr(node, "PosX", 0.0f);
                y = NumberOr(node, "PosY", 0.0f);
                Marker(renderer, x, y, (NumberAt(node, "OrientationArray", 0, 0.0f) * RENDERER_DEGREES), IndexColor(node, 1.0f), alpha, &bounds);
                (void)snprintf(caption, sizeof(caption), "%u symbols", (unsigned)NumberOr(node, "NumberOfSymbols", 0.0f));
                BoxText(renderer, x + RENDERER_MARKER_RADIUS, y - RENDERER_MARKER_RADIUS, RENDERER_CHIP_WIDTH, RENDERER_MARKER_RADIUS, caption, FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                break;
            case KIND_LABEL:
            case KIND_LABEL_COMPLEX:
                Rect(node, layout, &x, &y, &width, &height);
                angle = (NumberOr(node, "RotationAngle", 0.0f) * RENDERER_DEGREES);
                if (0.0f != angle)
                {
                    FACEOE_Gfx_DrawList_Rotate(renderer->drawList, x, y, angle);
                }
                text = Value(node, "LabelString", 0);
                if (NULL == text)
                {
                    text = "";
                }
                AlignOf(node, "Alignment", &horizontal, &vertical);
                if (renderer->options->showsHidden)
                {
                    Dashed(renderer, x, y, width, height, COLOR_BORDER, alpha * 0.5f, &bounds);
                }
                else
                {
                    ExtendBox(renderer, &bounds, x, y, width, height);
                }
                BoxText(renderer, x, y, width, height, text, horizontal, vertical, 0.0f, IndexColor(node, alpha));
                break;
            case KIND_EDIT_BOX_TEXT:
            case KIND_EDIT_BOX_MASKED:
            case KIND_EDIT_BOX_MULTI_LINE:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, &bounds);
                text = Value(node, "LabelString", 0);
                if (NULL == text)
                {
                    text = "";
                }
                AlignOf(node, "Alignment", &horizontal, &vertical);
                if (KIND_EDIT_BOX_MULTI_LINE == kind)
                {
                    Scrollbar(renderer, node, "VerticalScroll", x, y, width, height, 1.0f, 0.0f, alpha);
                    BoxText(renderer, x + RENDERER_PAD, y + RENDERER_PAD, width - (2.0f * RENDERER_PAD), height - (2.0f * RENDERER_PAD), text, horizontal, FACEOE_GFX_ALIGN_START, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                else
                {
                    BoxText(renderer, x + RENDERER_PAD, y, width - (2.0f * RENDERER_PAD), height, text, horizontal, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                break;
            case KIND_EDIT_BOX_NUMERIC:
            case KIND_EDIT_BOX_NUMERIC_BCD:
                NumericBox(renderer, node, layout, 0.0f, alpha, &bounds);
                break;
            case KIND_NUMERIC_READOUT:
                NumericBox(renderer, node, layout, (NumberOr(node, "RotationAngle", 0.0f) * RENDERER_DEGREES), alpha, &bounds);
                break;
            case KIND_PUSH_BUTTON:
                Button(renderer, node, layout, Value(node, "LabelString", 0), NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_TOGGLE_BUTTON:
            case KIND_PICTURE_TOGGLE_BUTTON:
            case KIND_SYMBOL_TOGGLE_BUTTON:
                // Show the alternate label of a selected toggle with AlternateFlag set
                text = Value(node, "LabelString", 0);
                if (Is(node, "ToggleState", "A661_SELECTED") && Is(node, "AlternateFlag", "A661_TRUE") && (NULL != Value(node, "AlternateLabelString", 0)))
                {
                    text = Value(node, "AlternateLabelString", 0);
                }
                if (KIND_PICTURE_TOGGLE_BUTTON == kind)
                {
                    Button(renderer, node, layout, text, "PictureReference", "PicturePosition", alpha, &bounds, &x, &y, &width, &height);
                }
                else if (KIND_SYMBOL_TOGGLE_BUTTON == kind)
                {
                    Button(renderer, node, layout, text, "SymbolReference", "SymbolPosition", alpha, &bounds, &x, &y, &width, &height);
                }
                else
                {
                    Button(renderer, node, layout, text, NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                }
                if (Is(node, "ToggleState", "A661_SELECTED"))
                {
                    FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, x, y, width, height, Fade(COLOR_ROW_SELECTED, alpha * 0.6f));
                }
                break;
            case KIND_PICTURE_PUSH_BUTTON:
                Button(renderer, node, layout, Value(node, "LabelString", 0), "PictureReference", "PicturePosition", alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_SYMBOL_PUSH_BUTTON:
                Button(renderer, node, layout, Value(node, "LabelString", 0), "SymbolReference", "SymbolPosition", alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_CHECK_BUTTON:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_CONTAINER, COLOR_BORDER, alpha, &bounds);
                box = (height * 0.6f);
                if (width < box)
                {
                    box = width;
                }
                // Put the check box on the side PicturePosition names and the label beside it
                if (Is(node, "PicturePosition", "A661_RIGHT"))
                {
                    area_x = (x + width - box - RENDERER_PAD);
                    BoxText(renderer, x + RENDERER_PAD, y, width - box - (3.0f * RENDERER_PAD), height, Text(node, "LabelString"), FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                else
                {
                    area_x = (x + RENDERER_PAD);
                    BoxText(renderer, x + box + (2.0f * RENDERER_PAD), y, width - box - (3.0f * RENDERER_PAD), height, Text(node, "LabelString"), FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_CENTER, 0.0f, Fade(COLOR_TEXT, alpha));
                }
                FACEOE_Gfx_DrawList_StrokeRectangle(renderer->drawList, area_x, y + ((height - box) * 0.5f), box, box, Fade(COLOR_BUTTON_BORDER, alpha), RENDERER_THIN);
                if (Is(node, "CheckButtonState", "A661_SELECTED"))
                {
                    FACEOE_Gfx_DrawList_FillRectangle(renderer->drawList, area_x + (box * 0.2f), y + ((height - box) * 0.5f) + (box * 0.2f), box * 0.6f, box * 0.6f, Fade(COLOR_BUTTON_BORDER, alpha));
                }
                break;
            case KIND_MULTI_STATE_BUTTON:
                state = (uint32_t)NumberOr(node, "ButtonState", 1.0f);
                text  = NULL;
                if (0 < state)
                {
                    text = Value(node, "LabelStringArray", state - 1);
                }
                Button(renderer, node, layout, text, NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                break;
            case KIND_COMBO_BOX:
            case KIND_COMBO_BOX_EDIT:
            case KIND_SELECTION_LIST_BUTTON:
                selected = (uint32_t)NumberOr(node, "SelectedEntry", 0.0f);
                text     = NULL;
                if (KIND_SELECTION_LIST_BUTTON == kind)
                {
                    text = Value(node, "LabelString", 0);
                }
                else if (0 < selected)
                {
                    text = Value(node, "EntryList", selected - 1);
                }
                Button(renderer, node, layout, text, NULL, NULL, alpha, &bounds, &x, &y, &width, &height);
                // Draw the drop arrow at the right edge
                box = (height * 0.3f);
                FACEOE_Gfx_DrawList_Line(renderer->drawList, x + width - (2.0f * box) - RENDERER_PAD, y + (height * 0.5f) + (box * 0.4f), x + width - box - RENDERER_PAD, y + (height * 0.5f) - (box * 0.4f), Fade(COLOR_TEXT, alpha), RENDERER_THICK);
                FACEOE_Gfx_DrawList_Line(renderer->drawList, x + width - box - RENDERER_PAD, y + (height * 0.5f) - (box * 0.4f), x + width - RENDERER_PAD, y + (height * 0.5f) + (box * 0.4f), Fade(COLOR_TEXT, alpha), RENDERER_THICK);
                if (renderer->options->showsHidden)
                {
                    // Show the closed selecting area dashed, where OpeningMode would open it
                    area_width  = NumberOr(node, "SelectingAreaWidth", width);
                    area_height = NumberOr(node, "SelectingAreaHeight", height * 3.0f);
                    area_x      = x;
                    area_y      = (y - area_height);
                    if (Is(node, "OpeningMode", "A661_OPEN_UP"))
                    {
                        area_y = (y + height);
                    }
                    else if (Is(node, "OpeningMode", "A661_OPEN_CENTERED"))
                    {
                        area_y = (y + (height * 0.5f) - (area_height * 0.5f));
                    }
                    Dashed(renderer, area_x, area_y, area_width, area_height, COLOR_BORDER, alpha * 0.5f, &bounds);
                    EntryRows(renderer, node, "EntryList", NULL, 0, (uint32_t)NumberOr(node, "NumberOfEntries", 0.0f), selected, area_x, area_y, area_width, area_height, FACEOE_GFX_ALIGN_START, alpha * 0.5f);
                }
                break;
            case KIND_SCROLL_LIST:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_EDIT, COLOR_EDIT_BORDER, alpha, &bounds);
                count    = (uint32_t)NumberOr(node, "NumberOfEntries", 0.0f);
                first    = (uint32_t)NumberOr(node, "FirstVisibleEntry", 1.0f);
                selected = (uint32_t)NumberOr(node, "SelectedEntry", 0.0f);
                if (first < 1)
                {
                    first = 1;
                }
                AlignOf(node, "Alignment", &horizontal, &vertical);
                EntryRows(renderer, node, "LabelStringArray", "EnableArray", first - 1, count, selected, x, y, width, height, horizontal, alpha);
                fraction = 1.0f;
                start    = 0.0f;
                if (0 < count)
                {
                    fraction = (0.3f);
                    start    = ((float32_t)(first - 1) / (float32_t)count);
                }
                Scrollbar(renderer, node, "VerticalScroll", x, y, width, height, fraction, start, alpha);
                break;
            case KIND_TREE:
                DrawTree(renderer, node, layout, alpha, &bounds);
                break;
            case KIND_POP_UP_MENU:
                Rect(node, layout, &x, &y, &width, &height);
                Frame(renderer, x, y, width, height, COLOR_POPUP, COLOR_BORDER, alpha, &bounds);
                EntryRows(renderer, node, "StringArray", "EnableArray", 0, (uint32_t)NumberOr(node, "NumberOfEntries", 0.0f), 0, x, y, width, height, FACEOE_GFX_ALIGN_START, alpha);
                break;
            case KIND_POP_UP_MENU_BUTTON:
            case KIND_POP_UP_PANEL_BUTTON:
                Button(renderer, node, layout, Value(node, "LabelString", 0), "PictureReference", "PicturePosition", alpha, &bounds, &x, &y, &width, &height);
                if (renderer->options->showsHidden)
                {
                    // Outline the closed pop-up part where PopupPos places it
                    area_x      = NumberOr(node, "PopupPosX", x);
                    area_y      = NumberOr(node, "PopupPosY", y - height);
                    area_width  = NumberOr(node, "PopupSizeX", width);
                    area_height = NumberOr(node, "PopupSizeY", height);
                    Dashed(renderer, area_x, area_y, area_width, area_height, COLOR_BORDER, alpha * 0.5f, &bounds);
                    if (KIND_POP_UP_MENU_BUTTON == kind)
                    {
                        EntryRows(renderer, node, "StringArray", "EnableArray", 0, (uint32_t)NumberOr(node, "NumberOfEntries", 0.0f), 0, area_x, area_y, area_width, area_height, FACEOE_GFX_ALIGN_START, alpha * 0.5f);
                    }
                }
                break;
            case KIND_POP_UP_PANEL:
                // Keep the panel closed until its button or the UA opens it
                Rect(node, layout, &x, &y, &width, &height);
                if (renderer->options->showsHidden)
                {
                    Dashed(renderer, x, y, width, height, COLOR_BORDER, alpha * 0.6f, &bounds);
                    alpha *= 0.6f;
                }
                else
                {
                    is_drawn = FALSE;
                }
                FACEOE_Gfx_DrawList_Translate(renderer->drawList, x, y);
                break;
            case KIND_SLIDER:
                DrawSlider(renderer, node, layout, alpha, &bounds);
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
                DrawGp(renderer, node, kind, alpha, &bounds);
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
                Chip(renderer, node, alpha, &bounds);
                break;
            case KIND_ANIMATION_ROTATION:
                Marker(renderer, NumberOr(node, "CenterX", 0.0f), NumberOr(node, "CenterY", 0.0f), (NumberOr(node, "ToAngle", 0.0f) * RENDERER_DEGREES), COLOR_MARKER, alpha * 0.6f, &bounds);
                Chip(renderer, node, alpha, &bounds);
                break;
            default:
                // Draw connectors, data scaling, animations, focus, event, buffer, receiver, monitor, proxy, map sources, item lists, and unknown types as chips
                Chip(renderer, node, alpha, &bounds);
                break;
        }
        if (is_drawn)
        {
            DrawChildren(renderer, node, alpha, &children);
        }
        if (has_clip)
        {
            FACEOE_Gfx_DrawList_PopClip(renderer->drawList);
        }
        FACEOE_Gfx_DrawList_PopTransform(renderer->drawList);
        Record(renderer, node, &bounds, alpha);
        (void)value;
        (void)index;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void DrawChildren(Renderer_t* renderer, const FACEOE_Utl_Node_t* node, float32_t alpha, Children_t* children)
{
    const FACEOE_Utl_Node_t* child;
    Layout_t                 layout;
    float32_t                child_alpha;
    float32_t                size;
    bool_t                   is_shown;

    for (child = GetFirstChild(node); NULL != child; child = child->nextSibling)
    {
        if (WidgetOf(child)->isExtension)
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
                if (Ident(child) != children->ident)
                {
                    if (renderer->options->showsHidden)
                    {
                        child_alpha *= RENDERER_HIDDEN_ALPHA;
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
                    size          = NumberOr(child, "SizeX", children->buttonSize);
                    layout.x      = children->offset;
                    layout.y      = children->buttonPosition;
                    layout.width  = size;
                    layout.height = children->buttonSize;
                }
                else
                {
                    size          = NumberOr(child, "SizeY", children->buttonSize);
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
            DrawWidget(renderer, child, child_alpha, &layout);
        }
    }
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Lists the layer's animations once, fills the layer area when asked, then records every
 *  widget placed directly in the layer and, through it, everything below.
 */
void Arinc661_Renderer_DrawLayer(const Arinc661_Scene_t* scene, uint32_t layerId, FACEOE_Gfx_DrawList_t* drawList, Arinc661_Renderer_Options_t* options)
{
    const FACEOE_Utl_Node_t* layer;
    const FACEOE_Utl_Node_t* child;
    Renderer_t               renderer;
    bool_t                   is_active;

    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != options);

    options->boundsCount = 0;
    layer                = Arinc661_Scene_GetLayer(scene, layerId);
    if (NULL != layer)
    {
        is_active = ((const Arinc661_Scene_Node_t*)layer->item)->isActive;
    }
    else
    {
        is_active = FALSE;
    }

    if (is_active)
    {
        (void)memset(&renderer, 0, sizeof(renderer));
        renderer.drawList = drawList;
        renderer.options  = options;
        renderer.base     = (*FACEOE_Gfx_DrawList_Transform(drawList));
        if (0.0f <= options->animationTime)
        {
            // Collect the layer's animation widgets once so targets can be matched cheaply
            CollectAnimations(&renderer, layer);
        }
        Arinc661_Renderer_GetLayerSize(scene, layerId, &renderer.layerWidth, &renderer.layerHeight);
        if (options->fillsLayerArea)
        {
            FACEOE_Gfx_DrawList_FillRectangle(drawList, 0.0f, 0.0f, renderer.layerWidth, renderer.layerHeight, COLOR_LAYER);
            FACEOE_Gfx_DrawList_StrokeRectangle(drawList, 0.0f, 0.0f, renderer.layerWidth, renderer.layerHeight, COLOR_LAYER_BORDER, RENDERER_THIN);
        }
        for (child = GetFirstChild(layer); NULL != child; child = child->nextSibling)
        {
            if (FALSE == WidgetOf(child)->isExtension)
            {
                DrawWidget(&renderer, child, 1.0f, NULL);
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Ties go to the later entry. A widget's bounds follow its children's, so a container wins
 *  a tie with its child, and a later sibling, drawn on top, wins over an earlier one.
 */
const Arinc661_Widget_t* Arinc661_Renderer_Pick(const Arinc661_Renderer_Bounds_t* bounds, uint32_t count, FACEOE_Gfx_Point_t point)
{
    const Arinc661_Widget_t* best;
    float32_t                area;
    float32_t                best_area;
    uint32_t                 index;

    LOGASSERT((NULL != bounds) || (0 == count));
    best      = NULL;
    best_area = 0.0f;
    for (index = 0; index < count; ++index)
    {
        if ((bounds[index].minimum.x <= point.x) && (point.x <= bounds[index].maximum.x) && (bounds[index].minimum.y <= point.y) && (point.y <= bounds[index].maximum.y))
        {
            area = ((bounds[index].maximum.x - bounds[index].minimum.x) * (bounds[index].maximum.y - bounds[index].minimum.y));
            if ((NULL == best) || (area <= best_area))
            {
                best      = bounds[index].widget;
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
void Arinc661_Renderer_FormatNumber(const char_t* format, float64_t value, char_t* text, uint32_t capacity)
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
    integer_length = (uint32_t)FACEOE_Utl_Print_Uint64(integer_digits, sizeof(integer_digits), integer_part);
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
    for (index = 0; '\0' != format[index]; ++index)
    {
        if ((capacity - 1) <= out)
        {
            break;
        }
        symbol = format[index];
        if ('.' == symbol)
        {
            // Emit integer digits that found no slot before the point
            if (0 == integer_slots)
            {
                for (digit_index = 0; digit_index < integer_length; ++digit_index)
                {
                    if ((capacity - 1) <= out)
                    {
                        break;
                    }
                    text[out] = integer_digits[digit_index];
                    out += 1;
                }
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
                    while (digit_index < (integer_length - integer_slots))
                    {
                        if ((capacity - 1) <= out)
                        {
                            break;
                        }
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
        for (digit_index = 0; digit_index < integer_length; ++digit_index)
        {
            if ((capacity - 1) <= out)
            {
                break;
            }
            text[out] = integer_digits[digit_index];
            out += 1;
        }
    }
    text[out] = '\0';
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A missing layer, Width, or Height, or a size below 1, falls back to 100 mm.
 */
void Arinc661_Renderer_GetLayerSize(const Arinc661_Scene_t* scene, uint32_t layerId, float32_t* width, float32_t* height)
{
    const FACEOE_Utl_Node_t*   layer;
    const FACEOE_Utl_Node_t*   entry;
    const Arinc661_Property_t* property;
    float32_t                  number;
    bool_t                     is_width;
    bool_t                     is_height;
    bool_t                     is_size;

    LOGASSERT(NULL != scene);
    LOGASSERT(NULL != width);
    LOGASSERT(NULL != height);

    (*width)  = RENDERER_DEFAULT_EXTENT;
    (*height) = RENDERER_DEFAULT_EXTENT;
    layer     = Arinc661_Scene_GetLayer(scene, layerId);
    if (NULL != layer)
    {
        for (entry = GetFirstProperty(layer); NULL != entry; entry = entry->nextSibling)
        {
            property  = ((const Arinc661_Scene_Node_t*)entry->item)->property;
            is_width  = (0 == strncmp("Width", property->name, sizeof("Width")));
            is_height = (0 == strncmp("Height", property->name, sizeof("Height")));
            is_size   = ScanNumber(property->value, &number) && (1.0f <= number);
            if (is_width && is_size)
            {
                (*width) = number;
            }
            else if (is_height && is_size)
            {
                (*height) = number;
            }
            else
            {
                // Other layer properties do not size the layer
            }
        }
    }
}
