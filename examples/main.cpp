/*=================================================================================================
** 1. References
**===============================================================================================*/

/* Win32 and OpenGL setup adapted from imgui-docking/examples/example_win32_opengl3/main.cpp. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "arinc661_context.h"
#include "arinc661_icons.h"
#include "arinc661_definition_editor.h"
#include "arinc661_help.h"
#include "arinc661_update.h"
#include "arinc661_version.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

extern "C"
{
#include "faceoe_utl/faceoe_utl_util.h"
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
#include <shobjidl.h>
#include <shlobj.h>
#include <winternl.h>
#include <d3dkmthk.h>
#include <dwmapi.h>
#include <GL/gl.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

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
#define FILE_LIST_ROWS              (6)
#define FILE_COLUMN_COUNT           (5)
#define FILE_COLUMN_NAME            ("Name")
#define FILE_COLUMN_KIND            ("Kind")
#define FILE_COLUMN_SIZE            ("Size")
#define FILE_COLUMN_MODIFIED        ("Modified")
#define FILE_COLUMN_STATE           ("State")
#define FILE_COLUMN_NAME_ID         (1)
#define FILE_COLUMN_KIND_ID         (2)
#define FILE_COLUMN_SIZE_ID         (3)
#define FILE_COLUMN_MODIFIED_ID     (4)
#define FILE_COLUMN_STATE_ID        (5)
#define FILE_TABLE_FLAGS            (ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp)
#define FILE_KIND_DEFINITION        ("Definition File")
#define FILE_KIND_CONFIGURATION     ("Configuration")
#define FILE_STATE_OPEN             ("Open")
#define FILE_STATE_MODIFIED         ("Modified")
#define FILE_STATE_CLOSED           ("")
#define FILE_ROW_MENU_ID            ("##file row menu")
#define FILE_OPEN_ROW_LABEL         ("Open")
#define SIZE_BYTES_FORMAT           ("%llu B")
#define SIZE_KILOBYTES_FORMAT       ("%.1f KB")
#define SIZE_MEGABYTES_FORMAT       ("%.1f MB")
#define TIME_FORMAT                 ("%04u-%02u-%02u %02u:%02u")
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
#define UI_TEXT_SIZE                (16.0f)
#define PLAY_LABEL                  ("Play")
#define RECENT_MENU_LABEL           ("Open Recent")
#define OPEN_FILE_LABEL             ("Open File...")
#define OPEN_FILE_SHORTCUT          ("Ctrl+O")
#define OPEN_FILE_CHORD             (ImGuiMod_Ctrl | ImGuiKey_O)
#define OPEN_FOLDER_LABEL           ("Open Folder...")
#define REVEAL_LABEL                ("Reveal in Explorer")
#define OPEN_DIALOG_TITLE           (L"Open a Definition File or configuration")
#define FOLDER_DIALOG_TITLE         (L"Open a folder of Definition Files and configurations")
#define NOT_A_DOCUMENT_FORMAT       ("%s is not an ARINC 661 Definition File or a FACE configuration.")
#define OPENED_PATH_FORMAT          ("Opened %s")
#define ICON_RESOURCE_ID            (1)
#define ABOUT_LABEL                 ("About CDS Editor")
#define ABOUT_POPUP_ID              ("About CDS Editor##about")
// Unparenthesised so it stays one string literal
#define ABOUT_DETAILS_FORMAT        "Version %s\nBuilt %s %s\nDear ImGui %s (docking)\nOpenGL %s\nInstalled in %s\n"
#define ABOUT_UPDATE_URL_FORMAT     ("Update source: %s")
#define ABOUT_NO_UPDATE_URL_LABEL   ("Update source: not configured (set updateUrl in cds-editor.cfg or UpdateUrl in the registry)")
#define ABOUT_COPY_LABEL            ("Copy Details")
#define ABOUT_CLOSE_LABEL           ("Close")
#define ABOUT_HELP_LABEL            ("Open Help")
#define ABOUT_DESCRIPTION_LABEL     ("Cockpit Display System editor: ARINC 661 Definition Files and FACE display management configurations.")
#define ABOUT_COPYRIGHT_LABEL       ("FACEOE. Segoe UI and Segoe MDL2 Assets are Microsoft fonts used from the local system.")
#define CHECK_UPDATES_LABEL         ("Check for Updates...")
#define UPDATE_POPUP_ID             ("Software Update##update")
#define UPDATE_AVAILABLE_FORMAT     ("Version %s is available (you have %s).")
#define UPDATE_NOTES_FORMAT         ("What's new: %s")
#define UPDATE_INSTALL_LABEL        ("Download and Install")
#define UPDATE_LATER_LABEL          ("Later")
#define UPDATE_SKIP_LABEL           ("Skip This Version")
#define UPDATE_DOWNLOADING_LABEL    ("Downloading the update...")
#define UPDATE_CHECKING_LABEL       ("Checking for updates...")
#define UPDATE_UP_TO_DATE_LABEL     ("You have the newest version.")
#define UPDATE_UNREACHABLE_LABEL    ("The update source could not be reached.")
#define UPDATE_MALFORMED_LABEL      ("The update source did not return a readable manifest.")
#define UPDATE_CORRUPT_LABEL        ("The downloaded package failed its integrity check and was not installed.")
#define UPDATE_NO_SOURCE_LABEL      ("No update source is configured.")
#define UPDATE_APPLY_FAIL_LABEL     ("The installer could not be started.")
#define UPDATE_APPLYING_LABEL       ("Installing the update; the editor will restart.")
#define UPDATE_OK_LABEL             ("OK")
#define UPDATE_STATE_IDLE           (0)
#define UPDATE_STATE_CHECKING       (1)
#define UPDATE_STATE_CHECKED        (2)
#define UPDATE_STATE_DOWNLOADING    (3)
#define UPDATE_STATE_DOWNLOADED     (4)
#define UPDATE_CHECK_INTERVAL_SECONDS (24.0 * 60.0 * 60.0)
#define PREFERENCE_UPDATE_URL_KEY   ("updateUrl")
#define PREFERENCE_UPDATE_CHECKED_KEY ("updateChecked")
#define PREFERENCE_UPDATE_SKIP_KEY  ("updateSkip")
#define UPDATE_REGISTRY_KEY         ("Software\\FACEOE\\CDS Editor")
#define UPDATE_REGISTRY_VALUE       ("UpdateUrl")
#define WINDOW_TITLE                (L"CDS Editor")
#define WINDOW_DEFAULT_WIDTH        (1280)
#define WINDOW_DEFAULT_HEIGHT       (800)
#define DIALOG_IDLE                 (0)
#define DIALOG_RUNNING              (1)
#define DIALOG_PICKED_FILE          (2)
#define DIALOG_PICKED_FOLDER        (3)
#define DIALOG_CANCELLED            (4)
#define BROWSE_FILE_LABEL           ("Browse File...")
#define BROWSE_FOLDER_LABEL         ("Browse...")
#define PREFAB_EXTENSION            (".prefab.xml")
#define PREFAB_SECTION_LABEL        ("Prefabs")
#define PREFAB_HINT_LABEL           ("Click to add the prefab at the selection")
#define SAVE_PREFAB_LABEL           ("Save Selection as Prefab...")
#define SAVE_PREFAB_POPUP_ID        ("Save Prefab")
#define SAVE_PREFAB_NAME_HINT       ("Prefab name")
#define SAVE_PREFAB_OK_LABEL        ("Save")
#define SAVE_PREFAB_CANCEL_LABEL    ("Cancel")
#define SAVE_PREFAB_NOTE_FORMAT     ("Saved prefab %s")
#define SAVE_PREFAB_FAIL_LABEL      ("Could not save the prefab; the name must be a plain file name and something must be selected.")
#define MAX_PREFABS                 (64)
#define RECENT_EMPTY_LABEL          ("(no recent files)")
#define RECENT_CLEAR_LABEL          ("Clear Recent")
#define RECENT_COUNT                (8)
#define PREFERENCE_RECENT_KEY       ("recent")
#define PREFERENCE_FOLDER_KEY       ("folder")
#define START_TITLE_LABEL           ("CDS Editor")
#define START_SUBTITLE_LABEL        ("Open a file from the Files panel or the list below, or start something new.")
#define START_RECENT_LABEL          ("Recent")
#define START_LEARN_LABEL           ("Learn")
#define START_OPEN_HELP_LABEL       ("Open the reference (F1)")
#define START_SHORTCUTS_LABEL       ("Keyboard shortcuts")
#define START_BUTTON_WIDTH          (220.0f)
#define LOG_FILTER_HINT             ("Filter log")
#define LOG_ERRORS_LABEL            ("Errors")
#define LOG_WARNINGS_LABEL          ("Warnings")
#define LOG_INFO_LABEL              ("Info")
#define LOG_ERROR_COLOR             (ImVec4(1.0f, 0.45f, 0.45f, 1.0f))
#define LOG_WARNING_COLOR           (ImVec4(1.0f, 0.8f, 0.4f, 1.0f))
#define STOP_LABEL                  ("Stop")
#define PLAY_SHORTCUT_LABEL         ("F5")
#define PLAY_CHORD                  (ImGuiKey_F5)
#define BUILD_LABEL                 ("Build All")
#define BUILD_SHORTCUT_LABEL        ("F7")
#define BUILD_CHORD                 (ImGuiKey_F7)
#define BUILD_REPORT_BYTES          (8192)
#define COMMAND_PALETTE_LABEL       ("Command Palette")
#define COMMAND_PALETTE_SHORTCUT    ("Ctrl+Shift+P")
#define COMMAND_PALETTE_CHORD       (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P)
#define COMMAND_PALETTE_ID          ("##command palette")
#define COMMAND_PALETTE_HINT        ("Type a command, a file, or a widget name")
#define COMMAND_PALETTE_ROWS        (12.0f)
#define TIMELINE_PLAY_LABEL         ("Play")
#define TIMELINE_PAUSE_LABEL        ("Pause")
#define TIMELINE_STOP_LABEL         ("Rest")
#define TIMELINE_LOOP_LABEL         ("Loop")
#define TIMELINE_TIME_LABEL         ("Time (s)")
#define TIMELINE_EMPTY_LABEL        ("The active Definition File has no animation widgets; add an AnimationGroup with rotation, scale, translation, or parameter animations.")
#define TIMELINE_BAR_COLOR          (IM_COL32(97, 160, 245, 160))
#define TIMELINE_CURSOR_COLOR       (IM_COL32(255, 200, 80, 255))
#define TIMELINE_ROW_HEIGHT         (20.0f)
#define TIMELINE_NAME_WIDTH         (220.0f)
#define STATS_FORMAT                ("%u widgets  |  %u commands  |  %u points  |  %.1f ms")
#define PLAY_BANNER_LABEL           ("PLAY MODE: clicks act as a crew member; Stop restores the documents")
#define PLAY_BANNER_COLOR           (ImVec4(1.0f, 0.75f, 0.3f, 1.0f))
#define COPY_LABEL                  ("Copy")
#define PASTE_LABEL                 ("Paste")
#define RENAME_LABEL                ("Rename")
#define COPY_SHORTCUT_LABEL         ("Ctrl+C")
#define PASTE_SHORTCUT_LABEL        ("Ctrl+V")
#define RENAME_SHORTCUT_LABEL       ("F2")
#define COPY_CHORD                  (ImGuiMod_Ctrl | ImGuiKey_C)
#define PASTE_CHORD                 (ImGuiMod_Ctrl | ImGuiKey_V)
#define RENAME_CHORD                (ImGuiKey_F2)
#define UI_SCALE_LABEL              ("UI Scale")
#define THEME_DARK_LABEL            ("Dark Theme")
#define THEME_LIGHT_LABEL           ("Light Theme")
#define SHORTCUTS_LABEL             ("Keyboard Shortcuts")
#define UI_SCALE_MIN                (0.75f)
#define UI_SCALE_MAX                (2.0f)
#define PREFERENCES_FILE            ("cds-editor.cfg")
#define PREFERENCE_SCALE_KEY        ("scale")
#define PREFERENCE_THEME_KEY        ("theme")
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
#define LAYOUT_MENU_LABEL           ("Layout")
#define LAYOUT_DEFAULT_LABEL        ("Default")
#define LAYOUT_TALL_LABEL           ("Tall")
#define LAYOUT_WIDE_LABEL           ("Wide")
#define LAYOUT_TALL_SPLIT_FRACTION  (0.45f)
#define PANEL_COUNT                 (9)
#define PANEL_TIMELINE              ("Timeline")
#define PANEL_HISTORY               ("History")
#define HISTORY_EMPTY_LABEL         ("No Definition File is active.")
#define HISTORY_CURRENT_MARK        ("  <- current")
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
#define LAYOUT_RIGHT_FRACTION       (0.30f)
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
#define TIME_OPTION                 ("--time")
#define PLAY_OPTION                 ("--play")
#define ABOUT_OPTION                ("--about")
#define CHECK_UPDATES_OPTION        ("--check-updates")
#define FRAMES_OPTION               ("--frames")
#define LAYOUT_OPTION               ("--layout")
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
    PANEL_INDEX_FILES, PANEL_INDEX_PROBLEMS, PANEL_INDEX_HIERARCHY, PANEL_INDEX_PALETTE, PANEL_INDEX_SCENE, PANEL_INDEX_INSPECTOR, PANEL_INDEX_LOG, PANEL_INDEX_TIMELINE, PANEL_INDEX_HISTORY
} Panel_t;

/** Dock layout presets, as the Window menu lists them. */
typedef enum LayoutPreset_e
{
    LAYOUT_PRESET_DEFAULT, ///< Side columns and a bottom strip of tool tabs.
    LAYOUT_PRESET_TALL,    ///< Scene full height; tools stacked in the right column.
    LAYOUT_PRESET_WIDE     ///< Scene full width; every tool in the bottom strip.
} LayoutPreset_t;

