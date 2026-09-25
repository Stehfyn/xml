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
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_file.h"
#include "utl/utl_defines.h"
}

#include <inttypes.h>
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
#define STYLE_FILE_PATH_LABEL  ("Style File Path")
#define MAP_SOURCE_LAYER_LABEL ("Map Source Layer")
#define VISIBLE_LABEL          ("Visible")
#define SELECT_ITEM_PROMPT     ("Select a screen, layout, window, or user application.")

#define INTEGER_TEXT_BYTES       (32)
#define LABEL_FORMAT_BYTES       (64)
#define HIERARCHY_LABEL_BYTES    (UTL_ARINC661_MAX_NAME_BYTES + LABEL_FORMAT_BYTES)
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
    STATUS_READ_FAILED,
    STATUS_INPUT_TOO_LARGE,
    STATUS_INPUT_EMPTY,
    STATUS_LOADED,
    STATUS_INVALID_XML,
    STATUS_DUPLICATE_SCREEN_ID,
    STATUS_DUPLICATE_LAYOUT_ID,
    STATUS_DUPLICATE_WINDOW_ID,
    STATUS_DUPLICATE_APPLICATION_ID,
    STATUS_DUPLICATE_EXTERNAL_SOURCE_ID,
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
    SELECTION_USER_APPLICATION
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
    UTL_ARINC661_XML_UNIT_INCH,
    UTL_ARINC661_XML_UNIT_MM,
    UTL_ARINC661_XML_UNIT_SCREEN,
    UTL_ARINC661_XML_UNIT_PIXEL
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
 * \return Status text.
 */
static const char_t* EditorStatusToString(int32_t status);

/*************************************************************************************************/
/**
 * \brief Joins a folder and filename, adding a separator when needed.
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
 * \brief Loads the clicked XML file from the configured folder.
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
 * \return STATUS_NONE when all identifiers are unique, or the duplicate's status.
 */
