/*=================================================================================================
** 1. References
**===============================================================================================*/

/* Win32 and OpenGL setup adapted from imgui-docking/examples/example_win32_opengl3/main.cpp. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#include "arinc661_context.h"
#include "arinc661_icons.h"
#include "arinc661_definition_editor.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_diagnostics.h"
#include "os/os_file.h"
#include "os/os_malloc.h"
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <winternl.h>
#include <d3dkmthk.h>
#include <dwmapi.h>
#include <GL/gl.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* One page: shared folder and file list, one hierarchy holding both document kinds, one scene, one inspector. */
#define PAGE_WINDOW_NAME            ("ARINC 661")
#define PAGE_WINDOW_FLAGS           (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking)
#define FILE_MENU_LABEL             ("File")
#define NEW_CONFIGURATION_LABEL     ("New Configuration")
#define NEW_CONFIGURATION_SHORTCUT  ("Ctrl+N")
#define NEW_DEFINITION_LABEL        ("New Definition File")
#define NEW_DEFINITION_SHORTCUT     ("Ctrl+Shift+N")
#define SAVE_LABEL                  ("Save")
#define SAVE_SHORTCUT_LABEL         ("Ctrl+S")
#define SAVE_ALL_LABEL              ("Save All")
#define SAVE_ALL_SHORTCUT_LABEL     ("Ctrl+Shift+S")
#define CLOSE_LABEL                 ("Close")
#define CLOSE_SHORTCUT_LABEL        ("Ctrl+W")
#define REFRESH_LABEL               ("Refresh")
#define FILE_CONTROLS_HEADER        ("Files")
#define FOLDER_PATH_LABEL           ("Folder")
#define FOLDER_INPUT_ID             ("##pageFolderPath")
#define FILE_LIST_ID                ("##pageFiles")
#define FILE_LIST_ROWS              (4)
#define CONFIGURATION_FILE_FORMAT   ("[Configuration]    %s")
#define DEFINITION_FILE_FORMAT      ("[Definition File]  %s")
#define NO_FILES_LABEL              ("No configuration or Definition Files in this folder.")
#define FOLDER_UNREADABLE_LABEL     ("Folder could not be read.")
#define HIERARCHY_PANE_NAME         ("Hierarchy")
#define CONFIGURATION_SECTION_LABEL ("Configuration")
#define DEFINITION_SECTION_LABEL    ("Definition Files")
#define CONFIGURATION_SECTION_ID    ("##configurationSection")
#define DEFINITION_SECTION_ID       ("##definitionSection")
#define PANE_TABLE_ID               ("PagePanes")
#define PANE_TABLE_FLAGS            (ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoHostExtendY)
#define PANE_FLAGS                  (ImGuiChildFlags_AlwaysUseWindowPadding)
#define READY_LABEL                 ("Ready")
#define SHORTCUT_FLAGS              (ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive)
#define EDIT_MENU_LABEL             ("Edit")
#define VIEW_MENU_LABEL             ("View")
#define HELP_MENU_LABEL             ("Help")
#define UNDO_LABEL                  ("Undo")
#define REDO_LABEL                  ("Redo")
#define DUPLICATE_LABEL             ("Duplicate")
#define DELETE_LABEL                ("Delete")
#define ADD_LAYER_LABEL             ("Add Layer")
#define ADD_WIDGET_LABEL            ("Add Widget")
#define ADD_EXTENSION_LABEL         ("Add Extension")
#define ADD_SCREEN_LABEL            ("Add Screen")
#define ADD_LAYOUT_LABEL            ("Add Layout")
#define ADD_WINDOW_LABEL            ("Add Window")
#define ADD_APPLICATION_LABEL       ("Add User Application")
#define GRID_LABEL                  ("Grid")
#define SNAP_LABEL                  ("Snap To Millimetre")
#define FIT_LABEL                   ("Fit Layer")
#define HELP_WINDOW_LABEL           ("Help")
#define HELP_CONTEXT_LABEL          ("Help On Selection")
#define HELP_BROWSER_LABEL          ("Open Help Reference")
#define UNDO_SHORTCUT_LABEL         ("Ctrl+Z")
#define REDO_SHORTCUT_LABEL         ("Ctrl+Y")
#define DUPLICATE_SHORTCUT_LABEL    ("Ctrl+D")
#define DELETE_SHORTCUT_LABEL       ("Del")
#define GRID_SHORTCUT_LABEL         ("G")
#define SNAP_SHORTCUT_LABEL         ("S")
#define FIT_SHORTCUT_LABEL          ("F")
#define HELP_SHORTCUT_LABEL         ("F1")
#define HELP_BROWSER_SHORTCUT_LABEL ("Shift+F1")
#define UNDO_CHORD                  (ImGuiMod_Ctrl | ImGuiKey_Z)
#define REDO_CHORD                  (ImGuiMod_Ctrl | ImGuiKey_Y)
#define DUPLICATE_CHORD             (ImGuiMod_Ctrl | ImGuiKey_D)
#define HELP_CHORD                  (ImGuiKey_F1)
#define HELP_BROWSER_CHORD          (ImGuiMod_Shift | ImGuiKey_F1)
#define TOOLBAR_ID                  ("##toolbar")
#define HELP_PAGE_PATH              ("docs\\arinc661_help.html")
#define HELP_CHM_PATH               ("docs\\arinc661.chm")
#define HELP_LIBRARY_NAME           ("hhctrl.ocx")
#define HELP_FUNCTION_NAME          ("HtmlHelpA")
#define HELP_TOPIC_FORMAT           ("%s%s::/%s.htm")
#define HELP_DEFAULT_TOPIC          ("Getting-Started")
#define HH_DISPLAY_TOPIC_COMMAND    (0x0000U)
#define HH_DISPLAY_TOC_COMMAND      (0x0001U)
#define HH_DISPLAY_INDEX_COMMAND    (0x0002U)
#define HH_DISPLAY_SEARCH_COMMAND   (0x0003U)
#define HELP_CONTENTS_LABEL         ("Contents")
#define HELP_INDEX_LABEL            ("Index")
#define HELP_SEARCH_LABEL           ("Search")
#define HELP_REFERENCE_LABEL        ("Reference (in-app)")
#define HELP_SEARCH_HINT            ("Search widgets, extensions, FACE elements")
#define HELP_LIST_WIDTH             (260.0f)
#define HELP_WINDOW_SIZE            (ImVec2(900.0f, 600.0f))
#define HELP_GETTING_STARTED        ("Getting Started")
#define HELP_PARAMETERS_LABEL       ("Parameters")
#define HELP_CHANGE_COLUMN          ("Change")
#define HELP_NAME_COLUMN            ("Name")
#define HELP_DESCRIPTION_COLUMN     ("Description")
#define HELP_FACE_PREFIX            ("FACE ")
#define MAX_HELP_FILTER_BYTES       (64)
#define PROBLEMS_SECTION_FORMAT     ("Problems: %u errors, %u warnings###problems")
#define NO_PROBLEMS_LABEL           ("No problems across the open documents.")
#define OPEN_REFERENCED_LABEL       ("Open referenced Definition Files")
#define RECHECK_HINT_LABEL          ("Click a problem to go to it.")
#define PROBLEM_ERROR_PREFIX        ("[error]")
#define PROBLEM_WARNING_PREFIX      ("[warning]")
#define PROBLEMS_LIST_ROWS          (4.5f)
#define PROBLEM_ERROR_COLOR         (ImVec4(1.0f, 0.45f, 0.45f, 1.0f))
#define PROBLEM_WARNING_COLOR       (ImVec4(1.0f, 0.8f, 0.4f, 1.0f))
#define CONTEXT_BAR_FORMAT          ("Configuration: %s   |   Definition File: %s")
#define CONTEXT_BAR_GAP             (40.0f)
#define CONTEXT_CRUMB_BYTES         (512)
#define DOCKSPACE_ID                ("##arinc661 dockspace")
#define PANEL_FILES                 ("Files")
#define PANEL_PROBLEMS              ("Problems")
#define PANEL_HIERARCHY             ("Hierarchy")
#define PANEL_PALETTE               ("Palette")
#define PANEL_SCENE                 ("Scene")
#define PANEL_INSPECTOR             ("Inspector")
#define PANEL_LOG                   ("Log")
#define WINDOW_MENU_LABEL           ("Window")
#define RESET_LAYOUT_LABEL          ("Reset Layout")
#define PANEL_COUNT                 (7)
#define PALETTE_FILTER_HINT         ("Filter widgets")
#define PALETTE_PAYLOAD_ID          ("A661_PALETTE_TYPE")
#define PALETTE_HINT_LABEL          ("Click to add to the selection; drag onto a container, a layer, or the scene.")
#define PALETTE_OTHER_CATEGORY      ("Other")
#define PALETTE_EXTENSIONS_CATEGORY ("Extensions")
#define LOG_CAPACITY                (256)
#define LOG_LINE_BYTES              (320)
#define LOG_TIME_FORMAT             "[%02d:%02d:%02d] "
#define LOG_CLEAR_LABEL             ("Clear")
#define LOG_AUTOSCROLL_LABEL        ("Auto-scroll")
#define LAYOUT_LEFT_FRACTION        (0.22f)
#define LAYOUT_RIGHT_FRACTION       (0.26f)
#define LAYOUT_BOTTOM_FRACTION      (0.24f)
#define LAYOUT_PALETTE_FRACTION     (0.40f)
#define HELP_WRAP_WIDTH             (420.0f)
#define NEW_APPLICATION_LABEL       ("New Application (window + user application + Definition File)")
#define NEW_APPLICATION_SHORT_LABEL ("New Application")
#define SAVED_WITH_ERRORS_FORMAT    ("Saved with %u unresolved errors; see Problems.")
#define SAVED_WITH_ERRORS_BYTES     (96)
#define SCREENSHOT_OPTION           ("--screenshot")
#define OPEN_DEFINITION_OPTION      ("--definition")
#define OPEN_CONFIGURATION_OPTION   ("--configuration")
#define FOCUS_DEFINITION_OPTION     ("--focus-definition")
#define SELECT_OPTION               ("--select")
#define FRAMES_OPTION               ("--frames")
#define SCREENSHOT_DEFAULT_FRAMES   (12)
#define BMP_HEADER_BYTES            (54)
#define NEW_CONFIGURATION_CHORD     (ImGuiMod_Ctrl | ImGuiKey_N)
#define NEW_DEFINITION_CHORD        (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_N)
#define SAVE_CHORD                  (ImGuiMod_Ctrl | ImGuiKey_S)
#define SAVE_ALL_CHORD              (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)
#define CLOSE_CHORD                 (ImGuiMod_Ctrl | ImGuiKey_W)
#define DEFAULT_FOLDER_PATH         ("C:\\dev\\xml\\examples")
#define MAX_FOLDER_PATH_BYTES       (512)
#define MAX_FILE_NAME_BYTES         (260)
#define MAX_FILE_PATH_BYTES         (MAX_FOLDER_PATH_BYTES + MAX_FILE_NAME_BYTES + 1)
#define MAX_PAGE_FILES              (256)
#define FILE_HEAD_BYTES             (4096)
#define XML_EXTENSION               (".xml")
#define BINARY_EXTENSION            (".bin")
#define CONFIGURATION_ROOT_ELEMENT  ("<configuration")
#define FILE_SCAN_INTERVAL_SECONDS  (2.0)

#define ASSERT(expression) LOGASSERT(expression)
#define MOVE_SIZE_TIMER_ID (1U)
#define MOVE_SIZE_TIMER_INTERVAL_MS (16U)

#define OPENGL_MAJOR_VERSION_PARAMETER (0x821B)
#define OPENGL_MINOR_VERSION_PARAMETER (0x821C)
#define OPENGL_VERSION_3_0 (300)
#define WGL_MAJOR_VERSION_ATTRIBUTE (0x2091)
#define WGL_MINOR_VERSION_ATTRIBUTE (0x2092)
#define WGL_PROFILE_MASK_ATTRIBUTE (0x9126)
#define WGL_CORE_PROFILE_BIT (0x0001)

/** Dockable panels, indexed into panelOpen. */
typedef enum Panel_e
{
    PANEL_INDEX_FILES, PANEL_INDEX_PROBLEMS, PANEL_INDEX_HIERARCHY, PANEL_INDEX_PALETTE, PANEL_INDEX_SCENE, PANEL_INDEX_INSPECTOR, PANEL_INDEX_LOG
} Panel_t;

static const char_t* const c_panelNames[PANEL_COUNT] = {PANEL_FILES, PANEL_PROBLEMS, PANEL_HIERARCHY, PANEL_PALETTE, PANEL_SCENE, PANEL_INSPECTOR, PANEL_LOG};