static const char_t* const c_panelNames[PANEL_COUNT] = {PANEL_FILES, PANEL_PROBLEMS, PANEL_HIERARCHY, PANEL_PALETTE, PANEL_SCENE, PANEL_INSPECTOR, PANEL_LOG, PANEL_TIMELINE, PANEL_HISTORY};
/** Layout preset names, in LAYOUT_PRESET_ order. */
static const char_t* const c_layoutNames[] = {LAYOUT_DEFAULT_LABEL, LAYOUT_TALL_LABEL, LAYOUT_WIDE_LABEL};

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
    HWND                             windowHandle;                                         ///< Main Win32 window.
    HANDLE                           readyEvent;                                           ///< Signals when rendering setup is complete.
    HANDLE                           presentStoppedEvent;                                  ///< Signals when the presenter releases its device context.
    HGLRC                            presentContext;                                       ///< OpenGL context used by the presenter thread.
    Arinc661_Configuration_Editor_t* configurationEditor;                                  ///< FACE display management configuration editor.
    Arinc661_DefinitionEditor_t*     definitionEditor;                                     ///< ARINC 661 Definition File editor.
    Arinc661_Context_t*              context;                                              ///< Shared application context across both editors.
    bool_t                           isHelpOpen;                                           ///< TRUE while the Help window is shown.
    char_t                           helpFilter[MAX_HELP_FILTER_BYTES];                    ///< Search text of the Help window.
    char_t                           helpTopic[ARINC661_MAX_NAME_BYTES];                   ///< Topic the Help window shows.
    char_t                           saveNote[SAVED_WITH_ERRORS_BYTES];                    ///< Note after a configuration save with errors, or empty.
    bool                             panelOpen[PANEL_COUNT];                               ///< Visibility of each dockable panel, in PANEL_ order.
    bool_t                           hasLayout;                                            ///< TRUE once the default dock layout was built.
    bool_t                           resetsLayout;                                         ///< TRUE to rebuild the layout preset next frame.
    LayoutPreset_t                   layoutPreset;                                         ///< Preset that Reset Layout rebuilds.
    char_t                           paletteFilter[MAX_HELP_FILTER_BYTES];                 ///< Palette filter text.
    char_t                           logLines[LOG_CAPACITY][LOG_LINE_BYTES];               ///< Status history ring.
    uint32_t                         logCount;                                             ///< Lines written to the ring.
    char_t                           lastStatus[LOG_LINE_BYTES];                           ///< Last status text appended.
    bool                             logAutoScroll;                                        ///< TRUE to keep the log scrolled to the end.
    float32_t                        uiScale;                                              ///< Font scale chosen in the View menu.
    bool_t                           isTimelinePlaying;                                    ///< TRUE while the timeline advances with the clock.
    bool_t                           isTimelineLooping;                                    ///< TRUE to wrap at the end.
    float32_t                        timelineTime;                                         ///< Current animation time in seconds.
    float32_t                        timelineLength;                                       ///< End of the last animation in seconds.
    bool_t                           isPaletteOpen;                                        ///< TRUE while the command palette is shown.
    char_t                           recentNames[RECENT_COUNT][MAX_FILE_PATH_BYTES];       ///< Most recently opened files as full paths, newest first.
    char_t                           pendingOpenPath[MAX_FILE_PATH_BYTES];                 ///< A path dropped on the window or given on the command line, opened next frame.
    std::atomic<int32_t>             dialogState;                                          ///< DIALOG_* state of the native picker running on its own thread.
    std::atomic<int32_t>             updateState;                                          ///< UPDATE_STATE_* of the updater thread.
    Arinc661_Update_Result_t         updateResult;                                         ///< Outcome of the last check or download, read once the state settles.
    Arinc661_Update_Manifest_t       updateManifest;                                       ///< What the update source offers.
    char_t                           updateUrl[ARINC661_UPDATE_MAX_URL_BYTES];             ///< Manifest source, empty when not configured.
    char_t                           updatePackagePath[MAX_FILE_PATH_BYTES];               ///< Downloaded package.
    char_t                           updateSkipVersion[ARINC661_UPDATE_MAX_VERSION_BYTES]; ///< Version the user chose to skip.
    float64_t                        updateCheckedAt;                                      ///< Unix time of the last automatic check.
    bool_t                           updateIsManual;                                       ///< TRUE when the user asked, so every outcome is shown.
    bool_t                           opensUpdatePopup;                                     ///< TRUE to open the update pop-up this frame.
    bool_t                           opensAboutPopup;                                      ///< TRUE to open the About box this frame.
    bool_t                           isApplyingUpdate;                                     ///< TRUE once the installer has been started.
    bool_t                           dialogIsFolder;                                       ///< TRUE while the running picker picks a folder.
    char_t                           dialogPath[MAX_FILE_PATH_BYTES];                      ///< Path the picker chose, read once the state says picked.
    char_t                           prefabNames[MAX_PREFABS][MAX_FILE_NAME_BYTES];        ///< Prefab files found by the last folder scan.
    uint32_t                         prefabCount;                                          ///< Entries in the prefab list.
    bool_t                           opensPrefabPopup;                                     ///< TRUE to open the save-prefab pop-up this frame.
    char_t                           prefabName[MAX_FILE_NAME_BYTES];                      ///< Name typed into the save-prefab pop-up.
    bool_t                           recentIsDefinition[RECENT_COUNT];                     ///< Kind of each recent file.
    uint32_t                         recentCount;                                          ///< Entries in the recent list.
    char_t                           logFilter[MAX_HELP_FILTER_BYTES];                     ///< Log text filter.
    bool_t                           logShowsErrors;                                       ///< TRUE to list error lines.
    bool_t                           logShowsWarnings;                                     ///< TRUE to list warning lines.
    bool_t                           logShowsInfo;                                         ///< TRUE to list the other lines.
    char_t                           commandFilter[MAX_HELP_FILTER_BYTES];                 ///< Command palette text.
    bool_t                           isLightTheme;                                         ///< TRUE for the light colour scheme.
    bool_t                           preferencesDirty;                                     ///< TRUE until the preferences file is written.
    bool_t                           isDefinitionActive;                                   ///< TRUE while the Definition File section has the focus.
    bool_t                           hasEditors;                                           ///< TRUE once both editors exist.
    bool_t                           isFolderReadable;                                     ///< TRUE when the last scan could read the folder.
    float64_t                        lastScanTime;                                         ///< ImGui time of the last folder scan.
    uint32_t                         fileCount;                                            ///< Entries found by the last folder scan.
    char_t                           folderPath[MAX_FOLDER_PATH_BYTES];                    ///< Folder shared by both editors.
    char_t                           fileNames[MAX_PAGE_FILES][MAX_FILE_NAME_BYTES];       ///< Names found by the last folder scan.
    bool_t                           fileIsDefinition[MAX_PAGE_FILES];                     ///< TRUE for a Definition File, FALSE for a configuration.
    uint64_t                         fileSizes[MAX_PAGE_FILES];                            ///< Bytes of each file.
    uint64_t                         fileTimes[MAX_PAGE_FILES];                            ///< Last write time of each file as a FILETIME value.
    uint32_t                         fileOrder[MAX_PAGE_FILES];                            ///< Display order after sorting.
    float32_t                        mainScale;                                            ///< Initial display scale.
    SRWLOCK                          presentLock;                                          ///< Protects geometry, drawing, and buffer swaps.
    CONDITION_VARIABLE               presentCondition;                                     ///< Wakes the window thread after geometry is acknowledged.
    RECT                             rcClient;                                             ///< Current client rectangle.
    BOOL                             fPresentReady;                                        ///< TRUE after the first presented frame.
    BOOL                             fFrameReady;                                          ///< TRUE when a new frame can be presented.
    BOOL                             fGeometryPending;                                     ///< TRUE while geometry acknowledgement is pending.
    bool_t                           isInitialized;                                        ///< TRUE when rendering setup succeeds.
    std::atomic<bool_t>              isDone;                                               ///< TRUE after the main window is closed.
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
static float32_t     g_startTime = -1.0f;     ///< Animation time shown after opening, or negative for rest.
static const char_t* g_openPath;              ///< Full path given on the command line without an option, or NULL.
static bool_t        g_startsPlaying;         ///< TRUE to enter play mode after opening.
static bool_t        g_showsAbout;            ///< TRUE to open the About box after opening.
static bool_t        g_checksUpdates;         ///< TRUE to run a manual update check after opening.
static bool_t        g_focusDefinition;       ///< TRUE to focus the Definition File section at start.
static int32_t       g_layoutPreset = -1;     ///< Layout preset rebuilt at start, or -1 to keep the saved one.
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
 * \brief Orders the file rows by the table's sort specification.
 *
 * \param[in,out] window   Window and editor state
 * \param[in]     specs    Sort specification, or NULL
 *
 * \retval        None
 */
static void SortFiles(WindowThreadState_t* window, ImGuiTableSortSpecs* specs);

/*************************************************************************************************/
/**
 * \brief Formats a byte count as B, KB, or MB.
 *
 * \param[in]  bytes      Byte count
 * \param[out] text       Text written
 * \param[in]  capacity   Bytes in text
 *
 * \retval     None
 */
