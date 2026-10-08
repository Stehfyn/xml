/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1, Supplement 7: section 4.5.3.2 (binary Definition File) and
 * section 6 (XML Definition File); widget positions and sizes follow section 3.1.3.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_definition_editor.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "arinc661_context.h"
#include "arinc661_configuration.h"
#include "arinc661_gfx_imgui.h"
#include "arinc661_help.h"
#include "arinc661_icons.h"
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_file.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_scan.h"
}

#include <inttypes.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define WINDOW_NAME              ("ARINC 661 Definition File")
#define HIERARCHY_PANE_NAME      ("Hierarchy")
#define INSPECTOR_PANE_NAME      ("Inspector")
#define SCENE_PANE_NAME          ("Scene")
#define FILE_CONTROLS_HEADER     ("File Controls")
#define FILE_MENU_LABEL          ("File")
#define NEW_MENU_LABEL           ("New")
#define NEW_SHORTCUT_LABEL       ("Ctrl+N")
#define SAVE_MENU_LABEL          ("Save")
#define SAVE_ALL_MENU_LABEL      ("Save All")
#define SAVE_SHORTCUT_LABEL      ("Ctrl+S")
#define CLOSE_MENU_LABEL         ("Close")
#define FOLDER_PATH_LABEL        ("Folder Path")
#define XML_FILES_LABEL          ("Definition Files")
#define XML_EXTENSION            (".xml")
#define BINARY_EXTENSION         (".bin")
#define XML_LIST_MAX_ROWS        (4)
#define FOLDER_INPUT_ID          ("##definitionFolderPath")
#define XML_LIST_ID              ("##definitionFiles")
#define DOCUMENT_TAB_BAR_ID      ("##definitionDocuments")
#define DOCUMENT_TAB_FORMAT      ("%s###%u")
#define NEW_DOCUMENT_TAB_LABEL   ("+")
#define NEW_DOCUMENT_TAB_TOOLTIP ("New document")
#define UNTITLED_DOCUMENT_FORMAT ("Untitled %u.xml")
#define SAVE_AS_POPUP_ID         ("Save Definition File")
#define SAVE_AS_FOLDER_LABEL     ("Folder")
#define SAVE_AS_NAME_LABEL       ("File name")
#define SAVE_AS_FOLDER_INPUT_ID  ("##saveFolderPath")
#define SAVE_AS_NAME_INPUT_ID    ("##saveFileName")
#define SAVE_AS_CONFIRM_LABEL    ("Save")
#define SAVE_AS_CANCEL_LABEL     ("Cancel")
#define CLOSE_DOCUMENT_POPUP_ID  ("Unsaved Changes")
#define CLOSE_DOCUMENT_PROMPT    ("Save changes before closing this document?")
#define SAVE_DOCUMENT_LABEL      ("Save")
#define DISCARD_DOCUMENT_LABEL   ("Discard")
#define CANCEL_CLOSE_LABEL       ("Cancel")
#define PANE_TABLE_ID            ("DefinitionPanes")
#define SCENE_CANVAS_ID          ("Layer canvas")
#define EMPTY_SCENE_LABEL        ("Add a layer to preview the Definition File.")
#define PROPERTY_INPUT_ID        ("##value")
#define PROPERTY_NAME_INPUT_ID   ("##name")
#define PROPERTY_COLUMN          ("Property")
#define VALUE_COLUMN             ("Value")

#define HIERARCHY_PANE_COLUMN_IDX (0)
#define SCENE_PANE_COLUMN_IDX     (1)
#define INSPECTOR_PANE_COLUMN     (2)
#define PANE_COUNT                (3)
#define INVALID_HIERARCHY_INDEX   (UINT32_MAX)
#define INVALID_DOCUMENT_INDEX    (UINT32_MAX)

#define DEFAULT_FOLDER_PATH ("C:\\dev\\xml\\examples")

#define WINDOW_FLAGS               (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar)
#define CLOSE_DOCUMENT_POPUP_FLAGS (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)
#define PANE_TABLE_FLAGS           (ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoHostExtendY)
#define PANE_FLAGS                 (ImGuiChildFlags_AlwaysUseWindowPadding)
#define PROPERTY_TABLE_FLAGS       (ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)
#define NEW_DOCUMENT_TAB_FLAGS     (ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)

#define SAVE_SHORTCUT       (ImGuiMod_Ctrl | ImGuiKey_S)
#define NEW_SHORTCUT        (ImGuiMod_Ctrl | ImGuiKey_N)
#define SAVE_SHORTCUT_FLAGS (ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive)

#define ADD_LAYER_LABEL         ("+ Add Layer")
#define ADD_WIDGET_LABEL        ("+ Add Widget")
#define ADD_CHILD_LABEL         ("+ Add Child Widget")
#define ADD_EXTENSION_LABEL     ("+ Add Extension")
#define ADD_PROPERTY_LABEL      ("+ Add Property")
#define REMOVE_LAYER_LABEL      ("Remove Layer")
#define REMOVE_WIDGET_LABEL     ("Remove Widget")
#define REMOVE_EXTENSION_LABEL  ("Remove Extension")
#define REMOVE_PROPERTY_LABEL   ("Remove Property")
#define READY_LABEL             ("Ready")
#define REMOVE_GLYPH            ("x")

#define DEFINITION_NODE_ID        ("definition")
#define LAYER_NODE_ID             ("layer")
#define WIDGET_NODE_ID            ("widget")
#define DEFINITION_LABEL_FORMAT   ("Definition File %s")
#define LAYER_LABEL_FORMAT        ("Layer %s (%s)")
#define WIDGET_LABEL_FORMAT       ("%s (%s)")
#define EXTENSION_LABEL_FORMAT    ("%s")
#define LAYER_PREVIEW_FORMAT      ("Layer %s (%s x %s)")
#define REFERENCES_SECTION_LABEL  ("Referenced By")
#define ABOUT_FORMAT              ("About %s")
#define CATEGORIES_FORMAT         ("Categories: %s")
#define RESTRICTION_FORMAT        ("Restriction: %s")
#define CHANGE_FORMAT             ("[%s] %s")
#define HELP_WRAP_WIDTH           (420.0f)
#define ADD_POPUP_ID              ("##add widget")
#define ADD_FILTER_HINT           ("Filter widget types")
#define ADD_HERE_FORMAT           ("Add widget at (%d, %d) in %s")
#define ADD_EXTENSION_TO_FORMAT   ("Add extension to %s")
#define DUPLICATE_LABEL           ("Duplicate")
#define REMOVE_LABEL              ("Remove")
#define ADD_CHILD_WIDGET_LABEL    ("Add child widget")
#define ADD_EXTENSION_MENU_LABEL  ("Add extension")
#define SCENE_HINT_LABEL          ("Drag: move\nCorners: resize\nRing above: rotate (Shift snaps)\nArrows: nudge (Shift x10)\nDel: remove\nRight-click: add\nWheel: zoom\nMiddle/Space-drag: pan\nF: fit\nG: grid\nS: snap")
#define SCENE_HINT_TITLE          ("Scene controls")
#define PROPERTY_MENU_ID          ("##property menu")
#define HISTORY_LABEL_BYTES       (96)
#define HISTORY_OPEN_LABEL        ("Opened")
#define HISTORY_ADD_FORMAT        ("Add %s")
#define HISTORY_REMOVE_FORMAT     ("Remove %u widget(s)")
#define HISTORY_LAYER_LABEL       ("Change layers")
#define HISTORY_EDIT_FORMAT       ("Edit %s of %s")
#define HISTORY_EDIT_LAYER_FORMAT ("Edit %s of layer")
#define HISTORY_HEADER_LABEL      ("Edit header")
#define HISTORY_MOVE_LABEL        ("Move widgets")
#define PREFAB_LAYER_NAME         ("Prefab")
#define PREFAB_EXTENSION          (".prefab.xml")
#define REFERENCED_BY_LABEL       ("Referenced By")
#define REFERENCED_BY_FORMAT      ("%s (%s) via %s")
#define RESET_PROPERTY_LABEL      ("Reset to Default")
#define COPY_VALUE_LABEL          ("Copy Value")
#define PASTE_VALUE_LABEL         ("Paste Value")
#define MIXED_LABEL_FORMAT        ("%s (mixed)")
#define SHARED_EDIT_FORMAT        ("Edits apply to all %u selected widgets")
#define ROTATE_TOOLTIP_LABEL      ("Drag to rotate (Shift: 15\xC2\xB0 steps)")
#define SCENE_HINT_WRAP_WIDTH     (360.0f)
#define SCENE_ZOOM_FORMAT         ("%.0f%%")
#define SNAP_UNITS                (100.0f)
#define GRID_UNITS                (100.0f)
#define GRID_MAJOR_EVERY          (10)
#define ZOOM_MIN                  (0.1f)
#define ZOOM_MAX                  (40.0f)
#define ZOOM_WHEEL_FACTOR         (1.15f)
#define SCENE_GRID_COLOR          (IM_COL32(255, 255, 255, 14))
#define SCENE_GRID_MAJOR_COLOR    (IM_COL32(255, 255, 255, 36))
#define DROP_PAYLOAD_ID           ("A661_WIDGET")
#define PALETTE_PAYLOAD_ID        ("A661_PALETTE_TYPE")
#define COPY_SUFFIX               (" copy")
#define NUDGE_STEP                (10.0f)
#define NUDGE_FAST_STEP           (100.0f)
#define HANDLE_HALF_SIZE          (5.0f)
#define DRAG_THRESHOLD            (3.0f)
#define ADD_LIST_HEIGHT           (260.0f)
#define DRAG_NONE                 (0)
#define DRAG_MOVE                 (1)
#define DRAG_BOTTOM_LEFT          (2)
#define DRAG_BOTTOM_RIGHT         (3)
#define DRAG_TOP_LEFT             (4)
#define DRAG_TOP_RIGHT            (5)
#define DRAG_ROTATE               (6)
#define ROTATE_HANDLE_OFFSET      (28.0f)
#define ROTATE_HANDLE_RADIUS      (6.0f)
#define PROPERTY_ROTATION_ANGLE   ("RotationAngle")
#define SCENE_HANDLE_COLOR        (IM_COL32(255, 255, 255, 230))
#define SCENE_HOVER_COLOR         (IM_COL32(255, 255, 255, 120))
#define TOOL_ZOOM_IN_LABEL        ("Zoom in")
#define TOOL_ZOOM_OUT_LABEL       ("Zoom out")
#define TOOL_FIT_LABEL            ("Fit layer")
#define TOOL_GRID_LABEL           ("Millimetre grid")
#define TOOL_SNAP_LABEL           ("Snap to millimetre")
#define TOOL_ADD_LAYER_LABEL      ("Add layer")
#define TOOL_ADD_WIDGET_LABEL     ("Add widget to the selection")
#define TOOL_ADD_EXTENSION_LABEL  ("Add extension to the selected widget")
#define TOOL_DUPLICATE_LABEL      ("Duplicate selection")
#define TOOL_DELETE_LABEL         ("Delete selection")
#define TOOL_ALIGN_LEFT_LABEL     ("Align left edges")
#define TOOL_ALIGN_RIGHT_LABEL    ("Align right edges")
#define TOOL_ALIGN_TOP_LABEL      ("Align top edges")
#define TOOL_ALIGN_BOTTOM_LABEL   ("Align bottom edges")
#define TOOL_ALIGN_CENTER_X_LABEL ("Align horizontal centres")
#define TOOL_ALIGN_CENTER_Y_LABEL ("Align vertical centres")
#define TOOL_DISTRIBUTE_X_LABEL   ("Distribute horizontally")
#define TOOL_DISTRIBUTE_Y_LABEL   ("Distribute vertically")
#define TOOL_ZOOM_SELECTION_LABEL ("Zoom to selection")
#define SHORTCUT_Z                ("Z")
#define CURSOR_READOUT_FORMAT     "x %.0f  y %.0f  (0.01 mm)"
#define SELECTION_COUNT_FORMAT    "%u selected"
#define SCENE_MARQUEE_COLOR       (IM_COL32(97, 221, 245, 40))
#define SCENE_MARQUEE_BORDER      (IM_COL32(97, 221, 245, 200))
#define SCENE_RULER_COLOR         (IM_COL32(255, 255, 255, 90))
#define SCENE_RULER_BACK          (IM_COL32(0, 0, 0, 120))
#define SCENE_RULER_SIZE          (16.0f)
#define SCENE_RULER_FONT_SCALE    (0.70f)
#define RULER_UNITS               (100.0f)
#define SCENE_HINT_LABEL_2        ("Ctrl-click: add to selection\nDrag on empty space: box select\nZ: zoom to selection\nCtrl+C / Ctrl+V: copy and paste")
#define HIERARCHY_FILTER_HINT     ("Filter by name or type")
#define INSPECTOR_FILTER_HINT     ("Filter parameters")
#define RENAME_INPUT_ID           ("##rename")
#define HIDE_LABEL                ("Hide in editor")
#define SHOW_LABEL                ("Show in editor")
#define LOCK_LABEL                ("Lock")
#define UNLOCK_LABEL              ("Unlock")
#define RENAME_LABEL              ("Rename")
#define COPY_LABEL                ("Copy")
#define PASTE_LABEL               ("Paste")
#define EYE_BUTTON_SIZE           (18.0f)
#define STATUS_COPIED_FORMAT      ("Copied %u widgets.")
#define TOOL_LAYER_COMBO_ID       ("##layer")
#define TOOL_LAYER_FORMAT         ("Layer %s: %s")
#define TOOL_NO_LAYER_LABEL       ("No layers")
#define EMPTY_LAYER_BUTTON_LABEL  ("Add the first layer")
#define EMPTY_WIDGET_HINT_LABEL   ("Right-click the layer to add a widget, or use the + buttons above.")
#define ZOOM_STEP_FACTOR          (1.25f)
#define SHORTCUT_WHEEL            ("Wheel")
#define SHORTCUT_F                ("F")
#define SHORTCUT_G                ("G")
#define SHORTCUT_S                ("S")
#define SHORTCUT_CTRL_D           ("Ctrl+D")
#define SHORTCUT_DEL              ("Del")
#define SHORTCUT_CTRL_C           ("Ctrl+C")
#define SHORTCUT_CTRL_V           ("Ctrl+V")
#define SHORTCUT_F2               ("F2")
#define SCENE_HINT_COLOR          (IM_COL32(180, 190, 200, 200))
#define REFERENCE_FORMAT          ("%s: application %lld, window %lld")
#define SHOW_USER_APPLICATION_LABEL ("Show")
#define ADOPT_APPLICATION_ID_LABEL ("Use its applicationId")
#define APPLICATION_ID_MISMATCH_TOOLTIP ("ApplicationId differs from the user application's applicationId (ARINC 661 section 6.5.1.2, FACE H.3.1).")
#define NO_REFERENCES_LABEL       ("No user application names this file; set a dFPath in a configuration, or create one here.")
#define CREATE_APPLICATION_LABEL  ("Create user application")
#define CONFIGURATION_COMBO_ID    ("##configuration")
#define WINDOW_COMBO_ID           ("##window")
#define NO_CONFIGURATION_LABEL    ("Open or create a configuration first.")
#define WINDOW_CHOICE_FORMAT      ("Window %lld (%s)")
#define FRAME_LABEL_BYTES         (160)
#define SCENE_FRAME_COLOR         (IM_COL32(255, 200, 80, 200))
#define SCENE_FRAME_DASH          (6.0f)
#define REFERENCE_NONE_LABEL      ("0 (none)")
#define REFERENCE_WIDGET_FORMAT   ("%lld  %s (%s)")
#define REFERENCE_LAYER_FORMAT    ("%s  %s")
#define REFERENCE_APPLICATION_FORMAT ("%lld  %s (%s)")
#define REFERENCE_SOURCE_FORMAT   ("%u  %s %s")
#define REFERENCE_MISSING_FORMAT  ("%s  (no match)")
#define REFERENCE_GO_LABEL        ("Go")
#define REFERENCE_COMBO_ID        ("##reference")
#define REFERENCE_TOOLTIP_WIDGET  ("WidgetIdent of a widget in the same layer; 0 means none.")
#define REFERENCE_TOOLTIP_LAYER   ("LayerId of a layer of this Definition File.")
#define REFERENCE_TOOLTIP_APPLICATION ("ApplicationId of another user application in the configuration.")
#define REFERENCE_TOOLTIP_FOCUS_IN ("WidgetIdent of a FocusIn widget in the layer named by NextFocusInLayerId of the application named by NextFocusInAppId.")
#define REFERENCE_TOOLTIP_SOURCE  ("id of an externalSource in the configuration (FACE H.3.7).")
#define PARENT_LAYER_NAME         ("LAYER")
#define PARENT_ANY_NAME           ("")
#define REFERENCE_TEXT_BYTES      (320)
#define EXTENSION_NOT_APPLICABLE_FORMAT ("%s may not be attached to %s (Part 1-7, Table 8.4).")

#define DEFINITION_TABLE_ID  ("DefinitionProperties")
#define LAYER_TABLE_ID       ("LayerProperties")
#define WIDGET_TABLE_ID      ("WidgetProperties")
#define MODEL_TABLE_ID       ("ModelProperties")
#define OTHER_TABLE_ID       ("OtherProperties")

#define DEFINITION_SECTION_LABEL ("Definition File")
#define LAYER_SECTION_LABEL      ("Layer")
#define WIDGET_SECTION_LABEL     ("Widget")
#define EXTENSION_SECTION_LABEL  ("Extension")
#define MODEL_SECTION_LABEL      ("Model")
#define OTHER_SECTION_LABEL      ("Other Properties")
#define UNKNOWN_TYPE_LABEL       ("(unknown type)")

#define NAME_LABEL               ("Name")
#define TYPE_LABEL               ("Type")
#define LIBRARY_VERSION_LABEL    ("Library Version")
#define SUPPLEMENT_VERSION_LABEL ("Supplement Version")
#define CHARSET_ENCODING_LABEL   ("Charset Encoding")
#define PARENT_LABEL             ("Parent")
#define SELECT_ITEM_PROMPT       ("Select the Definition File, a layer, a widget, or an extension.")

#define PROPERTY_APPLICATION_ID ("ApplicationId")
#define PROPERTY_LAYER_ID       ("LayerId")
#define PROPERTY_CONTEXT_NUMBER ("ContextNumber")
#define PROPERTY_WIDTH          ("Width")
#define PROPERTY_HEIGHT         ("Height")
#define PROPERTY_WIDGET_IDENT   ("WidgetIdent")
#define PROPERTY_POS_X          ("PosX")
#define PROPERTY_POS_Y          ("PosY")
#define PROPERTY_SIZE_X         ("SizeX")
#define PROPERTY_SIZE_Y         ("SizeY")
#define PROPERTY_VISIBLE        ("Visible")
#define PROPERTY_ENABLE         ("Enable")
#define DEFAULT_WIDGET_TYPE     ("A661_PANEL")
#define DEFAULT_EXTENSION_TYPE  ("A661_LEGEND_ALIGN_EXTENSION")
#define DEFAULT_BOOLEAN_TRUE    ("A661_TRUE")
#define DEFAULT_LAYER_SIZE      ("10000")
#define DEFAULT_SUPPLEMENT      (7)
#define DEFAULT_LAYER_EXTENT    (10000.0f)
#define NO_CHARSET_LABEL        ("(none)")
#define ARINC661_DEFINITION_MAX_SEED_PROPERTIES (40) ///< Largest Creation Structure parameter count seeded on insertion

#define INTEGER_TEXT_BYTES       (32)
#define LABEL_FORMAT_BYTES       (64)
#define HIERARCHY_LABEL_BYTES    ((ARINC661_MAX_NAME_BYTES * 2) + LABEL_FORMAT_BYTES)
#define DOCUMENT_TAB_LABEL_BYTES (ARINC661_DEFINITION_MAX_XML_NAME_BYTES + 16)
#define SAVE_DIALOG_INPUT_WIDTH  (420.0f)
#define BINARY_MAGIC_HIGH        (0xA6)
#define BINARY_MAGIC_LOW         (0x61)
#define DEFINITION_ROOT_ELEMENT  ("<a661_df")

#define HIERARCHY_BRANCH_FLAGS (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap)
#define HIERARCHY_LEAF_FLAGS   (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap)
#define HIERARCHY_ADD_FLAGS    (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen)

#define SCENE_CANVAS_COLOR            (IM_COL32(22, 27, 34, 255))
#define SCENE_LAYER_COLOR             (IM_COL32(42, 50, 64, 255))
#define SCENE_LAYER_BORDER_COLOR      (IM_COL32(220, 230, 245, 255))
#define SCENE_WIDGET_COLOR            (IM_COL32(42, 154, 188, 80))
#define SCENE_WIDGET_BORDER_COLOR     (IM_COL32(97, 221, 245, 255))
#define SCENE_TEXT_COLOR              (IM_COL32(240, 248, 255, 255))
#define SCENE_SELECTION_OVERLAY_COLOR (IM_COL32(255, 255, 255, 40))
#define INVALID_VALUE_COLOR           (IM_COL32(255, 96, 96, 255))
#define INVALID_VALUE_TOOLTIP         ("This value cannot be written to the binary Definition File.")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Message shown in the editor status bar.
 */
typedef enum Status_e
{
    STATUS_NONE,
    STATUS_FOLDER_PATH_REQUIRED,
    STATUS_XML_FILE_REQUIRED,
    STATUS_SERIALIZATION_FAILED,
    STATUS_SAVED,
    STATUS_ALL_SAVED,
    STATUS_SAVE_ALL_FAILED,
    STATUS_NEW_DOCUMENT,
    STATUS_SAVE_NAME_REQUIRED,
    STATUS_SAVE_NAME_INVALID,
    STATUS_SAVE_FILE_EXISTS,
    STATUS_SAVE_FOLDER_INVALID,
    STATUS_SAVE_ALL_CANCELED,
    STATUS_WRITE_FAILED,
    STATUS_LAYER_ADDED,
    STATUS_LAYER_ADD_FAILED,
    STATUS_WIDGET_ADDED,
    STATUS_WIDGET_ADD_FAILED,
    STATUS_EXTENSION_ADDED,
    STATUS_PROPERTY_ADDED,
    STATUS_PROPERTY_ADD_FAILED,
    STATUS_LAYER_REMOVED,
    STATUS_WIDGET_REMOVED,
    STATUS_EXTENSION_REMOVED,
    STATUS_PROPERTY_REMOVED,
    STATUS_READ_FAILED,
    STATUS_INPUT_TOO_LARGE,
    STATUS_INPUT_EMPTY,
    STATUS_LOADED,
    STATUS_INVALID_XML,
    STATUS_INVALID_BINARY,
    STATUS_TOO_MANY_DOCUMENTS
} Status_t;

/*************************************************************************************************/
/**
 * \brief Kind of entity selected in the Definition File hierarchy.
 */
typedef enum SelectionKind_e
{
    SELECTION_NONE,
    SELECTION_DEFINITION,
    SELECTION_LAYER,
    SELECTION_WIDGET
} SelectionKind_t;

/*************************************************************************************************/
/**
 * \brief State shared with each filename shown in the list box.
 */
typedef struct XmlFileListContext_s
{
    Arinc661_Definition_Editor_t* editor;
    uint32_t                      xmlFileCount;
    bool_t                        shouldDraw;
} XmlFileListContext_t;

/*************************************************************************************************/
/**
 * \brief Filename sought while checking a new document's save destination.
 */
typedef struct FileMatchContext_s
{
    const char_t* fileName;
    bool_t        isFound;
} FileMatchContext_t;

/*************************************************************************************************/
/**
 * \brief Addition or removal requested while drawing the hierarchy, applied after traversal.
 */
typedef struct HierarchyRequest_s
{
    uint32_t removeLayerIndex;        ///< Layer to remove, or INVALID_HIERARCHY_INDEX
    uint32_t removeWidgetIndex;       ///< Widget or extension to remove, or INVALID_HIERARCHY_INDEX
    uint32_t addWidgetLayerIndex;     ///< Layer receiving a new widget, or INVALID_HIERARCHY_INDEX
    uint32_t addWidgetParentIndex;    ///< Parent of the new widget, or INVALID_HIERARCHY_INDEX for a layer child
    uint32_t addExtensionParentIndex; ///< Widget receiving a new extension, or INVALID_HIERARCHY_INDEX
    uint32_t duplicateWidgetIndex;    ///< Widget to duplicate with its subtree, or INVALID_HIERARCHY_INDEX
} HierarchyRequest_t;

/** What an identifier parameter refers to. */
typedef enum ReferenceKind_e
{
    REFERENCE_NONE,        ///< Plain value
    REFERENCE_WIDGET,      ///< WidgetIdent of a widget in the same layer
    REFERENCE_LAYER,       ///< LayerId of a layer of this file
    REFERENCE_APPLICATION, ///< ApplicationId of a user application
    REFERENCE_FOCUS_IN,    ///< WidgetIdent of a FocusIn widget in another application's layer
    REFERENCE_SOURCE       ///< id of a configuration external source
} ReferenceKind_t;

/** Parameter name to reference kind. */
typedef struct ReferenceParameter_s
{
    const char_t*   name; ///< Parameter name
    ReferenceKind_t kind; ///< What it refers to
} ReferenceParameter_t;

/** Where a widget type may sit (Part 1-7, section 3.2.3.1 and the Restriction clauses). */
typedef struct ParentRule_s
{
    const char_t* child;       ///< Widget type
    const char_t* parents[4];  ///< Allowed parent types; PARENT_LAYER_NAME for a direct layer child; NULL ends the list
    bool_t        isForbidden; ///< TRUE when the listed parents are forbidden instead of allowed
} ParentRule_t;

static const ReferenceParameter_t c_referenceParameters[] =
{
    {"NextFocusedWidget", REFERENCE_WIDGET},      {"TargetWidgetID", REFERENCE_WIDGET},       {"TargetWidgetIdent", REFERENCE_WIDGET},
    {"ActiveTabbedPanelID", REFERENCE_WIDGET},    {"VisibleChild", REFERENCE_WIDGET},         {"ShowIfFailIdent", REFERENCE_WIDGET},
    {"ShowNoServiceIdent", REFERENCE_WIDGET},     {"TriggerWidgetIdent", REFERENCE_WIDGET},   {"TabDirectionWidgetIdArray", REFERENCE_WIDGET},
    {"TriggerLayerIdent", REFERENCE_LAYER},       {"NextFocusInLayerId", REFERENCE_LAYER},    {"ConnectorReference", REFERENCE_LAYER},
    {"NextLayerConnectorRef", REFERENCE_LAYER},   {"NextFocusInAppId", REFERENCE_APPLICATION}, {"NextFocusInId", REFERENCE_FOCUS_IN},
    {"SourceReference", REFERENCE_SOURCE}
};

static const ParentRule_t c_parentRules[] =
{
    {"A661_BROADCAST_RECEIVER", {PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_BUFFER_FORMAT", {PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_DATA_SCALING_FR180", {PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_DATA_SCALING_LONG", {PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_DATA_SCALING_ULONG", {PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_ANIMATION_ONPARAM", {"A661_ANIMATION_GROUP", NULL}, FALSE},
    {"A661_ANIMATION_ROTATION", {"A661_ANIMATION_GROUP", NULL}, FALSE},
    {"A661_ANIMATION_SCALE", {"A661_ANIMATION_GROUP", NULL}, FALSE},
    {"A661_ANIMATION_TRANSLATION", {"A661_ANIMATION_GROUP", NULL}, FALSE},
    {"A661_MAPHORZ_CONTAINER", {"A661_MAPHORZ_SOURCE", "A661_MAPHORZ_CONTAINER", NULL}, FALSE},
    {"A661_MAPHORZ_ITEMLIST", {"A661_MAPHORZ_SOURCE", "A661_MAPHORZ_CONTAINER", NULL}, FALSE},
    {"A661_MAPHORZ_PANEL", {"A661_MAPHORZ_SOURCE", "A661_MAPHORZ_CONTAINER", NULL}, FALSE},
    {"A661_MAPHORZ_VERTEXBUFFER", {"A661_MAPHORZ_SOURCE", "A661_MAPHORZ_CONTAINER", NULL}, FALSE},
    {"A661_MAPHORZ_SOURCE", {"A661_MAPHORZ", "A661_MASK_CONTAINER", PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_MAPVERT_CONTAINER", {"A661_MAPVERT_SOURCE", "A661_MAPVERT_CONTAINER", NULL}, FALSE},
    {"A661_MAPVERT_ITEMLIST", {"A661_MAPVERT_SOURCE", "A661_MAPVERT_CONTAINER", NULL}, FALSE},
    {"A661_MAPVERT_PANEL", {"A661_MAPVERT_SOURCE", "A661_MAPVERT_CONTAINER", NULL}, FALSE},
    {"A661_MAPVERT_SOURCE", {"A661_MAPVERT", "A661_MASK_CONTAINER", PARENT_LAYER_NAME, NULL}, FALSE},
    {"A661_MAP_BOUNDARY", {"A661_MAPHORZ", NULL}, FALSE},
    {"A661_MAP_GRID", {"A661_MAPHORZ_SOURCE", "A661_MAPVERT_SOURCE", "A661_MAPHORZ_CONTAINER", "A661_MAPVERT_CONTAINER"}, FALSE},
    {"A661_TABBED_PANEL", {"A661_TABBED_PANEL_GROUP", NULL}, FALSE},
    {"A661_MULTI_STATE_BUTTON", {"A661_RADIO_BOX", NULL}, TRUE}
};

/*************************************************************************************************/
/**
 * \brief Model snapshot header; the layer, widget, and property arrays follow it in memory.
 */
typedef struct Snapshot_s
{
    uint32_t layerCount;                    ///< Layers stored
    uint32_t widgetCount;                   ///< Widgets stored
    uint32_t propertyCount;                 ///< Properties stored
    int32_t  selectedKind;                  ///< Selection at the time
    uint32_t selectedIndex;                 ///< Selection index at the time
    uint32_t libraryVersion;                ///< Definition File library version
    uint32_t supplementVersion;             ///< Definition File supplement version
    uint32_t charsetEncoding;               ///< Definition File charset encoding
    char_t   name[ARINC661_MAX_NAME_BYTES]; ///< Definition File name
    char_t   label[HISTORY_LABEL_BYTES];    ///< What changed to reach this state, for the history list
} Snapshot_t;

/*************************************************************************************************/
/**
 * \brief Widget rectangle in layer units, resolved through its container chain.
 */
typedef struct WidgetArea_s
{
    float32_t x;       ///< Left edge in layer units, measured from the layer's left edge
    float32_t y;       ///< Bottom edge in layer units, measured from the layer's bottom edge
    float32_t width;   ///< Width in layer units
    float32_t height;  ///< Height in layer units
    bool_t    isValid; ///< TRUE when the widget and all its containers carry position and size properties
} WidgetArea_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Charset encoding names in A661_Charset_Encoding order, with no encoding first.
 */
static const char_t* const c_charsetNames[] =
{
    NO_CHARSET_LABEL,
    "A661_ASCII_EXTENDED",
    "A661_UTF8",
    "A661_GBK"
};

/*************************************************************************************************/
/**
 * \brief Digits of a hexadecimal property value; one array, so digit pointers can be subtracted.
 */
static const char_t c_hexDigits[] = "0123456789ABCDEF";

/*************************************************************************************************/
/**
 * \brief Parameter descriptions for the layer and Definition File values the binary header carries.
 */
static const Arinc661_Parameter_t c_ucharParameter  = {NULL, ARINC661_PARAMETER_UCHAR,  0, 0, NULL, NULL, 0};
static const Arinc661_Parameter_t c_ushortParameter = {NULL, ARINC661_PARAMETER_USHORT, 0, 0, NULL, NULL, 0};
static const Arinc661_Parameter_t c_ulongParameter  = {NULL, ARINC661_PARAMETER_ULONG,  0, 0, NULL, NULL, 0};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/
/*************************************************************************************************/
/**
 * \brief Keeps all status bar messages in one place.
 *
 * \param status   Status code
 *
 * \retval const char_t*   Zero-terminated status text.
 */
static const char_t* EditorStatusToString(int32_t status);

/*************************************************************************************************/
/**
 * \brief Joins a folder and filename with normalized separators.
 *
 * \param[in]  folderPath   Nonempty folder path
 * \param[in]  fileName     Filename or search pattern
 * \param[out] filePath     Combined path
 * \param[in]  capacity     File path buffer size
 *
 * \retval     None
 */
static void JoinFolderPath(const char_t* folderPath, const char_t* fileName, char_t* filePath, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Resolves a clicked XML filename to a full path.
 *
 * \param[in,out] editor     Editor state and status
 * \param[in]     fileName   Clicked XML filename
 * \param[out]    filePath   Combined path when inputs are valid
 * \param[in]     capacity   File path buffer size
 *
 * \retval       TRUE       if a folder and XML filename are present.
 * \retval       FALSE      if either is missing.
 */
static bool_t GetXmlFilePath(Arinc661_Definition_Editor_t* editor, const char_t* fileName, char_t* filePath, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Compares the end of the filename with the extension, ignoring case.
 *
 * \param fileName    File name inside the folder
 * \param extension   File extension including the dot
 *
 * \retval TRUE   if the name ends with the extension, ignoring case.
 * \retval FALSE  otherwise.
 */
static bool_t HasExtension(const char_t* fileName, const char_t* extension);

/*************************************************************************************************/
/**
 * \brief Keeps a new file inside its selected folder and requires the XML or binary extension.
 *
 * \param fileName   File name inside the folder
 *
 * \retval TRUE   if the name is a plain file name with an accepted extension.
 * \retval FALSE  otherwise.
 */
static bool_t IsValidXmlFileName(const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Clears every count and selection while keeping the storage pointers and capacities.
 *
 * \param document   Document being edited
 *
 * \retval None
 */
static void ResetDocument(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Preserves an open document's edits when the selected path matches its stored path.
 *
 * \param editor     Editor state
 * \param fileName   File name inside the folder
 *
 * \retval None
 */
static void LoadDefinitionFile(Arinc661_Definition_Editor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Starts an empty, dirty document in one initialized slot.
 *
 * \param editor   Editor state
 *
 * \retval None
 */
static void CreateDocument(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Writes the binary form for a .bin path and the XML form otherwise, so Save As can * convert between the two.
 *
 * \param editor   Editor state
 *
 * \retval None
 */
static void SaveLoadedDocument(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves a named document or requests a filename for an untitled document.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot to save
 *
 * \retval        None
 */
static void SaveDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Saves every open document with unsaved changes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void SaveAllDocuments(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Checks one enumerated filename against a proposed save name.
 *
 * \param[in] fileName   Enumerated filename
 * \param[in] context    Proposed filename and match state
 *
 * \retval    None
 */
static void FindMatchingFile(const char_t* fileName, void* context);

/*************************************************************************************************/
/**
 * \brief Returns the first property of one model with the given name.
 *
 * \param document     Document being edited
 * \param owner        Owning model kind
 * \param ownerIndex   Layer or widget array index; zero for the Definition File
 * \param name         Property name
 *
 * \retval uint32_t   Property array index, or INVALID_HIERARCHY_INDEX.
 */
static uint32_t FindOwnedProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a numeric property such as PosX; the scan must consume the whole value.
 *
 * \param document     Document being edited
 * \param owner        Owning model kind
 * \param ownerIndex   Layer or widget array index; zero for the Definition File
 * \param name         Property name
 * \param value        Value edited in place
 *
 * \retval TRUE   if the property exists and is numeric.
 * \retval FALSE  otherwise.
 */
static bool_t GetPropertyNumber(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, float32_t* value);

/*************************************************************************************************/
/**
 * \brief Appends at the end of the property array, which keeps the order of each model's * existing properties.
 *
 * \param document     Document being edited
 * \param owner        Owning model kind
 * \param ownerIndex   Layer or widget array index; zero for the Definition File
 * \param name         Property name
 * \param value        Value edited in place
 *
 * \retval TRUE   if the property was added.
 * \retval FALSE  when the property array is full.
 */
static bool_t AppendProperty(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Closes the gap left by the removed property.
 *
 * \param document        Document being edited
 * \param propertyIndex   Property array index
 *
 * \retval None
 */
static void RemovePropertyAt(Arinc661_Definition_Document_t* document, uint32_t propertyIndex);

/*************************************************************************************************/
/**
 * \brief Returns the property at the given position among one model's properties of that name, * which is the array element index for array parameters.
 *
 * \param document     Document being edited
 * \param owner        Owning model kind
 * \param ownerIndex   Layer or widget array index; zero for the Definition File
 * \param name         Property name
 * \param occurrence   Index among same-named properties
 *
 * \retval uint32_t   Property array index, or INVALID_HIERARCHY_INDEX.
 */
static uint32_t FindOwnedPropertyAt(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, uint32_t occurrence);

/*************************************************************************************************/
/**
 * \brief Chooses the value a new property of this parameter starts with: A661_TRUE when the * table lists it, otherwise the first listed constant, zero for numbers, an empty string * for text, and zero digits for fixed bytes.
 *
 * \param parameter   Creation Structure parameter
 * \param value       Value edited in place
 * \param capacity    Bytes available in value
 *
 * \retval None
 */
static void DefaultParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Counts uppercase hexadecimal digits; a value with any other character, or an odd * count, is not a byte string.
 *
 * \param value   Value edited in place
 *
 * \retval uint32_t   Number of bytes the hexadecimal text encodes, or INVALID_HIERARCHY_INDEX when malformed.
 */
static uint32_t HexByteCount(const char_t* value);

/*************************************************************************************************/
/**
 * \brief Applies the same acceptance rules as the binary writer: an integer parameter takes a * decimal number in its range or a section 4.6 constant name whose value fits; a decimal * parameter takes a number within its range; byte parameters take hexadecimal digits of * the right length; strings take anything.
 *
 * \param parameter   Creation Structure parameter
 * \param value       Value edited in place
 *
 * \retval TRUE   if the binary writer would accept the value.
 * \retval FALSE  otherwise.
 */
static bool_t IsParameterValueValid(const Arinc661_Parameter_t* parameter, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Makes a widget's properties match its type's Creation Structure: properties that name * no parameter are dropped (WidgetIdent is kept on widgets), every missing single * parameter is added with its default, a kept value that no longer fits its parameter * kind is reseeded, and every array is sized to its count.
 *
 * \param document      Document being edited
 * \param widgetIndex   Widget array index
 *
 * \retval TRUE   if any property was added, removed, or reseeded.
 * \retval FALSE  otherwise.
 */
static bool_t ConformParameters(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Follows parent links from the candidate toward its layer.
 *
 * \param document        Document being edited
 * \param widgetIndex     Widget array index
 * \param ancestorIndex   Candidate ancestor widget index
 *
 * \retval TRUE   if the widget sits under the ancestor.
 * \retval FALSE  otherwise.
 */
static bool_t IsDescendant(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, uint32_t ancestorIndex);

/*************************************************************************************************/
/**
 * \brief Returns the array index following the last descendant of a widget, which is where a * new child keeps parents before children and extensions directly after their widget.
 *
 * \param document      Document being edited
 * \param widgetIndex   Widget array index
 *
 * \retval uint32_t   Index one past the last descendant.
 */
static uint32_t SubtreeEnd(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Opens a widget slot at insertIndex and renumbers every parent link and property * owner at or beyond it.
 *
 * \param document      Document being edited
 * \param insertIndex   Index the new widget takes
 *
 * \retval TRUE   if a slot was opened.
 * \retval FALSE  when the widget array is full.
 */
static bool_t InsertWidgetAt(Arinc661_Definition_Document_t* document, uint32_t insertIndex);

/*************************************************************************************************/
/**
 * \brief Returns one more than the largest WidgetIdent in the layer, starting at one.
 *
 * \param document     Document being edited
 * \param layerIndex   Layer array index
 *
 * \retval int64_t   Lowest unused WidgetIdent in the layer.
 */
static int64_t NextWidgetIdent(const Arinc661_Definition_Document_t* document, uint32_t layerIndex);

/*************************************************************************************************/
/**
 * \brief A new layer takes the next LayerId and the default 10000 by 10000 extent.
 *
 * \param document   Document being edited
 *
 * \retval None
 */
static void AddLayer(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief A widget goes after its parent's subtree, or at the end of the array for a direct * layer child; an extension goes directly after its widget and any earlier extensions, * as the binary form requires.
 *
 * \param document      Document being edited
 * \param layerIndex    Layer array index
 * \param parentIndex   Parent widget index, or INVALID_HIERARCHY_INDEX for a layer child
 * \param isExtension   TRUE for an extension type
 *
 * \retval None
 */
static void AddWidget(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, bool_t isExtension);

/*************************************************************************************************/
/**
 * \brief Removes a widget with its descendants and extensions, drops their properties, and * renumbers the surviving parent links and property owners.
 *
 * \param document      Document being edited
 * \param widgetIndex   Widget array index
 *
 * \retval None
 */
static void RemoveWidgetSubtree(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Removes the layer's widgets first, then the layer and its properties, then renumbers * later layers.
 *
 * \param document     Document being edited
 * \param layerIndex   Layer array index
 *
 * \retval None
 */
static void RemoveLayer(Arinc661_Definition_Document_t* document, uint32_t layerIndex);

/*************************************************************************************************/
/**
 * \brief Draws an X remove control beside the preceding tree row.
 *
 * \param[in] label       Button identifier and tooltip text
 * \param[in] rowHeight   Height of the tree row
 *
 * \retval    TRUE        if the remove control was clicked.
 * \retval    FALSE       if the remove control was not clicked.
 */
static bool_t DrawRemoveButton(const char_t* label, float32_t rowHeight);

/*************************************************************************************************/
/**
 * \brief Draws the widgets of one parent and recurses into each widget's children and * extensions.
 *
 * \param document      Document being edited
 * \param layerIndex    Layer array index
 * \param parentIndex   Parent widget index, or INVALID_HIERARCHY_INDEX for a layer child
 * \param request       Request recorded during traversal
 *
 * \retval None
 */
static void DrawWidgetNodes(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, HierarchyRequest_t* request);

/*************************************************************************************************/
/**
 * \brief Shows the Definition File root, its layers, and each layer's widget tree, then applies * the one request recorded during traversal.
 *
 * \param document   Document being edited
 *
 * \retval None
 */
static void DrawHierarchy(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief All inspector groups use the same property and value columns.
 *
 * \param name   Property name
 *
 * \retval TRUE   if the table is open and rows may follow.
 * \retval FALSE  otherwise.
 */
static bool_t BeginPropertyTable(const char_t* name);

/*************************************************************************************************/
/**
 * \brief Draws one short text value in the two-column inspector.
 *
 * \param[in]     label      Visible field label
 * \param[in,out] value      Terminated text storage
 * \param[in]     capacity   Number of text bytes
 *
 * \retval        TRUE        if the value changed.
 * \retval        FALSE       if the value did not change.
 */
static bool_t DrawTextField(const char_t* label, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Edits an unsigned 32-bit value in a property row.
 *
 * \param label   Row label
 * \param value   Value edited in place
 *
 * \retval TRUE   if the value changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawUnsignedField(const char_t* label, uint32_t* value);

/*************************************************************************************************/
/**
 * \brief Offers no encoding and the three Table 4.6-13 encodings; the index is the value.
 *
 * \param label   Row label
 * \param value   Value edited in place
 *
 * \retval TRUE   if the value changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawCharsetField(const char_t* label, uint32_t* value);

/*************************************************************************************************/
/**
 * \brief Records an inspector edit as an unsaved document change.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     hasChanged   TRUE if a field changed
 *
 * \retval        None
 */
static void MarkDocumentDirty(Arinc661_Definition_Document_t* editor, bool_t hasChanged);

/*************************************************************************************************/
/**
 * \brief Tells whether a property is edited by a typed row elsewhere in the inspector, so the * generic table only shows what the schema does not cover.
 *
 * \param document     Document being edited
 * \param owner        Owning model kind
 * \param ownerIndex   Layer or widget array index; zero for the Definition File
 * \param name         Property name
 *
 * \retval TRUE   if the Creation Structure names the property.
 * \retval FALSE  otherwise.
 */
static bool_t IsSchemaProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Edits one value in place according to the parameter kind: a combo over the listed * constants, a ranged integer, a decimal, or text.
 *
 * \param parameter   Creation Structure parameter
 * \param value       Value edited in place
 * \param capacity    Bytes available in value
 *
 * \retval TRUE   if the value changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Draws one labeled row for a parameter value held by a property, or by a scratch buffer * when the property does not exist yet; an edit of a missing value creates the property.
 *
 * \param document       Document being edited
 * \param owner          Owning model kind
 * \param ownerIndex     Layer or widget array index; zero for the Definition File
 * \param label          Row label
 * \param propertyName   Property name
 * \param occurrence     Index among same-named properties
 * \param parameter      Creation Structure parameter
 *
 * \retval TRUE   if the value changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawParameterRow(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* label, const char_t* propertyName, uint32_t occurrence, const Arinc661_Parameter_t* parameter);

/*************************************************************************************************/
/**
 * \brief Grows or shrinks an array's properties to its count parameter times the elements per * entry, so the binary form and the rows shown always agree.
 *
 * \param document      Document being edited
 * \param widgetIndex   Widget array index
 * \param parameter     Creation Structure parameter
 *
 * \retval TRUE   if properties were added or removed.
 * \retval FALSE  otherwise.
 */
static bool_t SyncArrayLength(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, const Arinc661_Parameter_t* parameter);

/*************************************************************************************************/
/**
 * \brief Draws every Creation Structure parameter of the widget's type: single values as one * row each, arrays as one row per element after syncing their length to the count.
 *
 * \param document      Document being edited
 * \param widgetIndex   Widget array index
 *
 * \retval None
 */
static void DrawParameterTable(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Offers every widget type, or every extension type, known to the binary form.
 *
 * \param label         Row label
 * \param type          Type name edited in place
 * \param capacity      Bytes available in value
 * \param isExtension   TRUE for an extension type
 *
 * \retval TRUE   if the type changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawTypeField(const char_t* label, char_t* type, uint32_t capacity, bool_t isExtension, const char_t* parentType);

/*************************************************************************************************/
/**
 * \brief Lists one model's properties as editable name and value rows, each with a remove * button, followed by an add row.
 *
 * \param document     Document being edited
 * \param owner        Owning model kind
 * \param ownerIndex   Layer or widget array index; zero for the Definition File
 *
 * \retval None
 */
static void DrawModelProperties(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex);

/*************************************************************************************************/
/**
 * \brief Edits the a661_df attributes, the charset encoding, and the root model.
 *
 * \param document   Document being edited
 *
 * \retval None
 */
static void DrawDefinitionInspector(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Shows the specification's description, categories, and restriction of a type, wrapped.
 *
 * \param type   Widget or extension type name
 *
 * \retval None
 */
static void DrawTypeHelp(const char_t* type);

/*************************************************************************************************/
/**
 * \brief Hovering a parameter name shows the Parameters table row: the change flag and the description from the type's section.
 *
 * \param document       Document
 * \param owner          Editor owning the document
 * \param ownerIndex     Owner array index
 * \param propertyName   Property name
 *
 * \retval None
 */
static void DrawParameterTooltip(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* propertyName);

/*************************************************************************************************/
/**
 * \brief Table 3.2.3.1: the widgets that can be a parent.
 *
 * \param type   Widget or extension type name
 *
 * \retval TRUE   if the type can be a parent.
 * \retval FALSE  otherwise.
 */
static bool_t IsContainerType(const char_t* type);

/*************************************************************************************************/
/**
 * \brief Writes an integer value into an existing property, or appends it.
 *
 * \param document     Document
 * \param owner        Editor owning the document
 * \param ownerIndex   Owner array index
 * \param name         Property or element name
 * \param value        Value
 *
 * \retval None
 */
static void SetPropertyNumber(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, float32_t value);

/*************************************************************************************************/
/**
 * \brief Sums PosX and PosY from the widget up through its containers, skipping containers without a reference point as section 3.2.3 says.
 *
 * \param document      Document
 * \param widgetIndex   Widget array index
 * \param x             X in layer units
 * \param y             Y in layer units
 *
 * \retval None
 */
static void AbsoluteOrigin(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, float32_t* x, float32_t* y);

/*************************************************************************************************/
/**
 * \brief Copies the widget, its extensions, and its descendants directly after the source subtree, remapping parents into the copy and giving every copied widget a fresh WidgetIdent.
 *
 * \param document      Document
 * \param widgetIndex   Widget array index
 *
 * \retval uint32_t   Index of the copied root, or INVALID_HIERARCHY_INDEX when out of room.
 */
static uint32_t DuplicateWidget(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Removes whatever is selected and selects its parent.
 *
 * \param document   Document
 *
 * \retval None
 */
static void RemoveSelection(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Right-click menu shared by hierarchy nodes and the scene: add a child, add an extension, duplicate, or remove.
 *
 * \param document      Document
 * \param widgetIndex   Widget array index
 * \param request       Hierarchy request to fill
 *
 * \retval None
 */
static void DrawWidgetContextMenu(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, HierarchyRequest_t* request);

/*************************************************************************************************/
/**
 * \brief The pop-up opened by a right-click on the scene: a filter box, the matching widget types with their descriptions as tooltips, and the actions on the widget under the cursor.
 *
 * \param document   Document
 *
 * \retval None
 */
static void DrawAddPopup(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Rounds to whole millimetres when snapping is on.
 *
 * \param editor   Editor state
 * \param value    Value
 *
 * \retval float32_t   Value rounded to whole millimetres when snapping is on.
 */
static float32_t Snap(const Arinc661_Definition_Editor_t* editor, float32_t value);

/*************************************************************************************************/
/**
 * \brief Millimetre lines, with every tenth line stronger, drawn only when a millimetre spans at least a few pixels so the grid never turns into a wall.
 *
 * \param drawList   Target draw list
 * \param layerMin   Screen top-left of the layer
 * \param layerMax   Screen bottom-right of the layer
 * \param scale      Screen pixels per layer unit
 * \param clipMin    Clip rectangle top-left
 * \param clipMax    Clip rectangle bottom-right
 *
 * \retval None
 */
static void DrawGrid(ImDrawList* drawList, ImVec2 layerMin, ImVec2 layerMax, float32_t scale, ImVec2 clipMin, ImVec2 clipMax);

/*************************************************************************************************/
/**
 * \brief Draws a dashed rectangle in screen space.
 *
 * \param[in,out] drawList   Target draw list
 * \param[in]     minimum    Top-left corner
 * \param[in]     maximum    Bottom-right corner
 * \param[in]     color      Line colour
 *
 * \retval        None
 */
static void DrawDashedScreenRect(ImDrawList* drawList, ImVec2 minimum, ImVec2 maximum, ImU32 color);

/*************************************************************************************************/
/**
 * \brief Re-parents a widget with its subtree and extensions: the subtree is copied after the new parent's subtree with parents remapped, then the original is removed.
 *
 * \param document      Document
 * \param widgetIndex   Widget array index
 * \param newLayer      Layer the subtree moves to
 * \param newParent     New parent widget, or INVALID_HIERARCHY_INDEX
 *
 * \retval TRUE   if the widget moved.
 * \retval FALSE  otherwise.
 */
static bool_t MoveWidgetSubtree(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, uint32_t newLayer, uint32_t newParent);

/*************************************************************************************************/
/**
 * \brief Icon row above the canvas: zoom, fit, grid, snap, then add layer, add widget, add extension, duplicate, delete, and a combo choosing the layer shown.
 *
 * \param document     Document
 * \param layerIndex   Layer array index
 *
 * \retval None
 */
/*************************************************************************************************/
/**
 * \brief Returns a layer's LayerId property text.
 *
 * \param[in] document     Document being edited
 * \param[in] layerIndex   Layer array index
 *
 * \retval    const char_t*   LayerId text, or an empty string.
 */
static const char_t* LayerIdText(const Arinc661_Definition_Document_t* document, uint32_t layerIndex);

/*************************************************************************************************/
/**
 * \brief Draws the icon row above the scene canvas and the layer chooser.
 *
 * \param[in,out] document     Document being edited
 * \param[in,out] layerIndex   Layer shown; updated by the chooser
 *
 * \retval        None
 */
static void DrawSceneToolbar(Arinc661_Definition_Document_t* document, uint32_t* layerIndex);

/*************************************************************************************************/
/**
 * \brief Counts the widgets in the multi-selection.
 *
 * \param[in] document   Document being edited
 *
 * \retval    uint32_t   Number of selected widgets.
 */
static uint32_t SelectedCount(const Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Tests whether a widget, or any descendant, matches the hierarchy filter.
 *
 * \param[in] document      Document being edited
 * \param[in] widgetIndex   Widget array index
 * \param[in] filter        Filter text
 *
 * \retval    TRUE          if the name or type contains the filter, ignoring case, here or below.
 * \retval    FALSE         otherwise.
 */
static bool_t MatchesFilter(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, const char_t* filter);

/*************************************************************************************************/
/**
 * \brief Draws a small icon toggle used for the hierarchy's eye and lock.
 *
 * \param[in] icon       Icon
 * \param[in] tooltip    Tooltip text; also the id
 * \param[in] isActive   TRUE to draw it highlighted
 *
 * \retval    TRUE       if clicked.
 * \retval    FALSE      otherwise.
 */
static bool_t DrawFlagButton(Arinc661_Icon_t icon, const char_t* tooltip, bool_t isActive);

/*************************************************************************************************/
/**
 * \brief Inserts a copy of clipboard subtrees under a parent.
 *
 * \param[in,out] document      Document being edited
 * \param[in]     layerIndex    Layer array index
 * \param[in]     parentIndex   Parent widget, or INVALID_HIERARCHY_INDEX for a layer child
 *
 * \retval        uint32_t      Widgets inserted.
 */
static uint32_t InsertClipboard(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex);

/*************************************************************************************************/
/**
 * \brief Makes one widget the whole selection.
 *
 * \param[in,out] document      Document being edited
 * \param[in]     widgetIndex   Widget to select
 *
 * \retval        None
 */
static void SelectOnly(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Adds a widget to, or removes it from, the multi-selection.
 *
 * \param[in,out] document      Document being edited
 * \param[in]     widgetIndex   Widget to toggle
 *
 * \retval        None
 */
static void ToggleSelected(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Clears the multi-selection flags.
 *
 * \param[in,out] document   Document being edited
 *
 * \retval        None
 */
static void ClearSelectionFlags(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Aligns or distributes the selected widgets by their absolute bounds.
 *
 * \param[in,out] document   Document being edited
 * \param[in]     mode       0 left, 1 right, 2 top, 3 bottom, 4 centre X, 5 centre Y, 6 distribute X, 7 distribute Y
 *
 * \retval        None
 */
static void AlignSelection(Arinc661_Definition_Document_t* document, int32_t mode);

/*************************************************************************************************/
/**
 * \brief Draws rulers along the canvas edges in millimetres.
 *
 * \param[in,out] drawList    Target draw list
 * \param[in]     canvasMin   Canvas top-left
 * \param[in]     canvasMax   Canvas bottom-right
 * \param[in]     layerMin    Screen top-left of the layer
 * \param[in]     layerMax    Screen bottom-right of the layer
 * \param[in]     scale       Screen pixels per layer unit
 *
 * \retval        None
 */
static void DrawRulers(ImDrawList* drawList, ImVec2 canvasMin, ImVec2 canvasMax, ImVec2 layerMin, ImVec2 layerMax, float32_t scale);

/*************************************************************************************************/
/**
 * \brief Direct manipulation on the scene canvas.
 *
 * \param document          Document
 * \param layerIndex        Layer array index
 * \param layerMin          Screen top-left of the layer
 * \param layerMax          Screen bottom-right of the layer
 * \param scale             Screen pixels per layer unit
 * \param drawList          Target draw list
 * \param isCanvasHovered   TRUE while the mouse is over the canvas
 *
 * \retval None
 */
static void DrawSceneInteraction(Arinc661_Definition_Document_t* document, uint32_t layerIndex, ImVec2 layerMin, ImVec2 layerMax, float32_t scale, ImDrawList* drawList, bool_t isCanvasHovered);

/*************************************************************************************************/
/**
 * \brief Copies the used parts of the model into one heap block.
 *
 * \param document   Document
 *
 * \retval void*   Heap block holding the snapshot; the caller frees it.
 */
static void* TakeSnapshot(const Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Copies a snapshot back over the model, selection included.
 *
 * \param document   Document
 * \param snapshot   Model snapshot
 *
 * \retval None
 */
static void RestoreSnapshot(Arinc661_Definition_Document_t* document, const void* snapshot);

/*************************************************************************************************/
/**
 * \brief Compares the counts, the header fields, and the used arrays; the selection is ignored so selecting alone never makes an undo step.
 *
 * \param document   Document
 * \param snapshot   Model snapshot
 *
 * \retval TRUE   if the model equals the snapshot.
 * \retval FALSE  otherwise.
 */
static bool_t SnapshotMatches(const Arinc661_Definition_Document_t* document, const void* snapshot);

/*************************************************************************************************/
/**
 * \brief Frees every snapshot.
 *
 * \param document   Document
 *
 * \retval None
 */
static void ClearHistory(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Called once per frame: when the model differs from the shadow, the shadow becomes an undo step, the redo stack is dropped, and a fresh shadow is taken.
 *
 * \param document   Document
 *
 * \retval None
 */
static void CommitHistory(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Writes a short label for the change between a snapshot and the model.
 *
 * \param document   Document
 * \param before     Snapshot of the earlier state
 * \param label      Label written
 * \param capacity   Bytes in label
 *
 * \retval None
 */
static void DescribeChange(const Arinc661_Definition_Document_t* document, const Snapshot_t* before, char_t* label, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief The current state moves to the redo stack; the newest undo step becomes the model and the shadow.
 *
 * \param document   Document
 *
 * \retval None
 */
static void UndoDocument(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Mirror of UndoDocument.
 *
 * \param document   Document
 *
 * \retval None
 */
static void RedoDocument(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Lists every user application of the open configurations whose dFPath names this document, flags ApplicationId mismatches against FACE applicationId, offers the known user application identifiers for the ApplicationId property, and jumps to a user application.
 *
 * \param document   Document
 *
 * \retval None
 */
static void DrawContextReferences(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Draws the configuration and window choice and the button creating a user application.
 *
 * \param[in,out] document   Document being edited
 *
 * \retval        None
 */
static void DrawCreateApplication(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Maps a parameter name to what it refers to.
 *
 * \param[in] parameterName   Parameter name
 *
 * \retval    ReferenceKind_t   Kind, or REFERENCE_NONE.
 */
static ReferenceKind_t ReferenceKindOf(const char_t* parameterName);

/*************************************************************************************************/
/**
 * \brief Adds a widget or extension of a type under a parent, optionally at a layer position.
 *
 * \param[in,out] document      Document being edited
 * \param[in]     layerIndex    Layer array index
 * \param[in]     parentIndex   Parent widget, or INVALID_HIERARCHY_INDEX for a layer child
 * \param[in]     type          Widget or extension type name
 * \param[in]     x             Layer X of the reference point
 * \param[in]     y             Layer Y of the reference point
 * \param[in]     hasPosition   TRUE to apply x and y
 *
 * \retval        None
 */
static void AddTypedWidget(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, const char_t* type, float32_t x, float32_t y, bool_t hasPosition);

/*************************************************************************************************/
/**
 * \brief Finds the widget of a layer carrying a WidgetIdent.
 *
 * \param[in] document     Document being edited
 * \param[in] layerIndex   Layer array index
 * \param[in] ident        WidgetIdent value
 *
 * \retval    uint32_t   Widget array index, or INVALID_HIERARCHY_INDEX.
 */
static uint32_t FindWidgetByIdent(const Arinc661_Definition_Document_t* document, uint32_t layerIndex, int64_t ident);

/*************************************************************************************************/
/**
 * \brief Finds the layer carrying a LayerId.
 *
 * \param[in] document   Document being edited
 * \param[in] layerId    LayerId value
 *
 * \retval    uint32_t   Layer array index, or INVALID_HIERARCHY_INDEX.
 */
static uint32_t FindLayerById(const Arinc661_Definition_Document_t* document, int64_t layerId);

/*************************************************************************************************/
/**
 * \brief Resolves a reference value to the document and item it names.
 *
 * \param[in]  document      Document being edited
 * \param[in]  widgetIndex   Widget holding the reference
 * \param[in]  kind          Reference kind
 * \param[in]  value         Property value
 * \param[out] targetDocument   Definition File document slot of the target, when a widget or layer
 * \param[out] targetIndex      Widget or layer array index, user application index, or external source index
 * \param[out] label         Text naming the target, or the no-match text
 * \param[in]  labelCapacity Bytes in label
 *
 * \retval     TRUE          if the value is zero or names an existing item.
 * \retval     FALSE         otherwise.
 */
static bool_t ResolveReference(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, ReferenceKind_t kind, const char_t* value, uint32_t* targetDocument, uint32_t* targetIndex, char_t* label, uint32_t labelCapacity);

/*************************************************************************************************/
/**
 * \brief Draws a reference parameter as a picker with a go-to button.
 *
 * \param[in,out] document      Document being edited
 * \param[in]     widgetIndex   Widget holding the reference
 * \param[in]     kind          Reference kind
 * \param[in,out] value         Property value edited in place
 * \param[in]     capacity      Bytes available in value
 *
 * \retval        TRUE          if the value changed.
 * \retval        FALSE         otherwise.
 */
static bool_t DrawReferenceValue(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, ReferenceKind_t kind, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Tests whether a widget type may sit under a parent type, or directly under a layer.
 *
 * \param[in] childType    Widget type
 * \param[in] parentType   Parent widget type, or NULL for a layer child
 *
 * \retval    TRUE         if the specification allows it.
 * \retval    FALSE        otherwise.
 */
static bool_t IsParentAllowed(const char_t* childType, const char_t* parentType);

/*************************************************************************************************/
/**
 * \brief Edits the layer name and its model.
 *
 * \param document   Document being edited
 *
 * \retval None
 */
static void DrawLayerInspector(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Edits the widget or extension type, the widget name, and the model.
 *
 * \param document   Document being edited
 *
 * \retval None
 */
static void DrawWidgetInspector(Arinc661_Definition_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Lists the widgets of the layer that reference a widget's ident, each selectable.
 *
 * \param document      Document
 * \param widgetIndex   Widget array index
 *
 * \retval None
 */
static void DrawReferencedBy(Arinc661_Definition_Document_t* document, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Lists XML and binary Definition Files; a selected name is loaded through the folder path.
 *
 * \param fileName   File name inside the folder
 * \param context    Caller context passed through the enumeration
 *
 * \retval None
 */
static void DrawXmlFileEntry(const char_t* fileName, void* context);

/*************************************************************************************************/
/**
 * \brief Lists XML files in the configured folder and updates the selected filename.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     folderInputX  Horizontal position of the folder input
 *
 * \retval        None
 */
static void DrawXmlFileList(Arinc661_Definition_Editor_t* editor, float32_t folderInputX);

/*************************************************************************************************/
/**
 * \brief Draws the file path, file actions, and status above the panes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawFileControls(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Frees the slot and moves the active tab to another loaded document, if any.
 *
 * \param editor          Editor state
 * \param documentIndex   Document slot
 *
 * \retval None
 */
static void CloseDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Draws the File menu for the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawFileMenuBar(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws document tabs and handles save confirmation for dirty tabs.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawDocumentTabs(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the filename dialog for an untitled document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawSaveAsDialog(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the document state and latest editor message below the panes.
 *
 * \param[in] editor   Editor state
 *
 * \retval    None
 */
static void DrawStatusBar(const Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the configuration hierarchy in the left child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawHierarchyPane(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Dispatches to the inspector for the selected entity.
 *
 * \param editor   Editor state
 *
 * \retval None
 */
static void DrawInspectorPane(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Sums PosX and PosY through the container chain, since section 3.1.3 positions a widget * relative to its parent's lower-left corner.
 *
 * \param document      Document being edited
 * \param widgetIndex   Widget array index
 * \param area          Resolved area output
 *
 * \retval None
 */
static void ResolveWidgetArea(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, WidgetArea_t* area);

/*************************************************************************************************/
/**
 * \brief Draws the selected layer, or the layer owning the selected widget, scaled to fit the * pane with its origin at the lower-left corner as in section 3.1.3.
 *
 * \param editor   Editor state
 *
 * \retval None
 */
static void DrawScenePane(Arinc661_Definition_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Limits pending pane drags before ImGui applies them.
 *
 * \param[in] minimumHierarchyWidth   Width required by the hierarchy controls
 *
 * \retval    None
 */
static void ClampPaneResize(float32_t minimumHierarchyWidth);

/*************************************************************************************************/
/**
 * \brief Sizes the hierarchy to its widest row and the inspector to its label and value columns.
 *
 * \param editor           Editor state
 * \param hierarchyWidth   Minimum hierarchy width output
 * \param inspectorWidth   Inspector width output
 *
 * \retval None
 */
static void MeasurePaneWidths(const Arinc661_Definition_Document_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*************************************************************************************************/
/**
 * \brief Draws the hierarchy, scene, and inspector panes for the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawPanes(Arinc661_Definition_Editor_t* editor);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps all status bar messages in one place.
 */
static const char_t* EditorStatusToString(int32_t status)
{
    switch (status)
    {
        case STATUS_NONE:                 return "";
        case STATUS_FOLDER_PATH_REQUIRED: return "Enter a folder path.";
        case STATUS_XML_FILE_REQUIRED:    return "Select a Definition File.";
        case STATUS_SERIALIZATION_FAILED: return "Could not build the Definition File; check widget types, property names, and values.";
        case STATUS_SAVED:                return "Saved Definition File.";
        case STATUS_ALL_SAVED:            return "Saved all changed Definition Files.";
        case STATUS_SAVE_ALL_FAILED:      return "Could not save every Definition File.";
        case STATUS_NEW_DOCUMENT:         return "Created a new Definition File.";
        case STATUS_SAVE_NAME_REQUIRED:   return "Enter a filename.";
        case STATUS_SAVE_NAME_INVALID:    return "Enter a valid filename ending in .xml or .bin.";
        case STATUS_SAVE_FILE_EXISTS:     return "A file with that name already exists.";
        case STATUS_SAVE_FOLDER_INVALID:  return "Could not read the save folder.";
        case STATUS_SAVE_ALL_CANCELED:    return "Save All canceled.";
        case STATUS_WRITE_FAILED:         return "Could not write Definition File.";
        case STATUS_LAYER_ADDED:          return "Added layer.";
        case STATUS_LAYER_ADD_FAILED:     return "Could not add another layer.";
        case STATUS_WIDGET_ADDED:         return "Added widget.";
        case STATUS_WIDGET_ADD_FAILED:    return "Could not add another widget.";
        case STATUS_EXTENSION_ADDED:      return "Added extension.";
        case STATUS_PROPERTY_ADDED:       return "Added property.";
        case STATUS_PROPERTY_ADD_FAILED:  return "Could not add another property.";
        case STATUS_LAYER_REMOVED:        return "Removed layer.";
        case STATUS_WIDGET_REMOVED:       return "Removed widget.";
        case STATUS_EXTENSION_REMOVED:    return "Removed extension.";
        case STATUS_PROPERTY_REMOVED:     return "Removed property.";
        case STATUS_READ_FAILED:          return "Could not read Definition File.";
        case STATUS_INPUT_TOO_LARGE:      return "The Definition File exceeds the input buffer.";
        case STATUS_INPUT_EMPTY:          return "The Definition File is empty.";
        case STATUS_LOADED:               return "Loaded Definition File.";
        case STATUS_INVALID_XML:          return "Invalid or unsupported XML Definition File.";
        case STATUS_INVALID_BINARY:       return "Invalid or unsupported binary Definition File.";
        case STATUS_TOO_MANY_DOCUMENTS:   return "Close a document before opening another Definition File.";
        default:
          LOGFATAL("Invalid Status_t!");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses forward slashes in joined paths so either folder separator spelling compares equally.
 */
static void JoinFolderPath(const char_t* folderPath, const char_t* fileName, char_t* filePath, uint32_t capacity)
{
    uint32_t folder_length;
    uint32_t file_name_length;
    uint32_t index;
    bool_t   ends_with_backslash;
    bool_t   ends_with_slash;
    bool_t   needs_separator;

    folder_length       = (uint32_t)ImStrlen(folderPath);
    file_name_length    = (uint32_t)ImStrlen(fileName);
    LOGASSERT(0 < folder_length);
    ends_with_backslash = ('\\' == folderPath[folder_length - 1]);
    ends_with_slash     = ('/' == folderPath[folder_length - 1]);
    needs_separator     = (FALSE == ends_with_backslash) && (FALSE == ends_with_slash);
    LOGASSERT((folder_length + (uint32_t)needs_separator + file_name_length + 1) <= capacity);

    (void)memcpy(filePath, folderPath, folder_length);
    for (index = 0; index < folder_length; ++index)
    {
        if ('\\' == filePath[index])
        {
            filePath[index] = '/';
        }
    }
    if (needs_separator)
    {
        filePath[folder_length] = '/';
        folder_length += 1;
    }
    (void)memcpy(filePath + folder_length, fileName, file_name_length + 1);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reports which part of the requested XML path is missing.
 */
static bool_t GetXmlFilePath(Arinc661_Definition_Editor_t* editor, const char_t* fileName, char_t* filePath, uint32_t capacity)
{
    bool_t has_folder_path;
    bool_t has_xml_file_name;
    bool_t has_file_path;

    has_folder_path    = ('\0' != editor->folderPath[0]);
    has_xml_file_name  = ('\0' != fileName[0]);
    has_file_path      = FALSE;
    if (FALSE == has_folder_path)
    {
        editor->status = STATUS_FOLDER_PATH_REQUIRED;
    }
    else if (FALSE == has_xml_file_name)
    {
        editor->status = STATUS_XML_FILE_REQUIRED;
    }
    else
    {
        JoinFolderPath(editor->folderPath, fileName, filePath, capacity);
        has_file_path = TRUE;
    }

    return has_file_path;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares the end of the filename with the extension, ignoring case.
 */
static bool_t HasExtension(const char_t* fileName, const char_t* extension)
{
    size_t file_name_length;
    size_t extension_length;
    bool_t has_extension;

    file_name_length = ImStrlen(fileName);
    extension_length = ImStrlen(extension);
    has_extension    = (extension_length < file_name_length);
    if (has_extension)
    {
        has_extension = (0 == ImStricmp(fileName + file_name_length - extension_length, extension));
    }

    return has_extension;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps a new file inside its selected folder and requires the XML or binary extension.
 */
static bool_t IsValidXmlFileName(const char_t* fileName)
{
    size_t file_name_length;
    size_t index;
    bool_t is_valid;

    file_name_length = ImStrlen(fileName);
    is_valid         = !!(HasExtension(fileName, XML_EXTENSION) || HasExtension(fileName, BINARY_EXTENSION));
    if (is_valid)
    {
        for (index = 0; index < file_name_length; ++index)
        {
            switch (fileName[index])
            {
                case '\\':
                case '/':
                case ':':
                case '*':
                case '?':
                case '"':
                case '<':
                case '>':
                case '|':
                {
                    is_valid = FALSE;
                    break;
                }
                default:
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

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Clears every count and selection while keeping the storage pointers and capacities.
 */
static void ResetDocument(Arinc661_Definition_Document_t* document)
{
    Arinc661_Definition_Editor_t* owner_editor;

    owner_editor = document->owner;
    ClearHistory(document);
    document->status                     = STATUS_NONE;
    document->isLoaded                   = FALSE;
    document->isDirty                    = FALSE;
    document->isBinary                   = FALSE;
    document->selectedKind               = SELECTION_NONE;
    document->selectedIndex              = 0;
    document->documentName[0]            = '\0';
    document->documentPath[0]            = '\0';
    document->definition.name[0]         = '\0';
    document->definition.libraryVersion  = 0;
    document->definition.supplementVersion = DEFAULT_SUPPLEMENT;
    document->definition.layerCount      = 0;
    document->definition.widgetCount     = 0;
    document->definition.propertyCount   = 0;
    document->definition.charsetEncoding = 0;
    document->definition.blockByteCount  = 0;
    document->owner = owner_editor;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves an open document's edits when the selected path matches its stored path.
 *  A file starting with A661_DF_MAGIC_NUMBER is read as binary; any other file as XML.
 */
static void LoadDefinitionFile(Arinc661_Definition_Editor_t* editor, const char_t* fileName)
{
    Arinc661_Definition_Document_t* document;
    char_t                          file_path[ARINC661_DEFINITION_MAX_FILE_PATH_BYTES];
    int32_t                         document_byte_count;
    uint32_t                        index;
    uint32_t                        document_index;
    uint32_t                        available_index;
    bool_t                          is_definition;
    bool_t                          has_file_path;
    bool_t                          has_matching_path;

    has_file_path = GetXmlFilePath(editor, fileName, file_path, sizeof(file_path));
    if (has_file_path)
    {
        available_index = INVALID_DOCUMENT_INDEX;
        document_index  = INVALID_DOCUMENT_INDEX;
        for (index = 0; index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++index)
        {
            document = &editor->documents[index];
            if (document->isLoaded)
            {
                has_matching_path = (0 == ImStricmp(document->documentPath, file_path));
                if (has_matching_path)
                {
                    document_index = index;
                    if (document->isDirty)
                    {
                        break;
                    }
                }
            }
            else if (INVALID_DOCUMENT_INDEX == available_index)
            {
                available_index = index;
            }
        }
        if (INVALID_DOCUMENT_INDEX != document_index)
        {
            editor->activeDocumentIndex    = document_index;
            editor->requestedDocumentIndex = document_index;
            editor->status                 = STATUS_NONE;
        }
        else if (INVALID_DOCUMENT_INDEX == available_index)
        {
            editor->status = STATUS_TOO_MANY_DOCUMENTS;
        }
        else
        {
            document = &editor->documents[available_index];
            ResetDocument(document);
            document_byte_count = OS_File_ReadFile(file_path, document->inputBuffer, sizeof(document->inputBuffer));
            if (0 > document_byte_count)
            {
                editor->status = STATUS_READ_FAILED;
            }
            else if (0 == document_byte_count)
            {
                editor->status = STATUS_INPUT_EMPTY;
            }
            else if (ARINC661_DEFINITION_MAX_DOCUMENT_BYTES < document_byte_count)
            {
                editor->status = STATUS_INPUT_TOO_LARGE;
            }
            else
            {
                // Detect the binary form by its magic number rather than by the file extension
                document->isBinary = !!((1 < document_byte_count) && (BINARY_MAGIC_HIGH == document->inputBuffer[0]) && (BINARY_MAGIC_LOW == document->inputBuffer[1]));
                if (document->isBinary)
                {
                    is_definition = Arinc661_Definition_ReadBinary(&document->definition, document->inputBuffer, (uint64_t)document_byte_count);
                }
                else
                {
                    is_definition = Arinc661_Definition_Read(&document->definition, document->inputBuffer, (uint64_t)document_byte_count, document->workspace, sizeof(document->workspace));
                }
                if (is_definition)
                {
                    document->isLoaded     = TRUE;
                    document->selectedKind = SELECTION_DEFINITION;
                    ImStrncpy(document->documentName, fileName, sizeof(document->documentName));
                    ImStrncpy(document->documentPath, file_path, sizeof(document->documentPath));
                    document->status               = STATUS_LOADED;
                    editor->activeDocumentIndex    = available_index;
                    editor->requestedDocumentIndex = available_index;
                    editor->status                 = STATUS_NONE;
                }
                else if (document->isBinary)
                {
                    editor->status = STATUS_INVALID_BINARY;
                }
                else
                {
                    editor->status = STATUS_INVALID_XML;
                }
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Starts an empty, dirty document in one initialized slot.
 */
static void CreateDocument(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        document_index;
    uint32_t                        available_index;

    available_index = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (FALSE == editor->documents[document_index].isLoaded)
        {
            available_index = document_index;
            break;
        }
    }
    if (INVALID_DOCUMENT_INDEX == available_index)
    {
        editor->status = STATUS_TOO_MANY_DOCUMENTS;
    }
    else
    {
        document = &editor->documents[available_index];
        ResetDocument(document);
        document->status       = STATUS_NEW_DOCUMENT;
        document->isLoaded     = TRUE;
        document->isDirty      = TRUE;
        document->selectedKind = SELECTION_DEFINITION;
        ImFormatString(document->documentName, sizeof(document->documentName), UNTITLED_DOCUMENT_FORMAT, available_index + 1);
        editor->activeDocumentIndex    = available_index;
        editor->requestedDocumentIndex = available_index;
        editor->status                 = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the binary form for a .bin path and the XML form otherwise, so Save As can
 *  convert between the two.
 */
static void SaveLoadedDocument(Arinc661_Definition_Document_t* editor)
{
    uint64_t output_byte_count;
    int32_t  bytes_written;
    bool_t   can_save_document;

    can_save_document = !!(editor->isLoaded && ('\0' != editor->documentPath[0]));
    if (can_save_document)
    {
        editor->isBinary = HasExtension(editor->documentPath, BINARY_EXTENSION);
        if (editor->isBinary)
        {
            output_byte_count = Arinc661_Definition_WriteBinary(&editor->definition, editor->outputBuffer, sizeof(editor->outputBuffer));
        }
        else
        {
            output_byte_count = Arinc661_Definition_Write(&editor->definition, editor->outputBuffer, sizeof(editor->outputBuffer), editor->workspace, sizeof(editor->workspace));
        }
        if (0 == output_byte_count)
        {
            editor->status = STATUS_SERIALIZATION_FAILED;
        }
        else
        {
            bytes_written = OS_File_WriteFile(editor->documentPath, editor->outputBuffer, (uint32_t)output_byte_count);
            if (output_byte_count == (uint64_t)bytes_written)
            {
                editor->isDirty = FALSE;
                editor->status  = STATUS_SAVED;
            }
            else
            {
                editor->status = STATUS_WRITE_FAILED;
            }
        }
    }
    else
    {
        editor->status = STATUS_XML_FILE_REQUIRED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Untitled documents need a filename before their first write.
 */
static void SaveDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    Arinc661_Definition_Document_t* document;

    document       = &editor->documents[documentIndex];
    editor->status = STATUS_NONE;
    if ('\0' == document->documentPath[0])
    {
        editor->pendingSaveDocumentIndex = documentIndex;
        ImStrncpy(editor->saveFolderPath, editor->folderPath, sizeof(editor->saveFolderPath));
        ImStrncpy(editor->saveFileName, document->documentName, sizeof(editor->saveFileName));
    }
    else
    {
        SaveLoadedDocument(document);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attempts every dirty document and leaves a failed document marked dirty.
 */
static void SaveAllDocuments(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        document_index;
    bool_t                          all_saved;

    all_saved = TRUE;
    editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        if (document->isLoaded && document->isDirty)
        {
            if ('\0' == document->documentPath[0])
            {
                if (INVALID_DOCUMENT_INDEX == editor->pendingSaveDocumentIndex)
                {
                    SaveDocument(editor, document_index);
                }
                all_saved = FALSE;
            }
            else
            {
                SaveLoadedDocument(document);
                if (document->isDirty)
                {
                    all_saved = FALSE;
                }
            }
        }
    }
    if (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex)
    {
        editor->isSaveAllPending = TRUE;
    }
    else if (all_saved)
    {
        editor->isSaveAllPending = FALSE;
        editor->status           = STATUS_ALL_SAVED;
    }
    else
    {
        editor->isSaveAllPending = FALSE;
        editor->status           = STATUS_SAVE_ALL_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the directory enumerator's existing filename conversion to detect collisions.
 */
static void FindMatchingFile(const char_t* fileName, void* context)
{
    FileMatchContext_t* match_context;

    match_context = (FileMatchContext_t*)context;
    if (0 == ImStricmp(fileName, match_context->fileName))
    {
        match_context->isFound = TRUE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the first property of one model with the given name.
 */
static uint32_t FindOwnedProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name)
{
    const Arinc661_Property_t* property;
    uint32_t                   property_index;
    uint32_t                   found_index;

    found_index = INVALID_HIERARCHY_INDEX;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((owner == property->owner) && (ownerIndex == property->ownerIndex) && (0 == strncmp(name, property->name, sizeof(property->name))))
        {
            found_index = property_index;
            break;
        }
    }

    return found_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a numeric property such as PosX; the scan must consume the whole value.
 */
static bool_t GetPropertyNumber(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, float32_t* value)
{
    const char_t* text;
    float64_t     parsed_value;
    uint32_t      property_index;
    int32_t       consumed;
    bool_t        has_number;

    property_index = FindOwnedProperty(document, owner, ownerIndex, name);
    has_number     = (INVALID_HIERARCHY_INDEX != property_index);
    if (has_number)
    {
        text       = document->properties[property_index].value;
        consumed   = FACEOE_Utl_Scan_Float(text, &parsed_value);
        has_number = !!((0 < consumed) && ('\0' == text[consumed]));
    }
    if (has_number)
    {
        (*value) = (float32_t)parsed_value;
    }

    return has_number;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Appends at the end of the property array, which keeps the order of each model's
 *  existing properties.
 */
static bool_t AppendProperty(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value)
{
    Arinc661_Property_t* property;
    bool_t               has_room;

    has_room = (document->definition.propertyCount < ARINC661_DEFINITION_MAX_PROPERTIES);
    if (has_room)
    {
        property = &document->properties[document->definition.propertyCount];
        (void)memset(property, 0, sizeof((*property)));
        property->owner      = owner;
        property->ownerIndex = ownerIndex;
        ImStrncpy(property->name, name, sizeof(property->name));
        ImStrncpy(property->value, value, sizeof(property->value));
        document->definition.propertyCount += 1;
    }

    return has_room;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Closes the gap left by the removed property.
 */
static void RemovePropertyAt(Arinc661_Definition_Document_t* document, uint32_t propertyIndex)
{
    uint32_t remaining_count;

    LOGASSERT(propertyIndex < document->definition.propertyCount);
    remaining_count = (document->definition.propertyCount - propertyIndex - 1);
    (void)memmove(&document->properties[propertyIndex], &document->properties[propertyIndex + 1], remaining_count * sizeof(document->properties[0]));
    document->definition.propertyCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the property at the given position among one model's properties of that name,
 *  which is the array element index for array parameters.
 */
static uint32_t FindOwnedPropertyAt(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, uint32_t occurrence)
{
    const Arinc661_Property_t* property;
    uint32_t                   property_index;
    uint32_t                   found_index;
    uint32_t                   match_count;

    found_index = INVALID_HIERARCHY_INDEX;
    match_count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((owner == property->owner) && (ownerIndex == property->ownerIndex) && (0 == strncmp(name, property->name, sizeof(property->name))))
        {
            if (occurrence == match_count)
            {
                found_index = property_index;
                break;
            }
            match_count += 1;
        }
    }

    return found_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Chooses the value a new property of this parameter starts with: A661_TRUE when the
 *  table lists it, otherwise the first listed constant, zero for numbers, an empty string
 *  for text, and zero digits for fixed bytes. SizeX and SizeY start visible.
 */
static void DefaultParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity)
{
    uint32_t value_index;
    uint32_t digit_index;

    ImStrncpy(value, "0", capacity);
    if (0 < parameter->valueCount)
    {
        ImStrncpy(value, parameter->values[0], capacity);
        for (value_index = 0; value_index < parameter->valueCount; ++value_index)
        {
            if (0 == strncmp(DEFAULT_BOOLEAN_TRUE, parameter->values[value_index], sizeof(DEFAULT_BOOLEAN_TRUE)))
            {
                ImStrncpy(value, DEFAULT_BOOLEAN_TRUE, capacity);
                break;
            }
        }
    }
    else if ((ARINC661_PARAMETER_STRING == parameter->kind) || (ARINC661_PARAMETER_SIZED_BYTES == parameter->kind))
    {
        value[0] = '\0';
    }
    else if ((ARINC661_PARAMETER_REGION == parameter->kind) || (ARINC661_PARAMETER_LAYER_REQUEST == parameter->kind))
    {
        ImStrncpy(value, "00000000", capacity);
    }
    else if (ARINC661_PARAMETER_BYTES == parameter->kind)
    {
        LOGASSERT((parameter->byteCount * 2U) < capacity);
        for (digit_index = 0; digit_index < (parameter->byteCount * 2U); ++digit_index)
        {
            value[digit_index] = '0';
        }
        value[parameter->byteCount * 2U] = '\0';
    }
    else if ((0 == strncmp(PROPERTY_SIZE_X, parameter->name, sizeof(PROPERTY_SIZE_X))) || (0 == strncmp(PROPERTY_SIZE_Y, parameter->name, sizeof(PROPERTY_SIZE_Y))))
    {
        ImStrncpy(value, DEFAULT_LAYER_SIZE, capacity);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts uppercase hexadecimal digits; a value with any other character, or an odd
 *  count, is not a byte string.
 */
static uint32_t HexByteCount(const char_t* value)
{
    size_t   length;
    size_t   index;
    uint32_t byte_count;

    length     = ImStrlen(value);
    byte_count = (uint32_t)(length / 2U);
    if (0 != (length % 2U))
    {
        byte_count = INVALID_HIERARCHY_INDEX;
    }
    for (index = 0; (INVALID_HIERARCHY_INDEX != byte_count) && (index < length); ++index)
    {
        if (NULL == strchr(c_hexDigits, value[index]))
        {
            byte_count = INVALID_HIERARCHY_INDEX;
        }
    }

    return byte_count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Applies the same acceptance rules as the binary writer: an integer parameter takes a
 *  decimal number in its range or a section 4.6 constant name whose value fits; a decimal
 *  parameter takes a number within its range; byte parameters take hexadecimal digits of
 *  the right length; strings take anything.
 */
static bool_t IsParameterValueValid(const Arinc661_Parameter_t* parameter, const char_t* value)
{
    float64_t real_value;
    int64_t   integer_value;
    int64_t   minimum;
    int64_t   maximum;
    int32_t   consumed;
    uint32_t  constant_index;
    uint32_t  byte_count;
    bool_t    is_valid;

    is_valid = TRUE;
    if ((ARINC661_PARAMETER_UCHAR == parameter->kind) || (ARINC661_PARAMETER_USHORT == parameter->kind) || (ARINC661_PARAMETER_LONG == parameter->kind) || (ARINC661_PARAMETER_ULONG == parameter->kind))
    {
        minimum = 0;
        maximum = UINT32_MAX;
        if (ARINC661_PARAMETER_UCHAR == parameter->kind)
        {
            maximum = UINT8_MAX;
        }
        else if (ARINC661_PARAMETER_USHORT == parameter->kind)
        {
            maximum = UINT16_MAX;
        }
        else if (ARINC661_PARAMETER_LONG == parameter->kind)
        {
            minimum = INT32_MIN;
            maximum = INT32_MAX;
        }
        consumed = FACEOE_Utl_Scan_Int64(value, &integer_value);
        is_valid = !!((0 < consumed) && ('\0' == value[consumed]));
        if (FALSE == is_valid)
        {
            for (constant_index = 0; constant_index < Arinc661_Definition_GetConstantCount(); ++constant_index)
            {
                if (0 == strncmp(value, Arinc661_Definition_GetConstantName(constant_index), ARINC661_MAX_NAME_BYTES))
                {
                    integer_value = Arinc661_Definition_GetConstantValue(constant_index);
                    is_valid      = TRUE;
                    break;
                }
            }
        }
        if (is_valid)
        {
            is_valid = !!((minimum <= integer_value) && (integer_value <= maximum));
        }
    }
    else if ((ARINC661_PARAMETER_FLOAT == parameter->kind) || (ARINC661_PARAMETER_FR180 == parameter->kind) || (ARINC661_PARAMETER_FR32768 == parameter->kind))
    {
        consumed = FACEOE_Utl_Scan_Float(value, &real_value);
        is_valid = !!((0 < consumed) && ('\0' == value[consumed]));
        if (is_valid && (ARINC661_PARAMETER_FR180 == parameter->kind))
        {
            is_valid = !!((-180.0 <= real_value) && (real_value <= 180.0));
        }
        else if (is_valid && (ARINC661_PARAMETER_FR32768 == parameter->kind))
        {
            is_valid = !!((-32768.0 <= real_value) && (real_value <= 32768.0));
        }
    }
    else if (ARINC661_PARAMETER_BYTES == parameter->kind)
    {
        is_valid = (parameter->byteCount == HexByteCount(value));
    }
    else if (ARINC661_PARAMETER_SIZED_BYTES == parameter->kind)
    {
        byte_count = HexByteCount(value);
        is_valid   = !!((INVALID_HIERARCHY_INDEX != byte_count) && (byte_count <= UINT16_MAX));
    }
    else if (ARINC661_PARAMETER_REGION == parameter->kind)
    {
        // Table 8.6.7-3: the second byte selects a structure of 4, 20, 28, or 24 bytes
        byte_count = HexByteCount(value);
        is_valid   = !!((INVALID_HIERARCHY_INDEX != byte_count) && (4 <= byte_count) && ('0' == value[2]));
        if (is_valid)
        {
            is_valid = !!((('0' == value[3]) && (4 == byte_count)) || (('1' == value[3]) && (20 == byte_count)) || (('2' == value[3]) && (28 == byte_count)) || (('3' == value[3]) && (24 == byte_count)));
        }
    }
    else if (ARINC661_PARAMETER_LAYER_REQUEST == parameter->kind)
    {
        // Table 3.9.17-6: the third and fourth bytes hold the size of the request that follows
        byte_count = HexByteCount(value);
        is_valid   = !!((INVALID_HIERARCHY_INDEX != byte_count) && (4 <= byte_count));
        if (is_valid)
        {
            integer_value = ((int64_t)(strchr(c_hexDigits, value[4]) - c_hexDigits) << 12) | ((int64_t)(strchr(c_hexDigits, value[5]) - c_hexDigits) << 8) |
                            ((int64_t)(strchr(c_hexDigits, value[6]) - c_hexDigits) << 4) | (int64_t)(strchr(c_hexDigits, value[7]) - c_hexDigits);
            is_valid      = !!((byte_count == (4 + integer_value)) && (0 == (integer_value % 4)));
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Makes a widget's properties match its type's Creation Structure: properties that name
 *  no parameter are dropped (WidgetIdent is kept on widgets), every missing single
 *  parameter is added with its default, a kept value that no longer fits its parameter
 *  kind is reseeded, and every array is sized to its count.
 */
static bool_t ConformParameters(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    const Arinc661_Widget_t* widget;
    Arinc661_Parameter_t     parameter;
    char_t                   value[ARINC661_MAX_PROPERTY_BYTES];
    uint32_t                 parameter_count;
    uint32_t                 parameter_index;
    uint32_t                 property_index;
    bool_t                   is_known;
    bool_t                   is_parameter;
    bool_t                   has_changed;

    widget      = &document->widgets[widgetIndex];
    has_changed = FALSE;
    is_known    = Arinc661_Definition_FindType(widget->type, widget->isExtension, &parameter_count);
    if (is_known)
    {
        property_index = 0;
        while (property_index < document->definition.propertyCount)
        {
            is_parameter = TRUE;
            if ((ARINC661_PROPERTY_WIDGET == document->properties[property_index].owner) && (widgetIndex == document->properties[property_index].ownerIndex))
            {
                is_parameter = !!((FALSE == widget->isExtension) && (0 == strncmp(PROPERTY_WIDGET_IDENT, document->properties[property_index].name, sizeof(PROPERTY_WIDGET_IDENT))));
                for (parameter_index = 0; (FALSE == is_parameter) && (parameter_index < parameter_count); ++parameter_index)
                {
                    (void)Arinc661_Definition_GetParameter(widget->type, widget->isExtension, parameter_index, &parameter);
                    is_parameter = (0 == strncmp(parameter.name, document->properties[property_index].name, sizeof(document->properties[property_index].name)));
                }
            }
            if (is_parameter)
            {
                property_index += 1;
            }
            else
            {
                RemovePropertyAt(document, property_index);
                has_changed = TRUE;
            }
        }
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Definition_GetParameter(widget->type, widget->isExtension, parameter_index, &parameter);
            if (0 != parameter.elementsPerEntry)
            {
                continue;
            }
            property_index = FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name);
            if (INVALID_HIERARCHY_INDEX == property_index)
            {
                DefaultParameterValue(&parameter, value, sizeof(value));
                has_changed = !!(AppendProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name, value) || has_changed);
            }
            else if (FALSE == IsParameterValueValid(&parameter, document->properties[property_index].value))
            {
                // Reseed a value kept from another type whose kind it no longer fits
                DefaultParameterValue(&parameter, document->properties[property_index].value, sizeof(document->properties[property_index].value));
                has_changed = TRUE;
            }
        }
        // Size every array to what its count parameter promises
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Definition_GetParameter(widget->type, widget->isExtension, parameter_index, &parameter);
            if (0 != parameter.elementsPerEntry)
            {
                has_changed = !!(SyncArrayLength(document, widgetIndex, &parameter) || has_changed);
            }
        }
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Follows parent links from the candidate toward its layer.
 */
static bool_t IsDescendant(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, uint32_t ancestorIndex)
{
    uint32_t parent_index;
    uint32_t depth;
    bool_t   is_descendant;

    is_descendant = FALSE;
    parent_index  = document->widgets[widgetIndex].parentWidgetIndex;
    for (depth = 0; depth < document->definition.widgetCount; ++depth)
    {
        if (INVALID_HIERARCHY_INDEX == parent_index)
        {
            break;
        }
        if (ancestorIndex == parent_index)
        {
            is_descendant = TRUE;
            break;
        }
        parent_index = document->widgets[parent_index].parentWidgetIndex;
    }

    return is_descendant;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the array index following the last descendant of a widget, which is where a
 *  new child keeps parents before children and extensions directly after their widget.
 */
static uint32_t SubtreeEnd(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    uint32_t end_index;

    end_index = widgetIndex + 1;
    while ((end_index < document->definition.widgetCount) && IsDescendant(document, end_index, widgetIndex))
    {
        end_index += 1;
    }

    return end_index;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Opens a widget slot at insertIndex and renumbers every parent link and property
 *  owner at or beyond it. The new slot is zeroed.
 */
static bool_t InsertWidgetAt(Arinc661_Definition_Document_t* document, uint32_t insertIndex)
{
    Arinc661_Widget_t*   widget;
    Arinc661_Property_t* property;
    uint32_t             widget_index;
    uint32_t             property_index;
    uint32_t             moved_count;
    bool_t               has_room;

    has_room = (document->definition.widgetCount < ARINC661_DEFINITION_MAX_WIDGETS);
    if (has_room)
    {
        moved_count = (document->definition.widgetCount - insertIndex);
        (void)memmove(&document->widgets[insertIndex + 1], &document->widgets[insertIndex], moved_count * sizeof(document->widgets[0]));
        (void)memset(&document->widgets[insertIndex], 0, sizeof(document->widgets[0]));
        document->definition.widgetCount += 1;
        for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
        {
            widget = &document->widgets[widget_index];
            if ((INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex) && (insertIndex <= widget->parentWidgetIndex) && (widget_index != insertIndex))
            {
                widget->parentWidgetIndex += 1;
            }
        }
        for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
        {
            property = &document->properties[property_index];
            if ((ARINC661_PROPERTY_WIDGET == property->owner) && (insertIndex <= property->ownerIndex))
            {
                property->ownerIndex += 1;
            }
        }
    }

    return has_room;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns one more than the largest WidgetIdent in the layer, starting at one.
 */
static int64_t NextWidgetIdent(const Arinc661_Definition_Document_t* document, uint32_t layerIndex)
{
    float32_t ident;
    int64_t   maximum_ident;
    uint32_t  widget_index;

    maximum_ident = 0;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        if ((layerIndex == document->widgets[widget_index].layerIndex) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widget_index, PROPERTY_WIDGET_IDENT, &ident))
        {
            maximum_ident = ImMax(maximum_ident, (int64_t)ident);
        }
    }

    return maximum_ident + 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A new layer takes the next LayerId and the default 10000 by 10000 extent.
 */
static void AddLayer(Arinc661_Definition_Document_t* document)
{
    Arinc661_Layer_t* layer;
    char_t            text[INTEGER_TEXT_BYTES];
    uint32_t          layer_index;
    bool_t            has_room;

    has_room = !!((document->definition.layerCount < ARINC661_DEFINITION_MAX_LAYERS) && ((document->definition.propertyCount + 4) <= ARINC661_DEFINITION_MAX_PROPERTIES));
    if (has_room)
    {
        layer_index = document->definition.layerCount;
        layer       = &document->layers[layer_index];
        (void)memset(layer, 0, sizeof((*layer)));
        document->definition.layerCount += 1;
        ImFormatString(text, sizeof(text), "%u", layer_index + 1);
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_LAYER_ID, text);
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_CONTEXT_NUMBER, "0");
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_HEIGHT, DEFAULT_LAYER_SIZE);
        (void)AppendProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_WIDTH, DEFAULT_LAYER_SIZE);
        document->selectedKind  = SELECTION_LAYER;
        document->selectedIndex = layer_index;
        document->isDirty       = TRUE;
        document->status        = STATUS_LAYER_ADDED;
    }
    else
    {
        document->status = STATUS_LAYER_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A widget goes after its parent's subtree, or at the end of the array for a direct
 *  layer child; an extension goes directly after its widget and any earlier extensions,
 *  as the binary form requires. The new widget starts as a panel with the next
 *  WidgetIdent and the common properties of section 3.1.3.
 */
static void AddWidget(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, bool_t isExtension)
{
    Arinc661_Widget_t* widget;
    char_t             text[INTEGER_TEXT_BYTES];
    uint32_t           insert_index;
    uint32_t           type_index;
    bool_t             has_room;

    insert_index = document->definition.widgetCount;
    if (INVALID_HIERARCHY_INDEX != parentIndex)
    {
        insert_index = SubtreeEnd(document, parentIndex);
        if (isExtension)
        {
            insert_index = parentIndex + 1;
            while ((insert_index < document->definition.widgetCount) && document->widgets[insert_index].isExtension && (parentIndex == document->widgets[insert_index].parentWidgetIndex))
            {
                insert_index += 1;
            }
        }
    }
    has_room = ((document->definition.propertyCount + ARINC661_DEFINITION_MAX_SEED_PROPERTIES) <= ARINC661_DEFINITION_MAX_PROPERTIES);
    if (has_room)
    {
        has_room = InsertWidgetAt(document, insert_index);
    }
    if (has_room)
    {
        widget                    = &document->widgets[insert_index];
        widget->layerIndex        = layerIndex;
        widget->parentWidgetIndex = parentIndex;
        widget->isExtension       = isExtension;
        if (isExtension)
        {
            ImStrncpy(widget->type, DEFAULT_EXTENSION_TYPE, sizeof(widget->type));
            // Prefer an extension Table 8.4 allows on the parent
            for (type_index = 0; (INVALID_HIERARCHY_INDEX != parentIndex) && (type_index < Arinc661_Definition_GetTypeCount(TRUE)); ++type_index)
            {
                if (Arinc661_Definition_ExtensionApplies(Arinc661_Definition_GetTypeName(TRUE, type_index), document->widgets[parentIndex].type))
                {
                    ImStrncpy(widget->type, Arinc661_Definition_GetTypeName(TRUE, type_index), sizeof(widget->type));
                    break;
                }
            }
            document->status = STATUS_EXTENSION_ADDED;
        }
        else
        {
            ImStrncpy(widget->type, DEFAULT_WIDGET_TYPE, sizeof(widget->type));
            ImFormatString(text, sizeof(text), "%" PRId64, NextWidgetIdent(document, layerIndex));
            (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, insert_index, PROPERTY_WIDGET_IDENT, text);
            document->status = STATUS_WIDGET_ADDED;
        }
        // Seed every Creation Structure parameter of the type with its default
        (void)ConformParameters(document, insert_index);
        document->selectedKind  = SELECTION_WIDGET;
        document->selectedIndex = insert_index;
        document->isDirty       = TRUE;
    }
    else
    {
        document->status = STATUS_WIDGET_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes a widget with its descendants and extensions, drops their properties, and
 *  renumbers the surviving parent links and property owners.
 */
static void RemoveWidgetSubtree(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    Arinc661_Widget_t*   widget;
    Arinc661_Property_t* property;
    uint32_t             end_index;
    uint32_t             removed_count;
    uint32_t             widget_index;
    uint32_t             property_index;
    uint32_t             kept_count;

    end_index     = SubtreeEnd(document, widgetIndex);
    removed_count = (end_index - widgetIndex);
    // Drop the properties of every removed widget and renumber the owners after the gap
    kept_count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((ARINC661_PROPERTY_WIDGET == property->owner) && (widgetIndex <= property->ownerIndex) && (property->ownerIndex < end_index))
        {
            continue;
        }
        if ((ARINC661_PROPERTY_WIDGET == property->owner) && (end_index <= property->ownerIndex))
        {
            property->ownerIndex -= removed_count;
        }
        document->properties[kept_count] = (*property);
        kept_count += 1;
    }
    document->definition.propertyCount = kept_count;
    // Close the widget gap; descendants are contiguous after their ancestor
    (void)memmove(&document->widgets[widgetIndex], &document->widgets[end_index], (document->definition.widgetCount - end_index) * sizeof(document->widgets[0]));
    document->definition.widgetCount -= removed_count;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        widget = &document->widgets[widget_index];
        if ((INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex) && (end_index <= widget->parentWidgetIndex))
        {
            widget->parentWidgetIndex -= removed_count;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes the layer's widgets first, then the layer and its properties, then renumbers
 *  later layers.
 */
static void RemoveLayer(Arinc661_Definition_Document_t* document, uint32_t layerIndex)
{
    Arinc661_Widget_t*   widget;
    Arinc661_Property_t* property;
    uint32_t             widget_index;
    uint32_t             property_index;
    uint32_t             kept_count;

    widget_index = 0;
    while (widget_index < document->definition.widgetCount)
    {
        if (layerIndex == document->widgets[widget_index].layerIndex)
        {
            RemoveWidgetSubtree(document, widget_index);
        }
        else
        {
            widget_index += 1;
        }
    }
    kept_count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((ARINC661_PROPERTY_LAYER == property->owner) && (layerIndex == property->ownerIndex))
        {
            continue;
        }
        if ((ARINC661_PROPERTY_LAYER == property->owner) && (layerIndex < property->ownerIndex))
        {
            property->ownerIndex -= 1;
        }
        document->properties[kept_count] = (*property);
        kept_count += 1;
    }
    document->definition.propertyCount = kept_count;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        widget = &document->widgets[widget_index];
        if (layerIndex < widget->layerIndex)
        {
            widget->layerIndex -= 1;
        }
    }
    (void)memmove(&document->layers[layerIndex], &document->layers[layerIndex + 1], (document->definition.layerCount - layerIndex - 1) * sizeof(document->layers[0]));
    document->definition.layerCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies the used parts of the model into one heap block.
 */
static void* TakeSnapshot(const Arinc661_Definition_Document_t* document)
{
    Snapshot_t* snapshot;
    byte_t*     cursor;
    size_t      layer_bytes;
    size_t      widget_bytes;
    size_t      property_bytes;

    layer_bytes    = ((size_t)document->definition.layerCount * sizeof(Arinc661_Layer_t));
    widget_bytes   = ((size_t)document->definition.widgetCount * sizeof(Arinc661_Widget_t));
    property_bytes = ((size_t)document->definition.propertyCount * sizeof(Arinc661_Property_t));
    snapshot       = (Snapshot_t*)malloc(sizeof(Snapshot_t) + layer_bytes + widget_bytes + property_bytes);
    LOGASSERT(NULL != snapshot);
    snapshot->layerCount        = document->definition.layerCount;
    snapshot->widgetCount       = document->definition.widgetCount;
    snapshot->propertyCount     = document->definition.propertyCount;
    snapshot->selectedKind      = document->selectedKind;
    snapshot->selectedIndex     = document->selectedIndex;
    snapshot->libraryVersion    = document->definition.libraryVersion;
    snapshot->supplementVersion = document->definition.supplementVersion;
    snapshot->charsetEncoding   = document->definition.charsetEncoding;
    (void)memcpy(snapshot->name, document->definition.name, sizeof(snapshot->name));
    ImStrncpy(snapshot->label, HISTORY_OPEN_LABEL, sizeof(snapshot->label));
    cursor = (byte_t*)(snapshot + 1);
    (void)memcpy(cursor, document->layers, layer_bytes);
    cursor += layer_bytes;
    (void)memcpy(cursor, document->widgets, widget_bytes);
    cursor += widget_bytes;
    (void)memcpy(cursor, document->properties, property_bytes);

    return snapshot;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies a snapshot back over the model, selection included.
 */
static void RestoreSnapshot(Arinc661_Definition_Document_t* document, const void* snapshot)
{
    const Snapshot_t* header;
    const byte_t*     cursor;
    size_t            layer_bytes;
    size_t            widget_bytes;
    size_t            property_bytes;

    header         = (const Snapshot_t*)snapshot;
    layer_bytes    = ((size_t)header->layerCount * sizeof(Arinc661_Layer_t));
    widget_bytes   = ((size_t)header->widgetCount * sizeof(Arinc661_Widget_t));
    property_bytes = ((size_t)header->propertyCount * sizeof(Arinc661_Property_t));
    cursor         = (const byte_t*)(header + 1);
    (void)memcpy(document->layers, cursor, layer_bytes);
    cursor += layer_bytes;
    (void)memcpy(document->widgets, cursor, widget_bytes);
    cursor += widget_bytes;
    (void)memcpy(document->properties, cursor, property_bytes);
    document->definition.layerCount        = header->layerCount;
    document->definition.widgetCount       = header->widgetCount;
    document->definition.propertyCount     = header->propertyCount;
    document->definition.libraryVersion    = header->libraryVersion;
    document->definition.supplementVersion = header->supplementVersion;
    document->definition.charsetEncoding   = header->charsetEncoding;
    (void)memcpy(document->definition.name, header->name, sizeof(document->definition.name));
    document->selectedKind  = header->selectedKind;
    document->selectedIndex = header->selectedIndex;
    document->isDirty       = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares the counts, the header fields, and the used arrays; the selection is ignored so
 *  selecting alone never makes an undo step.
 */
static bool_t SnapshotMatches(const Arinc661_Definition_Document_t* document, const void* snapshot)
{
    const Snapshot_t* header;
    const byte_t*     cursor;
    size_t            layer_bytes;
    size_t            widget_bytes;
    size_t            property_bytes;
    bool_t            is_same;

    header  = (const Snapshot_t*)snapshot;
    is_same = !!((header->layerCount == document->definition.layerCount) && (header->widgetCount == document->definition.widgetCount) && (header->propertyCount == document->definition.propertyCount) &&
                 (header->libraryVersion == document->definition.libraryVersion) && (header->supplementVersion == document->definition.supplementVersion) && (header->charsetEncoding == document->definition.charsetEncoding) &&
                 (0 == memcmp(header->name, document->definition.name, sizeof(header->name))));
    if (is_same)
    {
        layer_bytes    = ((size_t)header->layerCount * sizeof(Arinc661_Layer_t));
        widget_bytes   = ((size_t)header->widgetCount * sizeof(Arinc661_Widget_t));
        property_bytes = ((size_t)header->propertyCount * sizeof(Arinc661_Property_t));
        cursor         = (const byte_t*)(header + 1);
        is_same        = !!((0 == memcmp(document->layers, cursor, layer_bytes)) && (0 == memcmp(document->widgets, cursor + layer_bytes, widget_bytes)) && (0 == memcmp(document->properties, cursor + layer_bytes + widget_bytes, property_bytes)));
    }

    return is_same;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Frees every snapshot.
 */
static void ClearHistory(Arinc661_Definition_Document_t* document)
{
    uint32_t index;

    for (index = 0; index < document->undoCount; ++index)
    {
        free(document->undoStack[index]);
    }
    for (index = 0; index < document->redoCount; ++index)
    {
        free(document->redoStack[index]);
    }
    free(document->shadow);
    document->shadow    = NULL;
    document->undoCount = 0;
    document->redoCount = 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Called once per frame: when the model differs from the shadow, the shadow becomes an undo
 *  step, the redo stack is dropped, and a fresh shadow is taken. The oldest step goes when
 *  the stack is full.
 */
static void CommitHistory(Arinc661_Definition_Document_t* document)
{
    char_t label[HISTORY_LABEL_BYTES];

    if (document->isLoaded)
    {
        if (NULL == document->shadow)
        {
            document->shadow = TakeSnapshot(document);
        }
        else if (FALSE == SnapshotMatches(document, document->shadow))
        {
            DescribeChange(document, (const Snapshot_t*)document->shadow, label, sizeof(label));
            if (ARINC661_DEFINITION_MAX_HISTORY == document->undoCount)
            {
                free(document->undoStack[0]);
                (void)memmove(&document->undoStack[0], &document->undoStack[1], (ARINC661_DEFINITION_MAX_HISTORY - 1) * sizeof(document->undoStack[0]));
                document->undoCount -= 1;
            }
            document->undoStack[document->undoCount] = document->shadow;
            document->undoCount += 1;
            while (0 < document->redoCount)
            {
                document->redoCount -= 1;
                free(document->redoStack[document->redoCount]);
            }
            document->shadow = TakeSnapshot(document);
            ImStrncpy(((Snapshot_t*)document->shadow)->label, label, HISTORY_LABEL_BYTES);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares counts first, then the first property whose value differs, so the label names the
 *  widget and the parameter a crew of editors would recognise in a history list.
 */
static void DescribeChange(const Arinc661_Definition_Document_t* document, const Snapshot_t* before, char_t* label, uint32_t capacity)
{
    const Arinc661_Property_t* old_properties;
    const Arinc661_Widget_t*   old_widgets;
    const byte_t*              cursor;
    uint32_t                   index;
    uint32_t                   owner_index;

    cursor         = (const byte_t*)(before + 1);
    cursor        += ((size_t)before->layerCount * sizeof(Arinc661_Layer_t));
    old_widgets    = (const Arinc661_Widget_t*)cursor;
    cursor        += ((size_t)before->widgetCount * sizeof(Arinc661_Widget_t));
    old_properties = (const Arinc661_Property_t*)cursor;
    if (before->layerCount != document->definition.layerCount)
    {
        ImStrncpy(label, HISTORY_LAYER_LABEL, (int)capacity);
    }
    else if (before->widgetCount < document->definition.widgetCount)
    {
        // The first widget whose name differs from the old array is the addition
        for (index = 0; index < before->widgetCount; ++index)
        {
            if (0 != strncmp(old_widgets[index].name, document->widgets[index].name, sizeof(document->widgets[index].name)))
            {
                break;
            }
        }
        ImFormatString(label, capacity, HISTORY_ADD_FORMAT, document->widgets[index].name);
    }
    else if (before->widgetCount > document->definition.widgetCount)
    {
        ImFormatString(label, capacity, HISTORY_REMOVE_FORMAT, (unsigned)(before->widgetCount - document->definition.widgetCount));
    }
    else
    {
        ImStrncpy(label, HISTORY_MOVE_LABEL, (int)capacity);
        for (index = 0; (index < before->propertyCount) && (index < document->definition.propertyCount); ++index)
        {
            if ((0 != strncmp(old_properties[index].value, document->properties[index].value, sizeof(document->properties[index].value))) || (0 != strncmp(old_properties[index].name, document->properties[index].name, sizeof(document->properties[index].name))))
            {
                owner_index = document->properties[index].ownerIndex;
                if ((ARINC661_PROPERTY_WIDGET == document->properties[index].owner) && (owner_index < document->definition.widgetCount))
                {
                    ImFormatString(label, capacity, HISTORY_EDIT_FORMAT, document->properties[index].name, document->widgets[owner_index].name);
                }
                else if (ARINC661_PROPERTY_LAYER == document->properties[index].owner)
                {
                    ImFormatString(label, capacity, HISTORY_EDIT_LAYER_FORMAT, document->properties[index].name);
                }
                else
                {
                    ImStrncpy(label, HISTORY_HEADER_LABEL, (int)capacity);
                }
                break;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The current state moves to the redo stack; the newest undo step becomes the model and
 *  the shadow.
 */
static void UndoDocument(Arinc661_Definition_Document_t* document)
{
    CommitHistory(document);
    if (0 < document->undoCount)
    {
        if (ARINC661_DEFINITION_MAX_HISTORY == document->redoCount)
        {
            free(document->redoStack[0]);
            (void)memmove(&document->redoStack[0], &document->redoStack[1], (ARINC661_DEFINITION_MAX_HISTORY - 1) * sizeof(document->redoStack[0]));
            document->redoCount -= 1;
        }
        document->redoStack[document->redoCount] = document->shadow;
        document->redoCount += 1;
        document->undoCount -= 1;
        document->shadow = document->undoStack[document->undoCount];
        RestoreSnapshot(document, document->shadow);
        document->status = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Mirror of UndoDocument.
 */
static void RedoDocument(Arinc661_Definition_Document_t* document)
{
    CommitHistory(document);
    if (0 < document->redoCount)
    {
        document->undoStack[document->undoCount] = document->shadow;
        document->undoCount += 1;
        document->redoCount -= 1;
        document->shadow = document->redoStack[document->redoCount];
        RestoreSnapshot(document, document->shadow);
        document->status = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Table 3.2.3.1: the widgets that can be a parent.
 */
static bool_t IsContainerType(const char_t* type)
{
    static const char_t* const c_containers[] =
    {
        "A661_ANIMATION_GROUP", "A661_BASIC_CONTAINER", "A661_BLINKING_CONTAINER", "A661_FRAMING_REFRESH_CONTAINER", "A661_MAP_BOUNDARY", "A661_MAPHORZ",
        "A661_MAPHORZ_CONTAINER", "A661_MAPHORZ_PANEL", "A661_MAPHORZ_SOURCE", "A661_MAPVERT", "A661_MAPVERT_CONTAINER", "A661_MAPVERT_PANEL", "A661_MAPVERT_SOURCE",
        "A661_MASK_CONTAINER", "A661_MENU_BAR", "A661_MUTUALLY_EXCLUSIVE_CONTAINER", "A661_NO_SERVICE_MONITOR", "A661_PAGING_CONTAINER", "A661_PANEL", "A661_POP_UP_PANEL",
        "A661_RADIO_BOX", "A661_ROTATION_CONTAINER", "A661_SCALE_CONTAINER", "A661_SCROLL_PANEL", "A661_SHUFFLE_TO_FIT_CONTAINER", "A661_SIZE_TO_FIT_CONTAINER",
        "A661_TABBED_PANEL", "A661_TABBED_PANEL_GROUP", "A661_TRANSLATION_CONTAINER", "A661_WATCHDOG_CONTAINER"
    };
    uint32_t index;
    bool_t   is_container;

    is_container = FALSE;
    for (index = 0; index < COUNT_OF(c_containers); ++index)
    {
        if (0 == strncmp(type, c_containers[index], ARINC661_MAX_NAME_BYTES))
        {
            is_container = TRUE;
            break;
        }
    }

    return is_container;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes an integer value into an existing property, or appends it.
 */
static void SetPropertyNumber(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, float32_t value)
{
    char_t   text[INTEGER_TEXT_BYTES];
    uint32_t property_index;

    ImFormatString(text, sizeof(text), "%" PRId64, (int64_t)ImFloor(value + 0.5f));
    property_index = FindOwnedProperty(document, owner, ownerIndex, name);
    if (INVALID_HIERARCHY_INDEX != property_index)
    {
        ImStrncpy(document->properties[property_index].value, text, sizeof(document->properties[property_index].value));
    }
    else
    {
        (void)AppendProperty(document, owner, ownerIndex, name, text);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Sums PosX and PosY from the widget up through its containers, skipping containers
 *  without a reference point as section 3.2.3 says.
 */
static void AbsoluteOrigin(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, float32_t* x, float32_t* y)
{
    float32_t position_x;
    float32_t position_y;
    uint32_t  index;
    uint32_t  depth;

    (*x)  = 0.0f;
    (*y)  = 0.0f;
    index = widgetIndex;
    for (depth = 0; (INVALID_HIERARCHY_INDEX != index) && (depth < document->definition.widgetCount); ++depth)
    {
        if (GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_X, &position_x) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_Y, &position_y))
        {
            (*x) += position_x;
            (*y) += position_y;
        }
        index = document->widgets[index].parentWidgetIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies the widget, its extensions, and its descendants directly after the source
 *  subtree, remapping parents into the copy and giving every copied widget a fresh
 *  WidgetIdent. The copy of the root is selected.
 *
 * \retval uint32_t   Index of the copied root, or INVALID_HIERARCHY_INDEX when out of room.
 */
static uint32_t DuplicateWidget(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    char_t   text[INTEGER_TEXT_BYTES];
    uint32_t source_end;
    uint32_t source_count;
    uint32_t source;
    uint32_t destination;
    uint32_t property_index;
    uint32_t property_count;
    uint32_t parent;
    uint32_t root_copy;
    bool_t   has_room;

    source_end   = SubtreeEnd(document, widgetIndex);
    source_count = (source_end - widgetIndex);
    has_room     = ((document->definition.widgetCount + source_count) <= ARINC661_DEFINITION_MAX_WIDGETS);
    root_copy    = INVALID_HIERARCHY_INDEX;
    if (has_room)
    {
        property_count = 0;
        for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
        {
            if ((ARINC661_PROPERTY_WIDGET == document->properties[property_index].owner) && (widgetIndex <= document->properties[property_index].ownerIndex) && (document->properties[property_index].ownerIndex < source_end))
            {
                property_count += 1;
            }
        }
        has_room = ((document->definition.propertyCount + property_count + source_count) <= ARINC661_DEFINITION_MAX_PROPERTIES);
    }
    if (has_room)
    {
        // Sources sit before the insertion point, so their indices stay put while copies are appended
        for (source = widgetIndex; source < source_end; ++source)
        {
            destination = (source_end + (source - widgetIndex));
            if (FALSE == InsertWidgetAt(document, destination))
            {
                break;
            }
            document->widgets[destination] = document->widgets[source];
            parent                         = document->widgets[source].parentWidgetIndex;
            if ((widgetIndex <= parent) && (parent < source_end))
            {
                document->widgets[destination].parentWidgetIndex = (source_end + (parent - widgetIndex));
            }
            if (source == widgetIndex)
            {
                root_copy = destination;
                if (FALSE == document->widgets[destination].isExtension)
                {
                    if ((strlen(document->widgets[destination].name) + strlen(COPY_SUFFIX)) < sizeof(document->widgets[destination].name))
                    {
                        strcat(document->widgets[destination].name, COPY_SUFFIX);
                    }
                }
            }
            property_count = document->definition.propertyCount;
            for (property_index = 0; property_index < property_count; ++property_index)
            {
                if ((ARINC661_PROPERTY_WIDGET == document->properties[property_index].owner) && (source == document->properties[property_index].ownerIndex))
                {
                    if (0 == strncmp(PROPERTY_WIDGET_IDENT, document->properties[property_index].name, sizeof(PROPERTY_WIDGET_IDENT)))
                    {
                        ImFormatString(text, sizeof(text), "%" PRId64, NextWidgetIdent(document, document->widgets[source].layerIndex));
                        (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, destination, PROPERTY_WIDGET_IDENT, text);
                    }
                    else
                    {
                        (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, destination, document->properties[property_index].name, document->properties[property_index].value);
                    }
                }
            }
        }
        if (INVALID_HIERARCHY_INDEX != root_copy)
        {
            document->selectedKind  = SELECTION_WIDGET;
            document->selectedIndex = root_copy;
            document->isDirty       = TRUE;
            document->status        = STATUS_WIDGET_ADDED;
        }
    }
    else
    {
        document->status = STATUS_WIDGET_ADD_FAILED;
    }

    return root_copy;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes whatever is selected and selects its parent.
 */
static void RemoveSelection(Arinc661_Definition_Document_t* document)
{
    uint32_t parent_index;
    uint32_t index;
    uint32_t count;

    count = SelectedCount(document);
    if ((SELECTION_WIDGET == document->selectedKind) && (1 < count))
    {
        // Remove the multi-selection from the end so earlier indices stay valid
        for (index = document->definition.widgetCount; 0 < index; --index)
        {
            if ((index - 1) < document->definition.widgetCount && document->isSelected[index - 1] && (FALSE == document->isLocked[index - 1]))
            {
                RemoveWidgetSubtree(document, index - 1);
            }
        }
        ClearSelectionFlags(document);
        document->selectedKind  = SELECTION_LAYER;
        document->selectedIndex = 0;
        document->isDirty       = TRUE;
        document->status        = STATUS_WIDGET_REMOVED;
        return;
    }
    if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
    {
        parent_index            = document->widgets[document->selectedIndex].parentWidgetIndex;
        document->status        = STATUS_WIDGET_REMOVED;
        if (document->widgets[document->selectedIndex].isExtension)
        {
            document->status = STATUS_EXTENSION_REMOVED;
        }
        RemoveWidgetSubtree(document, document->selectedIndex);
        document->selectedKind  = SELECTION_LAYER;
        document->selectedIndex = 0;
        if (INVALID_HIERARCHY_INDEX != parent_index)
        {
            document->selectedKind  = SELECTION_WIDGET;
            document->selectedIndex = parent_index;
        }
        document->isDirty = TRUE;
    }
    else if ((SELECTION_LAYER == document->selectedKind) && (document->selectedIndex < document->definition.layerCount))
    {
        RemoveLayer(document, document->selectedIndex);
        document->selectedKind  = SELECTION_DEFINITION;
        document->selectedIndex = 0;
        document->isDirty       = TRUE;
        document->status        = STATUS_LAYER_REMOVED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An extension type attaches to the parent widget; a widget type goes into the parent when
 *  it is a container the rules allow, else beside it. The reference point lands at the given
 *  layer position when the type has one and hasPosition is set.
 */
static void AddTypedWidget(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, const char_t* type, float32_t x, float32_t y, bool_t hasPosition)
{
    float32_t parent_x;
    float32_t parent_y;
    uint32_t  new_index;
    bool_t    is_extension;

    is_extension = (0 < Arinc661_Definition_FindType(type, TRUE, &new_index));
    if (is_extension)
    {
        if (INVALID_HIERARCHY_INDEX != parentIndex)
        {
            AddWidget(document, layerIndex, parentIndex, TRUE);
            if ((SELECTION_WIDGET == document->selectedKind) && document->widgets[document->selectedIndex].isExtension)
            {
                ImStrncpy(document->widgets[document->selectedIndex].type, type, sizeof(document->widgets[document->selectedIndex].type));
                (void)ConformParameters(document, document->selectedIndex);
            }
        }
    }
    else
    {
        if ((INVALID_HIERARCHY_INDEX != parentIndex) && ((FALSE == IsContainerType(document->widgets[parentIndex].type)) || (FALSE == IsParentAllowed(type, document->widgets[parentIndex].type))))
        {
            parentIndex = document->widgets[parentIndex].parentWidgetIndex;
        }
        AddWidget(document, layerIndex, parentIndex, FALSE);
        if ((SELECTION_WIDGET == document->selectedKind) && (0 == strncmp(DEFAULT_WIDGET_TYPE, document->widgets[document->selectedIndex].type, sizeof(DEFAULT_WIDGET_TYPE))))
        {
            new_index = document->selectedIndex;
            ImStrncpy(document->widgets[new_index].type, type, sizeof(document->widgets[new_index].type));
            (void)ConformParameters(document, new_index);
            if (hasPosition && (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, new_index, PROPERTY_POS_X)))
            {
                parent_x = 0.0f;
                parent_y = 0.0f;
                if (INVALID_HIERARCHY_INDEX != parentIndex)
                {
                    AbsoluteOrigin(document, parentIndex, &parent_x, &parent_y);
                }
                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, new_index, PROPERTY_POS_X, x - parent_x);
                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, new_index, PROPERTY_POS_Y, y - parent_y);
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Linear scan of the parameter table.
 */
static ReferenceKind_t ReferenceKindOf(const char_t* parameterName)
{
    ReferenceKind_t kind;
    uint32_t        index;

    kind = REFERENCE_NONE;
    for (index = 0; index < COUNT_OF(c_referenceParameters); ++index)
    {
        if (0 == strncmp(parameterName, c_referenceParameters[index].name, ARINC661_MAX_NAME_BYTES))
        {
            kind = c_referenceParameters[index].kind;
            break;
        }
    }

    return kind;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Extensions carry no WidgetIdent and are skipped.
 */
static uint32_t FindWidgetByIdent(const Arinc661_Definition_Document_t* document, uint32_t layerIndex, int64_t ident)
{
    uint32_t  widget_index;
    uint32_t  found;
    float32_t number;

    found = INVALID_HIERARCHY_INDEX;
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        if ((layerIndex == document->widgets[widget_index].layerIndex) && (FALSE == document->widgets[widget_index].isExtension) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widget_index, PROPERTY_WIDGET_IDENT, &number) && ((int64_t)number == ident))
        {
            found = widget_index;
            break;
        }
    }

    return found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares the LayerId property numerically.
 */
static uint32_t FindLayerById(const Arinc661_Definition_Document_t* document, int64_t layerId)
{
    uint32_t  layer_index;
    uint32_t  found;
    float32_t number;

    found = INVALID_HIERARCHY_INDEX;
    for (layer_index = 0; layer_index < document->definition.layerCount; ++layer_index)
    {
        if (GetPropertyNumber(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_LAYER_ID, &number) && ((int64_t)number == layerId))
        {
            found = layer_index;
            break;
        }
    }

    return found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Zero names nothing and is always acceptable. Widgets are looked up in the holder's layer,
 *  layers in this file, user applications and external sources through the context, and
 *  FocusIn targets in the file whose ApplicationId equals the holder's NextFocusInAppId.
 */
static bool_t ResolveReference(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, ReferenceKind_t kind, const char_t* value, uint32_t* targetDocument, uint32_t* targetIndex, char_t* label, uint32_t labelCapacity)
{
    Arinc661_Context_t*                   context;
    const Arinc661_Definition_Document_t* target;
    Arinc661_Context_UserApplication_t    application;
    const char_t*                         source_name;
    const char_t*                         source_type;
    int64_t                               ident;
    float32_t                             number;
    uint32_t                              index;
    uint32_t                              other_document;
    uint32_t                              layer_index;
    uint32_t                              source_id;
    bool_t                                is_valid;

    context          = document->owner->context;
    (*targetDocument) = INVALID_DOCUMENT_INDEX;
    (*targetIndex)    = INVALID_HIERARCHY_INDEX;
    ImStrncpy(label, REFERENCE_NONE_LABEL, (int)labelCapacity);
    is_valid = (0 < FACEOE_Utl_Scan_Int64(value, &ident));
    if (is_valid && (0 == ident))
    {
        is_valid = TRUE;
    }
    else if (is_valid)
    {
        is_valid = FALSE;
        switch (kind)
        {
            case REFERENCE_WIDGET:
                index = FindWidgetByIdent(document, document->widgets[widgetIndex].layerIndex, ident);
                if (INVALID_HIERARCHY_INDEX != index)
                {
                    (*targetDocument) = (uint32_t)(document - document->owner->documents);
                    (*targetIndex)    = index;
                    ImFormatString(label, labelCapacity, REFERENCE_WIDGET_FORMAT, (long long)ident, document->widgets[index].name, document->widgets[index].type);
                    is_valid = TRUE;
                }
                break;
            case REFERENCE_LAYER:
                index = FindLayerById(document, ident);
                if (INVALID_HIERARCHY_INDEX != index)
                {
                    (*targetDocument) = (uint32_t)(document - document->owner->documents);
                    (*targetIndex)    = index;
                    ImFormatString(label, labelCapacity, REFERENCE_LAYER_FORMAT, LayerIdText(document, index), document->layers[index].name);
                    is_valid = TRUE;
                }
                break;
            case REFERENCE_APPLICATION:
                for (index = 0; (NULL != context) && Arinc661_Context_UserApplication(context, index, &application); ++index)
                {
                    if (application.applicationId == ident)
                    {
                        (*targetDocument) = application.documentIndex;
                        (*targetIndex)    = application.applicationIndex;
                        ImFormatString(label, labelCapacity, REFERENCE_APPLICATION_FORMAT, (long long)ident, application.dFPath, application.documentName);
                        is_valid = TRUE;
                        break;
                    }
                }
                break;
            case REFERENCE_FOCUS_IN:
                // Follow NextFocusInAppId to the other file, NextFocusInLayerId to its layer, then the ident
                other_document = INVALID_DOCUMENT_INDEX;
                if ((NULL != context) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "NextFocusInAppId", &number))
                {
                    other_document = Arinc661_Context_DefinitionByApplicationId(context, (int64_t)number);
                }
                if ((INVALID_DOCUMENT_INDEX != other_document) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "NextFocusInLayerId", &number))
                {
                    target      = &document->owner->documents[other_document];
                    layer_index = FindLayerById(target, (int64_t)number);
                    if (INVALID_HIERARCHY_INDEX != layer_index)
                    {
                        index = FindWidgetByIdent(target, layer_index, ident);
                        if ((INVALID_HIERARCHY_INDEX != index) && (0 == strncmp("A661_FOCUS_IN", target->widgets[index].type, sizeof("A661_FOCUS_IN"))))
                        {
                            (*targetDocument) = other_document;
                            (*targetIndex)    = index;
                            ImFormatString(label, labelCapacity, REFERENCE_WIDGET_FORMAT, (long long)ident, target->widgets[index].name, target->documentName);
                            is_valid = TRUE;
                        }
                    }
                }
                break;
            case REFERENCE_SOURCE:
                for (other_document = 0; (NULL != context) && (other_document < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && (FALSE == is_valid); ++other_document)
                {
                    for (index = 0; Arinc661_Configuration_ExternalSourceAt(context->configurationEditor, other_document, index, &source_id, &source_name, &source_type); ++index)
                    {
                        if ((int64_t)source_id == ident)
                        {
                            (*targetDocument) = other_document;
                            (*targetIndex)    = index;
                            ImFormatString(label, labelCapacity, REFERENCE_SOURCE_FORMAT, (unsigned)source_id, source_name, source_type);
                            is_valid = TRUE;
                            break;
                        }
                    }
                }
                break;
            default:
                is_valid = TRUE;
                break;
        }
        if (FALSE == is_valid)
        {
            ImFormatString(label, labelCapacity, REFERENCE_MISSING_FORMAT, value);
        }
    }
    else
    {
        ImFormatString(label, labelCapacity, REFERENCE_MISSING_FORMAT, value);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A combo lists what the parameter may name, with the resolved target as its preview, in
 *  red when nothing matches. The Go button selects the target: a widget or layer in its
 *  file, a user application or external source in its configuration.
 */
static bool_t DrawReferenceValue(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, ReferenceKind_t kind, char_t* value, uint32_t capacity)
{
    Arinc661_Context_t*                   context;
    const Arinc661_Definition_Document_t* target;
    Arinc661_Context_UserApplication_t    application;
    const char_t*                         source_name;
    const char_t*                         source_type;
    const char_t*                         tooltip;
    char_t                                label[REFERENCE_TEXT_BYTES];
    char_t                                item[REFERENCE_TEXT_BYTES];
    char_t                                text[INTEGER_TEXT_BYTES];
    float32_t                             number;
    int64_t                               ident;
    uint32_t                              target_document;
    uint32_t                              target_index;
    uint32_t                              index;
    uint32_t                              other_document;
    uint32_t                              layer_index;
    uint32_t                              source_id;
    bool_t                                is_valid;
    bool_t                                has_changed;

    context     = document->owner->context;
    has_changed = FALSE;
    is_valid    = ResolveReference(document, widgetIndex, kind, value, &target_document, &target_index, label, sizeof(label));
    if (FALSE == is_valid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
    }
    ImGui::SetNextItemWidth(-(ImGui::CalcTextSize(REFERENCE_GO_LABEL).x + (3.0f * ImGui::GetStyle().FramePadding.x) + ImGui::GetStyle().ItemSpacing.x));
    if (ImGui::BeginCombo(REFERENCE_COMBO_ID, label))
    {
        if (ImGui::Selectable(REFERENCE_NONE_LABEL))
        {
            ImStrncpy(value, "0", (int)capacity);
            has_changed = TRUE;
        }
        switch (kind)
        {
            case REFERENCE_WIDGET:
                for (index = 0; index < document->definition.widgetCount; ++index)
                {
                    if ((index != widgetIndex) && (document->widgets[index].layerIndex == document->widgets[widgetIndex].layerIndex) && (FALSE == document->widgets[index].isExtension) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_WIDGET_IDENT, &number))
                    {
                        ImFormatString(item, sizeof(item), REFERENCE_WIDGET_FORMAT, (long long)number, document->widgets[index].name, document->widgets[index].type);
                        ImGui::PushID((int)index);
                        if (ImGui::Selectable(item))
                        {
                            ImFormatString(text, sizeof(text), "%lld", (long long)number);
                            ImStrncpy(value, text, (int)capacity);
                            has_changed = TRUE;
                        }
                        ImGui::PopID();
                    }
                }
                break;
            case REFERENCE_LAYER:
                for (index = 0; index < document->definition.layerCount; ++index)
                {
                    if (GetPropertyNumber(document, ARINC661_PROPERTY_LAYER, index, PROPERTY_LAYER_ID, &number))
                    {
                        ImFormatString(item, sizeof(item), REFERENCE_LAYER_FORMAT, LayerIdText(document, index), document->layers[index].name);
                        ImGui::PushID((int)index);
                        if (ImGui::Selectable(item))
                        {
                            ImFormatString(text, sizeof(text), "%lld", (long long)number);
                            ImStrncpy(value, text, (int)capacity);
                            has_changed = TRUE;
                        }
                        ImGui::PopID();
                    }
                }
                break;
            case REFERENCE_APPLICATION:
                for (index = 0; (NULL != context) && Arinc661_Context_UserApplication(context, index, &application); ++index)
                {
                    ImFormatString(item, sizeof(item), REFERENCE_APPLICATION_FORMAT, (long long)application.applicationId, application.dFPath, application.documentName);
                    ImGui::PushID((int)index);
                    if (ImGui::Selectable(item))
                    {
                        ImFormatString(text, sizeof(text), "%lld", (long long)application.applicationId);
                        ImStrncpy(value, text, (int)capacity);
                        has_changed = TRUE;
                    }
                    ImGui::PopID();
                }
                break;
            case REFERENCE_FOCUS_IN:
                other_document = INVALID_DOCUMENT_INDEX;
                if ((NULL != context) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "NextFocusInAppId", &number))
                {
                    other_document = Arinc661_Context_DefinitionByApplicationId(context, (int64_t)number);
                }
                if ((INVALID_DOCUMENT_INDEX != other_document) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "NextFocusInLayerId", &number))
                {
                    target      = &document->owner->documents[other_document];
                    layer_index = FindLayerById(target, (int64_t)number);
                    for (index = 0; (INVALID_HIERARCHY_INDEX != layer_index) && (index < target->definition.widgetCount); ++index)
                    {
                        if ((target->widgets[index].layerIndex == layer_index) && (0 == strncmp("A661_FOCUS_IN", target->widgets[index].type, sizeof("A661_FOCUS_IN"))) && GetPropertyNumber(target, ARINC661_PROPERTY_WIDGET, index, PROPERTY_WIDGET_IDENT, &number))
                        {
                            ImFormatString(item, sizeof(item), REFERENCE_WIDGET_FORMAT, (long long)number, target->widgets[index].name, target->documentName);
                            ImGui::PushID((int)index);
                            if (ImGui::Selectable(item))
                            {
                                ImFormatString(text, sizeof(text), "%lld", (long long)number);
                                ImStrncpy(value, text, (int)capacity);
                                has_changed = TRUE;
                            }
                            ImGui::PopID();
                        }
                    }
                }
                break;
            case REFERENCE_SOURCE:
                for (other_document = 0; (NULL != context) && (other_document < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS); ++other_document)
                {
                    for (index = 0; Arinc661_Configuration_ExternalSourceAt(context->configurationEditor, other_document, index, &source_id, &source_name, &source_type); ++index)
                    {
                        ImFormatString(item, sizeof(item), REFERENCE_SOURCE_FORMAT, (unsigned)source_id, source_name, source_type);
                        ImGui::PushID((int)((other_document * 4096) + index));
                        if (ImGui::Selectable(item))
                        {
                            ImFormatString(text, sizeof(text), "%u", (unsigned)source_id);
                            ImStrncpy(value, text, (int)capacity);
                            has_changed = TRUE;
                        }
                        ImGui::PopID();
                    }
                }
                break;
            default:
                break;
        }
        ImGui::EndCombo();
    }
    if (FALSE == is_valid)
    {
        ImGui::PopStyleColor();
    }
    tooltip = REFERENCE_TOOLTIP_WIDGET;
    if (REFERENCE_LAYER == kind)
    {
        tooltip = REFERENCE_TOOLTIP_LAYER;
    }
    else if (REFERENCE_APPLICATION == kind)
    {
        tooltip = REFERENCE_TOOLTIP_APPLICATION;
    }
    else if (REFERENCE_FOCUS_IN == kind)
    {
        tooltip = REFERENCE_TOOLTIP_FOCUS_IN;
    }
    else if (REFERENCE_SOURCE == kind)
    {
        tooltip = REFERENCE_TOOLTIP_SOURCE;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!!((FALSE == is_valid) || (INVALID_HIERARCHY_INDEX == target_index)));
    if (ImGui::SmallButton(REFERENCE_GO_LABEL))
    {
        switch (kind)
        {
            case REFERENCE_WIDGET:
            case REFERENCE_FOCUS_IN:
                Arinc661_Definition_SelectWidget(document->owner, target_document, target_index);
                break;
            case REFERENCE_LAYER:
                Arinc661_Definition_SelectLayer(document->owner, target_document, target_index);
                break;
            case REFERENCE_APPLICATION:
                if (NULL != context)
                {
                    Arinc661_Configuration_SelectUserApplication(context->configurationEditor, target_document, target_index);
                    context->isDefinitionFocused = FALSE;
                    context->hasFocusRequest     = TRUE;
                }
                break;
            case REFERENCE_SOURCE:
                if (NULL != context)
                {
                    Arinc661_Configuration_SelectExternalSource(context->configurationEditor, target_document, target_index);
                    context->isDefinitionFocused = FALSE;
                    context->hasFocusRequest     = TRUE;
                }
                break;
            default:
                break;
        }
    }
    ImGui::EndDisabled();
    (void)ident;

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Types without a rule may sit anywhere; a forbidden rule lists the parents that are not
 *  allowed.
 */
static bool_t IsParentAllowed(const char_t* childType, const char_t* parentType)
{
    const ParentRule_t* rule;
    uint32_t            rule_index;
    uint32_t            parent_index;
    bool_t              is_listed;
    bool_t              is_allowed;

    is_allowed = TRUE;
    for (rule_index = 0; rule_index < COUNT_OF(c_parentRules); ++rule_index)
    {
        rule = &c_parentRules[rule_index];
        if (0 != strncmp(childType, rule->child, ARINC661_MAX_NAME_BYTES))
        {
            continue;
        }
        is_listed = FALSE;
        for (parent_index = 0; (parent_index < 4) && (NULL != rule->parents[parent_index]); ++parent_index)
        {
            if (NULL == parentType)
            {
                if (0 == strncmp(rule->parents[parent_index], PARENT_LAYER_NAME, sizeof(PARENT_LAYER_NAME)))
                {
                    is_listed = TRUE;
                }
            }
            else if (0 == strncmp(rule->parents[parent_index], parentType, ARINC661_MAX_NAME_BYTES))
            {
                is_listed = TRUE;
            }
        }
        if (rule->isForbidden)
        {
            is_allowed = (FALSE == is_listed);
        }
        else
        {
            is_allowed = is_listed;
        }
        break;
    }
    // A TabbedPanelGroup takes only TabbedPanel or Connector children
    if (is_allowed && (NULL != parentType) && (0 == strncmp(parentType, "A661_TABBED_PANEL_GROUP", sizeof("A661_TABBED_PANEL_GROUP"))) && (0 != strncmp(childType, "A661_TABBED_PANEL", sizeof("A661_TABBED_PANEL"))) && (0 != strncmp(childType, "A661_CONNECTOR", sizeof("A661_CONNECTOR"))))
    {
        is_allowed = FALSE;
    }

    return is_allowed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the specification's description, categories, and restriction of a type, wrapped.
 */
static void DrawTypeHelp(const char_t* type)
{
    const char_t* description;
    const char_t* categories;
    const char_t* restriction;
    char_t        label[LABEL_FORMAT_BYTES + ARINC661_MAX_NAME_BYTES];

    if (Arinc661_Help_GetType(type, &description, &categories, &restriction))
    {
        ImFormatString(label, sizeof(label), ABOUT_FORMAT, type);
        if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(description);
            if ('\0' != categories[0])
            {
                ImGui::TextDisabled(CATEGORIES_FORMAT, categories);
            }
            if ('\0' != restriction[0])
            {
                ImGui::TextDisabled(RESTRICTION_FORMAT, restriction);
            }
            ImGui::PopTextWrapPos();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Hovering a parameter name shows the Parameters table row: the change flag and the
 *  description from the type's section.
 */
static void DrawParameterTooltip(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* propertyName)
{
    const char_t* description;
    const char_t* change;
    const char_t* type;

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        type = "";
        if (ARINC661_PROPERTY_WIDGET == owner)
        {
            type = document->widgets[ownerIndex].type;
        }
        description = Arinc661_Help_GetParameter(type, propertyName, &change);
        if ('\0' != description[0])
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(HELP_WRAP_WIDTH);
            ImGui::Text(CHANGE_FORMAT, change, description);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Right-click menu shared by hierarchy nodes and the scene: add a child, add an extension,
 *  duplicate, or remove.
 */
static void DrawWidgetContextMenu(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, HierarchyRequest_t* request)
{
    const Arinc661_Widget_t* widget;

    widget = &document->widgets[widgetIndex];
    if (FALSE == widget->isExtension)
    {
        if (ImGui::MenuItem(ADD_CHILD_WIDGET_LABEL, NULL, false, IsContainerType(widget->type)))
        {
            request->addWidgetLayerIndex  = widget->layerIndex;
            request->addWidgetParentIndex = widgetIndex;
        }
        if (ImGui::MenuItem(ADD_EXTENSION_MENU_LABEL))
        {
            request->addExtensionParentIndex = widgetIndex;
        }
    }
    if (ImGui::MenuItem(DUPLICATE_LABEL))
    {
        request->duplicateWidgetIndex = widgetIndex;
    }
    if (ImGui::MenuItem(COPY_LABEL, SHORTCUT_CTRL_C))
    {
        SelectOnly(document, widgetIndex);
        (void)Arinc661_Definition_Copy(document->owner);
    }
    if (ImGui::MenuItem(PASTE_LABEL, SHORTCUT_CTRL_V, false, 0 < document->owner->clipboardWidgetCount))
    {
        SelectOnly(document, widgetIndex);
        (void)Arinc661_Definition_Paste(document->owner);
    }
    if ((FALSE == widget->isExtension) && ImGui::MenuItem(RENAME_LABEL, SHORTCUT_F2))
    {
        document->owner->renamingWidget = widgetIndex;
        ImStrncpy(document->owner->renameBuffer, widget->name, sizeof(document->owner->renameBuffer));
    }
    if (ImGui::MenuItem(REMOVE_LABEL))
    {
        request->removeWidgetIndex = widgetIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The pop-up opened by a right-click on the scene: a filter box, the matching widget types
 *  with their descriptions as tooltips, and the actions on the widget under the cursor.
 *  Choosing a type adds the widget to the container under the cursor at that point.
 */
static void DrawAddPopup(Arinc661_Definition_Document_t* document)
{
    Arinc661_Definition_Editor_t* owner;
    HierarchyRequest_t            request;
    const char_t*                 type_name;
    const char_t*                 description;
    const char_t*                 categories;
    const char_t*                 restriction;
    const char_t*                 parent_name;
    const char_t*                 parent_type;
    char_t                        label[LABEL_FORMAT_BYTES + (2 * ARINC661_MAX_NAME_BYTES)];
    char_t                        lower_type[ARINC661_MAX_NAME_BYTES];
    char_t                        lower_filter[ARINC661_DEFINITION_MAX_FILTER_BYTES];
    float32_t                     parent_x;
    float32_t                     parent_y;
    uint32_t                      type_index;
    uint32_t                      new_index;
    uint32_t                      character;
    bool_t                        is_match;

    owner = document->owner;
    if (ImGui::BeginPopup(ADD_POPUP_ID))
    {
        (void)memset(&request, 0xFF, sizeof(request));
        parent_name = document->layers[owner->addLayer].name;
        if (INVALID_HIERARCHY_INDEX != owner->addParent)
        {
            parent_name = document->widgets[owner->addParent].name;
        }
        ImFormatString(label, sizeof(label), ADD_HERE_FORMAT, (int)owner->addX, (int)owner->addY, parent_name);
        ImGui::TextDisabled("%s", label);
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1.0f);
        (void)ImGui::InputTextWithHint("##filter", ADD_FILTER_HINT, owner->addFilter, sizeof(owner->addFilter));
        for (character = 0; character < sizeof(lower_filter); ++character)
        {
            lower_filter[character] = (char_t)ImToUpper(owner->addFilter[character]);
            if ('\0' == owner->addFilter[character])
            {
                break;
            }
        }
        if (ImGui::BeginChild("##types", ImVec2(HELP_WRAP_WIDTH, ADD_LIST_HEIGHT), ImGuiChildFlags_Borders))
        {
            for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(FALSE); ++type_index)
            {
                type_name = Arinc661_Definition_GetTypeName(FALSE, type_index);
                for (character = 0; character < sizeof(lower_type); ++character)
                {
                    lower_type[character] = (char_t)ImToUpper(type_name[character]);
                    if ('\0' == type_name[character])
                    {
                        break;
                    }
                }
                is_match = !!(('\0' == lower_filter[0]) || (NULL != strstr(lower_type, lower_filter)));
                parent_type = NULL;
                if (INVALID_HIERARCHY_INDEX != owner->addParent)
                {
                    parent_type = document->widgets[owner->addParent].type;
                }
                if (is_match && (FALSE == IsParentAllowed(type_name, parent_type)))
                {
                    is_match = FALSE;
                }
                if (is_match)
                {
                    if (ImGui::Selectable(type_name))
                    {
                        AddWidget(document, owner->addLayer, owner->addParent, FALSE);
                        if ((SELECTION_WIDGET == document->selectedKind) && (0 == strncmp(DEFAULT_WIDGET_TYPE, document->widgets[document->selectedIndex].type, sizeof(DEFAULT_WIDGET_TYPE))))
                        {
                            new_index = document->selectedIndex;
                            ImStrncpy(document->widgets[new_index].type, type_name, sizeof(document->widgets[new_index].type));
                            (void)ConformParameters(document, new_index);
                            // Place the reference point at the clicked spot, relative to the container's origin
                            parent_x = 0.0f;
                            parent_y = 0.0f;
                            if (INVALID_HIERARCHY_INDEX != owner->addParent)
                            {
                                AbsoluteOrigin(document, owner->addParent, &parent_x, &parent_y);
                            }
                            if (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, new_index, PROPERTY_POS_X))
                            {
                                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, new_index, PROPERTY_POS_X, owner->addX - parent_x);
                                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, new_index, PROPERTY_POS_Y, owner->addY - parent_y);
                            }
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && Arinc661_Help_GetType(type_name, &description, &categories, &restriction))
                    {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(HELP_WRAP_WIDTH);
                        ImGui::TextUnformatted(description);
                        ImGui::TextDisabled(CATEGORIES_FORMAT, categories);
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                }
            }
        }
        ImGui::EndChild();
        if ((INVALID_HIERARCHY_INDEX != owner->addHit) && (owner->addHit < document->definition.widgetCount))
        {
            ImGui::Separator();
            ImFormatString(label, sizeof(label), WIDGET_LABEL_FORMAT, document->widgets[owner->addHit].name, document->widgets[owner->addHit].type);
            ImGui::TextDisabled("%s", label);
            DrawWidgetContextMenu(document, owner->addHit, &request);
            if (INVALID_HIERARCHY_INDEX != request.removeWidgetIndex)
            {
                document->selectedKind  = SELECTION_WIDGET;
                document->selectedIndex = request.removeWidgetIndex;
                RemoveSelection(document);
                ImGui::CloseCurrentPopup();
            }
            else if (INVALID_HIERARCHY_INDEX != request.duplicateWidgetIndex)
            {
                (void)DuplicateWidget(document, request.duplicateWidgetIndex);
                ImGui::CloseCurrentPopup();
            }
            else if (INVALID_HIERARCHY_INDEX != request.addExtensionParentIndex)
            {
                AddWidget(document, document->widgets[request.addExtensionParentIndex].layerIndex, request.addExtensionParentIndex, TRUE);
                ImGui::CloseCurrentPopup();
            }
            else if (INVALID_HIERARCHY_INDEX != request.addWidgetLayerIndex)
            {
                AddWidget(document, request.addWidgetLayerIndex, request.addWidgetParentIndex, FALSE);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rounds to whole millimetres when snapping is on.
 */
static float32_t Snap(const Arinc661_Definition_Editor_t* editor, float32_t value)
{
    if (editor->isSnapping)
    {
        value = (ImFloor((value / SNAP_UNITS) + 0.5f) * SNAP_UNITS);
    }

    return value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks each edge in dash-length steps.
 */
static void DrawDashedScreenRect(ImDrawList* drawList, ImVec2 minimum, ImVec2 maximum, ImU32 color)
{
    float32_t position;

    for (position = minimum.x; position < maximum.x; position += (2.0f * SCENE_FRAME_DASH))
    {
        drawList->AddLine(ImVec2(position, minimum.y), ImVec2(ImMin(position + SCENE_FRAME_DASH, maximum.x), minimum.y), color, 1.5f);
        drawList->AddLine(ImVec2(position, maximum.y), ImVec2(ImMin(position + SCENE_FRAME_DASH, maximum.x), maximum.y), color, 1.5f);
    }
    for (position = minimum.y; position < maximum.y; position += (2.0f * SCENE_FRAME_DASH))
    {
        drawList->AddLine(ImVec2(minimum.x, position), ImVec2(minimum.x, ImMin(position + SCENE_FRAME_DASH, maximum.y)), color, 1.5f);
        drawList->AddLine(ImVec2(maximum.x, position), ImVec2(maximum.x, ImMin(position + SCENE_FRAME_DASH, maximum.y)), color, 1.5f);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Millimetre lines, with every tenth line stronger, drawn only when a millimetre spans at
 *  least a few pixels so the grid never turns into a wall.
 */
static void DrawGrid(ImDrawList* drawList, ImVec2 layerMin, ImVec2 layerMax, float32_t scale, ImVec2 clipMin, ImVec2 clipMax)
{
    float32_t pitch;
    float32_t position;
    ImU32     color;
    int32_t   line;

    pitch = (GRID_UNITS * scale);
    if (4.0f <= pitch)
    {
        for (line = 0, position = layerMin.x; position <= layerMax.x; ++line, position += pitch)
        {
            color = SCENE_GRID_COLOR;
            if (0 == (line % GRID_MAJOR_EVERY))
            {
                color = SCENE_GRID_MAJOR_COLOR;
            }
            if ((clipMin.x <= position) && (position <= clipMax.x))
            {
                drawList->AddLine(ImVec2(position, ImMax(layerMin.y, clipMin.y)), ImVec2(position, ImMin(layerMax.y, clipMax.y)), color);
            }
        }
        // Count rows from the bottom edge, where the layer origin is
        for (line = 0, position = layerMax.y; layerMin.y <= position; ++line, position -= pitch)
        {
            color = SCENE_GRID_COLOR;
            if (0 == (line % GRID_MAJOR_EVERY))
            {
                color = SCENE_GRID_MAJOR_COLOR;
            }
            if ((clipMin.y <= position) && (position <= clipMax.y))
            {
                drawList->AddLine(ImVec2(ImMax(layerMin.x, clipMin.x), position), ImVec2(ImMin(layerMax.x, clipMax.x), position), color);
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Re-parents a widget with its subtree and extensions: the subtree is copied after the
 *  new parent's subtree with parents remapped, then the original is removed. Dropping a
 *  widget onto itself or one of its descendants is refused. PosX and PosY are kept, so the
 *  widget keeps its offset relative to its new container.
 *
 * \retval TRUE    if the widget moved.
 * \retval FALSE   otherwise.
 */
static bool_t MoveWidgetSubtree(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, uint32_t newLayer, uint32_t newParent)
{
    uint32_t source_end;
    uint32_t source_count;
    uint32_t insert_at;
    uint32_t source;
    uint32_t destination;
    uint32_t parent;
    uint32_t property_index;
    uint32_t property_count;
    uint32_t root_copy;
    bool_t   is_allowed;

    is_allowed = !!((widgetIndex != newParent) && ((INVALID_HIERARCHY_INDEX == newParent) || (FALSE == IsDescendant(document, newParent, widgetIndex))));
    if (is_allowed && (INVALID_HIERARCHY_INDEX != newParent))
    {
        is_allowed = !!((FALSE == document->widgets[newParent].isExtension) && (newParent < document->definition.widgetCount));
    }
    if (is_allowed)
    {
        is_allowed = !!((widgetIndex != newParent) && ((INVALID_HIERARCHY_INDEX != newParent) || (newLayer != document->widgets[widgetIndex].layerIndex) || (INVALID_HIERARCHY_INDEX != document->widgets[widgetIndex].parentWidgetIndex)));
    }
    root_copy = INVALID_HIERARCHY_INDEX;
    if (is_allowed)
    {
        source_end   = SubtreeEnd(document, widgetIndex);
        source_count = (source_end - widgetIndex);
        is_allowed   = ((document->definition.widgetCount + source_count) <= ARINC661_DEFINITION_MAX_WIDGETS);
    }
    if (is_allowed)
    {
        insert_at = document->definition.widgetCount;
        if (INVALID_HIERARCHY_INDEX != newParent)
        {
            insert_at = SubtreeEnd(document, newParent);
            if (document->widgets[widgetIndex].isExtension)
            {
                insert_at = (newParent + 1);
            }
        }
        // Open the slots after the target subtree; a target before the sources shifts them
        for (source = widgetIndex; source < source_end; ++source)
        {
            (void)InsertWidgetAt(document, insert_at + (source - widgetIndex));
        }
        if (insert_at <= widgetIndex)
        {
            widgetIndex += source_count;
            source_end += source_count;
        }
        for (source = widgetIndex; source < source_end; ++source)
        {
            destination                    = insert_at + (source - widgetIndex);
            document->widgets[destination] = document->widgets[source];
            document->widgets[destination].layerIndex = newLayer;
            parent                         = document->widgets[source].parentWidgetIndex;
            if ((widgetIndex <= parent) && (parent < source_end))
            {
                document->widgets[destination].parentWidgetIndex = insert_at + (parent - widgetIndex);
            }
            else
            {
                document->widgets[destination].parentWidgetIndex = newParent;
            }
            if (source == widgetIndex)
            {
                root_copy = destination;
            }
            property_count = document->definition.propertyCount;
            for (property_index = 0; property_index < property_count; ++property_index)
            {
                if ((ARINC661_PROPERTY_WIDGET == document->properties[property_index].owner) && (source == document->properties[property_index].ownerIndex))
                {
                    (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, destination, document->properties[property_index].name, document->properties[property_index].value);
                }
            }
        }
        RemoveWidgetSubtree(document, widgetIndex);
        if (widgetIndex < root_copy)
        {
            root_copy -= source_count;
        }
        document->selectedKind  = SELECTION_WIDGET;
        document->selectedIndex = root_copy;
        document->isDirty       = TRUE;
        document->status        = STATUS_WIDGET_ADDED;
    }

    return !!(INVALID_HIERARCHY_INDEX != root_copy);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Recurses into children so a matching descendant keeps its ancestors visible.
 */
static bool_t MatchesFilter(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, const char_t* filter)
{
    uint32_t child_index;
    bool_t   is_match;

    is_match = !!((NULL != ImStristr(document->widgets[widgetIndex].name, NULL, filter, NULL)) || (NULL != ImStristr(document->widgets[widgetIndex].type, NULL, filter, NULL)));
    for (child_index = widgetIndex + 1; (FALSE == is_match) && (child_index < document->definition.widgetCount); ++child_index)
    {
        if (document->widgets[child_index].parentWidgetIndex == widgetIndex)
        {
            is_match = MatchesFilter(document, child_index, filter);
        }
    }

    return is_match;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An invisible button with the glyph drawn over it, dimmed when inactive.
 */
static bool_t DrawFlagButton(Arinc661_Icon_t icon, const char_t* tooltip, bool_t isActive)
{
    ImU32  color;
    bool_t is_pressed;

    ImGui::PushID(tooltip);
    is_pressed = ImGui::InvisibleButton("##flag", ImVec2(EYE_BUTTON_SIZE, EYE_BUTTON_SIZE));
    color      = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    if (isActive)
    {
        color = ImGui::GetColorU32(ImGuiCol_Text);
    }
    Arinc661_Icons_Draw(ImGui::GetWindowDrawList(), icon, ImGui::GetItemRectMin() + ImVec2(2.0f, 2.0f), ImGui::GetItemRectMax() - ImVec2(2.0f, 2.0f), color);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();

    return is_pressed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each clipboard root goes after the parent's subtree in clipboard order; parents inside a
 *  copied subtree are remapped, every copied widget gets a fresh WidgetIdent, and the
 *  extensions keep their place. The first pasted root becomes the selection.
 */
static uint32_t InsertClipboard(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex)
{
    Arinc661_Definition_Editor_t* owner;
    char_t                        text[INTEGER_TEXT_BYTES];
    uint32_t                      mapping[ARINC661_DEFINITION_CLIPBOARD_WIDGETS];
    uint32_t                      clip_index;
    uint32_t                      property_index;
    uint32_t                      destination;
    uint32_t                      parent;
    uint32_t                      first;
    uint32_t                      inserted;

    owner    = document->owner;
    inserted = 0;
    first    = INVALID_HIERARCHY_INDEX;
    if ((document->definition.widgetCount + owner->clipboardWidgetCount) > ARINC661_DEFINITION_MAX_WIDGETS)
    {
        document->status = STATUS_WIDGET_ADD_FAILED;
        return 0;
    }
    for (clip_index = 0; clip_index < owner->clipboardWidgetCount; ++clip_index)
    {
        parent = owner->clipboardWidgets[clip_index].parentWidgetIndex;
        if (INVALID_HIERARCHY_INDEX == parent)
        {
            parent = parentIndex;
        }
        else
        {
            parent = mapping[parent];
        }
        destination = document->definition.widgetCount;
        if (INVALID_HIERARCHY_INDEX != parent)
        {
            destination = SubtreeEnd(document, parent);
        }
        if (FALSE == InsertWidgetAt(document, destination))
        {
            break;
        }
        document->widgets[destination]                   = owner->clipboardWidgets[clip_index];
        document->widgets[destination].layerIndex        = layerIndex;
        document->widgets[destination].parentWidgetIndex = parent;
        // Earlier mappings that sit at or after the insertion point shift by one
        for (property_index = 0; property_index < clip_index; ++property_index)
        {
            if ((INVALID_HIERARCHY_INDEX != mapping[property_index]) && (destination <= mapping[property_index]))
            {
                mapping[property_index] += 1;
            }
        }
        mapping[clip_index] = destination;
        if (INVALID_HIERARCHY_INDEX == first)
        {
            first = destination;
        }
        else if (destination <= first)
        {
            first += 1;
        }
        for (property_index = 0; property_index < owner->clipboardPropertyCount; ++property_index)
        {
            if (owner->clipboardProperties[property_index].ownerIndex == clip_index)
            {
                if (0 == strncmp(PROPERTY_WIDGET_IDENT, owner->clipboardProperties[property_index].name, sizeof(PROPERTY_WIDGET_IDENT)))
                {
                    ImFormatString(text, sizeof(text), "%" PRId64, NextWidgetIdent(document, layerIndex));
                    (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, destination, PROPERTY_WIDGET_IDENT, text);
                }
                else
                {
                    (void)AppendProperty(document, ARINC661_PROPERTY_WIDGET, destination, owner->clipboardProperties[property_index].name, owner->clipboardProperties[property_index].value);
                }
            }
        }
        inserted += 1;
    }
    if (INVALID_HIERARCHY_INDEX != first)
    {
        SelectOnly(document, first);
        document->isDirty = TRUE;
        document->status  = STATUS_WIDGET_ADDED;
    }

    return inserted;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts the flags.
 */
static uint32_t SelectedCount(const Arinc661_Definition_Document_t* document)
{
    uint32_t index;
    uint32_t count;

    count = 0;
    for (index = 0; index < document->definition.widgetCount; ++index)
    {
        if (document->isSelected[index])
        {
            count += 1;
        }
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Clears every flag, sets one, and makes it the primary selection.
 */
static void SelectOnly(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    ClearSelectionFlags(document);
    if (widgetIndex < document->definition.widgetCount)
    {
        document->isSelected[widgetIndex] = 1;
        document->selectedKind            = SELECTION_WIDGET;
        document->selectedIndex           = widgetIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A toggled-in widget becomes the primary; toggling the primary out hands the primary to
 *  another selected widget, or to its layer when none remains.
 */
static void ToggleSelected(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    uint32_t index;

    if (widgetIndex < document->definition.widgetCount)
    {
        if (document->isSelected[widgetIndex])
        {
            document->isSelected[widgetIndex] = 0;
            if ((SELECTION_WIDGET == document->selectedKind) && (widgetIndex == document->selectedIndex))
            {
                document->selectedKind  = SELECTION_LAYER;
                document->selectedIndex = document->widgets[widgetIndex].layerIndex;
                for (index = 0; index < document->definition.widgetCount; ++index)
                {
                    if (document->isSelected[index])
                    {
                        document->selectedKind  = SELECTION_WIDGET;
                        document->selectedIndex = index;
                        break;
                    }
                }
            }
        }
        else
        {
            document->isSelected[widgetIndex] = 1;
            document->selectedKind            = SELECTION_WIDGET;
            document->selectedIndex           = widgetIndex;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Flags only; the primary selection is untouched.
 */
static void ClearSelectionFlags(Arinc661_Definition_Document_t* document)
{
    (void)memset(document->isSelected, 0, sizeof(document->isSelected));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Works on absolute reference points and sizes so widgets in different containers line up
 *  on screen; each widget's PosX and PosY are shifted by the difference. Distribution
 *  spaces the widgets' left edges or bottom edges evenly between the two outermost.
 */
static void AlignSelection(Arinc661_Definition_Document_t* document, int32_t mode)
{
    float32_t x[ARINC661_DEFINITION_MAX_WIDGETS];
    float32_t y[ARINC661_DEFINITION_MAX_WIDGETS];
    float32_t width[ARINC661_DEFINITION_MAX_WIDGETS];
    float32_t height[ARINC661_DEFINITION_MAX_WIDGETS];
    uint32_t  order[ARINC661_DEFINITION_MAX_WIDGETS];
    float32_t minimum;
    float32_t maximum;
    float32_t target;
    float32_t step;
    float32_t position;
    float32_t own_x;
    float32_t own_y;
    uint32_t  index;
    uint32_t  count;
    uint32_t  slot;
    uint32_t  swap;
    uint32_t  rank;
    bool_t    has_first;

    count     = 0;
    has_first = FALSE;
    minimum   = 0.0f;
    maximum   = 0.0f;
    for (index = 0; index < document->definition.widgetCount; ++index)
    {
        if (document->isSelected[index] && (FALSE == document->widgets[index].isExtension) && (FALSE == document->isLocked[index]))
        {
            AbsoluteOrigin(document, index, &x[index], &y[index]);
            width[index]  = 0.0f;
            height[index] = 0.0f;
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_SIZE_X, &width[index]);
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_SIZE_Y, &height[index]);
            order[count] = index;
            count += 1;
        }
    }
    if (count < 2)
    {
        return;
    }
    for (slot = 0; slot < count; ++slot)
    {
        index = order[slot];
        switch (mode)
        {
            case 0: position = x[index]; break;
            case 1: position = (x[index] + width[index]); break;
            case 2: position = (y[index] + height[index]); break;
            case 3: position = y[index]; break;
            case 4: position = (x[index] + (width[index] * 0.5f)); break;
            case 5: position = (y[index] + (height[index] * 0.5f)); break;
            case 6: position = x[index]; break;
            default: position = y[index]; break;
        }
        if ((FALSE == has_first) || (position < minimum))
        {
            minimum = position;
        }
        if ((FALSE == has_first) || (maximum < position))
        {
            maximum = position;
        }
        has_first = TRUE;
    }
    // Left, top, and centre modes meet at the extreme the mode names; distribution sorts by position
    target = minimum;
    if ((1 == mode) || (2 == mode))
    {
        target = maximum;
    }
    else if ((4 == mode) || (5 == mode))
    {
        target = ((minimum + maximum) * 0.5f);
    }
    if ((6 == mode) || (7 == mode))
    {
        for (slot = 0; slot < count; ++slot)
        {
            for (rank = slot + 1; rank < count; ++rank)
            {
                if (((6 == mode) && (x[order[rank]] < x[order[slot]])) || ((7 == mode) && (y[order[rank]] < y[order[slot]])))
                {
                    swap        = order[slot];
                    order[slot] = order[rank];
                    order[rank] = swap;
                }
            }
        }
        step = ((maximum - minimum) / (float32_t)(count - 1));
    }
    for (slot = 0; slot < count; ++slot)
    {
        index = order[slot];
        own_x = 0.0f;
        own_y = 0.0f;
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_X, &own_x);
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_Y, &own_y);
        switch (mode)
        {
            case 0: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_X, own_x + (target - x[index])); break;
            case 1: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_X, own_x + (target - (x[index] + width[index]))); break;
            case 2: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_Y, own_y + (target - (y[index] + height[index]))); break;
            case 3: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_Y, own_y + (target - y[index])); break;
            case 4: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_X, own_x + (target - (x[index] + (width[index] * 0.5f)))); break;
            case 5: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_Y, own_y + (target - (y[index] + (height[index] * 0.5f)))); break;
            case 6: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_X, own_x + ((minimum + (step * (float32_t)slot)) - x[index])); break;
            default: SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, index, PROPERTY_POS_Y, own_y + ((minimum + (step * (float32_t)slot)) - y[index])); break;
        }
    }
    document->isDirty = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Ticks every millimetre with a longer tick and a label every centimetre, laid along the
 *  top and left edges of the canvas, counting from the layer origin at its lower-left corner.
 */
static void DrawRulers(ImDrawList* drawList, ImVec2 canvasMin, ImVec2 canvasMax, ImVec2 layerMin, ImVec2 layerMax, float32_t scale)
{
    char_t    label[LABEL_FORMAT_BYTES];
    float32_t font_size;
    float32_t pitch;
    float32_t position;
    float32_t length;
    int32_t   tick;
    int32_t   first;
    int32_t   last;

    pitch = (RULER_UNITS * scale);
    if (pitch < 2.0f)
    {
        return;
    }
    drawList->AddRectFilled(canvasMin, ImVec2(canvasMax.x, canvasMin.y + SCENE_RULER_SIZE), SCENE_RULER_BACK);
    drawList->AddRectFilled(canvasMin, ImVec2(canvasMin.x + SCENE_RULER_SIZE, canvasMax.y), SCENE_RULER_BACK);
    font_size = (ImGui::GetFontSize() * SCENE_RULER_FONT_SCALE);
    drawList->PushClipRect(ImVec2(canvasMin.x + SCENE_RULER_SIZE, canvasMin.y), ImVec2(canvasMax.x, canvasMin.y + SCENE_RULER_SIZE), true);
    first = (int32_t)ImFloor((canvasMin.x - layerMin.x) / pitch);
    last  = (int32_t)ImCeil((canvasMax.x - layerMin.x) / pitch);
    for (tick = first; tick <= last; ++tick)
    {
        position = (layerMin.x + ((float32_t)tick * pitch));
        length   = (SCENE_RULER_SIZE * 0.35f);
        if (0 == (tick % 10))
        {
            length = SCENE_RULER_SIZE;
            ImFormatString(label, sizeof(label), "%d", tick / 10);
            drawList->AddText(ImGui::GetFont(), font_size, ImVec2(position + 2.0f, canvasMin.y), SCENE_RULER_COLOR, label);
        }
        drawList->AddLine(ImVec2(position, canvasMin.y + SCENE_RULER_SIZE - length), ImVec2(position, canvasMin.y + SCENE_RULER_SIZE), SCENE_RULER_COLOR);
    }
    drawList->PopClipRect();
    drawList->PushClipRect(ImVec2(canvasMin.x, canvasMin.y + SCENE_RULER_SIZE), ImVec2(canvasMin.x + SCENE_RULER_SIZE, canvasMax.y), true);
    first = (int32_t)ImFloor((layerMax.y - canvasMax.y) / pitch);
    last  = (int32_t)ImCeil((layerMax.y - canvasMin.y) / pitch);
    for (tick = first; tick <= last; ++tick)
    {
        position = (layerMax.y - ((float32_t)tick * pitch));
        length   = (SCENE_RULER_SIZE * 0.35f);
        if (0 == (tick % 10))
        {
            length = SCENE_RULER_SIZE;
            ImFormatString(label, sizeof(label), "%d", tick / 10);
            drawList->AddText(ImGui::GetFont(), font_size, ImVec2(canvasMin.x + 1.0f, position - font_size), SCENE_RULER_COLOR, label);
        }
        drawList->AddLine(ImVec2(canvasMin.x + SCENE_RULER_SIZE - length, position), ImVec2(canvasMin.x + SCENE_RULER_SIZE, position), SCENE_RULER_COLOR);
    }
    drawList->PopClipRect();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The LayerId property names a layer to the user application; an empty string stands for a
 *  layer without one.
 */
static const char_t* LayerIdText(const Arinc661_Definition_Document_t* document, uint32_t layerIndex)
{
    const char_t* text;
    uint32_t      property_index;

    text           = "";
    property_index = FindOwnedProperty(document, ARINC661_PROPERTY_LAYER, layerIndex, PROPERTY_LAYER_ID);
    if (INVALID_HIERARCHY_INDEX != property_index)
    {
        text = document->properties[property_index].value;
    }

    return text;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Icon row above the canvas: zoom, fit, grid, snap, then add layer, add widget, add
 *  extension, duplicate, delete, and a combo choosing the layer shown.
 */
static void DrawSceneToolbar(Arinc661_Definition_Document_t* document, uint32_t* layerIndex)
{
    Arinc661_Definition_Editor_t* owner;
    char_t                        label[HIERARCHY_LABEL_BYTES];
    uint32_t                      index;
    bool_t                        has_widget;
    bool_t                        has_layer;
    bool_t                        has_group;

    owner      = document->owner;
    has_layer  = (0 < document->definition.layerCount);
    has_widget = !!((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount));
    has_group  = FALSE;
    if (Arinc661_Icons_Button(ARINC661_ICON_ZOOM_IN, TOOL_ZOOM_IN_LABEL, SHORTCUT_WHEEL, has_layer, FALSE))
    {
        document->viewZoom = ImClamp(ImMax(document->viewZoom, 1.0f) * ZOOM_STEP_FACTOR, ZOOM_MIN, ZOOM_MAX);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ZOOM_OUT, TOOL_ZOOM_OUT_LABEL, SHORTCUT_WHEEL, has_layer, FALSE))
    {
        document->viewZoom = ImClamp(ImMax(document->viewZoom, 1.0f) / ZOOM_STEP_FACTOR, ZOOM_MIN, ZOOM_MAX);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_FIT, TOOL_FIT_LABEL, SHORTCUT_F, has_layer, FALSE))
    {
        document->viewZoom = 1.0f;
        document->viewPanX = 0.0f;
        document->viewPanY = 0.0f;
    }
    Arinc661_Icons_Gap();
    if (Arinc661_Icons_Button(ARINC661_ICON_GRID, TOOL_GRID_LABEL, SHORTCUT_G, TRUE, owner->showsGrid))
    {
        owner->showsGrid = !owner->showsGrid;
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_SNAP, TOOL_SNAP_LABEL, SHORTCUT_S, TRUE, owner->isSnapping))
    {
        owner->isSnapping = !owner->isSnapping;
    }
    Arinc661_Icons_Gap();
    if (Arinc661_Icons_Button(ARINC661_ICON_ADD_LAYER, TOOL_ADD_LAYER_LABEL, NULL, TRUE, FALSE))
    {
        AddLayer(document);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ADD_WIDGET, TOOL_ADD_WIDGET_LABEL, NULL, has_layer, FALSE))
    {
        Arinc661_Definition_AddWidget(owner);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ADD_EXTENSION, TOOL_ADD_EXTENSION_LABEL, NULL, has_widget, FALSE))
    {
        Arinc661_Definition_AddExtension(owner);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_DUPLICATE, TOOL_DUPLICATE_LABEL, SHORTCUT_CTRL_D, has_widget, FALSE))
    {
        (void)DuplicateWidget(document, document->selectedIndex);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_DELETE, TOOL_DELETE_LABEL, SHORTCUT_DEL, has_widget || (SELECTION_LAYER == document->selectedKind), FALSE))
    {
        RemoveSelection(document);
    }
    Arinc661_Icons_Gap();
    has_group = (1 < SelectedCount(document));
    if (Arinc661_Icons_Button(ARINC661_ICON_ALIGN_LEFT, TOOL_ALIGN_LEFT_LABEL, NULL, has_group, FALSE))
    {
        AlignSelection(document, 0);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ALIGN_CENTER_X, TOOL_ALIGN_CENTER_X_LABEL, NULL, has_group, FALSE))
    {
        AlignSelection(document, 4);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ALIGN_RIGHT, TOOL_ALIGN_RIGHT_LABEL, NULL, has_group, FALSE))
    {
        AlignSelection(document, 1);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ALIGN_TOP, TOOL_ALIGN_TOP_LABEL, NULL, has_group, FALSE))
    {
        AlignSelection(document, 2);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ALIGN_CENTER_Y, TOOL_ALIGN_CENTER_Y_LABEL, NULL, has_group, FALSE))
    {
        AlignSelection(document, 5);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ALIGN_BOTTOM, TOOL_ALIGN_BOTTOM_LABEL, NULL, has_group, FALSE))
    {
        AlignSelection(document, 3);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_DISTRIBUTE_X, TOOL_DISTRIBUTE_X_LABEL, NULL, 2 < SelectedCount(document), FALSE))
    {
        AlignSelection(document, 6);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_DISTRIBUTE_Y, TOOL_DISTRIBUTE_Y_LABEL, NULL, 2 < SelectedCount(document), FALSE))
    {
        AlignSelection(document, 7);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_ZOOM_SELECTION, TOOL_ZOOM_SELECTION_LABEL, SHORTCUT_Z, has_widget, FALSE))
    {
        owner->zoomsToSelection = TRUE;
    }
    Arinc661_Icons_Gap();
    (void)Arinc661_Icons_Button(ARINC661_ICON_HELP, SCENE_HINT_TITLE, NULL, TRUE, FALSE);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(SCENE_HINT_WRAP_WIDTH);
        ImGui::TextUnformatted(SCENE_HINT_LABEL);
        ImGui::TextUnformatted(SCENE_HINT_LABEL_2);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    if (ImGui::GetContentRegionAvail().x < 220.0f)
    {
        ImGui::NewLine();
    }
    else
    {
        Arinc661_Icons_Gap();
    }
    ImGui::SetNextItemWidth(ImMax(120.0f, ImGui::GetContentRegionAvail().x));
    label[0] = '\0';
    if ((*layerIndex) < document->definition.layerCount)
    {
        ImFormatString(label, sizeof(label), TOOL_LAYER_FORMAT, LayerIdText(document, *layerIndex), document->layers[*layerIndex].name);
    }
    else
    {
        ImStrncpy(label, TOOL_NO_LAYER_LABEL, sizeof(label));
    }
    if (ImGui::BeginCombo(TOOL_LAYER_COMBO_ID, label))
    {
        for (index = 0; index < document->definition.layerCount; ++index)
        {
            ImFormatString(label, sizeof(label), TOOL_LAYER_FORMAT, LayerIdText(document, index), document->layers[index].name);
            if (ImGui::Selectable(label, index == (*layerIndex)))
            {
                document->selectedKind  = SELECTION_LAYER;
                document->selectedIndex = index;
                (*layerIndex)           = index;
            }
        }
        ImGui::EndCombo();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Direct manipulation on the scene canvas. A press selects the smallest widget under the
 *  cursor and starts a move; a press on a corner handle of the selected widget starts a
 *  resize; the arrow keys nudge PosX and PosY; Delete removes the selection; a right-click
 *  opens the add pop-up at that point. Layer units grow upward, so screen deltas flip in Y.
 */
static void DrawSceneInteraction(Arinc661_Definition_Document_t* document, uint32_t layerIndex, ImVec2 layerMin, ImVec2 layerMax, float32_t scale, ImDrawList* drawList, bool_t isCanvasHovered)
{
    Arinc661_Definition_Editor_t*  owner;
    const Arinc661_Scene_Bounds_t* bounds;
    FACEOE_Gfx_Point_t             mouse_point;
    ImVec2                         mouse;
    ImVec2                         corners[4];
    ImVec2                         marquee_min;
    ImVec2                         marquee_max;
    ImVec2                         selected_min;
    ImVec2                         selected_max;
    float32_t                      delta_x;
    float32_t                      delta_y;
    float32_t                      step;
    float32_t                      value;
    float32_t                      width;
    float32_t                      height;
    uint32_t                       hit_index;
    uint32_t                       bounds_index;
    uint32_t                       corner;
    int32_t                        hover_mode;
    bool_t                         has_selected_bounds;
    bool_t                         has_position;
    bool_t                         has_size;
    bool_t                         has_angle;
    ImVec2                         rotate_handle;
    ImVec2                         center;

    owner               = document->owner;
    mouse               = ImGui::GetMousePos();
    mouse_point.x       = mouse.x;
    mouse_point.y       = mouse.y;
    has_selected_bounds = FALSE;
    has_position        = FALSE;
    has_size            = FALSE;
    has_angle           = FALSE;
    hover_mode          = DRAG_NONE;
    if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
    {
        has_angle    = !!(INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_ROTATION_ANGLE));
        has_position = !!((INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_POS_X)) && (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_POS_Y)));
        has_size     = !!((INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_SIZE_X)) && (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_SIZE_Y)));
        for (bounds_index = 0; bounds_index < owner->sceneBoundsCount; ++bounds_index)
        {
            bounds = &owner->sceneBounds[bounds_index];
            if (document->selectedIndex == bounds->widgetIndex)
            {
                selected_min        = ImVec2(bounds->minimum.x, bounds->minimum.y);
                selected_max        = ImVec2(bounds->maximum.x, bounds->maximum.y);
                has_selected_bounds = TRUE;
            }
        }
    }
    for (bounds_index = 0; bounds_index < owner->sceneBoundsCount; ++bounds_index)
    {
        bounds = &owner->sceneBounds[bounds_index];
        if ((bounds->widgetIndex < document->definition.widgetCount) && document->isSelected[bounds->widgetIndex] && ((SELECTION_WIDGET != document->selectedKind) || (bounds->widgetIndex != document->selectedIndex)))
        {
            drawList->AddRect(ImVec2(bounds->minimum.x, bounds->minimum.y), ImVec2(bounds->maximum.x, bounds->maximum.y), SCENE_MARQUEE_BORDER);
        }
    }
    if (has_selected_bounds)
    {
        drawList->AddRectFilled(selected_min, selected_max, SCENE_SELECTION_OVERLAY_COLOR);
        drawList->AddRect(selected_min, selected_max, SCENE_TEXT_COLOR);
        if (has_size && has_position)
        {
            // Corner handles: bottom-left, bottom-right, top-left, top-right in layer terms
            corners[0] = ImVec2(selected_min.x, selected_max.y);
            corners[1] = ImVec2(selected_max.x, selected_max.y);
            corners[2] = ImVec2(selected_min.x, selected_min.y);
            corners[3] = ImVec2(selected_max.x, selected_min.y);
            for (corner = 0; corner < 4; ++corner)
            {
                drawList->AddRectFilled(corners[corner] - ImVec2(HANDLE_HALF_SIZE, HANDLE_HALF_SIZE), corners[corner] + ImVec2(HANDLE_HALF_SIZE, HANDLE_HALF_SIZE), SCENE_HANDLE_COLOR);
                if ((ImFabs(mouse.x - corners[corner].x) <= HANDLE_HALF_SIZE) && (ImFabs(mouse.y - corners[corner].y) <= HANDLE_HALF_SIZE))
                {
                    hover_mode = (DRAG_BOTTOM_LEFT + (int32_t)corner);
                }
            }
        }
        if (has_angle)
        {
            // Rotation gizmo: a ring on a stem above the widget, as a 2D engine editor shows it
            center        = ImVec2((selected_min.x + selected_max.x) * 0.5f, (selected_min.y + selected_max.y) * 0.5f);
            rotate_handle = ImVec2(center.x, selected_min.y - ROTATE_HANDLE_OFFSET);
            drawList->AddLine(ImVec2(center.x, selected_min.y), rotate_handle, SCENE_HANDLE_COLOR);
            drawList->AddCircle(rotate_handle, ROTATE_HANDLE_RADIUS, SCENE_HANDLE_COLOR, 0, 2.0f);
            if (ImLengthSqr(mouse - rotate_handle) <= (ROTATE_HANDLE_RADIUS * ROTATE_HANDLE_RADIUS * 2.0f))
            {
                hover_mode = DRAG_ROTATE;
            }
            if (DRAG_ROTATE == owner->dragMode)
            {
                drawList->AddLine(ImVec2(owner->dragCenterX, owner->dragCenterY), mouse, SCENE_HANDLE_COLOR);
            }
        }
    }
    if (isCanvasHovered && (DRAG_NONE != hover_mode))
    {
        if (DRAG_ROTATE == hover_mode)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("%s", ROTATE_TOOLTIP_LABEL);
        }
        else if ((DRAG_BOTTOM_LEFT == hover_mode) || (DRAG_TOP_RIGHT == hover_mode))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        }
    }
    // Outline the widget under the cursor so the pick target is visible before clicking
    if (isCanvasHovered && (DRAG_NONE == owner->dragMode) && (DRAG_NONE == hover_mode))
    {
        hit_index = Arinc661_Scene_Pick(owner->sceneBounds, owner->sceneBoundsCount, mouse_point);
        if ((ARINC661_SCENE_NO_WIDGET != hit_index) && ((SELECTION_WIDGET != document->selectedKind) || (hit_index != document->selectedIndex)))
        {
            for (bounds_index = 0; bounds_index < owner->sceneBoundsCount; ++bounds_index)
            {
                if (hit_index == owner->sceneBounds[bounds_index].widgetIndex)
                {
                    drawList->AddRect(ImVec2(owner->sceneBounds[bounds_index].minimum.x, owner->sceneBounds[bounds_index].minimum.y), ImVec2(owner->sceneBounds[bounds_index].maximum.x, owner->sceneBounds[bounds_index].maximum.y), SCENE_HOVER_COLOR);
                }
            }
        }
    }
    if (isCanvasHovered && (0 == document->definition.widgetCount))
    {
        drawList->AddText(ImVec2(layerMin.x + 8.0f, layerMin.y + 8.0f), SCENE_HINT_COLOR, EMPTY_WIDGET_HINT_LABEL);
    }
    // Press: start a resize on a handle, otherwise select what is under the cursor and start a
    // move; Ctrl toggles membership; empty space starts a rubber-band selection
    if (ImGui::IsItemActivated() && (FALSE == ImGui::IsKeyDown(ImGuiKey_Space)))
    {
        owner->dragMode   = DRAG_NONE;
        owner->hasDragged = FALSE;
        owner->isMarquee  = FALSE;
        owner->dragStartX = mouse.x;
        owner->dragStartY = mouse.y;
        owner->dragScale  = scale;
        if (DRAG_NONE != hover_mode)
        {
            owner->dragMode   = hover_mode;
            owner->dragWidget = document->selectedIndex;
        }
        else
        {
            hit_index = Arinc661_Scene_Pick(owner->sceneBounds, owner->sceneBoundsCount, mouse_point);
            while ((ARINC661_SCENE_NO_WIDGET != hit_index) && document->isLocked[hit_index])
            {
                // Locked widgets are not pickable; fall through to what lies under them
                hit_index = ARINC661_SCENE_NO_WIDGET;
            }
            if (ARINC661_SCENE_NO_WIDGET != hit_index)
            {
                if (ImGui::GetIO().KeyCtrl)
                {
                    ToggleSelected(document, hit_index);
                }
                else if (FALSE == document->isSelected[hit_index])
                {
                    SelectOnly(document, hit_index);
                }
                else
                {
                    document->selectedKind  = SELECTION_WIDGET;
                    document->selectedIndex = hit_index;
                }
                if (document->isSelected[hit_index] && (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, hit_index, PROPERTY_POS_X)) && (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, hit_index, PROPERTY_POS_Y)))
                {
                    owner->dragMode   = DRAG_MOVE;
                    owner->dragWidget = hit_index;
                }
            }
            else
            {
                if (FALSE == ImGui::GetIO().KeyCtrl)
                {
                    ClearSelectionFlags(document);
                    document->selectedKind  = SELECTION_LAYER;
                    document->selectedIndex = layerIndex;
                }
                owner->isMarquee     = TRUE;
                owner->marqueeStartX = mouse.x;
                owner->marqueeStartY = mouse.y;
            }
        }
        // Remember where every selected widget started so the group moves together
        for (bounds_index = 0; bounds_index < document->definition.widgetCount; ++bounds_index)
        {
            owner->dragOrigins[bounds_index][0] = 0.0f;
            owner->dragOrigins[bounds_index][1] = 0.0f;
            if (document->isSelected[bounds_index])
            {
                (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, bounds_index, PROPERTY_POS_X, &owner->dragOrigins[bounds_index][0]);
                (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, bounds_index, PROPERTY_POS_Y, &owner->dragOrigins[bounds_index][1]);
            }
        }
        if (DRAG_NONE != owner->dragMode)
        {
            owner->dragOriginX      = 0.0f;
            owner->dragOriginY      = 0.0f;
            owner->dragOriginWidth  = 0.0f;
            owner->dragOriginHeight = 0.0f;
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_POS_X, &owner->dragOriginX);
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_POS_Y, &owner->dragOriginY);
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_SIZE_X, &owner->dragOriginWidth);
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_SIZE_Y, &owner->dragOriginHeight);
            owner->dragOriginAngle = 0.0f;
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_ROTATION_ANGLE, &owner->dragOriginAngle);
            owner->dragCenterX = ((selected_min.x + selected_max.x) * 0.5f);
            owner->dragCenterY = ((selected_min.y + selected_max.y) * 0.5f);
        }
    }
    // Drag: apply the mouse delta in layer units to the origin values
    if (ImGui::IsItemActive() && (DRAG_NONE != owner->dragMode) && (owner->dragWidget < document->definition.widgetCount) && (0.0f < owner->dragScale))
    {
        delta_x = ((mouse.x - owner->dragStartX) / owner->dragScale);
        delta_y = (-(mouse.y - owner->dragStartY) / owner->dragScale);
        if ((DRAG_THRESHOLD <= ImFabs(mouse.x - owner->dragStartX)) || (DRAG_THRESHOLD <= ImFabs(mouse.y - owner->dragStartY)))
        {
            owner->hasDragged = TRUE;
        }
        if (owner->hasDragged)
        {
            if (DRAG_ROTATE == owner->dragMode)
            {
                // Angle swept about the pivot since the press, counter-clockwise positive as the spec reads it; Shift snaps to 15 degrees
                value = (owner->dragOriginAngle + ((ImAtan2(-(mouse.y - owner->dragCenterY), mouse.x - owner->dragCenterX) - ImAtan2(-(owner->dragStartY - owner->dragCenterY), owner->dragStartX - owner->dragCenterX)) * (180.0f / IM_PI)));
                if (ImGui::GetIO().KeyShift)
                {
                    value = (ImFloor((value / 15.0f) + 0.5f) * 15.0f);
                }
                while (value < 0.0f)
                {
                    value += 360.0f;
                }
                while (360.0f <= value)
                {
                    value -= 360.0f;
                }
                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_ROTATION_ANGLE, value);
            }
            else if (DRAG_MOVE == owner->dragMode)
            {
                // Every selected, unlocked widget with a reference point follows the drag
                for (bounds_index = 0; bounds_index < document->definition.widgetCount; ++bounds_index)
                {
                    if (document->isSelected[bounds_index] && (FALSE == document->isLocked[bounds_index]) && (INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, bounds_index, PROPERTY_POS_X)))
                    {
                        SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, bounds_index, PROPERTY_POS_X, Snap(owner, owner->dragOrigins[bounds_index][0] + delta_x));
                        SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, bounds_index, PROPERTY_POS_Y, Snap(owner, owner->dragOrigins[bounds_index][1] + delta_y));
                    }
                }
            }
            else
            {
                width  = owner->dragOriginWidth;
                height = owner->dragOriginHeight;
                if ((DRAG_BOTTOM_RIGHT == owner->dragMode) || (DRAG_TOP_RIGHT == owner->dragMode))
                {
                    width = ImMax(1.0f, Snap(owner, owner->dragOriginWidth + delta_x));
                }
                else
                {
                    width = ImMax(1.0f, Snap(owner, owner->dragOriginWidth - delta_x));
                    SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_POS_X, owner->dragOriginX + (owner->dragOriginWidth - width));
                }
                if ((DRAG_TOP_LEFT == owner->dragMode) || (DRAG_TOP_RIGHT == owner->dragMode))
                {
                    height = ImMax(1.0f, Snap(owner, owner->dragOriginHeight + delta_y));
                }
                else
                {
                    height = ImMax(1.0f, Snap(owner, owner->dragOriginHeight - delta_y));
                    SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_POS_Y, owner->dragOriginY + (owner->dragOriginHeight - height));
                }
                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_SIZE_X, width);
                SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, owner->dragWidget, PROPERTY_SIZE_Y, height);
            }
            MarkDocumentDirty(document, TRUE);
        }
    }
    if (owner->isMarquee && ImGui::IsItemActive())
    {
        marquee_min = ImVec2(ImMin(owner->marqueeStartX, mouse.x), ImMin(owner->marqueeStartY, mouse.y));
        marquee_max = ImVec2(ImMax(owner->marqueeStartX, mouse.x), ImMax(owner->marqueeStartY, mouse.y));
        drawList->AddRectFilled(marquee_min, marquee_max, SCENE_MARQUEE_COLOR);
        drawList->AddRect(marquee_min, marquee_max, SCENE_MARQUEE_BORDER);
    }
    if (ImGui::IsItemDeactivated())
    {
        if (owner->isMarquee)
        {
            // Widgets wholly inside the band join the selection; the first becomes primary
            marquee_min = ImVec2(ImMin(owner->marqueeStartX, mouse.x), ImMin(owner->marqueeStartY, mouse.y));
            marquee_max = ImVec2(ImMax(owner->marqueeStartX, mouse.x), ImMax(owner->marqueeStartY, mouse.y));
            if ((DRAG_THRESHOLD <= (marquee_max.x - marquee_min.x)) || (DRAG_THRESHOLD <= (marquee_max.y - marquee_min.y)))
            {
                for (bounds_index = 0; bounds_index < owner->sceneBoundsCount; ++bounds_index)
                {
                    hit_index = owner->sceneBounds[bounds_index].widgetIndex;
                    if ((hit_index < document->definition.widgetCount) && (FALSE == document->isLocked[hit_index]) && (marquee_min.x <= owner->sceneBounds[bounds_index].minimum.x) && (owner->sceneBounds[bounds_index].maximum.x <= marquee_max.x) && (marquee_min.y <= owner->sceneBounds[bounds_index].minimum.y) && (owner->sceneBounds[bounds_index].maximum.y <= marquee_max.y))
                    {
                        if ((SELECTION_WIDGET != document->selectedKind) || (FALSE == document->isSelected[document->selectedIndex]))
                        {
                            document->selectedKind  = SELECTION_WIDGET;
                            document->selectedIndex = hit_index;
                        }
                        document->isSelected[hit_index] = 1;
                    }
                }
            }
            owner->isMarquee = FALSE;
        }
        owner->dragMode = DRAG_NONE;
    }
    // Keyboard: nudge and delete while the scene has the focus
    if (ImGui::IsWindowFocused() && (SELECTION_WIDGET == document->selectedKind) && has_position && (FALSE == ImGui::IsAnyItemActive()))
    {
        step = NUDGE_STEP;
        if (ImGui::GetIO().KeyShift)
        {
            step = NUDGE_FAST_STEP;
        }
        delta_x = 0.0f;
        delta_y = 0.0f;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            delta_x = -step;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            delta_x = step;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            delta_y = step;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            delta_y = -step;
        }
        if ((0.0f != delta_x) || (0.0f != delta_y))
        {
            value = 0.0f;
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_POS_X, &value);
            SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_POS_X, value + delta_x);
            value = 0.0f;
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_POS_Y, &value);
            SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_POS_Y, value + delta_y);
            MarkDocumentDirty(document, TRUE);
        }
    }
    if (ImGui::IsWindowFocused() && (FALSE == ImGui::IsAnyItemActive()) && ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        RemoveSelection(document);
    }
    // A palette type dropped on the canvas lands where it was dropped, in the container under it
    if (ImGui::BeginDragDropTarget())
    {
        if (NULL != ImGui::AcceptDragDropPayload(PALETTE_PAYLOAD_ID))
        {
            hit_index = Arinc661_Scene_Pick(owner->sceneBounds, owner->sceneBoundsCount, mouse_point);
            if (ARINC661_SCENE_NO_WIDGET == hit_index)
            {
                hit_index = INVALID_HIERARCHY_INDEX;
            }
            AddTypedWidget(document, layerIndex, hit_index, (const char_t*)ImGui::GetDragDropPayload()->Data, Snap(owner, (mouse.x - layerMin.x) / scale), Snap(owner, (layerMax.y - mouse.y) / scale), TRUE);
        }
        ImGui::EndDragDropTarget();
    }
    // Right-click: remember where and on what, then open the add pop-up
    if (isCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        owner->addLayer  = layerIndex;
        owner->addX      = Snap(owner, (mouse.x - layerMin.x) / scale);
        owner->addY      = Snap(owner, (layerMax.y - mouse.y) / scale);
        owner->addHit    = Arinc661_Scene_Pick(owner->sceneBounds, owner->sceneBoundsCount, mouse_point);
        owner->addParent = INVALID_HIERARCHY_INDEX;
        if (ARINC661_SCENE_NO_WIDGET != owner->addHit)
        {
            owner->addParent = owner->addHit;
            if (FALSE == IsContainerType(document->widgets[owner->addHit].type))
            {
                owner->addParent = document->widgets[owner->addHit].parentWidgetIndex;
            }
        }
        owner->addFilter[0] = '\0';
        ImGui::OpenPopup(ADD_POPUP_ID);
    }
    DrawAddPopup(document);
    (void)layerMax;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fits the X to the current font and centers it in a tree-row-height hit area.
 */
static bool_t DrawRemoveButton(const char_t* label, float32_t rowHeight)
{
    ImDrawList* draw_list;
    ImVec2      button_size;
    ImVec2      cross_half_size;
    ImVec2      cross_center;
    ImVec2      cross_min;
    ImVec2      cross_max;
    ImVec2      cross_left_bottom;
    ImVec2      cross_right_top;
    ImU32       cross_color;
    float32_t   cross_size;
    bool_t      is_clicked;

    draw_list       = ImGui::GetWindowDrawList();
    button_size     = ImVec2(ImGui::GetTextLineHeight(), rowHeight);
    cross_size      = ImMin(ImGui::CalcTextSize(REMOVE_GLYPH).x, ImGui::GetTextLineHeight());
    cross_half_size = (ImVec2(cross_size, cross_size) * 0.5f);

    ImGui::SameLine();
    is_clicked = ImGui::InvisibleButton(label, button_size);
    ImGui::SetItemTooltip("%s", label);
    // Show the cross only while the pointer is on the row, as a modern editor keeps rows clean
    if (FALSE == ImGui::IsMouseHoveringRect(ImVec2(ImGui::GetWindowPos().x, ImGui::GetItemRectMin().y), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetItemRectMax().y)))
    {
        return is_clicked;
    }
    cross_center      = ((ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f);
    cross_min         = (cross_center - cross_half_size);
    cross_max         = (cross_center + cross_half_size);
    cross_left_bottom = ImVec2(cross_min.x, cross_max.y);
    cross_right_top   = ImVec2(cross_max.x, cross_min.y);
    cross_color       = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    if (ImGui::IsItemHovered())
    {
        cross_color = ImGui::GetColorU32(ImGuiCol_Text);
    }
    draw_list->AddLine(cross_min, cross_max, cross_color);
    draw_list->AddLine(cross_left_bottom, cross_right_top, cross_color);

    return is_clicked;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the widgets of one parent and recurses into each widget's children and
 *  extensions. Requested additions and removals are recorded for the caller to apply
 *  after traversal.
 */
static void DrawWidgetNodes(Arinc661_Definition_Document_t* document, uint32_t layerIndex, uint32_t parentIndex, HierarchyRequest_t* request)
{
    Arinc661_Widget_t*       widget;
    const char_t*            eye_label;
    const char_t*            lock_label;
    ImGuiTreeNodeFlags       flags;
    float32_t                row_height;
    uint32_t                 widget_index;
    bool_t                   is_open;

    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        widget = &document->widgets[widget_index];
        if ((layerIndex != widget->layerIndex) || (parentIndex != widget->parentWidgetIndex))
        {
            continue;
        }
        flags = HIERARCHY_BRANCH_FLAGS;
        if (widget->isExtension)
        {
            flags = HIERARCHY_LEAF_FLAGS;
        }
        if ((SELECTION_WIDGET == document->selectedKind) && (widget_index == document->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (('\0' != document->owner->hierarchyFilter[0]) && (FALSE == MatchesFilter(document, widget_index, document->owner->hierarchyFilter)))
        {
            continue;
        }
        ImGui::PushID(widget);
        // Eye and lock toggles in a left gutter, before the node
        if (FALSE == widget->isExtension)
        {
            eye_label = HIDE_LABEL;
            if (document->isHiddenInEditor[widget_index])
            {
                eye_label = SHOW_LABEL;
            }
            if (DrawFlagButton(ARINC661_ICON_EYE, eye_label, FALSE == document->isHiddenInEditor[widget_index]))
            {
                document->isHiddenInEditor[widget_index] = (byte_t)(FALSE == document->isHiddenInEditor[widget_index]);
            }
            ImGui::SameLine(0.0f, 2.0f);
            lock_label = LOCK_LABEL;
            if (document->isLocked[widget_index])
            {
                lock_label = UNLOCK_LABEL;
            }
            if (DrawFlagButton(ARINC661_ICON_LOCK, lock_label, FALSE != document->isLocked[widget_index]))
            {
                document->isLocked[widget_index] = (byte_t)(FALSE == document->isLocked[widget_index]);
            }
            ImGui::SameLine(0.0f, 4.0f);
        }
        ImGui::PopID();
        if (document->isHiddenInEditor[widget_index] || document->isLocked[widget_index])
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        }
        ImGui::PushID(widget);
        if (widget->isExtension)
        {
            is_open = FALSE;
            (void)ImGui::TreeNodeEx(WIDGET_NODE_ID, flags, EXTENSION_LABEL_FORMAT, widget->type);
        }
        else if (widget_index == document->owner->renamingWidget)
        {
            // Inline rename replaces the label until Enter or focus loss
            is_open = ImGui::TreeNodeEx(WIDGET_NODE_ID, flags | ImGuiTreeNodeFlags_AllowOverlap, "%s", "");
            ImGui::SameLine();
            ImGui::SetKeyboardFocusHere();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(RENAME_INPUT_ID, document->owner->renameBuffer, sizeof(document->owner->renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                ImStrncpy(widget->name, document->owner->renameBuffer, sizeof(widget->name));
                document->isDirty                = TRUE;
                document->owner->renamingWidget = INVALID_HIERARCHY_INDEX;
            }
            else if (ImGui::IsItemDeactivated() || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                document->owner->renamingWidget = INVALID_HIERARCHY_INDEX;
            }
        }
        else
        {
            is_open = ImGui::TreeNodeEx(WIDGET_NODE_ID, flags, WIDGET_LABEL_FORMAT, widget->name, widget->type);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                document->owner->renamingWidget = widget_index;
                ImStrncpy(document->owner->renameBuffer, widget->name, sizeof(document->owner->renameBuffer));
            }
        }
        if (document->isHiddenInEditor[widget_index] || document->isLocked[widget_index])
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemClicked() || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            if (ImGui::GetIO().KeyCtrl)
            {
                ToggleSelected(document, widget_index);
            }
            else
            {
                SelectOnly(document, widget_index);
            }
        }
        if (ImGui::BeginPopupContextItem())
        {
            DrawWidgetContextMenu(document, widget_index, request);
            ImGui::EndPopup();
        }
        row_height = ImGui::GetItemRectSize().y;
        if (widget->isExtension)
        {
            if (DrawRemoveButton(REMOVE_EXTENSION_LABEL, row_height))
            {
                request->removeWidgetIndex = widget_index;
            }
        }
        else if (DrawRemoveButton(REMOVE_WIDGET_LABEL, row_height))
        {
            request->removeWidgetIndex = widget_index;
        }
        ImGui::PopID();
        if (is_open)
        {
            DrawWidgetNodes(document, layerIndex, widget_index, request);
            (void)ImGui::TreeNodeEx(ADD_CHILD_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                request->addWidgetParentIndex = widget_index;
                request->addWidgetLayerIndex  = layerIndex;
            }
            (void)ImGui::TreeNodeEx(ADD_EXTENSION_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                request->addExtensionParentIndex = widget_index;
            }
            ImGui::TreePop();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the Definition File root, its layers, and each layer's widget tree, then applies
 *  the one request recorded during traversal.
 */
static void DrawHierarchy(Arinc661_Definition_Document_t* document)
{
    const Arinc661_Layer_t* layer;
    HierarchyRequest_t      request;
    ImGuiTreeNodeFlags      flags;
    char_t                  layer_id[ARINC661_MAX_PROPERTY_BYTES];
    float32_t               row_height;
    uint32_t                layer_index;
    uint32_t                property_index;
    uint32_t                parent_index;
    bool_t                  is_layer_open;

    request.removeLayerIndex        = INVALID_HIERARCHY_INDEX;
    request.removeWidgetIndex       = INVALID_HIERARCHY_INDEX;
    request.addWidgetLayerIndex     = INVALID_HIERARCHY_INDEX;
    request.addWidgetParentIndex    = INVALID_HIERARCHY_INDEX;
    request.addExtensionParentIndex = INVALID_HIERARCHY_INDEX;
    request.duplicateWidgetIndex     = INVALID_HIERARCHY_INDEX;
    flags = HIERARCHY_LEAF_FLAGS;
    if (SELECTION_DEFINITION == document->selectedKind)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    ImGui::SetNextItemWidth(-1.0f);
    (void)ImGui::InputTextWithHint("##hierarchy filter", HIERARCHY_FILTER_HINT, document->owner->hierarchyFilter, sizeof(document->owner->hierarchyFilter));
    (void)ImGui::TreeNodeEx(DEFINITION_NODE_ID, flags, DEFINITION_LABEL_FORMAT, document->definition.name);
    if (ImGui::IsItemClicked())
    {
        document->selectedKind  = SELECTION_DEFINITION;
        document->selectedIndex = 0;
    }
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem(TOOL_ADD_LAYER_LABEL))
        {
            AddLayer(document);
        }
        ImGui::EndPopup();
    }
    for (layer_index = 0; layer_index < document->definition.layerCount; ++layer_index)
    {
        layer = &document->layers[layer_index];
        flags = HIERARCHY_BRANCH_FLAGS;
        if ((SELECTION_LAYER == document->selectedKind) && (layer_index == document->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        layer_id[0]    = '\0';
        property_index = FindOwnedProperty(document, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_LAYER_ID);
        if (INVALID_HIERARCHY_INDEX != property_index)
        {
            ImStrncpy(layer_id, document->properties[property_index].value, sizeof(layer_id));
        }
        ImGui::PushID(layer);
        is_layer_open = ImGui::TreeNodeEx(LAYER_NODE_ID, flags, LAYER_LABEL_FORMAT, layer_id, layer->name);
        if (ImGui::IsItemClicked())
        {
            document->selectedKind  = SELECTION_LAYER;
            document->selectedIndex = layer_index;
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (NULL != ImGui::AcceptDragDropPayload(DROP_PAYLOAD_ID))
            {
                document->owner->dropWidget = *(const uint32_t*)ImGui::GetDragDropPayload()->Data;
                document->owner->dropParent = INVALID_HIERARCHY_INDEX;
                document->owner->dropLayer  = layer_index;
            }
            if (NULL != ImGui::AcceptDragDropPayload(PALETTE_PAYLOAD_ID))
            {
                ImStrncpy(document->owner->dropType, (const char_t*)ImGui::GetDragDropPayload()->Data, sizeof(document->owner->dropType));
                document->owner->dropTypeParent = INVALID_HIERARCHY_INDEX;
                document->owner->dropTypeLayer  = layer_index;
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem(ADD_CHILD_WIDGET_LABEL))
            {
                request.addWidgetLayerIndex  = layer_index;
                request.addWidgetParentIndex = INVALID_HIERARCHY_INDEX;
            }
            if (ImGui::MenuItem(REMOVE_LABEL))
            {
                request.removeLayerIndex = layer_index;
            }
            ImGui::EndPopup();
        }
        row_height = ImGui::GetItemRectSize().y;
        if (DrawRemoveButton(REMOVE_LAYER_LABEL, row_height))
        {
            request.removeLayerIndex = layer_index;
        }
        ImGui::PopID();
        if (is_layer_open)
        {
            DrawWidgetNodes(document, layer_index, INVALID_HIERARCHY_INDEX, &request);
            (void)ImGui::TreeNodeEx(ADD_WIDGET_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                request.addWidgetLayerIndex  = layer_index;
                request.addWidgetParentIndex = INVALID_HIERARCHY_INDEX;
            }
            ImGui::TreePop();
        }
    }
    (void)ImGui::TreeNodeEx(ADD_LAYER_LABEL, HIERARCHY_ADD_FLAGS);
    if (ImGui::IsItemClicked())
    {
        AddLayer(document);
    }
    if (INVALID_HIERARCHY_INDEX != request.removeLayerIndex)
    {
        RemoveLayer(document, request.removeLayerIndex);
        document->selectedKind  = SELECTION_DEFINITION;
        document->selectedIndex = 0;
        document->isDirty       = TRUE;
        document->status        = STATUS_LAYER_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != request.removeWidgetIndex)
    {
        parent_index     = document->widgets[request.removeWidgetIndex].parentWidgetIndex;
        document->status = STATUS_WIDGET_REMOVED;
        if (document->widgets[request.removeWidgetIndex].isExtension)
        {
            document->status = STATUS_EXTENSION_REMOVED;
        }
        document->selectedKind  = SELECTION_LAYER;
        document->selectedIndex = document->widgets[request.removeWidgetIndex].layerIndex;
        if (INVALID_HIERARCHY_INDEX != parent_index)
        {
            document->selectedKind  = SELECTION_WIDGET;
            document->selectedIndex = parent_index;
        }
        RemoveWidgetSubtree(document, request.removeWidgetIndex);
        document->isDirty = TRUE;
    }
    else if (INVALID_HIERARCHY_INDEX != request.duplicateWidgetIndex)
    {
        (void)DuplicateWidget(document, request.duplicateWidgetIndex);
    }
    else if (INVALID_HIERARCHY_INDEX != document->owner->dropWidget)
    {
        (void)MoveWidgetSubtree(document, document->owner->dropWidget, document->owner->dropLayer, document->owner->dropParent);
        document->owner->dropWidget = INVALID_HIERARCHY_INDEX;
    }
    else if ('\0' != document->owner->dropType[0])
    {
        AddTypedWidget(document, document->owner->dropTypeLayer, document->owner->dropTypeParent, document->owner->dropType, 0.0f, 0.0f, FALSE);
        document->owner->dropType[0] = '\0';
    }
    else if (INVALID_HIERARCHY_INDEX != request.addExtensionParentIndex)
    {
        AddWidget(document, document->widgets[request.addExtensionParentIndex].layerIndex, request.addExtensionParentIndex, TRUE);
    }
    else if (INVALID_HIERARCHY_INDEX != request.addWidgetLayerIndex)
    {
        AddWidget(document, request.addWidgetLayerIndex, request.addWidgetParentIndex, FALSE);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  All inspector groups use the same property and value columns.
 */
static bool_t BeginPropertyTable(const char_t* name)
{
    const ImGuiStyle* style;
    ImVec2            label_size;
    float32_t         property_width;
    bool_t            is_visible;

    style          = &ImGui::GetStyle();
    label_size     = ImGui::CalcTextSize(SUPPLEMENT_VERSION_LABEL);
    property_width = (label_size.x + (style->FramePadding.x * 2.0f));
    // Keep the value column at least as wide as the label column when the pane is narrow
    property_width = ImMin(property_width, ImGui::GetContentRegionAvail().x * 0.45f);
    is_visible     = ImGui::BeginTable(name, 2, PROPERTY_TABLE_FLAGS);
    if (is_visible)
    {
        ImGui::TableSetupColumn(PROPERTY_COLUMN, ImGuiTableColumnFlags_WidthFixed, property_width);
        ImGui::TableSetupColumn(VALUE_COLUMN, ImGuiTableColumnFlags_WidthStretch);
    }

    return is_visible;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws a bounded text input for a string-valued property.
 */
static bool_t DrawTextField(const char_t* label, char_t* value, uint32_t capacity)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits an unsigned 32-bit value in a property row.
 */
static bool_t DrawUnsignedField(const char_t* label, uint32_t* value)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputScalar(PROPERTY_INPUT_ID, ImGuiDataType_U32, value);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offers no encoding and the three Table 4.6-13 encodings; the index is the value.
 */
static bool_t DrawCharsetField(const char_t* label, uint32_t* value)
{
    int    selected;
    bool_t has_changed;

    selected = (int)(*value);
    if ((int)COUNT_OF(c_charsetNames) <= selected)
    {
        selected = 0;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::Combo(PROPERTY_INPUT_ID, &selected, c_charsetNames, (int)COUNT_OF(c_charsetNames));
    ImGui::PopID();
    if (has_changed)
    {
        (*value) = (uint32_t)selected;
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A new field edit replaces any stale save or load message in the status bar.
 */
static void MarkDocumentDirty(Arinc661_Definition_Document_t* editor, bool_t hasChanged)
{
    if (hasChanged)
    {
        editor->isDirty = TRUE;
        editor->status  = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Tells whether a property is edited by a typed row elsewhere in the inspector, so the
 *  generic table only shows what the schema does not cover.
 */
static bool_t IsSchemaProperty(const Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name)
{
    const Arinc661_Widget_t* widget;
    Arinc661_Parameter_t     parameter;
    uint32_t                 parameter_count;
    uint32_t                 parameter_index;
    bool_t                   is_schema;

    is_schema = FALSE;
    if (ARINC661_PROPERTY_DEFINITION == owner)
    {
        is_schema = (0 == strncmp(PROPERTY_APPLICATION_ID, name, sizeof(PROPERTY_APPLICATION_ID)));
    }
    else if (ARINC661_PROPERTY_LAYER == owner)
    {
        is_schema = !!((0 == strncmp(PROPERTY_LAYER_ID, name, sizeof(PROPERTY_LAYER_ID))) || (0 == strncmp(PROPERTY_CONTEXT_NUMBER, name, sizeof(PROPERTY_CONTEXT_NUMBER))) || (0 == strncmp(PROPERTY_WIDTH, name, sizeof(PROPERTY_WIDTH))) || (0 == strncmp(PROPERTY_HEIGHT, name, sizeof(PROPERTY_HEIGHT))));
    }
    else
    {
        widget    = &document->widgets[ownerIndex];
        is_schema = !!((FALSE == widget->isExtension) && (0 == strncmp(PROPERTY_WIDGET_IDENT, name, sizeof(PROPERTY_WIDGET_IDENT))));
        if ((FALSE == is_schema) && Arinc661_Definition_FindType(widget->type, widget->isExtension, &parameter_count))
        {
            for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
            {
                (void)Arinc661_Definition_GetParameter(widget->type, widget->isExtension, parameter_index, &parameter);
                if (0 == strncmp(parameter.name, name, ARINC661_MAX_NAME_BYTES))
                {
                    is_schema = TRUE;
                    break;
                }
            }
        }
    }

    return is_schema;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits one value in place according to the parameter kind: a combo over the listed
 *  constants, a ranged integer, a decimal, or text. A value that does not parse for its
 *  kind, such as a constant name in an integer parameter, falls back to text.
 */
static bool_t DrawParameterValue(const Arinc661_Parameter_t* parameter, char_t* value, uint32_t capacity)
{
    const char_t* text;
    float64_t     real_value;
    int64_t       integer_value;
    int64_t       minimum;
    int64_t       maximum;
    int32_t       consumed;
    int           selected;
    uint32_t      value_index;
    bool_t        has_changed;
    bool_t        is_number;

    has_changed = FALSE;
    ImGui::SetNextItemWidth(-1.0f);
    if (0 < parameter->valueCount)
    {
        selected = -1;
        for (value_index = 0; value_index < parameter->valueCount; ++value_index)
        {
            if (0 == strncmp(value, parameter->values[value_index], ARINC661_MAX_NAME_BYTES))
            {
                selected = (int)value_index;
                break;
            }
        }
        has_changed = ImGui::Combo(PROPERTY_INPUT_ID, &selected, parameter->values, (int)parameter->valueCount);
        if (has_changed)
        {
            ImStrncpy(value, parameter->values[selected], capacity);
        }
    }
    else if ((ARINC661_PARAMETER_UCHAR == parameter->kind) || (ARINC661_PARAMETER_USHORT == parameter->kind) || (ARINC661_PARAMETER_LONG == parameter->kind) || (ARINC661_PARAMETER_ULONG == parameter->kind))
    {
        consumed  = FACEOE_Utl_Scan_Int64(value, &integer_value);
        is_number = !!((0 < consumed) && ('\0' == value[consumed]));
        if (is_number)
        {
            minimum = 0;
            maximum = UINT32_MAX;
            if (ARINC661_PARAMETER_UCHAR == parameter->kind)
            {
                maximum = UINT8_MAX;
            }
            else if (ARINC661_PARAMETER_USHORT == parameter->kind)
            {
                maximum = UINT16_MAX;
            }
            else if (ARINC661_PARAMETER_LONG == parameter->kind)
            {
                minimum = INT32_MIN;
                maximum = INT32_MAX;
            }
            // Drag to scrub; Ctrl-click to type
            has_changed = ImGui::DragScalar(PROPERTY_INPUT_ID, ImGuiDataType_S64, &integer_value, 1.0f, NULL, NULL, "%lld");
            if (has_changed)
            {
                integer_value = ImClamp(integer_value, minimum, maximum);
                ImFormatString(value, capacity, "%" PRId64, integer_value);
            }
        }
        else
        {
            has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
        }
    }
    else if ((ARINC661_PARAMETER_FLOAT == parameter->kind) || (ARINC661_PARAMETER_FR180 == parameter->kind) || (ARINC661_PARAMETER_FR32768 == parameter->kind))
    {
        text      = value;
        consumed  = FACEOE_Utl_Scan_Float(text, &real_value);
        is_number = !!((0 < consumed) && ('\0' == text[consumed]));
        if (is_number)
        {
            has_changed = ImGui::DragScalar(PROPERTY_INPUT_ID, ImGuiDataType_Double, &real_value, 0.1f, NULL, NULL, "%.6g");
            if (has_changed)
            {
                if (ARINC661_PARAMETER_FR180 == parameter->kind)
                {
                    real_value = ImClamp(real_value, -180.0, 180.0);
                }
                else if (ARINC661_PARAMETER_FR32768 == parameter->kind)
                {
                    real_value = ImClamp(real_value, -32768.0, 32768.0);
                }
                ImFormatString(value, capacity, "%.6g", real_value);
            }
        }
        else
        {
            has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
        }
    }
    else
    {
        has_changed = ImGui::InputText(PROPERTY_INPUT_ID, value, capacity);
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws one labeled row for a parameter value held by a property, or by a scratch buffer
 *  when the property does not exist yet; an edit of a missing value creates the property.
 */
static bool_t DrawParameterRow(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* label, const char_t* propertyName, uint32_t occurrence, const Arinc661_Parameter_t* parameter)
{
    char_t        scratch[ARINC661_MAX_PROPERTY_BYTES];
    char_t*       value;
    const char_t* clipboard;
    uint32_t      property_index;
    uint32_t      other_index;
    uint32_t      other_property;
    bool_t        has_changed;
    bool_t        is_invalid;
    bool_t        is_shared;
    bool_t        is_mixed;

    property_index = FindOwnedPropertyAt(document, owner, ownerIndex, propertyName, occurrence);
    value          = scratch;
    scratch[0]     = '\0';
    if (INVALID_HIERARCHY_INDEX != property_index)
    {
        value = document->properties[property_index].value;
    }
    // Multi-object editing: a widget parameter other than the ident is shared with every other selected widget that has it
    is_shared = !!((ARINC661_PROPERTY_WIDGET == owner) && (1 < SelectedCount(document)) && (0 != strncmp(propertyName, PROPERTY_WIDGET_IDENT, sizeof(PROPERTY_WIDGET_IDENT))));
    is_mixed  = FALSE;
    for (other_index = 0; is_shared && (other_index < document->definition.widgetCount); ++other_index)
    {
        other_property = FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, other_index, propertyName, occurrence);
        if ((other_index != ownerIndex) && document->isSelected[other_index] && (INVALID_HIERARCHY_INDEX != other_property) && (0 != strncmp(document->properties[other_property].value, value, sizeof(document->properties[other_property].value))))
        {
            is_mixed = TRUE;
        }
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::PushID(label);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
    if (is_mixed)
    {
        ImGui::TextDisabled(MIXED_LABEL_FORMAT, label);
    }
    else
    {
        ImGui::TextUnformatted(label);
    }
    ImGui::PopTextWrapPos();
    DrawParameterTooltip(document, owner, ownerIndex, propertyName);
    has_changed = FALSE;
    // Right-click a parameter for reset, copy, and paste as an engine editor offers
    if (ImGui::BeginPopupContextItem(PROPERTY_MENU_ID))
    {
        if (ImGui::MenuItem(RESET_PROPERTY_LABEL))
        {
            DefaultParameterValue(parameter, scratch, sizeof(scratch));
            if (INVALID_HIERARCHY_INDEX != property_index)
            {
                ImStrncpy(value, scratch, ARINC661_MAX_PROPERTY_BYTES);
            }
            has_changed = TRUE;
        }
        if (ImGui::MenuItem(COPY_VALUE_LABEL))
        {
            ImGui::SetClipboardText(value);
        }
        clipboard = ImGui::GetClipboardText();
        if (ImGui::MenuItem(PASTE_VALUE_LABEL, NULL, false, (NULL != clipboard) && ('\0' != clipboard[0]) && (NULL == strchr(clipboard, '\n'))))
        {
            ImStrncpy(scratch, clipboard, sizeof(scratch));
            if (INVALID_HIERARCHY_INDEX != property_index)
            {
                ImStrncpy(value, scratch, ARINC661_MAX_PROPERTY_BYTES);
            }
            has_changed = TRUE;
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    // Show a value the binary writer would reject in the error color
    is_invalid = !!((INVALID_HIERARCHY_INDEX != property_index) && (FALSE == IsParameterValueValid(parameter, value)));
    if (is_invalid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
    }
    if ((ARINC661_PROPERTY_WIDGET == owner) && (REFERENCE_NONE != ReferenceKindOf(propertyName)) && (FALSE == document->widgets[ownerIndex].isExtension))
    {
        has_changed = !!(DrawReferenceValue(document, ownerIndex, ReferenceKindOf(propertyName), value, ARINC661_MAX_PROPERTY_BYTES) || has_changed);
    }
    else
    {
        has_changed = !!(DrawParameterValue(parameter, value, ARINC661_MAX_PROPERTY_BYTES) || has_changed);
    }
    if (is_invalid)
    {
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("%s", INVALID_VALUE_TOOLTIP);
    }
    if (is_shared && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
        ImGui::SetTooltip(SHARED_EDIT_FORMAT, (unsigned)SelectedCount(document));
    }
    ImGui::PopID();
    if (has_changed && (INVALID_HIERARCHY_INDEX == property_index))
    {
        has_changed = AppendProperty(document, owner, ownerIndex, propertyName, scratch);
    }
    // Apply the edit to the rest of the selection
    for (other_index = 0; has_changed && is_shared && (other_index < document->definition.widgetCount); ++other_index)
    {
        other_property = FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, other_index, propertyName, occurrence);
        if ((other_index != ownerIndex) && document->isSelected[other_index] && (FALSE == document->isLocked[other_index]) && (INVALID_HIERARCHY_INDEX != other_property))
        {
            ImStrncpy(document->properties[other_property].value, value, ARINC661_MAX_PROPERTY_BYTES);
        }
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Grows or shrinks an array's properties to its count parameter times the elements per
 *  entry, so the binary form and the rows shown always agree.
 */
static bool_t SyncArrayLength(Arinc661_Definition_Document_t* document, uint32_t widgetIndex, const Arinc661_Parameter_t* parameter)
{
    char_t    value[ARINC661_MAX_PROPERTY_BYTES];
    float32_t count;
    uint32_t  wanted;
    uint32_t  existing;
    uint32_t  property_index;
    bool_t    has_changed;

    count = 0.0f;
    (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->countName, &count);
    count       = ImClamp(count, 0.0f, (float32_t)UINT16_MAX);
    wanted      = ((uint32_t)count * parameter->elementsPerEntry);
    existing    = 0;
    has_changed = FALSE;
    while (INVALID_HIERARCHY_INDEX != FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->name, existing))
    {
        existing += 1;
    }
    DefaultParameterValue(parameter, value, sizeof(value));
    while ((existing < wanted) && AppendProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->name, value))
    {
        existing   += 1;
        has_changed = TRUE;
    }
    while (existing > wanted)
    {
        property_index = FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter->name, existing - 1);
        RemovePropertyAt(document, property_index);
        existing   -= 1;
        has_changed = TRUE;
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws every Creation Structure parameter of the widget's type: single values as one
 *  row each, arrays as one row per element after syncing their length to the count.
 */
static void DrawParameterTable(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    const Arinc661_Widget_t* widget;
    Arinc661_Parameter_t     parameter;
    char_t                   label[LABEL_FORMAT_BYTES + ARINC661_MAX_NAME_BYTES];
    uint32_t                 parameter_count;
    uint32_t                 parameter_index;
    uint32_t                 element_count;
    uint32_t                 element_index;

    widget = &document->widgets[widgetIndex];
    (void)Arinc661_Definition_FindType(widget->type, widget->isExtension, &parameter_count);
    ImGui::SetNextItemWidth(-1.0f);
    (void)ImGui::InputTextWithHint("##inspector filter", INSPECTOR_FILTER_HINT, document->owner->inspectorFilter, sizeof(document->owner->inspectorFilter));
    if (BeginPropertyTable(MODEL_TABLE_ID))
    {
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Definition_GetParameter(widget->type, widget->isExtension, parameter_index, &parameter);
            if (('\0' != document->owner->inspectorFilter[0]) && (NULL == ImStristr(parameter.name, NULL, document->owner->inspectorFilter, NULL)))
            {
                continue;
            }
            if (0 == parameter.elementsPerEntry)
            {
                MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name, parameter.name, 0, &parameter));
            }
            else
            {
                MarkDocumentDirty(document, SyncArrayLength(document, widgetIndex, &parameter));
                element_count = 0;
                while (INVALID_HIERARCHY_INDEX != FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, widgetIndex, parameter.name, element_count))
                {
                    element_count += 1;
                }
                for (element_index = 0; element_index < element_count; ++element_index)
                {
                    ImFormatString(label, sizeof(label), "%s[%u]", parameter.name, element_index);
                    MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_WIDGET, widgetIndex, label, parameter.name, element_index, &parameter));
                }
            }
        }
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offers every widget type, or every extension type, known to the binary form.
 */
static bool_t DrawTypeField(const char_t* label, char_t* type, uint32_t capacity, bool_t isExtension, const char_t* parentType)
{
    const char_t* preview;
    const char_t* description;
    const char_t* categories;
    const char_t* restriction;
    bool_t        is_applicable;
    uint32_t      type_count;
    uint32_t      type_index;
    int           selected;
    bool_t        has_changed;

    type_count = Arinc661_Definition_GetTypeCount(isExtension);
    selected   = -1;
    for (type_index = 0; type_index < type_count; ++type_index)
    {
        if (0 == strncmp(type, Arinc661_Definition_GetTypeName(isExtension, type_index), ARINC661_MAX_NAME_BYTES))
        {
            selected = (int)type_index;
            break;
        }
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = FALSE;
    preview     = UNKNOWN_TYPE_LABEL;
    if (0 <= selected)
    {
        preview = type;
    }
    if (ImGui::BeginCombo(PROPERTY_INPUT_ID, preview))
    {
        for (type_index = 0; type_index < type_count; ++type_index)
        {
            // Table 8.4: an extension the parent widget cannot take is shown greyed and not selectable
            is_applicable = !!((FALSE == isExtension) || (NULL == parentType) || Arinc661_Definition_ExtensionApplies(Arinc661_Definition_GetTypeName(isExtension, type_index), parentType));
            ImGui::BeginDisabled(FALSE == is_applicable);
            if (ImGui::Selectable(Arinc661_Definition_GetTypeName(isExtension, type_index), (int)type_index == selected))
            {
                ImStrncpy(type, Arinc661_Definition_GetTypeName(isExtension, type_index), capacity);
                has_changed = TRUE;
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && Arinc661_Help_GetType(Arinc661_Definition_GetTypeName(isExtension, type_index), &description, &categories, &restriction))
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(HELP_WRAP_WIDTH);
                ImGui::TextUnformatted(description);
                ImGui::TextDisabled(CATEGORIES_FORMAT, categories);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();

    return has_changed;
}
/*************************************************************************************************/
/**
 * \par Details:
 *  Lists one model's properties as editable name and value rows, each with a remove
 *  button, followed by an add row. Removal and addition are applied after the table.
 */
static void DrawModelProperties(Arinc661_Definition_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    Arinc661_Property_t* property;
    float32_t            row_height;
    uint32_t             property_index;
    uint32_t             remove_index;
    bool_t               should_add;

    remove_index = INVALID_HIERARCHY_INDEX;
    should_add   = FALSE;
    ImGui::SeparatorText(OTHER_SECTION_LABEL);
    if (ImGui::BeginTable(OTHER_TABLE_ID, 3, PROPERTY_TABLE_FLAGS))
    {
        ImGui::TableSetupColumn(PROPERTY_COLUMN, ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn(VALUE_COLUMN, ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::GetTextLineHeight());
        for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
        {
            property = &document->properties[property_index];
            if ((owner != property->owner) || (ownerIndex != property->ownerIndex) || IsSchemaProperty(document, owner, ownerIndex, property->name))
            {
                continue;
            }
            ImGui::PushID(property);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::SetNextItemWidth(-1.0f);
            MarkDocumentDirty(document, ImGui::InputText(PROPERTY_NAME_INPUT_ID, property->name, sizeof(property->name)));
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0f);
            MarkDocumentDirty(document, ImGui::InputText(PROPERTY_INPUT_ID, property->value, sizeof(property->value)));
            row_height = ImGui::GetItemRectSize().y;
            ImGui::TableSetColumnIndex(2);
            if (DrawRemoveButton(REMOVE_PROPERTY_LABEL, row_height))
            {
                remove_index = property_index;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (ImGui::Button(ADD_PROPERTY_LABEL))
    {
        should_add = TRUE;
    }
    if (INVALID_HIERARCHY_INDEX != remove_index)
    {
        RemovePropertyAt(document, remove_index);
        document->isDirty = TRUE;
        document->status  = STATUS_PROPERTY_REMOVED;
    }
    else if (should_add)
    {
        if (AppendProperty(document, owner, ownerIndex, "", ""))
        {
            document->isDirty = TRUE;
            document->status  = STATUS_PROPERTY_ADDED;
        }
        else
        {
            document->status = STATUS_PROPERTY_ADD_FAILED;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits the a661_df attributes, the charset encoding, and the root model.
 */
static void DrawDefinitionInspector(Arinc661_Definition_Document_t* document)
{
    ImGui::SeparatorText(DEFINITION_SECTION_LABEL);
    if (BeginPropertyTable(DEFINITION_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawTextField(NAME_LABEL, document->definition.name, sizeof(document->definition.name)));
        MarkDocumentDirty(document, DrawUnsignedField(LIBRARY_VERSION_LABEL, &document->definition.libraryVersion));
        MarkDocumentDirty(document, DrawUnsignedField(SUPPLEMENT_VERSION_LABEL, &document->definition.supplementVersion));
        MarkDocumentDirty(document, DrawCharsetField(CHARSET_ENCODING_LABEL, &document->definition.charsetEncoding));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::SeparatorText(MODEL_SECTION_LABEL);
    if (BeginPropertyTable(MODEL_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_DEFINITION, 0, PROPERTY_APPLICATION_ID, PROPERTY_APPLICATION_ID, 0, &c_ushortParameter));
        ImGui::EndTable();
    }
    DrawContextReferences(document);
    ImGui::Separator();
    DrawModelProperties(document, ARINC661_PROPERTY_DEFINITION, 0);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Lists every user application of the open configurations whose dFPath names this document,
 *  flags ApplicationId mismatches against FACE applicationId, offers the known user
 *  application identifiers for the ApplicationId property, and jumps to a user application.
 */
static void DrawContextReferences(Arinc661_Definition_Document_t* document)
{
    Arinc661_Context_t*                context;
    Arinc661_Context_UserApplication_t application;
    char_t                             label[HIERARCHY_LABEL_BYTES];
    char_t                             value[ARINC661_MAX_PROPERTY_BYTES];
    uint32_t                           property_index;
    uint32_t                           application_index;
    uint32_t                           reference_count;
    int64_t                            application_id;
    bool_t                             has_application_id;
    bool_t                             is_match;

    context = document->owner->context;
    if (NULL != context)
    {
        ImGui::Separator();
        ImGui::SeparatorText(REFERENCES_SECTION_LABEL);
        property_index     = FindOwnedProperty(document, ARINC661_PROPERTY_DEFINITION, 0, PROPERTY_APPLICATION_ID);
        has_application_id = !!((INVALID_HIERARCHY_INDEX != property_index) && (0 < FACEOE_Utl_Scan_Int64(document->properties[property_index].value, &application_id)));
        reference_count    = 0;
        for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
        {
            if (0 != _stricmp(application.dFPath, document->documentName))
            {
                continue;
            }
            reference_count += 1;
            is_match = !!(has_application_id && (application.applicationId == application_id));
            ImFormatString(label, sizeof(label), REFERENCE_FORMAT, application.documentName, (long long)application.applicationId, (long long)application.windowId);
            if (FALSE == is_match)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
            }
            ImGui::TextWrapped("%s", label);
            if (FALSE == is_match)
            {
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", APPLICATION_ID_MISMATCH_TOOLTIP);
                }
            }
            ImGui::PushID((int)application_index);
            if (ImGui::SmallButton(SHOW_USER_APPLICATION_LABEL))
            {
                Arinc661_Context_ShowUserApplication(context, &application);
            }
            if (has_application_id && (FALSE == is_match))
            {
                ImGui::SameLine();
                if (ImGui::SmallButton(ADOPT_APPLICATION_ID_LABEL))
                {
                    ImFormatString(value, sizeof(value), "%lld", (long long)application.applicationId);
                    strcpy(document->properties[property_index].value, value);
                    MarkDocumentDirty(document, TRUE);
                }
            }
            ImGui::PopID();
        }
        if (0 == reference_count)
        {
            ImGui::TextDisabled("%s", NO_REFERENCES_LABEL);
        }
        DrawCreateApplication(document);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offers the open configurations and their windows; the context creates a user application
 *  referencing this file with its ApplicationId.
 */
static void DrawCreateApplication(Arinc661_Definition_Document_t* document)
{
    Arinc661_Definition_Editor_t* owner;
    const char_t*                 configuration_name;
    const char_t*                 window_name;
    char_t                        label[HIERARCHY_LABEL_BYTES];
    int64_t                       window_id;
    uint32_t                      configuration_index;
    uint32_t                      window_index;
    uint32_t                      configuration_count;

    owner = document->owner;
    configuration_count = 0;
    for (configuration_index = 0; configuration_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++configuration_index)
    {
        if (NULL != Arinc661_Configuration_DocumentName(owner->context->configurationEditor, configuration_index))
        {
            configuration_count += 1;
        }
    }
    if (0 == configuration_count)
    {
        ImGui::TextDisabled("%s", NO_CONFIGURATION_LABEL);
    }
    else
    {
        if ((owner->createConfiguration >= ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) || (NULL == Arinc661_Configuration_DocumentName(owner->context->configurationEditor, owner->createConfiguration)))
        {
            for (configuration_index = 0; configuration_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++configuration_index)
            {
                if (NULL != Arinc661_Configuration_DocumentName(owner->context->configurationEditor, configuration_index))
                {
                    owner->createConfiguration = configuration_index;
                    break;
                }
            }
        }
        configuration_name = Arinc661_Configuration_DocumentName(owner->context->configurationEditor, owner->createConfiguration);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        if (ImGui::BeginCombo(CONFIGURATION_COMBO_ID, configuration_name))
        {
            for (configuration_index = 0; configuration_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++configuration_index)
            {
                if (NULL != Arinc661_Configuration_DocumentName(owner->context->configurationEditor, configuration_index))
                {
                    if (ImGui::Selectable(Arinc661_Configuration_DocumentName(owner->context->configurationEditor, configuration_index), configuration_index == owner->createConfiguration))
                    {
                        owner->createConfiguration = configuration_index;
                        owner->createWindow        = 0;
                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        label[0] = '\0';
        if (Arinc661_Configuration_WindowAt(owner->context->configurationEditor, owner->createConfiguration, owner->createWindow, &window_id, &window_name))
        {
            ImFormatString(label, sizeof(label), WINDOW_CHOICE_FORMAT, (long long)window_id, window_name);
        }
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo(WINDOW_COMBO_ID, label))
        {
            for (window_index = 0; Arinc661_Configuration_WindowAt(owner->context->configurationEditor, owner->createConfiguration, window_index, &window_id, &window_name); ++window_index)
            {
                ImFormatString(label, sizeof(label), WINDOW_CHOICE_FORMAT, (long long)window_id, window_name);
                ImGui::PushID((int)window_index);
                if (ImGui::Selectable(label, window_index == owner->createWindow))
                {
                    owner->createWindow = window_index;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        if (ImGui::SmallButton(CREATE_APPLICATION_LABEL) && Arinc661_Configuration_WindowAt(owner->context->configurationEditor, owner->createConfiguration, owner->createWindow, &window_id, &window_name))
        {
            (void)Arinc661_Context_CreateUserApplicationFor(owner->context, (uint32_t)(document - owner->documents), owner->createConfiguration, window_id);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits the layer name and its model.
 */
static void DrawLayerInspector(Arinc661_Definition_Document_t* document)
{
    Arinc661_Layer_t* layer;

    layer = &document->layers[document->selectedIndex];
    ImGui::SeparatorText(LAYER_SECTION_LABEL);
    if (BeginPropertyTable(LAYER_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawTextField(NAME_LABEL, layer->name, sizeof(layer->name)));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::SeparatorText(MODEL_SECTION_LABEL);
    if (BeginPropertyTable(MODEL_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_LAYER_ID, PROPERTY_LAYER_ID, 0, &c_ucharParameter));
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_CONTEXT_NUMBER, PROPERTY_CONTEXT_NUMBER, 0, &c_ushortParameter));
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_WIDTH, PROPERTY_WIDTH, 0, &c_ulongParameter));
        MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_LAYER, document->selectedIndex, PROPERTY_HEIGHT, PROPERTY_HEIGHT, 0, &c_ulongParameter));
        ImGui::EndTable();
    }
    ImGui::Separator();
    DrawModelProperties(document, ARINC661_PROPERTY_LAYER, document->selectedIndex);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Edits the widget or extension type, the widget name, and the model. The parent is
 *  shown for orientation and changed through the hierarchy, not here.
 */
static void DrawWidgetInspector(Arinc661_Definition_Document_t* document)
{
    Arinc661_Widget_t* widget;
    const char_t*      section_label;
    const char_t*      parent_name;
    const char_t*      parent_type;
    uint32_t           parameter_count;

    widget        = &document->widgets[document->selectedIndex];
    section_label = WIDGET_SECTION_LABEL;
    if (widget->isExtension)
    {
        section_label = EXTENSION_SECTION_LABEL;
    }
    parent_name = document->layers[widget->layerIndex].name;
    if (INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex)
    {
        parent_name = document->widgets[widget->parentWidgetIndex].name;
    }
    ImGui::SeparatorText(section_label);
    if (BeginPropertyTable(WIDGET_TABLE_ID))
    {
        if (FALSE == widget->isExtension)
        {
            MarkDocumentDirty(document, DrawTextField(NAME_LABEL, widget->name, sizeof(widget->name)));
        }
        parent_type = NULL;
        if (widget->isExtension && (INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex))
        {
            parent_type = document->widgets[widget->parentWidgetIndex].type;
        }
        if (DrawTypeField(TYPE_LABEL, widget->type, sizeof(widget->type), widget->isExtension, parent_type))
        {
            // Keep the parameters the new type shares and reseed the rest
            (void)ConformParameters(document, document->selectedIndex);
            MarkDocumentDirty(document, TRUE);
        }
        if (FALSE == widget->isExtension)
        {
            MarkDocumentDirty(document, DrawParameterRow(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex, PROPERTY_WIDGET_IDENT, PROPERTY_WIDGET_IDENT, 0, &c_ushortParameter));
        }
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(PARENT_LABEL);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(parent_name);
        ImGui::EndTable();
    }
    if (widget->isExtension && (NULL != parent_type) && (FALSE == Arinc661_Definition_ExtensionApplies(widget->type, parent_type)))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
        ImGui::TextWrapped(EXTENSION_NOT_APPLICABLE_FORMAT, widget->type, parent_type);
        ImGui::PopStyleColor();
    }
    ImGui::Separator();
    DrawTypeHelp(widget->type);
    if (Arinc661_Definition_FindType(widget->type, widget->isExtension, &parameter_count))
    {
        ImGui::SeparatorText(MODEL_SECTION_LABEL);
        DrawParameterTable(document, document->selectedIndex);
        ImGui::Separator();
    }
    DrawModelProperties(document, ARINC661_PROPERTY_WIDGET, document->selectedIndex);
    DrawReferencedBy(document, document->selectedIndex);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Every widget-reference parameter in the layer whose value equals this widget's ident is a
 *  reference; the list is the reference viewer of an engine editor, and a click selects.
 */
static void DrawReferencedBy(Arinc661_Definition_Document_t* document, uint32_t widgetIndex)
{
    char_t   label[LABEL_FORMAT_BYTES];
    uint32_t property_index;
    uint32_t owner_index;
    uint32_t count;
    uint32_t ident_property;

    ident_property = FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_WIDGET_IDENT);
    if (INVALID_HIERARCHY_INDEX == ident_property)
    {
        return;
    }
    count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        owner_index = document->properties[property_index].ownerIndex;
        if ((ARINC661_PROPERTY_WIDGET != document->properties[property_index].owner) || (owner_index == widgetIndex) || (owner_index >= document->definition.widgetCount))
        {
            continue;
        }
        if ((REFERENCE_WIDGET != ReferenceKindOf(document->properties[property_index].name)) || (document->widgets[owner_index].layerIndex != document->widgets[widgetIndex].layerIndex) || (0 != strncmp(document->properties[property_index].value, document->properties[ident_property].value, sizeof(document->properties[property_index].value))))
        {
            continue;
        }
        if (0 == count)
        {
            ImGui::SeparatorText(REFERENCED_BY_LABEL);
        }
        ImFormatString(label, sizeof(label), REFERENCED_BY_FORMAT, document->widgets[owner_index].name, document->widgets[owner_index].type, document->properties[property_index].name);
        ImGui::PushID((int)property_index);
        if (ImGui::Selectable(label))
        {
            SelectOnly(document, owner_index);
        }
        ImGui::PopID();
        count += 1;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Lists XML and binary Definition Files; a selected name is loaded through the folder path.
 */
static void DrawXmlFileEntry(const char_t* fileName, void* context)
{
    XmlFileListContext_t* list_context;
    bool_t                is_definition_file;

    list_context       = (XmlFileListContext_t*)context;
    is_definition_file = !!(HasExtension(fileName, XML_EXTENSION) || HasExtension(fileName, BINARY_EXTENSION));
    if (is_definition_file)
    {
        is_definition_file = (ImStrlen(fileName) < ARINC661_DEFINITION_MAX_XML_NAME_BYTES);
    }
    if (is_definition_file)
    {
        if (list_context->shouldDraw)
        {
            if (ImGui::Selectable(fileName))
            {
                LoadDefinitionFile(list_context->editor, fileName);
            }
        }
        else
        {
            ++list_context->xmlFileCount;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows a short scrolling list of XML files in the configured folder.
 */
static void DrawXmlFileList(Arinc661_Definition_Editor_t* editor, float32_t folderInputX)
{
    XmlFileListContext_t context;
    bool_t               is_enumerated;
    uint32_t             visible_row_count;
    float32_t            list_height;

    context.editor       = editor;
    context.xmlFileCount = 0;
    context.shouldDraw   = FALSE;
    is_enumerated        = OS_File_EnumerateFiles(editor->folderPath, DrawXmlFileEntry, &context);

    visible_row_count = 1;
    if (is_enumerated && (0 < context.xmlFileCount))
    {
        visible_row_count = context.xmlFileCount;
    }
    if (XML_LIST_MAX_ROWS < visible_row_count)
    {
        visible_row_count = XML_LIST_MAX_ROWS;
    }
    list_height = ((ImGui::GetTextLineHeightWithSpacing() * visible_row_count) + (ImGui::GetStyle().FramePadding.y * 2.0f));

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(XML_FILES_LABEL);
    ImGui::SameLine();
    ImGui::SetCursorPosX(folderInputX);
    if (ImGui::BeginListBox(XML_LIST_ID, ImVec2(-1.0f, list_height)))
    {
        if (FALSE == is_enumerated)
        {
            ImGui::TextUnformatted("Could not read folder.");
        }
        else if (0 == context.xmlFileCount)
        {
            ImGui::TextUnformatted("No XML files found.");
        }
        else
        {
            context.shouldDraw = TRUE;
            (void)OS_File_EnumerateFiles(editor->folderPath, DrawXmlFileEntry, &context);
        }
        ImGui::EndListBox();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps file controls together above the editor panes.
 */
static void DrawFileControls(Arinc661_Definition_Editor_t* editor)
{
    float32_t folder_input_x;

    if (ImGui::CollapsingHeader(FILE_CONTROLS_HEADER, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(FOLDER_PATH_LABEL);
        ImGui::SameLine();
        folder_input_x = ImGui::GetCursorPosX();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText(FOLDER_INPUT_ID, editor->folderPath, sizeof(editor->folderPath)))
        {
            if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
            {
                editor->status = STATUS_NONE;
            }
            else if ('\0' == editor->folderPath[0])
            {
                editor->status = STATUS_FOLDER_PATH_REQUIRED;
            }
            else
            {
                editor->status = STATUS_XML_FILE_REQUIRED;
            }
        }
        if ('\0' != editor->folderPath[0])
        {
            DrawXmlFileList(editor, folder_input_x);
        }
    }
    ImGui::Separator();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Frees the slot and moves the active tab to another loaded document, if any.
 */
static void CloseDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    uint32_t index;

    ResetDocument(&editor->documents[documentIndex]);
    if (documentIndex == editor->activeDocumentIndex)
    {
        editor->activeDocumentIndex    = INVALID_DOCUMENT_INDEX;
        editor->requestedDocumentIndex = INVALID_DOCUMENT_INDEX;
        for (index = 0; index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++index)
        {
            if (editor->documents[index].isLoaded)
            {
                editor->activeDocumentIndex    = index;
                editor->requestedDocumentIndex = index;
                break;
            }
        }
    }
    if (INVALID_DOCUMENT_INDEX == editor->activeDocumentIndex)
    {
        editor->status = STATUS_XML_FILE_REQUIRED;
    }
    else
    {
        editor->status = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps document actions in the editor's menu bar.
 */
static void DrawFileMenuBar(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        document_index;
    bool_t                          has_active_document;
    bool_t                          has_dirty_documents;
    bool_t                          has_available_document;

    has_active_document    = (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex);
    has_dirty_documents    = FALSE;
    has_available_document = FALSE;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        if (document->isLoaded)
        {
            if (document->isDirty)
            {
                has_dirty_documents = TRUE;
            }
        }
        else
        {
            has_available_document = TRUE;
        }
    }
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(FILE_MENU_LABEL))
        {
            if (ImGui::MenuItem(NEW_MENU_LABEL, NEW_SHORTCUT_LABEL, false, has_available_document))
            {
                CreateDocument(editor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(SAVE_MENU_LABEL, SAVE_SHORTCUT_LABEL, false, has_active_document))
            {
                editor->isSaveAllPending = FALSE;
                SaveDocument(editor, editor->activeDocumentIndex);
            }
            if (ImGui::MenuItem(SAVE_ALL_MENU_LABEL, NULL, false, has_dirty_documents))
            {
                SaveAllDocuments(editor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(CLOSE_MENU_LABEL, NULL, false, has_active_document))
            {
                document = &editor->documents[editor->activeDocumentIndex];
                if (document->isDirty)
                {
                    editor->pendingCloseDocumentIndex = editor->activeDocumentIndex;
                }
                else
                {
                    CloseDocument(editor, editor->activeDocumentIndex);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if (ImGui::Shortcut(NEW_SHORTCUT, SAVE_SHORTCUT_FLAGS))
    {
        CreateDocument(editor);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each tab points to its own fixed document slot. The unsaved flag displays a dot on dirty tabs.
 */
static void DrawDocumentTabs(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    ImGuiTabItemFlags               tab_flags;
    ImVec2                          popup_center;
    char_t                          tab_label[DOCUMENT_TAB_LABEL_BYTES];
    uint32_t                        document_index;
    uint32_t                        document_to_close;
    bool                            is_tab_open;
    bool                            is_tab_visible;
    bool_t                          has_available_document;
    bool_t                          has_pending_close;
    bool_t                          has_pending_save;
    bool_t                          should_create_document;

    has_available_document = FALSE;
    should_create_document = FALSE;
    document_to_close      = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (FALSE == editor->documents[document_index].isLoaded)
        {
            has_available_document = TRUE;
        }
    }
    if (ImGui::BeginTabBar(DOCUMENT_TAB_BAR_ID))
    {
        for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
        {
            document = &editor->documents[document_index];
            if (document->isLoaded)
            {
                tab_flags = ImGuiTabItemFlags_None;
                if (document->isDirty)
                {
                    tab_flags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                if (document_index == editor->requestedDocumentIndex)
                {
                    tab_flags |= ImGuiTabItemFlags_SetSelected;
                }
                ImFormatString(tab_label, sizeof(tab_label), DOCUMENT_TAB_FORMAT, document->documentName, document_index);
                is_tab_open    = true;
                is_tab_visible = ImGui::BeginTabItem(tab_label, &is_tab_open, tab_flags);
                if (is_tab_visible)
                {
                    editor->activeDocumentIndex = document_index;
                    ImGui::EndTabItem();
                }
                if (false == is_tab_open)
                {
                    if (document->isDirty)
                    {
                        editor->pendingCloseDocumentIndex = document_index;
                        editor->activeDocumentIndex       = document_index;
                    }
                    else
                    {
                        document_to_close = document_index;
                    }
                }
            }
        }
        if (has_available_document)
        {
            should_create_document = ImGui::TabItemButton(NEW_DOCUMENT_TAB_LABEL, NEW_DOCUMENT_TAB_FLAGS);
            ImGui::SetItemTooltip(NEW_DOCUMENT_TAB_TOOLTIP);
        }
        ImGui::EndTabBar();
    }
    editor->requestedDocumentIndex = INVALID_DOCUMENT_INDEX;
    if (INVALID_DOCUMENT_INDEX != document_to_close)
    {
        CloseDocument(editor, document_to_close);
    }
    if (should_create_document)
    {
        CreateDocument(editor);
    }
    has_pending_close = (INVALID_DOCUMENT_INDEX != editor->pendingCloseDocumentIndex);
    has_pending_save  = (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex);
    if (has_pending_close && (FALSE == has_pending_save))
    {
        document = &editor->documents[editor->pendingCloseDocumentIndex];
        popup_center = (ImGui::GetWindowPos() + (ImGui::GetWindowSize() * 0.5f));
        ImGui::OpenPopup(CLOSE_DOCUMENT_POPUP_ID);
        ImGui::SetNextWindowPos(popup_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(CLOSE_DOCUMENT_POPUP_ID, NULL, CLOSE_DOCUMENT_POPUP_FLAGS))
        {
            ImGui::TextUnformatted(CLOSE_DOCUMENT_PROMPT);
            if (ImGui::Button(SAVE_DOCUMENT_LABEL))
            {
                SaveDocument(editor, editor->pendingCloseDocumentIndex);
                if (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex)
                {
                    ImGui::CloseCurrentPopup();
                }
                else if (FALSE == document->isDirty)
                {
                    CloseDocument(editor, editor->pendingCloseDocumentIndex);
                    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(DISCARD_DOCUMENT_LABEL))
            {
                CloseDocument(editor, editor->pendingCloseDocumentIndex);
                editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(CANCEL_CLOSE_LABEL))
            {
                editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                ImGui::CloseCurrentPopup();
            }
            if (STATUS_WRITE_FAILED == document->status)
            {
                ImGui::TextUnformatted(EditorStatusToString(document->status));
            }
            if (STATUS_SERIALIZATION_FAILED == document->status)
            {
                ImGui::TextUnformatted(EditorStatusToString(document->status));
            }
            ImGui::EndPopup();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Names an untitled document in an existing folder before its first write.
 */
static void DrawSaveAsDialog(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    FileMatchContext_t              match_context;
    const char_t*                   status_text;
    char_t                          file_path[ARINC661_DEFINITION_MAX_FILE_PATH_BYTES];
    char_t                          previous_name[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];
    uint32_t                        document_index;
    uint32_t                        open_document_index;
    bool_t                          has_folder_path;
    bool_t                          has_file_name;
    bool_t                          has_valid_name;
    bool_t                          is_enumerated;
    bool_t                          is_already_open;
    bool_t                          was_save_all_pending;

    if (INVALID_DOCUMENT_INDEX != editor->pendingSaveDocumentIndex)
    {
        document_index = editor->pendingSaveDocumentIndex;
        document       = &editor->documents[document_index];
        ImGui::OpenPopup(SAVE_AS_POPUP_ID);
        if (ImGui::BeginPopupModal(SAVE_AS_POPUP_ID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(SAVE_AS_FOLDER_LABEL);
            ImGui::SetNextItemWidth(SAVE_DIALOG_INPUT_WIDTH);
            if (ImGui::InputText(SAVE_AS_FOLDER_INPUT_ID, editor->saveFolderPath, sizeof(editor->saveFolderPath)))
            {
                editor->status = STATUS_NONE;
            }
            ImGui::TextUnformatted(SAVE_AS_NAME_LABEL);
            ImGui::SetNextItemWidth(SAVE_DIALOG_INPUT_WIDTH);
            if (ImGui::InputText(SAVE_AS_NAME_INPUT_ID, editor->saveFileName, sizeof(editor->saveFileName)))
            {
                editor->status = STATUS_NONE;
            }
            if (ImGui::Button(SAVE_AS_CONFIRM_LABEL))
            {
                has_folder_path = ('\0' != editor->saveFolderPath[0]);
                has_file_name   = ('\0' != editor->saveFileName[0]);
                has_valid_name  = IsValidXmlFileName(editor->saveFileName);
                if (FALSE == has_folder_path)
                {
                    editor->status = STATUS_FOLDER_PATH_REQUIRED;
                }
                else if (FALSE == has_file_name)
                {
                    editor->status = STATUS_SAVE_NAME_REQUIRED;
                }
                else if (FALSE == has_valid_name)
                {
                    editor->status = STATUS_SAVE_NAME_INVALID;
                }
                else
                {
                    match_context.fileName = editor->saveFileName;
                    match_context.isFound  = FALSE;
                    is_enumerated = OS_File_EnumerateFiles(editor->saveFolderPath, FindMatchingFile, &match_context);
                    if (FALSE == is_enumerated)
                    {
                        editor->status = STATUS_SAVE_FOLDER_INVALID;
                    }
                    else if (match_context.isFound)
                    {
                        editor->status = STATUS_SAVE_FILE_EXISTS;
                    }
                    else
                    {
                        JoinFolderPath(editor->saveFolderPath, editor->saveFileName, file_path, sizeof(file_path));
                        is_already_open = FALSE;
                        for (open_document_index = 0; open_document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++open_document_index)
                        {
                            if (editor->documents[open_document_index].isLoaded)
                            {
                                if (0 == ImStricmp(editor->documents[open_document_index].documentPath, file_path))
                                {
                                    is_already_open = TRUE;
                                }
                            }
                        }
                        if (is_already_open)
                        {
                            editor->status = STATUS_SAVE_FILE_EXISTS;
                        }
                        else
                        {
                            ImStrncpy(previous_name, document->documentName, sizeof(previous_name));
                            ImStrncpy(document->documentName, editor->saveFileName, sizeof(document->documentName));
                            ImStrncpy(document->documentPath, file_path, sizeof(document->documentPath));
                            SaveLoadedDocument(document);
                            if (document->isDirty)
                            {
                                editor->status           = document->status;
                                document->documentPath[0] = '\0';
                                ImStrncpy(document->documentName, previous_name, sizeof(document->documentName));
                            }
                            else
                            {
                                // Let the context offer to repoint user applications at the new name
                                if (0 != ImStricmp(previous_name, document->documentName))
                                {
                                    ImStrncpy(editor->renamedFrom, previous_name, sizeof(editor->renamedFrom));
                                    ImStrncpy(editor->renamedTo, document->documentName, sizeof(editor->renamedTo));
                                }
                                editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
                                editor->status                   = STATUS_NONE;
                                ImGui::CloseCurrentPopup();
                                if (document_index == editor->pendingCloseDocumentIndex)
                                {
                                    CloseDocument(editor, document_index);
                                    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                                }
                                else if (editor->isSaveAllPending)
                                {
                                    SaveAllDocuments(editor);
                                }
                            }
                        }
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(SAVE_AS_CANCEL_LABEL))
            {
                was_save_all_pending = editor->isSaveAllPending;
                editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
                editor->isSaveAllPending          = FALSE;
                if (document_index == editor->pendingCloseDocumentIndex)
                {
                    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
                }
                if (was_save_all_pending)
                {
                    editor->status = STATUS_SAVE_ALL_CANCELED;
                }
                else
                {
                    editor->status = STATUS_NONE;
                }
                ImGui::CloseCurrentPopup();
            }
            status_text = EditorStatusToString(editor->status);
            if ('\0' != status_text[0])
            {
                ImGui::TextUnformatted(status_text);
            }
            ImGui::EndPopup();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the current document message or file browser error.
 */
static void DrawStatusBar(const Arinc661_Definition_Editor_t* editor)
{
    const Arinc661_Definition_Document_t* document;
    const char_t*                         status_text;

    status_text = EditorStatusToString(editor->status);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        if ('\0' == status_text[0])
        {
            document    = &editor->documents[editor->activeDocumentIndex];
            status_text = EditorStatusToString(document->status);
        }
    }
    ImGui::Separator();
    if ('\0' == status_text[0])
    {
        status_text = READY_LABEL;
    }
    ImGui::TextUnformatted(status_text);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps the hierarchy in the left child pane.
 */
static void DrawHierarchyPane(Arinc661_Definition_Document_t* editor)
{
    if (ImGui::BeginChild(HIERARCHY_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (editor->isLoaded)
        {
            DrawHierarchy(editor);
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches to the inspector for the selected entity.
 */
static void DrawInspectorPane(Arinc661_Definition_Document_t* editor)
{
    if (ImGui::BeginChild(INSPECTOR_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        ImGui::PushTextWrapPos(0.0f);
        if (editor->isLoaded)
        {
            if (SELECTION_DEFINITION == editor->selectedKind)
            {
                DrawDefinitionInspector(editor);
            }
            else if (SELECTION_LAYER == editor->selectedKind)
            {
                DrawLayerInspector(editor);
            }
            else if (SELECTION_WIDGET == editor->selectedKind)
            {
                DrawWidgetInspector(editor);
            }
            else
            {
                ImGui::TextWrapped("%s", SELECT_ITEM_PROMPT);
            }
        }
        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Sums PosX and PosY through the container chain, since section 3.1.3 positions a widget
 *  relative to its parent's lower-left corner. Extensions and widgets without all four
 *  properties leave isValid FALSE.
 */
static void ResolveWidgetArea(const Arinc661_Definition_Document_t* document, uint32_t widgetIndex, WidgetArea_t* area)
{
    float32_t position_x;
    float32_t position_y;
    uint32_t  ancestor_index;
    uint32_t  depth;

    area->isValid = !!((FALSE == document->widgets[widgetIndex].isExtension) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_POS_X, &area->x) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_POS_Y, &area->y) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_SIZE_X, &area->width) &&
                       GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, PROPERTY_SIZE_Y, &area->height));
    ancestor_index = document->widgets[widgetIndex].parentWidgetIndex;
    for (depth = 0; area->isValid && (depth < document->definition.widgetCount); ++depth)
    {
        if (INVALID_HIERARCHY_INDEX == ancestor_index)
        {
            break;
        }
        // Add no offset for containers without a position, such as connectors
        if (GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, ancestor_index, PROPERTY_POS_X, &position_x) &&
            GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, ancestor_index, PROPERTY_POS_Y, &position_y))
        {
            area->x += position_x;
            area->y += position_y;
        }
        ancestor_index = document->widgets[ancestor_index].parentWidgetIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the selected layer, or the layer owning the selected widget, scaled to fit the
 *  pane with its origin at the lower-left corner as in section 3.1.3. Each positioned
 *  widget is outlined and labeled; clicking the smallest widget under the cursor selects it.
 */
static void DrawScenePane(Arinc661_Definition_Document_t* editor)
{
    const ImGuiStyle*             style;
    Arinc661_Definition_Editor_t* owner;
    Arinc661_Scene_Options_t      options;
    FACEOE_Gfx_Transform_t        transform;
    ImDrawList*                   draw_list;
    ImVec2                        canvas_min;
    ImVec2                        canvas_size;
    ImVec2                        canvas_max;
    ImVec2                        layer_min;
    ImVec2                        layer_size;
    char_t                        layer_label[HIERARCHY_LABEL_BYTES];
    const char_t*                 width_text;
    const char_t*                 height_text;
    float32_t                     layer_width;
    float32_t                     layer_height;
    float32_t                     label_height;
    float32_t                     scale;
    uint32_t                      layer_index;
    uint32_t                      property_index;
    char_t                        zoom_label[LABEL_FORMAT_BYTES];
    char_t                        readout[LABEL_FORMAT_BYTES * 2];
    WidgetArea_t                  zoom_area;
    char_t                        frame_label[FRAME_LABEL_BYTES];
    ImVec2                        frame_min;
    ImVec2                        frame_max;
    float32_t                     frame_width;
    float32_t                     frame_height;
    uint32_t                      frame_index;
    float32_t                     zoom_factor;
    bool_t                        has_layer;
    bool_t                        is_canvas_hovered;

    owner = editor->owner;
    // A primary selection made elsewhere (hierarchy, problems, context) becomes the whole selection
    if ((SELECTION_WIDGET == editor->selectedKind) && (editor->selectedIndex < editor->definition.widgetCount) && (FALSE == editor->isSelected[editor->selectedIndex]))
    {
        SelectOnly(editor, editor->selectedIndex);
    }
    else if (SELECTION_WIDGET != editor->selectedKind)
    {
        ClearSelectionFlags(editor);
    }
    if (ImGui::BeginChild(SCENE_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        style     = &ImGui::GetStyle();
        draw_list = ImGui::GetWindowDrawList();
        layer_index = 0;
        if (SELECTION_LAYER == editor->selectedKind)
        {
            layer_index = editor->selectedIndex;
        }
        else if (SELECTION_WIDGET == editor->selectedKind)
        {
            layer_index = editor->widgets[editor->selectedIndex].layerIndex;
        }
        if (editor->isLoaded)
        {
            DrawSceneToolbar(editor, &layer_index);
        }
        else
        {
            ImGui::TextUnformatted(SCENE_PANE_NAME);
        }
        ImGui::Separator();
        has_layer = !!(editor->isLoaded && (layer_index < editor->definition.layerCount));
        if (FALSE == editor->isLoaded)
        {
            ImGui::TextWrapped("%s", "No Definition File loaded.");
        }
        else if (FALSE == has_layer)
        {
            ImGui::TextWrapped("%s", EMPTY_SCENE_LABEL);
            if (ImGui::Button(EMPTY_LAYER_BUTTON_LABEL))
            {
                AddLayer(editor);
            }
        }
        else
        {
            Arinc661_Scene_LayerSize(&editor->definition, layer_index, &layer_width, &layer_height);
            width_text  = "";
            height_text = "";
            property_index = FindOwnedProperty(editor, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_WIDTH);
            if (INVALID_HIERARCHY_INDEX != property_index)
            {
                width_text = editor->properties[property_index].value;
            }
            property_index = FindOwnedProperty(editor, ARINC661_PROPERTY_LAYER, layer_index, PROPERTY_HEIGHT);
            if (INVALID_HIERARCHY_INDEX != property_index)
            {
                height_text = editor->properties[property_index].value;
            }
            ImFormatString(layer_label, sizeof(layer_label), LAYER_PREVIEW_FORMAT, editor->layers[layer_index].name, width_text, height_text);
            label_height = ImGui::GetTextLineHeightWithSpacing();
            canvas_min   = ImGui::GetCursorScreenPos();
            canvas_size  = ImGui::GetContentRegionAvail();
            canvas_size.x = ImMax(canvas_size.x, 1.0f);
            canvas_size.y = ImMax(canvas_size.y, label_height + 1.0f);
            canvas_max    = (canvas_min + canvas_size);
            (void)ImGui::InvisibleButton(SCENE_CANVAS_ID, canvas_size);
            draw_list->AddRectFilled(canvas_min, canvas_max, SCENE_CANVAS_COLOR);
            // Fit the layer below its label while keeping its aspect ratio, then apply the view zoom and pan
            scale      = ImMin((canvas_size.x - (style->WindowPadding.x * 2.0f)) / layer_width, (canvas_size.y - label_height - (style->WindowPadding.y * 2.0f)) / layer_height);
            scale      = ImMax(scale, 0.0f);
            if (editor->viewZoom <= 0.0f)
            {
                editor->viewZoom = 1.0f;
                editor->viewPanX = 0.0f;
                editor->viewPanY = 0.0f;
            }
            is_canvas_hovered = ImGui::IsItemHovered();
            if (is_canvas_hovered && (0.0f != ImGui::GetIO().MouseWheel) && (0.0f < scale))
            {
                // Zoom about the cursor: keep the layer point under the mouse fixed
                zoom_factor = ZOOM_WHEEL_FACTOR;
                if (ImGui::GetIO().MouseWheel < 0.0f)
                {
                    zoom_factor = (1.0f / ZOOM_WHEEL_FACTOR);
                }
                zoom_factor = (ImClamp(editor->viewZoom * zoom_factor, ZOOM_MIN, ZOOM_MAX) / editor->viewZoom);
                layer_size  = ImVec2(layer_width * scale * editor->viewZoom, layer_height * scale * editor->viewZoom);
                layer_min   = ImVec2(canvas_min.x + ((canvas_size.x - layer_size.x) * 0.5f) + editor->viewPanX, canvas_min.y + label_height + ((canvas_size.y - label_height - layer_size.y) * 0.5f) + editor->viewPanY);
                layer_min   = ImVec2(ImGui::GetMousePos().x - ((ImGui::GetMousePos().x - layer_min.x) * zoom_factor), ImGui::GetMousePos().y - ((ImGui::GetMousePos().y - layer_min.y) * zoom_factor));
                editor->viewZoom *= zoom_factor;
                layer_size  = ImVec2(layer_width * scale * editor->viewZoom, layer_height * scale * editor->viewZoom);
                editor->viewPanX = (layer_min.x - (canvas_min.x + ((canvas_size.x - layer_size.x) * 0.5f)));
                editor->viewPanY = (layer_min.y - (canvas_min.y + label_height + ((canvas_size.y - label_height - layer_size.y) * 0.5f)));
            }
            if (is_canvas_hovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || (ImGui::IsKeyDown(ImGuiKey_Space) && ImGui::IsMouseDragging(ImGuiMouseButton_Left))))
            {
                editor->viewPanX += ImGui::GetIO().MouseDelta.x;
                editor->viewPanY += ImGui::GetIO().MouseDelta.y;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
            if (is_canvas_hovered && (FALSE == ImGui::IsAnyItemActive()) && ImGui::IsKeyPressed(ImGuiKey_F))
            {
                editor->viewZoom = 1.0f;
                editor->viewPanX = 0.0f;
                editor->viewPanY = 0.0f;
            }
            if (is_canvas_hovered && (FALSE == ImGui::IsAnyItemActive()) && ImGui::IsKeyPressed(ImGuiKey_G))
            {
                owner->showsGrid = !owner->showsGrid;
            }
            if (is_canvas_hovered && (FALSE == ImGui::IsAnyItemActive()) && ImGui::IsKeyPressed(ImGuiKey_S) && (FALSE == ImGui::GetIO().KeyCtrl))
            {
                owner->isSnapping = !owner->isSnapping;
            }
            if (owner->zoomsToSelection || (is_canvas_hovered && (FALSE == ImGui::IsAnyItemActive()) && ImGui::IsKeyPressed(ImGuiKey_Z)))
            {
                // Zoom so the primary selection fills most of the canvas, centred
                owner->zoomsToSelection = FALSE;
                if ((SELECTION_WIDGET == editor->selectedKind) && (editor->selectedIndex < editor->definition.widgetCount))
                {
                    ResolveWidgetArea(editor, editor->selectedIndex, &zoom_area);
                    if (zoom_area.isValid && (0.0f < zoom_area.width) && (0.0f < zoom_area.height) && (0.0f < scale))
                    {
                        editor->viewZoom = ImClamp(ImMin((canvas_size.x * 0.6f) / (zoom_area.width * scale), (canvas_size.y * 0.6f) / (zoom_area.height * scale)), ZOOM_MIN, ZOOM_MAX);
                        layer_size       = ImVec2(layer_width * scale * editor->viewZoom, layer_height * scale * editor->viewZoom);
                        // Pan so the widget's centre sits at the canvas centre
                        editor->viewPanX = ((canvas_size.x * 0.5f) - (layer_size.x * 0.5f)) - (((canvas_size.x - layer_size.x) * 0.5f)) - (((zoom_area.x + (zoom_area.width * 0.5f)) * scale * editor->viewZoom) - (layer_size.x * 0.5f));
                        editor->viewPanY = (((canvas_size.y - label_height) * 0.5f) - (layer_size.y * 0.5f)) - (((canvas_size.y - label_height - layer_size.y) * 0.5f)) + (((zoom_area.y + (zoom_area.height * 0.5f)) * scale * editor->viewZoom) - (layer_size.y * 0.5f));
                    }
                }
            }
            scale     *= editor->viewZoom;
            layer_size = ImVec2(layer_width * scale, layer_height * scale);
            layer_min  = ImVec2(canvas_min.x + ((canvas_size.x - layer_size.x) * 0.5f) + editor->viewPanX, canvas_min.y + label_height + ((canvas_size.y - label_height - layer_size.y) * 0.5f) + editor->viewPanY);
            ImFormatString(zoom_label, sizeof(zoom_label), SCENE_ZOOM_FORMAT, (float64_t)(editor->viewZoom * 100.0f));
            draw_list->AddText(ImVec2(canvas_min.x + style->WindowPadding.x, canvas_min.y + style->WindowPadding.y * 0.5f), SCENE_TEXT_COLOR, layer_label);
            draw_list->AddText(ImVec2(canvas_max.x - style->WindowPadding.x - ImGui::CalcTextSize(zoom_label).x, canvas_min.y + style->WindowPadding.y * 0.5f), SCENE_TEXT_COLOR, zoom_label);
            // Record the layer through the scene library with the origin at its lower-left corner
            memset(&transform, 0, sizeof(transform));
            transform.a = scale;
            transform.d = -scale;
            transform.e = layer_min.x;
            transform.f = (layer_min.y + layer_size.y);
            FACEOE_Gfx_DrawList_Clear(&owner->drawList);
            FACEOE_Gfx_DrawList_SetTransform(&owner->drawList, &transform);
            memset(&options, 0, sizeof(options));
            options.showsHidden       = TRUE;
            options.showsNames        = TRUE;
            options.showsNonGraphical = TRUE;
            options.skipFlags         = editor->isHiddenInEditor;
            options.animationTime     = owner->animationTime;
            options.fillsLayerArea    = FALSE;
            options.fontSize          = ImGui::GetFontSize();
            options.bounds            = owner->sceneBounds;
            options.boundsCapacity    = ARINC661_DEFINITION_MAX_WIDGETS;
            Arinc661_Scene_DrawLayer(&editor->definition, layer_index, &owner->drawList, &options);
            owner->sceneBoundsCount = options.boundsCount;
            draw_list->PushClipRect(canvas_min, canvas_max, true);
            draw_list->AddRectFilled(layer_min, layer_min + layer_size, SCENE_LAYER_COLOR);
            draw_list->AddRect(layer_min, layer_min + layer_size, SCENE_LAYER_BORDER_COLOR);
            if (owner->showsGrid)
            {
                DrawGrid(draw_list, layer_min, layer_min + layer_size, scale, canvas_min, canvas_max);
            }
            Arinc661_Gfx_ImGui_Render(&owner->drawList, draw_list);
            // Outline the windows that will show this file, from the layer origin, in layer units
            if (NULL != owner->context)
            {
                for (frame_index = 0; Arinc661_Context_WindowFrame(owner->context, editor->documentName, frame_index, &frame_width, &frame_height, frame_label, sizeof(frame_label)); ++frame_index)
                {
                    frame_max = ImVec2(layer_min.x + (frame_width * scale), layer_min.y + layer_size.y);
                    frame_min = ImVec2(layer_min.x, (layer_min.y + layer_size.y) - (frame_height * scale));
                    DrawDashedScreenRect(draw_list, frame_min, frame_max, SCENE_FRAME_COLOR);
                    draw_list->AddText(ImVec2(frame_min.x + 4.0f, frame_max.y - ImGui::GetTextLineHeight() - 2.0f), SCENE_FRAME_COLOR, frame_label);
                }
            }
            DrawSceneInteraction(editor, layer_index, layer_min, layer_min + layer_size, scale, draw_list, is_canvas_hovered);
            DrawRulers(draw_list, ImVec2(canvas_min.x, canvas_min.y + label_height), canvas_max, layer_min, layer_min + layer_size, scale);
            if (is_canvas_hovered && (0.0f < scale))
            {
                // Cursor readout in layer units, and the selection count, at the top-right
                if (1 < SelectedCount(editor))
                {
                    ImFormatString(readout, sizeof(readout), SELECTION_COUNT_FORMAT "   " CURSOR_READOUT_FORMAT, (unsigned)SelectedCount(editor), (float64_t)((ImGui::GetMousePos().x - layer_min.x) / scale), (float64_t)((layer_min.y + layer_size.y - ImGui::GetMousePos().y) / scale));
                }
                else
                {
                    ImFormatString(readout, sizeof(readout), CURSOR_READOUT_FORMAT, (float64_t)((ImGui::GetMousePos().x - layer_min.x) / scale), (float64_t)((layer_min.y + layer_size.y - ImGui::GetMousePos().y) / scale));
                }
                draw_list->AddText(ImVec2(canvas_max.x - style->WindowPadding.x - ImGui::CalcTextSize(readout).x, canvas_min.y + label_height + SCENE_RULER_SIZE + style->WindowPadding.y * 0.5f), SCENE_TEXT_COLOR, readout);
            }
            draw_list->PopClipRect();
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps the hierarchy controls visible and stops the left divider at the right divider.
 *  Stops the right divider when the inspector reaches the hierarchy's width.
 */
static void ClampPaneResize(float32_t minimumHierarchyWidth)
{
    ImGuiTable* table;
    float32_t   maximum_hierarchy_width;
    float32_t   maximum_scene_width;
    float32_t   minimum_divider_spacing;
    float32_t   minimum_hierarchy_width;

    table = ImGui::GetCurrentTable();
    if (HIERARCHY_PANE_COLUMN_IDX == table->ResizedColumn)
    {
        minimum_divider_spacing  = (table->MinColumnWidth + (table->CellPaddingX * 2.0f));
        minimum_divider_spacing += table->CellSpacingX1;
        minimum_divider_spacing += table->CellSpacingX2;

        maximum_hierarchy_width  = table->Columns[INSPECTOR_PANE_COLUMN].MinX;
        maximum_hierarchy_width -= table->Columns[HIERARCHY_PANE_COLUMN_IDX].MaxX;
        maximum_hierarchy_width += table->Columns[HIERARCHY_PANE_COLUMN_IDX].WidthGiven;
        maximum_hierarchy_width -= minimum_divider_spacing;

        minimum_hierarchy_width = ImMin(minimumHierarchyWidth, maximum_hierarchy_width);

        table->ResizedColumnNextWidth = ImClamp(table->ResizedColumnNextWidth, minimum_hierarchy_width, maximum_hierarchy_width);
    }
    else if (SCENE_PANE_COLUMN_IDX == table->ResizedColumn)
    {
        maximum_scene_width = (table->Columns[SCENE_PANE_COLUMN_IDX].WidthRequest + table->Columns[INSPECTOR_PANE_COLUMN].WidthRequest);
        maximum_scene_width -= table->Columns[HIERARCHY_PANE_COLUMN_IDX].WidthGiven;
        table->ResizedColumnNextWidth = ImMin(table->ResizedColumnNextWidth, maximum_scene_width);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Sizes the hierarchy to its widest row and the inspector to its label and value columns.
 */
static void MeasurePaneWidths(const Arinc661_Definition_Document_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth)
{
    const ImGuiStyle* style;
    ImVec2            button_padding;
    char_t            hierarchy_label[HIERARCHY_LABEL_BYTES];
    float32_t         hierarchy_row_padding;
    float32_t         hierarchy_row_width;
    float32_t         property_width;
    float32_t         value_width;
    uint32_t          layer_index;
    uint32_t          widget_index;
    uint32_t          depth;
    uint32_t          ancestor_index;

    style = &ImGui::GetStyle();
    button_padding = (style->FramePadding + style->FramePadding);
    property_width = (ImGui::CalcTextSize(SUPPLEMENT_VERSION_LABEL).x + button_padding.x);
    value_width    = (ImGui::CalcTextSize(DEFAULT_EXTENSION_TYPE).x + button_padding.x);
    (*hierarchyWidth) = (ImGui::CalcTextSize(HIERARCHY_PANE_NAME).x + (style->WindowPadding.x * 2.0f));
    hierarchy_row_padding  = (style->WindowPadding.x * 2.0f);
    hierarchy_row_padding += ImGui::GetTreeNodeToLabelSpacing();
    hierarchy_row_padding += style->ItemSpacing.x;
    hierarchy_row_padding += ImGui::GetTextLineHeight();
    hierarchy_row_width = (ImGui::CalcTextSize(ADD_CHILD_LABEL).x + hierarchy_row_padding + (style->IndentSpacing * 2.0f));
    (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    ImFormatString(hierarchy_label, sizeof(hierarchy_label), DEFINITION_LABEL_FORMAT, editor->definition.name);
    hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding);
    (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    for (layer_index = 0; layer_index < editor->definition.layerCount; ++layer_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), LAYER_LABEL_FORMAT, "000", editor->layers[layer_index].name);
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding);
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }
    for (widget_index = 0; widget_index < editor->definition.widgetCount; ++widget_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), WIDGET_LABEL_FORMAT, editor->widgets[widget_index].name, editor->widgets[widget_index].type);
        // Indent once for the layer and once per container above the widget
        depth          = 1;
        ancestor_index = editor->widgets[widget_index].parentWidgetIndex;
        while ((INVALID_HIERARCHY_INDEX != ancestor_index) && (depth < editor->definition.widgetCount))
        {
            depth         += 1;
            ancestor_index = editor->widgets[ancestor_index].parentWidgetIndex;
        }
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding + (style->IndentSpacing * (float32_t)depth));
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }
    (*hierarchyWidth) += style->ScrollbarSize;
    (*inspectorWidth) = (property_width + value_width);
    (*inspectorWidth) += (style->CellPadding.x * 4.0f);
    (*inspectorWidth) += (style->WindowPadding.x * 2.0f);
    (*inspectorWidth) += style->ScrollbarSize;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the active document in three resizable panes above the status bar.
 */
static void DrawPanes(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    ImVec2                          table_size;
    float32_t                       minimum_hierarchy_width;
    float32_t                       inspector_width;
    float32_t                       status_bar_height;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    MeasurePaneWidths(document, &minimum_hierarchy_width, &inspector_width);
    status_bar_height = (ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y);
    table_size        = ImGui::GetContentRegionAvail();
    table_size.y      = ImMax(ImGui::GetTextLineHeight(), (table_size.y - status_bar_height));
    ImGui::PushID(document);
    if (ImGui::BeginTable(PANE_TABLE_ID, PANE_COUNT, PANE_TABLE_FLAGS, table_size))
    {
        ImGui::TableSetupColumn(HIERARCHY_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, minimum_hierarchy_width);
        ImGui::TableSetupColumn(SCENE_PANE_NAME, ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(INSPECTOR_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, inspector_width);
        ClampPaneResize(minimum_hierarchy_width);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
        ImGui::TableNextRow();
        ImGui::PopStyleVar();
        ImGui::TableSetColumnIndex(HIERARCHY_PANE_COLUMN_IDX);
        DrawHierarchyPane(document);
        ImGui::TableSetColumnIndex(INSPECTOR_PANE_COLUMN);
        DrawInspectorPane(document);
        ImGui::TableSetColumnIndex(SCENE_PANE_COLUMN_IDX);
        DrawScenePane(document);
        ImGui::EndTable();
    }
    ImGui::PopID();
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Points every document's Definition File at its owned arrays before clearing it.
 */
void Arinc661_Definition_Initialize(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        document_index;

    editor->status                    = STATUS_XML_FILE_REQUIRED;
    editor->activeDocumentIndex       = INVALID_DOCUMENT_INDEX;
    editor->requestedDocumentIndex    = INVALID_DOCUMENT_INDEX;
    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
    editor->pendingSaveDocumentIndex  = INVALID_DOCUMENT_INDEX;
    editor->isSaveAllPending          = FALSE;
    ImStrncpy(editor->folderPath, DEFAULT_FOLDER_PATH, sizeof(editor->folderPath));
    (void)memset(editor->saveFolderPath, 0, sizeof(editor->saveFolderPath));
    (void)memset(editor->saveFileName, 0, sizeof(editor->saveFileName));
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        (void)memset(document->documentName, 0, sizeof(document->documentName));
        (void)memset(document->documentPath, 0, sizeof(document->documentPath));
        (void)memset(document->inputBuffer, 0, sizeof(document->inputBuffer));
        (void)memset(document->outputBuffer, 0, sizeof(document->outputBuffer));
        (void)memset(document->workspace, 0, sizeof(document->workspace));
        (void)memset(document->layers, 0, sizeof(document->layers));
        (void)memset(document->widgets, 0, sizeof(document->widgets));
        (void)memset(document->properties, 0, sizeof(document->properties));
        (void)memset(document->blocks, 0, sizeof(document->blocks));
        Arinc661_Definition_Initialize(&document->definition, document->layers, ARINC661_DEFINITION_MAX_LAYERS, document->widgets, ARINC661_DEFINITION_MAX_WIDGETS, document->properties, ARINC661_DEFINITION_MAX_PROPERTIES, document->blocks, ARINC661_DEFINITION_MAX_BLOCK_BYTES);
        ResetDocument(document);
    }
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        editor->documents[document_index].owner = editor;
    }
    editor->dropWidget     = INVALID_HIERARCHY_INDEX;
    editor->renamingWidget = INVALID_HIERARCHY_INDEX;
    editor->animationTime  = -1.0f;
    editor->showsGrid  = TRUE;
    editor->isSnapping = TRUE;
    FACEOE_Gfx_DrawList_Initialize(&editor->drawList, editor->sceneCommands, ARINC661_DEFINITION_MAX_SCENE_COMMANDS, editor->scenePoints, ARINC661_DEFINITION_MAX_SCENE_POINTS, editor->sceneText, ARINC661_DEFINITION_MAX_SCENE_TEXT_BYTES);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws file controls above the panes and a full-width status bar below them.
 */
void Arinc661_Definition_DrawEditor(Arinc661_Definition_Editor_t* editor)
{
    const ImGuiViewport* viewport;

    viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    if (ImGui::Begin(WINDOW_NAME, NULL, WINDOW_FLAGS))
    {
        DrawFileMenuBar(editor);
        DrawFileControls(editor);
        DrawDocumentTabs(editor);
        DrawSaveAsDialog(editor);
        DrawPanes(editor);
        if (ImGui::Shortcut(SAVE_SHORTCUT, SAVE_SHORTCUT_FLAGS))
        {
            if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
            {
                editor->isSaveAllPending = FALSE;
                SaveDocument(editor, editor->activeDocumentIndex);
            }
        }
        DrawStatusBar(editor);
    }
    ImGui::End();
    Arinc661_Definition_EndFrame(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Replaces the folder; the next file list shows that folder's Definition Files.
 */
void Arinc661_Definition_SetFolderPath(Arinc661_Definition_Editor_t* editor, const char_t* folderPath)
{
    ImStrncpy(editor->folderPath, folderPath, sizeof(editor->folderPath));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as selecting the file in the file list.
 */
void Arinc661_Definition_LoadFile(Arinc661_Definition_Editor_t* editor, const char_t* fileName)
{
    LoadDefinitionFile(editor, fileName);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as the File menu's New.
 */
void Arinc661_Definition_CreateDocument(Arinc661_Definition_Editor_t* editor)
{
    CreateDocument(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as the File menu's Save; nothing happens without an active document.
 */
void Arinc661_Definition_SaveActiveDocument(Arinc661_Definition_Editor_t* editor)
{
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        editor->isSaveAllPending = FALSE;
        SaveDocument(editor, editor->activeDocumentIndex);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as the File menu's Save All.
 */
void Arinc661_Definition_SaveAllDocuments(Arinc661_Definition_Editor_t* editor)
{
    SaveAllDocuments(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A changed document is closed through the prompt drawn by the document tabs.
 */
void Arinc661_Definition_CloseActiveDocument(Arinc661_Definition_Editor_t* editor)
{
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        if (editor->documents[editor->activeDocumentIndex].isDirty)
        {
            editor->pendingCloseDocumentIndex = editor->activeDocumentIndex;
        }
        else
        {
            CloseDocument(editor, editor->activeDocumentIndex);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Scans the document slots once for all three answers.
 */
bool_t Arinc661_Definition_HasActiveDocument(const Arinc661_Definition_Editor_t* editor, bool_t* isDirty, bool_t* hasFree)
{
    uint32_t document_index;
    bool_t   is_dirty;
    bool_t   has_free;

    is_dirty = FALSE;
    has_free = FALSE;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (editor->documents[document_index].isLoaded)
        {
            is_dirty = !!(is_dirty || editor->documents[document_index].isDirty);
        }
        else
        {
            has_free = TRUE;
        }
    }
    if (NULL != isDirty)
    {
        (*isDirty) = is_dirty;
    }
    if (NULL != hasFree)
    {
        (*hasFree) = has_free;
    }

    return (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws into the current window.
 */
void Arinc661_Definition_DrawDocumentTabs(Arinc661_Definition_Editor_t* editor)
{

    DrawDocumentTabs(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws nothing without an active document.
 */
void Arinc661_Definition_DrawHierarchy(Arinc661_Definition_Editor_t* editor)
{
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        DrawHierarchy(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the first slot, which is never loaded, when no document is active.
 */
void Arinc661_Definition_DrawScene(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    ImGui::PushID(document);
    DrawScenePane(document);
    ImGui::PopID();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the first slot, which is never loaded, when no document is active.
 */
void Arinc661_Definition_DrawInspector(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    ImGui::PushID(document);
    DrawInspectorPane(document);
    ImGui::PopID();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The close prompt belongs to the document tabs; only the save-as dialog is drawn here.
 */
void Arinc661_Definition_DrawDialogs(Arinc661_Definition_Editor_t* editor)
{
    DrawSaveAsDialog(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The editor message wins over the active document's message.
 */
const char_t* Arinc661_Definition_StatusText(const Arinc661_Definition_Editor_t* editor)
{
    const char_t* status_text;

    status_text = EditorStatusToString(editor->status);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && ('\0' == status_text[0]))
    {
        status_text = EditorStatusToString(editor->documents[editor->activeDocumentIndex].status);
    }

    return status_text;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Measures the first slot, which is never loaded, when no document is active.
 */
void Arinc661_Definition_MeasurePanes(const Arinc661_Definition_Editor_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth)
{
    const Arinc661_Definition_Document_t* document;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    MeasurePaneWidths(document, hierarchyWidth, inspectorWidth);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A binary Definition File starts with A661_DF_MAGIC_NUMBER; an XML one has a661_df as its
 *  document element, which is found after any declaration, comments, and whitespace.
 */
bool_t Arinc661_Definition_IsDefinitionFile(const byte_t* bytes, uint64_t byteCount)
{
    const char_t* text;
    const char_t* element;
    uint64_t      index;
    bool_t        is_definition;

    is_definition = !!((1 < byteCount) && (BINARY_MAGIC_HIGH == bytes[0]) && (BINARY_MAGIC_LOW == bytes[1]));
    if (FALSE == is_definition)
    {
        text  = (const char_t*)bytes;
        index = 0;
        // Skip the declaration, comments, and processing instructions to the first element
        while (index < byteCount)
        {
            if ('<' != text[index])
            {
                index += 1;
                continue;
            }
            if (((index + 1) < byteCount) && (('?' == text[index + 1]) || ('!' == text[index + 1])))
            {
                while ((index < byteCount) && ('>' != text[index]))
                {
                    index += 1;
                }
                continue;
            }
            break;
        }
        element       = DEFINITION_ROOT_ELEMENT;
        is_definition = !!(((index + ImStrlen(element)) < byteCount) && (0 == memcmp(&text[index], element, ImStrlen(element))));
        if (is_definition)
        {
            // Require a complete element name
            is_definition = !!((' ' == text[index + ImStrlen(element)]) || ('>' == text[index + ImStrlen(element)]) || ('/' == text[index + ImStrlen(element)]) || ('\n' == text[index + ImStrlen(element)]) || ('\r' == text[index + ImStrlen(element)]) || ('\t' == text[index + ImStrlen(element)]));
        }
    }

    return is_definition;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Stores the pointer; the inspector consults it when drawing.
 */
void Arinc661_Definition_SetContext(Arinc661_Definition_Editor_t* editor, Arinc661_Context_t* context)
{
    LOGASSERT(NULL != editor);
    editor->context = context;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares against each loaded document's tab name, ignoring case.
 */
uint32_t Arinc661_Definition_FindDocument(const Arinc661_Definition_Editor_t* editor, const char_t* fileName)
{
    uint32_t document_index;
    uint32_t found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != fileName);
    found = ARINC661_DEFINITION_NO_DOCUMENT;
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (editor->documents[document_index].isLoaded && (0 == _stricmp(fileName, editor->documents[document_index].documentName)))
        {
            found = document_index;
            break;
        }
    }

    return found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  NULL for an empty slot.
 */
const Arinc661_Definition_t* Arinc661_Definition_Document(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    const Arinc661_Definition_t* definition;

    LOGASSERT(NULL != editor);
    definition = NULL;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        definition = &editor->documents[documentIndex].definition;
    }

    return definition;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  NULL for an empty slot.
 */
const char_t* Arinc661_Definition_DocumentName(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    const char_t* name;

    LOGASSERT(NULL != editor);
    name = NULL;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        name = editor->documents[documentIndex].documentName;
    }

    return name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the ApplicationId model property of the a661_df element.
 */
bool_t Arinc661_Definition_DocumentApplicationId(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, int64_t* applicationId)
{
    const Arinc661_Definition_Document_t* document;
    uint32_t                              property_index;
    bool_t                                has_id;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != applicationId);
    has_id = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document       = &editor->documents[documentIndex];
        property_index = FindOwnedProperty(document, ARINC661_PROPERTY_DEFINITION, 0, PROPERTY_APPLICATION_ID);
        if (INVALID_HIERARCHY_INDEX != property_index)
        {
            has_id = (0 < FACEOE_Utl_Scan_Int64(document->properties[property_index].value, applicationId));
        }
    }

    return has_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The tab bar honours the request on its next draw.
 */
void Arinc661_Definition_SelectDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    LOGASSERT(NULL != editor);
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        editor->activeDocumentIndex    = documentIndex;
        editor->requestedDocumentIndex = documentIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layers are recorded in order so later layers overdraw earlier ones, as in a window.
 */
bool_t Arinc661_Definition_RecordDocument(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize)
{
    const Arinc661_Definition_Document_t* document;
    Arinc661_Scene_Options_t              options;
    uint32_t                              layer_index;
    bool_t                                has_layers;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != drawList);
    has_layers = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document = &editor->documents[documentIndex];
        memset(&options, 0, sizeof(options));
        options.fontSize      = fontSize;
        options.animationTime = editor->animationTime;
        for (layer_index = 0; layer_index < document->definition.layerCount; ++layer_index)
        {
            Arinc661_Scene_DrawLayer(&document->definition, layer_index, drawList, &options);
            has_layers = TRUE;
        }
    }

    return has_layers;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Definition_Undo(Arinc661_Definition_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        UndoDocument(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Definition_Redo(Arinc661_Definition_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        RedoDocument(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uncommitted changes of the current frame count as undoable.
 */
void Arinc661_Definition_History(const Arinc661_Definition_Editor_t* editor, bool_t* canUndo, bool_t* canRedo)
{
    const Arinc661_Definition_Document_t* document;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != canUndo);
    LOGASSERT(NULL != canRedo);
    (*canUndo) = FALSE;
    (*canRedo) = FALSE;
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document   = &editor->documents[editor->activeDocumentIndex];
        (*canUndo) = !!((0 < document->undoCount) || ((NULL != document->shadow) && (FALSE == SnapshotMatches(document, document->shadow))));
        (*canRedo) = (0 < document->redoCount);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Definition_DeleteSelection(Arinc661_Definition_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        RemoveSelection(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document's selected widget.
 */
void Arinc661_Definition_DuplicateSelection(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;

    LOGASSERT(NULL != editor);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            (void)DuplicateWidget(document, document->selectedIndex);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Snapping rounds moves, sizes, and add points to whole millimetres.
 */
bool_t Arinc661_Definition_Snap(Arinc661_Definition_Editor_t* editor, bool_t toggle)
{
    LOGASSERT(NULL != editor);
    if (toggle)
    {
        editor->isSnapping = !editor->isSnapping;
    }

    return editor->isSnapping;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The grid is one millimetre with a stronger line every centimetre.
 */
bool_t Arinc661_Definition_Grid(Arinc661_Definition_Editor_t* editor, bool_t toggle)
{
    LOGASSERT(NULL != editor);
    if (toggle)
    {
        editor->showsGrid = !editor->showsGrid;
    }

    return editor->showsGrid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Resets zoom and pan.
 */
void Arinc661_Definition_FitView(Arinc661_Definition_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        editor->documents[editor->activeDocumentIndex].viewZoom = 1.0f;
        editor->documents[editor->activeDocumentIndex].viewPanX = 0.0f;
        editor->documents[editor->activeDocumentIndex].viewPanY = 0.0f;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Definition_AddLayer(Arinc661_Definition_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        AddLayer(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A selected container receives the widget; a selected leaf gets a sibling; a selected
 *  layer or the first layer gets a direct child. A document without layers gets one first.
 */
void Arinc661_Definition_AddWidget(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        layer_index;
    uint32_t                        parent_index;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if (0 == document->definition.layerCount)
        {
            AddLayer(document);
        }
        layer_index  = 0;
        parent_index = INVALID_HIERARCHY_INDEX;
        if ((SELECTION_LAYER == document->selectedKind) && (document->selectedIndex < document->definition.layerCount))
        {
            layer_index = document->selectedIndex;
        }
        else if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            layer_index  = document->widgets[document->selectedIndex].layerIndex;
            parent_index = document->selectedIndex;
            if (document->widgets[parent_index].isExtension || (FALSE == IsContainerType(document->widgets[parent_index].type)))
            {
                parent_index = document->widgets[parent_index].parentWidgetIndex;
            }
        }
        if (0 < document->definition.layerCount)
        {
            AddWidget(document, layer_index, parent_index, FALSE);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document's selected widget.
 */
void Arinc661_Definition_AddExtension(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        parent_index;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            parent_index = document->selectedIndex;
            if (document->widgets[parent_index].isExtension)
            {
                parent_index = document->widgets[parent_index].parentWidgetIndex;
            }
            AddWidget(document, document->widgets[parent_index].layerIndex, parent_index, TRUE);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  NULL unless a widget or extension is selected.
 */
const char_t* Arinc661_Definition_SelectedType(const Arinc661_Definition_Editor_t* editor)
{
    const Arinc661_Definition_Document_t* document;
    const char_t*                         type;

    LOGASSERT(NULL != editor);
    type = NULL;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            type = document->widgets[document->selectedIndex].type;
        }
    }

    return type;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares names exactly.
 */
bool_t Arinc661_Definition_SelectByName(Arinc661_Definition_Editor_t* editor, const char_t* name)
{
    Arinc661_Definition_Document_t* document;
    const char_t*                   cursor;
    size_t                          name_length;
    uint32_t                        widget_index;
    bool_t                          is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != name);
    is_found = FALSE;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        // A comma-separated list selects several widgets; the first named is primary
        for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
        {
            name_length = strlen(document->widgets[widget_index].name);
            cursor      = name;
            while (NULL != cursor)
            {
                if ((0 == strncmp(cursor, document->widgets[widget_index].name, name_length)) && ((',' == cursor[name_length]) || ('\0' == cursor[name_length])))
                {
                    if (FALSE == is_found)
                    {
                        ClearSelectionFlags(document);
                        document->selectedKind  = SELECTION_WIDGET;
                        document->selectedIndex = widget_index;
                    }
                    document->isSelected[widget_index] = TRUE;
                    is_found                           = TRUE;
                    break;
                }
                cursor = strchr(cursor, ',');
                if (NULL != cursor)
                {
                    cursor += 1;
                }
            }
        }
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Turns the changes of the frame into one undo step per changed document.
 */
void Arinc661_Definition_EndFrame(Arinc661_Definition_Editor_t* editor)
{
    uint32_t document_index;

    LOGASSERT(NULL != editor);
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        CommitHistory(&editor->documents[document_index]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Starts an untitled document, names it after the user application's dFPath, writes the
 *  ApplicationId model property, and adds one layer whose Width and Height match the window.
 */
bool_t Arinc661_Definition_CreateFor(Arinc661_Definition_Editor_t* editor, const char_t* fileName, int64_t applicationId, float32_t layerWidth, float32_t layerHeight)
{
    Arinc661_Definition_Document_t* document;
    char_t                          text[INTEGER_TEXT_BYTES];
    uint32_t                        property_index;
    bool_t                          is_created;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != fileName);
    is_created = FALSE;
    CreateDocument(editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded && (0 == editor->documents[editor->activeDocumentIndex].definition.layerCount))
    {
        document = &editor->documents[editor->activeDocumentIndex];
        ImStrncpy(document->documentName, fileName, sizeof(document->documentName));
        ImStrncpy(document->definition.name, fileName, sizeof(document->definition.name));
        ImFormatString(text, sizeof(text), "%" PRId64, applicationId);
        property_index = FindOwnedProperty(document, ARINC661_PROPERTY_DEFINITION, 0, PROPERTY_APPLICATION_ID);
        if (INVALID_HIERARCHY_INDEX != property_index)
        {
            ImStrncpy(document->properties[property_index].value, text, sizeof(document->properties[property_index].value));
        }
        else
        {
            (void)AppendProperty(document, ARINC661_PROPERTY_DEFINITION, 0, PROPERTY_APPLICATION_ID, text);
        }
        AddLayer(document);
        if (0 < document->definition.layerCount)
        {
            SetPropertyNumber(document, ARINC661_PROPERTY_LAYER, 0, PROPERTY_WIDTH, ImMax(1.0f, layerWidth));
            SetPropertyNumber(document, ARINC661_PROPERTY_LAYER, 0, PROPERTY_HEIGHT, ImMax(1.0f, layerHeight));
        }
        document->isDirty = TRUE;
        is_created        = TRUE;
    }

    return is_created;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The tab bar honours the document request on its next draw.
 */
void Arinc661_Definition_SelectLayer(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t layerIndex)
{
    LOGASSERT(NULL != editor);
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (layerIndex < editor->documents[documentIndex].definition.layerCount))
    {
        Arinc661_Definition_SelectDocument(editor, documentIndex);
        editor->documents[documentIndex].selectedKind  = SELECTION_LAYER;
        editor->documents[documentIndex].selectedIndex = layerIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The tab bar honours the document request on its next draw.
 */
void Arinc661_Definition_SelectWidget(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t widgetIndex)
{
    LOGASSERT(NULL != editor);
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (widgetIndex < editor->documents[documentIndex].definition.widgetCount))
    {
        Arinc661_Definition_SelectDocument(editor, documentIndex);
        editor->documents[documentIndex].selectedKind  = SELECTION_WIDGET;
        editor->documents[documentIndex].selectedIndex = widgetIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Like Arinc661_Definition_RecordDocument, with the bounds of every drawn widget appended
 *  across the layers so a host can pick through the preview.
 */
bool_t Arinc661_Definition_RecordDocumentBounds(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize, Arinc661_Scene_Bounds_t* bounds, uint32_t boundsCapacity, uint32_t* boundsCount)
{
    const Arinc661_Definition_Document_t* document;
    Arinc661_Scene_Options_t              options;
    uint32_t                              layer_index;
    uint32_t                              total;
    bool_t                                has_layers;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != drawList);
    has_layers = FALSE;
    total      = 0;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document = &editor->documents[documentIndex];
        for (layer_index = 0; layer_index < document->definition.layerCount; ++layer_index)
        {
            memset(&options, 0, sizeof(options));
            options.fontSize      = fontSize;
            options.animationTime = editor->animationTime;
            if ((NULL != bounds) && (total < boundsCapacity))
            {
                options.bounds         = &bounds[total];
                options.boundsCapacity = (boundsCapacity - total);
            }
            Arinc661_Scene_DrawLayer(&document->definition, layer_index, drawList, &options);
            total += options.boundsCount;
            has_layers = TRUE;
        }
    }
    if (NULL != boundsCount)
    {
        (*boundsCount) = total;
    }

    return has_layers;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reports once per Save As.
 */
bool_t Arinc661_Definition_TakeRename(Arinc661_Definition_Editor_t* editor, const char_t** previousName, const char_t** currentName)
{
    bool_t has_rename;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != previousName);
    LOGASSERT(NULL != currentName);
    has_rename      = ('\0' != editor->renamedFrom[0]);
    (*previousName) = editor->renamedFrom;
    (*currentName)  = editor->renamedTo;

    return has_rename;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Wraps the document's tree in its own id scope; a selection change makes the document
 *  active so the inspector follows.
 */
bool_t Arinc661_Definition_DrawDocumentHierarchy(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    Arinc661_Definition_Document_t* document;
    int32_t                         kind_before;
    uint32_t                        index_before;
    bool_t                          has_changed;

    LOGASSERT(NULL != editor);
    has_changed = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document     = &editor->documents[documentIndex];
        kind_before  = document->selectedKind;
        index_before = document->selectedIndex;
        ImGui::PushID(document);
        DrawHierarchy(document);
        ImGui::PopID();
        has_changed = !!((kind_before != document->selectedKind) || (index_before != document->selectedIndex));
        if (has_changed)
        {
            Arinc661_Definition_SelectDocument(editor, documentIndex);
        }
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  File, then layer, then widget with its type.
 */
void Arinc661_Definition_SelectionText(const Arinc661_Definition_Editor_t* editor, char_t* text, uint32_t capacity)
{
    const Arinc661_Definition_Document_t* document;
    const Arinc661_Widget_t*              widget;
    uint32_t                              layer_index;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != text);
    text[0] = '\0';
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            widget      = &document->widgets[document->selectedIndex];
            layer_index = widget->layerIndex;
            ImFormatString(text, capacity, "%s > layer %s %s > %s (%s)", document->documentName, LayerIdText(document, layer_index), document->layers[layer_index].name, widget->name, widget->type);
        }
        else if ((SELECTION_LAYER == document->selectedKind) && (document->selectedIndex < document->definition.layerCount))
        {
            ImFormatString(text, capacity, "%s > layer %s %s", document->documentName, LayerIdText(document, document->selectedIndex), document->layers[document->selectedIndex].name);
        }
        else
        {
            ImFormatString(text, capacity, "%s", document->documentName);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the layer arrays and the Width and Height properties.
 */
bool_t Arinc661_Definition_LayerAt(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t layerIndex, const char_t** layerId, const char_t** name, float32_t* width, float32_t* height)
{
    const Arinc661_Definition_Document_t* document;
    bool_t                                is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != layerId);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != width);
    LOGASSERT(NULL != height);
    is_found = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (layerIndex < editor->documents[documentIndex].definition.layerCount))
    {
        document   = &editor->documents[documentIndex];
        (*layerId) = LayerIdText(document, layerIndex);
        (*name)    = document->layers[layerIndex].name;
        Arinc661_Scene_LayerSize(&document->definition, layerIndex, width, height);
        is_found = TRUE;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks every widget property that is a reference, every widget for its parent rule and
 *  WidgetIdent uniqueness, and every layer for LayerId uniqueness, in that order, and
 *  reports the index-th finding.
 */
bool_t Arinc661_Definition_ProblemAt(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t index, char_t* text, uint32_t capacity, uint32_t* widgetIndex, uint32_t* layerIndex)
{
    const Arinc661_Definition_Document_t* document;
    const Arinc661_Property_t*            property;
    const Arinc661_Widget_t*              widget;
    const char_t*                         parent_type;
    char_t                                label[REFERENCE_TEXT_BYTES];
    float32_t                             number;
    float32_t                             other_number;
    uint32_t                              property_index;
    uint32_t                              widget_slot;
    uint32_t                              other_slot;
    uint32_t                              layer_slot;
    uint32_t                              target_document;
    uint32_t                              target_index;
    uint32_t                              seen;
    ReferenceKind_t                       kind;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != text);
    LOGASSERT(NULL != widgetIndex);
    LOGASSERT(NULL != layerIndex);
    text[0]        = '\0';
    (*widgetIndex) = INVALID_HIERARCHY_INDEX;
    (*layerIndex)  = INVALID_HIERARCHY_INDEX;
    seen           = 0;
    if ((documentIndex >= ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) || (FALSE == editor->documents[documentIndex].isLoaded))
    {
        return FALSE;
    }
    document = &editor->documents[documentIndex];
    // Dangling references
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        property = &document->properties[property_index];
        if ((ARINC661_PROPERTY_WIDGET != property->owner) || (property->ownerIndex >= document->definition.widgetCount) || document->widgets[property->ownerIndex].isExtension)
        {
            continue;
        }
        kind = ReferenceKindOf(property->name);
        if ((REFERENCE_NONE != kind) && (FALSE == ResolveReference(document, property->ownerIndex, kind, property->value, &target_document, &target_index, label, sizeof(label))))
        {
            if (seen == index)
            {
                widget = &document->widgets[property->ownerIndex];
                ImFormatString(text, capacity, "%s: %s (%s) %s = %s names nothing.", document->documentName, widget->name, widget->type, property->name, property->value);
                (*widgetIndex) = property->ownerIndex;
                return TRUE;
            }
            seen += 1;
        }
    }
    // Parent rules and duplicate WidgetIdent within a layer; extensions against Table 8.4
    for (widget_slot = 0; widget_slot < document->definition.widgetCount; ++widget_slot)
    {
        widget = &document->widgets[widget_slot];
        if (widget->isExtension)
        {
            if ((INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex) && (FALSE == Arinc661_Definition_ExtensionApplies(widget->type, document->widgets[widget->parentWidgetIndex].type)))
            {
                if (seen == index)
                {
                    ImFormatString(text, capacity, "%s: extension %s is attached to %s (%s), which Table 8.4 does not allow.", document->documentName, widget->type, document->widgets[widget->parentWidgetIndex].name, document->widgets[widget->parentWidgetIndex].type);
                    (*widgetIndex) = widget_slot;
                    return TRUE;
                }
                seen += 1;
            }
            continue;
        }
        parent_type = NULL;
        if (INVALID_HIERARCHY_INDEX != widget->parentWidgetIndex)
        {
            parent_type = document->widgets[widget->parentWidgetIndex].type;
        }
        if (FALSE == IsParentAllowed(widget->type, parent_type))
        {
            if (seen == index)
            {
                if (NULL == parent_type)
                {
                    ImFormatString(text, capacity, "%s: %s (%s) may not be a direct child of a layer (Part 1-7, 3.2.3.1).", document->documentName, widget->name, widget->type);
                }
                else
                {
                    ImFormatString(text, capacity, "%s: %s (%s) may not be a child of %s (%s) (Part 1-7, 3.2.3.1).", document->documentName, widget->name, widget->type, document->widgets[widget->parentWidgetIndex].name, parent_type);
                }
                (*widgetIndex) = widget_slot;
                return TRUE;
            }
            seen += 1;
        }
        if (GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widget_slot, PROPERTY_WIDGET_IDENT, &number))
        {
            for (other_slot = widget_slot + 1; other_slot < document->definition.widgetCount; ++other_slot)
            {
                if ((FALSE == document->widgets[other_slot].isExtension) && (document->widgets[other_slot].layerIndex == widget->layerIndex) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, other_slot, PROPERTY_WIDGET_IDENT, &other_number) && (other_number == number))
                {
                    if (seen == index)
                    {
                        ImFormatString(text, capacity, "%s: %s and %s share WidgetIdent %lld in layer %s.", document->documentName, widget->name, document->widgets[other_slot].name, (long long)number, LayerIdText(document, widget->layerIndex));
                        (*widgetIndex) = other_slot;
                        return TRUE;
                    }
                    seen += 1;
                }
            }
        }
    }
    // Duplicate LayerId
    for (layer_slot = 0; layer_slot < document->definition.layerCount; ++layer_slot)
    {
        if (GetPropertyNumber(document, ARINC661_PROPERTY_LAYER, layer_slot, PROPERTY_LAYER_ID, &number))
        {
            for (other_slot = layer_slot + 1; other_slot < document->definition.layerCount; ++other_slot)
            {
                if (GetPropertyNumber(document, ARINC661_PROPERTY_LAYER, other_slot, PROPERTY_LAYER_ID, &other_number) && (other_number == number))
                {
                    if (seen == index)
                    {
                        ImFormatString(text, capacity, "%s: layers %s and %s share LayerId %lld.", document->documentName, document->layers[layer_slot].name, document->layers[other_slot].name, (long long)number);
                        (*layerIndex) = other_slot;
                        return TRUE;
                    }
                    seen += 1;
                }
            }
        }
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts widgets of the two map source types across the document.
 */
bool_t Arinc661_Definition_HasMapSource(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex)
{
    const Arinc661_Definition_Document_t* document;
    uint32_t                              widget_index;
    bool_t                                has_source;

    LOGASSERT(NULL != editor);
    has_source = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document = &editor->documents[documentIndex];
        for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
        {
            if ((0 == strncmp("A661_MAPHORZ_SOURCE", document->widgets[widget_index].type, sizeof("A661_MAPHORZ_SOURCE"))) || (0 == strncmp("A661_MAPVERT_SOURCE", document->widgets[widget_index].type, sizeof("A661_MAPVERT_SOURCE"))))
            {
                has_source = TRUE;
                break;
            }
        }
    }

    return has_source;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the index-th layer's LayerId numerically.
 */
bool_t Arinc661_Definition_LayerIdAt(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t layerIndex, int64_t* layerId)
{
    float32_t number;
    bool_t    is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != layerId);
    is_found = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (layerIndex < editor->documents[documentIndex].definition.layerCount))
    {
        is_found = GetPropertyNumber(&editor->documents[documentIndex], ARINC661_PROPERTY_LAYER, layerIndex, PROPERTY_LAYER_ID, &number);
        if (is_found)
        {
            (*layerId) = (int64_t)number;
        }
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the document's selection.
 */
bool_t Arinc661_Definition_SelectedWidgetIndex(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t* widgetIndex)
{
    bool_t is_selected;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != widgetIndex);
    is_selected = FALSE;
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (SELECTION_WIDGET == editor->documents[documentIndex].selectedKind) && (editor->documents[documentIndex].selectedIndex < editor->documents[documentIndex].definition.widgetCount))
    {
        (*widgetIndex) = editor->documents[documentIndex].selectedIndex;
        is_selected    = TRUE;
    }

    return is_selected;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The note is shown after the editor's own status text until replaced.
 */
void Arinc661_Definition_SetStatusNote(Arinc661_Definition_Editor_t* editor, const char_t* text)
{
    LOGASSERT(NULL != editor);
    editor->statusNote[0] = '\0';
    if (NULL != text)
    {
        ImStrncpy(editor->statusNote, text, sizeof(editor->statusNote));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same placement rule as Arinc661_Definition_AddWidget, with the type set.
 */
void Arinc661_Definition_AddWidgetOfType(Arinc661_Definition_Editor_t* editor, const char_t* type)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        layer_index;
    uint32_t                        parent_index;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != type);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if (0 == document->definition.layerCount)
        {
            AddLayer(document);
        }
        layer_index  = 0;
        parent_index = INVALID_HIERARCHY_INDEX;
        if ((SELECTION_LAYER == document->selectedKind) && (document->selectedIndex < document->definition.layerCount))
        {
            layer_index = document->selectedIndex;
        }
        else if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            layer_index  = document->widgets[document->selectedIndex].layerIndex;
            parent_index = document->selectedIndex;
            if (document->widgets[parent_index].isExtension)
            {
                parent_index = document->widgets[parent_index].parentWidgetIndex;
            }
        }
        if (0 < document->definition.layerCount)
        {
            AddTypedWidget(document, layer_index, parent_index, type, 0.0f, 0.0f, FALSE);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attaches to the selected widget, or to the selected extension's widget.
 */
void Arinc661_Definition_AddExtensionOfType(Arinc661_Definition_Editor_t* editor, const char_t* type)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        parent_index;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != type);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            parent_index = document->selectedIndex;
            if (document->widgets[parent_index].isExtension)
            {
                parent_index = document->widgets[parent_index].parentWidgetIndex;
            }
            AddTypedWidget(document, document->widgets[parent_index].layerIndex, parent_index, type, 0.0f, 0.0f, FALSE);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Selected widgets whose ancestors are not selected are the roots; each root's subtree,
 *  extensions included, is copied with parent indices relative to the clipboard.
 */
uint32_t Arinc661_Definition_Copy(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        mapping[ARINC661_DEFINITION_MAX_WIDGETS];
    uint32_t                        widget_index;
    uint32_t                        root;
    uint32_t                        end;
    uint32_t                        source;
    uint32_t                        property_index;
    uint32_t                        ancestor;
    uint32_t                        depth;
    bool_t                          has_selected_ancestor;

    LOGASSERT(NULL != editor);
    editor->clipboardWidgetCount   = 0;
    editor->clipboardPropertyCount = 0;
    if ((INVALID_DOCUMENT_INDEX == editor->activeDocumentIndex) || (FALSE == editor->documents[editor->activeDocumentIndex].isLoaded))
    {
        return 0;
    }
    document = &editor->documents[editor->activeDocumentIndex];
    for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
    {
        mapping[widget_index] = INVALID_HIERARCHY_INDEX;
    }
    for (root = 0; root < document->definition.widgetCount; ++root)
    {
        if (FALSE == document->isSelected[root])
        {
            continue;
        }
        has_selected_ancestor = FALSE;
        ancestor              = document->widgets[root].parentWidgetIndex;
        for (depth = 0; (INVALID_HIERARCHY_INDEX != ancestor) && (depth < document->definition.widgetCount); ++depth)
        {
            if (document->isSelected[ancestor])
            {
                has_selected_ancestor = TRUE;
            }
            ancestor = document->widgets[ancestor].parentWidgetIndex;
        }
        if (has_selected_ancestor)
        {
            continue;
        }
        end = SubtreeEnd(document, root);
        for (source = root; source < end; ++source)
        {
            if (editor->clipboardWidgetCount >= ARINC661_DEFINITION_CLIPBOARD_WIDGETS)
            {
                break;
            }
            editor->clipboardWidgets[editor->clipboardWidgetCount] = document->widgets[source];
            mapping[source]                                         = editor->clipboardWidgetCount;
            if (source == root)
            {
                editor->clipboardWidgets[editor->clipboardWidgetCount].parentWidgetIndex = INVALID_HIERARCHY_INDEX;
            }
            else
            {
                editor->clipboardWidgets[editor->clipboardWidgetCount].parentWidgetIndex = mapping[document->widgets[source].parentWidgetIndex];
            }
            for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
            {
                if ((ARINC661_PROPERTY_WIDGET == document->properties[property_index].owner) && (source == document->properties[property_index].ownerIndex) && (editor->clipboardPropertyCount < ARINC661_DEFINITION_CLIPBOARD_PROPERTIES))
                {
                    editor->clipboardProperties[editor->clipboardPropertyCount]            = document->properties[property_index];
                    editor->clipboardProperties[editor->clipboardPropertyCount].ownerIndex = editor->clipboardWidgetCount;
                    editor->clipboardPropertyCount += 1;
                }
            }
            editor->clipboardWidgetCount += 1;
        }
    }
    if (0 < editor->clipboardWidgetCount)
    {
        ImFormatString(editor->statusNote, sizeof(editor->statusNote), STATUS_COPIED_FORMAT, (unsigned)editor->clipboardWidgetCount);
    }

    return editor->clipboardWidgetCount;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A selected container receives the copies; a selected leaf gets them as siblings; a
 *  selected layer gets them as direct children.
 */
uint32_t Arinc661_Definition_Paste(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        layer_index;
    uint32_t                        parent_index;
    uint32_t                        pasted;

    LOGASSERT(NULL != editor);
    pasted = 0;
    if ((0 < editor->clipboardWidgetCount) && (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if (0 == document->definition.layerCount)
        {
            AddLayer(document);
        }
        layer_index  = 0;
        parent_index = INVALID_HIERARCHY_INDEX;
        if ((SELECTION_LAYER == document->selectedKind) && (document->selectedIndex < document->definition.layerCount))
        {
            layer_index = document->selectedIndex;
        }
        else if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount))
        {
            layer_index  = document->widgets[document->selectedIndex].layerIndex;
            parent_index = document->selectedIndex;
            if (document->widgets[parent_index].isExtension || (FALSE == IsContainerType(document->widgets[parent_index].type)))
            {
                parent_index = document->widgets[parent_index].parentWidgetIndex;
            }
        }
        if (0 < document->definition.layerCount)
        {
            pasted = InsertClipboard(document, layer_index, parent_index);
        }
    }

    return pasted;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The hierarchy shows the rename field on its next draw.
 */
void Arinc661_Definition_BeginRename(Arinc661_Definition_Editor_t* editor)
{
    Arinc661_Definition_Document_t* document;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if ((SELECTION_WIDGET == document->selectedKind) && (document->selectedIndex < document->definition.widgetCount) && (FALSE == document->widgets[document->selectedIndex].isExtension))
        {
            editor->renamingWidget = document->selectedIndex;
            ImStrncpy(editor->renameBuffer, document->widgets[document->selectedIndex].name, sizeof(editor->renameBuffer));
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Negative stops the animations and shows the rest state.
 */
void Arinc661_Definition_SetAnimationTime(Arinc661_Definition_Editor_t* editor, float32_t seconds)
{
    LOGASSERT(NULL != editor);
    editor->animationTime = seconds;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks the widgets of the active document for the three transform animations and the
 *  parameter animation, reporting Delay and Duration in seconds.
 */
bool_t Arinc661_Definition_AnimationAt(const Arinc661_Definition_Editor_t* editor, uint32_t index, const char_t** name, const char_t** type, float32_t* delaySeconds, float32_t* durationSeconds)
{
    const Arinc661_Definition_Document_t* document;
    float32_t                             number;
    uint32_t                              widget_index;
    uint32_t                              seen;
    bool_t                                is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != type);
    LOGASSERT(NULL != delaySeconds);
    LOGASSERT(NULL != durationSeconds);
    is_found = FALSE;
    seen     = 0;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        for (widget_index = 0; widget_index < document->definition.widgetCount; ++widget_index)
        {
            if (0 != strncmp("A661_ANIMATION_", document->widgets[widget_index].type, 15) || (0 == strncmp("A661_ANIMATION_GROUP", document->widgets[widget_index].type, sizeof("A661_ANIMATION_GROUP"))))
            {
                continue;
            }
            if (seen == index)
            {
                (*name) = document->widgets[widget_index].name;
                (*type) = document->widgets[widget_index].type;
                number  = 0.0f;
                (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widget_index, "Delay", &number);
                (*delaySeconds) = (number / 1000.0f);
                number = 0.0f;
                (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widget_index, "Duration", &number);
                (*durationSeconds) = (number / 1000.0f);
                is_found = TRUE;
                break;
            }
            seen += 1;
        }
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Play mode snapshots every open document so interaction can change states freely; stopping
 *  restores them, as a game editor discards play-mode edits.
 */
void Arinc661_Definition_BeginPlay(Arinc661_Definition_Editor_t* editor)
{
    uint32_t document_index;

    LOGASSERT(NULL != editor);
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        free(editor->playSnapshots[document_index]);
        editor->playSnapshots[document_index] = NULL;
        if (editor->documents[document_index].isLoaded)
        {
            editor->playSnapshots[document_index] = TakeSnapshot(&editor->documents[document_index]);
            editor->playWasDirty[document_index]  = editor->documents[document_index].isDirty;
        }
    }
    editor->isPlaying = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Restores the snapshots and keeps the dirty flags as they were before play.
 */
void Arinc661_Definition_EndPlay(Arinc661_Definition_Editor_t* editor)
{
    uint32_t document_index;

    LOGASSERT(NULL != editor);
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if ((NULL != editor->playSnapshots[document_index]) && editor->documents[document_index].isLoaded)
        {
            RestoreSnapshot(&editor->documents[document_index], editor->playSnapshots[document_index]);
            editor->documents[document_index].isDirty = editor->playWasDirty[document_index];
        }
        free(editor->playSnapshots[document_index]);
        editor->playSnapshots[document_index] = NULL;
    }
    editor->isPlaying     = FALSE;
    editor->animationTime = -1.0f;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The interactions a crew member could make on a CDS that change a Creation Structure
 *  value: toggle and check buttons flip, multi-state buttons advance, combo boxes, selection
 *  lists, and scroll lists step their entry, tabbed panel groups activate the clicked tab,
 *  sliders accept the clicked fraction, and mutually exclusive containers show the next child.
 */
bool_t Arinc661_Definition_Interact(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t widgetIndex, float32_t fractionX, float32_t fractionY)
{
    Arinc661_Definition_Document_t* document;
    const char_t*                   type;
    uint32_t                        property_index;
    uint32_t                        child_index;
    uint32_t                        first_child;
    float32_t                       number;
    float32_t                       count;
    float32_t                       minimum;
    float32_t                       maximum;
    float32_t                       ident;
    bool_t                          has_changed;
    bool_t                          takes_next;

    LOGASSERT(NULL != editor);
    has_changed = FALSE;
    if ((documentIndex >= ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) || (FALSE == editor->documents[documentIndex].isLoaded) || (widgetIndex >= editor->documents[documentIndex].definition.widgetCount))
    {
        return FALSE;
    }
    document = &editor->documents[documentIndex];
    type     = document->widgets[widgetIndex].type;
    if ((0 == strncmp(type, "A661_TOGGLE_BUTTON", sizeof("A661_TOGGLE_BUTTON"))) || (0 == strncmp(type, "A661_PICTURE_TOGGLE_BUTTON", sizeof("A661_PICTURE_TOGGLE_BUTTON"))) || (0 == strncmp(type, "A661_SYMBOL_TOGGLE_BUTTON", sizeof("A661_SYMBOL_TOGGLE_BUTTON"))))
    {
        property_index = FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "ToggleState");
        if (INVALID_HIERARCHY_INDEX != property_index)
        {
            if (0 == strncmp(document->properties[property_index].value, "A661_SELECTED", sizeof("A661_SELECTED")))
            {
                ImStrncpy(document->properties[property_index].value, "A661_UNSELECTED", sizeof(document->properties[property_index].value));
            }
            else
            {
                ImStrncpy(document->properties[property_index].value, "A661_SELECTED", sizeof(document->properties[property_index].value));
            }
            has_changed = TRUE;
        }
    }
    else if (0 == strncmp(type, "A661_CHECK_BUTTON", sizeof("A661_CHECK_BUTTON")))
    {
        property_index = FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "CheckButtonState");
        if (INVALID_HIERARCHY_INDEX != property_index)
        {
            if (0 == strncmp(document->properties[property_index].value, "A661_SELECTED", sizeof("A661_SELECTED")))
            {
                ImStrncpy(document->properties[property_index].value, "A661_UNSELECTED", sizeof(document->properties[property_index].value));
            }
            else
            {
                ImStrncpy(document->properties[property_index].value, "A661_SELECTED", sizeof(document->properties[property_index].value));
            }
            has_changed = TRUE;
        }
    }
    else if (0 == strncmp(type, "A661_MULTI_STATE_BUTTON", sizeof("A661_MULTI_STATE_BUTTON")))
    {
        number = 1.0f;
        count  = 1.0f;
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "ButtonState", &number);
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "NumberOfStates", &count);
        number += 1.0f;
        if (number > count)
        {
            number = 1.0f;
        }
        SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "ButtonState", number);
        has_changed = TRUE;
    }
    else if ((0 == strncmp(type, "A661_COMBO_BOX", sizeof("A661_COMBO_BOX"))) || (0 == strncmp(type, "A661_COMBO_BOX_EDIT", sizeof("A661_COMBO_BOX_EDIT"))) || (0 == strncmp(type, "A661_SELECTION_LIST_BUTTON", sizeof("A661_SELECTION_LIST_BUTTON"))) || (0 == strncmp(type, "A661_SCROLL_LIST", sizeof("A661_SCROLL_LIST"))) || (0 == strncmp(type, "A661_TREE", sizeof("A661_TREE"))))
    {
        number = 0.0f;
        count  = 0.0f;
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "SelectedEntry", &number);
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "NumberOfEntries", &count);
        number += 1.0f;
        if (number > count)
        {
            number = 0.0f;
        }
        SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "SelectedEntry", number);
        has_changed = TRUE;
    }
    else if (0 == strncmp(type, "A661_SLIDER", sizeof("A661_SLIDER")))
    {
        minimum = 0.0f;
        maximum = 100.0f;
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "MinValue", &minimum);
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "MaxValue", &maximum);
        number = fractionX;
        if ((INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "Orientation")) && (0 == strncmp(document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "Orientation")].value, "A661_BOTTOM", 11)))
        {
            number = fractionY;
        }
        SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "Value", minimum + ((maximum - minimum) * ImClamp(number, 0.0f, 1.0f)));
        has_changed = TRUE;
    }
    else if ((0 == strncmp(type, "A661_MUTUALLY_EXCLUSIVE_CONTAINER", sizeof("A661_MUTUALLY_EXCLUSIVE_CONTAINER"))) || (0 == strncmp(type, "A661_PAGING_CONTAINER", sizeof("A661_PAGING_CONTAINER"))) || (0 == strncmp(type, "A661_TABBED_PANEL_GROUP", sizeof("A661_TABBED_PANEL_GROUP"))))
    {
        // Advance to the next child's ident, wrapping to the first
        number = 0.0f;
        (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "VisibleChild", &number);
        if (0 == strncmp(type, "A661_TABBED_PANEL_GROUP", sizeof("A661_TABBED_PANEL_GROUP")))
        {
            (void)GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "ActiveTabbedPanelID", &number);
        }
        first_child = INVALID_HIERARCHY_INDEX;
        takes_next  = FALSE;
        count       = 0.0f;
        for (child_index = widgetIndex + 1; child_index < document->definition.widgetCount; ++child_index)
        {
            if ((document->widgets[child_index].parentWidgetIndex == widgetIndex) && (FALSE == document->widgets[child_index].isExtension) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, child_index, PROPERTY_WIDGET_IDENT, &ident))
            {
                if (INVALID_HIERARCHY_INDEX == first_child)
                {
                    first_child = child_index;
                }
                if (takes_next)
                {
                    count      = ident;
                    takes_next = FALSE;
                    break;
                }
                if (ident == number)
                {
                    takes_next = TRUE;
                }
            }
        }
        if ((0.0f == count) && (INVALID_HIERARCHY_INDEX != first_child) && GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, first_child, PROPERTY_WIDGET_IDENT, &ident))
        {
            count = ident;
        }
        if (0 == strncmp(type, "A661_TABBED_PANEL_GROUP", sizeof("A661_TABBED_PANEL_GROUP")))
        {
            SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "ActiveTabbedPanelID", count);
        }
        else
        {
            SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, widgetIndex, "VisibleChild", count);
        }
        has_changed = TRUE;
    }
    if (has_changed)
    {
        document->isDirty = TRUE;
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The binary twin takes the document's name with the .bin extension beside the XML, or the
 *  document path itself when it already is binary.
 */
bool_t Arinc661_Definition_ExportBinary(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, char_t* path, uint32_t capacity)
{
    Arinc661_Definition_Document_t* document;
    uint64_t                        byte_count;
    size_t                          length;
    bool_t                          is_written;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != path);
    is_written = FALSE;
    path[0]    = '\0';
    if ((documentIndex < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document = &editor->documents[documentIndex];
        if ('\0' != document->documentPath[0])
        {
            ImStrncpy(path, document->documentPath, (int)capacity);
        }
        else
        {
            JoinFolderPath(editor->folderPath, document->documentName, path, capacity);
        }
        length = strlen(path);
        if (FALSE == HasExtension(path, BINARY_EXTENSION))
        {
            while ((0 < length) && ('.' != path[length - 1]) && ('\\' != path[length - 1]))
            {
                length -= 1;
            }
            if ((0 < length) && ('.' == path[length - 1]))
            {
                path[length - 1] = '\0';
            }
            ImStrncpy(&path[strlen(path)], BINARY_EXTENSION, (int)(capacity - strlen(path)));
        }
        byte_count = Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer));
        if (0 < byte_count)
        {
            is_written = (0 <= OS_File_WriteFile(path, document->outputBuffer, (uint32_t)byte_count));
        }
    }

    return is_written;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Zero when no document is active.
 */
uint32_t Arinc661_Definition_WidgetCount(const Arinc661_Definition_Editor_t* editor)
{
    uint32_t count;

    LOGASSERT(NULL != editor);
    count = 0;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        count = editor->documents[editor->activeDocumentIndex].definition.widgetCount;
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The clipboard is written as a one-layer Definition File named after the prefab, so a prefab
 *  is readable by any ARINC 661 tool and by this editor's reader.
 */
bool_t Arinc661_Definition_SavePrefab(Arinc661_Definition_Editor_t* editor, const char_t* name, char_t* path, uint32_t capacity)
{
    Arinc661_Definition_Document_t* document;
    Arinc661_Definition_t           prefab;
    Arinc661_Layer_t                layer;
    char_t                          file_name[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];
    uint64_t                        byte_count;
    uint32_t                        widget_index;
    bool_t                          is_written;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != path);
    is_written = FALSE;
    path[0]    = '\0';
    if ((0 == editor->clipboardWidgetCount) || (INVALID_DOCUMENT_INDEX == editor->activeDocumentIndex) || ('\0' == name[0]) || (NULL != strpbrk(name, "\\/:*?\"<>|")))
    {
        return FALSE;
    }
    document = &editor->documents[editor->activeDocumentIndex];
    Arinc661_Definition_Initialize(&prefab, &layer, 1, editor->clipboardWidgets, ARINC661_DEFINITION_CLIPBOARD_WIDGETS, editor->clipboardProperties, ARINC661_DEFINITION_CLIPBOARD_PROPERTIES, NULL, 0);
    (void)memset(&layer, 0, sizeof(layer));
    ImStrncpy(prefab.name, name, sizeof(prefab.name));
    ImStrncpy(layer.name, PREFAB_LAYER_NAME, sizeof(layer.name));
    prefab.supplementVersion = document->definition.supplementVersion;
    prefab.layerCount        = 1;
    prefab.widgetCount       = editor->clipboardWidgetCount;
    prefab.propertyCount     = editor->clipboardPropertyCount;
    for (widget_index = 0; widget_index < editor->clipboardWidgetCount; ++widget_index)
    {
        editor->clipboardWidgets[widget_index].layerIndex = 0;
    }
    byte_count = Arinc661_Definition_Write(&prefab, document->outputBuffer, sizeof(document->outputBuffer), document->workspace, sizeof(document->workspace));
    if (0 < byte_count)
    {
        ImFormatString(file_name, sizeof(file_name), "%s%s", name, PREFAB_EXTENSION);
        JoinFolderPath(editor->folderPath, file_name, path, capacity);
        is_written = (0 <= OS_File_WriteFile(path, document->outputBuffer, (uint32_t)byte_count));
    }

    return is_written;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The prefab's widgets become the clipboard and are pasted at the selection as a copy would be.
 */
bool_t Arinc661_Definition_LoadPrefab(Arinc661_Definition_Editor_t* editor, const char_t* fileName)
{
    Arinc661_Definition_Document_t* document;
    Arinc661_Definition_t           prefab;
    Arinc661_Layer_t                layer;
    char_t                          path[ARINC661_DEFINITION_MAX_FILE_PATH_BYTES];
    int32_t                         byte_count;
    bool_t                          is_loaded;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != fileName);
    is_loaded = FALSE;
    if ((INVALID_DOCUMENT_INDEX == editor->activeDocumentIndex) || (FALSE == editor->documents[editor->activeDocumentIndex].isLoaded))
    {
        return FALSE;
    }
    document = &editor->documents[editor->activeDocumentIndex];
    JoinFolderPath(editor->folderPath, fileName, path, sizeof(path));
    byte_count = OS_File_ReadFile(path, document->inputBuffer, sizeof(document->inputBuffer));
    if (0 < byte_count)
    {
        Arinc661_Definition_Initialize(&prefab, &layer, 1, editor->clipboardWidgets, ARINC661_DEFINITION_CLIPBOARD_WIDGETS, editor->clipboardProperties, ARINC661_DEFINITION_CLIPBOARD_PROPERTIES, NULL, 0);
        (void)memset(&layer, 0, sizeof(layer));
        is_loaded = Arinc661_Definition_Read(&prefab, document->inputBuffer, (uint64_t)byte_count, document->workspace, sizeof(document->workspace));
        if (is_loaded)
        {
            editor->clipboardWidgetCount   = prefab.widgetCount;
            editor->clipboardPropertyCount = prefab.propertyCount;
            is_loaded = (0 < Arinc661_Definition_Paste(editor));
        }
        else
        {
            editor->clipboardWidgetCount   = 0;
            editor->clipboardPropertyCount = 0;
        }
    }

    return is_loaded;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Index zero is the oldest undo step; the current state follows the undo steps, then the redo
 *  steps newest first as the list reads downward.
 */
uint32_t Arinc661_Definition_HistoryCount(const Arinc661_Definition_Editor_t* editor, uint32_t* currentIndex)
{
    const Arinc661_Definition_Document_t* document;
    uint32_t                              count;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != currentIndex);
    count           = 0;
    (*currentIndex) = 0;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document        = &editor->documents[editor->activeDocumentIndex];
        (*currentIndex) = document->undoCount;
        count           = (document->undoCount + document->redoCount);
        if (NULL != document->shadow)
        {
            count += 1;
        }
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Undo steps come from the undo stack in order, the current state from the shadow, and redo
 *  steps from the top of the redo stack downward.
 */
const char_t* Arinc661_Definition_HistoryLabel(const Arinc661_Definition_Editor_t* editor, uint32_t index)
{
    const Arinc661_Definition_Document_t* document;
    const Snapshot_t*                     snapshot;

    LOGASSERT(NULL != editor);
    snapshot = NULL;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if (index < document->undoCount)
        {
            snapshot = (const Snapshot_t*)document->undoStack[index];
        }
        else if ((index == document->undoCount) && (NULL != document->shadow))
        {
            snapshot = (const Snapshot_t*)document->shadow;
        }
        else if ((index > document->undoCount) && ((index - document->undoCount - 1) < document->redoCount))
        {
            snapshot = (const Snapshot_t*)document->redoStack[document->redoCount - 1 - (index - document->undoCount - 1)];
        }
    }
    if (NULL == snapshot)
    {
        return "";
    }

    return snapshot->label;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Undoes or redoes until the current index equals the one asked for.
 */
void Arinc661_Definition_GoToHistory(Arinc661_Definition_Editor_t* editor, uint32_t index)
{
    Arinc661_Definition_Document_t* document;
    uint32_t                        guard;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        for (guard = 0; (index < document->undoCount) && (guard < ARINC661_DEFINITION_MAX_HISTORY); ++guard)
        {
            UndoDocument(document);
        }
        for (guard = 0; (index > document->undoCount) && (0 < document->redoCount) && (guard < ARINC661_DEFINITION_MAX_HISTORY); ++guard)
        {
            RedoDocument(document);
        }
    }
}
