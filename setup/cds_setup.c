/*=================================================================================================
** 1. References
**
** Windows Installer external UI: MsiSetInternalUI, MsiSetExternalUIRecord, MsiInstallProduct,
**   "Handling Progress Messages Using MsiSetExternalUI".
** Raymond Chen, The Old New Thing: the dialog manager series; modeless child dialogs as pages
**   of a frame (DS_CONTROL).
** Dark mode: DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE), SetWindowTheme
**   "DarkMode_Explorer", WM_CTLCOLOR* for the text and backgrounds.
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#define COBJMACROS

#include "resource.h"
#include "../examples/arinc661_version.h"

#include "faceoe_utl/faceoe_utl_defines.h"
#include "os/os_types.h"

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <msi.h>
#include <msiquery.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define UPGRADE_CODE              L"{01ADB398-6743-45F5-B841-75BDF9612E7B}"
#define EDITOR_EXECUTABLE         L"cds-editor.exe"
#define DEFAULT_FOLDER_SUFFIX     L"\\CDS Editor"
#define PACKAGE_FILE_FORMAT       L"%scds-editor-setup-%lu.msi"
#define ELEVATED_SWITCH           L"/elevated"
#define THEME_LIGHT_SWITCH        L"/light"
#define THEME_DARK_SWITCH         L"/dark"
#define MODE_INSTALL              L"install"
#define MODE_REPAIR               L"repair"
#define MODE_REMOVE               L"remove"
#define OPTION_STARTMENU          (1u << 0)
#define OPTION_DESKTOP            (1u << 1)
#define OPTION_OPENWITH           (1u << 2)
#define OPTION_SHELL              (1u << 3)
#define OPTION_LAUNCH             (1u << 4)
#define OPTION_DEFAULTS           (OPTION_STARTMENU | OPTION_OPENWITH | OPTION_SHELL | OPTION_LAUNCH)
#define DWMWA_IMMERSIVE_DARK_MODE (20)
#define DWMWA_IMMERSIVE_DARK_MODE_OLD (19)
#define BUILD_DARK_MODE           (17763) /* Windows 10 1809: dark title bar (attribute 19) and DarkMode_Explorer */
#define BUILD_DARK_MODE_ATTRIBUTE (19041) /* Windows 10 2004: the documented attribute 20 */
#define BUILD_DARK_MODE_APP_MODE  (18362) /* Windows 10 1903: uxtheme 135 takes the PreferredAppMode enum */
#define UXTHEME_ORDINAL_APP_MODE  (135)
#define UXTHEME_ORDINAL_FLUSH_MENUS (136)
#define APP_MODE_FORCE_DARK       (2)
#define APP_MODE_DEFAULT          (0)
#define THEME_DARK_CONTROLS       L"DarkMode_Explorer"
#define THEME_LIGHT_CONTROLS      L"Explorer"
#define THEME_CHANGED_SECTION     L"ImmersiveColorSet"
#define THEMES_KEY                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"
#define THEMES_LIGHT_VALUE        L"AppsUseLightTheme"
#define COLOR_DARK_PAGE           RGB(32, 32, 32)
#define COLOR_DARK_WELL           RGB(43, 43, 43)
#define COLOR_DARK_TEXT           RGB(235, 235, 235)
#define COLOR_DARK_MUTED          RGB(160, 160, 160)
#define COLOR_LIGHT_MUTED         RGB(96, 96, 96)
#define COLOR_DARK_FRAME          RGB(84, 84, 84)
#define COLOR_DARK_LINE           RGB(64, 64, 64)
#define COLOR_DARK_HEADER         RGB(48, 48, 48)
#define THEME_DARK_HEADER         L"DarkMode_ItemsView"
#define HEADER_SUBCLASS_ID        (1)
#define CHECK_BOX_GAP             (3)
#define ANCHOR_LEFT               (1u << 0)
#define ANCHOR_TOP                (1u << 1)
#define ANCHOR_RIGHT              (1u << 2)
#define ANCHOR_BOTTOM             (1u << 3)
#define ANCHOR_ALL                (ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT | ANCHOR_BOTTOM)
#define PROP_ORIGIN               L"CdsSetupOrigin"
#define PROP_SIZE                 L"CdsSetupSize"
#define PROP_CLIENT               L"CdsSetupClient"
#define HEADLINE_POINTS           (11)
#define WM_APP_PROGRESS           (WM_APP + 1)
#define WM_APP_ACTION             (WM_APP + 2)
#define WM_APP_DONE               (WM_APP + 3)
#define PROGRESS_RANGE            (1000)
#define MAX_FILE_ROWS             (64)
#define MAX_TEXT                  (512)
#define MAX_PROPERTIES            (1024)
#define FILE_QUERY                L"SELECT `FileName`, `FileSize`, `Directory_` FROM `File`, `Component` WHERE `File`.`Component_` = `Component`.`Component`"
#define SUMMARY_FOLDER_FORMAT     L"Destination location:\r\n      %s\r\n"
#define SUMMARY_TASKS_HEADING     L"\r\nAdditional tasks:\r\n"
#define SUMMARY_SHORTCUTS_HEADING L"      Additional shortcuts:\r\n"
#define SUMMARY_ASSOCIATIONS_HEADING L"      Windows Explorer:\r\n"
#define SUMMARY_ITEM_PREFIX       L"            "
#define TREE_FILE_FORMAT          L"%s  (%s)"
#define TREE_EXAMPLES_NAME        L"examples"
#define TREE_SHORTCUT_NAME        L"CDS Editor"
#define TREE_REGISTRY_NAME        L"Registry"
#define WHERE_PROGRAM             L"Destination folder"
#define WHERE_EXAMPLES            L"examples\\"
#define WHERE_STARTMENU           L"Start menu"
#define WHERE_DESKTOP             L"Desktop"
#define WHERE_REGISTRY            L"Registry"
#define ROW_OPENWITH              L"Open With list (.xml, .bin)"
#define ROW_SHELL                 L"Explorer icons and menu command"
#define GROUP_FILES_FORMAT        L"Files to be installed (%u files, %s)"
#define NEXT_LABEL                L"&Next >"
#define INSTALL_LABEL             L"&Install"
#define REPAIR_LABEL              L"&Repair"
#define REMOVE_LABEL              L"&Remove"
#define FINISH_LABEL              L"&Finish"
#define CANCEL_LABEL              L"Cancel"
#define CLOSE_LABEL               L"Close"
#define TITLE_REPAIRING           L"Repairing CDS Editor"
#define TITLE_REMOVING            L"Removing CDS Editor"
#define DESCRIPTION_REPAIRING     L"Please wait while the Setup Wizard repairs CDS Editor."
#define DESCRIPTION_REMOVING      L"Please wait while the Setup Wizard removes CDS Editor."
#define HEADLINE_REMOVED          L"Completed the CDS Editor Setup Wizard"
#define BODY_REMOVED              L"Click the Finish button to exit the Setup Wizard."
#define HEADLINE_FAILED           L"CDS Editor Setup Wizard ended prematurely"
#define BODY_FAILED_FORMAT        L"CDS Editor Setup Wizard ended prematurely because of an error (%lu). Your system has not been modified. To install this program at a later time, run Setup Wizard again.\r\n\r\nClick the Finish button to exit the Setup Wizard."
#define HEADLINE_CANCELLED        L"CDS Editor Setup Wizard was interrupted"
#define BODY_CANCELLED            L"CDS Editor setup was interrupted. Your system has not been modified. To install this program at a later time, run Setup Wizard again.\r\n\r\nClick the Finish button to exit the Setup Wizard."
#define ASK_CANCEL_TEXT           L"Are you sure you want to cancel CDS Editor installation?"
#define ASK_CANCEL_TITLE          L"CDS Editor Setup"
#define ELEVATE_FAILED_TEXT       L"Setup needs permission to make changes to this computer."
#define VERSION_FORMAT            L"Version %S"
#define OPTION_YES_STARTMENU      L"Create a Start menu shortcut\r\n"
#define OPTION_YES_DESKTOP        L"Create a desktop shortcut\r\n"
#define OPTION_YES_OPENWITH       L"Add CDS Editor to the Open With list of all .xml and .bin files\r\n"
#define OPTION_YES_SHELL          L"Show icons for Definition Files and configurations in Explorer\r\n"
#define PROPERTIES_FORMAT         L"INSTALLFOLDER=\"%s\" ADDLOCAL=%s REBOOT=ReallySuppress"
#define FEATURES_MAIN             L"Main"
#define FEATURE_STARTMENU         L",StartMenu"
#define FEATURE_DESKTOP           L",Desktop"
#define FEATURE_OPENWITH          L",OpenWith"
#define FEATURE_SHELL             L",ShellExt"

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/** Pages of the wizard, in install order; maintenance has its own path. */
typedef enum Page_e
{
    PAGE_WELCOME,
    PAGE_LICENSE,
    PAGE_OPTIONS,
    PAGE_FOLDER,
    PAGE_READY,
    PAGE_PROGRESS,
    PAGE_FINISH,
    PAGE_MAINTENANCE,
    PAGE_COUNT
} Page_t;

/** What the wizard is doing. */
typedef enum Mode_e
{
    MODE_INSTALLING,
    MODE_REPAIRING,
    MODE_REMOVING
} Mode_t;

/** How a control follows its window's size. */
typedef struct Anchor_s
{
    int      id;      ///< Control
    uint32_t anchors; ///< ANCHOR_* bits: a set edge keeps its distance from that side
} Anchor_t;

/** One row of the file grid. */
typedef struct FileRow_s
{
    wchar_t name[MAX_TEXT];  ///< Shown name
    wchar_t size[32];        ///< Size text, or a dash
    wchar_t where[MAX_TEXT]; ///< Destination text
} FileRow_t;

/** Everything the wizard remembers between pages. */
typedef struct Setup_s
{
    HINSTANCE module;                      ///< This program
    HWND      frame;                       ///< The frame dialog
    HWND      page;                        ///< The current page dialog
    Page_t    pageId;                      ///< Which page is up
    Mode_t    mode;                        ///< Install, repair, or remove
    uint32_t  build;                       ///< Windows build number
    uint32_t  dpi;                         ///< Dots per inch of the frame's monitor
    bool_t    isDark;                      ///< TRUE when apps use the dark theme and Windows can draw it
    bool_t    isElevated;                  ///< TRUE when running as administrator
    bool_t    isInstalled;                 ///< TRUE when a copy is registered
    bool_t    isAccepted;                  ///< TRUE once the license is accepted
    bool_t    isCancelRequested;           ///< TRUE while the installer should stop
    bool_t    isDone;                      ///< TRUE once the installer finished
    uint32_t  options;                     ///< OPTION_* bits
    uint32_t  result;                      ///< Windows Installer result
    wchar_t   folder[MAX_PATH];            ///< Destination folder
    wchar_t   packagePath[MAX_PATH];       ///< Extracted MSI
    wchar_t   productCode[64];             ///< Installed product, when any
    FileRow_t rows[MAX_FILE_ROWS];         ///< File rows
    HTREEITEM folderItem;                  ///< Tree node of the destination folder, renamed as the folder box changes
    SIZE      minimumFrame;                ///< Frame window size as the template made it
    uint32_t  rowCount;                    ///< Rows filled
    uint32_t  fileCount;                   ///< Rows that are files
    uint64_t  totalBytes;                  ///< Bytes of the files
    HICON     icon;                        ///< Product icon, drawn into the banner and the side panel
    HBRUSH    pageBrush;                   ///< Page background in dark mode
    HBRUSH    wellBrush;                   ///< Edit and list background in dark mode
    HFONT     headlineFont;                ///< Large page headline
    HFONT     textFont;                    ///< The dialog font, for painting the header and group captions
    HFONT     boldFont;                    ///< Section headings of the options page
    HANDLE    thread;                      ///< Installer thread
    int32_t   progressTotal;               ///< Ticks the installer announced
    int32_t   progressDone;                ///< Ticks reported so far
    int32_t   progressStep;                ///< Ticks per action data message
    bool_t    progressForward;             ///< TRUE while progress counts up
    bool_t    progressEnabled;             ///< TRUE while action data advances the bar
} Setup_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