/** FACE H.3 element help shown beside the ARINC 661 reference. */
typedef struct FaceTopic_s
{
    const char_t* name;        ///< Element name
    const char_t* description; ///< What it is and how it relates to the others
} FaceTopic_t;

static const FaceTopic_t c_faceTopics[] =
{
    {"Screen", "A physical display surface (FACE Technical Standard, Appendix H.3.4): pixelSize gives its resolution, physicalDimensions its size in mm or inches. A screen holds layouts. The editor uses pixelSize over physicalDimensions as the pixel density for previews when a window has no pixel-per-mm scaling factor."},
    {"Layout", "A set of windows that are shown together on one screen (H.3.6). Select a layout to see and add its windows; a window belongs to exactly one layout."},
    {"Window", "A rectangle of a screen, in pixels (H.3.2): pixelArea is its origin and size, scalingFactor converts ARINC 661 screen units to pixels. User applications target windows by id. Drag a window in the scene to move it, drag its corners to resize."},
    {"UserApplication", "An ARINC 661 User Application (H.3.1): applicationId must equal the ApplicationId inside its Definition File, window names the window its layers appear in, dFPath names the Definition File. The inspector checks the identifier against the open Definition File and previews its layers inside the window."},
    {"ExternalSource", "An external video or graphics source (H.3.7) that an ExternalSource widget can show by SourceReference."},
    {"Configuration", "The FACE display management configuration file (H.3): screens, layouts, windows, user applications, and external sources. Create one with File > New Configuration, add a screen, a layout, a window, then a user application pointing at a Definition File."}
};

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Device context owned by an ImGui platform viewport.
 */
typedef struct WglWindowData_s
{
    HDC deviceContext; ///< Win32 device context for the viewport.
} WglWindowData_t;

/*************************************************************************************************/
/**
 * \brief Window thread and frame presentation state.
 */
typedef struct WindowThreadState_s
{
    HWND                   windowHandle;        ///< Main Win32 window.
    HANDLE                 readyEvent;          ///< Signals when rendering setup is complete.
    HANDLE                 presentStoppedEvent; ///< Signals when the presenter releases its device context.
    HGLRC                  presentContext;      ///< OpenGL context used by the presenter thread.
    Arinc661_Configuration_Editor_t* configurationEditor; ///< FACE display management configuration editor.
    Arinc661_Definition_Editor_t*    definitionEditor;    ///< ARINC 661 Definition File editor.
    Arinc661_Context_t*              context;             ///< Shared application context across both editors.
    bool_t                           isHelpOpen;          ///< TRUE while the Help window is shown.
    char_t                           helpFilter[MAX_HELP_FILTER_BYTES]; ///< Search text of the Help window.
    char_t                           helpTopic[ARINC661_MAX_NAME_BYTES]; ///< Topic the Help window shows.
    char_t                           saveNote[SAVED_WITH_ERRORS_BYTES];   ///< Note after a configuration save with errors, or empty.
    bool                             panelOpen[PANEL_COUNT];              ///< Visibility of each dockable panel, in PANEL_ order.
    bool_t                           hasLayout;                           ///< TRUE once the default dock layout was built.
    bool_t                           resetsLayout;                        ///< TRUE to rebuild the default layout next frame.
    char_t                           paletteFilter[MAX_HELP_FILTER_BYTES]; ///< Palette filter text.
    char_t                           logLines[LOG_CAPACITY][LOG_LINE_BYTES]; ///< Status history ring.
    uint32_t                         logCount;                            ///< Lines written to the ring.
    char_t                           lastStatus[LOG_LINE_BYTES];          ///< Last status text appended.
    bool                             logAutoScroll;                       ///< TRUE to keep the log scrolled to the end.
    bool_t                           isDefinitionActive;  ///< TRUE while the Definition File section has the focus.
    bool_t                           hasEditors;          ///< TRUE once both editors exist.
    bool_t                           isFolderReadable;    ///< TRUE when the last scan could read the folder.
    float64_t                        lastScanTime;        ///< ImGui time of the last folder scan.
    uint32_t                         fileCount;           ///< Entries found by the last folder scan.
    char_t                           folderPath[MAX_FOLDER_PATH_BYTES];                ///< Folder shared by both editors.
    char_t                           fileNames[MAX_PAGE_FILES][MAX_FILE_NAME_BYTES];  ///< Names found by the last folder scan.
    bool_t                           fileIsDefinition[MAX_PAGE_FILES];                  ///< TRUE for a Definition File, FALSE for a configuration.
    float32_t              mainScale;           ///< Initial display scale.
    SRWLOCK                presentLock;         ///< Protects geometry, drawing, and buffer swaps.
    CONDITION_VARIABLE     presentCondition;    ///< Wakes the window thread after geometry is acknowledged.
    RECT                   rcClient;            ///< Current client rectangle.
    BOOL                   fPresentReady;       ///< TRUE after the first presented frame.
    BOOL                   fFrameReady;         ///< TRUE when a new frame can be presented.
    BOOL                   fGeometryPending;    ///< TRUE while geometry acknowledgement is pending.
    bool_t                 isInitialized;       ///< TRUE when rendering setup succeeds.
    std::atomic<bool_t>    isDone;              ///< TRUE after the main window is closed.
} WindowThreadState_t;

/*************************************************************************************************/
/**
 * \brief WGL extension signature for creating an OpenGL context.
 */
typedef HGLRC(WINAPI* WglCreateContextAttribsFunction_t)(HDC, HGLRC, const int*);

/*************************************************************************************************/
/**
 * \brief WGL extension signature for setting the swap interval.
 */
typedef BOOL(WINAPI* WglSwapIntervalFunction_t)(int);

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Rendering state shared with Dear ImGui platform viewport callbacks.
 */
static HGLRC           g_renderContext;
static WglWindowData_t g_mainWindow;
static const char_t* g_screenshotPath;        ///< BMP written after the capture frame, or NULL.
static const char_t* g_openDefinition;        ///< Definition File opened at start, or NULL.
static const char_t* g_openConfiguration;     ///< Configuration opened at start, or NULL.
static const char_t* g_selectName;            ///< Widget name selected after opening, or NULL.
static bool_t        g_focusDefinition;       ///< TRUE to focus the Definition File section at start.
static uint32_t      g_captureFrame;          ///< Frame on which the screenshot is taken.
static uint32_t      g_frameCount;            ///< Frames rendered so far.

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Opens the display source used to wait for vertical blank.
 *
 * \param[in]  deviceContext        Window device context on the display
 * \param[out] verticalBlankEvent   Adapter and display source for the wait
 *
 * \retval     None
 */
static void OpenVerticalBlankEvent(HDC deviceContext, D3DKMT_WAITFORVERTICALBLANKEVENT* verticalBlankEvent);

/*************************************************************************************************/
/**
 * \brief Starts a new Dear ImGui frame.
 *
 * \retval        None
 */
static void BeginFrame(void);

/*************************************************************************************************/
/**
 * \brief Renders the Dear ImGui frame into the main window's back buffer.
 *
 * \param[in,out] window   Window and presentation state
 *
 * \retval        None
 */
