/*=================================================================================================
** 1. References
**===============================================================================================*/

/* Win32 and OpenGL setup adapted from imgui-docking/examples/example_win32_opengl3/main.cpp. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#include "arinc661_context.h"
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
#include <winternl.h>
#include <d3dkmthk.h>
#include <dwmapi.h>
#include <GL/gl.h>

#include <atomic>
#include <cstdio>
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
            ImGui::Separator();
            if (ImGui::MenuItem(SAVE_LABEL, SAVE_SHORTCUT_LABEL, false, has_active))
            {
                if (window->isDefinitionActive)
                {
                    Arinc661_Definition_SaveActiveDocument(window->definitionEditor);
                }
                else
                {
                    Arinc661_Configuration_SaveActiveDocument(window->configurationEditor);
                }
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
        ImGui::EndMainMenuBar();
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
        if (window->isDefinitionActive)
        {
            Arinc661_Definition_SaveActiveDocument(window->definitionEditor);
        }
        else
        {
            Arinc661_Configuration_SaveActiveDocument(window->configurationEditor);
        }
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
    if (ImGui::BeginChild(HIERARCHY_PANE_NAME, ImVec2(0.0f, 0.0f), PANE_FLAGS))
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
    bool_t               is_definition_focused;
    const char_t*        status_text;
    ImVec2               table_size;
    float32_t            configuration_hierarchy_width;
    float32_t            configuration_inspector_width;
    float32_t            definition_hierarchy_width;
    float32_t            definition_inspector_width;
    float32_t            hierarchy_width;
    float32_t            inspector_width;
    float32_t            status_bar_height;

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
    if (ImGui::Begin(PAGE_WINDOW_NAME, NULL, PAGE_WINDOW_FLAGS))
    {
        DrawPageFiles(window);
        Arinc661_Configuration_MeasurePanes(window->configurationEditor, &configuration_hierarchy_width, &configuration_inspector_width);
        Arinc661_Definition_MeasurePanes(window->definitionEditor, &definition_hierarchy_width, &definition_inspector_width);
        hierarchy_width   = ImMax(configuration_hierarchy_width, definition_hierarchy_width);
        inspector_width   = ImMax(configuration_inspector_width, definition_inspector_width);
        status_bar_height = (ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y);
        table_size        = ImGui::GetContentRegionAvail();
        table_size.y      = ImMax(ImGui::GetTextLineHeight(), (table_size.y - status_bar_height));
        if (ImGui::BeginTable(PANE_TABLE_ID, 3, PANE_TABLE_FLAGS, table_size))
        {
            ImGui::TableSetupColumn(HIERARCHY_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, hierarchy_width);
            ImGui::TableSetupColumn("Scene", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, inspector_width);
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
            ImGui::TableNextRow();
            ImGui::PopStyleVar();
            ImGui::TableSetColumnIndex(0);
            DrawPageHierarchy(window);
            ImGui::TableSetColumnIndex(1);
            if (window->isDefinitionActive)
            {
                Arinc661_Definition_DrawScene(window->definitionEditor);
            }
            else
            {
                Arinc661_Configuration_DrawScene(window->configurationEditor);
            }
            ImGui::TableSetColumnIndex(2);
            if (window->isDefinitionActive)
            {
                Arinc661_Definition_DrawInspector(window->definitionEditor);
            }
            else
            {
                Arinc661_Configuration_DrawInspector(window->configurationEditor);
            }
            ImGui::EndTable();
        }
        Arinc661_Configuration_DrawDialogs(window->configurationEditor);
        Arinc661_Definition_DrawDialogs(window->definitionEditor);
        status_text = Arinc661_Configuration_StatusText(window->configurationEditor);
        if (window->isDefinitionActive)
        {
            status_text = Arinc661_Definition_StatusText(window->definitionEditor);
        }
        if ('\0' == status_text[0])
        {
            status_text = READY_LABEL;
        }
        ImGui::Separator();
        ImGui::TextUnformatted(status_text);
    }
    ImGui::End();
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
    window->context = (Arinc661_Context_t*)OS_CALLOC(sizeof(Arinc661_Context_t));
    Arinc661_Context_Initialize(window->context, window->configurationEditor, window->definitionEditor);
    ScanFolder(window);
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
int main(void)
{
    float32_t                            main_scale;
    WindowThreadState_t                  window;
    HANDLE                               window_thread;
    HDC                                  present_device_context;
    D3DKMT_WAITFORVERTICALBLANKEVENT      vbe;

    // Route assertion and runtime failures to stderr instead of dialog boxes
    Os_Diagnostics_Initialize();
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