static Setup_t s_setup;

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Reads the elevated relaunch arguments, when present.
 *
 * \param[in] commandLine   Command line
 *
 * \retval    TRUE          if the process was relaunched elevated with a job to do.
 * \retval    FALSE         otherwise.
 */
static bool_t ParseElevatedArguments(const wchar_t* commandLine);

/*************************************************************************************************/
/**
 * \brief Tells whether the process runs with administrator rights.
 *
 * \retval TRUE    if elevated.
 * \retval FALSE   otherwise.
 */
static bool_t IsElevated(void);

/*************************************************************************************************/
/**
 * \brief Reads the Windows build number.
 *
 * \par Details:
 *  RtlGetVersion returns what GetVersionEx would without the compatibility shim that caps the
 *  version for programs without a supportedOS entry, so the answer is the running system.
 *
 * \retval uint32_t   Build number, zero when unknown.
 */
static uint32_t ReadBuildNumber(void);

/*************************************************************************************************/
/**
 * \brief Reads the user's app theme.
 *
 * \retval TRUE    when apps use the dark theme.
 * \retval FALSE   otherwise.
 */
static bool_t ReadsDarkTheme(void);
static void ApplyMenuTheme(void);
static void ApplyTheme(bool_t isDark);

/*************************************************************************************************/
/**
 * \brief Writes the embedded package to the temporary folder.
 *
 * \retval TRUE    if the file was written.
 * \retval FALSE   otherwise.
 */
static bool_t ExtractPackage(void);

/*************************************************************************************************/
/**
 * \brief Finds an installed copy by the upgrade code.
 *
 * \retval None
 */
static void FindInstalledProduct(void);

/*************************************************************************************************/
/**
 * \brief Reads the package's File table into the grid rows.
 *
 * \retval None
 */
static void ReadFileRows(void);

/*************************************************************************************************/
/**
 * \brief Formats a byte count as B, KB, or MB.
 *
 * \param[in]  bytes      Byte count
 * \param[out] text       Text written
 * \param[in]  capacity   Characters in text
 *
 * \retval     None
 */
