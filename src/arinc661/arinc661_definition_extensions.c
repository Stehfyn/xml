/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, Table 8.4, Possible Widgets for Extension: which widget types each
 * extension may be attached to. Generated from the specification's word positions.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"

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

static const char_t* const c_targets0[] = {"A661_MAPHORZ_ITEMLIST"};
static const char_t* const c_targets1[] = {"A661_MUTUALLY_EXCLUSIVE_CONTAINER", "A661_PAGING_CONTAINER", "A661_TABBED_PANEL_GROUP"};
static const char_t* const c_targets2[] = {"A661_COMBO_BOX", "A661_SCROLL_LIST", "A661_SELECTOR", "A661_TREE"};
static const char_t* const c_targets3[] = {"A661_ACTIVE_AREA", "A661_PICTURE_PUSH_BUTTON", "A661_PUSH_BUTTON", "A661_SYMBOL_PUSH_BUTTON"};
static const char_t* const c_targets4[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_CURSOR_OVER", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_INK_AREA", "A661_KEYBOARD_AREA", "A661_MAPHORZ_ITEMLIST", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SCROLL_WHEEL_AREA", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_targets5[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_targets6[] = {"A661_MAP_BOUNDARY"};
static const char_t* const c_targets7[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_INK_AREA", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_targets8[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_INK_AREA", "A661_MULTI_STATE_BUTTON", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SLIDER", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_targets9[] = {"A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_NUMERIC_READOUT"};
static const char_t* const c_targets10[] = {"A661_MAPHORZ", "A661_MAPHORZ_SOURCE", "A661_MAP_EXTERNAL_SOURCE"};
static const char_t* const c_targets11[] = {"A661_MAPVERT", "A661_MAPVERT_SOURCE", "A661_MAP_EXTERNAL_SOURCE"};
static const char_t* const c_targets12[] = {"A661_SCROLL_LIST", "A661_TREE"};
static const char_t* const c_targets13[] = {"A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_MULTI_STATE_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_targets14[] = {"A661_ACTIVE_AREA", "A661_BASIC_CONTAINER", "A661_BLINKING_CONTAINER", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_CONNECTOR", "A661_CURSOR_OVER", "A661_CURSOR_POS_OVERLAY", "A661_CURSOR_REF", "A661_DATA_CONNECTOR", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_EXTERNAL_SOURCE", "A661_GESTURE_AREA", "A661_GP_ARC_CIRCLE", "A661_GP_ARC_ELLIPSE", "A661_GP_CROWN", "A661_GP_LINE", "A661_GP_LINE_POLAR", "A661_GP_POLYLINE", "A661_GP_RECTANGLE", "A661_GP_TRIANGLE", "A661_INK_AREA", "A661_KEYBOARD_AREA", "A661_LABEL", "A661_LABEL_COMPLEX", "A661_MAPHORZ", "A661_MAPHORZ_CONTAINER", "A661_MAPHORZ_ITEMLIST", "A661_MAPHORZ_PANEL", "A661_MAPHORZ_SOURCE", "A661_MAPVERT", "A661_MAPVERT_CONTAINER", "A661_MAPVERT_ITEMLIST", "A661_MAPVERT_PANEL", "A661_MAPVERT_SOURCE", "A661_MAP_GRID", "A661_MASK_CONTAINER", "A661_MENU_BAR", "A661_MULTI_STATE_BUTTON", "A661_MUTUALLY_EXCLUSIVE_CONTAINER", "A661_NUMERIC_READOUT", "A661_PAGING_CONTAINER", "A661_PANEL", "A661_PICTURE", "A661_PICTURE_ANIMATED", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_RADIO_BOX", "A661_ROTATION_CONTAINER", "A661_SCALE_CONTAINER", "A661_SCROLL_LIST", "A661_SCROLL_PANEL", "A661_SCROLL_WHEEL_AREA", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SHUFFLE_TO_FIT_CONTAINER", "A661_SIZE_TO_FIT_CONTAINER", "A661_SLIDER", "A661_SYMBOL", "A661_SYMBOL_ANIMATED", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TABBED_PANEL_GROUP", "A661_TOGGLE_BUTTON", "A661_TOUCH_AREA", "A661_TRANSLATION_CONTAINER", "A661_TREE"};
static const char_t* const c_targets15[] = {"A661_BUFFER_FORMAT"};
static const char_t* const c_targets16[] = {"A661_ACTIVE_AREA", "A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_CURSOR_OVER", "A661_EDIT_BOX_MASKED", "A661_EDIT_BOX_MULTI_LINE", "A661_EDIT_BOX_NUMERIC", "A661_EDIT_BOX_NUMERIC_BCD", "A661_EDIT_BOX_TEXT", "A661_EXTERNAL_SOURCE", "A661_GESTURE_AREA", "A661_GP_ARC_CIRCLE", "A661_GP_ARC_ELLIPSE", "A661_GP_CROWN", "A661_GP_LINE", "A661_GP_LINE_POLAR", "A661_GP_POLYLINE", "A661_GP_RECTANGLE", "A661_GP_TRIANGLE", "A661_INK_AREA", "A661_LABEL", "A661_LABEL_COMPLEX", "A661_MAPHORZ_PANEL", "A661_MULTI_STATE_BUTTON", "A661_NUMERIC_READOUT", "A661_PAGING_CONTAINER", "A661_PANEL", "A661_PICTURE", "A661_PICTURE_ANIMATED", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SCROLL_PANEL", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_SHUFFLE_TO_FIT_CONTAINER", "A661_SIZE_TO_FIT_CONTAINER", "A661_SLIDER", "A661_SYMBOL", "A661_SYMBOL_ANIMATED", "A661_SYMBOL_PUSH_BUTTON", "A661_SYMBOL_TOGGLE_BUTTON", "A661_TABBED_PANEL", "A661_TABBED_PANEL_GROUP", "A661_TOGGLE_BUTTON", "A661_TOUCH_AREA", "A661_TREE"};
static const char_t* const c_targets17[] = {"A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_MULTI_STATE_BUTTON", "A661_POP_UP_MENU", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_SELECTOR", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};
static const char_t* const c_targets18[] = {"A661_EDIT_BOX_NUMERIC"};
static const char_t* const c_targets19[] = {"A661_CHECK_BUTTON", "A661_COMBO_BOX", "A661_COMBO_BOX_EDIT", "A661_LABEL", "A661_LABEL_COMPLEX", "A661_NUMERIC_READOUT", "A661_PICTURE_PUSH_BUTTON", "A661_PICTURE_TOGGLE_BUTTON", "A661_POP_UP_MENU_BUTTON", "A661_POP_UP_PANEL", "A661_POP_UP_PANEL_BUTTON", "A661_PUSH_BUTTON", "A661_SCROLL_LIST", "A661_SELECTION_LIST_BUTTON", "A661_TABBED_PANEL", "A661_TOGGLE_BUTTON", "A661_TREE"};

static const ExtensionTargets_t c_extensionTargets[] =
{
    {"A661_BOUNDARY_CHECK_EXTENSION", c_targets0, 1U},
    {"A661_CHILD_INDEX_EXTENSION", c_targets1, 3U},
    {"A661_CURSOR_ENTRY_EVENTS_EXTENSION", c_targets2, 4U},
    {"A661_CURSOR_EVENTS_EXTENSION", c_targets3, 4U},
    {"A661_CURSOR_SHAPE_EXTENSION", c_targets4, 29U},
    {"A661_DIRECTIONAL_TABBING_EXTENSION", c_targets5, 24U},
    {"A661_EXCLUDED_REGIONS_EXTENSION", c_targets6, 1U},
    {"A661_FOCUS_STOP_EXTENSION", c_targets7, 25U},
    {"A661_INITIAL_FOCUS_EXTENSION", c_targets8, 25U},
    {"A661_LEGEND_ALIGN_EXTENSION", c_targets9, 3U},
    {"A661_MAPHORZ_COORD_SYSTEM_EVENT_EXTENSION", c_targets10, 3U},
    {"A661_MAPVERT_COORD_SYSTEM_EVENT_EXTENSION", c_targets11, 3U},
    {"A661_MULTI_SELECTION_EXTENSION", c_targets12, 2U},
    {"A661_PICTURE_EXTENSION", c_targets13, 10U},
    {"A661_REORDER_EXTENSION", c_targets14, 78U},
    {"A661_STATIC_PARAM_BUFFER_EXTENSION", c_targets15, 1U},
    {"A661_STYLE_SET_EXTENSION", c_targets16, 53U},
    {"A661_SYMBOL_EXTENSION", c_targets17, 14U},
    {"A661_TICS_ARRAY_EXTENSION", c_targets18, 1U},
    {"A661_WORD_WRAP_EXTENSION", c_targets19, 17U},
};

#define EXTENSION_TARGET_COUNT ((uint32_t)(sizeof(c_extensionTargets) / sizeof(c_extensionTargets[0])))

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  An extension type the table does not list applies nowhere.
 */
bool_t Arinc661_ExtensionApplies(const char_t* extensionType, const char_t* widgetType)
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
const char_t* Arinc661_ExtensionTarget(const char_t* extensionType, uint32_t index)
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