static Status_t CheckIdentifierUniqueness(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds the largest screen identifier used to choose the next one.
 *
 * \param[in]  editor   Editor state
 *
 * \return Largest screen identifier, or NO_IDENTIFIER if there are no screens.
 */
static int64_t GetMaxScreenIdentifier(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds the largest identifier within a screen's layouts.
 *
 * \param[in]  editor        Editor state
 * \param[in]  screenIndex   Owning screen index
 *
 * \return Largest layout identifier, or NO_IDENTIFIER if the screen has no layouts.
 */
static int64_t GetMaxLayoutIdentifier(const Arinc661_Configuration_Document_t* editor, uint32_t screenIndex);

/*************************************************************************************************/
/**
 * \brief Finds the largest window identifier used to choose the next one.
 *
 * \param[in]  editor   Editor state
 *
 * \return Largest window identifier, or NO_IDENTIFIER if there are no windows.
 */
static int64_t GetMaxWindowIdentifier(const Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds the largest user application identifier used to choose the next one.
 *
 * \param[in] editor   Editor state
 *
 * \return Largest application identifier, or NO_IDENTIFIER if there are none.
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
static bool_t DrawUnitField(const char_t* label, Utl_Arinc661_Unit_t* unit);

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
static void DrawUserApplicationInspector(Arinc661_Configuration_Document_t* editor);

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
static void DrawInspectorPane(Arinc661_Configuration_Document_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws screen and window geometry with selected hierarchy items highlighted.
 *
 * \param[in,out] editor   Editor state and hierarchy selection
 *
 * \retval        None
 */
static void DrawScenePane(Arinc661_Configuration_Document_t* editor);

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
        case STATUS_READ_FAILED:                     return "Could not read XML file.";
        case STATUS_INPUT_TOO_LARGE:                 return "The XML file exceeds the input buffer.";
        case STATUS_INPUT_EMPTY:                     return "The XML file is empty.";
        case STATUS_LOADED:                          return "Loaded XML file.";
        case STATUS_INVALID_XML:                     return "Invalid or unsupported display configuration XML.";
        case STATUS_DUPLICATE_SCREEN_ID:             return "Screen IDs must be unique in this document.";
        case STATUS_DUPLICATE_LAYOUT_ID:             return "Layout IDs must be unique within a screen.";
        case STATUS_DUPLICATE_WINDOW_ID:             return "Window IDs must be unique in this document.";
        case STATUS_DUPLICATE_APPLICATION_ID:        return "Application IDs must be unique in this document.";
        case STATUS_DUPLICATE_EXTERNAL_SOURCE_ID:    return "External source IDs must be unique in this document.";
        case STATUS_TOO_MANY_DOCUMENTS:              return "Close a document before opening another XML file.";
        default:
          LOGFATAL("Invalid Status_t!");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves folder paths that already end in either supported separator.
 */
static void JoinFolderPath(const char_t* folderPath, const char_t* fileName, char_t* filePath, uint32_t capacity)
{
    uint32_t folder_length;
    uint32_t file_name_length;
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
    if (needs_separator)
    {
        filePath[folder_length] = '\\';
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
    const Utl_Arinc661_Screen_t* screen;
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
 *  Reads the configured file and replaces the editor's typed configuration and selection.
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
                if (0 == ImStricmp(document->documentPath, file_path))
                {
                    document_index = index;
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
                is_configuration = Utl_Arinc661_ReadConfiguration(&document->configuration, document->inputBuffer, (uint64_t)document_byte_count, document->workspace, sizeof(document->workspace));
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
        editor->activeDocumentIndex    = available_index;
        editor->requestedDocumentIndex = available_index;
        editor->status                 = STATUS_NONE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps file output outside the in-memory utl_arinc661 module.
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
            output_byte_count = Utl_Arinc661_WriteConfiguration(&editor->configuration, editor->outputBuffer, sizeof(editor->outputBuffer), editor->workspace, sizeof(editor->workspace));
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
 *  Dispatches to the identifier search for the selected entity type.
 */
static bool_t NextIdentifier(const Arinc661_Configuration_Document_t* editor, SelectionKind_t kind, uint32_t screenIndex, int64_t* id)
{
    int64_t maximum_id;
    bool_t  has_next_id;

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
                break;
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
    Utl_Arinc661_Screen_t* screen;
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
        screen->pixelSize.units           = UTL_ARINC661_UNIT_PIXEL;
        screen->physicalDimensions.width  = DEFAULT_SCREEN_PHYSICAL_WIDTH;
        screen->physicalDimensions.height = DEFAULT_SCREEN_PHYSICAL_HEIGHT;
        screen->physicalDimensions.units  = UTL_ARINC661_UNIT_INCH;
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
    Utl_Arinc661_Layout_t* layout;
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
    Utl_Arinc661_Window_t*       window;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Screen_t* screen;
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
        window->scalingFactor.baseUnit = UTL_ARINC661_UNIT_SCREEN;
        window->scalingFactor.perUnit  = UTL_ARINC661_UNIT_PIXEL;
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
    Utl_Arinc661_UserApplication_t* application;
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
    const Utl_Arinc661_Screen_t*          screen;
    const Utl_Arinc661_Layout_t*          layout;
    const Utl_Arinc661_Window_t*          window;
    const Utl_Arinc661_UserApplication_t* application;
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
    bool_t                                is_screen_open;
    bool_t                                is_layout_open;

    add_layout_screen_index  = INVALID_HIERARCHY_INDEX;
    add_window_layout_index  = INVALID_HIERARCHY_INDEX;
    remove_screen_index      = INVALID_HIERARCHY_INDEX;
    remove_layout_index      = INVALID_HIERARCHY_INDEX;
    remove_window_index      = INVALID_HIERARCHY_INDEX;
    remove_application_index = INVALID_HIERARCHY_INDEX;
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
        (void)ImGui::TreeNodeEx(USER_APPLICATION_NODE_ID, flags, USER_APPLICATION_LABEL_FORMAT, application->applicationId);
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
        ImGui::PopID();
    }
    (void)ImGui::TreeNodeEx(ADD_USER_APPLICATION_LABEL, HIERARCHY_ADD_FLAGS);
    if (ImGui::IsItemClicked())
    {
        AddUserApplication(editor);
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
static bool_t DrawUnitField(const char_t* label, Utl_Arinc661_Unit_t* unit)
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
                    (*unit)     = (Utl_Arinc661_Unit_t)unit_index;
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
    Utl_Arinc661_Screen_t* screen;
    bool_t                 has_width_changed;
    bool_t                 has_height_changed;
    bool_t                 has_pixel_size_changed;

    screen = &editor->screens[editor->selectedIndex];

    ImGui::TextUnformatted(SCREEN_SECTION_LABEL);
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
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layouts have an identifier; their screen relationship is shown by the hierarchy.
 */
static void DrawLayoutInspector(Arinc661_Configuration_Document_t* editor)
{
    Utl_Arinc661_Layout_t* layout;

    layout = &editor->layouts[editor->selectedIndex];

    ImGui::TextUnformatted(LAYOUT_SECTION_LABEL);
    if (BeginPropertyTable(LAYOUT_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIdentifierField(editor, SELECTION_LAYOUT, layout->screenIndex, editor->selectedIndex, &layout->id));
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Window geometry and scaling values are edited as typed fields.
 */
static void DrawWindowInspector(Arinc661_Configuration_Document_t* editor)
{
    Utl_Arinc661_Window_t* window;

    window = &editor->windows[editor->selectedIndex];

    ImGui::TextUnformatted(WINDOW_SECTION_LABEL);
    if (BeginPropertyTable(WINDOW_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIdentifierField(editor, SELECTION_WINDOW, 0, editor->selectedIndex, &window->id));
        MarkDocumentDirty(editor, DrawTextField(NAME_LABEL, window->name, sizeof(window->name)));
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
}

/*************************************************************************************************/
/**
 * \par Details:
 *  User application attributes are edited in their stored order.
 */
static void DrawUserApplicationInspector(Arinc661_Configuration_Document_t* editor)
{
    Utl_Arinc661_UserApplication_t* application;

    application = &editor->userApplications[editor->selectedIndex];

    ImGui::TextUnformatted(USER_APPLICATION_SECTION_LABEL);
    if (BeginPropertyTable(USER_APPLICATION_TABLE_ID))
    {
        MarkDocumentDirty(editor, DrawIdentifierField(editor, SELECTION_USER_APPLICATION, 0, editor->selectedIndex, &application->applicationId));
        MarkDocumentDirty(editor, DrawIntegerField(WINDOW_ID_LABEL, &application->window));
        MarkDocumentDirty(editor, DrawTextField(DF_PATH_LABEL, application->dFPath, sizeof(application->dFPath)));
        MarkDocumentDirty(editor, DrawTextField(STYLE_FILE_PATH_LABEL, application->styleFilePath, sizeof(application->styleFilePath)));
        MarkDocumentDirty(editor, DrawBooleanField(MAP_SOURCE_LAYER_LABEL, &application->mapSourceLayer));
        MarkDocumentDirty(editor, DrawBooleanField(VISIBLE_LABEL, &application->visible));
        ImGui::EndTable();
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
    if (ImGui::BeginChild(HIERARCHY_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        ImGui::TextUnformatted(HIERARCHY_PANE_NAME);
        ImGui::Separator();
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
static void DrawInspectorPane(Arinc661_Configuration_Document_t* editor)
{
    if (ImGui::BeginChild(INSPECTOR_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        ImGui::TextUnformatted(INSPECTOR_PANE_NAME);
        ImGui::Separator();

        if (editor->isLoaded)
        {
            if (SELECTION_SCREEN == editor->selectedKind)
            {
                DrawScreenInspector(editor);
            }
            else if (SELECTION_LAYOUT == editor->selectedKind)
            {
                DrawLayoutInspector(editor);
            }
            else if (SELECTION_WINDOW == editor->selectedKind)
            {
                DrawWindowInspector(editor);
            }
            else if (SELECTION_USER_APPLICATION == editor->selectedKind)
            {
                DrawUserApplicationInspector(editor);
            }
            else
            {
                ImGui::TextWrapped("%s", SELECT_ITEM_PROMPT);
            }
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fits the combined screen widths and tallest screen inside the available canvas, selects clicked geometry, and draws the selection overlay.
 */
static void DrawScenePane(Arinc661_Configuration_Document_t* editor)
{
    const ImGuiStyle*            style;
    const Utl_Arinc661_Screen_t* screen;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Window_t* window;
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
    char_t                       window_label[UTL_ARINC661_MAX_NAME_BYTES + LABEL_FORMAT_BYTES];
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

    if (ImGui::BeginChild(SCENE_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
    {
        style     = &ImGui::GetStyle();
        draw_list = ImGui::GetWindowDrawList();

        ImGui::TextUnformatted(SCENE_PANE_NAME);
        ImGui::Separator();

        if (FALSE == editor->isLoaded)
        {
            ImGui::TextWrapped("%s", "No configuration loaded.");
        }
        else if (0 == editor->configuration.screenCount)
        {
            ImGui::TextWrapped("%s", EMPTY_SCENE_LABEL);
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
            content_size.y -= label_height;

            has_canvas_width  = (0.0f < content_size.x);
            has_canvas_height = (0.0f < content_size.y);
            has_canvas_space  = !!(has_canvas_width && has_canvas_height);
            if (FALSE == has_canvas_space)
            {
                ImGui::TextWrapped("%s", "Preview area is too small.");
            }
            else
            {
                (void)ImGui::InvisibleButton(SCENE_CANVAS_ID, canvas_size);
                is_canvas_clicked = ImGui::IsItemClicked();
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
                screen_position.y += (label_height + (unused_height / 2.0f));
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
                        screen_label_position.y = (screen_label_position.y - (screen_label_size.y + style->ItemSpacing.y));
                        is_screen_body_hit      = FALSE;
                        if (is_canvas_clicked)
                        {
                            is_screen_body_hit  = ImRect(screen_min, screen_max).Contains(mouse_position);
                            is_screen_label_hit = ImRect(screen_label_position, (screen_label_position + screen_label_size)).Contains(mouse_position);
                            is_screen_hit       = !!(is_screen_body_hit || is_screen_label_hit);
                            if (is_screen_hit)
                            {
                                editor->selectedKind  = SELECTION_SCREEN;
                                editor->selectedIndex = screen_index;
                            }
                        }
                        draw_list->AddText(screen_label_position, SCENE_TEXT_COLOR, screen_label);
                        draw_list->PushClipRect(screen_min, screen_max, TRUE);
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
                                area_min    = (screen_min + (area_offset * scale));
                                area_max    = (area_min + (area_size * scale));

                                if (is_screen_body_hit)
                                {
                                    is_window_hit = ImRect(area_min, area_max).Contains(mouse_position);
                                    if (is_window_hit)
                                    {
                                        editor->selectedKind  = SELECTION_WINDOW;
                                        editor->selectedIndex = window_index;
                                    }
                                }
                                is_selected_layout_window = FALSE;
                                if (SELECTION_LAYOUT == editor->selectedKind)
                                {
                                    is_selected_layout_window = (window->layoutIndex == editor->selectedIndex);
                                }
                                else if (SELECTION_WINDOW == editor->selectedKind)
                                {
                                    if (window_index == editor->selectedIndex)
                                    {
                                        selected_window_min      = area_min;
                                        selected_window_max      = area_max;
                                        has_selected_window_area = TRUE;
                                    }
                                }

                                draw_list->AddRectFilled(area_min, area_max, SCENE_WINDOW_COLOR);
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
                                draw_list->AddText((area_min + style->FramePadding), SCENE_TEXT_COLOR, window_label);
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
                draw_list->PopClipRect();
            }
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
 *  Initializes every editor field and its owned array storage.
 */
void Arinc661_Configuration_Initialize(Arinc661_Configuration_Editor_t* editor)
{
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
    }
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
}