static void FormatSize(uint64_t bytes, wchar_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Frame dialog procedure: navigation, theme colours, installer messages.
 */
static INT_PTR CALLBACK FrameProcedure(HWND window, UINT message, WPARAM wordParameter, LPARAM longParameter);

/*************************************************************************************************/
/**
 * \brief Page dialog procedure shared by every page; the page is told apart by its resource id.
 */
static INT_PTR CALLBACK PageProcedure(HWND window, UINT message, WPARAM wordParameter, LPARAM longParameter);

/*************************************************************************************************/
/**
 * \brief Remembers every child's place and the client size, for the layout on resize.
 *
 * \param[in] window   Frame or page
 *
 * \retval    None
 */
static void RememberLayout(HWND window);

/*************************************************************************************************/
/**
 * \brief Moves and sizes the children after the window changed size, by their anchors.
 *
 * \param[in] window   Frame or page
 *
 * \retval    None
 */
static void ApplyLayout(HWND window);

/*************************************************************************************************/
/**
 * \brief Looks up a control's anchors.
 *
 * \param[in] id   Control
 *
 * \retval    uint32_t   ANCHOR_* bits; left and top when the control is not listed.
 */
static uint32_t AnchorsOf(int id);

/*************************************************************************************************/
/**
 * \brief Replaces the current page with another.
 *
 * \param[in] page   Page to show
 *
 * \retval    None
 */
static void ShowPage(Page_t page);

/*************************************************************************************************/
/**
 * \brief Sets the frame's buttons for the current page.
 *
 * \retval None
 */
static void UpdateButtons(void);

/*************************************************************************************************/
/**
 * \brief Applies the dark theme to a page's controls when the user runs dark.
 *
 * \param[in] page   Page dialog
 *
 * \retval    None
 */
static void ThemePage(HWND page);

/*************************************************************************************************/
/**
 * \brief Answers WM_CTLCOLOR* for dark pages.
 *
 * \param[in] message         WM_CTLCOLOR* message
 * \param[in] deviceContext   Control's device context
 * \param[in] control         Control
 *
 * \retval    INT_PTR         Brush to paint with, or zero to let the system paint.
 */
static INT_PTR ControlColor(UINT message, HDC deviceContext, HWND control);

/*************************************************************************************************/
/**
 * \brief Paints the chrome the system would draw light: group frames, etched lines, and the
 *  frames of edits and lists, for a dark page.
 *
 * \param[in] window          Page or frame
 * \param[in] deviceContext   Target
 *
 * \retval    None
 */
static void PaintDarkChrome(HWND window, HDC deviceContext);

/*************************************************************************************************/
/**
 * \brief Repaints a themed check box's label in the dark text colour after the theme drew it.
 *
 * \param[in] draw   Custom draw notification of the check box
 *
 * \retval    LRESULT   Custom draw answer.
 */
static LRESULT PaintCheckBoxLabel(const NMCUSTOMDRAW* draw);

/*************************************************************************************************/
/**
 * \brief Subclass of the list header: paints it dark with light column names.
 */
static LRESULT CALLBACK HeaderProcedure(HWND window, UINT message, WPARAM wordParameter, LPARAM longParameter, UINT_PTR subclassId, DWORD_PTR reference);

/*************************************************************************************************/
/**
 * \brief Creates the fonts for the frame's DPI.
 *
 * \param[in] frame   The frame
 *
 * \retval    None
 */
static void CreateFonts(HWND frame);

/*************************************************************************************************/
/**
 * \brief Draws a banner or side image stretched into its control.
 *
 * \param[in] item   Draw item of an owner-drawn static
 *
 * \retval    None
 */
static void DrawImage(const DRAWITEMSTRUCT* item);

/*************************************************************************************************/
/**
 * \brief Fills the file tree of the folder page: the destination folder and its files, the
 *  shortcuts, and the registry entries of the chosen tasks.
 *
 * \param[in] page   Folder page
 *
 * \retval    None
 */
static void FillFileTree(HWND page);

/*************************************************************************************************/
/**
 * \brief Adds a node to the file tree.
 *
 * \param[in] tree     Tree view
 * \param[in] parent   Parent node, or TVI_ROOT
 * \param[in] text     Node text
 * \param[in] icon     System image list index
 *
 * \retval    HTREEITEM   The node.
 */
static HTREEITEM AddTreeNode(HWND tree, HTREEITEM parent, const wchar_t* text, int icon);

/*************************************************************************************************/
/**
 * \brief Looks up the shell icon of a file name or a folder in the system image list.
 *
 * \param[in] name       File name, used for its extension
 * \param[in] isFolder   TRUE for a folder icon
 *
 * \retval    int        Image index, or -1.
 */
static int ShellIconIndex(const wchar_t* name, bool_t isFolder);

/*************************************************************************************************/
/**
 * \brief Fills the summary of the ready page.
 *
 * \param[in] page   Ready page
 *
 * \retval    None
 */
static void FillSummary(HWND page);

/*************************************************************************************************/
/**
 * \brief Shows the folder picker and takes the choice.
 *
 * \param[in] page   Folder page
 *
 * \retval    None
 */
static void BrowseFolder(HWND page);

/*************************************************************************************************/
/**
 * \brief Starts the installer, elevating through a relaunch when needed.
 *
 * \retval None
 */
static void BeginInstall(void);

/*************************************************************************************************/
/**
 * \brief Installer thread: runs Windows Installer on the package.
 *
 * \param[in] parameter   Unused
 *
 * \retval    DWORD       Zero.
 */
static DWORD WINAPI InstallThread(LPVOID parameter);

/*************************************************************************************************/
/**
 * \brief Windows Installer external UI handler: progress and actions to the frame.
 */
static INT CALLBACK InstallerHandler(LPVOID context, UINT messageType, MSIHANDLE record);

/*************************************************************************************************/
/**
 * \brief Starts the installed editor without elevation.
 *
 * \retval None
 */
static void LaunchEditor(void);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  /elevated <install|repair|remove> "<folder>" <options>
 */
static bool_t ParseElevatedArguments(const wchar_t* commandLine)
{
    wchar_t** arguments;
    int       count;
    bool_t    is_elevated_run;

    is_elevated_run = FALSE;
    arguments       = CommandLineToArgvW(commandLine, &count);
    if (NULL == arguments)
    {
        return FALSE;
    }
    if ((5 <= count) && (0 == wcscmp(arguments[1], ELEVATED_SWITCH)))
    {
        is_elevated_run = TRUE;
        s_setup.mode    = MODE_INSTALLING;
        if (0 == wcscmp(arguments[2], MODE_REPAIR))
        {
            s_setup.mode = MODE_REPAIRING;
        }
        else if (0 == wcscmp(arguments[2], MODE_REMOVE))
        {
            s_setup.mode = MODE_REMOVING;
        }
        (void)wcsncpy_s(s_setup.folder, COUNT_OF(s_setup.folder), arguments[3], _TRUNCATE);
        s_setup.options = (uint32_t)wcstoul(arguments[4], NULL, 10);
    }
    LocalFree(arguments);

    return is_elevated_run;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t IsElevated(void)
{
    HANDLE          token;
    TOKEN_ELEVATION elevation;
    DWORD           size;
    bool_t          is_elevated;

    is_elevated = FALSE;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        size = sizeof(elevation);
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            is_elevated = (bool_t)(0 != elevation.TokenIsElevated);
        }
        CloseHandle(token);
    }

    return is_elevated;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static uint32_t ReadBuildNumber(void)
{
    typedef LONG(WINAPI * RtlGetVersion_t)(RTL_OSVERSIONINFOW*);
    RTL_OSVERSIONINFOW version;
    RtlGetVersion_t    get_version;
    HMODULE            ntdll;

    ntdll = GetModuleHandleW(L"ntdll.dll");
    if (NULL == ntdll)
    {
        return 0;
    }
    get_version = (RtlGetVersion_t)(void*)GetProcAddress(ntdll, "RtlGetVersion");
    if (NULL == get_version)
    {
        return 0;
    }
    (void)memset(&version, 0, sizeof(version));
    version.dwOSVersionInfoSize = sizeof(version);
    if (0 != get_version(&version))
    {
        return 0;
    }

    return (uint32_t)version.dwBuildNumber;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A missing value means light.
 */
static bool_t ReadsDarkTheme(void)
{
    HKEY  key;
    DWORD value;
    DWORD size;
    DWORD type;
    bool_t is_dark;

    is_dark = FALSE;
    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, THEMES_KEY, 0, KEY_READ, &key))
    {
        size = sizeof(value);
        if ((ERROR_SUCCESS == RegQueryValueExW(key, THEMES_LIGHT_VALUE, NULL, &type, (BYTE*)&value, &size)) && (REG_DWORD == type))
        {
            is_dark = (bool_t)(0 == value);
        }
        RegCloseKey(key);
    }

    return is_dark;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The system menu and other popup menus follow an undocumented per-process preference in
 *  uxtheme: ordinal 135 is AllowDarkModeForApp(BOOL) on 1809 and SetPreferredAppMode(enum) from
 *  1903, where ForceDark is 2; ordinal 136, FlushMenuThemes, applies it. Must run before the
 *  frame is created, and again on a theme change. Missing exports leave the menus light.
 */
static void ApplyMenuTheme(void)
{
    typedef int(WINAPI * SetPreferredAppMode_t)(int);
    typedef void(WINAPI * FlushMenuThemes_t)(void);
    HMODULE                uxtheme;
    SetPreferredAppMode_t  set_mode;
    FlushMenuThemes_t      flush;
    int                    mode;

    uxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (NULL == uxtheme)
    {
        return;
    }
    set_mode = (SetPreferredAppMode_t)(void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_ORDINAL_APP_MODE));
    flush    = (FlushMenuThemes_t)(void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_ORDINAL_FLUSH_MENUS));
    if ((NULL != set_mode) && (NULL != flush))
    {
        // Both ABI variants read light as zero
        mode = APP_MODE_DEFAULT;
        if (s_setup.isDark && (BUILD_DARK_MODE_APP_MODE <= s_setup.build))
        {
            mode = APP_MODE_FORCE_DARK;
        }
        else if (s_setup.isDark)
        {
            mode = TRUE;
        }
        (void)set_mode(mode);
        flush();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Everything of the frame that depends on the theme: the brushes, the title bar (asked of DWM
 *  by the attribute number of this build), the wizard buttons, the popup menus, and the etched
 *  line that dark mode paints itself. The page is not touched; callers recreate it.
 */
static void ApplyTheme(bool_t isDark)
{
    const wchar_t* controls;
    HWND           frame;
    BOOL           on;
    int            line_show;

    frame          = s_setup.frame;
    s_setup.isDark = isDark;
    if (NULL != s_setup.pageBrush) { DeleteObject(s_setup.pageBrush); s_setup.pageBrush = NULL; }
    if (NULL != s_setup.wellBrush) { DeleteObject(s_setup.wellBrush); s_setup.wellBrush = NULL; }
    controls  = THEME_LIGHT_CONTROLS;
    line_show = SW_SHOW;
    on        = FALSE;
    if (isDark)
    {
        s_setup.pageBrush = CreateSolidBrush(COLOR_DARK_PAGE);
        s_setup.wellBrush = CreateSolidBrush(COLOR_DARK_WELL);
        controls          = THEME_DARK_CONTROLS;
        line_show         = SW_HIDE;
        on                = TRUE;
    }
    ApplyMenuTheme();
    // The attribute number moved between Windows 10 builds
    if (BUILD_DARK_MODE_ATTRIBUTE <= s_setup.build)
    {
        (void)DwmSetWindowAttribute(frame, DWMWA_IMMERSIVE_DARK_MODE, &on, sizeof(on));
    }
    else
    {
        (void)DwmSetWindowAttribute(frame, DWMWA_IMMERSIVE_DARK_MODE_OLD, &on, sizeof(on));
    }
    (void)SetWindowTheme(GetDlgItem(frame, IDC_BACK), controls, NULL);
    (void)SetWindowTheme(GetDlgItem(frame, IDC_NEXT), controls, NULL);
    (void)SetWindowTheme(GetDlgItem(frame, IDCANCEL), controls, NULL);
    ShowWindow(GetDlgItem(frame, IDC_BOTTOM_LINE), line_show);
    RedrawWindow(frame, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ExtractPackage(void)
{
    HRSRC   resource;
    HGLOBAL loaded;
    void*   bytes;
    DWORD   byte_count;
    DWORD   written;
    HANDLE  file;
    wchar_t temp_folder[MAX_PATH];
    bool_t  is_written;

    is_written = FALSE;
    resource   = FindResourceW(s_setup.module, MAKEINTRESOURCEW(IDR_PACKAGE), RT_RCDATA);
    if (NULL == resource)
    {
        return FALSE;
    }
    loaded     = LoadResource(s_setup.module, resource);
    bytes      = LockResource(loaded);
    byte_count = SizeofResource(s_setup.module, resource);
    if ((NULL == bytes) || (0 == byte_count) || (0 == GetTempPathW(COUNT_OF(temp_folder), temp_folder)))
    {
        return FALSE;
    }
    (void)swprintf_s(s_setup.packagePath, COUNT_OF(s_setup.packagePath), PACKAGE_FILE_FORMAT, temp_folder, (unsigned long)GetCurrentProcessId());
    file = CreateFileW(s_setup.packagePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (INVALID_HANDLE_VALUE != file)
    {
        written    = 0;
        is_written = (bool_t)(WriteFile(file, bytes, byte_count, &written, NULL) && (written == byte_count));
        CloseHandle(file);
    }

    return is_written;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The first related product is the installed copy; the setup never installs two.
 */
static void FindInstalledProduct(void)
{
    s_setup.isInstalled    = FALSE;
    s_setup.productCode[0] = L'\0';
    if (ERROR_SUCCESS == MsiEnumRelatedProductsW(UPGRADE_CODE, 0, 0, s_setup.productCode))
    {
        s_setup.isInstalled = TRUE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The long name follows the bar of a short|long file name; the directory id names the place.
 */
static void ReadFileRows(void)
{
    MSIHANDLE database;
    MSIHANDLE view;
    MSIHANDLE record;
    wchar_t   name[MAX_TEXT];
    wchar_t   directory[64];
    wchar_t*  bar;
    DWORD     length;
    int       size;

    s_setup.rowCount   = 0;
    s_setup.fileCount  = 0;
    s_setup.totalBytes = 0;
    database           = 0;
    if (ERROR_SUCCESS != MsiOpenDatabaseW(s_setup.packagePath, MSIDBOPEN_READONLY, &database))
    {
        return;
    }
    view = 0;
    if ((ERROR_SUCCESS == MsiDatabaseOpenViewW(database, FILE_QUERY, &view)) && (ERROR_SUCCESS == MsiViewExecute(view, 0)))
    {
        while ((s_setup.rowCount < MAX_FILE_ROWS) && (ERROR_SUCCESS == MsiViewFetch(view, &record)))
        {
            length = COUNT_OF(name);
            (void)MsiRecordGetStringW(record, 1, name, &length);
            size   = MsiRecordGetInteger(record, 2);
            length = COUNT_OF(directory);
            (void)MsiRecordGetStringW(record, 3, directory, &length);
            MsiCloseHandle(record);
            bar = wcschr(name, L'|');
            if (NULL == bar)
            {
                bar = name;
            }
            else
            {
                bar += 1;
            }
            (void)wcsncpy_s(s_setup.rows[s_setup.rowCount].name, MAX_TEXT, bar, _TRUNCATE);
            FormatSize((uint64_t)size, s_setup.rows[s_setup.rowCount].size, COUNT_OF(s_setup.rows[s_setup.rowCount].size));
            if (0 == wcscmp(directory, L"ExamplesFolder"))
            {
                (void)wcsncpy_s(s_setup.rows[s_setup.rowCount].where, MAX_TEXT, WHERE_EXAMPLES, _TRUNCATE);
            }
            else
            {
                (void)wcsncpy_s(s_setup.rows[s_setup.rowCount].where, MAX_TEXT, WHERE_PROGRAM, _TRUNCATE);
            }
            s_setup.totalBytes += (uint64_t)size;
            s_setup.rowCount   += 1;
            s_setup.fileCount  += 1;
        }
        MsiCloseHandle(view);
    }
    MsiCloseHandle(database);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void FormatSize(uint64_t bytes, wchar_t* text, uint32_t capacity)
{
    if (bytes < 1024)
    {
        (void)swprintf_s(text, capacity, L"%llu B", (unsigned long long)bytes);
    }
    else if (bytes < (1024 * 1024))
    {
        (void)swprintf_s(text, capacity, L"%.0f KB", (double)bytes / 1024.0);
    }
    else
    {
        (void)swprintf_s(text, capacity, L"%.1f MB", (double)bytes / (1024.0 * 1024.0));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A theme switch in Settings arrives as WM_SETTINGCHANGE for ImmersiveColorSet; the frame is
 *  rethemed in place and the page recreated, after saving a folder being typed.
 */
static INT_PTR CALLBACK FrameProcedure(HWND window, UINT message, WPARAM wordParameter, LPARAM longParameter)
{
    PAINTSTRUCT paint;
    HDC         paint_context;
    RECT        frame_area;
    bool_t      is_dark;
    int32_t     percent;
    wchar_t*    action;

    switch (message)
    {
    case WM_INITDIALOG:
        s_setup.frame = window;
        CreateFonts(window);
        RememberLayout(window);
        GetWindowRect(window, &frame_area);
        s_setup.minimumFrame.cx = (frame_area.right - frame_area.left);
        s_setup.minimumFrame.cy = (frame_area.bottom - frame_area.top);
        // Focus cues appear once the keyboard is used, as in every other dialog
        SendMessageW(window, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
        SendMessageW(window, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(s_setup.module, MAKEINTRESOURCEW(IDI_SETUP)));
        SendMessageW(window, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconW(s_setup.module, MAKEINTRESOURCEW(IDI_SETUP)));
        ApplyTheme(s_setup.isDark);
        if (s_setup.isElevated && (s_setup.isInstalled || (MODE_INSTALLING == s_setup.mode)) && (0 != s_setup.options) && (L'\0' != s_setup.folder[0]))
        {
            // Relaunched elevated with a job: straight to the progress page
            ShowPage(PAGE_PROGRESS);
            BeginInstall();
        }
        else if (s_setup.isInstalled)
        {
            ShowPage(PAGE_MAINTENANCE);
        }
        else
        {
            ShowPage(PAGE_WELCOME);
        }
        return TRUE;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        return ControlColor(message, (HDC)wordParameter, (HWND)longParameter);
    case WM_PAINT:
        if (s_setup.isDark)
        {
            paint_context = BeginPaint(window, &paint);
            PaintDarkChrome(window, paint_context);
            EndPaint(window, &paint);
            return TRUE;
        }
        break;
    case WM_GETMINMAXINFO:
        if (0 < s_setup.minimumFrame.cx)
        {
            ((MINMAXINFO*)longParameter)->ptMinTrackSize.x = s_setup.minimumFrame.cx;
            ((MINMAXINFO*)longParameter)->ptMinTrackSize.y = s_setup.minimumFrame.cy;
        }
        return TRUE;
    case WM_SIZE:
        ApplyLayout(window);
        if (NULL != s_setup.page)
        {
            GetWindowRect(GetDlgItem(window, IDC_PAGE_AREA), &frame_area);
            MapWindowPoints(NULL, window, (POINT*)&frame_area, 2);
            SetWindowPos(s_setup.page, NULL, frame_area.left, frame_area.top, frame_area.right - frame_area.left, frame_area.bottom - frame_area.top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wordParameter))
        {
        case IDC_BACK:
            if (PAGE_WELCOME < s_setup.pageId && s_setup.pageId <= PAGE_READY)
            {
                ShowPage((Page_t)(s_setup.pageId - 1));
            }
            return TRUE;
        case IDC_NEXT:
            switch (s_setup.pageId)
            {
            case PAGE_WELCOME:
            case PAGE_LICENSE:
            case PAGE_OPTIONS:
                ShowPage((Page_t)(s_setup.pageId + 1));
                break;
            case PAGE_FOLDER:
                GetDlgItemTextW(s_setup.page, IDC_FOLDER, s_setup.folder, COUNT_OF(s_setup.folder));
                PathRemoveBackslashW(s_setup.folder);
                ShowPage(PAGE_READY);
                break;
            case PAGE_READY:
            case PAGE_MAINTENANCE:
                BeginInstall();
                break;
            case PAGE_FINISH:
                if ((MODE_INSTALLING == s_setup.mode) && (ERROR_SUCCESS == s_setup.result) && (0 != (s_setup.options & OPTION_LAUNCH)))
                {
                    LaunchEditor();
                }
                EndDialog(window, 0);
                break;
            default:
                break;
            }
            return TRUE;
        case IDCANCEL:
            if (PAGE_PROGRESS == s_setup.pageId)
            {
                if (IDYES == MessageBoxW(window, ASK_CANCEL_TEXT, ASK_CANCEL_TITLE, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
                {
                    s_setup.isCancelRequested = TRUE;
                    EnableWindow(GetDlgItem(window, IDCANCEL), FALSE);
                }
            }
            else if (PAGE_FINISH == s_setup.pageId)
            {
                EndDialog(window, 0);
            }
            else
            {
                EndDialog(window, 1);
            }
            return TRUE;
        default:
            break;
        }
        break;
    case WM_APP_PROGRESS:
        percent = (int32_t)wordParameter;
        if (NULL != s_setup.page)
        {
            SendDlgItemMessageW(s_setup.page, IDC_PROGRESS_BAR, PBM_SETPOS, (WPARAM)percent, 0);
        }
        return TRUE;
    case WM_APP_ACTION:
        action = (wchar_t*)longParameter;
        if ((NULL != s_setup.page) && (NULL != action))
        {
            SetDlgItemTextW(s_setup.page, IDC_PROGRESS_ACTION, action);
        }
        free(action);
        return TRUE;
    case WM_APP_DONE:
        s_setup.isDone = TRUE;
        if (NULL != s_setup.thread)
        {
            (void)WaitForSingleObject(s_setup.thread, INFINITE);
            CloseHandle(s_setup.thread);
            s_setup.thread = NULL;
        }
        ShowPage(PAGE_FINISH);
        return TRUE;
    case WM_SETTINGCHANGE:
        if ((0 != longParameter) && (0 == wcscmp((const wchar_t*)longParameter, THEME_CHANGED_SECTION)))
        {
            is_dark = (bool_t)(ReadsDarkTheme() && (BUILD_DARK_MODE <= s_setup.build));
            if (is_dark != s_setup.isDark)
            {
                if (PAGE_FOLDER == s_setup.pageId)
                {
                    GetDlgItemTextW(s_setup.page, IDC_FOLDER, s_setup.folder, COUNT_OF(s_setup.folder));
                }
                ApplyTheme(is_dark);
                ShowPage(s_setup.pageId);
            }
            // DefWindowProc re-reads the keyboard cues setting on this message and shows them again
            SendMessageW(window, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
        }
        return TRUE;
    case WM_CLOSE:
        if (PAGE_PROGRESS != s_setup.pageId)
        {
            EndDialog(window, 1);
        }
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The page's resource id is the dialog's parameter; it picks the fill-in and the handlers.
 */
static INT_PTR CALLBACK PageProcedure(HWND window, UINT message, WPARAM wordParameter, LPARAM longParameter)
{
    const NMHDR* header;
    PAINTSTRUCT  paint;
    HDC          paint_context;
    LONG_PTR     style;
    TVITEMW      tree_item;
    HRSRC   resource;
    HGLOBAL loaded;
    char*   bytes;
    DWORD   byte_count;
    wchar_t text[MAX_TEXT];
    wchar_t* wide;
    int     wide_count;
    int     page_id;

    switch (message)
    {
    case WM_INITDIALOG:
        page_id = (int)longParameter;
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)page_id);
        if ((IDD_WELCOME == page_id) || (IDD_FINISH == page_id) || (IDD_MAINTENANCE == page_id))
        {
            SendDlgItemMessageW(window, IDC_HEADLINE, WM_SETFONT, (WPARAM)s_setup.headlineFont, TRUE);
        }
        else
        {
            SendDlgItemMessageW(window, IDC_TITLE, WM_SETFONT, (WPARAM)s_setup.headlineFont, FALSE);
        }
        switch (page_id)
        {
        case IDD_WELCOME:
            (void)swprintf_s(text, COUNT_OF(text), VERSION_FORMAT, ARINC661_EDITOR_VERSION_TEXT);
            SetDlgItemTextW(window, IDC_VERSION, text);
            break;
        case IDD_LICENSE:
            resource = FindResourceW(s_setup.module, MAKEINTRESOURCEW(IDR_LICENSE), RT_RCDATA);
            if (NULL != resource)
            {
                loaded     = LoadResource(s_setup.module, resource);
                bytes      = (char*)LockResource(loaded);
                byte_count = SizeofResource(s_setup.module, resource);
                wide_count = MultiByteToWideChar(CP_UTF8, 0, bytes, (int)byte_count, NULL, 0);
                wide       = (wchar_t*)calloc((size_t)wide_count + 1, sizeof(wchar_t));
                if (NULL != wide)
                {
                    (void)MultiByteToWideChar(CP_UTF8, 0, bytes, (int)byte_count, wide, wide_count);
                    SetDlgItemTextW(window, IDC_LICENSE_TEXT, wide);
                    free(wide);
                }
            }
            CheckDlgButton(window, IDC_ACCEPT, s_setup.isAccepted ? BST_CHECKED : BST_UNCHECKED);
            break;
        case IDD_OPTIONS:
            SendDlgItemMessageW(window, IDC_GROUP_SHORTCUTS, WM_SETFONT, (WPARAM)s_setup.boldFont, FALSE);
            SendDlgItemMessageW(window, IDC_GROUP_EXPLORER, WM_SETFONT, (WPARAM)s_setup.boldFont, FALSE);
            CheckDlgButton(window, IDC_OPT_STARTMENU, (0 != (s_setup.options & OPTION_STARTMENU)) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(window, IDC_OPT_DESKTOP, (0 != (s_setup.options & OPTION_DESKTOP)) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(window, IDC_OPT_OPENWITH, (0 != (s_setup.options & OPTION_OPENWITH)) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(window, IDC_OPT_SHELL, (0 != (s_setup.options & OPTION_SHELL)) ? BST_CHECKED : BST_UNCHECKED);
            break;
        case IDD_FOLDER:
            SetDlgItemTextW(window, IDC_FOLDER, s_setup.folder);
            FillFileTree(window);
            break;
        case IDD_READY:
            FillSummary(window);
            break;
        case IDD_PROGRESS:
            SendDlgItemMessageW(window, IDC_PROGRESS_BAR, PBM_SETRANGE32, 0, PROGRESS_RANGE);
            if (MODE_REPAIRING == s_setup.mode)
            {
                SetDlgItemTextW(window, IDC_TITLE, TITLE_REPAIRING);
                SetDlgItemTextW(window, IDC_DESCRIPTION, DESCRIPTION_REPAIRING);
            }
            else if (MODE_REMOVING == s_setup.mode)
            {
                SetDlgItemTextW(window, IDC_TITLE, TITLE_REMOVING);
                SetDlgItemTextW(window, IDC_DESCRIPTION, DESCRIPTION_REMOVING);
            }
            break;
        case IDD_FINISH:
            CheckDlgButton(window, IDC_OPT_LAUNCH, (0 != (s_setup.options & OPTION_LAUNCH)) ? BST_CHECKED : BST_UNCHECKED);
            if ((MODE_INSTALLING != s_setup.mode) || ((ERROR_SUCCESS != s_setup.result) && (ERROR_SUCCESS_REBOOT_REQUIRED != s_setup.result)))
            {
                ShowWindow(GetDlgItem(window, IDC_OPT_LAUNCH), SW_HIDE);
            }
            if (ERROR_INSTALL_USEREXIT == s_setup.result)
            {
                SetDlgItemTextW(window, IDC_HEADLINE, HEADLINE_CANCELLED);
                SetDlgItemTextW(window, IDC_BODY, BODY_CANCELLED);
            }
            else if ((ERROR_SUCCESS != s_setup.result) && (ERROR_SUCCESS_REBOOT_REQUIRED != s_setup.result))
            {
                SetDlgItemTextW(window, IDC_HEADLINE, HEADLINE_FAILED);
                (void)swprintf_s(text, COUNT_OF(text), BODY_FAILED_FORMAT, (unsigned long)s_setup.result);
                SetDlgItemTextW(window, IDC_BODY, text);
            }
            else if (MODE_REMOVING == s_setup.mode)
            {
                SetDlgItemTextW(window, IDC_HEADLINE, HEADLINE_REMOVED);
                SetDlgItemTextW(window, IDC_BODY, BODY_REMOVED);
            }
            break;
        default:
            break;
        }
        RememberLayout(window);
        ThemePage(window);
        return TRUE;
    case WM_SIZE:
        ApplyLayout(window);
        return TRUE;
    case WM_DRAWITEM:
        DrawImage((const DRAWITEMSTRUCT*)longParameter);
        return TRUE;
    case WM_PAINT:
        if (s_setup.isDark)
        {
            paint_context = BeginPaint(window, &paint);
            PaintDarkChrome(window, paint_context);
            EndPaint(window, &paint);
            return TRUE;
        }
        break;
    case WM_NOTIFY:
        header = (const NMHDR*)longParameter;
        if (s_setup.isDark && (NM_CUSTOMDRAW == header->code))
        {
            style = GetWindowLongPtrW(header->hwndFrom, GWL_STYLE);
            if (BS_AUTOCHECKBOX == (style & BS_TYPEMASK))
            {
                SetWindowLongPtrW(window, DWLP_MSGRESULT, PaintCheckBoxLabel((const NMCUSTOMDRAW*)longParameter));
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        return ControlColor(message, (HDC)wordParameter, (HWND)longParameter);
    case WM_COMMAND:
        switch (LOWORD(wordParameter))
        {
        case IDC_ACCEPT:
            s_setup.isAccepted = (bool_t)(BST_CHECKED == IsDlgButtonChecked(window, IDC_ACCEPT));
            UpdateButtons();
            return TRUE;
        case IDC_OPT_STARTMENU:
        case IDC_OPT_DESKTOP:
        case IDC_OPT_OPENWITH:
        case IDC_OPT_SHELL:
            s_setup.options &= OPTION_LAUNCH;
            if (BST_CHECKED == IsDlgButtonChecked(window, IDC_OPT_STARTMENU)) { s_setup.options |= OPTION_STARTMENU; }
            if (BST_CHECKED == IsDlgButtonChecked(window, IDC_OPT_DESKTOP)) { s_setup.options |= OPTION_DESKTOP; }
            if (BST_CHECKED == IsDlgButtonChecked(window, IDC_OPT_OPENWITH)) { s_setup.options |= OPTION_OPENWITH; }
            if (BST_CHECKED == IsDlgButtonChecked(window, IDC_OPT_SHELL)) { s_setup.options |= OPTION_SHELL; }
            return TRUE;
        case IDC_OPT_LAUNCH:
            s_setup.options &= ~OPTION_LAUNCH;
            if (BST_CHECKED == IsDlgButtonChecked(window, IDC_OPT_LAUNCH)) { s_setup.options |= OPTION_LAUNCH; }
            return TRUE;
        case IDC_BROWSE:
            BrowseFolder(window);
            return TRUE;
        case IDC_FOLDER:
            if ((EN_CHANGE == HIWORD(wordParameter)) && (NULL != s_setup.folderItem))
            {
                // The tree's root follows the folder box
                GetDlgItemTextW(window, IDC_FOLDER, text, COUNT_OF(text));
                (void)memset(&tree_item, 0, sizeof(tree_item));
                tree_item.mask    = TVIF_TEXT | TVIF_HANDLE;
                tree_item.hItem   = s_setup.folderItem;
                tree_item.pszText = text;
                (void)TreeView_SetItem(GetDlgItem(window, IDC_FILE_LIST), &tree_item);
            }
            return TRUE;
        case IDC_REPAIR:
            s_setup.mode = MODE_REPAIRING;
            BeginInstall();
            return TRUE;
        case IDC_REMOVE:
            s_setup.mode = MODE_REMOVING;
            BeginInstall();
            return TRUE;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The table lists what stretches or follows the far edges; everything else stays put.
 */
static uint32_t AnchorsOf(int id)
{
    static const Anchor_t c_anchors[] =
    {
        {IDC_PAGE_AREA, ANCHOR_ALL},
        {IDC_BOTTOM_LINE, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM},
        {IDC_BACK, ANCHOR_RIGHT | ANCHOR_BOTTOM},
        {IDC_NEXT, ANCHOR_RIGHT | ANCHOR_BOTTOM},
        {IDCANCEL, ANCHOR_RIGHT | ANCHOR_BOTTOM},
        {IDC_SIDE_IMAGE, ANCHOR_ALL},
        {IDC_BANNER_IMAGE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_BANNER_LINE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_HEADLINE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_BODY, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_LICENSE_TEXT, ANCHOR_ALL},
        {IDC_ACCEPT, ANCHOR_LEFT | ANCHOR_RIGHT | ANCHOR_BOTTOM},
        {IDC_FOLDER, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_BROWSE, ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_GROUP_FILES, ANCHOR_ALL},
        {IDC_FILE_LIST, ANCHOR_ALL},
        {IDC_SUMMARY, ANCHOR_ALL},
        {IDC_PROGRESS_ACTION, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_PROGRESS_BAR, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_OPT_OPENWITH_NOTE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_OPT_SHELL_NOTE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_REPAIR_NOTE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
        {IDC_REMOVE_NOTE, ANCHOR_LEFT | ANCHOR_TOP | ANCHOR_RIGHT},
    };
    uint32_t index;

    for (index = 0; index < COUNT_OF(c_anchors); ++index)
    {
        if (c_anchors[index].id == id)
        {
            return c_anchors[index].anchors;
        }
    }

    return ANCHOR_LEFT | ANCHOR_TOP;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Origins and sizes go into window properties so the layout needs no table per page.
 */
static void RememberLayout(HWND window)
{
    HWND child;
    RECT client;
    RECT area;

    GetClientRect(window, &client);
    (void)SetPropW(window, PROP_CLIENT, (HANDLE)(UINT_PTR)MAKELONG(client.right, client.bottom));
    child = GetWindow(window, GW_CHILD);
    while (NULL != child)
    {
        GetWindowRect(child, &area);
        MapWindowPoints(NULL, window, (POINT*)&area, 2);
        (void)SetPropW(child, PROP_ORIGIN, (HANDLE)(UINT_PTR)MAKELONG(area.left, area.top));
        (void)SetPropW(child, PROP_SIZE, (HANDLE)(UINT_PTR)MAKELONG(area.right - area.left, area.bottom - area.top));
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A control anchored on both sides of an axis stretches; one anchored on the far side moves.
 */
static void ApplyLayout(HWND window)
{
    HWND     child;
    HDWP     positions;
    RECT     client;
    uint32_t anchors;
    LONG     original_width;
    LONG     original_height;
    LONG     origin;
    LONG     size;
    int      delta_x;
    int      delta_y;
    int      x;
    int      y;
    int      width;
    int      height;

    original_width  = (LONG)LOWORD((UINT_PTR)GetPropW(window, PROP_CLIENT));
    original_height = (LONG)HIWORD((UINT_PTR)GetPropW(window, PROP_CLIENT));
    if ((0 == original_width) || (0 == original_height))
    {
        return;
    }
    GetClientRect(window, &client);
    delta_x   = (int)(client.right - original_width);
    delta_y   = (int)(client.bottom - original_height);
    positions = BeginDeferWindowPos(32);
    child     = GetWindow(window, GW_CHILD);
    while (NULL != child)
    {
        origin  = (LONG)(UINT_PTR)GetPropW(child, PROP_ORIGIN);
        size    = (LONG)(UINT_PTR)GetPropW(child, PROP_SIZE);
        anchors = AnchorsOf(GetDlgCtrlID(child));
        x       = (int)(short)LOWORD(origin);
        y       = (int)(short)HIWORD(origin);
        width   = (int)(short)LOWORD(size);
        height  = (int)(short)HIWORD(size);
        if (0 != (anchors & ANCHOR_RIGHT))
        {
            if (0 != (anchors & ANCHOR_LEFT))
            {
                width += delta_x;
            }
            else
            {
                x += delta_x;
            }
        }
        if (0 != (anchors & ANCHOR_BOTTOM))
        {
            if (0 != (anchors & ANCHOR_TOP))
            {
                height += delta_y;
            }
            else
            {
                y += delta_y;
            }
        }
        if (NULL != positions)
        {
            positions = DeferWindowPos(positions, child, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
    if (NULL != positions)
    {
        (void)EndDeferWindowPos(positions);
    }
    InvalidateRect(window, NULL, TRUE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Pages are modeless child dialogs placed over the frame's page area, as the dialog manager
 *  intends for DS_CONTROL dialogs; the old page is destroyed first.
 */
static void ShowPage(Page_t page)
{
    static const int c_pageResources[PAGE_COUNT] = {IDD_WELCOME, IDD_LICENSE, IDD_OPTIONS, IDD_FOLDER, IDD_READY, IDD_PROGRESS, IDD_FINISH, IDD_MAINTENANCE};
    RECT area;

    if (NULL != s_setup.page)
    {
        DestroyWindow(s_setup.page);
        s_setup.page = NULL;
    }
    s_setup.pageId = page;
    s_setup.page   = CreateDialogParamW(s_setup.module, MAKEINTRESOURCEW(c_pageResources[page]), s_setup.frame, PageProcedure, (LPARAM)c_pageResources[page]);
    GetWindowRect(GetDlgItem(s_setup.frame, IDC_PAGE_AREA), &area);
    MapWindowPoints(NULL, s_setup.frame, (POINT*)&area, 2);
    SetWindowPos(s_setup.page, HWND_TOP, area.left, area.top, area.right - area.left, area.bottom - area.top, SWP_SHOWWINDOW);
    UpdateButtons();
    if (PAGE_LICENSE == page)
    {
        SetFocus(GetDlgItem(s_setup.page, IDC_ACCEPT));
    }
    else if (PAGE_FOLDER == page)
    {
        SetFocus(GetDlgItem(s_setup.page, IDC_FOLDER));
    }
    else
    {
        SetFocus(GetDlgItem(s_setup.frame, IDC_NEXT));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void UpdateButtons(void)
{
    HWND back;
    HWND next;
    HWND cancel;

    back   = GetDlgItem(s_setup.frame, IDC_BACK);
    next   = GetDlgItem(s_setup.frame, IDC_NEXT);
    cancel = GetDlgItem(s_setup.frame, IDCANCEL);
    SetWindowTextW(next, NEXT_LABEL);
    SetWindowTextW(cancel, CANCEL_LABEL);
    Button_SetElevationRequiredState(next, FALSE);
    EnableWindow(back, (BOOL)((PAGE_WELCOME < s_setup.pageId) && (s_setup.pageId <= PAGE_READY)));
    EnableWindow(next, TRUE);
    EnableWindow(cancel, TRUE);
    ShowWindow(next, SW_SHOW);
    switch (s_setup.pageId)
    {
    case PAGE_LICENSE:
        EnableWindow(next, (BOOL)s_setup.isAccepted);
        break;
    case PAGE_READY:
        SetWindowTextW(next, INSTALL_LABEL);
        // The shield says an elevation prompt follows
        Button_SetElevationRequiredState(next, (BOOL)(FALSE == s_setup.isElevated));
        break;
    case PAGE_PROGRESS:
        EnableWindow(next, FALSE);
        EnableWindow(cancel, (BOOL)(FALSE == s_setup.isCancelRequested));
        break;
    case PAGE_FINISH:
        SetWindowTextW(next, FINISH_LABEL);
        EnableWindow(cancel, FALSE);
        break;
    case PAGE_MAINTENANCE:
        ShowWindow(next, SW_HIDE);
        break;
    default:
        break;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Push buttons, edits, lists, and scroll bars take the dark Explorer theme. Check boxes and
 *  group boxes drop their theme so WM_CTLCOLORSTATIC can colour their text.
 */
static void ThemePage(HWND page)
{
    HWND     child;
    HWND     list_header;
    wchar_t  class_name[64];
    LONG_PTR style;
    LONG_PTR extended;

    if (FALSE == s_setup.isDark)
    {
        return;
    }
    child = GetWindow(page, GW_CHILD);
    while (NULL != child)
    {
        class_name[0] = L'\0';
        (void)GetClassNameW(child, class_name, COUNT_OF(class_name));
        style = GetWindowLongPtrW(child, GWL_STYLE);
        if (0 == wcscmp(class_name, L"Button"))
        {
            if (BS_GROUPBOX == (style & BS_TYPEMASK))
            {
                // Painted by PaintDarkChrome instead
                ShowWindow(child, SW_HIDE);
            }
            else
            {
                (void)SetWindowTheme(child, THEME_DARK_CONTROLS, NULL);
            }
        }
        else if (0 == wcscmp(class_name, L"Static"))
        {
            if (SS_ETCHEDHORZ == (style & SS_TYPEMASK))
            {
                ShowWindow(child, SW_HIDE);
            }
        }
        else if ((0 == wcscmp(class_name, L"Edit")) || (0 == wcscmp(class_name, WC_LISTVIEWW)) || (0 == wcscmp(class_name, WC_TREEVIEWW)))
        {
            // The system frame is light whatever the theme; drop it and paint a dark one
            extended = GetWindowLongPtrW(child, GWL_EXSTYLE);
            SetWindowLongPtrW(child, GWL_STYLE, style & ~(LONG_PTR)WS_BORDER);
            SetWindowLongPtrW(child, GWL_EXSTYLE, extended & ~(LONG_PTR)(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
            SetWindowPos(child, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            (void)SetWindowTheme(child, THEME_DARK_CONTROLS, NULL);
            if (0 == wcscmp(class_name, WC_TREEVIEWW))
            {
                (void)TreeView_SetBkColor(child, COLOR_DARK_WELL);
                (void)TreeView_SetTextColor(child, COLOR_DARK_TEXT);
                (void)TreeView_SetLineColor(child, COLOR_DARK_FRAME);
            }
            if (0 == wcscmp(class_name, WC_LISTVIEWW))
            {
                ListView_SetBkColor(child, COLOR_DARK_WELL);
                ListView_SetTextBkColor(child, COLOR_DARK_WELL);
                ListView_SetTextColor(child, COLOR_DARK_TEXT);
                list_header = ListView_GetHeader(child);
                if (NULL != list_header)
                {
                    (void)SetWindowTheme(list_header, THEME_DARK_HEADER, NULL);
                    (void)SetWindowSubclass(list_header, HeaderProcedure, HEADER_SUBCLASS_ID, 0);
                }
            }
        }
        else if ((0 == wcscmp(class_name, L"ScrollBar")) || (0 == wcscmp(class_name, L"msctls_progress32")))
        {
            (void)SetWindowTheme(child, THEME_DARK_CONTROLS, NULL);
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Hidden group boxes become a one pixel frame with their caption cut into the top edge;
 *  hidden etched statics become one pixel lines; edits and lists get a one pixel frame just
 *  outside their client area.
 */
static void PaintDarkChrome(HWND window, HDC deviceContext)
{
    HWND     child;
    wchar_t  class_name[64];
    wchar_t  caption[MAX_TEXT];
    RECT     area;
    RECT     text_area;
    SIZE     text_size;
    LONG_PTR style;
    HPEN     pen;
    HGDIOBJ  previous_pen;
    HGDIOBJ  previous_brush;
    HGDIOBJ  previous_font;
    int      length;
    int      caption_pad;

    pen            = CreatePen(PS_SOLID, 1, COLOR_DARK_FRAME);
    previous_pen   = SelectObject(deviceContext, pen);
    previous_brush = SelectObject(deviceContext, GetStockObject(HOLLOW_BRUSH));
    previous_font  = SelectObject(deviceContext, s_setup.textFont);
    SetBkMode(deviceContext, TRANSPARENT);
    SetTextColor(deviceContext, COLOR_DARK_TEXT);
    caption_pad = MulDiv(4, (int)s_setup.dpi, 96);
    child       = GetWindow(window, GW_CHILD);
    while (NULL != child)
    {
        class_name[0] = L'\0';
        (void)GetClassNameW(child, class_name, COUNT_OF(class_name));
        style = GetWindowLongPtrW(child, GWL_STYLE);
        GetWindowRect(child, &area);
        MapWindowPoints(NULL, window, (POINT*)&area, 2);
        if ((0 == wcscmp(class_name, L"Button")) && (BS_GROUPBOX == (style & BS_TYPEMASK)))
        {
            length = GetWindowTextW(child, caption, COUNT_OF(caption));
            GetTextExtentPoint32W(deviceContext, caption, length, &text_size);
            // The frame's top edge runs through the middle of the caption line
            area.top += (text_size.cy / 2);
            (void)Rectangle(deviceContext, area.left, area.top, area.right, area.bottom);
            text_area.left   = area.left + caption_pad * 2;
            text_area.top    = area.top - (text_size.cy / 2);
            text_area.right  = text_area.left + text_size.cx + caption_pad * 2;
            text_area.bottom = text_area.top + text_size.cy;
            FillRect(deviceContext, &text_area, s_setup.pageBrush);
            text_area.left += caption_pad;
            (void)DrawTextW(deviceContext, caption, length, &text_area, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        }
        else if ((0 == wcscmp(class_name, L"Static")) && (SS_ETCHEDHORZ == (style & SS_TYPEMASK)))
        {
            (void)MoveToEx(deviceContext, area.left, area.top, NULL);
            (void)LineTo(deviceContext, area.right, area.top);
        }
        else if (((0 == wcscmp(class_name, L"Edit")) && (0 == (style & ES_READONLY))) || (0 == wcscmp(class_name, WC_LISTVIEWW)) || (0 == wcscmp(class_name, WC_TREEVIEWW)) || ((0 == wcscmp(class_name, L"Edit")) && (0 != (style & WS_VSCROLL))))
        {
            (void)Rectangle(deviceContext, area.left - 1, area.top - 1, area.right + 1, area.bottom + 1);
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
    (void)SelectObject(deviceContext, previous_font);
    (void)SelectObject(deviceContext, previous_brush);
    (void)SelectObject(deviceContext, previous_pen);
    DeleteObject(pen);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The theme draws the box dark but the label black; after its paint the label area is filled
 *  with the page colour and the text drawn again in the dark theme's text colour.
 */
static LRESULT PaintCheckBoxLabel(const NMCUSTOMDRAW* draw)
{
    wchar_t text[MAX_TEXT];
    RECT    area;
    RECT    focus;
    HGDIOBJ previous_font;
    UINT    format;
    UINT    state;
    int     length;
    int     box;

    if (CDDS_PREPAINT == draw->dwDrawStage)
    {
        return CDRF_NOTIFYPOSTPAINT;
    }
    if (CDDS_POSTPAINT != draw->dwDrawStage)
    {
        return CDRF_DODEFAULT;
    }
    box        = GetSystemMetricsForDpi(SM_CXMENUCHECK, s_setup.dpi) + MulDiv(CHECK_BOX_GAP, (int)s_setup.dpi, 96);
    area       = draw->rc;
    area.left += box;
    FillRect(draw->hdc, &area, s_setup.pageBrush);
    length        = GetWindowTextW(draw->hdr.hwndFrom, text, COUNT_OF(text));
    previous_font = SelectObject(draw->hdc, s_setup.textFont);
    SetBkMode(draw->hdc, TRANSPARENT);
    SetTextColor(draw->hdc, COLOR_DARK_TEXT);
    format = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
    state  = (UINT)SendMessageW(draw->hdr.hwndFrom, WM_QUERYUISTATE, 0, 0);
    if (0 != (state & UISF_HIDEACCEL))
    {
        format |= DT_HIDEPREFIX;
    }
    (void)DrawTextW(draw->hdc, text, length, &area, format);
    if ((0 != (draw->uItemState & CDIS_FOCUS)) && (0 == (state & UISF_HIDEFOCUS)))
    {
        // The cue hugs the text, as the theme's own would
        focus = area;
        (void)DrawTextW(draw->hdc, text, length, &focus, format | DT_CALCRECT);
        focus.top    = area.top + (((area.bottom - area.top) - (focus.bottom - focus.top)) / 2);
        focus.bottom = focus.top + (focus.bottom - focus.top);
        InflateRect(&focus, 1, 1);
        (void)DrawFocusRect(draw->hdc, &focus);
    }
    (void)SelectObject(draw->hdc, previous_font);

    return CDRF_DODEFAULT;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Paints every column name on a dark band with a thin separator; nothing else of the header
 *  changes, so sizing and clicks keep working.
 */
static LRESULT CALLBACK HeaderProcedure(HWND window, UINT message, WPARAM wordParameter, LPARAM longParameter, UINT_PTR subclassId, DWORD_PTR reference)
{
    PAINTSTRUCT paint;
    HDC         context;
    HBRUSH      brush;
    HPEN        pen;
    HGDIOBJ     previous_pen;
    HGDIOBJ     previous_font;
    HDITEMW     item;
    RECT        area;
    RECT        client;
    wchar_t     text[MAX_TEXT];
    int         count;
    int         index;
    UINT        format;

    (void)subclassId;
    (void)reference;
    if (WM_PAINT != message)
    {
        if (WM_NCDESTROY == message)
        {
            (void)RemoveWindowSubclass(window, HeaderProcedure, HEADER_SUBCLASS_ID);
        }
        return DefSubclassProc(window, message, wordParameter, longParameter);
    }
    context = BeginPaint(window, &paint);
    GetClientRect(window, &client);
    brush = CreateSolidBrush(COLOR_DARK_HEADER);
    FillRect(context, &client, brush);
    DeleteObject(brush);
    pen           = CreatePen(PS_SOLID, 1, COLOR_DARK_FRAME);
    previous_pen  = SelectObject(context, pen);
    previous_font = SelectObject(context, s_setup.textFont);
    SetBkMode(context, TRANSPARENT);
    SetTextColor(context, COLOR_DARK_TEXT);
    count = (int)SendMessageW(window, HDM_GETITEMCOUNT, 0, 0);
    for (index = 0; index < count; ++index)
    {
        (void)memset(&item, 0, sizeof(item));
        item.mask       = HDI_TEXT | HDI_FORMAT;
        item.pszText    = text;
        item.cchTextMax = COUNT_OF(text);
        text[0]         = L'\0';
        (void)SendMessageW(window, HDM_GETITEMW, (WPARAM)index, (LPARAM)&item);
        (void)SendMessageW(window, HDM_GETITEMRECT, (WPARAM)index, (LPARAM)&area);
        format = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS;
        if (0 != (item.fmt & HDF_RIGHT))
        {
            format |= DT_RIGHT;
        }
        else
        {
            format |= DT_LEFT;
        }
        area.left  += MulDiv(6, (int)s_setup.dpi, 96);
        area.right -= MulDiv(6, (int)s_setup.dpi, 96);
        (void)DrawTextW(context, text, -1, &area, format);
        area.right += MulDiv(6, (int)s_setup.dpi, 96);
        (void)MoveToEx(context, area.right - 1, area.top, NULL);
        (void)LineTo(context, area.right - 1, area.bottom);
    }
    (void)MoveToEx(context, client.left, client.bottom - 1, NULL);
    (void)LineTo(context, client.right, client.bottom - 1);
    (void)SelectObject(context, previous_font);
    (void)SelectObject(context, previous_pen);
    DeleteObject(pen);
    EndPaint(window, &paint);

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Point sizes scale with the frame's monitor; the text font matches the dialog template.
 */
static void CreateFonts(HWND frame)
{
    LOGFONTW log_font;

    s_setup.dpi = GetDpiForWindow(frame);
    if (0 == s_setup.dpi)
    {
        s_setup.dpi = 96;
    }
    (void)memset(&log_font, 0, sizeof(log_font));
    log_font.lfHeight  = -MulDiv(HEADLINE_POINTS, (int)s_setup.dpi, 72);
    log_font.lfWeight  = FW_SEMIBOLD;
    log_font.lfQuality = CLEARTYPE_QUALITY;
    (void)wcscpy_s(log_font.lfFaceName, COUNT_OF(log_font.lfFaceName), L"Segoe UI Semibold");
    s_setup.headlineFont = CreateFontIndirectW(&log_font);
    log_font.lfHeight = -MulDiv(8, (int)s_setup.dpi, 72);
    log_font.lfWeight = FW_NORMAL;
    (void)wcscpy_s(log_font.lfFaceName, COUNT_OF(log_font.lfFaceName), L"Segoe UI");
    s_setup.textFont = CreateFontIndirectW(&log_font);
    log_font.lfWeight = FW_SEMIBOLD;
    s_setup.boldFont  = CreateFontIndirectW(&log_font);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Notes and the version line are muted; edits and lists sit in a darker well.
 */
static INT_PTR ControlColor(UINT message, HDC deviceContext, HWND control)
{
    int id;

    if (FALSE == s_setup.isDark)
    {
        if ((WM_CTLCOLORSTATIC == message) && (NULL != control))
        {
            id = GetDlgCtrlID(control);
            if ((IDC_OPT_OPENWITH_NOTE == id) || (IDC_OPT_SHELL_NOTE == id) || (IDC_VERSION == id) || (IDC_REPAIR_NOTE == id) || (IDC_REMOVE_NOTE == id))
            {
                SetTextColor(deviceContext, COLOR_LIGHT_MUTED);
            }
            if ((IDC_TITLE == id) || (IDC_DESCRIPTION == id) || (IDC_HEADLINE == id) || (IDC_VERSION == id) || (IDC_BODY == id) || (IDC_REPAIR_NOTE == id) || (IDC_REMOVE_NOTE == id))
            {
                // Text over the painted banner or side image: no background of its own
                SetBkMode(deviceContext, TRANSPARENT);
                return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
            }
        }
        return 0;
    }
    switch (message)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
        return (INT_PTR)s_setup.pageBrush;
    case WM_CTLCOLORSTATIC:
        id = 0;
        if (NULL != control)
        {
            id = GetDlgCtrlID(control);
        }
        SetBkMode(deviceContext, TRANSPARENT);
        if ((IDC_OPT_OPENWITH_NOTE == id) || (IDC_OPT_SHELL_NOTE == id) || (IDC_VERSION == id) || (IDC_REPAIR_NOTE == id) || (IDC_REMOVE_NOTE == id))
        {
            SetTextColor(deviceContext, COLOR_DARK_MUTED);
        }
        else
        {
            SetTextColor(deviceContext, COLOR_DARK_TEXT);
        }
        if ((IDC_LICENSE_TEXT == id) || (IDC_SUMMARY == id))
        {
            // Read-only edits ask as statics
            SetBkColor(deviceContext, COLOR_DARK_WELL);
            return (INT_PTR)s_setup.wellBrush;
        }
        if ((IDC_TITLE == id) || (IDC_DESCRIPTION == id) || (IDC_HEADLINE == id) || (IDC_VERSION == id) || (IDC_BODY == id) || (IDC_REPAIR_NOTE == id) || (IDC_REMOVE_NOTE == id))
        {
            return (INT_PTR)GetStockObject(HOLLOW_BRUSH);
        }
        return (INT_PTR)s_setup.pageBrush;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        SetTextColor(deviceContext, COLOR_DARK_TEXT);
        SetBkColor(deviceContext, COLOR_DARK_WELL);
        return (INT_PTR)s_setup.wellBrush;
    default:
        break;
    }

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The banner is the page colour with a dark brand block and the icon at the right; the side
 *  panel is a vertical gradient from the brand dark to a deep blue, the icon, the product name,
 *  and the three bars of the icon. Measures are pixels at the frame's DPI, so a larger window
 *  gains content space while the art keeps its size.
 */
static void DrawImage(const DRAWITEMSTRUCT* item)
{
    TRIVERTEX     vertices[2];
    GRADIENT_RECT rectangle;
    RECT          area;
    RECT          block;
    HBRUSH        brush;
    HFONT         name_font;
    HGDIOBJ       previous_font;
    LOGFONTW      log_font;
    COLORREF      page;
    int           height;
    int           width;
    int           icon_size;
    int           left;
    int           bar;
    int           bar_widths[3];
    int           bar_index;
    bool_t        is_side;

    area    = item->rcItem;
    width   = (area.right - area.left);
    height  = (area.bottom - area.top);
    is_side = (bool_t)(IDC_SIDE_IMAGE == (int)item->CtlID);
    page    = GetSysColor(COLOR_BTNFACE);
    if (s_setup.isDark)
    {
        page = COLOR_DARK_PAGE;
    }
    brush = CreateSolidBrush(page);
    FillRect(item->hDC, &area, brush);
    DeleteObject(brush);
    if (is_side)
    {
        // Gradient strip on the left
        block             = area;
        block.right       = area.left + MulDiv(150, (int)s_setup.dpi, 96);
        vertices[0].x     = block.left;
        vertices[0].y     = block.top;
        vertices[0].Red   = (COLOR16)(24 << 8);
        vertices[0].Green = (COLOR16)(28 << 8);
        vertices[0].Blue  = (COLOR16)(34 << 8);
        vertices[0].Alpha = 0;
        vertices[1].x     = block.right;
        vertices[1].y     = block.bottom;
        vertices[1].Red   = (COLOR16)(26 << 8);
        vertices[1].Green = (COLOR16)(72 << 8);
        vertices[1].Blue  = (COLOR16)(110 << 8);
        vertices[1].Alpha = 0;
        rectangle.UpperLeft  = 0;
        rectangle.LowerRight = 1;
        (void)GradientFill(item->hDC, vertices, 2, &rectangle, 1, GRADIENT_FILL_RECT_V);
        brush       = CreateSolidBrush(RGB(66, 150, 250));
        block.left  = block.right;
        block.right = block.left + MulDiv(3, (int)s_setup.dpi, 96);
        FillRect(item->hDC, &block, brush);
        DeleteObject(brush);
        left      = area.left + MulDiv(32, (int)s_setup.dpi, 96);
        icon_size = MulDiv(80, (int)s_setup.dpi, 96);
        (void)DrawIconEx(item->hDC, left, area.top + MulDiv(30, (int)s_setup.dpi, 96), s_setup.icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
        (void)memset(&log_font, 0, sizeof(log_font));
        log_font.lfHeight  = -MulDiv(16, (int)s_setup.dpi, 96);
        log_font.lfWeight  = FW_SEMIBOLD;
        log_font.lfQuality = CLEARTYPE_QUALITY;
        (void)wcscpy_s(log_font.lfFaceName, COUNT_OF(log_font.lfFaceName), L"Segoe UI Semibold");
        name_font     = CreateFontIndirectW(&log_font);
        previous_font = SelectObject(item->hDC, name_font);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, RGB(235, 238, 242));
        TextOutW(item->hDC, left, area.top + MulDiv(122, (int)s_setup.dpi, 96), L"CDS Editor", 10);
        (void)SelectObject(item->hDC, previous_font);
        DeleteObject(name_font);
        log_font.lfHeight = -MulDiv(11, (int)s_setup.dpi, 96);
        log_font.lfWeight = FW_NORMAL;
        (void)wcscpy_s(log_font.lfFaceName, COUNT_OF(log_font.lfFaceName), L"Segoe UI");
        name_font     = CreateFontIndirectW(&log_font);
        previous_font = SelectObject(item->hDC, name_font);
        SetTextColor(item->hDC, RGB(170, 190, 210));
        TextOutW(item->hDC, left, area.top + MulDiv(145, (int)s_setup.dpi, 96), L"ARINC 661 / FACE", 16);
        (void)SelectObject(item->hDC, previous_font);
        DeleteObject(name_font);
        bar_widths[0] = 80;
        bar_widths[1] = 56;
        bar_widths[2] = 36;
        for (bar_index = 0; bar_index < 3; ++bar_index)
        {
            bar          = area.top + MulDiv(178 + (bar_index * 15), (int)s_setup.dpi, 96);
            block.left   = left;
            block.right  = left + MulDiv(bar_widths[bar_index], (int)s_setup.dpi, 96);
            block.top    = bar;
            block.bottom = bar + MulDiv(6, (int)s_setup.dpi, 96);
            if (0 == bar_index)
            {
                brush = CreateSolidBrush(RGB(66, 150, 250));
            }
            else
            {
                brush = CreateSolidBrush(RGB(150, 175, 200));
            }
            FillRect(item->hDC, &block, brush);
            DeleteObject(brush);
        }
    }
    else
    {
        // Brand block with the icon at the right
        block       = area;
        block.left  = area.right - MulDiv(80, (int)s_setup.dpi, 96);
        brush       = CreateSolidBrush(RGB(24, 28, 34));
        FillRect(item->hDC, &block, brush);
        DeleteObject(brush);
        brush       = CreateSolidBrush(RGB(66, 150, 250));
        block.right = block.left + MulDiv(3, (int)s_setup.dpi, 96);
        FillRect(item->hDC, &block, brush);
        DeleteObject(brush);
        icon_size = MulDiv(32, (int)s_setup.dpi, 96);
        (void)DrawIconEx(item->hDC, area.right - MulDiv(56, (int)s_setup.dpi, 96), area.top + ((height - icon_size) / 2), s_setup.icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
    }
    (void)width;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The system image list supplies the shell's own icons; it is shared, so the tree must not
 *  destroy it. Every node starts expanded.
 */
static void FillFileTree(HWND page)
{
    HWND         tree;
    SHFILEINFOW  info;
    HIMAGELIST   images;
    HTREEITEM    examples;
    HTREEITEM    start_menu;
    HTREEITEM    desktop;
    HTREEITEM    registry;
    wchar_t      text[MAX_TEXT];
    wchar_t      size_text[32];
    wchar_t      group[MAX_TEXT];
    uint32_t     index;
    int          folder_icon;
    int          registry_icon;

    tree   = GetDlgItem(page, IDC_FILE_LIST);
    images = (HIMAGELIST)SHGetFileInfoW(L"folder", FILE_ATTRIBUTE_DIRECTORY, &info, sizeof(info), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    if (NULL != images)
    {
        (void)TreeView_SetImageList(tree, images, TVSIL_NORMAL);
    }
    folder_icon   = ShellIconIndex(L"folder", TRUE);
    registry_icon = ShellIconIndex(L"entries.reg", FALSE);
    (void)TreeView_DeleteAllItems(tree);
    s_setup.folderItem = AddTreeNode(tree, TVI_ROOT, s_setup.folder, folder_icon);
    examples           = NULL;
    for (index = 0; index < s_setup.rowCount; ++index)
    {
        (void)swprintf_s(text, COUNT_OF(text), TREE_FILE_FORMAT, s_setup.rows[index].name, s_setup.rows[index].size);
        if (0 == wcscmp(s_setup.rows[index].where, WHERE_EXAMPLES))
        {
            if (NULL == examples)
            {
                examples = AddTreeNode(tree, s_setup.folderItem, TREE_EXAMPLES_NAME, folder_icon);
            }
            (void)AddTreeNode(tree, examples, text, ShellIconIndex(s_setup.rows[index].name, FALSE));
        }
        else
        {
            (void)AddTreeNode(tree, s_setup.folderItem, text, ShellIconIndex(s_setup.rows[index].name, FALSE));
        }
    }
    if (0 != (s_setup.options & OPTION_STARTMENU))
    {
        start_menu = AddTreeNode(tree, TVI_ROOT, WHERE_STARTMENU, folder_icon);
        (void)AddTreeNode(tree, start_menu, TREE_SHORTCUT_NAME, ShellIconIndex(L"shortcut.lnk", FALSE));
        (void)TreeView_Expand(tree, start_menu, TVE_EXPAND);
    }
    if (0 != (s_setup.options & OPTION_DESKTOP))
    {
        desktop = AddTreeNode(tree, TVI_ROOT, WHERE_DESKTOP, folder_icon);
        (void)AddTreeNode(tree, desktop, TREE_SHORTCUT_NAME, ShellIconIndex(L"shortcut.lnk", FALSE));
        (void)TreeView_Expand(tree, desktop, TVE_EXPAND);
    }
    if (0 != (s_setup.options & (OPTION_OPENWITH | OPTION_SHELL)))
    {
        registry = AddTreeNode(tree, TVI_ROOT, TREE_REGISTRY_NAME, registry_icon);
        if (0 != (s_setup.options & OPTION_OPENWITH))
        {
            (void)AddTreeNode(tree, registry, ROW_OPENWITH, registry_icon);
        }
        if (0 != (s_setup.options & OPTION_SHELL))
        {
            (void)AddTreeNode(tree, registry, ROW_SHELL, registry_icon);
        }
        (void)TreeView_Expand(tree, registry, TVE_EXPAND);
    }
    if (NULL != examples)
    {
        (void)TreeView_Expand(tree, examples, TVE_EXPAND);
    }
    (void)TreeView_Expand(tree, s_setup.folderItem, TVE_EXPAND);
    (void)TreeView_EnsureVisible(tree, s_setup.folderItem);
    FormatSize(s_setup.totalBytes, size_text, COUNT_OF(size_text));
    (void)swprintf_s(group, COUNT_OF(group), GROUP_FILES_FORMAT, (unsigned)s_setup.fileCount, size_text);
    SetDlgItemTextW(page, IDC_GROUP_FILES, group);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static HTREEITEM AddTreeNode(HWND tree, HTREEITEM parent, const wchar_t* text, int icon)
{
    TVINSERTSTRUCTW insert;

    (void)memset(&insert, 0, sizeof(insert));
    insert.hParent             = parent;
    insert.hInsertAfter        = TVI_LAST;
    insert.item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    insert.item.pszText        = (wchar_t*)text;
    insert.item.iImage         = icon;
    insert.item.iSelectedImage = icon;

    return TreeView_InsertItem(tree, &insert);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attributes stand in for the file, which need not exist.
 */
static int ShellIconIndex(const wchar_t* name, bool_t isFolder)
{
    SHFILEINFOW info;
    DWORD       attributes;

    attributes = FILE_ATTRIBUTE_NORMAL;
    if (isFolder)
    {
        attributes = FILE_ATTRIBUTE_DIRECTORY;
    }
    (void)memset(&info, 0, sizeof(info));
    if (0 == SHGetFileInfoW(name, attributes, &info, sizeof(info), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON))
    {
        return -1;
    }

    return info.iIcon;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void FillSummary(HWND page)
{
    wchar_t summary[MAX_TEXT * 2];
    bool_t  has_shortcuts;
    bool_t  has_associations;

    has_shortcuts    = (bool_t)(0 != (s_setup.options & (OPTION_STARTMENU | OPTION_DESKTOP)));
    has_associations = (bool_t)(0 != (s_setup.options & (OPTION_OPENWITH | OPTION_SHELL)));
    (void)swprintf_s(summary, COUNT_OF(summary), SUMMARY_FOLDER_FORMAT, s_setup.folder);
    if (has_shortcuts || has_associations)
    {
        (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_TASKS_HEADING);
    }
    if (has_shortcuts)
    {
        (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_SHORTCUTS_HEADING);
        if (0 != (s_setup.options & OPTION_STARTMENU)) { (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_ITEM_PREFIX OPTION_YES_STARTMENU); }
        if (0 != (s_setup.options & OPTION_DESKTOP)) { (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_ITEM_PREFIX OPTION_YES_DESKTOP); }
    }
    if (has_associations)
    {
        (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_ASSOCIATIONS_HEADING);
        if (0 != (s_setup.options & OPTION_OPENWITH)) { (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_ITEM_PREFIX OPTION_YES_OPENWITH); }
        if (0 != (s_setup.options & OPTION_SHELL)) { (void)wcscat_s(summary, COUNT_OF(summary), SUMMARY_ITEM_PREFIX OPTION_YES_SHELL); }
    }
    SetDlgItemTextW(page, IDC_SUMMARY, summary);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The common item dialog in folder mode, started at the current choice.
 */
static void BrowseFolder(HWND page)
{
    IFileOpenDialog* dialog;
    IShellItem*      item;
    IShellItem*      start;
    wchar_t*         chosen;
    FILEOPENDIALOGOPTIONS options;

    dialog = NULL;
    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, (void**)&dialog)) || (NULL == dialog))
    {
        return;
    }
    options = 0;
    (void)IFileOpenDialog_GetOptions(dialog, &options);
    (void)IFileOpenDialog_SetOptions(dialog, options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    (void)IFileOpenDialog_SetTitle(dialog, L"Choose the destination folder");
    GetDlgItemTextW(page, IDC_FOLDER, s_setup.folder, COUNT_OF(s_setup.folder));
    start = NULL;
    if (SUCCEEDED(SHCreateItemFromParsingName(s_setup.folder, NULL, &IID_IShellItem, (void**)&start)) && (NULL != start))
    {
        (void)IFileOpenDialog_SetFolder(dialog, start);
        IShellItem_Release(start);
    }
    if (SUCCEEDED(IFileOpenDialog_Show(dialog, s_setup.frame)))
    {
        item = NULL;
        if (SUCCEEDED(IFileOpenDialog_GetResult(dialog, &item)) && (NULL != item))
        {
            chosen = NULL;
            if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &chosen)) && (NULL != chosen))
            {
                (void)wcsncpy_s(s_setup.folder, COUNT_OF(s_setup.folder), chosen, _TRUNCATE);
                SetDlgItemTextW(page, IDC_FOLDER, s_setup.folder);
                CoTaskMemFree(chosen);
            }
            IShellItem_Release(item);
        }
    }
    IFileOpenDialog_Release(dialog);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Without administrator rights the program relaunches itself elevated with the choices on
 *  the command line and leaves; the elevated copy shows the progress and finish pages.
 */
static void BeginInstall(void)
{
    SHELLEXECUTEINFOW execute;
    wchar_t           self[MAX_PATH];
    wchar_t           arguments[MAX_PATH + 128];
    const wchar_t*    mode_text;
    const wchar_t*    theme_text;

    if (FALSE == s_setup.isElevated)
    {
        mode_text = MODE_INSTALL;
        if (MODE_REPAIRING == s_setup.mode)
        {
            mode_text = MODE_REPAIR;
        }
        else if (MODE_REMOVING == s_setup.mode)
        {
            mode_text = MODE_REMOVE;
        }
        (void)GetModuleFileNameW(NULL, self, COUNT_OF(self));
        theme_text = L"";
        if (s_setup.isDark)
        {
            theme_text = THEME_DARK_SWITCH;
        }
        else
        {
            theme_text = THEME_LIGHT_SWITCH;
        }
        (void)swprintf_s(arguments, COUNT_OF(arguments), L"%s %s \"%s\" %u %s", ELEVATED_SWITCH, mode_text, s_setup.folder, (unsigned)s_setup.options, theme_text);
        (void)memset(&execute, 0, sizeof(execute));
        execute.cbSize       = sizeof(execute);
        execute.fMask        = SEE_MASK_NOASYNC;
        execute.hwnd         = s_setup.frame;
        execute.lpVerb       = L"runas";
        execute.lpFile       = self;
        execute.lpParameters = arguments;
        execute.nShow        = SW_SHOWNORMAL;
        if (ShellExecuteExW(&execute))
        {
            EndDialog(s_setup.frame, 0);
        }
        else
        {
            MessageBoxW(s_setup.frame, ELEVATE_FAILED_TEXT, ASK_CANCEL_TITLE, MB_OK | MB_ICONWARNING);
        }
        return;
    }
    if (PAGE_PROGRESS != s_setup.pageId)
    {
        ShowPage(PAGE_PROGRESS);
    }
    s_setup.isCancelRequested = FALSE;
    s_setup.isDone            = FALSE;
    s_setup.thread            = CreateThread(NULL, 0, InstallThread, NULL, 0, NULL);
    if (NULL == s_setup.thread)
    {
        s_setup.result = ERROR_INSTALL_FAILURE;
        PostMessageW(s_setup.frame, WM_APP_DONE, 0, 0);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Install passes the folder and the chosen features; repair and remove act on the registered
 *  product. The result is reported with WM_APP_DONE.
 */
static DWORD WINAPI InstallThread(LPVOID parameter)
{
    wchar_t properties[MAX_PROPERTIES];
    wchar_t features[128];
    UINT    result;

    (void)parameter;
    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
    (void)MsiSetExternalUIRecord(InstallerHandler, INSTALLLOGMODE_PROGRESS | INSTALLLOGMODE_ACTIONSTART | INSTALLLOGMODE_ACTIONDATA | INSTALLLOGMODE_ERROR | INSTALLLOGMODE_FATALEXIT, NULL, NULL);
    if (MODE_INSTALLING == s_setup.mode)
    {
        (void)wcscpy_s(features, COUNT_OF(features), FEATURES_MAIN);
        if (0 != (s_setup.options & OPTION_STARTMENU)) { (void)wcscat_s(features, COUNT_OF(features), FEATURE_STARTMENU); }
        if (0 != (s_setup.options & OPTION_DESKTOP)) { (void)wcscat_s(features, COUNT_OF(features), FEATURE_DESKTOP); }
        if (0 != (s_setup.options & OPTION_OPENWITH)) { (void)wcscat_s(features, COUNT_OF(features), FEATURE_OPENWITH); }
        if (0 != (s_setup.options & OPTION_SHELL)) { (void)wcscat_s(features, COUNT_OF(features), FEATURE_SHELL); }
        (void)swprintf_s(properties, COUNT_OF(properties), PROPERTIES_FORMAT, s_setup.folder, features);
        result = MsiInstallProductW(s_setup.packagePath, properties);
    }
    else if (MODE_REPAIRING == s_setup.mode)
    {
        result = MsiReinstallProductW(s_setup.productCode, REINSTALLMODE_FILEMISSING | REINSTALLMODE_FILEOLDERVERSION | REINSTALLMODE_FILEVERIFY | REINSTALLMODE_MACHINEDATA | REINSTALLMODE_USERDATA | REINSTALLMODE_SHORTCUT);
    }
    else
    {
        result = MsiConfigureProductExW(s_setup.productCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT, L"REBOOT=ReallySuppress");
    }
    s_setup.result = result;
    PostMessageW(s_setup.frame, WM_APP_DONE, 0, 0);

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Progress records: field 1 is the kind (0 reset with the total in field 2, 1 action info
 *  with the step in field 2, 2 progress of field 2 ticks, 3 extension of the total); the bar
 *  shows done over total. Returning IDCANCEL to a progress message stops the installer.
 */
static INT CALLBACK InstallerHandler(LPVOID context, UINT messageType, MSIHANDLE record)
{
    UINT     kind;
    int      field1;
    int      field2;
    int      field3;
    int32_t  percent;
    wchar_t  text[MAX_TEXT];
    wchar_t* copy;
    DWORD    length;

    (void)context;
    kind = (messageType & 0xFF000000u);
    switch (kind)
    {
    case INSTALLMESSAGE_PROGRESS:
        field1 = MsiRecordGetInteger(record, 1);
        field2 = MsiRecordGetInteger(record, 2);
        field3 = MsiRecordGetInteger(record, 3);
        if (0 == field1)
        {
            s_setup.progressTotal   = field2;
            s_setup.progressDone    = 0;
            s_setup.progressForward = (bool_t)(0 == field3);
            s_setup.progressEnabled = FALSE;
        }
        else if (1 == field1)
        {
            s_setup.progressStep    = field2;
            s_setup.progressEnabled = (bool_t)(1 == field3);
        }
        else if (2 == field1)
        {
            if (s_setup.progressForward)
            {
                s_setup.progressDone += field2;
            }
            else
            {
                s_setup.progressDone -= field2;
            }
        }
        else if (3 == field1)
        {
            s_setup.progressTotal += field2;
        }
        if (0 < s_setup.progressTotal)
        {
            percent = (int32_t)(((int64_t)s_setup.progressDone * PROGRESS_RANGE) / s_setup.progressTotal);
            if (percent < 0) { percent = 0; }
            if (PROGRESS_RANGE < percent) { percent = PROGRESS_RANGE; }
            PostMessageW(s_setup.frame, WM_APP_PROGRESS, (WPARAM)percent, 0);
        }
        if (s_setup.isCancelRequested)
        {
            return IDCANCEL;
        }
        return IDOK;
    case INSTALLMESSAGE_ACTIONSTART:
        length = COUNT_OF(text);
        text[0] = L'\0';
        (void)MsiRecordGetStringW(record, 3, text, &length);
        if (L'\0' == text[0])
        {
            length = COUNT_OF(text);
            (void)MsiRecordGetStringW(record, 2, text, &length);
        }
        copy = _wcsdup(text);
        if (NULL != copy)
        {
            PostMessageW(s_setup.frame, WM_APP_ACTION, 0, (LPARAM)copy);
        }
        return IDOK;
    case INSTALLMESSAGE_ACTIONDATA:
        if (s_setup.progressEnabled)
        {
            if (s_setup.progressForward)
            {
                s_setup.progressDone += s_setup.progressStep;
            }
            else
            {
                s_setup.progressDone -= s_setup.progressStep;
            }
        }
        return IDOK;
    case INSTALLMESSAGE_ERROR:
    case INSTALLMESSAGE_FATALEXIT:
        length = COUNT_OF(text);
        text[0] = L'\0';
        (void)MsiFormatRecordW(0, record, text, &length);
        if (L'\0' != text[0])
        {
            MessageBoxW(s_setup.frame, text, ASK_CANCEL_TITLE, MB_OK | MB_ICONERROR);
        }
        return IDOK;
    default:
        break;
    }

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Explorer starts the editor so it runs as the signed-in user, not as the elevated setup.
 */
static void LaunchEditor(void)
{
    wchar_t path[MAX_PATH];

    (void)swprintf_s(path, COUNT_OF(path), L"%s\\%s", s_setup.folder, EDITOR_EXECUTABLE);
    (void)ShellExecuteW(NULL, L"open", L"explorer.exe", path, NULL, SW_SHOWNORMAL);
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Entry point: prepares the theme, the package, and the frame; cleans up after.
 *
 * \param[in] instance          This program
 * \param[in] previousInstance  Unused
 * \param[in] commandLine       Unused; the wide command line is read instead
 * \param[in] showCommand       Unused
 *
 * \retval    int               Zero when Setup finished, one when the user left.
 */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int showCommand)
{
    INITCOMMONCONTROLSEX controls;
    wchar_t              program_files[MAX_PATH];
    bool_t               is_elevated_run;
    INT_PTR              outcome;

    (void)previousInstance;
    (void)commandLine;
    (void)showCommand;
    (void)memset(&s_setup, 0, sizeof(s_setup));
    s_setup.module     = instance;
    s_setup.options    = OPTION_DEFAULTS;
    s_setup.build      = ReadBuildNumber();
    // Dark drawing needs 1809 or later; older systems stay light whatever the theme says
    s_setup.isDark     = (bool_t)(ReadsDarkTheme() && (BUILD_DARK_MODE <= s_setup.build));
    // /light and /dark force a theme, for support and for screenshots
    if (NULL != wcsstr(GetCommandLineW(), THEME_LIGHT_SWITCH))
    {
        s_setup.isDark = FALSE;
    }
    else if ((NULL != wcsstr(GetCommandLineW(), THEME_DARK_SWITCH)) && (BUILD_DARK_MODE <= s_setup.build))
    {
        s_setup.isDark = TRUE;
    }
    ApplyMenuTheme();
    s_setup.isElevated = IsElevated();
    is_elevated_run    = ParseElevatedArguments(GetCommandLineW());
    (void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    controls.dwSize = sizeof(controls);
    controls.dwICC  = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    (void)InitCommonControlsEx(&controls);
    if (FALSE == is_elevated_run)
    {
        program_files[0] = L'\0';
        (void)SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, program_files);
        (void)swprintf_s(s_setup.folder, COUNT_OF(s_setup.folder), L"%s%s", program_files, DEFAULT_FOLDER_SUFFIX);
    }
    if (FALSE == ExtractPackage())
    {
        MessageBoxW(NULL, L"Setup could not unpack its installer package.", ASK_CANCEL_TITLE, MB_OK | MB_ICONERROR);
        return 1;
    }
    FindInstalledProduct();
    ReadFileRows();
    s_setup.icon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_SETUP), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR);
    outcome = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_FRAME), NULL, FrameProcedure, 0);

    if (NULL != s_setup.thread)
    {
        (void)WaitForSingleObject(s_setup.thread, INFINITE);
        CloseHandle(s_setup.thread);
    }
    (void)DeleteFileW(s_setup.packagePath);
    if (NULL != s_setup.headlineFont) { DeleteObject(s_setup.headlineFont); }
    if (NULL != s_setup.textFont) { DeleteObject(s_setup.textFont); }
    if (NULL != s_setup.boldFont) { DeleteObject(s_setup.boldFont); }
    if (NULL != s_setup.icon) { DestroyIcon(s_setup.icon); }
    if (NULL != s_setup.pageBrush) { DeleteObject(s_setup.pageBrush); }
    if (NULL != s_setup.wellBrush) { DeleteObject(s_setup.wellBrush); }
    CoUninitialize();

    return (int)outcome;
}