static void EndFrame(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws a frame under the presentation lock.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawFrame(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Classifies one folder entry as a configuration or a Definition File by content.
 *
 * \param[in]     fileName   Entry name
 * \param[in,out] context    WindowThreadState_t passed as an opaque pointer
 *
 * \retval        None
 */
static void ClassifyFolderFile(const char_t* fileName, void* context);

/*************************************************************************************************/
/**
 * \brief Writes the current back buffer as a 24-bit BMP and asks the window to close.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void CaptureScreenshot(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Reads the command line options for automated runs.
 *
 * \param[in] argc   Argument count
 * \param[in] argv   Arguments
 *
 * \retval    None
 */
static void ParseOptions(int argc, char** argv);

/*************************************************************************************************/
/**
 * \brief Rescans the shared folder and refreshes both editors' file knowledge.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void ScanFolder(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the main menu bar and handles its shortcuts.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawPageMenu(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the icon toolbar for the focused section.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawToolbar(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Opens the Help window on a topic.
 *
 * \param[in,out] window   Window and editor state
 * \param[in]     topic    Topic name, or NULL to keep the current topic
 *
 * \retval        None
 */
static void ShowHelp(WindowThreadState_t* window, const char_t* topic);

/*************************************************************************************************/
/**
 * \brief Shows the selection's page in Windows Help.
 *
 * \param[in] window   Window and editor state
 *
 * \retval    None
 */
static void OpenHelpInBrowser(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Shows the compiled Windows help file through the HTML Help engine.
 *
 * \param[in] window    Window and editor state
 * \param[in] command   HH_DISPLAY_TOPIC_COMMAND for the selection's page, or a contents, index, or search command
 *
 * \retval    None
 */
static void OpenWindowsHelp(WindowThreadState_t* window, uint32_t command);

/*************************************************************************************************/
/**
 * \brief Draws the searchable Help window when it is open.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawHelpWindow(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Names the help topic for the focused section's selection.
 *
 * \param[in] window   Window and editor state
 *
 * \retval    const char_t*   Topic name, or NULL.
 */
static const char_t* SelectionTopic(const WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the folder path and the file list of both kinds.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawPageFiles(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the cross-document problem list; clicking a row navigates to it.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawPageProblems(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Builds the default dock layout once, or again after a reset.
 *
 * \param[in,out] window        Window and editor state
 * \param[in]     dockspaceId   Dockspace node
 *
 * \retval        None
 */
static void BuildDefaultLayout(WindowThreadState_t* window, ImGuiID dockspaceId);

/*************************************************************************************************/
/**
 * \brief Draws the widget palette: every type by category, clickable and draggable.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawPalette(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Appends a status line to the log when it changes.
 *
 * \param[in,out] window   Window and editor state
 * \param[in]     text     Status text
 *
 * \retval        None
 */
static void AppendLog(WindowThreadState_t* window, const char_t* text);

/*************************************************************************************************/
/**
 * \brief Draws the status history panel.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawLog(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the status line with notes and the context breadcrumbs.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawStatusBar(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Saves the focused section's active document and notes unresolved errors.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void SaveActive(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws both hierarchies, giving the focus to the clicked section.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawPageHierarchy(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the single page: menu, toolbar, files, hierarchy, scene, inspector, dialogs.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawEditors(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Creates a Win32 OpenGL context for the main or platform window.
 *
 * \param[in]     windowHandle   Win32 window handle
 * \param[in,out] windowData     Window rendering data
 *
 * \retval    TRUE    if rendering setup succeeds.
 * \retval    FALSE   otherwise.
 */
static bool_t CreateDeviceWGL(HWND windowHandle, WglWindowData_t* windowData);

/*************************************************************************************************/
/**
 * \brief Releases a Win32 window device context.
 *
 * \param[in]     windowHandle   Win32 window handle
 * \param[in,out] windowData     Window rendering data
 *
 * \retval        None
 */
static void CleanupDeviceWGL(HWND windowHandle, WglWindowData_t* windowData);

/*************************************************************************************************/
/**
 * \brief Creates renderer data for an ImGui platform viewport.
 *
 * \param[in,out] viewport   ImGui platform viewport
 *
 * \retval        None
 */
static void HookRendererCreateWindow(ImGuiViewport* viewport);

/*************************************************************************************************/
/**
 * \brief Releases renderer data for an ImGui platform viewport.
 *
 * \param[in,out] viewport   ImGui platform viewport
 *
 * \retval        None
 */
static void HookRendererDestroyWindow(ImGuiViewport* viewport);

/*************************************************************************************************/
/**
 * \brief Activates the shared OpenGL context for a platform viewport.
 *
 * \param[in] viewport   ImGui platform viewport
 * \param[in] data       Unused renderer callback data
 *
 * \retval    None
 */
static void HookPlatformRenderWindow(ImGuiViewport* viewport, void* data);

/*************************************************************************************************/
/**
 * \brief Presents a platform viewport.
 *
 * \param[in] viewport   ImGui platform viewport
 * \param[in] data       Unused renderer callback data
 *
 * \retval    None
 */
static void HookRendererSwapBuffers(ImGuiViewport* viewport, void* data);

/*************************************************************************************************/
/**
 * \brief Routes Win32 input and window events to ImGui and the renderer.
 *
 * \param[in] windowHandle    Win32 window handle
 * \param[in] message         Win32 message identifier
 * \param[in] wordParameter   Win32 message word parameter
 * \param[in] longParameter   Win32 message long parameter
 *
 * \retval    LRESULT   Win32 message result.
 */
static LRESULT WINAPI WindowProcedure(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter);

/*************************************************************************************************/
/**
 * \brief Creates the main window and pumps its messages on a separate thread.
 *
 * \param[in,out] parameter   WindowThreadState_t passed as an opaque pointer
 *
 * \retval    0   if the window thread exits normally.
 * \retval    1   if rendering setup fails.
 */
static DWORD WINAPI WindowThread(LPVOID parameter);

/* The Win32 backend exposes this handler without declaring it in its public header. */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter);
/* OpenGL32 exports this entry point, but the Windows SDK does not declare it. */
extern "C" BOOL WINAPI wglSwapBuffers(HDC deviceContext);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Opens the adapter and display source for the window's display.
 */
static void OpenVerticalBlankEvent(HDC deviceContext, D3DKMT_WAITFORVERTICALBLANKEVENT* verticalBlankEvent)
{
    D3DKMT_OPENADAPTERFROMHDC open_adapter;

    memset(&open_adapter, 0, sizeof(open_adapter));
    open_adapter.hDc = deviceContext;
    LOGASSERT(NT_SUCCESS(D3DKMTOpenAdapterFromHdc(&open_adapter)));

    memset(verticalBlankEvent, 0, sizeof((*verticalBlankEvent)));
    verticalBlankEvent->hAdapter      = open_adapter.hAdapter;
    verticalBlankEvent->VidPnSourceId = open_adapter.VidPnSourceId;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Starts the OpenGL, Win32, and Dear ImGui frame state.
 */
static void BeginFrame(void)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Renders Dear ImGui draw data into the main window and presents platform windows.
 */
static void EndFrame(WindowThreadState_t* window)
{
    ImGui::Render();
    glViewport(0, 0, window->rcClient.right - window->rcClient.left, window->rcClient.bottom - window->rcClient.top);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (0 != (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        wglMakeCurrent(g_mainWindow.deviceContext, g_renderContext);
    }
    glFinish();
    window->fFrameReady = TRUE;
    g_frameCount += 1;
    if ((NULL != g_screenshotPath) && (g_frameCount == g_captureFrame))
    {
        CaptureScreenshot(window);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  OpenGL rows run bottom-up, which is the BMP order, so the pixels go straight to the
 *  file as BGR with each row padded to four bytes.
 */
static void CaptureScreenshot(WindowThreadState_t* window)
{
    byte_t*  pixels;
    byte_t*  row;
    byte_t   header[BMP_HEADER_BYTES];
    FILE*    file;
    int32_t  width;
    int32_t  height;
    int32_t  x;
    int32_t  y;
    uint32_t row_bytes;
    uint32_t image_bytes;
    uint32_t file_bytes;

    width  = (window->rcClient.right - window->rcClient.left);
    height = (window->rcClient.bottom - window->rcClient.top);
    if ((0 < width) && (0 < height))
    {
        pixels = (byte_t*)malloc((size_t)width * (size_t)height * 4U);
        LOGASSERT(NULL != pixels);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        row_bytes   = (((uint32_t)width * 3U + 3U) & ~3U);
        image_bytes = (row_bytes * (uint32_t)height);
        file_bytes  = (BMP_HEADER_BYTES + image_bytes);
        (void)memset(header, 0, sizeof(header));
        header[0] = 'B';
        header[1] = 'M';
        (void)memcpy(&header[2], &file_bytes, 4);
        header[10] = BMP_HEADER_BYTES;
        header[14] = 40;
        (void)memcpy(&header[18], &width, 4);
        (void)memcpy(&header[22], &height, 4);
        header[26] = 1;
        header[28] = 24;
        (void)memcpy(&header[34], &image_bytes, 4);
        file = fopen(g_screenshotPath, "wb");
        if (NULL != file)
        {
            (void)fwrite(header, 1, sizeof(header), file);
            row = (byte_t*)malloc(row_bytes);
            LOGASSERT(NULL != row);
            for (y = 0; y < height; ++y)
            {
                (void)memset(row, 0, row_bytes);
                for (x = 0; x < width; ++x)
                {
                    row[(x * 3) + 0] = pixels[((y * width) + x) * 4 + 2];
                    row[(x * 3) + 1] = pixels[((y * width) + x) * 4 + 1];
                    row[(x * 3) + 2] = pixels[((y * width) + x) * 4 + 0];
                }
                (void)fwrite(row, 1, row_bytes, file);
            }
            free(row);
            (void)fclose(file);
        }
        free(pixels);
    }
    (void)::PostMessageA(window->windowHandle, WM_CLOSE, 0, 0);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  --screenshot path.bmp [--frames n] [--definition name] [--configuration name]
 *  [--focus-definition] [--select widgetName].
 */
static void ParseOptions(int argc, char** argv)
{
    int index;

    g_captureFrame = SCREENSHOT_DEFAULT_FRAMES;
    for (index = 1; index < argc; ++index)
    {
        if ((0 == strcmp(argv[index], SCREENSHOT_OPTION)) && ((index + 1) < argc))
        {
            index += 1;
            g_screenshotPath = argv[index];
        }
        else if ((0 == strcmp(argv[index], OPEN_DEFINITION_OPTION)) && ((index + 1) < argc))
        {
            index += 1;
            g_openDefinition = argv[index];
        }
        else if ((0 == strcmp(argv[index], OPEN_CONFIGURATION_OPTION)) && ((index + 1) < argc))
        {
            index += 1;
            g_openConfiguration = argv[index];
        }
        else if ((0 == strcmp(argv[index], SELECT_OPTION)) && ((index + 1) < argc))
        {
            index += 1;
            g_selectName = argv[index];
        }
        else if ((0 == strcmp(argv[index], FRAMES_OPTION)) && ((index + 1) < argc))
        {
            index += 1;
            g_captureFrame = (uint32_t)atoi(argv[index]);
        }
        else if (0 == strcmp(argv[index], FOCUS_DEFINITION_OPTION))
        {
            g_focusDefinition = TRUE;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Classifies one file of the folder by its content: a binary or XML Definition File, or a
 *  FACE display management configuration. Other files are ignored. Definition Files are also
 *  offered to the configuration editor for user application dFPath choices.
 */
static void ClassifyFolderFile(const char_t* fileName, void* context)
{
    WindowThreadState_t* window;
    byte_t               head[FILE_HEAD_BYTES];
    char_t               file_path[MAX_FILE_PATH_BYTES];
    const char_t*        text;
    int32_t              byte_count;
    size_t               name_length;
    size_t               index;
    bool_t               has_known_extension;
    bool_t               is_definition;
    bool_t               is_configuration;

    window              = (WindowThreadState_t*)context;
    name_length         = strlen(fileName);
    has_known_extension = FALSE;
    if ((strlen(XML_EXTENSION) < name_length) && (0 == _stricmp(fileName + name_length - strlen(XML_EXTENSION), XML_EXTENSION)))
    {
        has_known_extension = TRUE;
    }
    if ((strlen(BINARY_EXTENSION) < name_length) && (0 == _stricmp(fileName + name_length - strlen(BINARY_EXTENSION), BINARY_EXTENSION)))
    {
        has_known_extension = TRUE;
    }
    if ((FALSE == has_known_extension) || (window->fileCount >= MAX_PAGE_FILES) || (name_length >= MAX_FILE_NAME_BYTES) || ((strlen(window->folderPath) + 1 + name_length) >= MAX_FILE_PATH_BYTES))
    {
        return;
    }
    (void)snprintf(file_path, sizeof(file_path), "%s/%s", window->folderPath, fileName);
    byte_count = OS_File_ReadFile(file_path, head, sizeof(head));
    if (0 >= byte_count)
    {
        return;
    }
    is_definition    = Arinc661_Definition_IsDefinitionFile(head, (uint64_t)byte_count);
    is_configuration = FALSE;
    if (FALSE == is_definition)
    {
        // Find the document element and require the FACE configuration root
        text  = (const char_t*)head;
        index = 0;
        while (index < (size_t)byte_count)
        {
            if ('<' != text[index])
            {
                index += 1;
                continue;
            }
            if (((index + 1) < (size_t)byte_count) && (('?' == text[index + 1]) || ('!' == text[index + 1])))
            {
                while ((index < (size_t)byte_count) && ('>' != text[index]))
                {
                    index += 1;
                }
                continue;
            }
            break;
        }
        is_configuration = !!(((index + strlen(CONFIGURATION_ROOT_ELEMENT)) < (size_t)byte_count) && (0 == memcmp(&text[index], CONFIGURATION_ROOT_ELEMENT, strlen(CONFIGURATION_ROOT_ELEMENT))));
    }
    if (is_definition || is_configuration)
    {
        strncpy_s(window->fileNames[window->fileCount], sizeof(window->fileNames[0]), fileName, _TRUNCATE);
        window->fileIsDefinition[window->fileCount] = is_definition;
        window->fileCount += 1;
        if (is_definition)
        {
            Arinc661_Configuration_AddDefinitionFile(window->configurationEditor, fileName);
            if (window->context->definitionFileCount < ARINC661_CONTEXT_MAX_DEFINITION_FILES)
            {
                strncpy_s(window->context->definitionFiles[window->context->definitionFileCount], sizeof(window->context->definitionFiles[0]), fileName, _TRUNCATE);
                window->context->definitionFileCount += 1;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rescans the shared folder, which refreshes the file list and the Definition File choices
 *  of the configuration editor. Both editors load and save relative to this folder.
 */
static void ScanFolder(WindowThreadState_t* window)
{
    window->fileCount    = 0;
    window->lastScanTime = ImGui::GetTime();
    Arinc661_Configuration_ClearDefinitionFiles(window->configurationEditor);
    window->context->definitionFileCount = 0;
    Arinc661_Configuration_SetFolderPath(window->configurationEditor, window->folderPath);
    Arinc661_Definition_SetFolderPath(window->definitionEditor, window->folderPath);
    window->isFolderReadable = FALSE;
    if ('\0' != window->folderPath[0])
    {
        window->isFolderReadable = OS_File_EnumerateFiles(window->folderPath, ClassifyFolderFile, window);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  One row of icon buttons for the focused section: file actions, undo and redo, then the
 *  section's add, duplicate, and delete actions, view toggles, and help.
 */
static void DrawToolbar(WindowThreadState_t* window)
{
    const char_t* new_label;
    const char_t* new_shortcut;
    bool_t has_active;
    bool_t is_dirty;
    bool_t has_free;
    bool_t can_undo;
    bool_t can_redo;
    bool_t has_selection;

    if (window->isDefinitionActive)
    {
        has_active = Arinc661_Definition_HasActiveDocument(window->definitionEditor, &is_dirty, &has_free);
        Arinc661_Definition_History(window->definitionEditor, &can_undo, &can_redo);
        has_selection = (NULL != Arinc661_Definition_SelectedType(window->definitionEditor));
    }
    else
    {
        has_active = Arinc661_Configuration_HasActiveDocument(window->configurationEditor, &is_dirty, &has_free);
        Arinc661_Configuration_History(window->configurationEditor, &can_undo, &can_redo);
        has_selection = (NULL != Arinc661_Configuration_SelectedElement(window->configurationEditor));
    }
    new_label = NEW_CONFIGURATION_LABEL;
    new_shortcut = NEW_CONFIGURATION_SHORTCUT;
    if (window->isDefinitionActive)
    {
        new_label    = NEW_DEFINITION_LABEL;
        new_shortcut = NEW_DEFINITION_SHORTCUT;
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_NEW, new_label, new_shortcut, has_free, FALSE))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_CreateDocument(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_CreateDocument(window->configurationEditor);
        }
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_APPLICATION, NEW_APPLICATION_SHORT_LABEL, NULL, NULL != Arinc661_Configuration_DocumentName(window->configurationEditor, window->configurationEditor->activeDocumentIndex), FALSE))
    {
        (void)Arinc661_Context_NewApplication(window->context);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_SAVE, SAVE_LABEL, SAVE_SHORTCUT_LABEL, has_active, FALSE))
    {
        SaveActive(window);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_SAVE_ALL, SAVE_ALL_LABEL, SAVE_ALL_SHORTCUT_LABEL, is_dirty, FALSE))
    {
        Arinc661_Configuration_SaveAllDocuments(window->configurationEditor);
        Arinc661_Definition_SaveAllDocuments(window->definitionEditor);
    }
    Arinc661_Icons_Gap();
    if (Arinc661_Icons_Button(ARINC661_ICON_UNDO, UNDO_LABEL, UNDO_SHORTCUT_LABEL, can_undo, FALSE))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_Undo(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_Undo(window->configurationEditor);
        }
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_REDO, REDO_LABEL, REDO_SHORTCUT_LABEL, can_redo, FALSE))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_Redo(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_Redo(window->configurationEditor);
        }
    }
    Arinc661_Icons_Gap();
    if (window->isDefinitionActive)
    {
        if (Arinc661_Icons_Button(ARINC661_ICON_ADD_LAYER, ADD_LAYER_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_Definition_AddLayer(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_ADD_WIDGET, ADD_WIDGET_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_Definition_AddWidget(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_ADD_EXTENSION, ADD_EXTENSION_LABEL, NULL, has_selection, FALSE))
        {
            Arinc661_Definition_AddExtension(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_DUPLICATE, DUPLICATE_LABEL, DUPLICATE_SHORTCUT_LABEL, has_selection, FALSE))
        {
            Arinc661_Definition_DuplicateSelection(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_DELETE, DELETE_LABEL, DELETE_SHORTCUT_LABEL, has_active, FALSE))
        {
            Arinc661_Definition_DeleteSelection(window->definitionEditor);
        }
        Arinc661_Icons_Gap();
        if (Arinc661_Icons_Button(ARINC661_ICON_GRID, GRID_LABEL, GRID_SHORTCUT_LABEL, TRUE, Arinc661_Definition_Grid(window->definitionEditor, FALSE)))
        {
            (void)Arinc661_Definition_Grid(window->definitionEditor, TRUE);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_SNAP, SNAP_LABEL, SNAP_SHORTCUT_LABEL, TRUE, Arinc661_Definition_Snap(window->definitionEditor, FALSE)))
        {
            (void)Arinc661_Definition_Snap(window->definitionEditor, TRUE);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_FIT, FIT_LABEL, FIT_SHORTCUT_LABEL, has_active, FALSE))
        {
            Arinc661_Definition_FitView(window->definitionEditor);
        }
    }
    else
    {
        if (Arinc661_Icons_Button(ARINC661_ICON_SCREEN, ADD_SCREEN_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_Configuration_AddScreen(window->configurationEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_LAYOUT, ADD_LAYOUT_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_Configuration_AddLayout(window->configurationEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_WINDOW, ADD_WINDOW_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_Configuration_AddWindow(window->configurationEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_APPLICATION, ADD_APPLICATION_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_Configuration_AddUserApplication(window->configurationEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_DUPLICATE, DUPLICATE_LABEL, DUPLICATE_SHORTCUT_LABEL, has_selection, FALSE))
        {
            Arinc661_Configuration_DuplicateSelection(window->configurationEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_DELETE, DELETE_LABEL, DELETE_SHORTCUT_LABEL, has_selection, FALSE))
        {
            Arinc661_Configuration_DeleteSelection(window->configurationEditor);
        }
    }
    Arinc661_Icons_Gap();
    if (Arinc661_Icons_Button(ARINC661_ICON_HELP, HELP_CONTEXT_LABEL, HELP_SHORTCUT_LABEL, TRUE, FALSE))
    {
        OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND);
    }
    ImGui::NewLine();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The selected widget type, or the FACE element kind, names the help topic.
 */
static const char_t* SelectionTopic(const WindowThreadState_t* window)
{
    const char_t* topic;

    if (window->isDefinitionActive)
    {
        topic = Arinc661_Definition_SelectedType(window->definitionEditor);
    }
    else
    {
        topic = Arinc661_Configuration_SelectedElement(window->configurationEditor);
    }

    return topic;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Opens the Help window on a topic; a NULL topic keeps the current one.
 */
static void ShowHelp(WindowThreadState_t* window, const char_t* topic)
{
    window->isHelpOpen = TRUE;
    if (NULL != topic)
    {
        strncpy_s(window->helpTopic, sizeof(window->helpTopic), topic, _TRUNCATE);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the selection's page in Windows Help.
 */
static void OpenHelpInBrowser(WindowThreadState_t* window)
{
    OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Loads the HTML Help engine (hhctrl.ocx) once and shows the compiled help file,
 *  docs\arinc661.chm, found by walking up from the executable's folder. A topic command
 *  opens the page of the current selection; the contents, index, and search commands open
 *  those panes. Without the engine or the file, the HTML page opens in the browser instead.
 */
static void OpenWindowsHelp(WindowThreadState_t* window, uint32_t command)
{
    static HMODULE s_helpLibrary;
    static HWND (WINAPI* s_htmlHelp)(HWND, LPCSTR, UINT, DWORD_PTR);
    char_t        path[MAX_PATH + ARINC661_MAX_NAME_BYTES + 16];
    char_t        folder[MAX_PATH];
    const char_t* topic;
    DWORD         length;
    bool_t        has_file;

    if (NULL == s_helpLibrary)
    {
        s_helpLibrary = LoadLibraryA(HELP_LIBRARY_NAME);
        if (NULL != s_helpLibrary)
        {
            s_htmlHelp = (HWND (WINAPI*)(HWND, LPCSTR, UINT, DWORD_PTR))GetProcAddress(s_helpLibrary, HELP_FUNCTION_NAME);
        }
    }
    length = GetModuleFileNameA(NULL, folder, sizeof(folder));
    while ((0 < length) && ('\\' != folder[length - 1]))
    {
        length -= 1;
    }
    folder[length] = '\0';
    // The help file sits in the repository docs; walk up from the executable until it is found
    has_file = FALSE;
    for (;;)
    {
        (void)snprintf(path, sizeof(path), "%s%s", folder, HELP_CHM_PATH);
        if (INVALID_FILE_ATTRIBUTES != GetFileAttributesA(path))
        {
            has_file = TRUE;
            break;
        }
        if (0 == length)
        {
            break;
        }
        length -= 1;
        while ((0 < length) && ('\\' != folder[length - 1]))
        {
            length -= 1;
        }
        folder[length] = '\0';
    }
    topic = SelectionTopic(window);
    if (NULL == topic)
    {
        topic = window->helpTopic;
    }
    if (0 == strcmp(topic, HELP_GETTING_STARTED))
    {
        topic = HELP_DEFAULT_TOPIC;
    }
    if (has_file && (NULL != s_htmlHelp))
    {
        if (HH_DISPLAY_TOPIC_COMMAND == command)
        {
            (void)snprintf(path, sizeof(path), HELP_TOPIC_FORMAT, folder, HELP_CHM_PATH, topic);
            (void)s_htmlHelp(NULL, path, HH_DISPLAY_TOPIC_COMMAND, 0);
        }
        else
        {
            (void)snprintf(path, sizeof(path), "%s%s", folder, HELP_CHM_PATH);
            (void)s_htmlHelp(NULL, path, (UINT)command, 0);
        }
    }
    else
    {
        (void)snprintf(path, sizeof(path), "file:///%s%s#%s", folder, HELP_PAGE_PATH, topic);
        (void)ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A searchable reference: a getting-started page, the FACE H.3 elements, and every ARINC
 *  661 widget and extension with its description, categories, restriction, and parameter
 *  table from the specification. The topic list follows the selection when F1 is pressed.
 */
static void DrawHelpWindow(WindowThreadState_t* window)
{
    const char_t* description;
    const char_t* categories;
    const char_t* restriction;
    const char_t* type_name;
    const char_t* change;
    const char_t* parameter_description;
    Arinc661_Parameter_t parameter;
    char_t        upper_filter[MAX_HELP_FILTER_BYTES];
    char_t        upper_name[ARINC661_MAX_NAME_BYTES];
    uint32_t      index;
    uint32_t      character;
    uint32_t      parameter_count;
    uint32_t      pass;
    bool_t        is_extension;
    bool_t        is_match;
    bool          is_open;

    if (window->isHelpOpen)
    {
        ImGui::SetNextWindowSize(HELP_WINDOW_SIZE, ImGuiCond_FirstUseEver);
        is_open = (FALSE != window->isHelpOpen);
        if (ImGui::Begin(HELP_WINDOW_LABEL, &is_open))
        {
            ImGui::SetNextItemWidth(HELP_LIST_WIDTH);
            (void)ImGui::InputTextWithHint("##search", HELP_SEARCH_HINT, window->helpFilter, sizeof(window->helpFilter));
            ImGui::SameLine();
            if (ImGui::SmallButton(HELP_BROWSER_LABEL))
            {
                OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND);
            }
            for (character = 0; character < sizeof(upper_filter); ++character)
            {
                upper_filter[character] = (char_t)ImToUpper(window->helpFilter[character]);
                if ('\0' == window->helpFilter[character])
                {
                    break;
                }
            }
            if (ImGui::BeginChild("##topics", ImVec2(HELP_LIST_WIDTH, 0.0f), ImGuiChildFlags_Borders))
            {
                if (ImGui::Selectable(HELP_GETTING_STARTED, 0 == strcmp(window->helpTopic, HELP_GETTING_STARTED)))
                {
                    strncpy_s(window->helpTopic, sizeof(window->helpTopic), HELP_GETTING_STARTED, _TRUNCATE);
                }
                for (index = 0; index < (sizeof(c_faceTopics) / sizeof(c_faceTopics[0])); ++index)
                {
                    for (character = 0; character < sizeof(upper_name); ++character)
                    {
                        upper_name[character] = (char_t)ImToUpper(c_faceTopics[index].name[character]);
                        if ('\0' == c_faceTopics[index].name[character])
                        {
                            break;
                        }
                    }
                    if (('\0' == upper_filter[0]) || (NULL != strstr(upper_name, upper_filter)))
                    {
                        ImGui::PushID((int)index);
                        if (ImGui::Selectable(c_faceTopics[index].name, 0 == strcmp(window->helpTopic, c_faceTopics[index].name)))
                        {
                            strncpy_s(window->helpTopic, sizeof(window->helpTopic), c_faceTopics[index].name, _TRUNCATE);
                        }
                        ImGui::PopID();
                    }
                }
                ImGui::Separator();
                for (pass = 0; pass < 2; ++pass)
                {
                    is_extension = (1 == pass);
                    for (index = 0; index < Arinc661_TypeCount(is_extension); ++index)
                    {
                        type_name = Arinc661_TypeName(is_extension, index);
                        for (character = 0; character < sizeof(upper_name); ++character)
                        {
                            upper_name[character] = (char_t)ImToUpper(type_name[character]);
                            if ('\0' == type_name[character])
                            {
                                break;
                            }
                        }
                        is_match = !!(('\0' == upper_filter[0]) || (NULL != strstr(upper_name, upper_filter)));
                        if (is_match)
                        {
                            if (ImGui::Selectable(type_name, 0 == strcmp(window->helpTopic, type_name)))
                            {
                                strncpy_s(window->helpTopic, sizeof(window->helpTopic), type_name, _TRUNCATE);
                            }
                            if (0 == strcmp(window->helpTopic, type_name) && ImGui::IsWindowAppearing())
                            {
                                ImGui::SetScrollHereY();
                            }
                        }
                    }
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if (ImGui::BeginChild("##topic", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
            {
                ImGui::PushTextWrapPos(0.0f);
                if (('\0' == window->helpTopic[0]) || (0 == strcmp(window->helpTopic, HELP_GETTING_STARTED)))
                {
                    ImGui::SeparatorText(HELP_GETTING_STARTED);
                    ImGui::TextUnformatted("1. Configuration: File > New Configuration. Add a Screen (toolbar), then a Layout, then a Window; set the window's pixelArea and scalingFactor. Add a User Application, pick its Window and its DF Path from the Definition Files in the folder.");
                    ImGui::TextUnformatted("2. Definition File: File > New Definition File. Add a Layer, then right-click the scene to add widgets where you click, or use the toolbar. Containers (Panel, BasicContainer, ...) accept children: right-click a container or drag a node onto it in the hierarchy.");
                    ImGui::TextUnformatted("3. Scene: drag widgets to move them, drag the corner handles to resize, arrow keys nudge, Delete removes, mouse wheel zooms, middle-drag or Space-drag pans, F fits, G toggles the grid, S toggles millimetre snapping.");
                    ImGui::TextUnformatted("4. Inspector: every parameter row shows its specification description on hover; the About section explains the selected widget. Red values would not fit the binary form.");
                    ImGui::TextUnformatted("5. Link both: set the Definition File's ApplicationId (root node) to the user application's applicationId. The user application inspector checks it and previews the Definition File's layers inside the window; the Definition File root lists who references it.");
                    ImGui::TextUnformatted("6. Save with Ctrl+S; a .bin name saves the binary form of ARINC 661 Part 1-7, any other name the XML form. Undo with Ctrl+Z, redo with Ctrl+Y. F1 opens this help on the selection; Shift+F1 opens the HTML reference in the browser.");
                }
                else
                {
                    is_match = FALSE;
                    for (index = 0; index < (sizeof(c_faceTopics) / sizeof(c_faceTopics[0])); ++index)
                    {
                        if (0 == strcmp(window->helpTopic, c_faceTopics[index].name))
                        {
                            ImGui::SeparatorText(c_faceTopics[index].name);
                            ImGui::TextUnformatted(c_faceTopics[index].description);
                            is_match = TRUE;
                        }
                    }
                    if ((FALSE == is_match) && Arinc661_TypeHelp(window->helpTopic, &description, &categories, &restriction))
                    {
                        ImGui::SeparatorText(window->helpTopic);
                        ImGui::TextUnformatted(description);
                        if ('\0' != categories[0])
                        {
                            ImGui::TextDisabled("Categories: %s", categories);
                        }
                        if ('\0' != restriction[0])
                        {
                            ImGui::TextDisabled("Restriction: %s", restriction);
                        }
                        is_extension = !!(Arinc661_FindType(window->helpTopic, TRUE, &parameter_count));
                        if (Arinc661_FindType(window->helpTopic, is_extension, &parameter_count))
                        {
                            ImGui::SeparatorText(HELP_PARAMETERS_LABEL);
                            if (ImGui::BeginTable("##parameters", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
                            {
                                ImGui::TableSetupColumn(HELP_NAME_COLUMN, ImGuiTableColumnFlags_WidthFixed, 180.0f);
                                ImGui::TableSetupColumn(HELP_CHANGE_COLUMN, ImGuiTableColumnFlags_WidthFixed, 50.0f);
                                ImGui::TableSetupColumn(HELP_DESCRIPTION_COLUMN);
                                ImGui::TableHeadersRow();
                                for (index = 0; index < parameter_count; ++index)
                                {
                                    (void)Arinc661_Parameter(window->helpTopic, is_extension, index, &parameter);
                                    parameter_description = Arinc661_ParameterHelp(window->helpTopic, parameter.name, &change);
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::TextUnformatted(parameter.name);
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::TextUnformatted(change);
                                    ImGui::TableSetColumnIndex(2);
                                    ImGui::TextWrapped("%s", parameter_description);
                                }
                                ImGui::EndTable();
                            }
                        }
                        is_match = TRUE;
                    }
                    if (FALSE == is_match)
                    {
                        ImGui::TextDisabled("%s", window->helpTopic);
                    }
                }
                ImGui::PopTextWrapPos();
            }
            ImGui::EndChild();
        }
        ImGui::End();
        window->isHelpOpen = !!is_open;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the File menu, whose Save and Close act on whichever section was used last, and the
 *  matching keyboard shortcuts.
 */
static void DrawPageMenu(WindowThreadState_t* window)
{
    bool_t has_active;
    bool_t is_dirty;
    bool_t has_free_configuration;
    bool_t has_free_definition;
    bool_t has_dirty_configuration;
    bool_t has_dirty_definition;
    bool_t has_active_configuration;
    bool_t has_active_definition;
    bool_t can_undo;
    bool_t can_redo;
    bool_t has_selection;
    uint32_t panel_index;

    has_active_configuration = Arinc661_Configuration_HasActiveDocument(window->configurationEditor, &has_dirty_configuration, &has_free_configuration);
    has_active_definition    = Arinc661_Definition_HasActiveDocument(window->definitionEditor, &has_dirty_definition, &has_free_definition);
    has_active = has_active_configuration;
    is_dirty   = !!(has_dirty_configuration || has_dirty_definition);
    if (window->isDefinitionActive)
    {
        has_active = has_active_definition;
    }
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(FILE_MENU_LABEL))
        {
            if (ImGui::MenuItem(NEW_CONFIGURATION_LABEL, NEW_CONFIGURATION_SHORTCUT, false, has_free_configuration))
            {
                Arinc661_Configuration_CreateDocument(window->configurationEditor);
                window->isDefinitionActive = FALSE;
            }
            if (ImGui::MenuItem(NEW_DEFINITION_LABEL, NEW_DEFINITION_SHORTCUT, false, has_free_definition))
            {
                Arinc661_Definition_CreateDocument(window->definitionEditor);
                window->isDefinitionActive = TRUE;
            }
            if (ImGui::MenuItem(NEW_APPLICATION_LABEL, NULL, false, has_active_configuration && has_free_definition))
            {
                (void)Arinc661_Context_NewApplication(window->context);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(SAVE_LABEL, SAVE_SHORTCUT_LABEL, false, has_active))
            {
                SaveActive(window);
            }
            if (ImGui::MenuItem(SAVE_ALL_LABEL, SAVE_ALL_SHORTCUT_LABEL, false, is_dirty))
            {
                Arinc661_Configuration_SaveAllDocuments(window->configurationEditor);
                Arinc661_Definition_SaveAllDocuments(window->definitionEditor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(CLOSE_LABEL, CLOSE_SHORTCUT_LABEL, false, has_active))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_Definition_CloseActiveDocument(window->definitionEditor);
                }
                else
                {
                    Arinc661_Configuration_CloseActiveDocument(window->configurationEditor);
                }
            }
            ImGui::EndMenu();
        }
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_History(window->definitionEditor, &can_undo, &can_redo);
            has_selection = (NULL != Arinc661_Definition_SelectedType(window->definitionEditor));
        }
        else
        {
            Arinc661_Configuration_History(window->configurationEditor, &can_undo, &can_redo);
            has_selection = (NULL != Arinc661_Configuration_SelectedElement(window->configurationEditor));
        }
        if (ImGui::BeginMenu(EDIT_MENU_LABEL))
        {
            if (ImGui::MenuItem(UNDO_LABEL, UNDO_SHORTCUT_LABEL, false, can_undo))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_Definition_Undo(window->definitionEditor);
                }
                else
                {
                    Arinc661_Configuration_Undo(window->configurationEditor);
                }
            }
            if (ImGui::MenuItem(REDO_LABEL, REDO_SHORTCUT_LABEL, false, can_redo))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_Definition_Redo(window->definitionEditor);
                }
                else
                {
                    Arinc661_Configuration_Redo(window->configurationEditor);
                }
            }
            ImGui::Separator();
            if (window->isDefinitionActive)
            {
                if (ImGui::MenuItem(ADD_LAYER_LABEL, NULL, false, has_active))
                {
                    Arinc661_Definition_AddLayer(window->definitionEditor);
                }
                if (ImGui::MenuItem(ADD_WIDGET_LABEL, NULL, false, has_active))
                {
                    Arinc661_Definition_AddWidget(window->definitionEditor);
                }
                if (ImGui::MenuItem(ADD_EXTENSION_LABEL, NULL, false, has_selection))
                {
                    Arinc661_Definition_AddExtension(window->definitionEditor);
                }
            }
            else
            {
                if (ImGui::MenuItem(ADD_SCREEN_LABEL, NULL, false, has_active))
                {
                    Arinc661_Configuration_AddScreen(window->configurationEditor);
                }
                if (ImGui::MenuItem(ADD_LAYOUT_LABEL, NULL, false, has_active))
                {
                    Arinc661_Configuration_AddLayout(window->configurationEditor);
                }
                if (ImGui::MenuItem(ADD_WINDOW_LABEL, NULL, false, has_active))
                {
                    Arinc661_Configuration_AddWindow(window->configurationEditor);
                }
                if (ImGui::MenuItem(ADD_APPLICATION_LABEL, NULL, false, has_active))
                {
                    Arinc661_Configuration_AddUserApplication(window->configurationEditor);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem(DUPLICATE_LABEL, DUPLICATE_SHORTCUT_LABEL, false, has_selection))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_Definition_DuplicateSelection(window->definitionEditor);
                }
                else
                {
                    Arinc661_Configuration_DuplicateSelection(window->configurationEditor);
                }
            }
            if (ImGui::MenuItem(DELETE_LABEL, DELETE_SHORTCUT_LABEL, false, has_active))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_Definition_DeleteSelection(window->definitionEditor);
                }
                else
                {
                    Arinc661_Configuration_DeleteSelection(window->configurationEditor);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(VIEW_MENU_LABEL))
        {
            if (ImGui::MenuItem(GRID_LABEL, GRID_SHORTCUT_LABEL, Arinc661_Definition_Grid(window->definitionEditor, FALSE)))
            {
                (void)Arinc661_Definition_Grid(window->definitionEditor, TRUE);
            }
            if (ImGui::MenuItem(SNAP_LABEL, SNAP_SHORTCUT_LABEL, Arinc661_Definition_Snap(window->definitionEditor, FALSE)))
            {
                (void)Arinc661_Definition_Snap(window->definitionEditor, TRUE);
            }
            if (ImGui::MenuItem(FIT_LABEL, FIT_SHORTCUT_LABEL))
            {
                Arinc661_Definition_FitView(window->definitionEditor);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(WINDOW_MENU_LABEL))
        {
            for (panel_index = 0; panel_index < PANEL_COUNT; ++panel_index)
            {
                ImGui::MenuItem(c_panelNames[panel_index], NULL, &window->panelOpen[panel_index]);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(RESET_LAYOUT_LABEL))
            {
                window->resetsLayout = TRUE;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(HELP_MENU_LABEL))
        {
            if (ImGui::MenuItem(HELP_CONTEXT_LABEL, HELP_SHORTCUT_LABEL))
            {
                OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND);
            }
            if (ImGui::MenuItem(HELP_CONTENTS_LABEL))
            {
                OpenWindowsHelp(window, HH_DISPLAY_TOC_COMMAND);
            }
            if (ImGui::MenuItem(HELP_INDEX_LABEL))
            {
                OpenWindowsHelp(window, HH_DISPLAY_INDEX_COMMAND);
            }
            if (ImGui::MenuItem(HELP_SEARCH_LABEL))
            {
                OpenWindowsHelp(window, HH_DISPLAY_SEARCH_COMMAND);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(HELP_REFERENCE_LABEL, HELP_BROWSER_SHORTCUT_LABEL))
            {
                ShowHelp(window, SelectionTopic(window));
            }
            if (ImGui::MenuItem(HELP_GETTING_STARTED))
            {
                ShowHelp(window, HELP_GETTING_STARTED);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::Shortcut(UNDO_CHORD, SHORTCUT_FLAGS))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_Undo(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_Undo(window->configurationEditor);
        }
    }
    if (ImGui::Shortcut(REDO_CHORD, SHORTCUT_FLAGS))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_Redo(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_Redo(window->configurationEditor);
        }
    }
    if (ImGui::Shortcut(DUPLICATE_CHORD, SHORTCUT_FLAGS))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_DuplicateSelection(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_DuplicateSelection(window->configurationEditor);
        }
    }
    if (ImGui::Shortcut(HELP_BROWSER_CHORD, SHORTCUT_FLAGS))
    {
        ShowHelp(window, SelectionTopic(window));
    }
    else if (ImGui::Shortcut(HELP_CHORD, SHORTCUT_FLAGS))
    {
        OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND);
    }
    if (ImGui::Shortcut(NEW_CONFIGURATION_CHORD, SHORTCUT_FLAGS) && has_free_configuration)
    {
        Arinc661_Configuration_CreateDocument(window->configurationEditor);
        window->isDefinitionActive = FALSE;
    }
    if (ImGui::Shortcut(NEW_DEFINITION_CHORD, SHORTCUT_FLAGS) && has_free_definition)
    {
        Arinc661_Definition_CreateDocument(window->definitionEditor);
        window->isDefinitionActive = TRUE;
    }
    if (ImGui::Shortcut(SAVE_CHORD, SHORTCUT_FLAGS) && has_active)
    {
        SaveActive(window);
    }
    if (ImGui::Shortcut(SAVE_ALL_CHORD, SHORTCUT_FLAGS) && is_dirty)
    {
        Arinc661_Configuration_SaveAllDocuments(window->configurationEditor);
        Arinc661_Definition_SaveAllDocuments(window->definitionEditor);
    }
    if (ImGui::Shortcut(CLOSE_CHORD, SHORTCUT_FLAGS) && has_active)
    {
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_CloseActiveDocument(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_CloseActiveDocument(window->configurationEditor);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Saving never blocks, but a document with errors in the Problems list gets a note in the
 *  status line so the author knows what was written.
 */
static void SaveActive(WindowThreadState_t* window)
{
    char_t   note[SAVED_WITH_ERRORS_BYTES];
    uint32_t errors;

    if (window->isDefinitionActive)
    {
        errors = Arinc661_Context_DocumentErrors(window->context, TRUE, window->definitionEditor->activeDocumentIndex);
        Arinc661_Definition_SaveActiveDocument(window->definitionEditor);
        if (0 < errors)
        {
            (void)snprintf(note, sizeof(note), SAVED_WITH_ERRORS_FORMAT, (unsigned)errors);
            Arinc661_Definition_SetStatusNote(window->definitionEditor, note);
        }
        else
        {
            Arinc661_Definition_SetStatusNote(window->definitionEditor, NULL);
        }
    }
    else
    {
        errors = Arinc661_Context_DocumentErrors(window->context, FALSE, window->configurationEditor->activeDocumentIndex);
        Arinc661_Configuration_SaveActiveDocument(window->configurationEditor);
        window->saveNote[0] = '\0';
        if (0 < errors)
        {
            (void)snprintf(window->saveNote, sizeof(window->saveNote), SAVED_WITH_ERRORS_FORMAT, (unsigned)errors);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the context check every frame and lists the findings under a collapsing header whose
 *  title carries the counts. Errors are red, warnings amber; a click selects the offending
 *  item in its editor and moves the focus there.
 */
static void DrawPageProblems(WindowThreadState_t* window)
{
    const Arinc661_Context_Problem_t* problem;
    char_t                            title[128];
    uint32_t                          problem_index;
    ImVec4                            color;
    bool                              is_open;

    (void)Arinc661_Context_Check(window->context);
    (void)snprintf(title, sizeof(title), PROBLEMS_SECTION_FORMAT, (unsigned)window->context->errorCount, (unsigned)(window->context->problemCount - window->context->errorCount));
    if (0 < window->context->errorCount)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, PROBLEM_ERROR_COLOR);
    }
    is_open = ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
    if (0 < window->context->errorCount)
    {
        ImGui::PopStyleColor();
    }
    if (is_open)
    {
        if (ImGui::SmallButton(OPEN_REFERENCED_LABEL))
        {
            (void)Arinc661_Context_OpenReferencedDefinitions(window->context);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", RECHECK_HINT_LABEL);
        if (ImGui::BeginChild("##problems", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * ImMin(PROBLEMS_LIST_ROWS, (float32_t)window->context->problemCount + 1.5f)), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY))
        {
            if (0 == window->context->problemCount)
            {
                ImGui::TextDisabled("%s", NO_PROBLEMS_LABEL);
            }
            for (problem_index = 0; problem_index < window->context->problemCount; ++problem_index)
            {
                problem = &window->context->problems[problem_index];
                color   = PROBLEM_WARNING_COLOR;
                if (problem->isError)
                {
                    color = PROBLEM_ERROR_COLOR;
                }
                ImGui::PushID((int)problem_index);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                if (problem->isError)
                {
                    ImGui::TextUnformatted(PROBLEM_ERROR_PREFIX);
                }
                else
                {
                    ImGui::TextUnformatted(PROBLEM_WARNING_PREFIX);
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
                if (ImGui::Selectable(problem->text))
                {
                    Arinc661_Context_ShowProblem(window->context, problem);
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the shared folder path and one list of every configuration and Definition File in
 *  it, each tagged with its kind. Selecting a file opens it in the matching section and
 *  gives that section the focus. The folder is rescanned on edit, on Refresh, and every few
 *  seconds so saved files appear.
 */
static void DrawPageFiles(WindowThreadState_t* window)
{
    char_t    label[MAX_FILE_NAME_BYTES + 32];
    ImVec2    list_size;
    uint32_t  file_index;
    bool_t    should_scan;

    should_scan = FALSE;
    if (ImGui::CollapsingHeader(FILE_CONTROLS_HEADER, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(FOLDER_PATH_LABEL);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-(ImGui::CalcTextSize(REFRESH_LABEL).x + (ImGui::GetStyle().FramePadding.x * 2.0f) + ImGui::GetStyle().ItemSpacing.x));
        if (ImGui::InputText(FOLDER_INPUT_ID, window->folderPath, sizeof(window->folderPath)))
        {
            should_scan = TRUE;
        }
        ImGui::SameLine();
        if (ImGui::Button(REFRESH_LABEL))
        {
            should_scan = TRUE;
        }
        if (should_scan || ((ImGui::GetTime() - window->lastScanTime) > FILE_SCAN_INTERVAL_SECONDS))
        {
            ScanFolder(window);
        }
        list_size = ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * FILE_LIST_ROWS);
        if (ImGui::BeginListBox(FILE_LIST_ID, list_size))
        {
            if (FALSE == window->isFolderReadable)
            {
                ImGui::TextDisabled("%s", FOLDER_UNREADABLE_LABEL);
            }
            else if (0 == window->fileCount)
            {
                ImGui::TextDisabled("%s", NO_FILES_LABEL);
            }
            for (file_index = 0; file_index < window->fileCount; ++file_index)
            {
                if (window->fileIsDefinition[file_index])
                {
                    (void)snprintf(label, sizeof(label), DEFINITION_FILE_FORMAT, window->fileNames[file_index]);
                }
                else
                {
                    (void)snprintf(label, sizeof(label), CONFIGURATION_FILE_FORMAT, window->fileNames[file_index]);
                }
                ImGui::PushID((int)file_index);
                if (ImGui::Selectable(label))
                {
                    if (window->fileIsDefinition[file_index])
                    {
                        Arinc661_Definition_LoadFile(window->definitionEditor, window->fileNames[file_index]);
                        window->isDefinitionActive = TRUE;
                    }
                    else
                    {
                        Arinc661_Configuration_LoadFile(window->configurationEditor, window->fileNames[file_index]);
                        (void)Arinc661_Context_OpenReferencedDefinitions(window->context);
                        window->isDefinitionActive = FALSE;
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndListBox();
        }
    }
    ImGui::Separator();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  One hierarchy pane holds both document kinds as two sections, each with its document tabs
 *  and tree. A click anywhere inside a section makes it the focused section, whose scene and
 *  inspector fill the other two panes.
 */
static void DrawPageHierarchy(WindowThreadState_t* window)
{
    if (ImGui::BeginChild(HIERARCHY_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::TextUnformatted(HIERARCHY_PANE_NAME);
        ImGui::Separator();
        if (ImGui::CollapsingHeader(CONFIGURATION_SECTION_LABEL, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(CONFIGURATION_SECTION_ID);
            ImGui::BeginGroup();
            Arinc661_Configuration_DrawDocumentTabs(window->configurationEditor);
            Arinc661_Configuration_DrawHierarchy(window->configurationEditor);
            ImGui::EndGroup();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped))
            {
                window->isDefinitionActive = FALSE;
            }
            ImGui::PopID();
        }
        if (ImGui::CollapsingHeader(DEFINITION_SECTION_LABEL, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(DEFINITION_SECTION_ID);
            ImGui::BeginGroup();
            Arinc661_Definition_DrawDocumentTabs(window->definitionEditor);
            Arinc661_Definition_DrawHierarchy(window->definitionEditor);
            ImGui::EndGroup();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped))
            {
                window->isDefinitionActive = TRUE;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the single page: the File menu, the shared file list, then the hierarchy, the scene
 *  of the focused section, and its inspector side by side, with dialogs and a status line.
 */
static void DrawEditors(WindowThreadState_t* window)
{
    const ImGuiViewport* viewport;
    ImGuiID              dockspace_id;
    uint32_t             panel_index;
    bool_t               is_definition_focused;

    // Honour a cross-editor navigation request before drawing so the focused section follows it
    if (Arinc661_Context_TakeFocusRequest(window->context, &is_definition_focused))
    {
        window->isDefinitionActive = is_definition_focused;
    }
    DrawPageMenu(window);
    viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin(PAGE_WINDOW_NAME, NULL, PAGE_WINDOW_FLAGS))
    {
        ImGui::PopStyleVar(2);
        DrawToolbar(window);
        dockspace_id = ImGui::GetID(DOCKSPACE_ID);
        if ((FALSE == window->hasLayout) || window->resetsLayout || (NULL == ImGui::DockBuilderGetNode(dockspace_id)))
        {
            BuildDefaultLayout(window, dockspace_id);
        }
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, -(ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y)), ImGuiDockNodeFlags_PassthruCentralNode);
        DrawStatusBar(window);
    }
    else
    {
        ImGui::PopStyleVar(2);
    }
    ImGui::End();
    // Panels dock into the space above; each can be closed from the Window menu
    for (panel_index = 0; panel_index < PANEL_COUNT; ++panel_index)
    {
        if (FALSE == window->panelOpen[panel_index])
        {
            continue;
        }
        if (ImGui::Begin(c_panelNames[panel_index], &window->panelOpen[panel_index]))
        {
            switch (panel_index)
            {
                case PANEL_INDEX_FILES:     DrawPageFiles(window); break;
                case PANEL_INDEX_PROBLEMS:  DrawPageProblems(window); break;
                case PANEL_INDEX_HIERARCHY: DrawPageHierarchy(window); break;
                case PANEL_INDEX_PALETTE:   DrawPalette(window); break;
                case PANEL_INDEX_SCENE:
                    if (window->isDefinitionActive)
                    {
                        Arinc661_Definition_DrawScene(window->definitionEditor);
                    }
                    else
                    {
                        Arinc661_Configuration_DrawScene(window->configurationEditor);
                    }
                    break;
                case PANEL_INDEX_INSPECTOR:
                    if (window->isDefinitionActive)
                    {
                        Arinc661_Definition_DrawInspector(window->definitionEditor);
                    }
                    else
                    {
                        Arinc661_Configuration_DrawInspector(window->configurationEditor);
                    }
                    break;
                case PANEL_INDEX_LOG:       DrawLog(window); break;
                default:                    break;
            }
        }
        ImGui::End();
    }
    DrawHelpWindow(window);
    (void)Arinc661_Context_FollowRenames(window->context);
    Arinc661_Configuration_EndFrame(window->configurationEditor);
    Arinc661_Definition_EndFrame(window->definitionEditor);
    Arinc661_Configuration_DrawDialogs(window->configurationEditor);
    Arinc661_Definition_DrawDialogs(window->definitionEditor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Left column: Hierarchy over Palette. Right column: Inspector. Bottom of the centre: Files,
 *  Problems, and Log as tabs. Centre: Scene. Every panel is reopened.
 */
static void BuildDefaultLayout(WindowThreadState_t* window, ImGuiID dockspaceId)
{
    ImGuiID  dock_main;
    ImGuiID  dock_left;
    ImGuiID  dock_left_bottom;
    ImGuiID  dock_right;
    ImGuiID  dock_bottom;
    uint32_t panel_index;

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);
    dock_main        = dockspaceId;
    dock_left        = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, LAYOUT_LEFT_FRACTION, NULL, &dock_main);
    dock_right       = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, LAYOUT_RIGHT_FRACTION, NULL, &dock_main);
    dock_bottom      = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, LAYOUT_BOTTOM_FRACTION, NULL, &dock_main);
    dock_left_bottom = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, LAYOUT_PALETTE_FRACTION, NULL, &dock_left);
    ImGui::DockBuilderDockWindow(PANEL_HIERARCHY, dock_left);
    ImGui::DockBuilderDockWindow(PANEL_PALETTE, dock_left_bottom);
    ImGui::DockBuilderDockWindow(PANEL_INSPECTOR, dock_right);
    ImGui::DockBuilderDockWindow(PANEL_FILES, dock_bottom);
    ImGui::DockBuilderDockWindow(PANEL_PROBLEMS, dock_bottom);
    ImGui::DockBuilderDockWindow(PANEL_LOG, dock_bottom);
    ImGui::DockBuilderDockWindow(PANEL_SCENE, dock_main);
    ImGui::DockBuilderFinish(dockspaceId);
    for (panel_index = 0; panel_index < PANEL_COUNT; ++panel_index)
    {
        window->panelOpen[panel_index] = true;
    }
    window->hasLayout    = TRUE;
    window->resetsLayout = FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Types are grouped by the first category the specification lists for them. A click adds a
 *  widget of that type to the Definition File selection; a drag carries the type name as a
 *  payload the hierarchy and the scene accept. Extensions get their own group.
 */
static void DrawPalette(WindowThreadState_t* window)
{
    static const char_t* const c_categories[] = {"Container", "Interactive", "Graphical representation", "Text string", "Map management", "Dynamic motion", "Utility", "Animation", PALETTE_OTHER_CATEGORY, PALETTE_EXTENSIONS_CATEGORY};
    const char_t* type_name;
    const char_t* description;
    const char_t* categories;
    const char_t* restriction;
    const char_t* category;
    char_t        upper_filter[MAX_HELP_FILTER_BYTES];
    char_t        upper_name[ARINC661_MAX_NAME_BYTES];
    uint32_t      category_index;
    uint32_t      type_index;
    uint32_t      character;
    bool_t        is_extension;
    bool_t        is_in_category;
    bool_t        has_header;

    ImGui::SetNextItemWidth(-1.0f);
    (void)ImGui::InputTextWithHint("##palette filter", PALETTE_FILTER_HINT, window->paletteFilter, sizeof(window->paletteFilter));
    ImGui::TextDisabled("%s", PALETTE_HINT_LABEL);
    for (character = 0; character < sizeof(upper_filter); ++character)
    {
        upper_filter[character] = (char_t)ImToUpper(window->paletteFilter[character]);
        if ('\0' == window->paletteFilter[character])
        {
            break;
        }
    }
    for (category_index = 0; category_index < (sizeof(c_categories) / sizeof(c_categories[0])); ++category_index)
    {
        category     = c_categories[category_index];
        is_extension = (0 == strcmp(category, PALETTE_EXTENSIONS_CATEGORY));
        has_header   = FALSE;
        for (type_index = 0; type_index < Arinc661_TypeCount(is_extension); ++type_index)
        {
            type_name = Arinc661_TypeName(is_extension, type_index);
            (void)Arinc661_TypeHelp(type_name, &description, &categories, &restriction);
            if (is_extension)
            {
                is_in_category = TRUE;
            }
            else if (0 == strcmp(category, PALETTE_OTHER_CATEGORY))
            {
                // Types whose first category matches none of the named groups
                is_in_category = TRUE;
                for (character = 0; character < (sizeof(c_categories) / sizeof(c_categories[0])) - 2; ++character)
                {
                    if (0 == ImStrnicmp(categories, c_categories[character], (int)strlen(c_categories[character])))
                    {
                        is_in_category = FALSE;
                    }
                }
            }
            else
            {
                is_in_category = (0 == ImStrnicmp(categories, category, (int)strlen(category)));
            }
            if (FALSE == is_in_category)
            {
                continue;
            }
            for (character = 0; character < sizeof(upper_name); ++character)
            {
                upper_name[character] = (char_t)ImToUpper(type_name[character]);
                if ('\0' == type_name[character])
                {
                    break;
                }
            }
            if (('\0' != upper_filter[0]) && (NULL == strstr(upper_name, upper_filter)))
            {
                continue;
            }
            if (FALSE == has_header)
            {
                ImGui::SeparatorText(category);
                has_header = TRUE;
            }
            ImGui::PushID(type_name);
            if (ImGui::Selectable(type_name + 5))
            {
                if (is_extension)
                {
                    Arinc661_Definition_AddExtensionOfType(window->definitionEditor, type_name);
                }
                else
                {
                    Arinc661_Definition_AddWidgetOfType(window->definitionEditor, type_name);
                }
                window->isDefinitionActive = TRUE;
            }
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(PALETTE_PAYLOAD_ID, type_name, strlen(type_name) + 1);
                ImGui::TextUnformatted(type_name);
                ImGui::EndDragDropSource();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ('\0' != description[0]))
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(HELP_WRAP_WIDTH);
                ImGui::TextUnformatted(type_name);
                ImGui::TextUnformatted(description);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
            ImGui::PopID();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Only a changed text is appended, stamped with the wall clock.
 */
static void AppendLog(WindowThreadState_t* window, const char_t* text)
{
    SYSTEMTIME now;
    char_t*    line;

    if ((NULL != text) && ('\0' != text[0]) && (0 != strcmp(text, window->lastStatus)))
    {
        strncpy_s(window->lastStatus, sizeof(window->lastStatus), text, _TRUNCATE);
        GetLocalTime(&now);
        line = window->logLines[window->logCount % LOG_CAPACITY];
        (void)snprintf(line, LOG_LINE_BYTES, LOG_TIME_FORMAT "%s", (int)now.wHour, (int)now.wMinute, (int)now.wSecond, text);
        window->logCount += 1;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Oldest first; the ring keeps the last LOG_CAPACITY lines.
 */
static void DrawLog(WindowThreadState_t* window)
{
    uint32_t first;
    uint32_t index;

    if (ImGui::SmallButton(LOG_CLEAR_LABEL))
    {
        window->logCount      = 0;
        window->lastStatus[0] = '\0';
    }
    ImGui::SameLine();
    ImGui::Checkbox(LOG_AUTOSCROLL_LABEL, &window->logAutoScroll);
    if (ImGui::BeginChild("##log lines", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
    {
        first = 0;
        if (LOG_CAPACITY < window->logCount)
        {
            first = (window->logCount - LOG_CAPACITY);
        }
        for (index = first; index < window->logCount; ++index)
        {
            ImGui::TextUnformatted(window->logLines[index % LOG_CAPACITY]);
        }
        if (window->logAutoScroll && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight()))
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Status text of the focused section, any save note, then the context breadcrumbs.
 */
static void DrawStatusBar(WindowThreadState_t* window)
{
    char_t        configuration_crumb[CONTEXT_CRUMB_BYTES];
    char_t        definition_crumb[CONTEXT_CRUMB_BYTES];
    const char_t* status_text;

    status_text = Arinc661_Configuration_StatusText(window->configurationEditor);
    if (window->isDefinitionActive)
    {
        status_text = Arinc661_Definition_StatusText(window->definitionEditor);
    }
    AppendLog(window, status_text);
    if ('\0' == status_text[0])
    {
        status_text = READY_LABEL;
    }
    ImGui::Separator();
    ImGui::TextUnformatted(status_text);
    if (window->isDefinitionActive && ('\0' != window->definitionEditor->statusNote[0]))
    {
        ImGui::SameLine();
        ImGui::TextColored(PROBLEM_ERROR_COLOR, "%s", window->definitionEditor->statusNote);
    }
    else if ((FALSE == window->isDefinitionActive) && ('\0' != window->saveNote[0]))
    {
        ImGui::SameLine();
        ImGui::TextColored(PROBLEM_ERROR_COLOR, "%s", window->saveNote);
    }
    Arinc661_Configuration_SelectionText(window->configurationEditor, configuration_crumb, sizeof(configuration_crumb));
    Arinc661_Definition_SelectionText(window->definitionEditor, definition_crumb, sizeof(definition_crumb));
    ImGui::SameLine(0.0f, CONTEXT_BAR_GAP);
    ImGui::TextDisabled(CONTEXT_BAR_FORMAT, configuration_crumb, definition_crumb);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws one ImGui frame while the presenter is excluded from the back buffer.
 */
static void DrawFrame(WindowThreadState_t* window)
{
    AcquireSRWLockExclusive(&window->presentLock);
    BeginFrame();
    DrawEditors(window);
    EndFrame(window);
    ReleaseSRWLockExclusive(&window->presentLock);
    LOGASSERT(S_OK == ::DwmFlush());
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates a Win32 OpenGL context for the main or platform window.
 */
static bool_t CreateDeviceWGL(HWND windowHandle, WglWindowData_t* windowData)
{
    HDC                               device_context;
    PIXELFORMATDESCRIPTOR             pixel_format_description;
    int                               pixel_format;
    HGLRC                             temporary_context;
    HGLRC                             new_context;
    GLint                             major;
    GLint                             minor;
    const char_t*                     version_string;
    GLuint                            version;
    WglCreateContextAttribsFunction_t create_context_attribs;
    int                               context_attributes[7];

    windowData->deviceContext = NULL;
    device_context            = ::GetDC(windowHandle);
    if (NULL == device_context)
    {
        return FALSE;
    }
    memset(&pixel_format_description, 0, sizeof(pixel_format_description));
    pixel_format_description.nSize      = sizeof(pixel_format_description);
    pixel_format_description.nVersion   = 1;
    pixel_format_description.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format_description.iPixelType = PFD_TYPE_RGBA;
    pixel_format_description.cColorBits = 32;

    pixel_format = ::ChoosePixelFormat(device_context, &pixel_format_description);
    if ((0 == pixel_format) || (FALSE == ::SetPixelFormat(device_context, pixel_format, &pixel_format_description)))
    {
        ::ReleaseDC(windowHandle, device_context);
        return FALSE;
    }
    ::ReleaseDC(windowHandle, device_context);

    windowData->deviceContext = ::GetDC(windowHandle);
    if (NULL == windowData->deviceContext)
    {
        return FALSE;
    }
    if (NULL != g_renderContext)
    {
        return TRUE;
    }

    temporary_context = wglCreateContext(windowData->deviceContext);
    if (NULL == temporary_context)
    {
        CleanupDeviceWGL(windowHandle, windowData);
        return FALSE;
    }
    if (FALSE == wglMakeCurrent(windowData->deviceContext, temporary_context))
    {
        wglDeleteContext(temporary_context);
        CleanupDeviceWGL(windowHandle, windowData);
        return FALSE;
    }

    major = 0;
    minor = 0;
    glGetIntegerv(OPENGL_MAJOR_VERSION_PARAMETER, &major);
    glGetIntegerv(OPENGL_MINOR_VERSION_PARAMETER, &minor);
    version_string = (const char_t*)glGetString(GL_VERSION);
    if ((0 == major) && (0 == minor) && (NULL != version_string))
    {
        sscanf_s(version_string, "%d.%d", &major, &minor);
    }
    version         = (GLuint)((major * 100) + (minor * 10));
    g_renderContext = temporary_context;
    if (OPENGL_VERSION_3_0 <= version)
    {
        return TRUE;
    }

    create_context_attribs = (WglCreateContextAttribsFunction_t)wglGetProcAddress("wglCreateContextAttribsARB");
    context_attributes[0]  = WGL_MAJOR_VERSION_ATTRIBUTE;
    context_attributes[1]  = 3;
    context_attributes[2]  = WGL_MINOR_VERSION_ATTRIBUTE;
    context_attributes[3]  = 0;
    context_attributes[4]  = WGL_PROFILE_MASK_ATTRIBUTE;
    context_attributes[5]  = WGL_CORE_PROFILE_BIT;
    context_attributes[6]  = 0;
    new_context            = NULL;
    if (NULL != create_context_attribs)
    {
        new_context = create_context_attribs(windowData->deviceContext, NULL, context_attributes);
    }
    if (NULL != new_context)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(temporary_context);
        g_renderContext = new_context;
        wglMakeCurrent(windowData->deviceContext, g_renderContext);
    }
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Releases a Win32 window device context.
 */
static void CleanupDeviceWGL(HWND windowHandle, WglWindowData_t* windowData)
{
    if (NULL != windowData->deviceContext)
    {
        wglMakeCurrent(NULL, NULL);
        ::ReleaseDC(windowHandle, windowData->deviceContext);
        windowData->deviceContext = NULL;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates renderer data for an ImGui platform viewport.
 */
static void HookRendererCreateWindow(ImGuiViewport* viewport)
{
    WglWindowData_t* window_data;

    LOGASSERT(NULL == viewport->RendererUserData);
    window_data = IM_NEW(WglWindowData_t);
    LOGASSERT(CreateDeviceWGL((HWND)viewport->PlatformHandle, window_data));
    viewport->RendererUserData = window_data;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Releases renderer data for an ImGui platform viewport.
 */
static void HookRendererDestroyWindow(ImGuiViewport* viewport)
{
    WglWindowData_t* window_data;

    if (NULL != viewport->RendererUserData)
    {
        window_data = (WglWindowData_t*)viewport->RendererUserData;
        CleanupDeviceWGL((HWND)viewport->PlatformHandle, window_data);
        IM_DELETE(window_data);
        viewport->RendererUserData = NULL;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Activates the shared OpenGL context for a platform viewport.
 */
static void HookPlatformRenderWindow(ImGuiViewport* viewport, void* data)
{
    WglWindowData_t* window_data;

    (void)data;
    window_data = (WglWindowData_t*)viewport->RendererUserData;
    if (NULL != window_data)
    {
        wglMakeCurrent(window_data->deviceContext, g_renderContext);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Presents a platform viewport.
 */
static void HookRendererSwapBuffers(ImGuiViewport* viewport, void* data)
{
    WglWindowData_t* window_data;

    (void)data;
    window_data = (WglWindowData_t*)viewport->RendererUserData;
    if (NULL != window_data)
    {
        LOGASSERT(FALSE != ::wglSwapBuffers(window_data->deviceContext));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Routes Win32 input and window events to ImGui and the renderer.
 */
static LRESULT WINAPI WindowProcedure(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter)
{
    WindowThreadState_t* window;
    CREATESTRUCTW*       create;
    RECT*                lpRect;
    LRESULT              result;

    if (WM_NCCREATE == message)
    {
        create = (CREATESTRUCTW*)longParameter;
        window = (WindowThreadState_t*)create->lpCreateParams;
        ::SetWindowLongPtrW(windowHandle, GWLP_USERDATA, (LONG_PTR)window);
    }
    window = (WindowThreadState_t*)::GetWindowLongPtrW(windowHandle, GWLP_USERDATA);
    if (0 != ImGui_ImplWin32_WndProcHandler(windowHandle, message, wordParameter, longParameter))
    {
        return TRUE;
    }
    switch (message)
    {
    case WM_ERASEBKGND:
    {
        return 1;
    }
    case WM_NCCALCSIZE:
    {
        result = ::DefWindowProcW(windowHandle, message, wordParameter, longParameter);
        lpRect = (RECT*)longParameter;
        AcquireSRWLockExclusive(&window->presentLock);
        window->rcClient = (*lpRect);
        if (window->fPresentReady)
        {
            window->fGeometryPending = TRUE;
            while (window->fGeometryPending)
            {
                ASSERT(SleepConditionVariableSRW(&window->presentCondition, &window->presentLock, INFINITE, 0));
            }
        }
        ReleaseSRWLockExclusive(&window->presentLock);
        return result;
    }
    case WM_SYSCOMMAND:
    {
        if (SC_KEYMENU == (wordParameter & 0xfff0))
        {
            return 0;
        }
        break;
    }
    case WM_ENTERSIZEMOVE:
    {
        LOGASSERT(0 != ::SetTimer(windowHandle, MOVE_SIZE_TIMER_ID, MOVE_SIZE_TIMER_INTERVAL_MS, NULL));
        return 0;
    }
    case WM_EXITSIZEMOVE:
    {
        LOGASSERT(FALSE != ::KillTimer(windowHandle, MOVE_SIZE_TIMER_ID));
        return 0;
    }
    case WM_TIMER:
    {
        if (MOVE_SIZE_TIMER_ID == wordParameter)
        {
            if (window->hasEditors && (FALSE == window->isDone.load()))
            {
                AcquireSRWLockExclusive(&window->presentLock);
                BeginFrame();
                DrawEditors(window);
                EndFrame(window);
                ReleaseSRWLockExclusive(&window->presentLock);
                LOGASSERT(S_OK == ::DwmFlush());
            }
            return 0;
        }
        break;
    }
    case WM_DESTROY:
    {
        window->isDone.store(TRUE);
        ::PostQuitMessage(0);
        return 0;
    }
    case WM_CLOSE:
    {
        window->isDone.store(TRUE);
        return 0;
    }
    default:
    {
        break;
    }
    }
    return ::DefWindowProcW(windowHandle, message, wordParameter, longParameter);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates the main window, signals the presenter thread, and dispatches Win32 messages.
 */
static DWORD WINAPI WindowThread(LPVOID parameter)
{
    uint32_t panel_index;
    WindowThreadState_t*       window;
    WNDCLASSEXW                window_class;
    ImGuiIO*                   io;
    ImGuiStyle*                style;
    ImGuiPlatformIO*           platform_io;
    WglSwapIntervalFunction_t  swap_interval;
    MSG                        message;

    window = (WindowThreadState_t*)parameter;
    window->configurationEditor = NULL;
    window->definitionEditor    = NULL;
    window->isDefinitionActive  = FALSE;
    window->hasEditors          = FALSE;
    window->isFolderReadable    = FALSE;
    window->lastScanTime        = -FILE_SCAN_INTERVAL_SECONDS;
    window->fileCount           = 0;
    strncpy_s(window->folderPath, sizeof(window->folderPath), DEFAULT_FOLDER_PATH, _TRUNCATE);
    memset(&window_class, 0, sizeof(window_class));
    window_class.cbSize        = sizeof(window_class);
    window_class.style         = CS_OWNDC;
    window_class.lpfnWndProc   = WindowProcedure;
    window_class.hInstance     = ::GetModuleHandleW(NULL);
    window_class.lpszClassName = L"ARINC661Configuration";
    LOGASSERT(0 != ::RegisterClassExW(&window_class));
    window->windowHandle = ::CreateWindowW(
        window_class.lpszClassName,
        L"ARINC 661 Configuration",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        (int)(1280 * window->mainScale),
        (int)(800 * window->mainScale),
        NULL,
        NULL,
        window_class.hInstance,
        window);
    LOGASSERT(NULL != window->windowHandle);
    ::ShowWindow(window->windowHandle, SW_SHOWDEFAULT);
    ::UpdateWindow(window->windowHandle);

    if (FALSE == CreateDeviceWGL(window->windowHandle, &g_mainWindow))
    {
        CleanupDeviceWGL(window->windowHandle, &g_mainWindow);
        window->isDone.store(TRUE);
        LOGASSERT(FALSE != ::SetEvent(window->readyEvent));
        LOGASSERT(FALSE != ::DestroyWindow(window->windowHandle));
        LOGASSERT(FALSE != ::UnregisterClassW(window_class.lpszClassName, window_class.hInstance));
        return 1;
    }
    LOGASSERT(FALSE != wglMakeCurrent(g_mainWindow.deviceContext, g_renderContext));
    window->presentContext = wglCreateContext(g_mainWindow.deviceContext);
    LOGASSERT(NULL != window->presentContext);
    LOGASSERT(FALSE != wglShareLists(g_renderContext, window->presentContext));
    swap_interval = (WglSwapIntervalFunction_t)wglGetProcAddress("wglSwapIntervalEXT");
    LOGASSERT(NULL != swap_interval);
    LOGASSERT(FALSE != swap_interval(1));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->UserData = window;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    style = &ImGui::GetStyle();
    style->ScaleAllSizes(window->mainScale);
    style->FontScaleDpi         = window->mainScale;
    io->ConfigDpiScaleFonts     = TRUE;
    io->ConfigDpiScaleViewports = TRUE;
    if (0 != (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        style->WindowRounding              = 0.0f;
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_InitForOpenGL(window->windowHandle);
    ImGui_ImplOpenGL3_Init();
    if (0 != (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        platform_io = &ImGui::GetPlatformIO();
        LOGASSERT(NULL == platform_io->Renderer_CreateWindow);
        LOGASSERT(NULL == platform_io->Renderer_DestroyWindow);
        LOGASSERT(NULL == platform_io->Renderer_SwapBuffers);
        LOGASSERT(NULL == platform_io->Platform_RenderWindow);
        platform_io->Renderer_CreateWindow  = HookRendererCreateWindow;
        platform_io->Renderer_DestroyWindow = HookRendererDestroyWindow;
        platform_io->Renderer_SwapBuffers   = HookRendererSwapBuffers;
        platform_io->Platform_RenderWindow  = HookPlatformRenderWindow;
    }

    window->configurationEditor = (Arinc661_Configuration_Editor_t*)OS_CALLOC(sizeof(Arinc661_Configuration_Editor_t));
    Arinc661_Configuration_Initialize(window->configurationEditor);
    window->definitionEditor = (Arinc661_Definition_Editor_t*)OS_CALLOC(sizeof(Arinc661_Definition_Editor_t));
    Arinc661_Definition_Initialize(window->definitionEditor);
    window->isHelpOpen    = FALSE;
    window->helpFilter[0] = 0;
    strncpy_s(window->helpTopic, sizeof(window->helpTopic), HELP_GETTING_STARTED, _TRUNCATE);
    window->isHelpOpen    = FALSE;
    window->saveNote[0]   = 0;
    window->hasLayout     = FALSE;
    window->resetsLayout  = FALSE;
    window->paletteFilter[0] = 0;
    window->logCount      = 0;
    window->lastStatus[0] = 0;
    window->logAutoScroll = true;
    for (panel_index = 0; panel_index < PANEL_COUNT; ++panel_index)
    {
        window->panelOpen[panel_index] = true;
    }
    window->context = (Arinc661_Context_t*)OS_CALLOC(sizeof(Arinc661_Context_t));
    Arinc661_Context_Initialize(window->context, window->configurationEditor, window->definitionEditor);
    ScanFolder(window);
    if (NULL != g_openConfiguration)
    {
        Arinc661_Configuration_LoadFile(window->configurationEditor, g_openConfiguration);
        (void)Arinc661_Context_OpenReferencedDefinitions(window->context);
    }
    if (NULL != g_openDefinition)
    {
        Arinc661_Definition_LoadFile(window->definitionEditor, g_openDefinition);
        if (NULL != g_selectName)
        {
            Arinc661_Definition_SelectByName(window->definitionEditor, g_selectName);
        }
    }
    window->isDefinitionActive = g_focusDefinition;
    window->hasEditors = TRUE;
    window->isInitialized = TRUE;
    LOGASSERT(FALSE != ::SetEvent(window->readyEvent));

    while (FALSE == window->isDone.load())
    {
        while (::PeekMessageW(&message, NULL, 0U, 0U, PM_REMOVE))
        {
            if (WM_QUIT == message.message)
            {
                window->isDone.store(TRUE);
                break;
            }
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }
        if (window->isDone.load())
        {
            break;
        }

        DrawFrame(window);
    }

    AcquireSRWLockExclusive(&window->presentLock);
    window->isDone.store(TRUE);
    window->fPresentReady = FALSE;
    if (window->fGeometryPending)
    {
        window->fGeometryPending = FALSE;
        WakeConditionVariable(&window->presentCondition);
    }
    ReleaseSRWLockExclusive(&window->presentLock);
    LOGASSERT(WAIT_OBJECT_0 == ::WaitForSingleObject(window->presentStoppedEvent, INFINITE));
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceWGL(window->windowHandle, &g_mainWindow);
    LOGASSERT(FALSE != wglDeleteContext(g_renderContext));
    LOGASSERT(FALSE != ::DestroyWindow(window->windowHandle));
    LOGASSERT(FALSE != ::UnregisterClassW(window_class.lpszClassName, window_class.hInstance));
    return 0;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Opens the configuration editor and its live scene view.
 */
int main(int argc, char** argv)
{
    float32_t                            main_scale;
    WindowThreadState_t                  window;
    HANDLE                               window_thread;
    HDC                                  present_device_context;
    D3DKMT_WAITFORVERTICALBLANKEVENT      vbe;

    // Route assertion and runtime failures to stderr instead of dialog boxes
    Os_Diagnostics_Initialize();
    ParseOptions(argc, argv);
    ImGui_ImplWin32_EnableDpiAwareness();
    main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    window.windowHandle     = NULL;
    window.presentContext   = NULL;
    window.mainScale        = main_scale;
    window.isInitialized    = FALSE;
    window.fPresentReady    = FALSE;
    window.fFrameReady      = FALSE;
    window.fGeometryPending = FALSE;
    window.isDone.store(FALSE);
    memset(&window.rcClient, 0, sizeof(window.rcClient));
    InitializeSRWLock(&window.presentLock);
    InitializeConditionVariable(&window.presentCondition);
    window.readyEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    LOGASSERT(NULL != window.readyEvent);
    window.presentStoppedEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    LOGASSERT(NULL != window.presentStoppedEvent);
    window_thread = ::CreateThread(NULL, 0, WindowThread, &window, 0, NULL);
    LOGASSERT(NULL != window_thread);
    LOGASSERT(WAIT_OBJECT_0 == ::WaitForSingleObject(window.readyEvent, INFINITE));
    LOGASSERT(FALSE != ::CloseHandle(window.readyEvent));
    if (FALSE == window.isInitialized)
    {
        LOGASSERT(WAIT_OBJECT_0 == ::WaitForSingleObject(window_thread, INFINITE));
        LOGASSERT(FALSE != ::CloseHandle(window.presentStoppedEvent));
        LOGASSERT(FALSE != ::CloseHandle(window_thread));
        return 1;
    }
    /* DCX_CACHE gives the presenter a separate HDC despite the window's CS_OWNDC style. */
    present_device_context = ::GetDCEx(window.windowHandle, NULL, DCX_CACHE);
    LOGASSERT(NULL != present_device_context);
    LOGASSERT(g_mainWindow.deviceContext != present_device_context);
    LOGASSERT(::GetPixelFormat(g_mainWindow.deviceContext) == ::GetPixelFormat(present_device_context));
    LOGASSERT(FALSE != wglMakeCurrent(present_device_context, window.presentContext));
    OpenVerticalBlankEvent(present_device_context, &vbe);
    while (FALSE == window.isDone.load())
    {
        LOGASSERT(NT_SUCCESS(D3DKMTWaitForVerticalBlankEvent(&vbe)));
        AcquireSRWLockExclusive(&window.presentLock);
        if (window.fGeometryPending)
        {
            window.fFrameReady = FALSE;
            window.fGeometryPending = FALSE;
            WakeConditionVariable(&window.presentCondition);
        }
        else if (window.fFrameReady)
        {
            LOGASSERT(FALSE != ::wglSwapBuffers(present_device_context));
            window.fFrameReady   = FALSE;
            window.fPresentReady = TRUE;
        }
        ReleaseSRWLockExclusive(&window.presentLock);
    }

    LOGASSERT(FALSE != wglMakeCurrent(NULL, NULL));
    LOGASSERT(FALSE != wglDeleteContext(window.presentContext));
    LOGASSERT(0 != ::ReleaseDC(window.windowHandle, present_device_context));
    LOGASSERT(FALSE != ::SetEvent(window.presentStoppedEvent));
    LOGASSERT(WAIT_OBJECT_0 == ::WaitForSingleObject(window_thread, INFINITE));
    LOGASSERT(FALSE != ::CloseHandle(window.presentStoppedEvent));
    LOGASSERT(FALSE != ::CloseHandle(window_thread));
    return 0;
}
