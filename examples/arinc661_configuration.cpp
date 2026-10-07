/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * FACE Technical Standard, Appendix H.3, DisplayManagement.xsd:
 * https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "arinc661_context.h"
#include "arinc661_definition_editor.h"
#include "arinc661_gfx_imgui.h"
#include "arinc661_icons.h"
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_file.h"
#include "faceoe_utl/faceoe_utl_defines.h"
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

#define WINDOW_NAME              ("ARINC 661 Configuration")
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
#define XML_FILES_LABEL          ("XML Files")
#define XML_EXTENSION            (".xml")
#define XML_LIST_MAX_ROWS        (4)
#define FOLDER_INPUT_ID          ("##configurationFolderPath")
#define XML_LIST_ID              ("##xmlFiles")
#define DOCUMENT_TAB_BAR_ID      ("##configurationDocuments")
#define DOCUMENT_TAB_FORMAT      ("%s###%u")
#define NEW_DOCUMENT_TAB_LABEL   ("+")
#define NEW_DOCUMENT_TAB_TOOLTIP ("New document")
#define UNTITLED_DOCUMENT_FORMAT ("Untitled %u.xml")
#define SAVE_AS_POPUP_ID         ("Save XML File")
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
#define PANE_TABLE_ID            ("ConfigurationPanes")
#define SCENE_CANVAS_ID          ("Screen canvas")
#define EMPTY_SCENE_LABEL        ("Add a screen to preview the configuration.")
#define PROPERTY_INPUT_ID        ("##value")
#define PROPERTY_COLUMN          ("Property")
#define VALUE_COLUMN             ("Value")

#define HIERARCHY_PANE_COLUMN_IDX (0)
#define SCENE_PANE_COLUMN_IDX     (1)
#define INSPECTOR_PANE_COLUMN     (2)
#define PANE_COUNT                (3)
#define INVALID_HIERARCHY_INDEX   (UINT32_MAX)
#define INVALID_DOCUMENT_INDEX    (UINT32_MAX)

#define DEFAULT_FOLDER_PATH ("C:\\dev\\xml\\examples")

#define NO_IDENTIFIER  (-1)
#define MAX_IDENTIFIER (INT64_MAX)

#define WINDOW_FLAGS               (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar)
#define CLOSE_DOCUMENT_POPUP_FLAGS (ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)
#define PANE_TABLE_FLAGS           (ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoHostExtendY)
#define PANE_FLAGS                 (ImGuiChildFlags_AlwaysUseWindowPadding)
#define PROPERTY_TABLE_FLAGS       (ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)
#define NEW_DOCUMENT_TAB_FLAGS     (ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)

#define SAVE_SHORTCUT       (ImGuiMod_Ctrl | ImGuiKey_S)
#define NEW_SHORTCUT        (ImGuiMod_Ctrl | ImGuiKey_N)
#define SAVE_SHORTCUT_FLAGS (ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive)

#define ADD_SCREEN_LABEL                ("+ Add Screen")
#define ADD_LAYOUT_LABEL                ("+ Add Layout")
#define ADD_WINDOW_LABEL                ("+ Add Window")
#define ADD_USER_APPLICATION_LABEL      ("+ Add User Application")
#define REMOVE_SCREEN_LABEL             ("Remove Screen")
#define REMOVE_LAYOUT_LABEL             ("Remove Layout")
#define REMOVE_WINDOW_LABEL             ("Remove Window")
#define REMOVE_USER_APPLICATION_LABEL   ("Remove User Application")
#define READY_LABEL                     ("Ready")
#define BASE_UNIT_LABEL                 ("Base Unit")
#define REMOVE_GLYPH                    ("x")

#define SCREEN_NODE_ID                ("screen")
#define LAYOUT_NODE_ID                ("layout")
#define WINDOW_NODE_ID                ("window")
#define USER_APPLICATION_NODE_ID      ("userApplication")
#define SCREEN_LABEL_FORMAT           ("Screen %" PRId64)
#define LAYOUT_LABEL_FORMAT           ("Layout %" PRId64)
#define WINDOW_LABEL_FORMAT           ("Window %" PRId64 " (%s)")
#define USER_APPLICATION_LABEL_FORMAT ("User Application %" PRId64)

#define SCREEN_PREVIEW_FORMAT       ("Screen %" PRId64 " (%" PRId64 " x %" PRId64 ")")
#define NAMED_WINDOW_PREVIEW_FORMAT ("%s (%" PRId64 " x %" PRId64 ")")
#define WINDOW_PREVIEW_FORMAT       ("Window %" PRId64 " (%" PRId64 " x %" PRId64 ")")

#define SCREEN_TABLE_ID              ("ScreenProperties")
#define PIXEL_SIZE_TABLE_ID          ("PixelSizeProperties")
#define PHYSICAL_DIMENSIONS_TABLE_ID ("PhysicalDimensionProperties")
#define LAYOUT_TABLE_ID              ("LayoutProperties")
#define WINDOW_TABLE_ID              ("WindowProperties")
#define USER_APPLICATION_TABLE_ID    ("UserApplicationProperties")
#define PIXEL_AREA_TABLE_ID          ("PixelAreaProperties")
#define SCALING_FACTOR_TABLE_ID      ("ScalingFactorProperties")

#define SCREEN_SECTION_LABEL              ("Screen")
#define PIXEL_SIZE_SECTION_LABEL          ("Pixel Size")
#define PHYSICAL_DIMENSIONS_SECTION_LABEL ("Physical Dimensions")
#define LAYOUT_SECTION_LABEL              ("Layout")
#define WINDOW_SECTION_LABEL              ("Window")
#define USER_APPLICATION_SECTION_LABEL    ("User Application")
#define EXTERNAL_SOURCE_SECTION_LABEL     ("External Source")
#define EXTERNAL_SOURCE_TABLE_ID          ("ExternalSourceProperties")
#define EXTERNAL_SOURCE_NODE_ID           ("##externalSource")
#define EXTERNAL_SOURCE_LABEL_FORMAT      ("External Source %u (%s)")
#define ADD_EXTERNAL_SOURCE_LABEL         ("+ Add External Source")
#define REMOVE_EXTERNAL_SOURCE_LABEL      ("Remove External Source")
#define EXTERNAL_TYPE_LABEL               ("Type")
#define EXTERNAL_SOURCE_HINT_LABEL        ("Referenced by ExternalSource and MapExternalSource widgets through SourceReference (FACE H.3.7).")
#define ELEMENT_EXTERNAL_SOURCE           ("ExternalSource")
#define BREADCRUMB_EXTERNAL_FORMAT        ("%s > external source %u %s")
#define PIXEL_AREA_SECTION_LABEL          ("Pixel Area")
#define SCALING_FACTOR_SECTION_LABEL      ("Scaling Factor")

#define ID_LABEL               ("ID")
#define NAME_LABEL             ("Name")
#define X_LABEL                ("X")
#define Y_LABEL                ("Y")
#define WIDTH_LABEL            ("Width")
#define HEIGHT_LABEL           ("Height")
#define UNITS_LABEL            ("Units")
#define X_SCALE_LABEL          ("X Scale")
#define Y_SCALE_LABEL          ("Y Scale")
#define PER_UNIT_LABEL         ("Per Unit")
#define WINDOW_ID_LABEL        ("Window ID")
#define DF_PATH_LABEL          ("DF Path")
#define DEFINITION_SECTION_LABEL      ("Definition File")
#define SHOW_DEFINITION_LABEL         ("Show Definition File")
#define DEFINITION_STATE_FORMAT       ("%u layers, %u widgets, ApplicationId %lld")
#define DEFINITION_NO_ID_FORMAT       ("%u layers, %u widgets, no ApplicationId")
#define DEFINITION_CLOSED_LABEL       ("In folder; open it to preview and check its ApplicationId.")
#define DEFINITION_MISSING_LABEL      ("No valid Definition File of this name in the folder.")
#define DEFINITION_EMPTY_LABEL        ("No Definition File chosen.")
#define APPLICATION_ID_MISMATCH_LABEL ("ApplicationId mismatch: the Definition File carries a different identifier (ARINC 661 6.5.1.2).")
#define ADOPT_DEFINITION_ID_LABEL     ("Use the Definition File's ApplicationId")
#define WINDOW_NO_APPLICATION_LABEL   ("No user application targets this window.")
#define WINDOW_APPLICATION_FORMAT     ("Application %lld: %s")
#define SCENE_PIXELS_PER_MM_FALLBACK  (4.0f)
#define SCREEN_CHOICE_FORMAT          ("Screen %" PRId64)
#define LAYOUT_WINDOWS_LABEL          ("Windows In This Layout")
#define LAYOUT_NO_WINDOWS_LABEL       ("No windows yet; add one below.")
#define ADD_WINDOW_HERE_LABEL         ("Add window to this layout")
#define LAYOUT_WINDOW_FORMAT          ("Window %" PRId64 " %s (%" PRId64 " x %" PRId64 " at %" PRId64 ", %" PRId64 ")")
#define LAYOUT_CHOICE_FORMAT          ("Layout %" PRId64 " on screen %" PRId64)
#define LAYOUT_LABEL                  ("Layout")
#define SCREEN_LABEL                  ("Screen")
#define SCREEN_LAYOUTS_LABEL          ("Layouts On This Screen")
#define ADD_LAYOUT_HERE_LABEL         ("Add layout to this screen")
#define LAYOUT_ITEM_FORMAT            ("Layout %" PRId64 " (%u windows)")
#define SCENE_HINT_LABEL              ("Drag: move window\nCorners: resize\nArrows: nudge (Shift x10)\nDel: remove\nRight-click: add\nDouble-click a previewed widget: edit it\nPlay mode: click acts on the widget")
#define SCENE_HINT_TITLE              ("Scene controls")
#define SCENE_HINT_COLOR              (IM_COL32(180, 190, 200, 200))
#define SCENE_HANDLE_COLOR            (IM_COL32(255, 255, 255, 230))
#define SCENE_LABEL_BACK_COLOR        (IM_COL32(0, 0, 0, 90))
#define HANDLE_HALF_SIZE              (5.0f)
#define DRAG_THRESHOLD                (3.0f)
#define DRAG_NONE                     (0)
#define DRAG_MOVE                     (1)
#define DRAG_BOTTOM_LEFT              (2)
#define DRAG_BOTTOM_RIGHT             (3)
#define DRAG_TOP_LEFT                 (4)
#define DRAG_TOP_RIGHT                (5)
#define SCENE_POPUP_ID                ("##scene add")
#define ADD_SCREEN_MENU_LABEL         ("Add screen")
#define ADD_LAYOUT_MENU_LABEL         ("Add layout to this screen")
#define ADD_WINDOW_MENU_LABEL         ("Add window here")
#define ADD_APPLICATION_MENU_LABEL    ("Add user application for this window")
#define CREATE_DEFINITION_LABEL       ("Create Definition File for this application")
#define APPLICATION_TREE_FORMAT       ("User Application %" PRId64 " -> %s")
#define APPLICATION_NO_FILE_FORMAT    ("User Application %" PRId64)
#define DEFINITION_CLOSED_NODE_FORMAT ("%s (not open; click to open)")
#define DEFINITION_MISSING_NODE_FORMAT ("%s (not in folder)")
#define LAYERS_SECTION_LABEL          ("Layers Of The Definition File")
#define LAYER_ROW_FORMAT              ("Layer %s %s: %.0f x %.0f (0.01 mm)")
#define LAYER_FIT_WARNING             ("  exceeds window")
#define BREADCRUMB_SCREEN_FORMAT      ("%s > screen %" PRId64)
#define BREADCRUMB_LAYOUT_FORMAT      ("%s > screen %" PRId64 " > layout %" PRId64)
#define BREADCRUMB_WINDOW_FORMAT      ("%s > screen %" PRId64 " > layout %" PRId64 " > window %" PRId64 " %s")
#define BREADCRUMB_APPLICATION_FORMAT ("%s > application %" PRId64 " > window %" PRId64 " > %s")
#define PREVIEW_HINT_LABEL            ("Double-click a widget in the preview to edit it in the Definition File.")
#define REMOVE_MENU_LABEL             ("Remove")
#define DUPLICATE_MENU_LABEL          ("Duplicate")
#define ELEMENT_SCREEN                ("Screen")
#define ELEMENT_LAYOUT                ("Layout")
#define ELEMENT_WINDOW                ("Window")
#define ELEMENT_USER_APPLICATION      ("UserApplication")
#define TOOL_ADD_SCREEN_LABEL         ("Add screen")
#define TOOL_ADD_LAYOUT_LABEL         ("Add layout to the selected screen")
#define TOOL_ADD_WINDOW_LABEL         ("Add window to the selected layout")
#define TOOL_ADD_APPLICATION_LABEL    ("Add user application for the selected window")
#define TOOL_DUPLICATE_LABEL          ("Duplicate selection")
#define TOOL_DELETE_LABEL             ("Delete selection")
#define EMPTY_SCREEN_BUTTON_LABEL     ("Add the first screen")
#define SHORTCUT_CTRL_D               ("Ctrl+D")
#define SHORTCUT_DEL                  ("Del")
#define STYLE_FILE_PATH_LABEL  ("Style File Path")
#define MAP_SOURCE_LAYER_LABEL ("Map Source Layer")
#define VISIBLE_LABEL          ("Visible")
#define SELECT_ITEM_PROMPT     ("Select a screen, layout, window, or user application.")
#define NO_WINDOW_LABEL        ("(none)")
#define NO_DEFINITION_LABEL    ("(none)")
#define WINDOW_CHOICE_FORMAT   ("Window %" PRId64 " (%s)")
#define CHOICE_LABEL_BYTES     (FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES + LABEL_FORMAT_BYTES)

#define INTEGER_TEXT_BYTES       (32)
#define LABEL_FORMAT_BYTES       (64)
#define HIERARCHY_LABEL_BYTES    (FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES + LABEL_FORMAT_BYTES)
#define DOCUMENT_TAB_LABEL_BYTES (ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES + 16)
#define SAVE_DIALOG_INPUT_WIDTH  (420.0f)

#define DEFAULT_SCREEN_PIXEL_WIDTH         (640)
#define DEFAULT_SCREEN_PIXEL_HEIGHT        (480)
#define DEFAULT_SCREEN_PHYSICAL_WIDTH      (5)
#define DEFAULT_SCREEN_PHYSICAL_HEIGHT     (4)
#define DEFAULT_USER_APPLICATION_WINDOW_ID (0)

#define HIERARCHY_BRANCH_FLAGS (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap)
#define HIERARCHY_LEAF_FLAGS   (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap)
#define HIERARCHY_ADD_FLAGS    (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen)

#define SCENE_CANVAS_COLOR        (IM_COL32(22, 27, 34, 255))
#define SCENE_SCREEN_COLOR        (IM_COL32(42, 50, 64, 255))
#define SCENE_SCREEN_BORDER_COLOR (IM_COL32(220, 230, 245, 255))
#define SCENE_WINDOW_COLOR        (IM_COL32(42, 154, 188, 80))
#define SCENE_WINDOW_BORDER_COLOR (IM_COL32(97, 221, 245, 255))
#define SCENE_TEXT_COLOR          (IM_COL32(240, 248, 255, 255))

#define SCENE_SELECTION_OVERLAY_COLOR (IM_COL32(255, 255, 255, 40))
#define INVALID_VALUE_COLOR           (ImVec4(1.0f, 0.4f, 0.4f, 1.0f))

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
    STATUS_SCREEN_ADDED,
    STATUS_SCREEN_ADD_FAILED,
    STATUS_LAYOUT_ADDED,
    STATUS_LAYOUT_ADD_FAILED,
    STATUS_WINDOW_ADDED,
    STATUS_WINDOW_ADD_FAILED,
    STATUS_USER_APPLICATION_ADDED,
    STATUS_USER_APPLICATION_ADD_FAILED,
    STATUS_SCREEN_REMOVED,
    STATUS_LAYOUT_REMOVED,
    STATUS_WINDOW_REMOVED,
    STATUS_USER_APPLICATION_REMOVED,
    STATUS_EXTERNAL_SOURCE_ADDED,
    STATUS_EXTERNAL_SOURCE_ADD_FAILED,
    STATUS_EXTERNAL_SOURCE_REMOVED,
    STATUS_DUPLICATE_EXTERNAL_SOURCE_ID,
    STATUS_READ_FAILED,
    STATUS_INPUT_TOO_LARGE,
    STATUS_INPUT_EMPTY,
    STATUS_LOADED,
    STATUS_INVALID_XML,
    STATUS_DUPLICATE_SCREEN_ID,
    STATUS_DUPLICATE_LAYOUT_ID,
    STATUS_DUPLICATE_WINDOW_ID,
    STATUS_DUPLICATE_APPLICATION_ID,
    STATUS_TOO_MANY_DOCUMENTS
} Status_t;

/*************************************************************************************************/
/**
 * \brief Kind of entity selected in the configuration hierarchy.
 */
typedef enum SelectionKind_e
{
    SELECTION_NONE,
    SELECTION_SCREEN,
    SELECTION_LAYOUT,
    SELECTION_WINDOW,
    SELECTION_USER_APPLICATION,
    SELECTION_EXTERNAL_SOURCE
} SelectionKind_t;

/*************************************************************************************************/
/**
 * \brief State shared with each XML filename shown in the list box.
 */