static void FormatSize(uint64_t bytes, char_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Formats a FILETIME value as a local date and time.
 *
 * \param[in]  fileTime   FILETIME as one value
 * \param[out] text       Text written
 * \param[in]  capacity   Bytes in text
 *
 * \retval     None
 */
static void FormatTime(uint64_t fileTime, char_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Tells whether a folder file is open in an editor, and whether it has unsaved changes.
 *
 * \param[in] window      Window and editor state
 * \param[in] fileIndex   Folder file
 *
 * \retval    const char_t*   Open, Modified, or empty.
 */
static const char_t* FileState(const WindowThreadState_t* window, uint32_t fileIndex);

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
 * \brief Opens a folder file in the matching editor and records it as recent.
 *
 * \param[in,out] window         Window and editor state
 * \param[in]     name           File name inside the folder
 * \param[in]     isDefinition   TRUE for a Definition File, FALSE for a configuration
 *
 * \retval        None
 */
static void OpenFolderFile(WindowThreadState_t* window, const char_t* name, bool_t isDefinition);

/*************************************************************************************************/
/**
 * \brief Opens a document by full path: the folder becomes the working folder, the file is
 *  classified by content and opened in the matching editor.
 *
 * \param[in,out] window   Window and editor state
 * \param[in]     path     Full path
 *
 * \retval        TRUE     if the file was a Definition File or a configuration.
 * \retval        FALSE    otherwise.
 */
static bool_t OpenPath(WindowThreadState_t* window, const char_t* path);

/*************************************************************************************************/
/**
 * \brief Starts the native file or folder picker on its own thread so the UI keeps running.
 *
 * \param[in,out] window     Window and editor state
 * \param[in]     isFolder   TRUE to pick a folder, FALSE to pick a file
 *
 * \retval        None
 */
static void BeginPick(WindowThreadState_t* window, bool_t isFolder);

/*************************************************************************************************/
/**
 * \brief Picker thread: shows the dialog and hands the result back through the window state.
 *
 * \param[in] parameter   Window and editor state
 *
 * \retval    DWORD       Zero.
 */
static DWORD WINAPI PickThread(LPVOID parameter);

/*************************************************************************************************/
/**
 * \brief Applies a finished picker: opens the file or switches the folder.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void FinishPick(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Opens Explorer with the active document selected, or the folder when none is active.
 *
 * \param[in] window   Window and editor state
 *
 * \retval    None
 */
static void RevealInExplorer(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the start page shown while no document is open.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawStartPage(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the undo history of the active Definition File.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawHistory(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the pop-up that names and saves a prefab from the selection.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawSavePrefabPopup(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Starts an update check on its own thread.
 *
 * \param[in,out] window     Window and editor state
 * \param[in]     isManual   TRUE when the user asked
 *
 * \retval        None
 */
static void BeginUpdateCheck(WindowThreadState_t* window, bool_t isManual);

/*************************************************************************************************/
/**
 * \brief Updater thread: checks, or downloads when asked to.
 *
 * \param[in] parameter   Window and editor state
 *
 * \retval    DWORD       Zero.
 */
static DWORD WINAPI UpdateThread(LPVOID parameter);

/*************************************************************************************************/
/**
 * \brief Draws the update pop-up and reacts to the updater thread's results.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawUpdatePopup(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the About box.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawAboutPopup(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Reads the update source from the registry when the preferences do not name one.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void LoadUpdateSource(WindowThreadState_t* window);

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
 * \brief Draws the animation timeline for the active Definition File.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawTimeline(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Enters or leaves play mode.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void TogglePlay(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Checks everything and writes every binary Definition File, logging the report.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void BuildAll(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Draws the command palette pop-up when open.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void DrawCommandPalette(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Applies the chosen colour scheme and font scale.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void ApplyTheme(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Reads the preferences file beside the executable, when present.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void LoadPreferences(WindowThreadState_t* window);

/*************************************************************************************************/
/**
 * \brief Writes the preferences file when a setting changed.
 *
 * \param[in,out] window   Window and editor state
 *
 * \retval        None
 */
static void SavePreferences(WindowThreadState_t* window);

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
 *  [--focus-definition] [--select widgetName] [--time seconds] [--play].
 */
static void ParseOptions(int argc, char** argv)
{
    int index;

    g_captureFrame = SCREENSHOT_DEFAULT_FRAMES;
    for (index = 1; index < argc; ++index)
    {
        if ((0 == strncmp(argv[index], SCREENSHOT_OPTION, sizeof(SCREENSHOT_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_screenshotPath = argv[index];
        }
        else if ((0 == strncmp(argv[index], OPEN_DEFINITION_OPTION, sizeof(OPEN_DEFINITION_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_openDefinition = argv[index];
        }
        else if ((0 == strncmp(argv[index], OPEN_CONFIGURATION_OPTION, sizeof(OPEN_CONFIGURATION_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_openConfiguration = argv[index];
        }
        else if ((0 == strncmp(argv[index], SELECT_OPTION, sizeof(SELECT_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_selectName = argv[index];
        }
        else if ((0 == strncmp(argv[index], TIME_OPTION, sizeof(TIME_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_startTime = (float32_t)atof(argv[index]);
        }
        else if (0 == strncmp(argv[index], PLAY_OPTION, sizeof(PLAY_OPTION)))
        {
            g_startsPlaying = TRUE;
        }
        else if (0 == strncmp(argv[index], ABOUT_OPTION, sizeof(ABOUT_OPTION)))
        {
            g_showsAbout = TRUE;
        }
        else if (0 == strncmp(argv[index], CHECK_UPDATES_OPTION, sizeof(CHECK_UPDATES_OPTION)))
        {
            g_checksUpdates = TRUE;
        }
        else if ((0 == strncmp(argv[index], FRAMES_OPTION, sizeof(FRAMES_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_captureFrame = (uint32_t)atoi(argv[index]);
        }
        else if (0 == strncmp(argv[index], FOCUS_DEFINITION_OPTION, sizeof(FOCUS_DEFINITION_OPTION)))
        {
            g_focusDefinition = TRUE;
        }
        else if ((0 == strncmp(argv[index], LAYOUT_OPTION, sizeof(LAYOUT_OPTION))) && ((index + 1) < argc))
        {
            index += 1;
            g_layoutPreset = atoi(argv[index]);
        }
        else if ('-' != argv[index][0])
        {
            // A bare path: the shell's open verb and drag-drop onto the executable
            g_openPath = argv[index];
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
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    WindowThreadState_t*      window;
    byte_t                    head[FILE_HEAD_BYTES];
    char_t                    file_path[MAX_FILE_PATH_BYTES];
    const char_t*             text;
    int32_t                   byte_count;
    size_t                    name_length;
    size_t                    index;
    bool_t                    has_known_extension;
    bool_t                    is_definition;
    bool_t                    is_configuration;

    window              = (WindowThreadState_t*)context;
    name_length         = strnlen(fileName, MAX_FILE_NAME_BYTES);
    has_known_extension = FALSE;
    if ((strnlen(XML_EXTENSION, sizeof(XML_EXTENSION)) < name_length) && (0 == _stricmp(fileName + name_length - strnlen(XML_EXTENSION, sizeof(XML_EXTENSION)), XML_EXTENSION)))
    {
        has_known_extension = TRUE;
    }
    if ((strnlen(BINARY_EXTENSION, sizeof(BINARY_EXTENSION)) < name_length) && (0 == _stricmp(fileName + name_length - strnlen(BINARY_EXTENSION, sizeof(BINARY_EXTENSION)), BINARY_EXTENSION)))
    {
        has_known_extension = TRUE;
    }
    if ((FALSE == has_known_extension) || (window->fileCount >= MAX_PAGE_FILES) || (name_length >= MAX_FILE_NAME_BYTES) || ((strnlen(window->folderPath, sizeof(window->folderPath)) + 1 + name_length) >= MAX_FILE_PATH_BYTES))
    {
        return;
    }
    // Prefabs are Definition File fragments kept apart from the documents
    if ((strnlen(PREFAB_EXTENSION, sizeof(PREFAB_EXTENSION)) < name_length) && (0 == _stricmp(fileName + name_length - strnlen(PREFAB_EXTENSION, sizeof(PREFAB_EXTENSION)), PREFAB_EXTENSION)))
    {
        if (window->prefabCount < MAX_PREFABS)
        {
            strncpy_s(window->prefabNames[window->prefabCount], sizeof(window->prefabNames[0]), fileName, _TRUNCATE);
            window->prefabCount += 1;
        }
        return;
    }
    (void)snprintf(file_path, sizeof(file_path), "%s/%s", window->folderPath, fileName);
    byte_count = OS_File_ReadFile(file_path, head, sizeof(head));
    if (0 >= byte_count)
    {
        return;
    }
    is_definition    = Arinc661_DefinitionEditor_IsDefinitionFile(head, (uint64_t)byte_count);
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
        is_configuration = !!(((index + strnlen(CONFIGURATION_ROOT_ELEMENT, sizeof(CONFIGURATION_ROOT_ELEMENT))) < (size_t)byte_count) && (0 == memcmp(&text[index], CONFIGURATION_ROOT_ELEMENT, strnlen(CONFIGURATION_ROOT_ELEMENT, sizeof(CONFIGURATION_ROOT_ELEMENT)))));
    }
    if (is_definition || is_configuration)
    {
        strncpy_s(window->fileNames[window->fileCount], sizeof(window->fileNames[0]), fileName, _TRUNCATE);
        window->fileIsDefinition[window->fileCount] = is_definition;
        window->fileSizes[window->fileCount]        = 0;
        window->fileTimes[window->fileCount]        = 0;
        if (::GetFileAttributesExA(file_path, GetFileExInfoStandard, &attributes))
        {
            window->fileSizes[window->fileCount] = (((uint64_t)attributes.nFileSizeHigh << 32) | attributes.nFileSizeLow);
            window->fileTimes[window->fileCount] = (((uint64_t)attributes.ftLastWriteTime.dwHighDateTime << 32) | attributes.ftLastWriteTime.dwLowDateTime);
        }
        window->fileOrder[window->fileCount] = window->fileCount;
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
    window->prefabCount  = 0;
    window->lastScanTime = ImGui::GetTime();
    Arinc661_Configuration_ClearDefinitionFiles(window->configurationEditor);
    window->context->definitionFileCount = 0;
    Arinc661_Configuration_SetFolderPath(window->configurationEditor, window->folderPath);
    Arinc661_DefinitionEditor_SetFolderPath(window->definitionEditor, window->folderPath);
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
    const char_t* play_label;
    bool_t        has_active;
    bool_t        is_dirty;
    bool_t        has_free;
    bool_t        can_undo;
    bool_t        can_redo;
    bool_t        has_selection;

    if (window->isDefinitionActive)
    {
        has_active = Arinc661_DefinitionEditor_HasActiveDocument(window->definitionEditor, &is_dirty, &has_free);
        Arinc661_DefinitionEditor_History(window->definitionEditor, &can_undo, &can_redo);
        has_selection = (NULL != Arinc661_DefinitionEditor_SelectedType(window->definitionEditor));
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
            Arinc661_DefinitionEditor_CreateDocument(window->definitionEditor);
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
        Arinc661_DefinitionEditor_SaveAllDocuments(window->definitionEditor);
    }
    Arinc661_Icons_Gap();
    if (Arinc661_Icons_Button(ARINC661_ICON_UNDO, UNDO_LABEL, UNDO_SHORTCUT_LABEL, can_undo, FALSE))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_DefinitionEditor_Undo(window->definitionEditor);
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
            Arinc661_DefinitionEditor_Redo(window->definitionEditor);
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
            Arinc661_DefinitionEditor_AddLayer(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_ADD_WIDGET, ADD_WIDGET_LABEL, NULL, has_active, FALSE))
        {
            Arinc661_DefinitionEditor_AddWidget(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_ADD_EXTENSION, ADD_EXTENSION_LABEL, NULL, has_selection, FALSE))
        {
            Arinc661_DefinitionEditor_AddExtension(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_DUPLICATE, DUPLICATE_LABEL, DUPLICATE_SHORTCUT_LABEL, has_selection, FALSE))
        {
            Arinc661_DefinitionEditor_DuplicateSelection(window->definitionEditor);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_DELETE, DELETE_LABEL, DELETE_SHORTCUT_LABEL, has_active, FALSE))
        {
            Arinc661_DefinitionEditor_DeleteSelection(window->definitionEditor);
        }
        Arinc661_Icons_Gap();
        if (Arinc661_Icons_Button(ARINC661_ICON_GRID, GRID_LABEL, GRID_SHORTCUT_LABEL, TRUE, Arinc661_DefinitionEditor_Grid(window->definitionEditor, FALSE)))
        {
            (void)Arinc661_DefinitionEditor_Grid(window->definitionEditor, TRUE);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_SNAP, SNAP_LABEL, SNAP_SHORTCUT_LABEL, TRUE, Arinc661_DefinitionEditor_Snap(window->definitionEditor, FALSE)))
        {
            (void)Arinc661_DefinitionEditor_Snap(window->definitionEditor, TRUE);
        }
        if (Arinc661_Icons_Button(ARINC661_ICON_FIT, FIT_LABEL, FIT_SHORTCUT_LABEL, has_active, FALSE))
        {
            Arinc661_DefinitionEditor_FitView(window->definitionEditor);
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
    play_label = PLAY_LABEL;
    if (window->definitionEditor->isPlaying)
    {
        play_label = STOP_LABEL;
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_PLAY, play_label, PLAY_SHORTCUT_LABEL, TRUE, window->definitionEditor->isPlaying))
    {
        TogglePlay(window);
    }
    if (Arinc661_Icons_Button(ARINC661_ICON_BUILD, BUILD_LABEL, BUILD_SHORTCUT_LABEL, TRUE, FALSE))
    {
        BuildAll(window);
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
        topic = Arinc661_DefinitionEditor_SelectedType(window->definitionEditor);
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
    if (0 == strncmp(topic, HELP_GETTING_STARTED, sizeof(HELP_GETTING_STARTED)))
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
    const char_t*        description;
    const char_t*        categories;
    const char_t*        restriction;
    const char_t*        type_name;
    const char_t*        change;
    const char_t*        parameter_description;
    Arinc661_Parameter_t parameter;
    char_t               upper_filter[MAX_HELP_FILTER_BYTES];
    char_t               upper_name[ARINC661_MAX_NAME_BYTES];
    uint32_t             index;
    uint32_t             character;
    uint32_t             parameter_count;
    uint32_t             pass;
    bool_t               is_extension;
    bool_t               is_match;
    bool                 is_open;

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
                if (ImGui::Selectable(HELP_GETTING_STARTED, 0 == strncmp(window->helpTopic, HELP_GETTING_STARTED, sizeof(HELP_GETTING_STARTED))))
                {
                    strncpy_s(window->helpTopic, sizeof(window->helpTopic), HELP_GETTING_STARTED, _TRUNCATE);
                }
                if (ImGui::Selectable(SHORTCUTS_LABEL, 0 == strncmp(window->helpTopic, SHORTCUTS_LABEL, sizeof(SHORTCUTS_LABEL))))
                {
                    strncpy_s(window->helpTopic, sizeof(window->helpTopic), SHORTCUTS_LABEL, _TRUNCATE);
                }
                for (index = 0; index < COUNT_OF(c_faceTopics); ++index)
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
                        if (ImGui::Selectable(c_faceTopics[index].name, 0 == strncmp(window->helpTopic, c_faceTopics[index].name, sizeof(window->helpTopic))))
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
                    for (index = 0; index < Arinc661_Definition_GetTypeCount(is_extension); ++index)
                    {
                        type_name = Arinc661_Definition_GetTypeName(is_extension, index);
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
                            if (ImGui::Selectable(type_name, 0 == strncmp(window->helpTopic, type_name, sizeof(window->helpTopic))))
                            {
                                strncpy_s(window->helpTopic, sizeof(window->helpTopic), type_name, _TRUNCATE);
                            }
                            if (0 == strncmp(window->helpTopic, type_name, sizeof(window->helpTopic)) && ImGui::IsWindowAppearing())
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
                if (('\0' == window->helpTopic[0]) || (0 == strncmp(window->helpTopic, HELP_GETTING_STARTED, sizeof(HELP_GETTING_STARTED))))
                {
                    ImGui::SeparatorText(HELP_GETTING_STARTED);
                    ImGui::TextUnformatted("1. Configuration: File > New Configuration. Add a Screen (toolbar), then a Layout, then a Window; set the window's pixelArea and scalingFactor. Add a User Application, pick its Window and its DF Path from the Definition Files in the folder.");
                    ImGui::TextUnformatted("2. Definition File: File > New Definition File. Add a Layer, then right-click the scene to add widgets where you click, or use the toolbar. Containers (Panel, BasicContainer, ...) accept children: right-click a container or drag a node onto it in the hierarchy.");
                    ImGui::TextUnformatted("3. Scene: drag widgets to move them, drag the corner handles to resize, arrow keys nudge, Delete removes, mouse wheel zooms, middle-drag or Space-drag pans, F fits, G toggles the grid, S toggles millimetre snapping.");
                    ImGui::TextUnformatted("4. Inspector: every parameter row shows its specification description on hover; the About section explains the selected widget. Red values would not fit the binary form.");
                    ImGui::TextUnformatted("5. Link both: set the Definition File's ApplicationId (root node) to the user application's applicationId. The user application inspector checks it and previews the Definition File's layers inside the window; the Definition File root lists who references it.");
                    ImGui::TextUnformatted("6. Save with Ctrl+S; a .bin name saves the binary form of ARINC 661 Part 1-7, any other name the XML form. Undo with Ctrl+Z, redo with Ctrl+Y. F1 opens this help on the selection; Shift+F1 opens the HTML reference in the browser.");
                }
                else if (0 == strncmp(window->helpTopic, SHORTCUTS_LABEL, sizeof(SHORTCUTS_LABEL)))
                {
                    ImGui::SeparatorText(SHORTCUTS_LABEL);
                    ImGui::TextUnformatted("Ctrl+N new configuration   Ctrl+Shift+N new Definition File   Ctrl+S save   Ctrl+Shift+S save all   Ctrl+W close");
                    ImGui::TextUnformatted("Ctrl+Z undo   Ctrl+Y redo   Ctrl+C copy   Ctrl+V paste   Ctrl+D duplicate   Del delete   F2 rename");
                    ImGui::TextUnformatted("Scene: drag move, corner handles resize, arrows nudge (Shift x10), Ctrl-click multi-select, drag on empty space box-select");
                    ImGui::TextUnformatted("Scene: wheel zoom, middle or Space drag pan, F fit, Z zoom to selection, G grid, S snap, right-click add");
                    ImGui::TextUnformatted("Hierarchy: double-click rename, drag onto a container to re-parent, right-click for the context menu, eye hides, lock freezes");
                    ImGui::TextUnformatted("Help: F1 Windows Help on the selection, Shift+F1 this reference");
                }
                else
                {
                    is_match = FALSE;
                    for (index = 0; index < COUNT_OF(c_faceTopics); ++index)
                    {
                        if (0 == strncmp(window->helpTopic, c_faceTopics[index].name, sizeof(window->helpTopic)))
                        {
                            ImGui::SeparatorText(c_faceTopics[index].name);
                            ImGui::TextUnformatted(c_faceTopics[index].description);
                            is_match = TRUE;
                        }
                    }
                    if ((FALSE == is_match) && Arinc661_Help_GetType(window->helpTopic, &description, &categories, &restriction))
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
                        is_extension = !!(Arinc661_Definition_FindType(window->helpTopic, TRUE, &parameter_count));
                        if (Arinc661_Definition_FindType(window->helpTopic, is_extension, &parameter_count))
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
                                    (void)Arinc661_Definition_GetParameter(window->helpTopic, is_extension, index, &parameter);
                                    parameter_description = Arinc661_Help_GetParameter(window->helpTopic, parameter.name, &change);
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
    bool_t        has_active;
    bool_t        is_dirty;
    bool_t        has_free_configuration;
    bool_t        has_free_definition;
    bool_t        has_dirty_configuration;
    bool_t        has_dirty_definition;
    bool_t        has_active_configuration;
    bool_t        has_active_definition;
    bool_t        can_undo;
    bool_t        can_redo;
    bool_t        has_selection;
    uint32_t      panel_index;
    uint32_t      preset_index;
    const char_t* play_label;

    has_active_configuration = Arinc661_Configuration_HasActiveDocument(window->configurationEditor, &has_dirty_configuration, &has_free_configuration);
    has_active_definition    = Arinc661_DefinitionEditor_HasActiveDocument(window->definitionEditor, &has_dirty_definition, &has_free_definition);
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
                Arinc661_DefinitionEditor_CreateDocument(window->definitionEditor);
                window->isDefinitionActive = TRUE;
            }
            if (ImGui::MenuItem(NEW_APPLICATION_LABEL, NULL, false, has_active_configuration && has_free_definition))
            {
                (void)Arinc661_Context_NewApplication(window->context);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(OPEN_FILE_LABEL, OPEN_FILE_SHORTCUT))
            {
                BeginPick(window, FALSE);
            }
            if (ImGui::MenuItem(OPEN_FOLDER_LABEL))
            {
                BeginPick(window, TRUE);
            }
            if (ImGui::MenuItem(REVEAL_LABEL))
            {
                RevealInExplorer(window);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(SAVE_LABEL, SAVE_SHORTCUT_LABEL, false, has_active))
            {
                SaveActive(window);
            }
            if (ImGui::MenuItem(BUILD_LABEL, BUILD_SHORTCUT_LABEL))
            {
                BuildAll(window);
            }
            if (ImGui::BeginMenu(RECENT_MENU_LABEL))
            {
                if (0 == window->recentCount)
                {
                    ImGui::MenuItem(RECENT_EMPTY_LABEL, NULL, false, false);
                }
                for (panel_index = 0; panel_index < window->recentCount; ++panel_index)
                {
                    ImGui::PushID((int)panel_index);
                    if (ImGui::MenuItem(window->recentNames[panel_index]))
                    {
                        (void)OpenPath(window, window->recentNames[panel_index]);
                    }
                    ImGui::PopID();
                }
                if (0 < window->recentCount)
                {
                    ImGui::Separator();
                    if (ImGui::MenuItem(RECENT_CLEAR_LABEL))
                    {
                        window->recentCount      = 0;
                        window->preferencesDirty = TRUE;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(SAVE_ALL_LABEL, SAVE_ALL_SHORTCUT_LABEL, false, is_dirty))
            {
                Arinc661_Configuration_SaveAllDocuments(window->configurationEditor);
                Arinc661_DefinitionEditor_SaveAllDocuments(window->definitionEditor);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(CLOSE_LABEL, CLOSE_SHORTCUT_LABEL, false, has_active))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_DefinitionEditor_CloseActiveDocument(window->definitionEditor);
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
            Arinc661_DefinitionEditor_History(window->definitionEditor, &can_undo, &can_redo);
            has_selection = (NULL != Arinc661_DefinitionEditor_SelectedType(window->definitionEditor));
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
                    Arinc661_DefinitionEditor_Undo(window->definitionEditor);
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
                    Arinc661_DefinitionEditor_Redo(window->definitionEditor);
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
                    Arinc661_DefinitionEditor_AddLayer(window->definitionEditor);
                }
                if (ImGui::MenuItem(ADD_WIDGET_LABEL, NULL, false, has_active))
                {
                    Arinc661_DefinitionEditor_AddWidget(window->definitionEditor);
                }
                if (ImGui::MenuItem(ADD_EXTENSION_LABEL, NULL, false, has_selection))
                {
                    Arinc661_DefinitionEditor_AddExtension(window->definitionEditor);
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
            if (ImGui::MenuItem(COPY_LABEL, COPY_SHORTCUT_LABEL, false, window->isDefinitionActive && has_selection))
            {
                (void)Arinc661_DefinitionEditor_Copy(window->definitionEditor);
            }
            if (ImGui::MenuItem(PASTE_LABEL, PASTE_SHORTCUT_LABEL, false, window->isDefinitionActive && (0 < window->definitionEditor->clipboardWidgetCount)))
            {
                (void)Arinc661_DefinitionEditor_Paste(window->definitionEditor);
            }
            if (ImGui::MenuItem(RENAME_LABEL, RENAME_SHORTCUT_LABEL, false, window->isDefinitionActive && has_selection))
            {
                Arinc661_DefinitionEditor_BeginRename(window->definitionEditor);
            }
            if (ImGui::MenuItem(DUPLICATE_LABEL, DUPLICATE_SHORTCUT_LABEL, false, has_selection))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_DefinitionEditor_DuplicateSelection(window->definitionEditor);
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
                    Arinc661_DefinitionEditor_DeleteSelection(window->definitionEditor);
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
            play_label = PLAY_LABEL;
            if (window->definitionEditor->isPlaying)
            {
                play_label = STOP_LABEL;
            }
            if (ImGui::MenuItem(play_label, PLAY_SHORTCUT_LABEL))
            {
                TogglePlay(window);
            }
            if (ImGui::MenuItem(SAVE_PREFAB_LABEL, NULL, false, window->isDefinitionActive && has_selection))
            {
                window->opensPrefabPopup = TRUE;
            }
            if (ImGui::MenuItem(COMMAND_PALETTE_LABEL, COMMAND_PALETTE_SHORTCUT))
            {
                window->isPaletteOpen   = TRUE;
                window->commandFilter[0] = '\0';
            }
            ImGui::Separator();
            if (ImGui::MenuItem(GRID_LABEL, GRID_SHORTCUT_LABEL, Arinc661_DefinitionEditor_Grid(window->definitionEditor, FALSE)))
            {
                (void)Arinc661_DefinitionEditor_Grid(window->definitionEditor, TRUE);
            }
            if (ImGui::MenuItem(SNAP_LABEL, SNAP_SHORTCUT_LABEL, Arinc661_DefinitionEditor_Snap(window->definitionEditor, FALSE)))
            {
                (void)Arinc661_DefinitionEditor_Snap(window->definitionEditor, TRUE);
            }
            if (ImGui::MenuItem(FIT_LABEL, FIT_SHORTCUT_LABEL))
            {
                Arinc661_DefinitionEditor_FitView(window->definitionEditor);
            }
            ImGui::Separator();
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::SliderFloat(UI_SCALE_LABEL, &window->uiScale, UI_SCALE_MIN, UI_SCALE_MAX, "%.2f"))
            {
                ImGui::GetStyle().FontScaleMain = window->uiScale;
                window->preferencesDirty        = TRUE;
            }
            if (ImGui::MenuItem(THEME_DARK_LABEL, NULL, FALSE == window->isLightTheme))
            {
                window->isLightTheme     = FALSE;
                window->preferencesDirty = TRUE;
                ApplyTheme(window);
            }
            if (ImGui::MenuItem(THEME_LIGHT_LABEL, NULL, FALSE != window->isLightTheme))
            {
                window->isLightTheme     = TRUE;
                window->preferencesDirty = TRUE;
                ApplyTheme(window);
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
            if (ImGui::BeginMenu(LAYOUT_MENU_LABEL))
            {
                for (preset_index = 0; preset_index < COUNT_OF(c_layoutNames); ++preset_index)
                {
                    if (ImGui::MenuItem(c_layoutNames[preset_index], NULL, (LayoutPreset_t)preset_index == window->layoutPreset))
                    {
                        window->layoutPreset = (LayoutPreset_t)preset_index;
                        window->resetsLayout = TRUE;
                    }
                }
                ImGui::EndMenu();
            }
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
            if (ImGui::MenuItem(SHORTCUTS_LABEL))
            {
                ShowHelp(window, SHORTCUTS_LABEL);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(CHECK_UPDATES_LABEL))
            {
                BeginUpdateCheck(window, TRUE);
                window->opensUpdatePopup = (bool_t)(UPDATE_STATE_IDLE != window->updateState.load());
            }
            if (ImGui::MenuItem(ABOUT_LABEL))
            {
                window->opensAboutPopup = TRUE;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::Shortcut(UNDO_CHORD, SHORTCUT_FLAGS))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_DefinitionEditor_Undo(window->definitionEditor);
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
            Arinc661_DefinitionEditor_Redo(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_Redo(window->configurationEditor);
        }
    }
    if (window->isDefinitionActive && ImGui::Shortcut(COPY_CHORD, SHORTCUT_FLAGS))
    {
        (void)Arinc661_DefinitionEditor_Copy(window->definitionEditor);
    }
    if (window->isDefinitionActive && ImGui::Shortcut(PASTE_CHORD, SHORTCUT_FLAGS))
    {
        (void)Arinc661_DefinitionEditor_Paste(window->definitionEditor);
    }
    if (window->isDefinitionActive && ImGui::Shortcut(RENAME_CHORD, SHORTCUT_FLAGS))
    {
        Arinc661_DefinitionEditor_BeginRename(window->definitionEditor);
    }
    if (ImGui::Shortcut(OPEN_FILE_CHORD, SHORTCUT_FLAGS))
    {
        BeginPick(window, FALSE);
    }
    if (ImGui::Shortcut(PLAY_CHORD, SHORTCUT_FLAGS))
    {
        TogglePlay(window);
    }
    if (ImGui::Shortcut(BUILD_CHORD, SHORTCUT_FLAGS))
    {
        BuildAll(window);
    }
    if (ImGui::Shortcut(COMMAND_PALETTE_CHORD, SHORTCUT_FLAGS))
    {
        window->isPaletteOpen    = TRUE;
        window->commandFilter[0] = '\0';
    }
    if (ImGui::Shortcut(DUPLICATE_CHORD, SHORTCUT_FLAGS))
    {
        if (window->isDefinitionActive)
        {
            Arinc661_DefinitionEditor_DuplicateSelection(window->definitionEditor);
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
        Arinc661_DefinitionEditor_CreateDocument(window->definitionEditor);
        window->isDefinitionActive = TRUE;
    }
    if (ImGui::Shortcut(SAVE_CHORD, SHORTCUT_FLAGS) && has_active)
    {
        SaveActive(window);
    }
    if (ImGui::Shortcut(SAVE_ALL_CHORD, SHORTCUT_FLAGS) && is_dirty)
    {
        Arinc661_Configuration_SaveAllDocuments(window->configurationEditor);
        Arinc661_DefinitionEditor_SaveAllDocuments(window->definitionEditor);
    }
    if (ImGui::Shortcut(CLOSE_CHORD, SHORTCUT_FLAGS) && has_active)
    {
        if (window->isDefinitionActive)
        {
            Arinc661_DefinitionEditor_CloseActiveDocument(window->definitionEditor);
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
 *  Insertion sort on the order array; the lists are short. Only the first sort column counts.
 */
static void SortFiles(WindowThreadState_t* window, ImGuiTableSortSpecs* specs)
{
    uint32_t row;
    uint32_t probe;
    uint32_t held;
    int32_t  comparison;
    bool_t   is_descending;
    ImGuiID  column;

    column        = FILE_COLUMN_NAME_ID;
    is_descending = FALSE;
    if ((NULL != specs) && (0 < specs->SpecsCount))
    {
        column        = specs->Specs[0].ColumnUserID;
        is_descending = (ImGuiSortDirection_Descending == specs->Specs[0].SortDirection);
    }
    for (row = 1; row < window->fileCount; ++row)
    {
        held  = window->fileOrder[row];
        probe = row;
        while (0 < probe)
        {
            switch (column)
            {
            case FILE_COLUMN_KIND_ID:
                comparison = ((int32_t)window->fileIsDefinition[held] - (int32_t)window->fileIsDefinition[window->fileOrder[probe - 1]]);
                break;
            case FILE_COLUMN_SIZE_ID:
                comparison = 0;
                if (window->fileSizes[held] < window->fileSizes[window->fileOrder[probe - 1]])
                {
                    comparison = -1;
                }
                else if (window->fileSizes[held] > window->fileSizes[window->fileOrder[probe - 1]])
                {
                    comparison = 1;
                }
                break;
            case FILE_COLUMN_MODIFIED_ID:
                comparison = 0;
                if (window->fileTimes[held] < window->fileTimes[window->fileOrder[probe - 1]])
                {
                    comparison = -1;
                }
                else if (window->fileTimes[held] > window->fileTimes[window->fileOrder[probe - 1]])
                {
                    comparison = 1;
                }
                break;
            case FILE_COLUMN_STATE_ID:
                comparison = strncmp(FileState(window, held), FileState(window, window->fileOrder[probe - 1]), sizeof(FILE_STATE_MODIFIED));
                break;
            default:
                comparison = _stricmp(window->fileNames[held], window->fileNames[window->fileOrder[probe - 1]]);
                break;
            }
            if (0 == comparison)
            {
                comparison = _stricmp(window->fileNames[held], window->fileNames[window->fileOrder[probe - 1]]);
            }
            if (is_descending)
            {
                comparison = -comparison;
            }
            if (0 <= comparison)
            {
                break;
            }
            window->fileOrder[probe] = window->fileOrder[probe - 1];
            probe -= 1;
        }
        window->fileOrder[probe] = held;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void FormatSize(uint64_t bytes, char_t* text, uint32_t capacity)
{
    if (bytes < 1024)
    {
        (void)snprintf(text, capacity, SIZE_BYTES_FORMAT, (unsigned long long)bytes);
    }
    else if (bytes < (1024 * 1024))
    {
        (void)snprintf(text, capacity, SIZE_KILOBYTES_FORMAT, (float64_t)bytes / 1024.0);
    }
    else
    {
        (void)snprintf(text, capacity, SIZE_MEGABYTES_FORMAT, (float64_t)bytes / (1024.0 * 1024.0));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void FormatTime(uint64_t fileTime, char_t* text, uint32_t capacity)
{
    FILETIME   file_time;
    FILETIME   local_time;
    SYSTEMTIME system_time;

    file_time.dwLowDateTime  = (DWORD)(fileTime & 0xFFFFFFFFu);
    file_time.dwHighDateTime = (DWORD)(fileTime >> 32);
    text[0] = '\0';
    if (::FileTimeToLocalFileTime(&file_time, &local_time) && ::FileTimeToSystemTime(&local_time, &system_time))
    {
        (void)snprintf(text, capacity, TIME_FORMAT, (unsigned)system_time.wYear, (unsigned)system_time.wMonth, (unsigned)system_time.wDay, (unsigned)system_time.wHour, (unsigned)system_time.wMinute);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Matches the name against the open documents of the editor of its kind.
 */
static const char_t* FileState(const WindowThreadState_t* window, uint32_t fileIndex)
{
    uint32_t slot;

    if (window->fileIsDefinition[fileIndex])
    {
        for (slot = 0; slot < ARINC661_DEFINITION_EDITOR_MAX_OPEN_DOCUMENTS; ++slot)
        {
            if (window->definitionEditor->documents[slot].isLoaded && (0 == _stricmp(window->definitionEditor->documents[slot].documentName, window->fileNames[fileIndex])))
            {
                if (window->definitionEditor->documents[slot].isDirty)
                {
                    return FILE_STATE_MODIFIED;
                }
                return FILE_STATE_OPEN;
            }
        }
    }
    else
    {
        for (slot = 0; slot < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++slot)
        {
            if (window->configurationEditor->documents[slot].isLoaded && (0 == _stricmp(window->configurationEditor->documents[slot].documentName, window->fileNames[fileIndex])))
            {
                if (window->configurationEditor->documents[slot].isDirty)
                {
                    return FILE_STATE_MODIFIED;
                }
                return FILE_STATE_OPEN;
            }
        }
    }

    return FILE_STATE_CLOSED;
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
        Arinc661_DefinitionEditor_SaveActiveDocument(window->definitionEditor);
        if (0 < errors)
        {
            (void)snprintf(note, sizeof(note), SAVED_WITH_ERRORS_FORMAT, (unsigned)errors);
            Arinc661_DefinitionEditor_SetStatusNote(window->definitionEditor, note);
        }
        else
        {
            Arinc661_DefinitionEditor_SetStatusNote(window->definitionEditor, NULL);
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
    char_t    label[MAX_FILE_PATH_BYTES + 32];
    float32_t buttons_width;
    uint32_t  file_index;
    uint32_t  row;
    bool_t    should_scan;

    should_scan = FALSE;
    if (ImGui::CollapsingHeader(FILE_CONTROLS_HEADER, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(FOLDER_PATH_LABEL);
        ImGui::SameLine();
        buttons_width = ImGui::CalcTextSize(BROWSE_FOLDER_LABEL).x + ImGui::CalcTextSize(BROWSE_FILE_LABEL).x + ImGui::CalcTextSize(REFRESH_LABEL).x + (ImGui::GetStyle().FramePadding.x * 6.0f) + (ImGui::GetStyle().ItemSpacing.x * 3.0f);
        ImGui::SetNextItemWidth(-buttons_width);
        if (ImGui::InputText(FOLDER_INPUT_ID, window->folderPath, sizeof(window->folderPath)))
        {
            should_scan = TRUE;
        }
        ImGui::SameLine();
        if (ImGui::Button(BROWSE_FOLDER_LABEL))
        {
            BeginPick(window, TRUE);
        }
        ImGui::SameLine();
        if (ImGui::Button(BROWSE_FILE_LABEL))
        {
            BeginPick(window, FALSE);
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
        if (FALSE == window->isFolderReadable)
        {
            ImGui::TextDisabled("%s", FOLDER_UNREADABLE_LABEL);
        }
        else if (0 == window->fileCount)
        {
            ImGui::TextDisabled("%s", NO_FILES_LABEL);
        }
        else if (ImGui::BeginTable(FILE_LIST_ID, FILE_COLUMN_COUNT, FILE_TABLE_FLAGS, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * (FILE_LIST_ROWS + 1.5f))))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(FILE_COLUMN_NAME, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 0.0f, FILE_COLUMN_NAME_ID);
            ImGui::TableSetupColumn(FILE_COLUMN_KIND, ImGuiTableColumnFlags_WidthFixed, 0.0f, FILE_COLUMN_KIND_ID);
            ImGui::TableSetupColumn(FILE_COLUMN_SIZE, ImGuiTableColumnFlags_WidthFixed, 0.0f, FILE_COLUMN_SIZE_ID);
            ImGui::TableSetupColumn(FILE_COLUMN_MODIFIED, ImGuiTableColumnFlags_WidthFixed, 0.0f, FILE_COLUMN_MODIFIED_ID);
            ImGui::TableSetupColumn(FILE_COLUMN_STATE, ImGuiTableColumnFlags_WidthFixed, 0.0f, FILE_COLUMN_STATE_ID);
            ImGui::TableHeadersRow();
            SortFiles(window, ImGui::TableGetSortSpecs());
            for (row = 0; row < window->fileCount; ++row)
            {
                file_index = window->fileOrder[row];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID((int)file_index);
                if (ImGui::Selectable(window->fileNames[file_index], false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
                {
                    OpenFolderFile(window, window->fileNames[file_index], window->fileIsDefinition[file_index]);
                }
                if (ImGui::BeginPopupContextItem(FILE_ROW_MENU_ID))
                {
                    if (ImGui::MenuItem(FILE_OPEN_ROW_LABEL))
                    {
                        OpenFolderFile(window, window->fileNames[file_index], window->fileIsDefinition[file_index]);
                    }
                    if (ImGui::MenuItem(REVEAL_LABEL))
                    {
                        (void)snprintf(label, sizeof(label), "/select,\"%s\\%s\"", window->folderPath, window->fileNames[file_index]);
                        (void)::ShellExecuteA(window->windowHandle, "open", "explorer.exe", label, NULL, SW_SHOWNORMAL);
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
                ImGui::TableSetColumnIndex(1);
                if (window->fileIsDefinition[file_index])
                {
                    ImGui::TextUnformatted(FILE_KIND_DEFINITION);
                }
                else
                {
                    ImGui::TextUnformatted(FILE_KIND_CONFIGURATION);
                }
                ImGui::TableSetColumnIndex(2);
                FormatSize(window->fileSizes[file_index], label, sizeof(label));
                ImGui::TextUnformatted(label);
                ImGui::TableSetColumnIndex(3);
                FormatTime(window->fileTimes[file_index], label, sizeof(label));
                ImGui::TextUnformatted(label);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(FileState(window, file_index));
            }
            ImGui::EndTable();
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
            Arinc661_DefinitionEditor_DrawDocumentTabs(window->definitionEditor);
            Arinc661_DefinitionEditor_DrawHierarchy(window->definitionEditor);
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
    const FACEOE_Gfx_DrawList_t* stats_list;
    bool_t                       is_dirty;
    bool_t                       has_free;
    const ImGuiViewport*         viewport;
    ImGuiID                      dockspace_id;
    uint32_t                     panel_index;
    bool_t                       is_definition_focused;

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
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, -((2.0f * ImGui::GetTextLineHeightWithSpacing()) + (2.0f * ImGui::GetStyle().ItemSpacing.y))), ImGuiDockNodeFlags_PassthruCentralNode);
        DrawStatusBar(window);
        // On the second frame, after a saved layout has placed every panel, bring the working tabs to the front
        if (4 == g_frameCount)
        {
            ImGui::SetWindowFocus(PANEL_FILES);
            ImGui::SetWindowFocus(PANEL_INSPECTOR);
        }
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
                    if (window->definitionEditor->isPlaying)
                    {
                        ImGui::TextColored(PLAY_BANNER_COLOR, "%s", PLAY_BANNER_LABEL);
                    }
                    if ((FALSE == Arinc661_Configuration_HasActiveDocument(window->configurationEditor, &is_dirty, &has_free)) && (FALSE == Arinc661_DefinitionEditor_HasActiveDocument(window->definitionEditor, &is_dirty, &has_free)))
                    {
                        DrawStartPage(window);
                        break;
                    }
                    // Stats line above the canvas: scene size and frame time, from whichever draw list the view replays
                    stats_list = &window->configurationEditor->drawList;
                    if (window->isDefinitionActive && (FALSE == window->definitionEditor->isPlaying))
                    {
                        stats_list = &window->definitionEditor->drawList;
                    }
                    ImGui::TextDisabled(STATS_FORMAT, (unsigned)Arinc661_DefinitionEditor_WidgetCount(window->definitionEditor), (unsigned)stats_list->commandCount, (unsigned)stats_list->pointCount, (float64_t)(ImGui::GetIO().DeltaTime * 1000.0f));
                    if (window->isDefinitionActive && (FALSE == window->definitionEditor->isPlaying))
                    {
                        Arinc661_DefinitionEditor_DrawScene(window->definitionEditor);
                    }
                    else
                    {
                        Arinc661_Configuration_DrawScene(window->configurationEditor);
                    }
                    break;
                case PANEL_INDEX_INSPECTOR:
                    if (window->isDefinitionActive)
                    {
                        Arinc661_DefinitionEditor_DrawInspector(window->definitionEditor);
                    }
                    else
                    {
                        Arinc661_Configuration_DrawInspector(window->configurationEditor);
                    }
                    break;
                case PANEL_INDEX_LOG:       DrawLog(window); break;
                case PANEL_INDEX_TIMELINE:  DrawTimeline(window); break;
                case PANEL_INDEX_HISTORY:   DrawHistory(window); break;
                default:                    break;
            }
        }
        ImGui::End();
    }
    if ('\0' != window->pendingOpenPath[0])
    {
        (void)OpenPath(window, window->pendingOpenPath);
        window->pendingOpenPath[0] = '\0';
    }
    FinishPick(window);
    DrawHelpWindow(window);
    DrawCommandPalette(window);
    DrawSavePrefabPopup(window);
    if (g_showsAbout && (6 == g_frameCount))
    {
        // Screenshot automation: open the About box once the layout has settled
        window->opensAboutPopup = TRUE;
    }
    if (g_checksUpdates && (6 == g_frameCount))
    {
        BeginUpdateCheck(window, TRUE);
        window->opensUpdatePopup = (bool_t)(UPDATE_STATE_IDLE != window->updateState.load());
    }
    DrawUpdatePopup(window);
    DrawAboutPopup(window);
    // One silent check a day
    if ((UPDATE_STATE_IDLE == window->updateState.load()) && ('\0' != window->updateUrl[0]) && (((float64_t)time(NULL) - window->updateCheckedAt) > UPDATE_CHECK_INTERVAL_SECONDS))
    {
        window->updateCheckedAt = (float64_t)time(NULL);
        BeginUpdateCheck(window, FALSE);
    }
    (void)Arinc661_Context_FollowRenames(window->context);
    Arinc661_Configuration_EndFrame(window->configurationEditor);
    Arinc661_DefinitionEditor_EndFrame(window->definitionEditor);
    Arinc661_Configuration_DrawDialogs(window->configurationEditor);
    Arinc661_DefinitionEditor_DrawDialogs(window->definitionEditor);
    SavePreferences(window);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Default: left column Hierarchy over Palette, right column Inspector, bottom strip of tool
 *  tabs, Scene in the centre. Tall: no bottom strip; the tool tabs stack under the Inspector so
 *  the Scene runs full height. Wide: no right column; Inspector and the tools share the bottom
 *  strip so the Scene runs full width. Every panel is reopened.
 */
static void BuildDefaultLayout(WindowThreadState_t* window, ImGuiID dockspaceId)
{
    ImGuiID  dock_main;
    ImGuiID  dock_left;
    ImGuiID  dock_left_bottom;
    ImGuiID  dock_right;
    ImGuiID  dock_tools;
    uint32_t panel_index;

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);
    dock_main        = dockspaceId;
    dock_left        = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, LAYOUT_LEFT_FRACTION, NULL, &dock_main);
    dock_left_bottom = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, LAYOUT_PALETTE_FRACTION, NULL, &dock_left);
    if (LAYOUT_PRESET_TALL == window->layoutPreset)
    {
        dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, LAYOUT_RIGHT_FRACTION, NULL, &dock_main);
        dock_tools = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, LAYOUT_TALL_SPLIT_FRACTION, NULL, &dock_right);
    }
    else if (LAYOUT_PRESET_WIDE == window->layoutPreset)
    {
        dock_tools = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, LAYOUT_BOTTOM_FRACTION, NULL, &dock_main);
        dock_right = dock_tools;
    }
    else
    {
        dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, LAYOUT_RIGHT_FRACTION, NULL, &dock_main);
        dock_tools = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, LAYOUT_BOTTOM_FRACTION, NULL, &dock_main);
    }
    ImGui::DockBuilderDockWindow(PANEL_HIERARCHY, dock_left);
    ImGui::DockBuilderDockWindow(PANEL_PALETTE, dock_left_bottom);
    // Tabs appear in this order; the last docked tab is the one selected, so Files and Inspector go last
    ImGui::DockBuilderDockWindow(PANEL_HISTORY, dock_right);
    ImGui::DockBuilderDockWindow(PANEL_TIMELINE, dock_tools);
    ImGui::DockBuilderDockWindow(PANEL_LOG, dock_tools);
    ImGui::DockBuilderDockWindow(PANEL_PROBLEMS, dock_tools);
    ImGui::DockBuilderDockWindow(PANEL_FILES, dock_tools);
    ImGui::DockBuilderDockWindow(PANEL_INSPECTOR, dock_right);
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
    ImGui::SetItemTooltip("%s", PALETTE_HINT_LABEL);
    if (0 < window->prefabCount)
    {
        ImGui::SeparatorText(PREFAB_SECTION_LABEL);
        for (character = 0; character < window->prefabCount; ++character)
        {
            (void)snprintf(upper_filter, sizeof(upper_filter), "%.*s", (int)(strnlen(window->prefabNames[character], sizeof(window->prefabNames[character])) - strnlen(PREFAB_EXTENSION, sizeof(PREFAB_EXTENSION))), window->prefabNames[character]);
            ImGui::PushID((int)(50000 + character));
            if (ImGui::Selectable(upper_filter))
            {
                if (Arinc661_DefinitionEditor_LoadPrefab(window->definitionEditor, window->prefabNames[character]))
                {
                    window->isDefinitionActive = TRUE;
                }
            }
            ImGui::SetItemTooltip("%s", PREFAB_HINT_LABEL);
            ImGui::PopID();
        }
    }
    for (character = 0; character < sizeof(upper_filter); ++character)
    {
        upper_filter[character] = (char_t)ImToUpper(window->paletteFilter[character]);
        if ('\0' == window->paletteFilter[character])
        {
            break;
        }
    }
    for (category_index = 0; category_index < COUNT_OF(c_categories); ++category_index)
    {
        category     = c_categories[category_index];
        is_extension = (0 == strncmp(category, PALETTE_EXTENSIONS_CATEGORY, sizeof(PALETTE_EXTENSIONS_CATEGORY)));
        has_header   = FALSE;
        for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(is_extension); ++type_index)
        {
            type_name = Arinc661_Definition_GetTypeName(is_extension, type_index);
            (void)Arinc661_Help_GetType(type_name, &description, &categories, &restriction);
            if (is_extension)
            {
                is_in_category = TRUE;
            }
            else if (0 == strncmp(category, PALETTE_OTHER_CATEGORY, sizeof(PALETTE_OTHER_CATEGORY)))
            {
                // Types whose first category matches none of the named groups
                is_in_category = TRUE;
                for (character = 0; character < COUNT_OF(c_categories) - 2; ++character)
                {
                    if (0 == ImStrnicmp(categories, c_categories[character], (int)strnlen(c_categories[character], ARINC661_MAX_NAME_BYTES)))
                    {
                        is_in_category = FALSE;
                    }
                }
            }
            else
            {
                is_in_category = (0 == ImStrnicmp(categories, category, (int)strnlen(category, ARINC661_MAX_NAME_BYTES)));
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
                    Arinc661_DefinitionEditor_AddExtensionOfType(window->definitionEditor, type_name);
                }
                else
                {
                    Arinc661_DefinitionEditor_AddWidgetOfType(window->definitionEditor, type_name);
                }
                window->isDefinitionActive = TRUE;
            }
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload(PALETTE_PAYLOAD_ID, type_name, strnlen(type_name, ARINC661_MAX_NAME_BYTES) + 1);
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

    if ((NULL != text) && ('\0' != text[0]) && (0 != strncmp(text, window->lastStatus, sizeof(window->lastStatus))))
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
    const char_t* line;
    uint32_t      first;
    uint32_t      index;
    bool_t        is_error;
    bool_t        is_warn;

    if (ImGui::SmallButton(LOG_CLEAR_LABEL))
    {
        window->logCount      = 0;
        window->lastStatus[0] = '\0';
    }
    ImGui::SameLine();
    ImGui::Checkbox(LOG_AUTOSCROLL_LABEL, &window->logAutoScroll);
    ImGui::SameLine();
    ImGui::Checkbox(LOG_ERRORS_LABEL, (bool*)&window->logShowsErrors);
    ImGui::SameLine();
    ImGui::Checkbox(LOG_WARNINGS_LABEL, (bool*)&window->logShowsWarnings);
    ImGui::SameLine();
    ImGui::Checkbox(LOG_INFO_LABEL, (bool*)&window->logShowsInfo);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImMax(120.0f, ImGui::GetContentRegionAvail().x));
    (void)ImGui::InputTextWithHint("##log filter", LOG_FILTER_HINT, window->logFilter, sizeof(window->logFilter));
    if (ImGui::BeginChild("##log lines", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
    {
        first = 0;
        if (LOG_CAPACITY < window->logCount)
        {
            first = (window->logCount - LOG_CAPACITY);
        }
        for (index = first; index < window->logCount; ++index)
        {
            line     = window->logLines[index % LOG_CAPACITY];
            is_error = !!((NULL != ImStristr(line, NULL, "error", NULL)) || (NULL != ImStristr(line, NULL, "could not", NULL)) || (NULL != ImStristr(line, NULL, "failed", NULL)));
            is_warn  = !!((FALSE == is_error) && (NULL != ImStristr(line, NULL, "warning", NULL)));
            if ((is_error && (FALSE == window->logShowsErrors)) || (is_warn && (FALSE == window->logShowsWarnings)) || ((FALSE == is_error) && (FALSE == is_warn) && (FALSE == window->logShowsInfo)))
            {
                continue;
            }
            if (('\0' != window->logFilter[0]) && (NULL == ImStristr(line, NULL, window->logFilter, NULL)))
            {
                continue;
            }
            if (is_error)
            {
                ImGui::TextColored(LOG_ERROR_COLOR, "%s", line);
            }
            else if (is_warn)
            {
                ImGui::TextColored(LOG_WARNING_COLOR, "%s", line);
            }
            else
            {
                ImGui::TextUnformatted(line);
            }
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
 *  The name moves to the front of the recent list, which the preferences file keeps.
 */
static void OpenFolderFile(WindowThreadState_t* window, const char_t* name, bool_t isDefinition)
{
    char_t   full_path[MAX_FILE_PATH_BYTES];
    uint32_t index;
    uint32_t found;

    if (isDefinition)
    {
        Arinc661_DefinitionEditor_LoadFile(window->definitionEditor, name);
        window->isDefinitionActive = TRUE;
    }
    else
    {
        Arinc661_Configuration_LoadFile(window->configurationEditor, name);
        (void)Arinc661_Context_OpenReferencedDefinitions(window->context);
        window->isDefinitionActive = FALSE;
    }
    (void)snprintf(full_path, sizeof(full_path), "%s\\%s", window->folderPath, name);
    ::SHAddToRecentDocs(SHARD_PATHA, full_path);
    found = window->recentCount;
    for (index = 0; index < window->recentCount; ++index)
    {
        if (0 == _stricmp(window->recentNames[index], full_path))
        {
            found = index;
        }
    }
    if (found == window->recentCount)
    {
        if (window->recentCount < RECENT_COUNT)
        {
            window->recentCount += 1;
        }
        found = (window->recentCount - 1);
    }
    for (index = found; 0 < index; --index)
    {
        strncpy_s(window->recentNames[index], sizeof(window->recentNames[0]), window->recentNames[index - 1], _TRUNCATE);
        window->recentIsDefinition[index] = window->recentIsDefinition[index - 1];
    }
    strncpy_s(window->recentNames[0], sizeof(window->recentNames[0]), full_path, _TRUNCATE);
    window->recentIsDefinition[0] = isDefinition;
    window->preferencesDirty      = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The folder is rescanned so the name is classified the same way the Files panel does it.
 */
static bool_t OpenPath(WindowThreadState_t* window, const char_t* path)
{
    char_t        note[MAX_FILE_PATH_BYTES + 80];
    const char_t* name;
    const char_t* cursor;
    size_t        folder_length;
    uint32_t      index;

    name = path;
    for (cursor = path; '\0' != (*cursor); ++cursor)
    {
        if (('\\' == (*cursor)) || ('/' == (*cursor)))
        {
            name = (cursor + 1);
        }
    }
    folder_length = (size_t)(name - path);
    if ((0 < folder_length) && (folder_length < sizeof(window->folderPath)))
    {
        (void)memcpy(window->folderPath, path, folder_length - 1);
        window->folderPath[folder_length - 1] = '\0';
        if ((2 == strnlen(window->folderPath, sizeof(window->folderPath))) && (':' == window->folderPath[1]))
        {
            // A drive root keeps its separator
            (void)strcat_s(window->folderPath, sizeof(window->folderPath), "\\");
        }
        window->preferencesDirty = TRUE;
    }
    ScanFolder(window);
    for (index = 0; index < window->fileCount; ++index)
    {
        if (0 == _stricmp(window->fileNames[index], name))
        {
            OpenFolderFile(window, window->fileNames[index], window->fileIsDefinition[index]);
            (void)snprintf(note, sizeof(note), OPENED_PATH_FORMAT, path);
            AppendLog(window, note);
            return TRUE;
        }
    }
    (void)snprintf(note, sizeof(note), NOT_A_DOCUMENT_FORMAT, path);
    AppendLog(window, note);
    window->panelOpen[PANEL_INDEX_LOG] = true;

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  One picker at a time; a second request while one is up is ignored.
 */
static void BeginPick(WindowThreadState_t* window, bool_t isFolder)
{
    HANDLE thread;

    if (DIALOG_IDLE != window->dialogState.load())
    {
        return;
    }
    window->dialogIsFolder = isFolder;
    window->dialogPath[0]  = '\0';
    window->dialogState.store(DIALOG_RUNNING);
    thread = ::CreateThread(NULL, 0, PickThread, window, 0, NULL);
    if (NULL == thread)
    {
        window->dialogState.store(DIALOG_IDLE);
        return;
    }
    ::CloseHandle(thread);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The common item dialog on an apartment thread of its own, owned by the editor window so it
 *  centres there and is modal to it, while the window thread keeps rendering. The path is
 *  written before the state changes, and the window thread reads it only after.
 */
static DWORD WINAPI PickThread(LPVOID parameter)
{
    WindowThreadState_t*  window;
    IFileOpenDialog*      dialog;
    IShellItem*           item;
    IShellItem*           folder;
    wchar_t               wide_folder[MAX_FILE_PATH_BYTES];
    wchar_t*              wide_path;
    FILEOPENDIALOGOPTIONS options;
    int32_t               outcome;

    window  = (WindowThreadState_t*)parameter;
    outcome = DIALOG_CANCELLED;
    (void)::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    dialog = NULL;
    if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))) && (NULL != dialog))
    {
        options = 0;
        (void)dialog->GetOptions(&options);
        options |= (FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
        if (window->dialogIsFolder)
        {
            options |= FOS_PICKFOLDERS;
            (void)dialog->SetTitle(FOLDER_DIALOG_TITLE);
        }
        else
        {
            (void)dialog->SetTitle(OPEN_DIALOG_TITLE);
        }
        (void)dialog->SetOptions(options);
        if (0 < ::MultiByteToWideChar(CP_UTF8, 0, window->folderPath, -1, wide_folder, (int)COUNT_OF(wide_folder)))
        {
            folder = NULL;
            if (SUCCEEDED(::SHCreateItemFromParsingName(wide_folder, NULL, IID_PPV_ARGS(&folder))) && (NULL != folder))
            {
                (void)dialog->SetFolder(folder);
                folder->Release();
            }
        }
        if (SUCCEEDED(dialog->Show(window->windowHandle)))
        {
            item = NULL;
            if (SUCCEEDED(dialog->GetResult(&item)) && (NULL != item))
            {
                wide_path = NULL;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &wide_path)) && (NULL != wide_path))
                {
                    if (0 < ::WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, window->dialogPath, (int)sizeof(window->dialogPath), NULL, NULL))
                    {
                        outcome = DIALOG_PICKED_FILE;
                        if (window->dialogIsFolder)
                        {
                            outcome = DIALOG_PICKED_FOLDER;
                        }
                    }
                    ::CoTaskMemFree(wide_path);
                }
                item->Release();
            }
        }
        dialog->Release();
    }
    ::CoUninitialize();
    window->dialogState.store(outcome);

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Called once per frame on the window thread.
 */
static void FinishPick(WindowThreadState_t* window)
{
    int32_t state;

    state = window->dialogState.load();
    if ((DIALOG_IDLE == state) || (DIALOG_RUNNING == state))
    {
        return;
    }
    if (DIALOG_PICKED_FILE == state)
    {
        (void)OpenPath(window, window->dialogPath);
    }
    else if (DIALOG_PICKED_FOLDER == state)
    {
        strncpy_s(window->folderPath, sizeof(window->folderPath), window->dialogPath, _TRUNCATE);
        ScanFolder(window);
        window->preferencesDirty = TRUE;
    }
    window->dialogState.store(DIALOG_IDLE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Explorer's select verb highlights the file; without a document the folder opens.
 */
static void RevealInExplorer(WindowThreadState_t* window)
{
    char_t        arguments[MAX_FILE_PATH_BYTES + 16];
    const char_t* name;

    name = NULL;
    if (window->isDefinitionActive && (window->definitionEditor->activeDocumentIndex < ARINC661_DEFINITION_EDITOR_MAX_OPEN_DOCUMENTS))
    {
        name = Arinc661_DefinitionEditor_DocumentName(window->definitionEditor, window->definitionEditor->activeDocumentIndex);
    }
    else if ((FALSE == window->isDefinitionActive) && (window->configurationEditor->activeDocumentIndex < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS))
    {
        name = Arinc661_Configuration_DocumentName(window->configurationEditor, window->configurationEditor->activeDocumentIndex);
    }
    if ((NULL != name) && ('\0' != name[0]))
    {
        (void)snprintf(arguments, sizeof(arguments), "/select,\"%s\\%s\"", window->folderPath, name);
        (void)::ShellExecuteA(window->windowHandle, "open", "explorer.exe", arguments, NULL, SW_SHOWNORMAL);
    }
    else
    {
        (void)::ShellExecuteA(window->windowHandle, "open", "explorer.exe", window->folderPath, NULL, SW_SHOWNORMAL);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  One row per state with the change that produced it; the current one is marked and a click
 *  undoes or redoes to any other.
 */
static void DrawHistory(WindowThreadState_t* window)
{
    char_t   label[160];
    uint32_t count;
    uint32_t current;
    uint32_t index;

    count = Arinc661_DefinitionEditor_HistoryCount(window->definitionEditor, &current);
    if (0 == count)
    {
        ImGui::TextDisabled("%s", HISTORY_EMPTY_LABEL);
        return;
    }
    for (index = 0; index < count; ++index)
    {
        if (index == current)
        {
            (void)snprintf(label, sizeof(label), "%u  %s%s", (unsigned)index, Arinc661_DefinitionEditor_HistoryLabel(window->definitionEditor, index), HISTORY_CURRENT_MARK);
        }
        else
        {
            (void)snprintf(label, sizeof(label), "%u  %s", (unsigned)index, Arinc661_DefinitionEditor_HistoryLabel(window->definitionEditor, index));
        }
        ImGui::PushID((int)index);
        if (index > current)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        }
        if (ImGui::Selectable(label, index == current))
        {
            Arinc661_DefinitionEditor_GoToHistory(window->definitionEditor, index);
        }
        if (index > current)
        {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A running check or download is left alone.
 */
static void BeginUpdateCheck(WindowThreadState_t* window, bool_t isManual)
{
    HANDLE thread;

    if (UPDATE_STATE_IDLE != window->updateState.load())
    {
        return;
    }
    window->updateIsManual = isManual;
    if ('\0' == window->updateUrl[0])
    {
        if (isManual)
        {
            AppendLog(window, UPDATE_NO_SOURCE_LABEL);
            window->panelOpen[PANEL_INDEX_LOG] = true;
        }
        return;
    }
    window->updateState.store(UPDATE_STATE_CHECKING);
    thread = ::CreateThread(NULL, 0, UpdateThread, window, 0, NULL);
    if (NULL == thread)
    {
        window->updateState.store(UPDATE_STATE_IDLE);
        return;
    }
    ::CloseHandle(thread);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  CHECKING fetches the manifest and settles to CHECKED; DOWNLOADING fetches the package and
 *  settles to DOWNLOADED. Results are written before the state changes.
 */
static DWORD WINAPI UpdateThread(LPVOID parameter)
{
    WindowThreadState_t*      window;
    Arinc661_Update_Version_t installed;

    window = (WindowThreadState_t*)parameter;
    if (UPDATE_STATE_CHECKING == window->updateState.load())
    {
        (void)Arinc661_Update_ParseVersion(ARINC661_EDITOR_VERSION_TEXT, &installed);
        window->updateResult = Arinc661_Update_Check(window->updateUrl, &installed, &window->updateManifest);
        window->updateState.store(UPDATE_STATE_CHECKED);
    }
    else if (UPDATE_STATE_DOWNLOADING == window->updateState.load())
    {
        window->updateResult = Arinc661_Update_Download(&window->updateManifest, window->updatePackagePath, sizeof(window->updatePackagePath));
        window->updateState.store(UPDATE_STATE_DOWNLOADED);
    }

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A silent check only speaks when a version the user has not skipped is available. A manual
 *  check reports every outcome. Installing starts msiexec and ends the session so files can be
 *  replaced; the installer relaunches the editor.
 */
static void DrawUpdatePopup(WindowThreadState_t* window)
{
    char_t  line[ARINC661_UPDATE_MAX_NOTE_BYTES + 128];
    char_t  executable[MAX_FILE_PATH_BYTES];
    int32_t state;
    HANDLE  thread;

    state = window->updateState.load();
    if (UPDATE_STATE_CHECKED == state)
    {
        window->updateState.store(UPDATE_STATE_IDLE);
        window->updateCheckedAt  = (float64_t)time(NULL);
        window->preferencesDirty = TRUE;
        if (ARINC661_UPDATE_RESULT_AVAILABLE == window->updateResult)
        {
            if (window->updateIsManual || (0 != strncmp(window->updateSkipVersion, window->updateManifest.versionText, sizeof(window->updateSkipVersion))))
            {
                window->opensUpdatePopup = TRUE;
            }
        }
        else if (window->updateIsManual)
        {
            window->opensUpdatePopup = TRUE;
        }
    }
    else if (UPDATE_STATE_DOWNLOADED == state)
    {
        window->updateState.store(UPDATE_STATE_IDLE);
        if (ARINC661_UPDATE_RESULT_DOWNLOADED == window->updateResult)
        {
            executable[0] = '\0';
            (void)::GetModuleFileNameA(NULL, executable, (DWORD)sizeof(executable));
            if (Arinc661_Update_Apply(window->updatePackagePath, executable))
            {
                AppendLog(window, UPDATE_APPLYING_LABEL);
                window->isApplyingUpdate = TRUE;
                SavePreferences(window);
                window->isDone.store(TRUE);
            }
            else
            {
                window->updateResult = ARINC661_UPDATE_RESULT_UNREACHABLE;
                AppendLog(window, UPDATE_APPLY_FAIL_LABEL);
            }
        }
    }
    if (window->opensUpdatePopup)
    {
        ImGui::OpenPopup(UPDATE_POPUP_ID);
        window->opensUpdatePopup = FALSE;
    }
    if (ImGui::BeginPopupModal(UPDATE_POPUP_ID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        state = window->updateState.load();
        if (UPDATE_STATE_CHECKING == state)
        {
            ImGui::TextUnformatted(UPDATE_CHECKING_LABEL);
        }
        else if (UPDATE_STATE_DOWNLOADING == state)
        {
            ImGui::TextUnformatted(UPDATE_DOWNLOADING_LABEL);
        }
        else if (ARINC661_UPDATE_RESULT_AVAILABLE == window->updateResult)
        {
            (void)snprintf(line, sizeof(line), UPDATE_AVAILABLE_FORMAT, window->updateManifest.versionText, ARINC661_EDITOR_VERSION_TEXT);
            ImGui::TextUnformatted(line);
            if ('\0' != window->updateManifest.notes[0])
            {
                (void)snprintf(line, sizeof(line), UPDATE_NOTES_FORMAT, window->updateManifest.notes);
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
                ImGui::TextUnformatted(line);
                ImGui::PopTextWrapPos();
            }
            ImGui::Separator();
            if (ImGui::Button(UPDATE_INSTALL_LABEL))
            {
                window->updateState.store(UPDATE_STATE_DOWNLOADING);
                thread = ::CreateThread(NULL, 0, UpdateThread, window, 0, NULL);
                if (NULL == thread)
                {
                    window->updateState.store(UPDATE_STATE_IDLE);
                    window->updateResult = ARINC661_UPDATE_RESULT_UNREACHABLE;
                }
                else
                {
                    ::CloseHandle(thread);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(UPDATE_LATER_LABEL))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(UPDATE_SKIP_LABEL))
            {
                strncpy_s(window->updateSkipVersion, sizeof(window->updateSkipVersion), window->updateManifest.versionText, _TRUNCATE);
                window->preferencesDirty = TRUE;
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            if (ARINC661_UPDATE_RESULT_UP_TO_DATE == window->updateResult)
            {
                ImGui::TextUnformatted(UPDATE_UP_TO_DATE_LABEL);
            }
            else if (ARINC661_UPDATE_RESULT_MALFORMED == window->updateResult)
            {
                ImGui::TextUnformatted(UPDATE_MALFORMED_LABEL);
            }
            else if (ARINC661_UPDATE_RESULT_CORRUPT == window->updateResult)
            {
                ImGui::TextUnformatted(UPDATE_CORRUPT_LABEL);
            }
            else
            {
                ImGui::TextUnformatted(UPDATE_UNREACHABLE_LABEL);
            }
            if (ImGui::Button(UPDATE_OK_LABEL))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Product, version, build, the libraries, where it runs from, and the update source; Copy
 *  Details puts the same text on the clipboard for a bug report.
 */
static void DrawAboutPopup(WindowThreadState_t* window)
{
    char_t        details[2048];
    char_t        executable[MAX_FILE_PATH_BYTES];
    const char_t* opengl;
    size_t        cut;

    if (window->opensAboutPopup)
    {
        ImGui::OpenPopup(ABOUT_POPUP_ID);
        window->opensAboutPopup = FALSE;
    }
    if (ImGui::BeginPopupModal(ABOUT_POPUP_ID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        executable[0] = '\0';
        (void)::GetModuleFileNameA(NULL, executable, (DWORD)sizeof(executable));
        cut = strnlen(executable, sizeof(executable));
        while ((0 < cut) && ('\\' != executable[cut - 1]))
        {
            cut -= 1;
        }
        if (0 < cut)
        {
            executable[cut - 1] = '\0';
        }
        opengl = (const char_t*)glGetString(GL_VERSION);
        if (NULL == opengl)
        {
            opengl = "";
        }
        ImGui::PushFont(NULL, ImGui::GetFontSize() * 1.4f);
        ImGui::TextUnformatted(ARINC661_EDITOR_PRODUCT_NAME);
        ImGui::PopFont();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
        ImGui::TextDisabled("%s", ABOUT_DESCRIPTION_LABEL);
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        (void)snprintf(details, sizeof(details), ABOUT_DETAILS_FORMAT, ARINC661_EDITOR_VERSION_TEXT, __DATE__, __TIME__, IMGUI_VERSION, opengl, executable);
        if ('\0' != window->updateUrl[0])
        {
            (void)snprintf(&details[strnlen(details, sizeof(details))], sizeof(details) - strnlen(details, sizeof(details)), ABOUT_UPDATE_URL_FORMAT, window->updateUrl);
        }
        else
        {
            (void)snprintf(&details[strnlen(details, sizeof(details))], sizeof(details) - strnlen(details, sizeof(details)), "%s", ABOUT_NO_UPDATE_URL_LABEL);
        }
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
        ImGui::TextUnformatted(details);
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
        ImGui::TextDisabled("%s", ABOUT_COPYRIGHT_LABEL);
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        if (ImGui::Button(CHECK_UPDATES_LABEL))
        {
            ImGui::CloseCurrentPopup();
            BeginUpdateCheck(window, TRUE);
            window->opensUpdatePopup = (bool_t)(UPDATE_STATE_IDLE != window->updateState.load());
        }
        ImGui::SameLine();
        if (ImGui::Button(ABOUT_COPY_LABEL))
        {
            ImGui::SetClipboardText(details);
        }
        ImGui::SameLine();
        if (ImGui::Button(ABOUT_HELP_LABEL))
        {
            OpenWindowsHelp(window, HH_DISPLAY_TOC_COMMAND);
        }
        ImGui::SameLine();
        if (ImGui::Button(ABOUT_CLOSE_LABEL) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  HKCU first, then HKLM, where the installer writes UPDATEURL.
 */
static void LoadUpdateSource(WindowThreadState_t* window)
{
    HKEY     roots[2];
    HKEY     key;
    DWORD    byte_count;
    DWORD    type;
    uint32_t index;

    if ('\0' != window->updateUrl[0])
    {
        return;
    }
    roots[0] = HKEY_CURRENT_USER;
    roots[1] = HKEY_LOCAL_MACHINE;
    for (index = 0; index < COUNT_OF(roots); ++index)
    {
        if ('\0' != window->updateUrl[0])
        {
            break;
        }
        if (ERROR_SUCCESS == ::RegOpenKeyExA(roots[index], UPDATE_REGISTRY_KEY, 0, KEY_READ, &key))
        {
            byte_count = (DWORD)(sizeof(window->updateUrl) - 1);
            type       = REG_NONE;
            if ((ERROR_SUCCESS == ::RegQueryValueExA(key, UPDATE_REGISTRY_VALUE, NULL, &type, (BYTE*)window->updateUrl, &byte_count)) && (REG_SZ == type))
            {
                window->updateUrl[byte_count] = '\0';
            }
            else
            {
                window->updateUrl[0] = '\0';
            }
            ::RegCloseKey(key);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies the selection, writes it under the typed name, and rescans the folder so the Palette
 *  lists it at once.
 */
static void DrawSavePrefabPopup(WindowThreadState_t* window)
{
    char_t path[MAX_FILE_PATH_BYTES];
    char_t note[MAX_FILE_PATH_BYTES + 32];

    if (window->opensPrefabPopup)
    {
        ImGui::OpenPopup(SAVE_PREFAB_POPUP_ID);
        window->opensPrefabPopup = FALSE;
    }
    if (ImGui::BeginPopupModal(SAVE_PREFAB_POPUP_ID, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        if (ImGui::InputTextWithHint("##prefab name", SAVE_PREFAB_NAME_HINT, window->prefabName, sizeof(window->prefabName), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button(SAVE_PREFAB_OK_LABEL))
        {
            (void)Arinc661_DefinitionEditor_Copy(window->definitionEditor);
            if (Arinc661_DefinitionEditor_SavePrefab(window->definitionEditor, window->prefabName, path, sizeof(path)))
            {
                (void)snprintf(note, sizeof(note), SAVE_PREFAB_NOTE_FORMAT, path);
                AppendLog(window, note);
                ScanFolder(window);
            }
            else
            {
                AppendLog(window, SAVE_PREFAB_FAIL_LABEL);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(SAVE_PREFAB_CANCEL_LABEL) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A project-hub style page: title, the three ways to start, the recent files, and the help
 *  entry points.
 */
static void DrawStartPage(WindowThreadState_t* window)
{
    char_t   label[MAX_FILE_NAME_BYTES + 32];
    uint32_t index;

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    ImGui::Indent(ImGui::GetTextLineHeight() * 2.0f);
    ImGui::PushFont(NULL, ImGui::GetFontSize() * 1.6f);
    ImGui::TextUnformatted(START_TITLE_LABEL);
    ImGui::PopFont();
    ImGui::TextDisabled("%s", START_SUBTITLE_LABEL);
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    if (ImGui::Button(NEW_CONFIGURATION_LABEL, ImVec2(ImMax(START_BUTTON_WIDTH, ImGui::CalcTextSize(NEW_APPLICATION_LABEL).x + ImGui::GetStyle().FramePadding.x * 4.0f), 0.0f)))
    {
        Arinc661_Configuration_CreateDocument(window->configurationEditor);
        window->isDefinitionActive = FALSE;
    }
    ImGui::SameLine();
    if (ImGui::Button(NEW_DEFINITION_LABEL, ImVec2(ImMax(START_BUTTON_WIDTH, ImGui::CalcTextSize(NEW_APPLICATION_LABEL).x + ImGui::GetStyle().FramePadding.x * 4.0f), 0.0f)))
    {
        Arinc661_DefinitionEditor_CreateDocument(window->definitionEditor);
        window->isDefinitionActive = TRUE;
    }
    ImGui::SameLine();
    if (ImGui::Button(NEW_APPLICATION_LABEL, ImVec2(ImMax(START_BUTTON_WIDTH, ImGui::CalcTextSize(NEW_APPLICATION_LABEL).x + ImGui::GetStyle().FramePadding.x * 4.0f), 0.0f)))
    {
        (void)Arinc661_Context_NewApplication(window->context);
    }
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    ImGui::SeparatorText(START_RECENT_LABEL);
    if (0 == window->recentCount)
    {
        ImGui::TextDisabled("%s", RECENT_EMPTY_LABEL);
    }
    for (index = 0; index < window->recentCount; ++index)
    {
        if (window->recentIsDefinition[index])
        {
            (void)snprintf(label, sizeof(label), DEFINITION_FILE_FORMAT, window->recentNames[index]);
        }
        else
        {
            (void)snprintf(label, sizeof(label), CONFIGURATION_FILE_FORMAT, window->recentNames[index]);
        }
        ImGui::PushID((int)index);
        if (ImGui::Selectable(label, false, 0, ImVec2(START_BUTTON_WIDTH * 3.0f, 0.0f)))
        {
            (void)OpenPath(window, window->recentNames[index]);
        }
        ImGui::PopID();
    }
    ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    ImGui::SeparatorText(START_LEARN_LABEL);
    if (ImGui::Selectable(START_OPEN_HELP_LABEL, false, 0, ImVec2(START_BUTTON_WIDTH * 3.0f, 0.0f)))
    {
        OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND);
    }
    if (ImGui::Selectable(START_SHORTCUTS_LABEL, false, 0, ImVec2(START_BUTTON_WIDTH * 3.0f, 0.0f)))
    {
        ShowHelp(window, SHORTCUTS_LABEL);
    }
    ImGui::Unindent(ImGui::GetTextLineHeight() * 2.0f);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rows list each animation with a bar from Delay to Delay plus Duration over a shared time
 *  axis; the cursor follows the time slider, which the Play button advances with the clock.
 *  The scene and the window previews draw the animated state at that time.
 */
static void DrawTimeline(WindowThreadState_t* window)
{
    const char_t* name;
    const char_t* type;
    ImDrawList*   draw_list;
    ImVec2        row_min;
    ImVec2        axis_min;
    float32_t     axis_width;
    float32_t     name_width;
    float32_t     delay;
    float32_t     duration;
    float32_t     end;
    uint32_t      index;
    char_t        label[ARINC661_MAX_NAME_BYTES + 64];

    window->timelineLength = 0.0f;
    for (index = 0; Arinc661_DefinitionEditor_AnimationAt(window->definitionEditor, index, &name, &type, &delay, &duration); ++index)
    {
        end = (delay + duration);
        if (window->timelineLength < end)
        {
            window->timelineLength = end;
        }
    }
    if (0 == index)
    {
        ImGui::TextWrapped("%s", TIMELINE_EMPTY_LABEL);
        window->isTimelinePlaying = FALSE;
        Arinc661_DefinitionEditor_SetAnimationTime(window->definitionEditor, -1.0f);
        return;
    }
    if (window->isTimelinePlaying)
    {
        if (ImGui::Button(TIMELINE_PAUSE_LABEL))
        {
            window->isTimelinePlaying = FALSE;
        }
    }
    else if (ImGui::Button(TIMELINE_PLAY_LABEL))
    {
        window->isTimelinePlaying = TRUE;
    }
    ImGui::SameLine();
    if (ImGui::Button(TIMELINE_STOP_LABEL))
    {
        window->isTimelinePlaying = FALSE;
        window->timelineTime      = 0.0f;
        Arinc661_DefinitionEditor_SetAnimationTime(window->definitionEditor, -1.0f);
    }
    ImGui::SameLine();
    ImGui::Checkbox(TIMELINE_LOOP_LABEL, (bool*)&window->isTimelineLooping);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderFloat(TIMELINE_TIME_LABEL, &window->timelineTime, 0.0f, ImMax(window->timelineLength, 0.001f), "%.2f s"))
    {
        Arinc661_DefinitionEditor_SetAnimationTime(window->definitionEditor, window->timelineTime);
    }
    if (window->isTimelinePlaying)
    {
        window->timelineTime += ImGui::GetIO().DeltaTime;
        if (window->timelineTime > window->timelineLength)
        {
            if (window->isTimelineLooping)
            {
                window->timelineTime = 0.0f;
            }
            else
            {
                window->timelineTime      = window->timelineLength;
                window->isTimelinePlaying = FALSE;
            }
        }
        Arinc661_DefinitionEditor_SetAnimationTime(window->definitionEditor, window->timelineTime);
    }
    draw_list  = ImGui::GetWindowDrawList();
    // Name column as wide as the longest label so bars never cross text
    name_width = TIMELINE_NAME_WIDTH;
    for (index = 0; Arinc661_DefinitionEditor_AnimationAt(window->definitionEditor, index, &name, &type, &delay, &duration); ++index)
    {
        (void)snprintf(label, sizeof(label), "%s (%s)", name, type + 5);
        name_width = ImMax(name_width, ImGui::CalcTextSize(label).x + ImGui::GetStyle().ItemSpacing.x * 2.0f);
    }
    axis_width = ImMax(50.0f, ImGui::GetContentRegionAvail().x - name_width);
    for (index = 0; Arinc661_DefinitionEditor_AnimationAt(window->definitionEditor, index, &name, &type, &delay, &duration); ++index)
    {
        row_min  = ImGui::GetCursorScreenPos();
        axis_min = ImVec2(row_min.x + name_width, row_min.y);
        ImGui::Text("%s (%s)", name, type + 5);
        draw_list->AddRectFilled(ImVec2(axis_min.x, row_min.y + 2.0f), ImVec2(axis_min.x + axis_width, row_min.y + TIMELINE_ROW_HEIGHT - 2.0f), IM_COL32(255, 255, 255, 12));
        if (0.0f < window->timelineLength)
        {
            draw_list->AddRectFilled(ImVec2(axis_min.x + (axis_width * (delay / window->timelineLength)), row_min.y + 4.0f), ImVec2(axis_min.x + (axis_width * ((delay + duration) / window->timelineLength)), row_min.y + TIMELINE_ROW_HEIGHT - 4.0f), TIMELINE_BAR_COLOR, 3.0f);
            draw_list->AddLine(ImVec2(axis_min.x + (axis_width * (window->timelineTime / window->timelineLength)), row_min.y), ImVec2(axis_min.x + (axis_width * (window->timelineTime / window->timelineLength)), row_min.y + TIMELINE_ROW_HEIGHT), TIMELINE_CURSOR_COLOR, 2.0f);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Entering play snapshots the Definition Files and focuses the configuration view, where the
 *  windows show the applications as a CDS would; leaving restores them.
 */
static void TogglePlay(WindowThreadState_t* window)
{
    if (window->definitionEditor->isPlaying)
    {
        Arinc661_DefinitionEditor_EndPlay(window->definitionEditor);
    }
    else
    {
        Arinc661_DefinitionEditor_BeginPlay(window->definitionEditor);
        window->isDefinitionActive = FALSE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each report line becomes a log line.
 */
static void BuildAll(WindowThreadState_t* window)
{
    char_t        report[BUILD_REPORT_BYTES];
    char_t*       line;
    char_t*       next;
    const char_t* line_end;

    (void)Arinc661_Context_Build(window->context, report, sizeof(report));
    line = report;
    while (NULL != line)
    {
        // Take the match as an index into the report, which stays writable
        line_end = FACEOE_Utl_StrChr(line, '\n', sizeof(report) - (size_t)(line - report));
        next     = NULL;
        if (NULL != line_end)
        {
            next    = &report[line_end - report];
            (*next) = '\0';
            next += 1;
        }
        if ('\0' != line[0])
        {
            window->lastStatus[0] = '\0';
            AppendLog(window, line);
        }
        line = next;
    }
    window->panelOpen[PANEL_INDEX_LOG] = true;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  One list mixes commands, folder files, and the active Definition File's widgets; the first
 *  match runs on Enter. Escape closes.
 */
static void DrawCommandPalette(WindowThreadState_t* window)
{
    static const char_t* const c_commands[] = {NEW_CONFIGURATION_LABEL, NEW_DEFINITION_LABEL, NEW_APPLICATION_LABEL, SAVE_LABEL, SAVE_ALL_LABEL, BUILD_LABEL, PLAY_LABEL, UNDO_LABEL, REDO_LABEL, OPEN_REFERENCED_LABEL, RESET_LAYOUT_LABEL, HELP_CONTEXT_LABEL, SHORTCUTS_LABEL};
    const Arinc661_Definition_t* definition;
    char_t                       upper_filter[MAX_HELP_FILTER_BYTES];
    char_t                       upper_item[ARINC661_MAX_NAME_BYTES + 64];
    char_t                       label[ARINC661_MAX_NAME_BYTES + 64];
    uint32_t                     index;
    uint32_t                     character;
    uint32_t                     shown;
    bool_t                       runs_first;
    bool_t                       is_match;

    if (FALSE == window->isPaletteOpen)
    {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->GetCenter().x, ImGui::GetMainViewport()->WorkPos.y + 80.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(640.0f, 0.0f));
    if (ImGui::Begin(COMMAND_PALETTE_ID, NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
    {
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1.0f);
        runs_first = ImGui::InputTextWithHint("##command", COMMAND_PALETTE_HINT, window->commandFilter, sizeof(window->commandFilter), ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            window->isPaletteOpen = FALSE;
        }
        for (character = 0; character < sizeof(upper_filter); ++character)
        {
            upper_filter[character] = (char_t)ImToUpper(window->commandFilter[character]);
            if ('\0' == window->commandFilter[character])
            {
                break;
            }
        }
        shown = 0;
        if (ImGui::BeginChild("##command list", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * COMMAND_PALETTE_ROWS), ImGuiChildFlags_Borders))
        {
            for (index = 0; index < COUNT_OF(c_commands); ++index)
            {
                for (character = 0; character < sizeof(upper_item); ++character)
                {
                    upper_item[character] = (char_t)ImToUpper(c_commands[index][character]);
                    if ('\0' == c_commands[index][character])
                    {
                        break;
                    }
                }
                is_match = !!(('\0' == upper_filter[0]) || (NULL != strstr(upper_item, upper_filter)));
                if (is_match)
                {
                    (void)snprintf(label, sizeof(label), "> %s", c_commands[index]);
                    if (ImGui::Selectable(label) || (runs_first && (0 == shown)))
                    {
                        if (0 == strncmp(c_commands[index], NEW_CONFIGURATION_LABEL, sizeof(NEW_CONFIGURATION_LABEL))) { Arinc661_Configuration_CreateDocument(window->configurationEditor); window->isDefinitionActive = FALSE; }
                        else if (0 == strncmp(c_commands[index], NEW_DEFINITION_LABEL, sizeof(NEW_DEFINITION_LABEL))) { Arinc661_DefinitionEditor_CreateDocument(window->definitionEditor); window->isDefinitionActive = TRUE; }
                        else if (0 == strncmp(c_commands[index], NEW_APPLICATION_LABEL, sizeof(NEW_APPLICATION_LABEL))) { (void)Arinc661_Context_NewApplication(window->context); }
                        else if (0 == strncmp(c_commands[index], SAVE_LABEL, sizeof(SAVE_LABEL))) { SaveActive(window); }
                        else if (0 == strncmp(c_commands[index], SAVE_ALL_LABEL, sizeof(SAVE_ALL_LABEL))) { Arinc661_Configuration_SaveAllDocuments(window->configurationEditor); Arinc661_DefinitionEditor_SaveAllDocuments(window->definitionEditor); }
                        else if (0 == strncmp(c_commands[index], BUILD_LABEL, sizeof(BUILD_LABEL))) { BuildAll(window); }
                        else if (0 == strncmp(c_commands[index], PLAY_LABEL, sizeof(PLAY_LABEL))) { TogglePlay(window); }
                        else if (0 == strncmp(c_commands[index], UNDO_LABEL, sizeof(UNDO_LABEL))) { if (window->isDefinitionActive) { Arinc661_DefinitionEditor_Undo(window->definitionEditor); } else { Arinc661_Configuration_Undo(window->configurationEditor); } }
                        else if (0 == strncmp(c_commands[index], REDO_LABEL, sizeof(REDO_LABEL))) { if (window->isDefinitionActive) { Arinc661_DefinitionEditor_Redo(window->definitionEditor); } else { Arinc661_Configuration_Redo(window->configurationEditor); } }
                        else if (0 == strncmp(c_commands[index], OPEN_REFERENCED_LABEL, sizeof(OPEN_REFERENCED_LABEL))) { (void)Arinc661_Context_OpenReferencedDefinitions(window->context); }
                        else if (0 == strncmp(c_commands[index], RESET_LAYOUT_LABEL, sizeof(RESET_LAYOUT_LABEL))) { window->resetsLayout = TRUE; }
                        else if (0 == strncmp(c_commands[index], HELP_CONTEXT_LABEL, sizeof(HELP_CONTEXT_LABEL))) { OpenWindowsHelp(window, HH_DISPLAY_TOPIC_COMMAND); }
                        else { ShowHelp(window, SHORTCUTS_LABEL); }
                        window->isPaletteOpen = FALSE;
                    }
                    shown += 1;
                }
            }
            // Files of the folder
            for (index = 0; index < window->fileCount; ++index)
            {
                for (character = 0; character < sizeof(upper_item); ++character)
                {
                    upper_item[character] = (char_t)ImToUpper(window->fileNames[index][character]);
                    if ('\0' == window->fileNames[index][character])
                    {
                        break;
                    }
                }
                if (('\0' != upper_filter[0]) && (NULL != strstr(upper_item, upper_filter)))
                {
                    (void)snprintf(label, sizeof(label), "Open %s", window->fileNames[index]);
                    if (ImGui::Selectable(label) || (runs_first && (0 == shown)))
                    {
                        OpenFolderFile(window, window->fileNames[index], window->fileIsDefinition[index]);
                        window->isPaletteOpen = FALSE;
                    }
                    shown += 1;
                }
            }
            // Widgets of the active Definition File
            definition = Arinc661_DefinitionEditor_Document(window->definitionEditor, window->definitionEditor->activeDocumentIndex);
            if ((NULL != definition) && ('\0' != upper_filter[0]))
            {
                for (index = 0; index < definition->widgetCount; ++index)
                {
                    for (character = 0; character < sizeof(upper_item); ++character)
                    {
                        upper_item[character] = (char_t)ImToUpper(definition->widgets[index].name[character]);
                        if ('\0' == definition->widgets[index].name[character])
                        {
                            break;
                        }
                    }
                    if (NULL != strstr(upper_item, upper_filter))
                    {
                        (void)snprintf(label, sizeof(label), "Select %s (%s)", definition->widgets[index].name, definition->widgets[index].type);
                        ImGui::PushID((int)(100000 + index));
                        if (ImGui::Selectable(label) || (runs_first && (0 == shown)))
                        {
                            Arinc661_DefinitionEditor_SelectWidget(window->definitionEditor, window->definitionEditor->activeDocumentIndex, index);
                            window->isDefinitionActive = TRUE;
                            window->isPaletteOpen      = FALSE;
                        }
                        ImGui::PopID();
                        shown += 1;
                    }
                }
            }
        }
        ImGui::EndChild();
        if ((FALSE == ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) && (FALSE == ImGui::IsWindowAppearing()))
        {
            window->isPaletteOpen = FALSE;
        }
    }
    ImGui::End();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dark or light ImGui scheme with the viewport tweaks, then the font scale.
 */
static void ApplyTheme(WindowThreadState_t* window)
{
    ImGuiStyle* style;
    ImVec4*     colors;
    ImVec4*     style_colors_dark;

    if (window->isLightTheme)
    {
        ImGui::StyleColorsLight();
    }
    else
    {
        ImGui::StyleColorsDark();
        style_colors_dark = ImGui::GetStyle().Colors;
        // Near-black panels, darker input wells, flat blue accent, as in a modern engine editor
        colors = style_colors_dark;
        colors[ImGuiCol_Text]                  = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]              = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.11f, 0.11f, 0.11f, 0.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.09f, 0.09f, 0.09f, 0.98f);
        colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]               = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg]               = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.08f, 0.08f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.16f, 0.38f, 0.66f, 1.00f);
        colors[ImGuiCol_Separator]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                   = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TabSelected]           = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline]   = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TabDimmed]             = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_DockingPreview]        = ImVec4(0.26f, 0.59f, 0.98f, 0.50f);
        colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_NavCursor]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    }
    style = &ImGui::GetStyle();
    // Flat engine-editor metrics: tight rounded frames, square tabs, thin borders
    style->WindowPadding     = ImVec2(8.0f, 8.0f);
    style->FramePadding      = ImVec2(6.0f, 4.0f);
    style->CellPadding       = ImVec2(6.0f, 3.0f);
    style->ItemSpacing       = ImVec2(8.0f, 5.0f);
    style->ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style->IndentSpacing     = 18.0f;
    style->ScrollbarSize     = 12.0f;
    style->GrabMinSize       = 10.0f;
    style->WindowBorderSize  = 1.0f;
    style->ChildBorderSize   = 1.0f;
    style->PopupBorderSize   = 1.0f;
    style->FrameBorderSize   = 0.0f;
    style->TabBorderSize     = 0.0f;
    style->WindowRounding    = 4.0f;
    style->ChildRounding     = 3.0f;
    style->FrameRounding     = 3.0f;
    style->PopupRounding     = 4.0f;
    style->ScrollbarRounding = 6.0f;
    style->GrabRounding      = 3.0f;
    style->TabRounding       = 0.0f;
    style->TabBarOverlineSize = 2.0f;
    style->DockingSeparatorSize = 2.0f;
    style->WindowTitleAlign  = ImVec2(0.0f, 0.5f);
    style->WindowMenuButtonPosition = ImGuiDir_None;
    style->SeparatorTextBorderSize = 1.0f;
    style->SeparatorTextPadding = ImVec2(12.0f, 4.0f);
    style->ScaleAllSizes(window->mainScale);
    style->FontScaleDpi  = window->mainScale;
    style->FontScaleMain = window->uiScale;
    if (0 != (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        style->WindowRounding              = 0.0f;
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Two lines, key=value: scale and theme.
 */
static void LoadPreferences(WindowThreadState_t* window)
{
    FILE*  file;
    char_t line[MAX_FILE_PATH_BYTES + 64];
    char_t key[32];
    char_t value[MAX_FILE_PATH_BYTES + 4];

    window->uiScale      = 1.0f;
    window->isLightTheme = FALSE;
    window->recentCount  = 0;
    window->updateUrl[0]         = '\0';
    window->updateSkipVersion[0] = '\0';
    window->updateCheckedAt      = 0.0;
    file = fopen(PREFERENCES_FILE, "r");
    if (NULL != file)
    {
        while (NULL != fgets(line, sizeof(line), file))
        {
            if (2 == sscanf_s(line, "%31[^=]=%1023[^\r\n]", key, (unsigned)sizeof(key), value, (unsigned)sizeof(value)))
            {
                if (0 == strncmp(key, PREFERENCE_SCALE_KEY, sizeof(PREFERENCE_SCALE_KEY)))
                {
                    window->uiScale = ImClamp((float32_t)atof(value), UI_SCALE_MIN, UI_SCALE_MAX);
                }
                else if (0 == strncmp(key, PREFERENCE_THEME_KEY, sizeof(PREFERENCE_THEME_KEY)))
                {
                    window->isLightTheme = (0 == strncmp(value, "light", sizeof("light")));
                }
                else if (0 == strncmp(key, PREFERENCE_FOLDER_KEY, sizeof(PREFERENCE_FOLDER_KEY)))
                {
                    strncpy_s(window->folderPath, sizeof(window->folderPath), value, _TRUNCATE);
                }
                else if (0 == strncmp(key, PREFERENCE_UPDATE_URL_KEY, sizeof(PREFERENCE_UPDATE_URL_KEY)))
                {
                    strncpy_s(window->updateUrl, sizeof(window->updateUrl), value, _TRUNCATE);
                }
                else if (0 == strncmp(key, PREFERENCE_UPDATE_CHECKED_KEY, sizeof(PREFERENCE_UPDATE_CHECKED_KEY)))
                {
                    window->updateCheckedAt = atof(value);
                }
                else if (0 == strncmp(key, PREFERENCE_UPDATE_SKIP_KEY, sizeof(PREFERENCE_UPDATE_SKIP_KEY)))
                {
                    strncpy_s(window->updateSkipVersion, sizeof(window->updateSkipVersion), value, _TRUNCATE);
                }
                else if ((0 == strncmp(key, PREFERENCE_RECENT_KEY, sizeof(PREFERENCE_RECENT_KEY))) && (('d' == value[0]) || ('c' == value[0])) && (':' == value[1]) && (window->recentCount < RECENT_COUNT))
                {
                    strncpy_s(window->recentNames[window->recentCount], sizeof(window->recentNames[0]), value + 2, _TRUNCATE);
                    window->recentIsDefinition[window->recentCount] = ('d' == value[0]);
                    window->recentCount += 1;
                }
            }
        }
        (void)fclose(file);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Written beside the executable's working directory.
 */
static void SavePreferences(WindowThreadState_t* window)
{
    FILE*         file;
    const char_t* theme;
    uint32_t      index;
    char_t        kind;

    if (window->preferencesDirty)
    {
        file = fopen(PREFERENCES_FILE, "w");
        if (NULL != file)
        {
            theme = "dark";
            if (window->isLightTheme)
            {
                theme = "light";
            }
            (void)fprintf(file, "%s=%.2f\n%s=%s\n%s=%s\n", PREFERENCE_SCALE_KEY, (float64_t)window->uiScale, PREFERENCE_THEME_KEY, theme, PREFERENCE_FOLDER_KEY, window->folderPath);
            (void)fprintf(file, "%s=%s\n%s=%.0f\n%s=%s\n", PREFERENCE_UPDATE_URL_KEY, window->updateUrl, PREFERENCE_UPDATE_CHECKED_KEY, window->updateCheckedAt, PREFERENCE_UPDATE_SKIP_KEY, window->updateSkipVersion);
            for (index = 0; index < window->recentCount; ++index)
            {
                kind = 'c';
                if (window->recentIsDefinition[index])
                {
                    kind = 'd';
                }
                (void)fprintf(file, "%s=%c:%s\n", PREFERENCE_RECENT_KEY, kind, window->recentNames[index]);
            }
            (void)fclose(file);
        }
        window->preferencesDirty = FALSE;
    }
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
        status_text = Arinc661_DefinitionEditor_StatusText(window->definitionEditor);
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
    Arinc661_DefinitionEditor_SelectionText(window->definitionEditor, definition_crumb, sizeof(definition_crumb));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped(CONTEXT_BAR_FORMAT, configuration_crumb, definition_crumb);
    ImGui::PopStyleColor();
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
    wchar_t              dropped[MAX_FILE_PATH_BYTES];

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
    case WM_DROPFILES:
    {
        // The first dropped file opens next frame on this thread
        if ((NULL != window) && (0 < ::DragQueryFileW((HDROP)wordParameter, 0, dropped, (UINT)COUNT_OF(dropped))))
        {
            (void)::WideCharToMultiByte(CP_UTF8, 0, dropped, -1, window->pendingOpenPath, (int)sizeof(window->pendingOpenPath), NULL, NULL);
        }
        ::DragFinish((HDROP)wordParameter);
        return 0;
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
    uint32_t                  panel_index;
    WindowThreadState_t*      window;
    WNDCLASSEXW               window_class;
    MONITORINFO               monitor_info;
    RECT                      monitor_rect;
    int                       window_width;
    int                       window_height;
    ImGuiIO*                  io;
    ImGuiStyle*               style;
    ImGuiPlatformIO*          platform_io;
    WglSwapIntervalFunction_t swap_interval;
    MSG                       message;

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
    window_class.hIcon         = ::LoadIconW(window_class.hInstance, MAKEINTRESOURCEW(ICON_RESOURCE_ID));
    window_class.hIconSm       = window_class.hIcon;
    LOGASSERT(0 != ::RegisterClassExW(&window_class));
    (void)::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    // Centre the window on the primary monitor's work area, sized to fit it
    monitor_info.cbSize = sizeof(monitor_info);
    monitor_rect.left   = 0;
    monitor_rect.top    = 0;
    monitor_rect.right  = ::GetSystemMetrics(SM_CXSCREEN);
    monitor_rect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
    if (::GetMonitorInfoW(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
    {
        monitor_rect = monitor_info.rcWork;
    }
    window_width  = ImMin((int)(WINDOW_DEFAULT_WIDTH * window->mainScale), (int)(monitor_rect.right - monitor_rect.left));
    window_height = ImMin((int)(WINDOW_DEFAULT_HEIGHT * window->mainScale), (int)(monitor_rect.bottom - monitor_rect.top));
    window->windowHandle = ::CreateWindowW(
        window_class.lpszClassName,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        (int)monitor_rect.left + (((int)(monitor_rect.right - monitor_rect.left) - window_width) / 2),
        (int)monitor_rect.top + (((int)(monitor_rect.bottom - monitor_rect.top) - window_height) / 2),
        window_width,
        window_height,
        NULL,
        NULL,
        window_class.hInstance,
        window);
    LOGASSERT(NULL != window->windowHandle);
    ::DragAcceptFiles(window->windowHandle, TRUE);
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
    if (NULL == g_screenshotPath)
    {
        // Pop-ups in their own OS windows cannot be read back from the main framebuffer, so
        // automation runs keep every window inside the main viewport
        io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    LoadPreferences(window);
    LoadUpdateSource(window);
    ApplyTheme(window);
    (void)Arinc661_Icons_LoadFonts(UI_TEXT_SIZE);
    style = &ImGui::GetStyle();
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
    window->definitionEditor = (Arinc661_DefinitionEditor_t*)OS_CALLOC(sizeof(Arinc661_DefinitionEditor_t));
    Arinc661_Definition_Initialize(window->definitionEditor);
    window->isHelpOpen    = FALSE;
    window->helpFilter[0] = 0;
    strncpy_s(window->helpTopic, sizeof(window->helpTopic), HELP_GETTING_STARTED, _TRUNCATE);
    window->isHelpOpen    = FALSE;
    window->saveNote[0]   = 0;
    window->hasLayout     = FALSE;
    window->resetsLayout  = FALSE;
    window->layoutPreset  = LAYOUT_PRESET_DEFAULT;
    window->paletteFilter[0] = 0;
    window->logCount      = 0;
    window->lastStatus[0] = 0;
    window->logAutoScroll = true;
    window->isTimelinePlaying = FALSE;
    window->isTimelineLooping = TRUE;
    window->timelineTime      = 0.0f;
    window->timelineLength    = 0.0f;
    window->isPaletteOpen     = FALSE;
    window->commandFilter[0]  = 0;
    window->logFilter[0]      = 0;
    window->pendingOpenPath[0] = 0;
    window->updateState.store(UPDATE_STATE_IDLE);
    window->updateResult       = ARINC661_UPDATE_RESULT_UP_TO_DATE;
    window->updatePackagePath[0] = 0;
    window->updateIsManual     = FALSE;
    window->opensUpdatePopup   = FALSE;
    window->opensAboutPopup    = FALSE;
    window->isApplyingUpdate   = FALSE;
    window->dialogState.store(DIALOG_IDLE);
    window->dialogIsFolder    = FALSE;
    window->dialogPath[0]     = 0;
    window->opensPrefabPopup  = FALSE;
    window->prefabName[0]     = 0;
    window->prefabCount       = 0;
    window->logShowsErrors    = TRUE;
    window->logShowsWarnings  = TRUE;
    window->logShowsInfo      = TRUE;
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
        Arinc661_DefinitionEditor_LoadFile(window->definitionEditor, g_openDefinition);
        if (NULL != g_selectName)
        {
            Arinc661_DefinitionEditor_SelectByName(window->definitionEditor, g_selectName);
        }
    }
    window->isDefinitionActive = g_focusDefinition;
    if ((0 <= g_layoutPreset) && (g_layoutPreset < (int32_t)COUNT_OF(c_layoutNames)))
    {
        window->layoutPreset = (LayoutPreset_t)g_layoutPreset;
        window->resetsLayout = TRUE;
    }
    if (NULL != g_openPath)
    {
        (void)OpenPath(window, g_openPath);
    }
    if (0.0f <= g_startTime)
    {
        window->timelineTime = g_startTime;
        Arinc661_DefinitionEditor_SetAnimationTime(window->definitionEditor, g_startTime);
    }
    if (g_startsPlaying)
    {
        TogglePlay(window);
    }
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
    float32_t                        main_scale;
    WindowThreadState_t              window;
    HANDLE                           window_thread;
    HDC                              present_device_context;
    D3DKMT_WAITFORVERTICALBLANKEVENT vbe;

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