typedef struct XmlFileListContext_s
{
    Arinc661_Configuration_Editor_t* editor;
    uint32_t                         xmlFileCount;
    bool_t                           shouldDraw;
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

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief FACE DisplayManagement.xsd unit spellings in enum order.
 */
static const char_t* const c_unitNames[] =
{
    FACEOE_DISPLAYMANAGEMENT_XML_UNIT_INCH,
    FACEOE_DISPLAYMANAGEMENT_XML_UNIT_MM,
    FACEOE_DISPLAYMANAGEMENT_XML_UNIT_SCREEN,
    FACEOE_DISPLAYMANAGEMENT_XML_UNIT_PIXEL
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Returns the status bar text for an editor status.
 *
 * \param[in] status   Editor status
 *
 * \retval    non-NULL   Status text.
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
static bool_t GetXmlFilePath(Arinc661_Configuration_Editor_t* editor, const char_t* fileName, char_t* filePath, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Recalculates the dimensions needed to fit drawable screens in the scene.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void UpdatePreviewMeasurements(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Focuses an open XML file or loads it into an available document slot.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     fileName   Clicked XML filename
 *
 * \retval        None
 */
static void LoadConfigurationFile(Arinc661_Configuration_Editor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Opens an untitled document in an available fixed slot.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void CreateDocument(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Writes the in-memory display configuration to the configured file path.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void UnloadConfigurationFile(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves a named document or requests a filename for an untitled document.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot to save
 *
 * \retval        None
 */
static void SaveDocument(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Saves every open document with unsaved changes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void SaveAllDocuments(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Checks whether a filename is suitable for a new XML document.
 *
 * \param[in] fileName   Proposed filename
 *
 * \retval    TRUE       if it is a valid XML filename.
 * \retval    FALSE      otherwise.
 */
static bool_t IsValidXmlFileName(const char_t* fileName);

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
 * \brief Chooses the next identifier for a selected entity type.
 *
 * \param[in]  editor        Editor state
 * \param[in]  kind          Entity type
 * \param[in]  screenIndex   Owning screen index for a layout
 * \param[out] id            Next available identifier
 *
 * \retval     TRUE        if an identifier is available.
 * \retval     FALSE       if the identifier range is exhausted or kind is invalid.
 */
static bool_t NextIdentifier(const Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, int64_t* id);

/*************************************************************************************************/
/**
 * \brief Checks an identifier against its required scope, excluding one existing item.
 *
 * \param[in] editor        Document to check
 * \param[in] kind          Entity type
 * \param[in] screenIndex   Owning screen for a layout
 * \param[in] itemIndex     Item whose identifier is being edited
 * \param[in] id            Proposed identifier
 *
 * \retval    TRUE          if the identifier is unused in its scope.
 * \retval    FALSE         if another item already uses it.
 */
static bool_t IsIdentifierUnique(const Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, uint32_t itemIndex, int64_t id);

/*************************************************************************************************/
/**
 * \brief Finds the first repeated configuration identifier.
 *
 * \param[in] editor   Document to check
 *
 * \retval    STATUS_NONE                          if all identifiers are unique.
 * \retval    STATUS_DUPLICATE_SCREEN_ID           if a screen identifier is duplicated.
 * \retval    STATUS_DUPLICATE_LAYOUT_ID           if a layout identifier is duplicated.
 * \retval    STATUS_DUPLICATE_WINDOW_ID           if a window identifier is duplicated.
 * \retval    STATUS_DUPLICATE_APPLICATION_ID      if an application identifier is duplicated.
 * \retval    STATUS_DUPLICATE_EXTERNAL_SOURCE_ID  if an external source identifier is duplicated.
 */
static Status_t CheckIdentifierUniqueness(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds the largest screen identifier used to choose the next one.
 *
 * \param[in]  editor   Editor state
 *
 * \retval    int64_t   Largest screen identifier, or NO_IDENTIFIER if there are no screens.
 */
static int64_t GetMaxScreenIdentifier(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds the largest identifier within a screen's layouts.
 *
 * \param[in]  editor        Editor state
 * \param[in]  screenIndex   Owning screen index
 *
 * \retval    int64_t   Largest layout identifier, or NO_IDENTIFIER if the screen has no layouts.
 */
static int64_t GetMaxLayoutIdentifier(const Arinc661_Configuration_Document_t* editor, uint32_t screenIndex);

/*************************************************************************************************/
/**
 * \brief Finds the largest window identifier used to choose the next one.
 *
 * \param[in]  editor   Editor state
 *
 * \retval    int64_t   Largest window identifier, or NO_IDENTIFIER if there are no windows.
 */
static int64_t GetMaxWindowIdentifier(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds the largest user application identifier used to choose the next one.
 *
 * \param[in] editor   Editor state
 *
 * \retval    int64_t   Largest application identifier, or NO_IDENTIFIER if there are none.
 */
static int64_t GetMaxUserApplicationIdentifier(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Appends a screen with default dimensions.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void AddScreen(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Appends a layout to the selected screen.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     screenIndex   Owning screen index
 *
 * \retval        None
 */
static void AddLayout(Arinc661_Configuration_Document_t* editor, uint32_t screenIndex);

/*************************************************************************************************/
/**
 * \brief Appends a window to the selected layout.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     layoutIndex   Owning layout index
 *
 * \retval        None
 */
static void AddWindow(Arinc661_Configuration_Document_t* editor, uint32_t layoutIndex);

/*************************************************************************************************/
/**
 * \brief Appends a user application with an unused identifier.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void AddUserApplication(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Removes one user application and updates its array.
 *
 * \param[in,out] editor             Editor state
 * \param[in]     applicationIndex   Application index to remove
 *
 * \retval        None
 */
static void RemoveUserApplication(Arinc661_Configuration_Document_t* editor, uint32_t applicationIndex);

/*************************************************************************************************/
/**
 * \brief Appends an external source with the next free id and selects it.
 *
 * \param[in,out] editor   Document being edited
 *
 * \retval        None
 */
static void AddExternalSource(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Removes one external source.
 *
 * \param[in,out] editor        Document being edited
 * \param[in]     sourceIndex   External source array index
 *
 * \retval        None
 */
static void RemoveExternalSource(Arinc661_Configuration_Document_t* editor, uint32_t sourceIndex);

/*************************************************************************************************/
/**
 * \brief Edits the selected external source's id, name, and type.
 *
 * \param[in,out] editor   Document being edited
 *
 * \retval        None
 */
static void DrawExternalSourceInspector(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Removes one window and updates its array.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     windowIndex   Window index to remove
 *
 * \retval        None
 */
static void RemoveWindow(Arinc661_Configuration_Document_t* editor, uint32_t windowIndex);

/*************************************************************************************************/
/**
 * \brief Removes one layout and all of its windows.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     layoutIndex   Layout index to remove
 *
 * \retval        None
 */
static void RemoveLayout(Arinc661_Configuration_Document_t* editor, uint32_t layoutIndex);

/*************************************************************************************************/
/**
 * \brief Removes one screen and all of its layouts and windows.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     screenIndex   Screen index to remove
 *
 * \retval        None
 */
static void RemoveScreen(Arinc661_Configuration_Document_t* editor, uint32_t screenIndex);

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
 * \brief Draws the selection tree and its add and remove controls.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawHierarchy(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Begins a two-column property table with the editor's standard columns.
 *
 * \param[in] name   Table identifier
 *
 * \retval    TRUE        if the table is visible.
 * \retval    FALSE       if the table is not visible.
 */
static bool_t BeginPropertyTable(const char_t* name);

/*************************************************************************************************/
/**
 * \brief Draws one signed integer in the two-column inspector.
 *
 * \param[in]     label   Visible field label
 * \param[in,out] value   Integer value
 *
 * \retval        TRUE        if the value changed.
 * \retval        FALSE       if the value did not change.
 */
static bool_t DrawIntegerField(const char_t* label, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Draws an identifier and accepts only values unused by another item.
 *
 * \param[in,out] editor        Document containing the item
 * \param[in]     kind          Entity type
 * \param[in]     screenIndex   Owning screen for a layout
 * \param[in]     itemIndex     Index of the selected item
 * \param[in,out] value         Identifier to edit
 *
 * \retval        TRUE          if the identifier changed.
 * \retval        FALSE         otherwise.
 */
static bool_t DrawIdentifierField(Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, uint32_t itemIndex, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Draws one decimal in the two-column inspector.
 *
 * \param[in]     label   Visible field label
 * \param[in,out] value   Decimal value
 *
 * \retval        TRUE        if the value changed.
 * \retval        FALSE       if the value did not change.
 */
static bool_t DrawDecimalField(const char_t* label, float32_t* value);

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
 * \brief Draws one Boolean in the two-column inspector.
 *
 * \param[in]     label   Visible field label
 * \param[in,out] value   Boolean value
 *
 * \retval        TRUE        if the value changed.
 * \retval        FALSE       if the value did not change.
 */
static bool_t DrawBooleanField(const char_t* label, bool_t* value);

/*************************************************************************************************/
/**
 * \brief Draws a unit value as a choice field.
 *
 * \param[in]     label   Visible field label
 * \param[in,out] unit    Unit value
 *
 * \retval        TRUE        if the value changed.
 * \retval        FALSE       if the value did not change.
 */
static bool_t DrawUnitField(const char_t* label, FACEOE_DisplayManagement_Unit_t* unit);

/*************************************************************************************************/
/**
 * \brief Draws a combo of the document's windows for a user application's window reference.
 *
 * \param[in]     editor   Document holding the windows
 * \param[in]     label    Row label
 * \param[in,out] value    Window identifier
 *
 * \retval        TRUE     if the value changed.
 * \retval        FALSE    otherwise.
 */
static bool_t DrawWindowChoiceField(const Arinc661_Configuration_Document_t* editor, const char_t* label, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Draws a combo of the known Definition Files for a user application's dFPath.
 *
 * \param[in]     editor     Editor holding the Definition File names
 * \param[in]     label      Row label
 * \param[in,out] value      Path text
 * \param[in]     capacity   Path capacity
 *
 * \retval        TRUE       if the value changed.
 * \retval        FALSE      otherwise.
 */
static bool_t DrawDefinitionChoiceField(const Arinc661_Configuration_Editor_t* editor, const char_t* label, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Records an inspector edit as an unsaved document change.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     hasChanged   TRUE if a field changed
 *
 * \retval        None
 */
static void MarkDocumentDirty(Arinc661_Configuration_Document_t* editor, bool_t hasChanged);

/*************************************************************************************************/
/**
 * \brief Draws the selected screen's properties.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawScreenInspector(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected layout's properties.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawLayoutInspector(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected window's properties.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawWindowInspector(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected user application's properties.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawUserApplicationInspector(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Lists the user applications whose window attribute names this window, with their Definition Files, and selects one on click.
 *
 * \param editor   Editor state
 * \param window   Window
 *
 * \retval None
 */
static void DrawWindowApplications(Arinc661_Configuration_Document_t* editor, const FACEOE_DisplayManagement_Window_t* window);

/*************************************************************************************************/
/**
 * \brief Copies the five arrays' used parts and the counts into one heap block.
 *
 * \param document   Document
 *
 * \retval void*   Heap block holding the snapshot; the caller frees it.
 */
static void* TakeSnapshot(const Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Copies a snapshot back and recomputes the derived screen extents.
 *
 * \param document   Document
 * \param snapshot   Model snapshot
 *
 * \retval None
 */
static void RestoreSnapshot(Arinc661_Configuration_Document_t* document, const void* snapshot);

/*************************************************************************************************/
/**
 * \brief Counts and used arrays must match; the selection is ignored.
 *
 * \param document   Document
 * \param snapshot   Model snapshot
 *
 * \retval TRUE   if the model equals the snapshot.
 * \retval FALSE  otherwise.
 */
static bool_t SnapshotMatches(const Arinc661_Configuration_Document_t* document, const void* snapshot);

/*************************************************************************************************/
/**
 * \brief Frees every snapshot.
 *
 * \param document   Document
 *
 * \retval None
 */
static void ClearHistory(Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Once per frame: a changed model turns the shadow into an undo step and drops the redo stack.
 *
 * \param document   Document
 *
 * \retval None
 */
static void CommitHistory(Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief The current state moves to the redo stack; the newest undo step becomes the model.
 *
 * \param document   Document
 *
 * \retval None
 */
static void UndoDocument(Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Mirror of UndoDocument.
 *
 * \param document   Document
 *
 * \retval None
 */
static void RedoDocument(Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Removes the selected element through the same paths the hierarchy uses.
 *
 * \param document   Document
 *
 * \retval None
 */
static void RemoveSelection(Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief A window copy keeps the geometry and layout with a fresh id, offset by a tenth of its size so it is visible; a user application copy keeps its window and paths with a fresh applicationId.
 *
 * \param document   Document
 *
 * \retval None
 */
static void DuplicateSelection(Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Combo over the document's screens, shown by id.
 *
 * \param document      Document
 * \param label         Row label
 * \param screenIndex   Screen array index
 *
 * \retval TRUE   if the value changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawScreenChoiceField(const Arinc661_Configuration_Document_t* document, const char_t* label, uint32_t* screenIndex);

/*************************************************************************************************/
/**
 * \brief Combo over the document's layouts, each shown with its screen.
 *
 * \param document      Document
 * \param label         Row label
 * \param layoutIndex   Layout array index
 *
 * \retval TRUE   if the value changed.
 * \retval FALSE  otherwise.
 */
static bool_t DrawLayoutChoiceField(const Arinc661_Configuration_Document_t* document, const char_t* label, uint32_t* layoutIndex);

/*************************************************************************************************/
/**
 * \brief Icon row above the canvas: add screen, layout, window, user application, then duplicate and delete for the selection.
 *
 * \param owner      Editor owning the document
 * \param document   Document
 *
 * \retval None
 */
static void DrawSceneToolbar(Arinc661_Configuration_Editor_t* owner, Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Direct manipulation of windows on the scene: press on a window selects it and starts a move; press on a corner handle of the selected window starts a resize; the arrow keys nudge; Delete removes the selection; a right-click opens a menu to add elements where the cursor is.
 *
 * \param owner             Editor owning the document
 * \param document          Document
 * \param hitScreen         Screen under the cursor
 * \param hitWindow         Window under the cursor
 * \param windowMin         Screen top-left of the selected window
 * \param windowMax         Screen bottom-right of the selected window
 * \param hasWindowArea     TRUE when a window is selected and drawn
 * \param scale             Screen pixels per layer unit
 * \param drawList          Target draw list
 * \param isCanvasHovered   TRUE while the mouse is over the canvas
 * \param hintPosition      Where the hint line is drawn
 *
 * \retval None
 */
static void DrawSceneInteraction(Arinc661_Configuration_Editor_t* owner, Arinc661_Configuration_Document_t* document, uint32_t hitScreen, uint32_t hitWindow, ImVec2 windowMin, ImVec2 windowMax, bool_t hasWindowArea, float32_t scale, ImDrawList* drawList, bool_t isCanvasHovered, ImVec2 hintPosition);

/*************************************************************************************************/
/**
 * \brief Asks the context what it knows about the user application's Definition File: presence in the folder, whether it is open, its layer and widget counts, and whether its ApplicationId agrees with the FACE applicationId.
 *
 * \param editor        Editor state
 * \param document      Document
 * \param application   User application
 *
 * \retval None
 */
static void CreateDefinitionForSelection(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Draws a user application's Definition File tree under its hierarchy node.
 *
 * \param[in,out] document      Document being edited
 * \param[in]     application   The user application
 *
 * \retval        None
 */
static void DrawEmbeddedDefinition(Arinc661_Configuration_Document_t* document, const FACEOE_DisplayManagement_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief Lists the layers of the user application's open Definition File with fit checks.
 *
 * \param[in,out] editor        Editor state
 * \param[in]     document      Document being edited
 * \param[in]     application   The user application
 *
 * \retval        None
 */
static void DrawDefinitionLayers(Arinc661_Configuration_Editor_t* editor, const Arinc661_Configuration_Document_t* document, const FACEOE_DisplayManagement_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief Draws what the context knows about the user application's Definition File.
 *
 * \param[in,out] editor        Editor state
 * \param[in,out] document      Document being edited
 * \param[in,out] application   The user application
 *
 * \retval        None
 */
static void DrawDefinitionState(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document, FACEOE_DisplayManagement_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief FACE H.3.2: a window's scalingFactor relates baseUnit to perUnit; when it relates pixels to millimetres or inches it gives pixels per 0.01 mm directly.
 *
 * \param screen         Screen
 * \param window         Window
 * \param isHorizontal   TRUE for the X axis
 *
 * \retval float32_t   Screen pixels per 0.01 mm along the axis.
 */
static float32_t PixelsPerLayerUnit(const FACEOE_DisplayManagement_Screen_t* screen, const FACEOE_DisplayManagement_Window_t* window, bool_t isHorizontal);

/*************************************************************************************************/
/**
 * \brief Every user application targeting the window has its open Definition File recorded with the layer origin at the window's lower-left corner, scaled by the window's pixel density and the scene scale, clipped to the window.
 *
 * \param editor     Editor state
 * \param document   Document
 * \param screen     Screen
 * \param window     Window
 * \param areaMin    Screen top-left of the window area
 * \param areaMax    Screen bottom-right of the window area
 * \param scale      Screen pixels per layer unit
 * \param drawList   Target draw list
 *
 * \retval None
 */
static void DrawWindowPreview(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document, const FACEOE_DisplayManagement_Screen_t* screen, const FACEOE_DisplayManagement_Window_t* window, ImVec2 areaMin, ImVec2 areaMax, float32_t scale, ImDrawList* drawList);

/*************************************************************************************************/
/**
 * \brief Counts and optionally draws one XML filename discovered by the OS file layer.
 *
 * \param[in] fileName   Filename in the selected folder
 * \param[in] context    XML file list state
 *
 * \retval    None
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
static void DrawXmlFileList(Arinc661_Configuration_Editor_t* editor, float32_t folderInputX);

/*************************************************************************************************/
/**
 * \brief Draws the file path, file actions, and status above the panes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawFileControls(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Closes a document slot and selects another open document.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot to close
 *
 * \retval        None
 */
static void CloseDocument(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Draws the File menu for the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawFileMenuBar(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the filename dialog for an untitled document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawSaveAsDialog(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws document tabs and handles save confirmation for dirty tabs.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawDocumentTabs(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the document state and latest editor message below the panes.
 *
 * \param[in] editor   Editor state
 *
 * \retval    None
 */
static void DrawStatusBar(const Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the configuration hierarchy in the left child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawHierarchyPane(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected configuration item's properties in the right child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawInspectorPane(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Draws screen and window geometry with selected hierarchy items highlighted.
 *
 * \param[in,out] editor   Editor state and hierarchy selection
 *
 * \retval        None
 */
static void DrawScenePane(Arinc661_Configuration_Editor_t* owner, Arinc661_Configuration_Document_t* editor);

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
 * \brief Measures the minimum hierarchy width and default inspector width.
 *
 * \param[in]  editor           Editor state containing hierarchy row labels
 * \param[out] hierarchyWidth   Minimum width needed for hierarchy controls
 * \param[out] inspectorWidth   Width needed for the property fields
 *
 * \retval     None
 */
static void MeasurePaneWidths(const Arinc661_Configuration_Document_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*************************************************************************************************/
/**
 * \brief Draws the hierarchy, scene, and inspector panes for the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
static void DrawPanes(Arinc661_Configuration_Editor_t* editor);

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
        case STATUS_NONE:                            return "";
        case STATUS_FOLDER_PATH_REQUIRED:            return "Enter a folder path.";
        case STATUS_XML_FILE_REQUIRED:               return "Select an XML file.";
        case STATUS_SERIALIZATION_FAILED:            return "Could not save XML file.";
        case STATUS_SAVED:                           return "Saved XML file.";
        case STATUS_ALL_SAVED:                       return "Saved all changed XML files.";
        case STATUS_SAVE_ALL_FAILED:                 return "Could not save every XML file.";
        case STATUS_NEW_DOCUMENT:                    return "Created a new XML document.";
        case STATUS_SAVE_NAME_REQUIRED:              return "Enter an XML filename.";
        case STATUS_SAVE_NAME_INVALID:               return "Enter a valid filename ending in .xml.";
        case STATUS_SAVE_FILE_EXISTS:                return "An XML file with that name already exists.";
        case STATUS_SAVE_FOLDER_INVALID:             return "Could not read the save folder.";
        case STATUS_SAVE_ALL_CANCELED:               return "Save All canceled.";
        case STATUS_WRITE_FAILED:                    return "Could not write XML file.";
        case STATUS_SCREEN_ADDED:                    return "Added screen.";
        case STATUS_SCREEN_ADD_FAILED:               return "Could not add another screen.";
        case STATUS_LAYOUT_ADDED:                    return "Added layout.";
        case STATUS_LAYOUT_ADD_FAILED:               return "Could not add another layout.";
        case STATUS_WINDOW_ADDED:                    return "Added window.";
        case STATUS_WINDOW_ADD_FAILED:               return "Could not add another window.";
        case STATUS_USER_APPLICATION_ADDED:          return "Added user application.";
        case STATUS_USER_APPLICATION_ADD_FAILED:     return "Could not add another user application.";
        case STATUS_SCREEN_REMOVED:                  return "Removed screen.";
        case STATUS_LAYOUT_REMOVED:                  return "Removed layout.";
        case STATUS_WINDOW_REMOVED:                  return "Removed window.";
        case STATUS_USER_APPLICATION_REMOVED:        return "Removed user application.";
        case STATUS_EXTERNAL_SOURCE_ADDED:           return "Added external source.";
        case STATUS_EXTERNAL_SOURCE_ADD_FAILED:      return "Could not add another external source.";
        case STATUS_EXTERNAL_SOURCE_REMOVED:         return "Removed external source.";
        case STATUS_DUPLICATE_EXTERNAL_SOURCE_ID:    return "External source IDs must be unique in this document.";
        case STATUS_READ_FAILED:                     return "Could not read XML file.";
        case STATUS_INPUT_TOO_LARGE:                 return "The XML file exceeds the input buffer.";
        case STATUS_INPUT_EMPTY:                     return "The XML file is empty.";
        case STATUS_LOADED:                          return "Loaded XML file.";
        case STATUS_INVALID_XML:                     return "Invalid or unsupported display configuration XML.";
        case STATUS_DUPLICATE_SCREEN_ID:             return "Screen IDs must be unique in this document.";
        case STATUS_DUPLICATE_LAYOUT_ID:             return "Layout IDs must be unique within a screen.";
        case STATUS_DUPLICATE_WINDOW_ID:             return "Window IDs must be unique in this document.";
        case STATUS_DUPLICATE_APPLICATION_ID:        return "Application IDs must be unique in this document.";
        case STATUS_TOO_MANY_DOCUMENTS:              return "Close a document before opening another XML file.";
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
static bool_t GetXmlFilePath(Arinc661_Configuration_Editor_t* editor, const char_t* fileName, char_t* filePath, uint32_t capacity)
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
 *  Sums valid screen widths and finds the tallest valid screen in the current configuration.
 */
static void UpdatePreviewMeasurements(Arinc661_Configuration_Document_t* editor)
{
    const FACEOE_DisplayManagement_Screen_t* screen;
    uint32_t                     screen_index;
    bool_t                       has_screen_width;
    bool_t                       has_screen_height;
    bool_t                       is_drawable_screen;

    editor->totalScreenWidth    = 0.0f;
    editor->maximumScreenHeight = 0.0f;
    editor->drawableScreenCount = 0;
    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        screen             = &editor->screens[screen_index];
        has_screen_width   = (0 < screen->pixelSize.width);
        has_screen_height  = (0 < screen->pixelSize.height);
        is_drawable_screen = !!(has_screen_width && has_screen_height);
        if (is_drawable_screen)
        {
            editor->drawableScreenCount += 1;
            editor->totalScreenWidth    += (float32_t)screen->pixelSize.width;
            editor->maximumScreenHeight = ImMax(editor->maximumScreenHeight, (float32_t)screen->pixelSize.height);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves an open document's edits when the selected path matches its stored path.
 */
static void LoadConfigurationFile(Arinc661_Configuration_Editor_t* editor, const char_t* fileName)
{
    Arinc661_Configuration_Document_t* document;
    Status_t                           identifier_status;
    char_t                             file_path[ARINC661_CONFIGURATION_MAX_FILE_PATH_BYTES];
    int32_t                            document_byte_count;
    uint32_t                           index;
    uint32_t                           document_index;
    uint32_t                           available_index;
    bool_t                             is_configuration;
    bool_t                             has_file_path;
    bool_t                             has_matching_path;

    has_file_path = GetXmlFilePath(editor, fileName, file_path, sizeof(file_path));
    if (has_file_path)
    {
        available_index = INVALID_DOCUMENT_INDEX;
        document_index  = INVALID_DOCUMENT_INDEX;
        for (index = 0; index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++index)
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
            ClearHistory(document);
            document->isLoaded            = FALSE;
            document->isDirty             = FALSE;
            document->selectedKind        = SELECTION_NONE;
            document->selectedIndex       = 0;
            document->totalScreenWidth    = 0.0f;
            document->maximumScreenHeight = 0.0f;
            document->drawableScreenCount = 0;
            document->configuration.screenCount          = 0;
            document->configuration.layoutCount          = 0;
            document->configuration.windowCount          = 0;
            document->configuration.userApplicationCount = 0;
            document->configuration.externalSourceCount  = 0;
            document->configuration.hasMetaData          = FALSE;
            document->configuration.hasXmlDeclaration    = FALSE;
            document->configuration.schemaLocation[0]    = '\0';
            document->documentName[0]                    = '\0';
            document->documentPath[0]                    = '\0';

            document_byte_count = OS_File_ReadFile(file_path, document->inputBuffer, sizeof(document->inputBuffer));
            if (0 > document_byte_count)
            {
                editor->status = STATUS_READ_FAILED;
            }
            else if (0 == document_byte_count)
            {
                editor->status = STATUS_INPUT_EMPTY;
            }
            else if (ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES < document_byte_count)
            {
                editor->status = STATUS_INPUT_TOO_LARGE;
            }
            else
            {
                is_configuration = FACEOE_DisplayManagement_ReadConfiguration(&document->configuration, document->inputBuffer, (uint64_t)document_byte_count, document->workspace, sizeof(document->workspace));
                if (is_configuration)
                {
                    identifier_status = CheckIdentifierUniqueness(document);
                }
                else
                {
                    identifier_status = STATUS_NONE;
                }
                if (is_configuration && (STATUS_NONE == identifier_status))
                {
                    document->isLoaded = TRUE;
                    ImStrncpy(document->documentName, fileName, sizeof(document->documentName));
                    ImStrncpy(document->documentPath, file_path, sizeof(document->documentPath));
                    if (0 < document->configuration.screenCount)
                    {
                        document->selectedKind = SELECTION_SCREEN;
                    }
                    else if (0 < document->configuration.userApplicationCount)
                    {
                        document->selectedKind = SELECTION_USER_APPLICATION;
                    }
                    UpdatePreviewMeasurements(document);
                    document->status              = STATUS_LOADED;
                    editor->activeDocumentIndex    = available_index;
                    editor->requestedDocumentIndex = available_index;
                    editor->status                 = STATUS_NONE;
                }
                else if (STATUS_NONE != identifier_status)
                {
                    editor->status = identifier_status;
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
static void CreateDocument(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           document_index;
    uint32_t                           available_index;

    available_index = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
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
        ClearHistory(document);
        document->status              = STATUS_NEW_DOCUMENT;
        document->isLoaded            = TRUE;
        document->isDirty             = TRUE;
        document->selectedKind        = SELECTION_NONE;
        document->selectedIndex       = 0;
        document->totalScreenWidth    = 0.0f;
        document->maximumScreenHeight = 0.0f;
        document->drawableScreenCount = 0;
        ImFormatString(document->documentName, sizeof(document->documentName), UNTITLED_DOCUMENT_FORMAT, available_index + 1);
        document->documentPath[0] = '\0';
        document->configuration.screenCount          = 0;
        document->configuration.layoutCount          = 0;
        document->configuration.windowCount          = 0;
        document->configuration.userApplicationCount = 0;
        document->configuration.externalSourceCount  = 0;
        document->configuration.hasMetaData          = FALSE;
        document->configuration.hasXmlDeclaration    = FALSE;
        document->configuration.schemaLocation[0]    = '\0';
        editor->activeDocumentIndex    = available_index;
        editor->requestedDocumentIndex = available_index;
        editor->status                 = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps file output outside the in-memory arinc661_definition module.
 */
static void UnloadConfigurationFile(Arinc661_Configuration_Document_t* editor)
{
    Status_t identifier_status;
    uint64_t output_byte_count;
    int32_t  bytes_written;
    bool_t   can_save_document;

    can_save_document = !!(editor->isLoaded && ('\0' != editor->documentPath[0]));
    if (can_save_document)
    {
        identifier_status = CheckIdentifierUniqueness(editor);
        if (STATUS_NONE != identifier_status)
        {
            editor->status = identifier_status;
        }
        else
        {
            output_byte_count = FACEOE_DisplayManagement_WriteConfiguration(&editor->configuration, editor->outputBuffer, sizeof(editor->outputBuffer), editor->workspace, sizeof(editor->workspace));
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
static void SaveDocument(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex)
{
    Arinc661_Configuration_Document_t* document;

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
        UnloadConfigurationFile(document);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attempts every dirty document and leaves a failed document marked dirty.
 */
static void SaveAllDocuments(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           document_index;
    bool_t                             all_saved;

    all_saved = TRUE;
    editor->pendingSaveDocumentIndex = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
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
                UnloadConfigurationFile(document);
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
 *  Keeps a new file inside its selected folder and requires the XML extension.
 */
static bool_t IsValidXmlFileName(const char_t* fileName)
{
    size_t  file_name_length;
    size_t  extension_length;
    size_t  index;
    bool_t  is_valid;
    bool_t  has_xml_extension;

    file_name_length = ImStrlen(fileName);
    extension_length = ImStrlen(XML_EXTENSION);
    is_valid         = (extension_length < file_name_length);
    if (is_valid)
    {
        has_xml_extension = (0 == ImStricmp(fileName + file_name_length - extension_length, XML_EXTENSION));
        is_valid          = has_xml_extension;
    }
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
 *  Lists every window of the document by identifier and name; FACE H.3.1 requires a user
 *  application's window to name one of them. A reference to no known window shows as none.
 */
static bool_t DrawWindowChoiceField(const Arinc661_Configuration_Document_t* editor, const char_t* label, int64_t* value)
{
    const FACEOE_DisplayManagement_Window_t* window;
    char_t                                   choice[CHOICE_LABEL_BYTES];
    const char_t*                            preview;
    uint32_t                                 window_index;
    bool_t                                   has_changed;

    has_changed = FALSE;
    preview     = NO_WINDOW_LABEL;
    for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
    {
        if ((*value) == editor->windows[window_index].id)
        {
            ImFormatString(choice, sizeof(choice), WINDOW_CHOICE_FORMAT, editor->windows[window_index].id, editor->windows[window_index].name);
            preview = choice;
            break;
        }
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo(PROPERTY_INPUT_ID, preview))
    {
        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
        {
            window = &editor->windows[window_index];
            ImGui::PushID(window);
            ImFormatString(choice, sizeof(choice), WINDOW_CHOICE_FORMAT, window->id, window->name);
            if (ImGui::Selectable(choice, (*value) == window->id))
            {
                (*value)    = window->id;
                has_changed = TRUE;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offers the Definition Files found in the folder, which the host validated as ARINC 661
 *  Definition Files, plus none. A path outside the list is shown as typed.
 */
static bool_t DrawDefinitionChoiceField(const Arinc661_Configuration_Editor_t* editor, const char_t* label, char_t* value, uint32_t capacity)
{
    const char_t* preview;
    uint32_t      file_index;
    bool_t        has_changed;

    has_changed = FALSE;
    preview     = value;
    if ('\0' == value[0])
    {
        preview = NO_DEFINITION_LABEL;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo(PROPERTY_INPUT_ID, preview))
    {
        if (ImGui::Selectable(NO_DEFINITION_LABEL, '\0' == value[0]))
        {
            value[0]    = '\0';
            has_changed = TRUE;
        }
        for (file_index = 0; file_index < editor->definitionFileCount; ++file_index)
        {
            if (ImGui::Selectable(editor->definitionFiles[file_index], 0 == strcmp(value, editor->definitionFiles[file_index])))
            {
                ImStrncpy(value, editor->definitionFiles[file_index], capacity);
                has_changed = TRUE;
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
 *  Dispatches to the identifier search for the selected entity type.
 */
static bool_t NextIdentifier(const Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, int64_t* id)
{
    int64_t maximum_id;
    bool_t  has_next_id;
    uint32_t index;

    switch (kind)
    {
        case SELECTION_SCREEN:
        {
            maximum_id = GetMaxScreenIdentifier(editor);
            break;
        }
        case SELECTION_LAYOUT:
        {
            maximum_id = GetMaxLayoutIdentifier(editor, screenIndex);
            break;
        }
        case SELECTION_WINDOW:
        {
            maximum_id = GetMaxWindowIdentifier(editor);
            break;
        }
        case SELECTION_EXTERNAL_SOURCE:
        {
            maximum_id = NO_IDENTIFIER;
            for (index = 0; index < editor->configuration.externalSourceCount; ++index)
            {
                maximum_id = ImMax(maximum_id, (int64_t)editor->externalSources[index].id);
            }
            break;
        }
        case SELECTION_USER_APPLICATION:
        {
            maximum_id = GetMaxUserApplicationIdentifier(editor);
            break;
        }
        case SELECTION_NONE:
        default:
        {
            return FALSE;
        }
    }

    has_next_id = (MAX_IDENTIFIER != maximum_id);
    if (has_next_id)
    {
        (*id) = maximum_id + 1;
    }

    return has_next_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Screen, window, and application IDs are document-wide; layout IDs are local to one screen.
 */
static bool_t IsIdentifierUnique(const Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, uint32_t itemIndex, int64_t id)
{
    uint32_t index;
    bool_t   is_unique;

    is_unique = TRUE;
    switch (kind)
    {
        case SELECTION_SCREEN:
        {
            for (index = 0; index < editor->configuration.screenCount; ++index)
            {
                if ((index != itemIndex) && (id == editor->screens[index].id))
                {
                    is_unique = FALSE;
                    break;
                }
            }
            break;
        }
        case SELECTION_LAYOUT:
        {
            for (index = 0; index < editor->configuration.layoutCount; ++index)
            {
                if ((index != itemIndex) && (screenIndex == editor->layouts[index].screenIndex) && (id == editor->layouts[index].id))
                {
                    is_unique = FALSE;
                    break;
                }
            }
            break;
        }
        case SELECTION_WINDOW:
        {
            for (index = 0; index < editor->configuration.windowCount; ++index)
            {
                if ((index != itemIndex) && (id == editor->windows[index].id))
                {
                    is_unique = FALSE;
                    break;
                }
            }
            break;
        }
        case SELECTION_EXTERNAL_SOURCE:
        {
            for (index = 0; index < editor->configuration.externalSourceCount; ++index)
            {
                if ((index != itemIndex) && (id == (int64_t)editor->externalSources[index].id))
                {
                    is_unique = FALSE;
                }
            }
            break;
        }
        case SELECTION_USER_APPLICATION:
        {
            for (index = 0; index < editor->configuration.userApplicationCount; ++index)
            {
                if ((index != itemIndex) && (id == editor->userApplications[index].applicationId))
                {
                    is_unique = FALSE;
                    break;
                }
            }
            break;
        }
        case SELECTION_NONE:
        default:
        {
            is_unique = FALSE;
            break;
        }
    }

    return is_unique;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks every identifier scope before loading or saving.
 */
static Status_t CheckIdentifierUniqueness(const Arinc661_Configuration_Document_t* editor)
{
    Status_t status;
    uint32_t item_index;
    uint32_t other_index;
    bool_t   is_unique;

    status = STATUS_NONE;
    for (item_index = 0; item_index < editor->configuration.screenCount; ++item_index)
    {
        is_unique = IsIdentifierUnique(editor, SELECTION_SCREEN, 0, item_index, editor->screens[item_index].id);
        if (FALSE == is_unique)
        {
            status = STATUS_DUPLICATE_SCREEN_ID;
            break;
        }
    }
    if (STATUS_NONE == status)
    {
        for (item_index = 0; item_index < editor->configuration.layoutCount; ++item_index)
        {
            is_unique = IsIdentifierUnique(editor, SELECTION_LAYOUT, editor->layouts[item_index].screenIndex, item_index, editor->layouts[item_index].id);
            if (FALSE == is_unique)
            {
                status = STATUS_DUPLICATE_LAYOUT_ID;
                break;
            }
        }
    }
    if (STATUS_NONE == status)
    {
        for (item_index = 0; item_index < editor->configuration.windowCount; ++item_index)
        {
            is_unique = IsIdentifierUnique(editor, SELECTION_WINDOW, 0, item_index, editor->windows[item_index].id);
            if (FALSE == is_unique)
            {
                status = STATUS_DUPLICATE_WINDOW_ID;
                break;
            }
        }
    }
    if (STATUS_NONE == status)
    {
        for (item_index = 0; item_index < editor->configuration.userApplicationCount; ++item_index)
        {
            is_unique = IsIdentifierUnique(editor, SELECTION_USER_APPLICATION, 0, item_index, editor->userApplications[item_index].applicationId);
            if (FALSE == is_unique)
            {
                status = STATUS_DUPLICATE_APPLICATION_ID;
            }
        }
    }
    if (STATUS_NONE == status)
    {
        for (item_index = 0; item_index < editor->configuration.externalSourceCount; ++item_index)
        {
            for (other_index = item_index + 1; other_index < editor->configuration.externalSourceCount; ++other_index)
            {
                if (editor->externalSources[item_index].id == editor->externalSources[other_index].id)
                {
                    status = STATUS_DUPLICATE_EXTERNAL_SOURCE_ID;
                    break;
                }
            }
            if (STATUS_NONE != status)
            {
                break;
            }
        }
    }

    return status;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Screen identifiers are global to the configuration.
 */
static int64_t GetMaxScreenIdentifier(const Arinc661_Configuration_Document_t* editor)
{
    int64_t  maximum_id;
    uint32_t index;

    maximum_id = NO_IDENTIFIER;

    for (index = 0; index < editor->configuration.screenCount; ++index)
    {
        maximum_id = ImMax(maximum_id, editor->screens[index].id);
    }

    return maximum_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layout identifiers are local to their owning screen.
 */
static int64_t GetMaxLayoutIdentifier(const Arinc661_Configuration_Document_t* editor, uint32_t screenIndex)
{
    int64_t  maximum_id;
    uint32_t index;

    maximum_id = NO_IDENTIFIER;

    for (index = 0; index < editor->configuration.layoutCount; ++index)
    {
        if (screenIndex == editor->layouts[index].screenIndex)
        {
            maximum_id = ImMax(maximum_id, editor->layouts[index].id);
        }
    }

    return maximum_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Window identifiers are global to the configuration.
 */
static int64_t GetMaxWindowIdentifier(const Arinc661_Configuration_Document_t* editor)
{
    int64_t  maximum_id;
    uint32_t index;

    maximum_id = NO_IDENTIFIER;

    for (index = 0; index < editor->configuration.windowCount; ++index)
    {
        maximum_id = ImMax(maximum_id, editor->windows[index].id);
    }

    return maximum_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  User application identifiers are global to the configuration.
 */
static int64_t GetMaxUserApplicationIdentifier(const Arinc661_Configuration_Document_t* editor)
{
    int64_t  maximum_id;
    uint32_t index;

    maximum_id = NO_IDENTIFIER;
    for (index = 0; index < editor->configuration.userApplicationCount; ++index)
    {
        maximum_id = ImMax(maximum_id, editor->userApplications[index].applicationId);
    }

    return maximum_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  New screens receive pixel and physical dimensions that can be edited immediately.
 */
static void AddScreen(Arinc661_Configuration_Document_t* editor)
{
    FACEOE_DisplayManagement_Screen_t* screen;
    int64_t                id;
    bool_t                 has_capacity;
    bool_t                 has_id;

    has_capacity = (editor->configuration.screenCount < editor->configuration.screenCapacity);
    has_id       = NextIdentifier(editor, SELECTION_SCREEN, 0, &id);
    if (has_capacity && has_id)
    {
        screen = &editor->screens[editor->configuration.screenCount];
        (void)memset(screen, 0, sizeof((*screen)));
        screen->id                        = id;
        screen->pixelSize.width           = DEFAULT_SCREEN_PIXEL_WIDTH;
        screen->pixelSize.height          = DEFAULT_SCREEN_PIXEL_HEIGHT;
        screen->pixelSize.units           = FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL;
        screen->physicalDimensions.width  = DEFAULT_SCREEN_PHYSICAL_WIDTH;
        screen->physicalDimensions.height = DEFAULT_SCREEN_PHYSICAL_HEIGHT;
        screen->physicalDimensions.units  = FACEOE_DISPLAYMANAGEMENT_UNIT_INCH;
        editor->selectedKind              = SELECTION_SCREEN;
        editor->selectedIndex             = editor->configuration.screenCount;
        editor->configuration.screenCount += 1;
        UpdatePreviewMeasurements(editor);
        editor->isDirty = TRUE;
        editor->status  = STATUS_SCREEN_ADDED;
    }
    else
    {
        editor->status = STATUS_SCREEN_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A layout stores its owning screen index and no XML location.
 */
static void AddLayout(Arinc661_Configuration_Document_t* editor, uint32_t screenIndex)
{
    FACEOE_DisplayManagement_Layout_t* layout;
    int64_t                id;
    bool_t                 has_capacity;
    bool_t                 has_id;

    has_capacity = (editor->configuration.layoutCount < editor->configuration.layoutCapacity);
    has_id       = NextIdentifier(editor, SELECTION_LAYOUT, screenIndex, &id);
    if (has_capacity && has_id)
    {
        layout = &editor->layouts[editor->configuration.layoutCount];
        (void)memset(layout, 0, sizeof((*layout)));
        layout->id            = id;
        layout->screenIndex   = screenIndex;
        editor->selectedKind  = SELECTION_LAYOUT;
        editor->selectedIndex = editor->configuration.layoutCount;
        editor->configuration.layoutCount += 1;
        editor->isDirty = TRUE;
        editor->status  = STATUS_LAYOUT_ADDED;
    }
    else
    {
        editor->status = STATUS_LAYOUT_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The window begins at the origin and uses its screen's current pixel size.
 */
static void AddWindow(Arinc661_Configuration_Document_t* editor, uint32_t layoutIndex)
{
    FACEOE_DisplayManagement_Window_t*       window;
    const FACEOE_DisplayManagement_Layout_t* layout;
    const FACEOE_DisplayManagement_Screen_t* screen;
    int64_t                      id;
    bool_t                       has_capacity;
    bool_t                       has_id;

    has_capacity = (editor->configuration.windowCount < editor->configuration.windowCapacity);
    has_id       = NextIdentifier(editor, SELECTION_WINDOW, 0, &id);
    if (has_capacity && has_id)
    {
        layout = &editor->layouts[layoutIndex];
        screen = &editor->screens[layout->screenIndex];
        window = &editor->windows[editor->configuration.windowCount];
        (void)memset(window, 0, sizeof((*window)));
        window->id = id;
        ImFormatString(window->name, sizeof(window->name), "window%" PRId64, id);
        window->layoutIndex            = layoutIndex;
        window->pixelArea.width        = screen->pixelSize.width;
        window->pixelArea.height       = screen->pixelSize.height;
        window->scalingFactor.xScale   = 20.0f;
        window->scalingFactor.yScale   = 20.0f;
        window->scalingFactor.baseUnit = FACEOE_DISPLAYMANAGEMENT_UNIT_SCREEN;
        window->scalingFactor.perUnit  = FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL;
        editor->selectedKind           = SELECTION_WINDOW;
        editor->selectedIndex          = editor->configuration.windowCount;
        editor->configuration.windowCount += 1;
        editor->isDirty = TRUE;
        editor->status  = STATUS_WINDOW_ADDED;
    }
    else
    {
        editor->status = STATUS_WINDOW_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A new application starts with an empty Definition File path and the first window if available.
 */
static void AddUserApplication(Arinc661_Configuration_Document_t* editor)
{
    FACEOE_DisplayManagement_UserApplication_t* application;
    int64_t                          id;
    bool_t                           has_capacity;
    bool_t                           has_id;
    bool_t                           has_window;

    has_capacity = (editor->configuration.userApplicationCount < editor->configuration.userApplicationCapacity);
    has_id       = NextIdentifier(editor, SELECTION_USER_APPLICATION, 0, &id);
    if (has_capacity && has_id)
    {
        application = &editor->userApplications[editor->configuration.userApplicationCount];
        (void)memset(application, 0, sizeof((*application)));
        application->applicationId = id;
        application->window        = DEFAULT_USER_APPLICATION_WINDOW_ID;
        has_window = (0 < editor->configuration.windowCount);
        if (has_window)
        {
            application->window = editor->windows[0].id;
        }
        editor->selectedKind  = SELECTION_USER_APPLICATION;
        editor->selectedIndex = editor->configuration.userApplicationCount;
        editor->configuration.userApplicationCount += 1;
        editor->isDirty = TRUE;
        editor->status  = STATUS_USER_APPLICATION_ADDED;
    }
    else
    {
        editor->status = STATUS_USER_APPLICATION_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compacts the user application array after the chosen entry.
 */
static void RemoveUserApplication(Arinc661_Configuration_Document_t* editor, uint32_t applicationIndex)
{
    uint32_t remaining_count;
    bool_t   has_remaining;

    LOGASSERT(applicationIndex < editor->configuration.userApplicationCount);
    remaining_count = (editor->configuration.userApplicationCount - (applicationIndex + 1));
    has_remaining   = (0 < remaining_count);
    if (has_remaining)
    {
        (void)memmove(
            &editor->userApplications[applicationIndex],
            &editor->userApplications[applicationIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->userApplications[0])));
    }
    editor->configuration.userApplicationCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The new source takes the next free id and empty name and type.
 */
static void AddExternalSource(Arinc661_Configuration_Document_t* editor)
{
    FACEOE_DisplayManagement_ExternalSource_t* source;
    int64_t                                    id;
    bool_t                                     has_capacity;
    bool_t                                     has_id;

    has_capacity = (editor->configuration.externalSourceCount < editor->configuration.externalSourceCapacity);
    has_id       = NextIdentifier(editor, SELECTION_EXTERNAL_SOURCE, 0, &id);
    if (has_capacity && has_id)
    {
        source = &editor->externalSources[editor->configuration.externalSourceCount];
        (void)memset(source, 0, sizeof((*source)));
        source->id            = (uint32_t)id;
        editor->selectedKind  = SELECTION_EXTERNAL_SOURCE;
        editor->selectedIndex = editor->configuration.externalSourceCount;
        editor->configuration.externalSourceCount += 1;
        editor->isDirty = TRUE;
        editor->status  = STATUS_EXTERNAL_SOURCE_ADDED;
    }
    else
    {
        editor->status = STATUS_EXTERNAL_SOURCE_ADD_FAILED;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compacts the external source array after the chosen entry.
 */
static void RemoveExternalSource(Arinc661_Configuration_Document_t* editor, uint32_t sourceIndex)
{
    uint32_t remaining_count;

    LOGASSERT(sourceIndex < editor->configuration.externalSourceCount);
    remaining_count = (editor->configuration.externalSourceCount - (sourceIndex + 1));
    if (0 < remaining_count)
    {
        (void)memmove(&editor->externalSources[sourceIndex], &editor->externalSources[sourceIndex + 1], ((uint64_t)remaining_count * sizeof(editor->externalSources[0])));
    }
    editor->configuration.externalSourceCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compacts the window array after the chosen entry.
 */
static void RemoveWindow(Arinc661_Configuration_Document_t* editor, uint32_t windowIndex)
{
    uint32_t remaining_count;
    bool_t   has_remaining;

    LOGASSERT(windowIndex < editor->configuration.windowCount);
    remaining_count = (editor->configuration.windowCount - (windowIndex + 1));
    has_remaining  = (0 < remaining_count);
    if (has_remaining)
    {
        (void)memmove(
            &editor->windows[windowIndex],
            &editor->windows[windowIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->windows[0])));
    }
    editor->configuration.windowCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes associated windows before compacting layout indices.
 */
static void RemoveLayout(Arinc661_Configuration_Document_t* editor, uint32_t layoutIndex)
{
    uint32_t window_index;
    uint32_t remaining_count;
    bool_t   has_remaining;

    LOGASSERT(layoutIndex < editor->configuration.layoutCount);
    window_index = 0;
    while (window_index < editor->configuration.windowCount)
    {
        if (layoutIndex == editor->windows[window_index].layoutIndex)
        {
            RemoveWindow(editor, window_index);
        }
        else
        {
            if (layoutIndex < editor->windows[window_index].layoutIndex)
            {
                editor->windows[window_index].layoutIndex -= 1;
            }
            window_index += 1;
        }
    }
    remaining_count = (editor->configuration.layoutCount - (layoutIndex + 1));
    has_remaining  = (0 < remaining_count);
    if (has_remaining)
    {
        (void)memmove(
            &editor->layouts[layoutIndex],
            &editor->layouts[layoutIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->layouts[0])));
    }
    editor->configuration.layoutCount -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes child layouts and windows before compacting screen indices.
 */
static void RemoveScreen(Arinc661_Configuration_Document_t* editor, uint32_t screenIndex)
{
    uint32_t layout_index;
    uint32_t remaining_count;
    bool_t   has_remaining;

    LOGASSERT(screenIndex < editor->configuration.screenCount);
    layout_index = 0;
    while (layout_index < editor->configuration.layoutCount)
    {
        if (screenIndex == editor->layouts[layout_index].screenIndex)
        {
            RemoveLayout(editor, layout_index);
        }
        else
        {
            if (screenIndex < editor->layouts[layout_index].screenIndex)
            {
                editor->layouts[layout_index].screenIndex -= 1;
            }
            layout_index += 1;
        }
    }
    remaining_count = (editor->configuration.screenCount - (screenIndex + 1));
    has_remaining  = (0 < remaining_count);
    if (has_remaining)
    {
        (void)memmove(
            &editor->screens[screenIndex],
            &editor->screens[screenIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->screens[0])));
    }
    editor->configuration.screenCount -= 1;
    UpdatePreviewMeasurements(editor);
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
 *  Iterates the typed parent indices and applies requested additions or removals after traversal.
 */
static void DrawHierarchy(Arinc661_Configuration_Document_t* editor)
{
    const FACEOE_DisplayManagement_Screen_t*          screen;
    const FACEOE_DisplayManagement_Layout_t*          layout;
    const FACEOE_DisplayManagement_Window_t*          window;
    const FACEOE_DisplayManagement_UserApplication_t* application;
    ImGuiTreeNodeFlags                    flags;
    float32_t                             row_height;
    uint32_t                              screen_index;
    uint32_t                              layout_index;
    uint32_t                              window_index;
    uint32_t                              application_index;
    uint32_t                              parent_index;
    uint32_t                              add_layout_screen_index;
    uint32_t                              add_window_layout_index;
    uint32_t                              remove_screen_index;
    uint32_t                              remove_layout_index;
    uint32_t                              remove_window_index;
    uint32_t                              remove_application_index;
    uint32_t                              remove_source_index;
    uint32_t                              source_index;
    bool_t                                is_screen_open;
    bool_t                                is_layout_open;
    bool_t                       is_open;

    add_layout_screen_index  = INVALID_HIERARCHY_INDEX;
    add_window_layout_index  = INVALID_HIERARCHY_INDEX;
    remove_screen_index      = INVALID_HIERARCHY_INDEX;
    remove_layout_index      = INVALID_HIERARCHY_INDEX;
    remove_window_index      = INVALID_HIERARCHY_INDEX;
    remove_application_index = INVALID_HIERARCHY_INDEX;
    remove_source_index      = INVALID_HIERARCHY_INDEX;
    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        screen = &editor->screens[screen_index];
        flags  = HIERARCHY_BRANCH_FLAGS;
        if ((SELECTION_SCREEN == editor->selectedKind) && (screen_index == editor->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushID(screen);
        is_screen_open = ImGui::TreeNodeEx(SCREEN_NODE_ID, flags, SCREEN_LABEL_FORMAT, screen->id);
        if (ImGui::IsItemClicked())
        {
            editor->selectedKind  = SELECTION_SCREEN;
            editor->selectedIndex = screen_index;
        }
        row_height = ImGui::GetItemRectSize().y;
        if (DrawRemoveButton(REMOVE_SCREEN_LABEL, row_height))
        {
            remove_screen_index = screen_index;
        }
        ImGui::PopID();
        if (is_screen_open)
        {
            for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
            {
                layout = &editor->layouts[layout_index];
                if (screen_index == layout->screenIndex)
                {
                    flags = HIERARCHY_BRANCH_FLAGS;
                    if ((SELECTION_LAYOUT == editor->selectedKind) && (layout_index == editor->selectedIndex))
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    ImGui::PushID(layout);
                    is_layout_open = ImGui::TreeNodeEx(LAYOUT_NODE_ID, flags, LAYOUT_LABEL_FORMAT, layout->id);
                    if (ImGui::IsItemClicked())
                    {
                        editor->selectedKind  = SELECTION_LAYOUT;
                        editor->selectedIndex = layout_index;
                    }
                    row_height = ImGui::GetItemRectSize().y;
                    if (DrawRemoveButton(REMOVE_LAYOUT_LABEL, row_height))
                    {
                        remove_layout_index = layout_index;
                    }
                    ImGui::PopID();
                    if (is_layout_open)
                    {
                        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                        {
                            window = &editor->windows[window_index];
                            if (layout_index == window->layoutIndex)
                            {
                                flags = HIERARCHY_LEAF_FLAGS;
                                if ((SELECTION_WINDOW == editor->selectedKind) && (window_index == editor->selectedIndex))
                                {
                                    flags |= ImGuiTreeNodeFlags_Selected;
                                }
                                ImGui::PushID(window);
                                (void)ImGui::TreeNodeEx(WINDOW_NODE_ID, flags, WINDOW_LABEL_FORMAT, window->id, window->name);
                                if (ImGui::IsItemClicked())
                                {
                                    editor->selectedKind  = SELECTION_WINDOW;
                                    editor->selectedIndex = window_index;
                                }
                                row_height = ImGui::GetItemRectSize().y;
                                if (DrawRemoveButton(REMOVE_WINDOW_LABEL, row_height))
                                {
                                    remove_window_index = window_index;
                                }
                                ImGui::PopID();
                            }
                        }
                        (void)ImGui::TreeNodeEx(ADD_WINDOW_LABEL, HIERARCHY_ADD_FLAGS);
                        if (ImGui::IsItemClicked())
                        {
                            add_window_layout_index = layout_index;
                        }
                        ImGui::TreePop();
                    }
                }
            }
            (void)ImGui::TreeNodeEx(ADD_LAYOUT_LABEL, HIERARCHY_ADD_FLAGS);
            if (ImGui::IsItemClicked())
            {
                add_layout_screen_index = screen_index;
            }
            ImGui::TreePop();
        }
    }
    (void)ImGui::TreeNodeEx(ADD_SCREEN_LABEL, HIERARCHY_ADD_FLAGS);
    if (ImGui::IsItemClicked())
    {
        AddScreen(editor);
    }
    ImGui::Separator();
    for (application_index = 0; application_index < editor->configuration.userApplicationCount; ++application_index)
    {
        application = &editor->userApplications[application_index];
        flags       = HIERARCHY_LEAF_FLAGS;
        if ((SELECTION_USER_APPLICATION == editor->selectedKind) && (application_index == editor->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushID(application);
        flags = HIERARCHY_BRANCH_FLAGS;
        if ((SELECTION_USER_APPLICATION == editor->selectedKind) && (application_index == editor->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if ('\0' != application->dFPath[0])
        {
            is_open = ImGui::TreeNodeEx(USER_APPLICATION_NODE_ID, flags, APPLICATION_TREE_FORMAT, application->applicationId, application->dFPath);
        }
        else
        {
            is_open = ImGui::TreeNodeEx(USER_APPLICATION_NODE_ID, flags, APPLICATION_NO_FILE_FORMAT, application->applicationId);
        }
        if (ImGui::IsItemClicked())
        {
            editor->selectedKind  = SELECTION_USER_APPLICATION;
            editor->selectedIndex = application_index;
        }
        row_height = ImGui::GetItemRectSize().y;
        if (DrawRemoveButton(REMOVE_USER_APPLICATION_LABEL, row_height))
        {
            remove_application_index = application_index;
        }
        if (is_open)
        {
            DrawEmbeddedDefinition(editor, application);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    (void)ImGui::TreeNodeEx(ADD_USER_APPLICATION_LABEL, HIERARCHY_ADD_FLAGS);
    if (ImGui::IsItemClicked())
    {
        AddUserApplication(editor);
    }
    ImGui::Separator();
    for (source_index = 0; source_index < editor->configuration.externalSourceCount; ++source_index)
    {
        flags = HIERARCHY_LEAF_FLAGS;
        if ((SELECTION_EXTERNAL_SOURCE == editor->selectedKind) && (source_index == editor->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushID(&editor->externalSources[source_index]);
        (void)ImGui::TreeNodeEx(EXTERNAL_SOURCE_NODE_ID, flags, EXTERNAL_SOURCE_LABEL_FORMAT, (unsigned)editor->externalSources[source_index].id, editor->externalSources[source_index].name);
        if (ImGui::IsItemClicked())
        {
            editor->selectedKind  = SELECTION_EXTERNAL_SOURCE;
            editor->selectedIndex = source_index;
        }
        row_height = ImGui::GetItemRectSize().y;
        if (DrawRemoveButton(REMOVE_EXTERNAL_SOURCE_LABEL, row_height))
        {
            remove_source_index = source_index;
        }
        ImGui::PopID();
    }
    (void)ImGui::TreeNodeEx(ADD_EXTERNAL_SOURCE_LABEL, HIERARCHY_ADD_FLAGS);
    if (ImGui::IsItemClicked())
    {
        AddExternalSource(editor);
    }
    if (INVALID_HIERARCHY_INDEX != remove_screen_index)
    {
        RemoveScreen(editor, remove_screen_index);
        editor->selectedKind  = SELECTION_NONE;
        editor->selectedIndex = 0;
        editor->isDirty       = TRUE;
        editor->status        = STATUS_SCREEN_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != remove_layout_index)
    {
        parent_index = editor->layouts[remove_layout_index].screenIndex;
        RemoveLayout(editor, remove_layout_index);
        editor->selectedKind  = SELECTION_SCREEN;
        editor->selectedIndex = parent_index;
        editor->isDirty       = TRUE;
        editor->status        = STATUS_LAYOUT_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != remove_window_index)
    {
        parent_index = editor->windows[remove_window_index].layoutIndex;
        RemoveWindow(editor, remove_window_index);
        editor->selectedKind  = SELECTION_LAYOUT;
        editor->selectedIndex = parent_index;
        editor->isDirty       = TRUE;
        editor->status        = STATUS_WINDOW_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != remove_application_index)
    {
        RemoveUserApplication(editor, remove_application_index);
        editor->selectedKind  = SELECTION_NONE;
        editor->selectedIndex = 0;
        editor->isDirty       = TRUE;
        editor->status        = STATUS_USER_APPLICATION_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != remove_source_index)
    {
        RemoveExternalSource(editor, remove_source_index);
        editor->selectedKind  = SELECTION_NONE;
        editor->selectedIndex = 0;
        editor->isDirty       = TRUE;
        editor->status        = STATUS_EXTERNAL_SOURCE_REMOVED;
    }
    else if (INVALID_HIERARCHY_INDEX != add_layout_screen_index)
    {
        AddLayout(editor, add_layout_screen_index);
    }
    else if (INVALID_HIERARCHY_INDEX != add_window_layout_index)
    {
        AddWindow(editor, add_window_layout_index);
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
    label_size     = ImGui::CalcTextSize(MAP_SOURCE_LAYER_LABEL);
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
 *  Draws a label and a signed numeric input in the current two-column table.
 */
static bool_t DrawIntegerField(const char_t* label, int64_t* value)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputScalar(PROPERTY_INPUT_ID, ImGuiDataType_S64, value);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The input edits a temporary value so a duplicate never enters the document.
 */
static bool_t DrawIdentifierField(Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, uint32_t itemIndex, int64_t* value)
{
    int64_t candidate;
    bool_t  has_changed;
    bool_t  is_unique;
    bool_t  was_updated;

    candidate   = (*value);
    has_changed = DrawIntegerField(ID_LABEL, &candidate);
    was_updated = FALSE;
    if (has_changed && (candidate != (*value)))
    {
        is_unique = IsIdentifierUnique(editor, kind, screenIndex, itemIndex, candidate);
        if (is_unique)
        {
            (*value)    = candidate;
            was_updated = TRUE;
        }
        else
        {
            switch (kind)
            {
                case SELECTION_SCREEN:           editor->status = STATUS_DUPLICATE_SCREEN_ID; break;
                case SELECTION_LAYOUT:           editor->status = STATUS_DUPLICATE_LAYOUT_ID; break;
                case SELECTION_WINDOW:           editor->status = STATUS_DUPLICATE_WINDOW_ID; break;
                case SELECTION_USER_APPLICATION: editor->status = STATUS_DUPLICATE_APPLICATION_ID; break;
                case SELECTION_EXTERNAL_SOURCE:  editor->status = STATUS_DUPLICATE_EXTERNAL_SOURCE_ID; break;
                case SELECTION_NONE:
                default:                         LOGFATAL("Invalid SelectionKind_t!");
            }
        }
    }

    return was_updated;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws a label and a floating-point input in the current two-column table.
 */
static bool_t DrawDecimalField(const char_t* label, float32_t* value)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputFloat(PROPERTY_INPUT_ID, value, 0.0f, 0.0f, "%.6g");
    ImGui::PopID();

    return has_changed;
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
 *  Converts the stored bool_t value for ImGui's bool checkbox interface.
 */
static bool_t DrawBooleanField(const char_t* label, bool_t* value)
{
    bool   is_checked;
    bool_t has_changed;

    is_checked = (FALSE != (*value));
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    has_changed = ImGui::Checkbox(PROPERTY_INPUT_ID, &is_checked);
    ImGui::PopID();
    if (has_changed)
    {
        (*value) = is_checked;
    }

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Restricts the choice to the FACE Units enumeration.
 */
static bool_t DrawUnitField(const char_t* label, FACEOE_DisplayManagement_Unit_t* unit)
{
    uint32_t unit_index;
    bool_t   is_selected;
    bool_t   has_changed;

    has_changed = FALSE;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo(PROPERTY_INPUT_ID, c_unitNames[(uint32_t)(*unit)]))
    {
        for (unit_index = 0; unit_index < COUNT_OF(c_unitNames); ++unit_index)
        {
            is_selected = (unit_index == (uint32_t)(*unit));
            if (ImGui::Selectable(c_unitNames[unit_index], is_selected))
            {
                if (FALSE == is_selected)
                {
                    (*unit)     = (FACEOE_DisplayManagement_Unit_t)unit_index;
                    has_changed = TRUE;
                }
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
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
 *  A new field edit replaces any stale save or load message in the status bar.
 */
static void MarkDocumentDirty(Arinc661_Configuration_Document_t* editor, bool_t hasChanged)
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
 *  Screen pixel and physical dimensions remain grouped under their screen selection.
 */
static void DrawScreenInspector(Arinc661_Configuration_Document_t* editor)
{
    char_t   layout_label[LABEL_FORMAT_BYTES];
    uint32_t layout_index;
    uint32_t window_index;
    uint32_t window_count;
    uint32_t layout_count;
    FACEOE_DisplayManagement_Screen_t* screen;
    bool_t                 has_width_changed;
    bool_t                 has_height_changed;
    bool_t                 has_pixel_size_changed;

    screen = &editor->screens[editor->selectedIndex];

    ImGui::SeparatorText(SCREEN_SECTION_LABEL);
    if (BeginPropertyTable(SCREEN_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIdentifierField(editor, SELECTION_SCREEN, 0, editor->selectedIndex, &screen->id));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(PIXEL_SIZE_SECTION_LABEL);
    if (BeginPropertyTable(PIXEL_SIZE_TABLE_ID))
    {
        has_width_changed      = DrawIntegerField(WIDTH_LABEL, &screen->pixelSize.width);
        has_height_changed     = DrawIntegerField(HEIGHT_LABEL, &screen->pixelSize.height);
        has_pixel_size_changed = !!(has_width_changed || has_height_changed);
        MarkDocumentDirty(editor, has_pixel_size_changed);
        MarkDocumentDirty(editor, DrawUnitField(UNITS_LABEL, &screen->pixelSize.units));
        ImGui::EndTable();
        if (has_pixel_size_changed)
        {
            UpdatePreviewMeasurements(editor);
        }
    }
    ImGui::Separator();
    ImGui::TextUnformatted(PHYSICAL_DIMENSIONS_SECTION_LABEL);
    if (BeginPropertyTable(PHYSICAL_DIMENSIONS_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIntegerField(WIDTH_LABEL, &screen->physicalDimensions.width));
        MarkDocumentDirty(editor, DrawIntegerField(HEIGHT_LABEL, &screen->physicalDimensions.height));
        MarkDocumentDirty(editor, DrawUnitField(UNITS_LABEL, &screen->physicalDimensions.units));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::SeparatorText(SCREEN_LAYOUTS_LABEL);
    layout_count = 0;
    for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
    {
        if (editor->layouts[layout_index].screenIndex == editor->selectedIndex)
        {
            layout_count += 1;
            window_count = 0;
            for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
            {
                if (editor->windows[window_index].layoutIndex == layout_index)
                {
                    window_count += 1;
                }
            }
            ImFormatString(layout_label, sizeof(layout_label), LAYOUT_ITEM_FORMAT, editor->layouts[layout_index].id, (unsigned)window_count);
            ImGui::PushID((int)layout_index);
            if (ImGui::Selectable(layout_label))
            {
                editor->selectedKind  = SELECTION_LAYOUT;
                editor->selectedIndex = layout_index;
            }
            ImGui::PopID();
        }
    }
    if (0 == layout_count)
    {
        ImGui::TextDisabled("%s", LAYOUT_NO_WINDOWS_LABEL);
    }
    if (ImGui::SmallButton(ADD_LAYOUT_HERE_LABEL))
    {
        AddLayout(editor, editor->selectedIndex);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layouts have an identifier; their screen relationship is shown by the hierarchy.
 */
static void DrawLayoutInspector(Arinc661_Configuration_Document_t* editor)
{
    FACEOE_DisplayManagement_Layout_t* layout;
    char_t                             label[FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES + (2 * LABEL_FORMAT_BYTES)];
    uint32_t                           window_index;
    uint32_t                           count;

    layout = &editor->layouts[editor->selectedIndex];

    ImGui::SeparatorText(LAYOUT_SECTION_LABEL);
    if (BeginPropertyTable(LAYOUT_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIdentifierField(editor, SELECTION_LAYOUT, layout->screenIndex, editor->selectedIndex, &layout->id));
        MarkDocumentDirty(editor, DrawScreenChoiceField(editor, SCREEN_LABEL, &layout->screenIndex));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::SeparatorText(LAYOUT_WINDOWS_LABEL);
    count = 0;
    for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
    {
        if (editor->windows[window_index].layoutIndex == editor->selectedIndex)
        {
            count += 1;
            ImFormatString(label, sizeof(label), LAYOUT_WINDOW_FORMAT, editor->windows[window_index].id, editor->windows[window_index].name, editor->windows[window_index].pixelArea.width, editor->windows[window_index].pixelArea.height, editor->windows[window_index].pixelArea.x, editor->windows[window_index].pixelArea.y);
            ImGui::PushID((int)window_index);
            if (ImGui::Selectable(label))
            {
                editor->selectedKind  = SELECTION_WINDOW;
                editor->selectedIndex = window_index;
            }
            ImGui::PopID();
        }
    }
    if (0 == count)
    {
        ImGui::TextDisabled("%s", LAYOUT_NO_WINDOWS_LABEL);
    }
    if (ImGui::SmallButton(ADD_WINDOW_HERE_LABEL))
    {
        AddWindow(editor, editor->selectedIndex);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Window geometry and scaling values are edited as typed fields.
 */
static void DrawWindowInspector(Arinc661_Configuration_Document_t* editor)
{
    FACEOE_DisplayManagement_Window_t* window;

    window = &editor->windows[editor->selectedIndex];

    ImGui::SeparatorText(WINDOW_SECTION_LABEL);
    if (BeginPropertyTable(WINDOW_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIdentifierField(editor, SELECTION_WINDOW, 0, editor->selectedIndex, &window->id));
        MarkDocumentDirty(editor, DrawTextField(NAME_LABEL, window->name, sizeof(window->name)));
        MarkDocumentDirty(editor, DrawLayoutChoiceField(editor, LAYOUT_LABEL, &window->layoutIndex));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(PIXEL_AREA_SECTION_LABEL);
    if (BeginPropertyTable(PIXEL_AREA_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIntegerField(X_LABEL, &window->pixelArea.x));
        MarkDocumentDirty(editor, DrawIntegerField(Y_LABEL, &window->pixelArea.y));
        MarkDocumentDirty(editor, DrawIntegerField(WIDTH_LABEL, &window->pixelArea.width));
        MarkDocumentDirty(editor, DrawIntegerField(HEIGHT_LABEL, &window->pixelArea.height));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(SCALING_FACTOR_SECTION_LABEL);
    if (BeginPropertyTable(SCALING_FACTOR_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawDecimalField(X_SCALE_LABEL, &window->scalingFactor.xScale));
        MarkDocumentDirty(editor, DrawDecimalField(Y_SCALE_LABEL, &window->scalingFactor.yScale));
        MarkDocumentDirty(editor, DrawUnitField(BASE_UNIT_LABEL, &window->scalingFactor.baseUnit));
        MarkDocumentDirty(editor, DrawUnitField(PER_UNIT_LABEL, &window->scalingFactor.perUnit));
        ImGui::EndTable();
    }
    DrawWindowApplications(editor, window);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Lists the user applications whose window attribute names this window, with their
 *  Definition Files, and selects one on click.
 */
static void DrawWindowApplications(Arinc661_Configuration_Document_t* editor, const FACEOE_DisplayManagement_Window_t* window)
{
    char_t   label[FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES + LABEL_FORMAT_BYTES];
    uint32_t application_index;
    uint32_t count;

    ImGui::Separator();
    ImGui::SeparatorText(USER_APPLICATION_SECTION_LABEL);
    count = 0;
    for (application_index = 0; application_index < editor->configuration.userApplicationCount; ++application_index)
    {
        if (editor->userApplications[application_index].window == window->id)
        {
            count += 1;
            ImFormatString(label, sizeof(label), WINDOW_APPLICATION_FORMAT, (long long)editor->userApplications[application_index].applicationId, editor->userApplications[application_index].dFPath);
            ImGui::PushID((int)application_index);
            if (ImGui::Selectable(label))
            {
                editor->selectedKind  = SELECTION_USER_APPLICATION;
                editor->selectedIndex = application_index;
            }
            ImGui::PopID();
        }
    }
    if (0 == count)
    {
        ImGui::TextDisabled("%s", WINDOW_NO_APPLICATION_LABEL);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  User application attributes are edited in their stored order.
 */
static void DrawExternalSourceInspector(Arinc661_Configuration_Document_t* editor)
{
    FACEOE_DisplayManagement_ExternalSource_t* source;
    int64_t                                    id;

    source = &editor->externalSources[editor->selectedIndex];
    ImGui::SeparatorText(EXTERNAL_SOURCE_SECTION_LABEL);
    if (BeginPropertyTable(EXTERNAL_SOURCE_TABLE_ID))
    {
        id = (int64_t)source->id;
        if (DrawIdentifierField(editor, SELECTION_EXTERNAL_SOURCE, 0, editor->selectedIndex, &id))
        {
            source->id = (uint32_t)id;
            MarkDocumentDirty(editor, TRUE);
        }
        MarkDocumentDirty(editor, DrawTextField(NAME_LABEL, source->name, sizeof(source->name)));
        MarkDocumentDirty(editor, DrawTextField(EXTERNAL_TYPE_LABEL, source->type, sizeof(source->type)));
        ImGui::EndTable();
    }
    ImGui::TextDisabled("%s", EXTERNAL_SOURCE_HINT_LABEL);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  User application attributes are edited in their stored order.
 */
static void DrawUserApplicationInspector(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document)
{
    FACEOE_DisplayManagement_UserApplication_t* application;

    application = &document->userApplications[document->selectedIndex];

    ImGui::SeparatorText(USER_APPLICATION_SECTION_LABEL);
    if (BeginPropertyTable(USER_APPLICATION_TABLE_ID))
    {
        MarkDocumentDirty(document, DrawIdentifierField(document, SELECTION_USER_APPLICATION, 0, document->selectedIndex, &application->applicationId));
        // Offer only windows of this document and Definition Files of the folder
        MarkDocumentDirty(document, DrawWindowChoiceField(document, WINDOW_ID_LABEL, &application->window));
        MarkDocumentDirty(document, DrawDefinitionChoiceField(editor, DF_PATH_LABEL, application->dFPath, sizeof(application->dFPath)));
        MarkDocumentDirty(document, DrawTextField(STYLE_FILE_PATH_LABEL, application->styleFilePath, sizeof(application->styleFilePath)));
        MarkDocumentDirty(document, DrawBooleanField(MAP_SOURCE_LAYER_LABEL, &application->mapSourceLayer));
        MarkDocumentDirty(document, DrawBooleanField(VISIBLE_LABEL, &application->visible));
        ImGui::EndTable();
    }
    DrawDefinitionState(editor, document, application);
    DrawDefinitionLayers(editor, document, application);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An open file shows its whole layer and widget tree, editable in place: a click inside
 *  selects in the Definition File editor and moves the focus there. A closed file in the
 *  folder opens on click; an unknown name is reported.
 */
static void DrawEmbeddedDefinition(Arinc661_Configuration_Document_t* document, const FACEOE_DisplayManagement_UserApplication_t* application)
{
    Arinc661_Context_t*           context;
    Arinc661_Context_Definition_t definition;
    char_t                        label[FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES + LABEL_FORMAT_BYTES];

    context = document->owner->context;
    if ((NULL != context) && ('\0' != application->dFPath[0]))
    {
        if (FALSE == Arinc661_Context_FindDefinition(context, application->dFPath, &definition))
        {
            ImFormatString(label, sizeof(label), DEFINITION_MISSING_NODE_FORMAT, application->dFPath);
            ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
            ImGui::BulletText("%s", label);
            ImGui::PopStyleColor();
        }
        else if (FALSE == definition.isOpen)
        {
            ImFormatString(label, sizeof(label), DEFINITION_CLOSED_NODE_FORMAT, application->dFPath);
            if (ImGui::Selectable(label))
            {
                Arinc661_Definition_LoadFile(context->definitionEditor, application->dFPath);
            }
        }
        else
        {
            if (Arinc661_Definition_DrawDocumentHierarchy(context->definitionEditor, definition.documentIndex))
            {
                context->isDefinitionFocused = TRUE;
                context->hasFocusRequest     = TRUE;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each layer row shows its id, name, and size against the window; a click selects the
 *  layer in the Definition File editor.
 */
static void DrawDefinitionLayers(Arinc661_Configuration_Editor_t* editor, const Arinc661_Configuration_Document_t* document, const FACEOE_DisplayManagement_UserApplication_t* application)
{
    Arinc661_Context_Definition_t definition;
    const char_t*                 layer_id;
    const char_t*                 layer_name;
    char_t                        label[LABEL_FORMAT_BYTES * 3];
    float32_t                     layer_width;
    float32_t                     layer_height;
    float32_t                     units_per_pixel_x;
    float32_t                     units_per_pixel_y;
    int64_t                       pixel_width;
    int64_t                       pixel_height;
    uint32_t                      layer_index;
    bool_t                        has_window;
    bool_t                        is_too_big;

    if ((NULL != editor->context) && Arinc661_Context_FindDefinition(editor->context, application->dFPath, &definition) && definition.isOpen)
    {
        ImGui::SeparatorText(LAYERS_SECTION_LABEL);
        has_window = Arinc661_Configuration_Window(editor, (uint32_t)(document - editor->documents), application->window, NULL, &pixel_width, &pixel_height, &units_per_pixel_x, &units_per_pixel_y);
        for (layer_index = 0; Arinc661_Definition_LayerAt(editor->context->definitionEditor, definition.documentIndex, layer_index, &layer_id, &layer_name, &layer_width, &layer_height); ++layer_index)
        {
            ImFormatString(label, sizeof(label), LAYER_ROW_FORMAT, layer_id, layer_name, (float64_t)layer_width, (float64_t)layer_height);
            is_too_big = !!(has_window && ((layer_width > ((float32_t)pixel_width * units_per_pixel_x * 1.001f)) || (layer_height > ((float32_t)pixel_height * units_per_pixel_y * 1.001f))));
            if (is_too_big)
            {
                ImStrncpy(&label[strlen(label)], LAYER_FIT_WARNING, (int)(sizeof(label) - strlen(label)));
                ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
            }
            ImGui::PushID((int)layer_index);
            if (ImGui::Selectable(label))
            {
                Arinc661_Definition_SelectLayer(editor->context->definitionEditor, definition.documentIndex, layer_index);
                editor->context->isDefinitionFocused = TRUE;
                editor->context->hasFocusRequest     = TRUE;
            }
            ImGui::PopID();
            if (is_too_big)
            {
                ImGui::PopStyleColor();
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Hands the selected user application to the context, which creates a Definition File named
 *  after its dFPath with its applicationId and a layer sized to its window.
 */
static void CreateDefinitionForSelection(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document)
{
    Arinc661_Context_UserApplication_t application;
    uint32_t                           document_index;

    document_index = (uint32_t)(document - editor->documents);
    (void)memset(&application, 0, sizeof(application));
    application.documentIndex    = document_index;
    application.applicationIndex = document->selectedIndex;
    application.documentName     = document->documentName;
    application.applicationId    = document->userApplications[document->selectedIndex].applicationId;
    application.windowId         = document->userApplications[document->selectedIndex].window;
    application.dFPath           = document->userApplications[document->selectedIndex].dFPath;
    if (Arinc661_Context_CreateDefinitionFor(editor->context, &application) && ('\0' == document->userApplications[document->selectedIndex].dFPath[0]))
    {
        ImFormatString(document->userApplications[document->selectedIndex].dFPath, sizeof(document->userApplications[document->selectedIndex].dFPath), "application_%lld.xml", (long long)application.applicationId);
        document->isDirty = TRUE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Asks the context what it knows about the user application's Definition File: presence in
 *  the folder, whether it is open, its layer and widget counts, and whether its ApplicationId
 *  agrees with the FACE applicationId. Offers to open it and to adopt its identifier.
 */
static void DrawDefinitionState(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document, FACEOE_DisplayManagement_UserApplication_t* application)
{
    Arinc661_Context_Definition_t definition;
    char_t                        text[LABEL_FORMAT_BYTES * 2];

    if (NULL != editor->context)
    {
        ImGui::Separator();
        ImGui::SeparatorText(DEFINITION_SECTION_LABEL);
        if ('\0' == application->dFPath[0])
        {
            ImGui::TextDisabled("%s", DEFINITION_EMPTY_LABEL);
            if (ImGui::SmallButton(CREATE_DEFINITION_LABEL))
            {
                CreateDefinitionForSelection(editor, document);
            }
        }
        else if (FALSE == Arinc661_Context_FindDefinition(editor->context, application->dFPath, &definition))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
            ImGui::TextWrapped("%s", DEFINITION_MISSING_LABEL);
            ImGui::PopStyleColor();
            if (ImGui::SmallButton(CREATE_DEFINITION_LABEL))
            {
                CreateDefinitionForSelection(editor, document);
            }
        }
        else
        {
            if (definition.isOpen)
            {
                if (definition.hasApplicationId)
                {
                    ImFormatString(text, sizeof(text), DEFINITION_STATE_FORMAT, (unsigned)definition.layerCount, (unsigned)definition.widgetCount, (long long)definition.applicationId);
                }
                else
                {
                    ImFormatString(text, sizeof(text), DEFINITION_NO_ID_FORMAT, (unsigned)definition.layerCount, (unsigned)definition.widgetCount);
                }
                ImGui::TextWrapped("%s", text);
                if (definition.hasApplicationId && (definition.applicationId != application->applicationId))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, INVALID_VALUE_COLOR);
                    ImGui::TextWrapped("%s", APPLICATION_ID_MISMATCH_LABEL);
                    ImGui::PopStyleColor();
                    if (ImGui::SmallButton(ADOPT_DEFINITION_ID_LABEL))
                    {
                        application->applicationId = definition.applicationId;
                        MarkDocumentDirty(document, TRUE);
                    }
                }
            }
            else
            {
                ImGui::TextWrapped("%s", DEFINITION_CLOSED_LABEL);
            }
            if (ImGui::SmallButton(SHOW_DEFINITION_LABEL))
            {
                Arinc661_Context_ShowDefinition(editor->context, application->dFPath);
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Filters regular files by extension before drawing a selectable row.
 */
static void DrawXmlFileEntry(const char_t* fileName, void* context)
{
    XmlFileListContext_t* list_context;
    size_t                file_name_length;
    size_t                extension_length;
    bool_t                is_xml_file;

    list_context     = (XmlFileListContext_t*)context;
    file_name_length = ImStrlen(fileName);
    extension_length = ImStrlen(XML_EXTENSION);
    is_xml_file      = (extension_length <= file_name_length);
    if (is_xml_file)
    {
        is_xml_file = (0 == ImStricmp(fileName + file_name_length - extension_length, XML_EXTENSION));
    }
    if (is_xml_file)
    {
        is_xml_file = (file_name_length < ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES);
    }
    if (is_xml_file)
    {
        if (list_context->shouldDraw)
        {
            if (ImGui::Selectable(fileName))
            {
                LoadConfigurationFile(list_context->editor, fileName);
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
static void DrawXmlFileList(Arinc661_Configuration_Editor_t* editor, float32_t folderInputX)
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
static void DrawFileControls(Arinc661_Configuration_Editor_t* editor)
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
 *  Returns the fixed slot to the available pool without releasing its allocated workspace.
 */
static void CloseDocument(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           index;

    document = &editor->documents[documentIndex];

    document->status              = STATUS_NONE;
    document->isLoaded            = FALSE;
    document->isDirty             = FALSE;
    document->selectedKind        = SELECTION_NONE;
    document->selectedIndex       = 0;
    document->totalScreenWidth    = 0.0f;
    document->maximumScreenHeight = 0.0f;
    document->drawableScreenCount = 0;
    document->documentName[0]     = '\0';
    document->documentPath[0]     = '\0';

    document->configuration.screenCount          = 0;
    document->configuration.layoutCount          = 0;
    document->configuration.windowCount          = 0;
    document->configuration.userApplicationCount = 0;
    document->configuration.externalSourceCount  = 0;
    document->configuration.hasMetaData          = FALSE;
    document->configuration.hasXmlDeclaration    = FALSE;
    document->configuration.schemaLocation[0]    = '\0';
    if (documentIndex == editor->activeDocumentIndex)
    {
        editor->activeDocumentIndex    = INVALID_DOCUMENT_INDEX;
        editor->requestedDocumentIndex = INVALID_DOCUMENT_INDEX;
        for (index = 0; index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++index)
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
static void DrawFileMenuBar(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           document_index;
    bool_t                             has_active_document;
    bool_t                             has_dirty_documents;
    bool_t                             has_available_document;

    has_active_document    = (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex);
    has_dirty_documents    = FALSE;
    has_available_document = FALSE;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
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
static void DrawDocumentTabs(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    ImGuiTabItemFlags                   tab_flags;
    ImVec2                              popup_center;
    char_t                              tab_label[DOCUMENT_TAB_LABEL_BYTES];
    uint32_t                            document_index;
    uint32_t                            document_to_close;
    bool                                is_tab_open;
    bool                                is_tab_visible;
    bool_t                              has_available_document;
    bool_t                              has_pending_close;
    bool_t                              has_pending_save;
    bool_t                              should_create_document;

    has_available_document = FALSE;
    should_create_document = FALSE;
    document_to_close      = INVALID_DOCUMENT_INDEX;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        if (FALSE == editor->documents[document_index].isLoaded)
        {
            has_available_document = TRUE;
        }
    }
    if (ImGui::BeginTabBar(DOCUMENT_TAB_BAR_ID))
    {
        for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
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
static void DrawSaveAsDialog(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    FileMatchContext_t                 match_context;
    const char_t*                      status_text;
    char_t                             file_path[ARINC661_CONFIGURATION_MAX_FILE_PATH_BYTES];
    char_t                             previous_name[ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES];
    uint32_t                           document_index;
    uint32_t                           open_document_index;
    bool_t                             has_folder_path;
    bool_t                             has_file_name;
    bool_t                             has_valid_name;
    bool_t                             is_enumerated;
    bool_t                             is_already_open;
    bool_t                             was_save_all_pending;

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
                        for (open_document_index = 0; open_document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++open_document_index)
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
                            UnloadConfigurationFile(document);
                            if (document->isDirty)
                            {
                                editor->status           = document->status;
                                document->documentPath[0] = '\0';
                                ImStrncpy(document->documentName, previous_name, sizeof(document->documentName));
                            }
                            else
                            {
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
static void DrawStatusBar(const Arinc661_Configuration_Editor_t* editor)
{
    const Arinc661_Configuration_Document_t* document;
    const char_t* status_text;

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
static void DrawHierarchyPane(Arinc661_Configuration_Document_t* editor)
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
 *  Shows the selected item's typed fields in the right child pane.
 */
static void DrawInspectorPane(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document)
{
    if (ImGui::BeginChild(INSPECTOR_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        ImGui::PushTextWrapPos(0.0f);

        if (document->isLoaded)
        {
            if (SELECTION_SCREEN == document->selectedKind)
            {
                DrawScreenInspector(document);
            }
            else if (SELECTION_LAYOUT == document->selectedKind)
            {
                DrawLayoutInspector(document);
            }
            else if (SELECTION_WINDOW == document->selectedKind)
            {
                DrawWindowInspector(document);
            }
            else if (SELECTION_USER_APPLICATION == document->selectedKind)
            {
                DrawUserApplicationInspector(editor, document);
            }
            else if (SELECTION_EXTERNAL_SOURCE == document->selectedKind)
            {
                DrawExternalSourceInspector(document);
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
 *  Fits the combined screen widths and tallest screen inside the available canvas, selects clicked geometry, and draws the selection overlay.
 */
static void DrawScenePane(Arinc661_Configuration_Editor_t* owner, Arinc661_Configuration_Document_t* editor)
{
    const ImGuiStyle*            style;
    const FACEOE_DisplayManagement_Screen_t* screen;
    const FACEOE_DisplayManagement_Layout_t* layout;
    const FACEOE_DisplayManagement_Window_t* window;
    const char_t*                fmt;
    ImDrawList*                  draw_list;
    ImVec2                       canvas_min;
    ImVec2                       canvas_size;
    ImVec2                       canvas_max;
    ImVec2                       content_size;
    ImVec2                       screen_min;
    ImVec2                       screen_position;
    ImVec2                       screen_size;
    ImVec2                       screen_max;
    ImVec2                       screen_label_size;
    ImVec2                       screen_label_position;
    ImVec2                       area_offset;
    ImVec2                       area_size;
    ImVec2                       area_min;
    ImVec2                       area_max;
    ImVec2                       selected_window_min;
    ImVec2                       selected_window_max;
    ImVec2                       mouse_position;
    char_t                       screen_label[LABEL_FORMAT_BYTES];
    char_t                       window_label[FACEOE_DISPLAYMANAGEMENT_MAX_NAME_BYTES + LABEL_FORMAT_BYTES];
    float32_t                    label_height;
    float32_t                    unused_height;
    float32_t                    width_scale;
    float32_t                    height_scale;
    float32_t                    scale;
    uint32_t                     screen_index;
    uint32_t                     window_index;
    bool_t                       has_canvas_width;
    bool_t                       has_canvas_height;
    bool_t                       has_canvas_space;
    bool_t                       has_valid_layout;
    bool_t                       is_window_on_screen;
    bool_t                       has_screen_width;
    bool_t                       has_screen_height;
    bool_t                       is_drawable_screen;
    bool_t                       is_selected_screen;
    bool_t                       is_selected_layout_window;
    bool_t                       has_selected_window_area;
    bool_t                       is_canvas_clicked;
    bool_t                       is_screen_body_hit;
    bool_t                       is_screen_label_hit;
    bool_t                       is_screen_hit;
    bool_t                       is_window_hit;
    bool_t                       is_canvas_hovered;
    uint32_t                     hover_screen;
    uint32_t                     hover_window;

    ImVec2                       handle_min;
    ImVec2                       handle_max;
    bool_t                       has_handles;

    selected_window_min      = ImVec2(0.0f, 0.0f);
    selected_window_max      = ImVec2(0.0f, 0.0f);
    has_selected_window_area = FALSE;
    handle_min               = ImVec2(0.0f, 0.0f);
    handle_max               = ImVec2(0.0f, 0.0f);
    has_handles              = FALSE;
    if (ImGui::BeginChild(SCENE_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        style     = &ImGui::GetStyle();
        draw_list = ImGui::GetWindowDrawList();

        if (editor->isLoaded)
        {
            DrawSceneToolbar(owner, editor);
        }
        else
        {
            ImGui::TextUnformatted(SCENE_PANE_NAME);
        }
        ImGui::Separator();

        if (FALSE == editor->isLoaded)
        {
            ImGui::TextWrapped("%s", "No configuration loaded.");
        }
        else if (0 == editor->configuration.screenCount)
        {
            ImGui::TextWrapped("%s", EMPTY_SCENE_LABEL);
            if (ImGui::Button(EMPTY_SCREEN_BUTTON_LABEL))
            {
                AddScreen(editor);
            }
        }
        else if (0 == editor->drawableScreenCount)
        {
            ImGui::TextWrapped("%s", "No screens with positive pixel dimensions.");
        }
        else
        {
            canvas_min   = ImGui::GetCursorScreenPos();
            canvas_size  = ImGui::GetContentRegionAvail();
            label_height = ImGui::GetTextLineHeightWithSpacing();

            content_size = (canvas_size - (style->WindowPadding + style->WindowPadding));
            content_size.x -= (style->ItemSpacing.x * (float32_t)(editor->drawableScreenCount - 1));
            content_size.y -= 0.0f;

            has_canvas_width  = (0.0f < content_size.x);
            has_canvas_height = (0.0f < content_size.y);
            has_canvas_space  = !!(has_canvas_width && has_canvas_height);
            if (FALSE == has_canvas_space)
            {
                ImGui::TextWrapped("%s", "Preview area is too small.");
            }
            else
            {
                owner->previewBoundsCount = 0;
                (void)ImGui::InvisibleButton(SCENE_CANVAS_ID, canvas_size);
                is_canvas_clicked = ImGui::IsItemActivated();
                is_canvas_hovered = ImGui::IsItemHovered();
                hover_screen      = ARINC661_CONFIGURATION_MAX_SCREENS;
                hover_window      = ARINC661_CONFIGURATION_MAX_WINDOWS;
                if (is_canvas_clicked)
                {
                    editor->selectedKind  = SELECTION_NONE;
                    editor->selectedIndex = 0;
                }
                mouse_position = ImGui::GetMousePos();
                canvas_max = (canvas_min + canvas_size);
                draw_list->PushClipRect(canvas_min, canvas_max, TRUE);
                draw_list->AddRectFilled(canvas_min, canvas_max, SCENE_CANVAS_COLOR);
                width_scale   = (content_size.x / editor->totalScreenWidth);
                height_scale  = (content_size.y / editor->maximumScreenHeight);
                scale         = ImMin(width_scale, height_scale);
                unused_height = (content_size.y - (editor->maximumScreenHeight * scale));

                screen_position = (canvas_min + style->WindowPadding);
                screen_position.y += (unused_height / 2.0f);
                for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
                {
                    screen             = &editor->screens[screen_index];
                    has_screen_width   = (0 < screen->pixelSize.width);
                    has_screen_height  = (0 < screen->pixelSize.height);
                    is_drawable_screen = !!(has_screen_width && has_screen_height);
                    if (is_drawable_screen)
                    {
                        screen_min  = screen_position;
                        screen_size = ImVec2((float32_t)screen->pixelSize.width, (float32_t)screen->pixelSize.height);
                        screen_max  = (screen_min + (screen_size * scale));

                        draw_list->AddRectFilled(screen_min, screen_max, SCENE_SCREEN_COLOR);
                        fmt = SCREEN_PREVIEW_FORMAT;
                        ImFormatString(screen_label, sizeof(screen_label), fmt, screen->id, screen->pixelSize.width, screen->pixelSize.height);
                        screen_label_size       = ImGui::CalcTextSize(screen_label);
                        screen_label_position   = screen_min;
                        is_screen_body_hit      = FALSE;
                        if (ImRect(screen_min, screen_max).Contains(mouse_position))
                        {
                            hover_screen = screen_index;
                        }
                        if (is_canvas_clicked)
                        {
                            is_screen_body_hit  = ImRect(screen_min, screen_max).Contains(mouse_position);
                            is_screen_label_hit = FALSE;
                            is_screen_hit       = !!(is_screen_body_hit || is_screen_label_hit);
                            if (is_screen_hit)
                            {
                                editor->selectedKind  = SELECTION_SCREEN;
                                editor->selectedIndex = screen_index;
                            }
                        }
                        draw_list->PushClipRect(screen_min, screen_max, TRUE);
                        draw_list->AddRectFilled(screen_min, ImVec2(screen_max.x, screen_min.y + screen_label_size.y + style->FramePadding.y), SCENE_LABEL_BACK_COLOR);
                        draw_list->AddText(ImVec2(screen_min.x + style->FramePadding.x, screen_min.y + (style->FramePadding.y * 0.5f)), SCENE_TEXT_COLOR, screen_label);
                        if (ImRect(screen_min, ImVec2(screen_max.x, screen_min.y + screen_label_size.y + style->FramePadding.y)).Contains(mouse_position) && ((screen_max.x - screen_min.x) < (screen_label_size.x + (2.0f * style->FramePadding.x))))
                        {
                            ImGui::SetTooltip("%s", screen_label);
                        }
                        has_selected_window_area = FALSE;
                        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                        {
                            window              = &editor->windows[window_index];
                            has_valid_layout    = (window->layoutIndex < editor->configuration.layoutCount);
                            is_window_on_screen = FALSE;
                            if (has_valid_layout)
                            {
                                layout              = &editor->layouts[window->layoutIndex];
                                is_window_on_screen = (screen_index == layout->screenIndex);
                            }
                            if (is_window_on_screen)
                            {
                                area_offset = ImVec2((float32_t)window->pixelArea.x, (float32_t)window->pixelArea.y);
                                area_size   = ImVec2((float32_t)window->pixelArea.width, (float32_t)window->pixelArea.height);
                                area_min    = ImVec2(screen_min.x + (area_offset.x * scale), screen_max.y - ((area_offset.y + area_size.y) * scale));
                                area_max    = ImVec2(area_min.x + (area_size.x * scale), screen_max.y - (area_offset.y * scale));

                                if (is_screen_body_hit)
                                {
                                    is_window_hit = ImRect(area_min, area_max).Contains(mouse_position);
                                    if (is_window_hit)
                                    {
                                        editor->selectedKind  = SELECTION_WINDOW;
                                        editor->selectedIndex = window_index;
                                    }
                                }
                                if (ImRect(area_min, area_max).Contains(mouse_position))
                                {
                                    hover_window = window_index;
                                }
                                is_selected_layout_window = FALSE;
                                if (SELECTION_LAYOUT == editor->selectedKind)
                                {
                                    is_selected_layout_window = (window->layoutIndex == editor->selectedIndex);
                                }
                                else if ((SELECTION_USER_APPLICATION == editor->selectedKind) && (editor->selectedIndex < editor->configuration.userApplicationCount))
                                {
                                    // The selected user application lights up the window it targets
                                    is_selected_layout_window = (editor->userApplications[editor->selectedIndex].window == window->id);
                                }
                                else if (SELECTION_WINDOW == editor->selectedKind)
                                {
                                    if (window_index == editor->selectedIndex)
                                    {
                                        selected_window_min      = area_min;
                                        selected_window_max      = area_max;
                                        has_selected_window_area = TRUE;
                                        handle_min               = area_min;
                                        handle_max               = area_max;
                                        has_handles              = TRUE;
                                    }
                                }

                                draw_list->AddRectFilled(area_min, area_max, SCENE_WINDOW_COLOR);
                                DrawWindowPreview(owner, editor, screen, window, area_min, area_max, scale, draw_list);
                                draw_list->AddRect(area_min, area_max, SCENE_WINDOW_BORDER_COLOR);
                                if ('\0' != window->name[0])
                                {
                                    fmt = NAMED_WINDOW_PREVIEW_FORMAT;
                                    ImFormatString(window_label, sizeof(window_label), fmt, window->name, window->pixelArea.width, window->pixelArea.height);
                                }
                                else
                                {
                                    fmt = WINDOW_PREVIEW_FORMAT;
                                    ImFormatString(window_label, sizeof(window_label), fmt, window->id, window->pixelArea.width, window->pixelArea.height);
                                }
                                draw_list->PushClipRect(ImVec2(ImMax(area_min.x, screen_min.x), ImMax(area_min.y, screen_min.y)), ImVec2(ImMin(area_max.x, screen_max.x), ImMin(area_max.y, screen_max.y)), TRUE);
                                draw_list->AddText(ImVec2(area_min.x + style->FramePadding.x, area_max.y - ImGui::GetTextLineHeight() - style->FramePadding.y), SCENE_TEXT_COLOR, window_label);
                                draw_list->PopClipRect();
                                if (ImRect(area_min, area_max).Contains(mouse_position) && ((area_max.x - area_min.x) < (ImGui::CalcTextSize(window_label).x + (2.0f * style->FramePadding.x))))
                                {
                                    ImGui::SetTooltip("%s", window_label);
                                }
                                if (is_selected_layout_window)
                                {
                                    draw_list->AddRectFilled(area_min, area_max, SCENE_SELECTION_OVERLAY_COLOR);
                                }
                            }
                        }
                        if (has_selected_window_area)
                        {
                            draw_list->AddRectFilled(selected_window_min, selected_window_max, SCENE_SELECTION_OVERLAY_COLOR);
                        }
                        is_selected_screen = (SELECTION_SCREEN == editor->selectedKind);
                        if (is_selected_screen)
                        {
                            is_selected_screen = (screen_index == editor->selectedIndex);
                        }
                        if (is_selected_screen)
                        {
                            draw_list->AddRectFilled(screen_min, screen_max, SCENE_SELECTION_OVERLAY_COLOR);
                        }
                        draw_list->PopClipRect();
                        draw_list->AddRect(screen_min, screen_max, SCENE_SCREEN_BORDER_COLOR);
                        screen_position.x = (screen_max.x + style->ItemSpacing.x);
                    }
                }
                DrawSceneInteraction(owner, editor, hover_screen, hover_window, handle_min, handle_max, has_handles, scale, draw_list, is_canvas_hovered, ImVec2(canvas_min.x + style->WindowPadding.x, canvas_max.y - label_height));
                // In play mode a click acts on the previewed widget as a CDS would
                if (is_canvas_hovered && owner->context->definitionEditor->isPlaying && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && (NULL != owner->context))
                {
                    FACEOE_Gfx_Point_t play_point;
                    uint32_t           play_index;
                    uint32_t           play_slot;

                    play_point.x = mouse_position.x;
                    play_point.y = mouse_position.y;
                    play_index   = Arinc661_Scene_Pick(owner->previewBounds, owner->previewBoundsCount, play_point);
                    for (play_slot = 0; (ARINC661_SCENE_NO_WIDGET != play_index) && (play_slot < owner->previewBoundsCount); ++play_slot)
                    {
                        if (owner->previewBounds[play_slot].widgetIndex == play_index)
                        {
                            (void)Arinc661_Definition_Interact(owner->context->definitionEditor, owner->previewDocuments[play_slot], play_index,
                                                               (mouse_position.x - owner->previewBounds[play_slot].minimum.x) / ImMax(1.0f, owner->previewBounds[play_slot].maximum.x - owner->previewBounds[play_slot].minimum.x),
                                                               (owner->previewBounds[play_slot].maximum.y - mouse_position.y) / ImMax(1.0f, owner->previewBounds[play_slot].maximum.y - owner->previewBounds[play_slot].minimum.y));
                            break;
                        }
                    }
                }
                // A double-click on a previewed widget opens it in the Definition File editor
                if (is_canvas_hovered && (FALSE == owner->context->definitionEditor->isPlaying) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (NULL != owner->context))
                {
                    FACEOE_Gfx_Point_t pick_point;
                    uint32_t           pick_index;
                    uint32_t           pick_slot;

                    pick_point.x = mouse_position.x;
                    pick_point.y = mouse_position.y;
                    pick_index   = Arinc661_Scene_Pick(owner->previewBounds, owner->previewBoundsCount, pick_point);
                    if (ARINC661_SCENE_NO_WIDGET != pick_index)
                    {
                        for (pick_slot = 0; pick_slot < owner->previewBoundsCount; ++pick_slot)
                        {
                            if (owner->previewBounds[pick_slot].widgetIndex == pick_index)
                            {
                                Arinc661_Definition_SelectWidget(owner->context->definitionEditor, owner->previewDocuments[pick_slot], pick_index);
                                owner->context->isDefinitionFocused = TRUE;
                                owner->context->hasFocusRequest     = TRUE;
                                break;
                            }
                        }
                    }
                }
                draw_list->PopClipRect();
            }
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies the five arrays' used parts and the counts into one heap block.
 */
static void* TakeSnapshot(const Arinc661_Configuration_Document_t* document)
{
    byte_t*  block;
    byte_t*  cursor;
    size_t   screen_bytes;
    size_t   layout_bytes;
    size_t   window_bytes;
    size_t   application_bytes;
    size_t   source_bytes;
    uint32_t counts[6];

    screen_bytes      = ((size_t)document->configuration.screenCount * sizeof(FACEOE_DisplayManagement_Screen_t));
    layout_bytes      = ((size_t)document->configuration.layoutCount * sizeof(FACEOE_DisplayManagement_Layout_t));
    window_bytes      = ((size_t)document->configuration.windowCount * sizeof(FACEOE_DisplayManagement_Window_t));
    application_bytes = ((size_t)document->configuration.userApplicationCount * sizeof(FACEOE_DisplayManagement_UserApplication_t));
    source_bytes      = ((size_t)document->configuration.externalSourceCount * sizeof(FACEOE_DisplayManagement_ExternalSource_t));
    block             = (byte_t*)malloc(sizeof(counts) + screen_bytes + layout_bytes + window_bytes + application_bytes + source_bytes);
    LOGASSERT(NULL != block);
    counts[0] = document->configuration.screenCount;
    counts[1] = document->configuration.layoutCount;
    counts[2] = document->configuration.windowCount;
    counts[3] = document->configuration.userApplicationCount;
    counts[4] = document->configuration.externalSourceCount;
    counts[5] = (uint32_t)document->selectedKind | ((uint32_t)document->selectedIndex << 8);
    cursor = block;
    (void)memcpy(cursor, counts, sizeof(counts));
    cursor += sizeof(counts);
    (void)memcpy(cursor, document->screens, screen_bytes);
    cursor += screen_bytes;
    (void)memcpy(cursor, document->layouts, layout_bytes);
    cursor += layout_bytes;
    (void)memcpy(cursor, document->windows, window_bytes);
    cursor += window_bytes;
    (void)memcpy(cursor, document->userApplications, application_bytes);
    cursor += application_bytes;
    (void)memcpy(cursor, document->externalSources, source_bytes);

    return block;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies a snapshot back and recomputes the derived screen extents.
 */
static void RestoreSnapshot(Arinc661_Configuration_Document_t* document, const void* snapshot)
{
    const byte_t* cursor;
    uint32_t      counts[6];
    size_t        screen_bytes;
    size_t        layout_bytes;
    size_t        window_bytes;
    size_t        application_bytes;
    size_t        source_bytes;

    cursor = (const byte_t*)snapshot;
    (void)memcpy(counts, cursor, sizeof(counts));
    cursor += sizeof(counts);
    screen_bytes      = ((size_t)counts[0] * sizeof(FACEOE_DisplayManagement_Screen_t));
    layout_bytes      = ((size_t)counts[1] * sizeof(FACEOE_DisplayManagement_Layout_t));
    window_bytes      = ((size_t)counts[2] * sizeof(FACEOE_DisplayManagement_Window_t));
    application_bytes = ((size_t)counts[3] * sizeof(FACEOE_DisplayManagement_UserApplication_t));
    source_bytes      = ((size_t)counts[4] * sizeof(FACEOE_DisplayManagement_ExternalSource_t));
    (void)memcpy(document->screens, cursor, screen_bytes);
    cursor += screen_bytes;
    (void)memcpy(document->layouts, cursor, layout_bytes);
    cursor += layout_bytes;
    (void)memcpy(document->windows, cursor, window_bytes);
    cursor += window_bytes;
    (void)memcpy(document->userApplications, cursor, application_bytes);
    cursor += application_bytes;
    (void)memcpy(document->externalSources, cursor, source_bytes);
    document->configuration.screenCount          = counts[0];
    document->configuration.layoutCount          = counts[1];
    document->configuration.windowCount          = counts[2];
    document->configuration.userApplicationCount = counts[3];
    document->configuration.externalSourceCount  = counts[4];
    document->selectedKind                       = (int32_t)(counts[5] & 0xFFU);
    document->selectedIndex                      = (counts[5] >> 8);
    document->isDirty                            = TRUE;
    UpdatePreviewMeasurements(document);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts and used arrays must match; the selection is ignored.
 */
static bool_t SnapshotMatches(const Arinc661_Configuration_Document_t* document, const void* snapshot)
{
    const byte_t* cursor;
    uint32_t      counts[6];
    size_t        screen_bytes;
    size_t        layout_bytes;
    size_t        window_bytes;
    size_t        application_bytes;
    size_t        source_bytes;
    bool_t        is_same;

    cursor = (const byte_t*)snapshot;
    (void)memcpy(counts, cursor, sizeof(counts));
    cursor += sizeof(counts);
    is_same = !!((counts[0] == document->configuration.screenCount) && (counts[1] == document->configuration.layoutCount) && (counts[2] == document->configuration.windowCount) &&
                 (counts[3] == document->configuration.userApplicationCount) && (counts[4] == document->configuration.externalSourceCount));
    if (is_same)
    {
        screen_bytes      = ((size_t)counts[0] * sizeof(FACEOE_DisplayManagement_Screen_t));
        layout_bytes      = ((size_t)counts[1] * sizeof(FACEOE_DisplayManagement_Layout_t));
        window_bytes      = ((size_t)counts[2] * sizeof(FACEOE_DisplayManagement_Window_t));
        application_bytes = ((size_t)counts[3] * sizeof(FACEOE_DisplayManagement_UserApplication_t));
        source_bytes      = ((size_t)counts[4] * sizeof(FACEOE_DisplayManagement_ExternalSource_t));
        is_same = !!((0 == memcmp(document->screens, cursor, screen_bytes)) && (0 == memcmp(document->layouts, cursor + screen_bytes, layout_bytes)) &&
                     (0 == memcmp(document->windows, cursor + screen_bytes + layout_bytes, window_bytes)) &&
                     (0 == memcmp(document->userApplications, cursor + screen_bytes + layout_bytes + window_bytes, application_bytes)) &&
                     (0 == memcmp(document->externalSources, cursor + screen_bytes + layout_bytes + window_bytes + application_bytes, source_bytes)));
    }

    return is_same;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Frees every snapshot.
 */
static void ClearHistory(Arinc661_Configuration_Document_t* document)
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
 *  Once per frame: a changed model turns the shadow into an undo step and drops the redo stack.
 */
static void CommitHistory(Arinc661_Configuration_Document_t* document)
{
    if (document->isLoaded)
    {
        if (NULL == document->shadow)
        {
            document->shadow = TakeSnapshot(document);
        }
        else if (FALSE == SnapshotMatches(document, document->shadow))
        {
            if (ARINC661_CONFIGURATION_MAX_HISTORY == document->undoCount)
            {
                free(document->undoStack[0]);
                (void)memmove(&document->undoStack[0], &document->undoStack[1], (ARINC661_CONFIGURATION_MAX_HISTORY - 1) * sizeof(document->undoStack[0]));
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
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The current state moves to the redo stack; the newest undo step becomes the model.
 */
static void UndoDocument(Arinc661_Configuration_Document_t* document)
{
    CommitHistory(document);
    if (0 < document->undoCount)
    {
        if (ARINC661_CONFIGURATION_MAX_HISTORY == document->redoCount)
        {
            free(document->redoStack[0]);
            (void)memmove(&document->redoStack[0], &document->redoStack[1], (ARINC661_CONFIGURATION_MAX_HISTORY - 1) * sizeof(document->redoStack[0]));
            document->redoCount -= 1;
        }
        document->redoStack[document->redoCount] = document->shadow;
        document->redoCount += 1;
        document->undoCount -= 1;
        document->shadow = document->undoStack[document->undoCount];
        RestoreSnapshot(document, document->shadow);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Mirror of UndoDocument.
 */
static void RedoDocument(Arinc661_Configuration_Document_t* document)
{
    CommitHistory(document);
    if (0 < document->redoCount)
    {
        document->undoStack[document->undoCount] = document->shadow;
        document->undoCount += 1;
        document->redoCount -= 1;
        document->shadow = document->redoStack[document->redoCount];
        RestoreSnapshot(document, document->shadow);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes the selected element through the same paths the hierarchy uses.
 */
static void RemoveSelection(Arinc661_Configuration_Document_t* document)
{
    switch (document->selectedKind)
    {
        case SELECTION_SCREEN:
            if (document->selectedIndex < document->configuration.screenCount)
            {
                RemoveScreen(document, document->selectedIndex);
            }
            break;
        case SELECTION_LAYOUT:
            if (document->selectedIndex < document->configuration.layoutCount)
            {
                RemoveLayout(document, document->selectedIndex);
            }
            break;
        case SELECTION_WINDOW:
            if (document->selectedIndex < document->configuration.windowCount)
            {
                RemoveWindow(document, document->selectedIndex);
            }
            break;
        case SELECTION_USER_APPLICATION:
            if (document->selectedIndex < document->configuration.userApplicationCount)
            {
                RemoveUserApplication(document, document->selectedIndex);
            }
            break;
        case SELECTION_EXTERNAL_SOURCE:
            if (document->selectedIndex < document->configuration.externalSourceCount)
            {
                RemoveExternalSource(document, document->selectedIndex);
                document->selectedKind = SELECTION_NONE;
                document->isDirty      = TRUE;
            }
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A window copy keeps the geometry and layout with a fresh id, offset by a tenth of its
 *  size so it is visible; a user application copy keeps its window and paths with a fresh
 *  applicationId.
 */
static void DuplicateSelection(Arinc661_Configuration_Document_t* document)
{
    FACEOE_DisplayManagement_Window_t          window;
    FACEOE_DisplayManagement_UserApplication_t application;
    int64_t                                    id;

    if ((SELECTION_WINDOW == document->selectedKind) && (document->selectedIndex < document->configuration.windowCount))
    {
        window = document->windows[document->selectedIndex];
        AddWindow(document, window.layoutIndex);
        if ((SELECTION_WINDOW == document->selectedKind) && (document->selectedIndex < document->configuration.windowCount))
        {
            id                                        = document->windows[document->selectedIndex].id;
            document->windows[document->selectedIndex] = window;
            document->windows[document->selectedIndex].id = id;
            document->windows[document->selectedIndex].pixelArea.x += (window.pixelArea.width / 10);
            document->windows[document->selectedIndex].pixelArea.y += (window.pixelArea.height / 10);
            document->isDirty = TRUE;
        }
    }
    else if ((SELECTION_USER_APPLICATION == document->selectedKind) && (document->selectedIndex < document->configuration.userApplicationCount))
    {
        application = document->userApplications[document->selectedIndex];
        AddUserApplication(document);
        if ((SELECTION_USER_APPLICATION == document->selectedKind) && (document->selectedIndex < document->configuration.userApplicationCount))
        {
            id                                                   = document->userApplications[document->selectedIndex].applicationId;
            document->userApplications[document->selectedIndex] = application;
            document->userApplications[document->selectedIndex].applicationId = id;
            document->isDirty = TRUE;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Combo over the document's screens, shown by id.
 */
static bool_t DrawScreenChoiceField(const Arinc661_Configuration_Document_t* document, const char_t* label, uint32_t* screenIndex)
{
    char_t   preview[LABEL_FORMAT_BYTES];
    char_t   item[LABEL_FORMAT_BYTES];
    uint32_t index;
    bool_t   has_changed;

    has_changed = FALSE;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    preview[0] = '\0';
    if ((*screenIndex) < document->configuration.screenCount)
    {
        ImFormatString(preview, sizeof(preview), SCREEN_CHOICE_FORMAT, document->screens[*screenIndex].id);
    }
    if (ImGui::BeginCombo("##screen", preview))
    {
        for (index = 0; index < document->configuration.screenCount; ++index)
        {
            ImFormatString(item, sizeof(item), SCREEN_CHOICE_FORMAT, document->screens[index].id);
            if (ImGui::Selectable(item, index == (*screenIndex)))
            {
                (*screenIndex) = index;
                has_changed    = TRUE;
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
 *  Combo over the document's layouts, each shown with its screen.
 */
static bool_t DrawLayoutChoiceField(const Arinc661_Configuration_Document_t* document, const char_t* label, uint32_t* layoutIndex)
{
    char_t   preview[LABEL_FORMAT_BYTES];
    char_t   item[LABEL_FORMAT_BYTES];
    uint32_t index;
    int64_t  screen_id;
    bool_t   has_changed;

    has_changed = FALSE;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    preview[0] = '\0';
    if ((*layoutIndex) < document->configuration.layoutCount)
    {
        screen_id = -1;
        if (document->layouts[*layoutIndex].screenIndex < document->configuration.screenCount)
        {
            screen_id = document->screens[document->layouts[*layoutIndex].screenIndex].id;
        }
        ImFormatString(preview, sizeof(preview), LAYOUT_CHOICE_FORMAT, document->layouts[*layoutIndex].id, screen_id);
    }
    if (ImGui::BeginCombo("##layout", preview))
    {
        for (index = 0; index < document->configuration.layoutCount; ++index)
        {
            screen_id = -1;
            if (document->layouts[index].screenIndex < document->configuration.screenCount)
            {
                screen_id = document->screens[document->layouts[index].screenIndex].id;
            }
            ImFormatString(item, sizeof(item), LAYOUT_CHOICE_FORMAT, document->layouts[index].id, screen_id);
            if (ImGui::Selectable(item, index == (*layoutIndex)))
            {
                (*layoutIndex) = index;
                has_changed    = TRUE;
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
 *  Icon row above the canvas: add screen, layout, window, user application, then duplicate
 *  and delete for the selection.
 */
static void DrawSceneToolbar(Arinc661_Configuration_Editor_t* owner, Arinc661_Configuration_Document_t* document)
{
    bool_t has_selection;

    has_selection = (SELECTION_NONE != document->selectedKind);
    if (Arinc661_Icons_Button(ARINC661_ICON_SCREEN, TOOL_ADD_SCREEN_LABEL, NULL, TRUE, FALSE))
    {
        Arinc661_Configuration_AddScreen(owner);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_LAYOUT, TOOL_ADD_LAYOUT_LABEL, NULL, TRUE, FALSE))
    {
        Arinc661_Configuration_AddLayout(owner);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_WINDOW, TOOL_ADD_WINDOW_LABEL, NULL, TRUE, FALSE))
    {
        Arinc661_Configuration_AddWindow(owner);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_APPLICATION, TOOL_ADD_APPLICATION_LABEL, NULL, TRUE, FALSE))
    {
        Arinc661_Configuration_AddUserApplication(owner);
    }
    Arinc661_Icons_Gap();
    if (Arinc661_Icons_Button(ARINC661_ICON_DUPLICATE, TOOL_DUPLICATE_LABEL, SHORTCUT_CTRL_D, has_selection, FALSE))
    {
        DuplicateSelection(document);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_DELETE, TOOL_DELETE_LABEL, SHORTCUT_DEL, has_selection, FALSE))
    {
        RemoveSelection(document);
    }
    Arinc661_Icons_Gap();
    (void)Arinc661_Icons_Button(ARINC661_ICON_HELP, SCENE_HINT_TITLE, NULL, TRUE, FALSE);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(SCENE_HINT_LABEL);
        ImGui::EndTooltip();
    }
    ImGui::NewLine();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Direct manipulation of windows on the scene: press on a window selects it and starts a
 *  move; press on a corner handle of the selected window starts a resize; the arrow keys
 *  nudge; Delete removes the selection; a right-click opens a menu to add elements where
 *  the cursor is. Screen pixels grow downward on both sides, so only the scale differs.
 */
static void DrawSceneInteraction(Arinc661_Configuration_Editor_t* owner, Arinc661_Configuration_Document_t* document, uint32_t hitScreen, uint32_t hitWindow, ImVec2 windowMin, ImVec2 windowMax, bool_t hasWindowArea, float32_t scale, ImDrawList* drawList, bool_t isCanvasHovered, ImVec2 hintPosition)
{
    FACEOE_DisplayManagement_Window_t* window;
    ImVec2                             mouse;
    ImVec2                             corners[4];
    float32_t                          delta_x;
    float32_t                          delta_y;
    int64_t                            step;
    int64_t                            width;
    int64_t                            height;
    uint32_t                           corner;
    uint32_t                           layout_index;
    int32_t                            hover_mode;

    mouse      = ImGui::GetMousePos();
    hover_mode = DRAG_NONE;
    if (hasWindowArea)
    {
        corners[0] = ImVec2(windowMin.x, windowMax.y);
        corners[1] = ImVec2(windowMax.x, windowMax.y);
        corners[2] = ImVec2(windowMin.x, windowMin.y);
        corners[3] = ImVec2(windowMax.x, windowMin.y);
        for (corner = 0; corner < 4; ++corner)
        {
            drawList->AddRectFilled(corners[corner] - ImVec2(HANDLE_HALF_SIZE, HANDLE_HALF_SIZE), corners[corner] + ImVec2(HANDLE_HALF_SIZE, HANDLE_HALF_SIZE), SCENE_HANDLE_COLOR);
            if ((ImFabs(mouse.x - corners[corner].x) <= HANDLE_HALF_SIZE) && (ImFabs(mouse.y - corners[corner].y) <= HANDLE_HALF_SIZE))
            {
                hover_mode = (DRAG_BOTTOM_LEFT + (int32_t)corner);
            }
        }
    }
    if (isCanvasHovered && (DRAG_NONE != hover_mode))
    {
        if ((DRAG_BOTTOM_LEFT == hover_mode) || (DRAG_TOP_RIGHT == hover_mode))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        }
        else
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        }
    }
    if (ImGui::IsItemActivated())
    {
        owner->dragMode   = DRAG_NONE;
        owner->hasDragged = FALSE;
        owner->dragStartX = mouse.x;
        owner->dragStartY = mouse.y;
        owner->dragScale  = scale;
        if ((DRAG_NONE != hover_mode) && (SELECTION_WINDOW == document->selectedKind))
        {
            owner->dragMode   = hover_mode;
            owner->dragWindow = document->selectedIndex;
        }
        else if (hitWindow < document->configuration.windowCount)
        {
            owner->dragMode   = DRAG_MOVE;
            owner->dragWindow = hitWindow;
        }
        if (DRAG_NONE != owner->dragMode)
        {
            window                  = &document->windows[owner->dragWindow];
            owner->dragOriginX      = window->pixelArea.x;
            owner->dragOriginY      = window->pixelArea.y;
            owner->dragOriginWidth  = window->pixelArea.width;
            owner->dragOriginHeight = window->pixelArea.height;
        }
    }
    if (ImGui::IsItemActive() && (DRAG_NONE != owner->dragMode) && (owner->dragWindow < document->configuration.windowCount) && (0.0f < owner->dragScale))
    {
        // Window y grows upward from the screen's bottom edge, as the scene draws it
        delta_x = ((mouse.x - owner->dragStartX) / owner->dragScale);
        delta_y = (-(mouse.y - owner->dragStartY) / owner->dragScale);
        if ((DRAG_THRESHOLD <= ImFabs(mouse.x - owner->dragStartX)) || (DRAG_THRESHOLD <= ImFabs(mouse.y - owner->dragStartY)))
        {
            owner->hasDragged = TRUE;
        }
        if (owner->hasDragged)
        {
            window = &document->windows[owner->dragWindow];
            if (DRAG_MOVE == owner->dragMode)
            {
                window->pixelArea.x = (owner->dragOriginX + (int64_t)ImFloor(delta_x + 0.5f));
                window->pixelArea.y = (owner->dragOriginY + (int64_t)ImFloor(delta_y + 0.5f));
            }
            else
            {
                width  = owner->dragOriginWidth;
                height = owner->dragOriginHeight;
                if ((DRAG_BOTTOM_RIGHT == owner->dragMode) || (DRAG_TOP_RIGHT == owner->dragMode))
                {
                    width = ImMax((int64_t)1, owner->dragOriginWidth + (int64_t)ImFloor(delta_x + 0.5f));
                }
                else
                {
                    width               = ImMax((int64_t)1, owner->dragOriginWidth - (int64_t)ImFloor(delta_x + 0.5f));
                    window->pixelArea.x = (owner->dragOriginX + (owner->dragOriginWidth - width));
                }
                if ((DRAG_TOP_LEFT == owner->dragMode) || (DRAG_TOP_RIGHT == owner->dragMode))
                {
                    height = ImMax((int64_t)1, owner->dragOriginHeight + (int64_t)ImFloor(delta_y + 0.5f));
                }
                else
                {
                    height              = ImMax((int64_t)1, owner->dragOriginHeight - (int64_t)ImFloor(delta_y + 0.5f));
                    window->pixelArea.y = (owner->dragOriginY + (owner->dragOriginHeight - height));
                }
                window->pixelArea.width  = width;
                window->pixelArea.height = height;
            }
            MarkDocumentDirty(document, TRUE);
        }
    }
    if (ImGui::IsItemDeactivated())
    {
        owner->dragMode = DRAG_NONE;
    }
    if (ImGui::IsWindowFocused() && (FALSE == ImGui::IsAnyItemActive()) && (SELECTION_WINDOW == document->selectedKind) && (document->selectedIndex < document->configuration.windowCount))
    {
        step = 1;
        if (ImGui::GetIO().KeyShift)
        {
            step = 10;
        }
        window = &document->windows[document->selectedIndex];
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            window->pixelArea.x -= step;
            MarkDocumentDirty(document, TRUE);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            window->pixelArea.x += step;
            MarkDocumentDirty(document, TRUE);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            window->pixelArea.y += step;
            MarkDocumentDirty(document, TRUE);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            window->pixelArea.y -= step;
            MarkDocumentDirty(document, TRUE);
        }
    }
    if (ImGui::IsWindowFocused() && (FALSE == ImGui::IsAnyItemActive()) && ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        RemoveSelection(document);
    }
    if (isCanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup(SCENE_POPUP_ID);
    }
    if (ImGui::BeginPopup(SCENE_POPUP_ID))
    {
        if (ImGui::MenuItem(ADD_SCREEN_MENU_LABEL))
        {
            AddScreen(document);
        }
        if (ImGui::MenuItem(ADD_LAYOUT_MENU_LABEL, NULL, false, hitScreen < document->configuration.screenCount))
        {
            AddLayout(document, hitScreen);
        }
        layout_index = ARINC661_CONFIGURATION_MAX_LAYOUTS;
        if (hitWindow < document->configuration.windowCount)
        {
            layout_index = document->windows[hitWindow].layoutIndex;
        }
        else if (hitScreen < document->configuration.screenCount)
        {
            for (corner = 0; corner < document->configuration.layoutCount; ++corner)
            {
                if (document->layouts[corner].screenIndex == hitScreen)
                {
                    layout_index = corner;
                    break;
                }
            }
        }
        if (ImGui::MenuItem(ADD_WINDOW_MENU_LABEL, NULL, false, layout_index < document->configuration.layoutCount))
        {
            AddWindow(document, layout_index);
        }
        if (ImGui::MenuItem(ADD_APPLICATION_MENU_LABEL, NULL, false, hitWindow < document->configuration.windowCount))
        {
            AddUserApplication(document);
            if ((SELECTION_USER_APPLICATION == document->selectedKind) && (document->selectedIndex < document->configuration.userApplicationCount))
            {
                document->userApplications[document->selectedIndex].window = document->windows[hitWindow].id;
            }
        }
        if (hitWindow < document->configuration.windowCount)
        {
            ImGui::Separator();
            if (ImGui::MenuItem(DUPLICATE_MENU_LABEL))
            {
                document->selectedKind  = SELECTION_WINDOW;
                document->selectedIndex = hitWindow;
                DuplicateSelection(document);
            }
            if (ImGui::MenuItem(REMOVE_MENU_LABEL))
            {
                RemoveWindow(document, hitWindow);
            }
        }
        ImGui::EndPopup();
    }
    (void)hintPosition;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  FACE H.3.2: a window's scalingFactor relates baseUnit to perUnit; when it relates pixels
 *  to millimetres or inches it gives pixels per 0.01 mm directly. Otherwise the screen's
 *  pixelSize over its physicalDimensions gives the density. With neither, a fallback density
 *  keeps the preview visible.
 */
static float32_t PixelsPerLayerUnit(const FACEOE_DisplayManagement_Screen_t* screen, const FACEOE_DisplayManagement_Window_t* window, bool_t isHorizontal)
{
    float32_t factor;
    float32_t pixels_per_mm;
    float32_t physical;
    float32_t pixels;

    pixels_per_mm = 0.0f;
    factor        = window->scalingFactor.xScale;
    if (FALSE == isHorizontal)
    {
        factor = window->scalingFactor.yScale;
    }
    if (0.0f < factor)
    {
        if ((FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL == window->scalingFactor.baseUnit) && (FACEOE_DISPLAYMANAGEMENT_UNIT_MM == window->scalingFactor.perUnit))
        {
            pixels_per_mm = factor;
        }
        else if ((FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL == window->scalingFactor.baseUnit) && (FACEOE_DISPLAYMANAGEMENT_UNIT_INCH == window->scalingFactor.perUnit))
        {
            pixels_per_mm = (factor / 25.4f);
        }
        else if ((FACEOE_DISPLAYMANAGEMENT_UNIT_MM == window->scalingFactor.baseUnit) && (FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL == window->scalingFactor.perUnit))
        {
            pixels_per_mm = (1.0f / factor);
        }
        else if ((FACEOE_DISPLAYMANAGEMENT_UNIT_INCH == window->scalingFactor.baseUnit) && (FACEOE_DISPLAYMANAGEMENT_UNIT_PIXEL == window->scalingFactor.perUnit))
        {
            pixels_per_mm = (1.0f / (factor * 25.4f));
        }
    }
    if ((0.0f >= pixels_per_mm) && (NULL != screen))
    {
        pixels   = (float32_t)screen->pixelSize.width;
        physical = (float32_t)screen->physicalDimensions.width;
        if (FALSE == isHorizontal)
        {
            pixels   = (float32_t)screen->pixelSize.height;
            physical = (float32_t)screen->physicalDimensions.height;
        }
        if ((0.0f < pixels) && (0.0f < physical))
        {
            if (FACEOE_DISPLAYMANAGEMENT_UNIT_MM == screen->physicalDimensions.units)
            {
                pixels_per_mm = (pixels / physical);
            }
            else if (FACEOE_DISPLAYMANAGEMENT_UNIT_INCH == screen->physicalDimensions.units)
            {
                pixels_per_mm = (pixels / (physical * 25.4f));
            }
        }
    }
    if (0.0f >= pixels_per_mm)
    {
        pixels_per_mm = SCENE_PIXELS_PER_MM_FALLBACK;
    }

    return (pixels_per_mm / 100.0f);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Every user application targeting the window has its open Definition File recorded with
 *  the layer origin at the window's lower-left corner, scaled by the window's pixel density
 *  and the scene scale, clipped to the window. Files that are not open draw nothing, so the
 *  preview shows only what the editor knows.
 */
static void DrawWindowPreview(Arinc661_Configuration_Editor_t* editor, Arinc661_Configuration_Document_t* document, const FACEOE_DisplayManagement_Screen_t* screen, const FACEOE_DisplayManagement_Window_t* window, ImVec2 areaMin, ImVec2 areaMax, float32_t scale, ImDrawList* drawList)
{
    const FACEOE_DisplayManagement_UserApplication_t* application;
    FACEOE_Gfx_Transform_t                            transform;
    FACEOE_Gfx_Point_t                                minimum;
    FACEOE_Gfx_Point_t                                maximum;
    uint32_t                                          application_index;
    uint32_t                                          document_index;
    uint32_t                                          bounds_count;
    uint32_t                                          bounds_index;
    uint32_t                                          selected_widget;

    if (NULL != editor->context)
    {
        minimum.x = areaMin.x;
        minimum.y = areaMin.y;
        maximum.x = areaMax.x;
        maximum.y = areaMax.y;
        for (application_index = 0; application_index < document->configuration.userApplicationCount; ++application_index)
        {
            application = &document->userApplications[application_index];
            if ((application->window != window->id) || ('\0' == application->dFPath[0]) || (FALSE == application->visible))
            {
                continue;
            }
            (void)memset(&transform, 0, sizeof(transform));
            transform.a = (PixelsPerLayerUnit(screen, window, TRUE) * scale);
            transform.d = (-PixelsPerLayerUnit(screen, window, FALSE) * scale);
            transform.e = areaMin.x;
            transform.f = areaMax.y;
            FACEOE_Gfx_DrawList_Clear(&editor->drawList);
            FACEOE_Gfx_DrawList_SetTransform(&editor->drawList, &transform);
            document_index = Arinc661_Definition_FindDocument(editor->context->definitionEditor, application->dFPath);
            if (ARINC661_DEFINITION_NO_DOCUMENT != document_index)
            {
                bounds_count = 0;
                if (Arinc661_Definition_RecordDocumentBounds(editor->context->definitionEditor, document_index, &editor->drawList, ImGui::GetFontSize(), &editor->previewBounds[editor->previewBoundsCount], ARINC661_CONFIGURATION_MAX_PREVIEW_BOUNDS - editor->previewBoundsCount, &bounds_count))
                {
                    for (bounds_index = 0; bounds_index < bounds_count; ++bounds_index)
                    {
                        editor->previewDocuments[editor->previewBoundsCount + bounds_index] = document_index;
                    }
                    editor->previewBoundsCount += bounds_count;
                    drawList->PushClipRect(ImVec2(minimum.x, minimum.y), ImVec2(maximum.x, maximum.y), true);
                    Arinc661_Gfx_ImGui_Render(&editor->drawList, drawList);
                    // Outline the widget selected in the Definition File editor so both views agree
                    if (Arinc661_Definition_SelectedWidgetIndex(editor->context->definitionEditor, document_index, &selected_widget))
                    {
                        for (bounds_index = 0; bounds_index < bounds_count; ++bounds_index)
                        {
                            if (editor->previewBounds[editor->previewBoundsCount - bounds_count + bounds_index].widgetIndex == selected_widget)
                            {
                                drawList->AddRect(ImVec2(editor->previewBounds[editor->previewBoundsCount - bounds_count + bounds_index].minimum.x, editor->previewBounds[editor->previewBoundsCount - bounds_count + bounds_index].minimum.y), ImVec2(editor->previewBounds[editor->previewBoundsCount - bounds_count + bounds_index].maximum.x, editor->previewBounds[editor->previewBoundsCount - bounds_count + bounds_index].maximum.y), SCENE_TEXT_COLOR, 0.0f, 0, 2.0f);
                            }
                        }
                    }
                    drawList->PopClipRect();
                }
            }
        }
    }
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
 *  Includes every hierarchy label and its adjacent remove control in the left pane minimum.
 */
static void MeasurePaneWidths(const Arinc661_Configuration_Document_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth)
{
    const ImGuiStyle* style;
    ImVec2            button_padding;
    char_t            integer_text[INTEGER_TEXT_BYTES];
    char_t            hierarchy_label[HIERARCHY_LABEL_BYTES];
    float32_t         hierarchy_row_padding;
    float32_t         hierarchy_row_width;
    float32_t         property_width;
    float32_t         value_width;
    uint32_t          screen_index;
    uint32_t          layout_index;
    uint32_t          window_index;
    uint32_t          application_index;

    style = &ImGui::GetStyle();

    ImFormatString(integer_text, sizeof(integer_text), "%" PRId64, INT64_MIN);
    button_padding = (style->FramePadding + style->FramePadding);

    property_width    = (ImGui::CalcTextSize(MAP_SOURCE_LAYER_LABEL).x + button_padding.x);
    value_width       = (ImGui::CalcTextSize(integer_text).x + button_padding.x);
    (*hierarchyWidth) = (ImGui::CalcTextSize(HIERARCHY_PANE_NAME).x + (style->WindowPadding.x * 2.0f));

    hierarchy_row_padding  = (style->WindowPadding.x * 2.0f);
    hierarchy_row_padding += ImGui::GetTreeNodeToLabelSpacing();
    hierarchy_row_padding += style->ItemSpacing.x;
    hierarchy_row_padding += ImGui::GetTextLineHeight();

    hierarchy_row_width = (ImGui::CalcTextSize(ADD_SCREEN_LABEL).x + hierarchy_row_padding);
    (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    hierarchy_row_width = (ImGui::CalcTextSize(ADD_USER_APPLICATION_LABEL).x + hierarchy_row_padding);
    (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);

    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), SCREEN_LABEL_FORMAT, editor->screens[screen_index].id);
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding);
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }

    for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), LAYOUT_LABEL_FORMAT, editor->layouts[layout_index].id);
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding + style->IndentSpacing);
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }

    for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), WINDOW_LABEL_FORMAT, editor->windows[window_index].id, editor->windows[window_index].name);
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding + (style->IndentSpacing * 2.0f));
        (*hierarchyWidth)   = ImMax((*hierarchyWidth), hierarchy_row_width);
    }

    for (application_index = 0; application_index < editor->configuration.userApplicationCount; ++application_index)
    {
        ImFormatString(hierarchy_label, sizeof(hierarchy_label), USER_APPLICATION_LABEL_FORMAT, editor->userApplications[application_index].applicationId);
        hierarchy_row_width = (ImGui::CalcTextSize(hierarchy_label).x + hierarchy_row_padding);
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
static void DrawPanes(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    ImVec2                             table_size;
    float32_t                          minimum_hierarchy_width;
    float32_t                          inspector_width;
    float32_t                          status_bar_height;

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
        DrawInspectorPane(editor, document);
        ImGui::TableSetColumnIndex(SCENE_PANE_COLUMN_IDX);
        DrawScenePane(editor, document);
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
 *  Initializes every editor field and its owned array storage.
 */
void Arinc661_Configuration_Initialize(Arinc661_Configuration_Editor_t* editor)
{
    uint32_t owner_index;
    Arinc661_Configuration_Document_t* document;
    uint32_t                           document_index;

    editor->status                    = STATUS_XML_FILE_REQUIRED;
    editor->activeDocumentIndex       = INVALID_DOCUMENT_INDEX;
    editor->requestedDocumentIndex    = INVALID_DOCUMENT_INDEX;
    editor->pendingCloseDocumentIndex = INVALID_DOCUMENT_INDEX;
    editor->pendingSaveDocumentIndex  = INVALID_DOCUMENT_INDEX;
    editor->isSaveAllPending          = FALSE;
    ImStrncpy(editor->folderPath, DEFAULT_FOLDER_PATH, sizeof(editor->folderPath));
    (void)memset(editor->saveFolderPath, 0, sizeof(editor->saveFolderPath));
    (void)memset(editor->saveFileName, 0, sizeof(editor->saveFileName));
    editor->definitionFileCount = 0;
    (void)memset(editor->definitionFiles, 0, sizeof(editor->definitionFiles));
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        document->status              = STATUS_NONE;
        document->isLoaded            = FALSE;
        document->isDirty             = FALSE;
        document->selectedKind        = SELECTION_NONE;
        document->selectedIndex       = 0;
        document->totalScreenWidth    = 0.0f;
        document->maximumScreenHeight = 0.0f;
        document->drawableScreenCount = 0;
        (void)memset(document->documentName, 0, sizeof(document->documentName));
        (void)memset(document->documentPath, 0, sizeof(document->documentPath));
        (void)memset(document->inputBuffer, 0, sizeof(document->inputBuffer));
        (void)memset(document->outputBuffer, 0, sizeof(document->outputBuffer));
        (void)memset(document->workspace, 0, sizeof(document->workspace));
        (void)memset(document->screens, 0, sizeof(document->screens));
        (void)memset(document->layouts, 0, sizeof(document->layouts));
        (void)memset(document->windows, 0, sizeof(document->windows));
        (void)memset(document->userApplications, 0, sizeof(document->userApplications));
        (void)memset(document->externalSources, 0, sizeof(document->externalSources));
        document->configuration.screens                 = document->screens;
        document->configuration.layouts                 = document->layouts;
        document->configuration.windows                 = document->windows;
        document->configuration.userApplications        = document->userApplications;
        document->configuration.externalSources         = document->externalSources;
        document->configuration.screenCapacity          = ARINC661_CONFIGURATION_MAX_SCREENS;
        document->configuration.layoutCapacity          = ARINC661_CONFIGURATION_MAX_LAYOUTS;
        document->configuration.windowCapacity          = ARINC661_CONFIGURATION_MAX_WINDOWS;
        document->configuration.userApplicationCapacity = ARINC661_CONFIGURATION_MAX_USER_APPLICATIONS;
        document->configuration.externalSourceCapacity  = ARINC661_CONFIGURATION_MAX_EXTERNAL_SOURCES;
        document->configuration.screenCount             = 0;
        document->configuration.layoutCount             = 0;
        document->configuration.windowCount             = 0;
        document->configuration.userApplicationCount    = 0;
        document->configuration.externalSourceCount     = 0;
        document->configuration.hasMetaData             = FALSE;
        document->configuration.hasXmlDeclaration       = FALSE;
        document->configuration.schemaLocation[0]       = '\0';
    }
    editor->context = NULL;
    for (owner_index = 0; owner_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++owner_index)
    {
        editor->documents[owner_index].owner = editor;
    }
    FACEOE_Gfx_DrawList_Initialize(&editor->drawList, editor->sceneCommands, ARINC661_CONFIGURATION_MAX_SCENE_COMMANDS, editor->scenePoints, ARINC661_CONFIGURATION_MAX_SCENE_POINTS, editor->sceneText, ARINC661_CONFIGURATION_MAX_SCENE_TEXT_BYTES);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws file controls above the panes and a full-width status bar below them.
 */
void Arinc661_Configuration_DrawEditor(Arinc661_Configuration_Editor_t* editor)
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
    Arinc661_Configuration_EndFrame(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Replaces the folder; the next file list shows that folder's XML files.
 */
void Arinc661_Configuration_SetFolderPath(Arinc661_Configuration_Editor_t* editor, const char_t* folderPath)
{
    ImStrncpy(editor->folderPath, folderPath, sizeof(editor->folderPath));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as selecting the file in the file list.
 */
void Arinc661_Configuration_LoadFile(Arinc661_Configuration_Editor_t* editor, const char_t* fileName)
{
    LoadConfigurationFile(editor, fileName);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as the File menu's New.
 */
void Arinc661_Configuration_CreateDocument(Arinc661_Configuration_Editor_t* editor)
{
    CreateDocument(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Same as the File menu's Save; nothing happens without an active document.
 */
void Arinc661_Configuration_SaveActiveDocument(Arinc661_Configuration_Editor_t* editor)
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
void Arinc661_Configuration_SaveAllDocuments(Arinc661_Configuration_Editor_t* editor)
{
    SaveAllDocuments(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A changed document is closed through the prompt drawn by the document tabs.
 */
void Arinc661_Configuration_CloseActiveDocument(Arinc661_Configuration_Editor_t* editor)
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
bool_t Arinc661_Configuration_HasActiveDocument(const Arinc661_Configuration_Editor_t* editor, bool_t* isDirty, bool_t* hasFree)
{
    uint32_t document_index;
    bool_t   is_dirty;
    bool_t   has_free;

    is_dirty = FALSE;
    has_free = FALSE;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
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
void Arinc661_Configuration_DrawDocumentTabs(Arinc661_Configuration_Editor_t* editor)
{

    DrawDocumentTabs(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws nothing without an active document.
 */
void Arinc661_Configuration_DrawHierarchy(Arinc661_Configuration_Editor_t* editor)
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
void Arinc661_Configuration_DrawScene(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    ImGui::PushID(document);
    DrawScenePane(editor, document);
    ImGui::PopID();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the first slot, which is never loaded, when no document is active.
 */
void Arinc661_Configuration_DrawInspector(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;

    document = &editor->documents[0];
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        document = &editor->documents[editor->activeDocumentIndex];
    }
    ImGui::PushID(document);
    DrawInspectorPane(editor, document);
    ImGui::PopID();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The close prompt belongs to the document tabs; only the save-as dialog is drawn here.
 */
void Arinc661_Configuration_DrawDialogs(Arinc661_Configuration_Editor_t* editor)
{
    DrawSaveAsDialog(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The editor message wins over the active document's message.
 */
const char_t* Arinc661_Configuration_StatusText(const Arinc661_Configuration_Editor_t* editor)
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
void Arinc661_Configuration_MeasurePanes(const Arinc661_Configuration_Editor_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth)
{
    const Arinc661_Configuration_Document_t* document;

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
 *  Empties the dFPath choices; the host adds the current folder's Definition Files again.
 */
void Arinc661_Configuration_ClearDefinitionFiles(Arinc661_Configuration_Editor_t* editor)
{
    editor->definitionFileCount = 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Names beyond the capacity are dropped.
 */
void Arinc661_Configuration_AddDefinitionFile(Arinc661_Configuration_Editor_t* editor, const char_t* fileName)
{
    if (editor->definitionFileCount < ARINC661_CONFIGURATION_MAX_DEFINITION_FILES)
    {
        ImStrncpy(editor->definitionFiles[editor->definitionFileCount], fileName, sizeof(editor->definitionFiles[0]));
        editor->definitionFileCount += 1;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Stores the pointer; the inspector and scene consult it when drawing.
 */
void Arinc661_Configuration_SetContext(Arinc661_Configuration_Editor_t* editor, Arinc661_Context_t* context)
{
    LOGASSERT(NULL != editor);
    editor->context = context;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  NULL for an empty slot.
 */
const char_t* Arinc661_Configuration_DocumentName(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex)
{
    const char_t* name;

    LOGASSERT(NULL != editor);
    name = NULL;
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        name = editor->documents[documentIndex].documentName;
    }

    return name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Zero for an empty slot.
 */
uint32_t Arinc661_Configuration_UserApplicationCount(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex)
{
    uint32_t count;

    LOGASSERT(NULL != editor);
    count = 0;
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        count = editor->documents[documentIndex].configuration.userApplicationCount;
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads straight from the document's user application array.
 */
bool_t Arinc661_Configuration_UserApplication(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex, int64_t* applicationId, int64_t* windowId, const char_t** dFPath)
{
    const FACEOE_DisplayManagement_UserApplication_t* application;
    bool_t                                            is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != applicationId);
    LOGASSERT(NULL != windowId);
    LOGASSERT(NULL != dFPath);
    is_found = FALSE;
    if (applicationIndex < Arinc661_Configuration_UserApplicationCount(editor, documentIndex))
    {
        application      = &editor->documents[documentIndex].userApplications[applicationIndex];
        (*applicationId) = application->applicationId;
        (*windowId)      = application->window;
        (*dFPath)        = application->dFPath;
        is_found         = TRUE;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The tab bar honours the document request on its next draw.
 */
void Arinc661_Configuration_SelectUserApplication(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex)
{
    LOGASSERT(NULL != editor);
    if (applicationIndex < Arinc661_Configuration_UserApplicationCount(editor, documentIndex))
    {
        editor->activeDocumentIndex                    = documentIndex;
        editor->requestedDocumentIndex                 = documentIndex;
        editor->documents[documentIndex].selectedKind  = SELECTION_USER_APPLICATION;
        editor->documents[documentIndex].selectedIndex = applicationIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Configuration_Undo(Arinc661_Configuration_Editor_t* editor)
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
void Arinc661_Configuration_Redo(Arinc661_Configuration_Editor_t* editor)
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
void Arinc661_Configuration_History(const Arinc661_Configuration_Editor_t* editor, bool_t* canUndo, bool_t* canRedo)
{
    const Arinc661_Configuration_Document_t* document;

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
void Arinc661_Configuration_DeleteSelection(Arinc661_Configuration_Editor_t* editor)
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
 *  Acts on the active document.
 */
void Arinc661_Configuration_DuplicateSelection(Arinc661_Configuration_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if (INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex)
    {
        DuplicateSelection(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Configuration_AddScreen(Arinc661_Configuration_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        AddScreen(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates a screen first when the document has none.
 */
void Arinc661_Configuration_AddLayout(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           screen_index;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if (0 == document->configuration.screenCount)
        {
            AddScreen(document);
        }
        screen_index = 0;
        if ((SELECTION_SCREEN == document->selectedKind) && (document->selectedIndex < document->configuration.screenCount))
        {
            screen_index = document->selectedIndex;
        }
        else if ((SELECTION_LAYOUT == document->selectedKind) && (document->selectedIndex < document->configuration.layoutCount))
        {
            screen_index = document->layouts[document->selectedIndex].screenIndex;
        }
        else if ((SELECTION_WINDOW == document->selectedKind) && (document->selectedIndex < document->configuration.windowCount) && (document->windows[document->selectedIndex].layoutIndex < document->configuration.layoutCount))
        {
            screen_index = document->layouts[document->windows[document->selectedIndex].layoutIndex].screenIndex;
        }
        if (screen_index < document->configuration.screenCount)
        {
            AddLayout(document, screen_index);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates a screen and a layout first when the document has none.
 */
void Arinc661_Configuration_AddWindow(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           layout_index;
    uint32_t                           index;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        if (0 == document->configuration.layoutCount)
        {
            Arinc661_Configuration_AddLayout(editor);
        }
        layout_index = 0;
        if ((SELECTION_LAYOUT == document->selectedKind) && (document->selectedIndex < document->configuration.layoutCount))
        {
            layout_index = document->selectedIndex;
        }
        else if ((SELECTION_WINDOW == document->selectedKind) && (document->selectedIndex < document->configuration.windowCount))
        {
            layout_index = document->windows[document->selectedIndex].layoutIndex;
        }
        else if ((SELECTION_SCREEN == document->selectedKind) && (document->selectedIndex < document->configuration.screenCount))
        {
            for (index = 0; index < document->configuration.layoutCount; ++index)
            {
                if (document->layouts[index].screenIndex == document->selectedIndex)
                {
                    layout_index = index;
                    break;
                }
            }
        }
        if (layout_index < document->configuration.layoutCount)
        {
            AddWindow(document, layout_index);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The new user application targets the selected window when one is selected.
 */
void Arinc661_Configuration_AddUserApplication(Arinc661_Configuration_Editor_t* editor)
{
    Arinc661_Configuration_Document_t* document;
    int64_t                            window_id;
    bool_t                             has_window;

    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document   = &editor->documents[editor->activeDocumentIndex];
        has_window = !!((SELECTION_WINDOW == document->selectedKind) && (document->selectedIndex < document->configuration.windowCount));
        window_id  = 0;
        if (has_window)
        {
            window_id = document->windows[document->selectedIndex].id;
        }
        AddUserApplication(document);
        if (has_window && (SELECTION_USER_APPLICATION == document->selectedKind) && (document->selectedIndex < document->configuration.userApplicationCount))
        {
            document->userApplications[document->selectedIndex].window = window_id;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Names follow FACE H.3 element names.
 */
const char_t* Arinc661_Configuration_SelectedElement(const Arinc661_Configuration_Editor_t* editor)
{
    const Arinc661_Configuration_Document_t* document;
    const char_t*                            element;

    LOGASSERT(NULL != editor);
    element = NULL;
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        switch (document->selectedKind)
        {
            case SELECTION_SCREEN:           element = ELEMENT_SCREEN; break;
            case SELECTION_LAYOUT:           element = ELEMENT_LAYOUT; break;
            case SELECTION_WINDOW:           element = ELEMENT_WINDOW; break;
            case SELECTION_USER_APPLICATION: element = ELEMENT_USER_APPLICATION; break;
            case SELECTION_EXTERNAL_SOURCE:  element = ELEMENT_EXTERNAL_SOURCE; break;
            default:                         break;
        }
    }

    return element;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Turns the changes of the frame into one undo step per changed document.
 */
void Arinc661_Configuration_EndFrame(Arinc661_Configuration_Editor_t* editor)
{
    uint32_t document_index;

    LOGASSERT(NULL != editor);
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        CommitHistory(&editor->documents[document_index]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Density comes from the window's scalingFactor or its screen, as the preview uses it.
 */
bool_t Arinc661_Configuration_Window(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, int64_t windowId, uint32_t* windowIndex, int64_t* pixelWidth, int64_t* pixelHeight, float32_t* unitsPerPixelX, float32_t* unitsPerPixelY)
{
    const Arinc661_Configuration_Document_t*  document;
    const FACEOE_DisplayManagement_Window_t*  window;
    const FACEOE_DisplayManagement_Screen_t*  screen;
    uint32_t                                  index;
    float32_t                                 pixels_per_unit;
    bool_t                                    is_found;

    LOGASSERT(NULL != editor);
    is_found = FALSE;
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document = &editor->documents[documentIndex];
        for (index = 0; index < document->configuration.windowCount; ++index)
        {
            window = &document->windows[index];
            if (window->id == windowId)
            {
                screen = NULL;
                if ((window->layoutIndex < document->configuration.layoutCount) && (document->layouts[window->layoutIndex].screenIndex < document->configuration.screenCount))
                {
                    screen = &document->screens[document->layouts[window->layoutIndex].screenIndex];
                }
                if (NULL != windowIndex)
                {
                    (*windowIndex) = index;
                }
                if (NULL != pixelWidth)
                {
                    (*pixelWidth) = window->pixelArea.width;
                }
                if (NULL != pixelHeight)
                {
                    (*pixelHeight) = window->pixelArea.height;
                }
                if (NULL != unitsPerPixelX)
                {
                    pixels_per_unit   = PixelsPerLayerUnit(screen, window, TRUE);
                    (*unitsPerPixelX) = (1.0f / pixels_per_unit);
                }
                if (NULL != unitsPerPixelY)
                {
                    pixels_per_unit   = PixelsPerLayerUnit(screen, window, FALSE);
                    (*unitsPerPixelY) = (1.0f / pixels_per_unit);
                }
                is_found = TRUE;
                break;
            }
        }
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Zero for an empty slot.
 */
uint32_t Arinc661_Configuration_WindowCount(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex)
{
    uint32_t count;

    LOGASSERT(NULL != editor);
    count = 0;
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        count = editor->documents[documentIndex].configuration.windowCount;
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads straight from the window array.
 */
bool_t Arinc661_Configuration_WindowAt(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t windowIndex, int64_t* windowId, const char_t** name)
{
    bool_t is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != windowId);
    LOGASSERT(NULL != name);
    is_found = FALSE;
    if (windowIndex < Arinc661_Configuration_WindowCount(editor, documentIndex))
    {
        (*windowId) = editor->documents[documentIndex].windows[windowIndex].id;
        (*name)     = editor->documents[documentIndex].windows[windowIndex].name;
        is_found    = TRUE;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The tab bar honours the document request on its next draw.
 */
void Arinc661_Configuration_SelectWindow(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t windowIndex)
{
    LOGASSERT(NULL != editor);
    if (windowIndex < Arinc661_Configuration_WindowCount(editor, documentIndex))
    {
        editor->activeDocumentIndex                    = documentIndex;
        editor->requestedDocumentIndex                 = documentIndex;
        editor->documents[documentIndex].selectedKind  = SELECTION_WINDOW;
        editor->documents[documentIndex].selectedIndex = windowIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the ordinary add path, then overwrites the seeded values.
 */
bool_t Arinc661_Configuration_AddUserApplicationFor(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, int64_t applicationId, int64_t windowId, const char_t* dFPath)
{
    Arinc661_Configuration_Document_t*          document;
    FACEOE_DisplayManagement_UserApplication_t* application;
    bool_t                                      is_added;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != dFPath);
    is_added = FALSE;
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded)
    {
        document = &editor->documents[documentIndex];
        AddUserApplication(document);
        if ((SELECTION_USER_APPLICATION == document->selectedKind) && (document->selectedIndex < document->configuration.userApplicationCount))
        {
            application                = &document->userApplications[document->selectedIndex];
            application->applicationId = applicationId;
            application->window        = windowId;
            ImStrncpy(application->dFPath, dFPath, sizeof(application->dFPath));
            application->visible           = TRUE;
            editor->activeDocumentIndex    = documentIndex;
            editor->requestedDocumentIndex = documentIndex;
            document->isDirty              = TRUE;
            is_added                       = TRUE;
        }
    }

    return is_added;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks every open document; changed documents become dirty.
 */
uint32_t Arinc661_Configuration_RepointDefinition(Arinc661_Configuration_Editor_t* editor, const char_t* previousName, const char_t* newName)
{
    Arinc661_Configuration_Document_t* document;
    uint32_t                           document_index;
    uint32_t                           application_index;
    uint32_t                           count;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != previousName);
    LOGASSERT(NULL != newName);
    count = 0;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document = &editor->documents[document_index];
        if (document->isLoaded)
        {
            for (application_index = 0; application_index < document->configuration.userApplicationCount; ++application_index)
            {
                if (0 == ImStricmp(document->userApplications[application_index].dFPath, previousName))
                {
                    ImStrncpy(document->userApplications[application_index].dFPath, newName, sizeof(document->userApplications[application_index].dFPath));
                    document->isDirty = TRUE;
                    count += 1;
                }
            }
        }
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Configuration, then the selected screen, layout, window, or user application.
 */
void Arinc661_Configuration_SelectionText(const Arinc661_Configuration_Editor_t* editor, char_t* text, uint32_t capacity)
{
    const Arinc661_Configuration_Document_t* document;
    const FACEOE_DisplayManagement_Window_t* window;
    const FACEOE_DisplayManagement_Layout_t* layout;
    int64_t                                  screen_id;
    int64_t                                  layout_id;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != text);
    text[0] = '\0';
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        document = &editor->documents[editor->activeDocumentIndex];
        switch (document->selectedKind)
        {
            case SELECTION_SCREEN:
                if (document->selectedIndex < document->configuration.screenCount)
                {
                    ImFormatString(text, capacity, BREADCRUMB_SCREEN_FORMAT, document->documentName, document->screens[document->selectedIndex].id);
                }
                break;
            case SELECTION_LAYOUT:
                if (document->selectedIndex < document->configuration.layoutCount)
                {
                    layout    = &document->layouts[document->selectedIndex];
                    screen_id = -1;
                    if (layout->screenIndex < document->configuration.screenCount)
                    {
                        screen_id = document->screens[layout->screenIndex].id;
                    }
                    ImFormatString(text, capacity, BREADCRUMB_LAYOUT_FORMAT, document->documentName, screen_id, layout->id);
                }
                break;
            case SELECTION_WINDOW:
                if (document->selectedIndex < document->configuration.windowCount)
                {
                    window    = &document->windows[document->selectedIndex];
                    screen_id = -1;
                    layout_id = -1;
                    if (window->layoutIndex < document->configuration.layoutCount)
                    {
                        layout    = &document->layouts[window->layoutIndex];
                        layout_id = layout->id;
                        if (layout->screenIndex < document->configuration.screenCount)
                        {
                            screen_id = document->screens[layout->screenIndex].id;
                        }
                    }
                    ImFormatString(text, capacity, BREADCRUMB_WINDOW_FORMAT, document->documentName, screen_id, layout_id, window->id, window->name);
                }
                break;
            case SELECTION_EXTERNAL_SOURCE:
                if (document->selectedIndex < document->configuration.externalSourceCount)
                {
                    ImFormatString(text, capacity, BREADCRUMB_EXTERNAL_FORMAT, document->documentName, (unsigned)document->externalSources[document->selectedIndex].id, document->externalSources[document->selectedIndex].name);
                }
                break;
            case SELECTION_USER_APPLICATION:
                if (document->selectedIndex < document->configuration.userApplicationCount)
                {
                    ImFormatString(text, capacity, BREADCRUMB_APPLICATION_FORMAT, document->documentName, document->userApplications[document->selectedIndex].applicationId, document->userApplications[document->selectedIndex].window, document->userApplications[document->selectedIndex].dFPath);
                }
                break;
            default:
                ImFormatString(text, capacity, "%s", document->documentName);
                break;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads straight from the external source array.
 */
bool_t Arinc661_Configuration_ExternalSourceAt(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t sourceIndex, uint32_t* sourceId, const char_t** name, const char_t** type)
{
    const Arinc661_Configuration_Document_t* document;
    bool_t                                   is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != sourceId);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != type);
    is_found = FALSE;
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (sourceIndex < editor->documents[documentIndex].configuration.externalSourceCount))
    {
        document    = &editor->documents[documentIndex];
        (*sourceId) = document->externalSources[sourceIndex].id;
        (*name)     = document->externalSources[sourceIndex].name;
        (*type)     = document->externalSources[sourceIndex].type;
        is_found    = TRUE;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The tab bar honours the document request on its next draw.
 */
void Arinc661_Configuration_SelectExternalSource(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t sourceIndex)
{
    LOGASSERT(NULL != editor);
    if ((documentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS) && editor->documents[documentIndex].isLoaded && (sourceIndex < editor->documents[documentIndex].configuration.externalSourceCount))
    {
        editor->activeDocumentIndex                    = documentIndex;
        editor->requestedDocumentIndex                 = documentIndex;
        editor->documents[documentIndex].selectedKind  = SELECTION_EXTERNAL_SOURCE;
        editor->documents[documentIndex].selectedIndex = sourceIndex;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Acts on the active document.
 */
void Arinc661_Configuration_AddExternalSource(Arinc661_Configuration_Editor_t* editor)
{
    LOGASSERT(NULL != editor);
    if ((INVALID_DOCUMENT_INDEX != editor->activeDocumentIndex) && editor->documents[editor->activeDocumentIndex].isLoaded)
    {
        AddExternalSource(&editor->documents[editor->activeDocumentIndex]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the user application's flags and style path.
 */
bool_t Arinc661_Configuration_UserApplicationDetails(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex, bool_t* mapSourceLayer, bool_t* isVisible, const char_t** styleFilePath)
{
    const FACEOE_DisplayManagement_UserApplication_t* application;
    bool_t                                            is_found;

    LOGASSERT(NULL != editor);
    LOGASSERT(NULL != mapSourceLayer);
    LOGASSERT(NULL != isVisible);
    LOGASSERT(NULL != styleFilePath);
    is_found = FALSE;
    if (applicationIndex < Arinc661_Configuration_UserApplicationCount(editor, documentIndex))
    {
        application       = &editor->documents[documentIndex].userApplications[applicationIndex];
        (*mapSourceLayer) = application->mapSourceLayer;
        (*isVisible)      = application->visible;
        (*styleFilePath)  = application->styleFilePath;
        is_found          = TRUE;
    }

    return is_found;
}
