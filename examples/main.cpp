/*=================================================================================================
** 1. References
**===============================================================================================*/

/* Win32 and OpenGL setup adapted from imgui-docking/examples/example_win32_opengl3/main.cpp. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#include "arinc661_definition_editor.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_diagnostics.h"
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

/* Main menu bar that switches between the two editors; each keeps its own documents. */
#define EDITOR_MENU_LABEL              ("Editor")
#define CONFIGURATION_EDITOR_LABEL     ("Configuration")
#define CONFIGURATION_EDITOR_SHORTCUT  ("Ctrl+1")
#define DEFINITION_EDITOR_LABEL        ("Definition File")
#define DEFINITION_EDITOR_SHORTCUT     ("Ctrl+2")
#define CONFIGURATION_EDITOR_KEY_CHORD (ImGuiMod_Ctrl | ImGuiKey_1)
#define DEFINITION_EDITOR_KEY_CHORD    (ImGuiMod_Ctrl | ImGuiKey_2)
#define EDITOR_SHORTCUT_FLAGS          (ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverActive)

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
    bool_t                           isDefinitionActive;  ///< TRUE while the Definition File editor is shown.
    bool_t                           hasEditors;          ///< TRUE once both editors exist.
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
 *  Draws the main menu bar that selects an editor, then the selected editor below it. The
 *  menu bar shrinks the viewport work area, so each editor still fills the rest of the window.
 *  Both editors keep their documents while hidden.
 */
static void DrawEditors(WindowThreadState_t* window)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(EDITOR_MENU_LABEL))
        {
            if (ImGui::MenuItem(CONFIGURATION_EDITOR_LABEL, CONFIGURATION_EDITOR_SHORTCUT, FALSE == window->isDefinitionActive))
            {
                window->isDefinitionActive = FALSE;
            }
            if (ImGui::MenuItem(DEFINITION_EDITOR_LABEL, DEFINITION_EDITOR_SHORTCUT, TRUE == window->isDefinitionActive))
            {
                window->isDefinitionActive = TRUE;
            }
            ImGui::EndMenu();
        }
        // Keep the switch visible as two checked items in the bar itself
        ImGui::Separator();
        if (ImGui::MenuItem(CONFIGURATION_EDITOR_LABEL, NULL, FALSE == window->isDefinitionActive))
        {
            window->isDefinitionActive = FALSE;
        }
        if (ImGui::MenuItem(DEFINITION_EDITOR_LABEL, NULL, TRUE == window->isDefinitionActive))
        {
            window->isDefinitionActive = TRUE;
        }
        ImGui::EndMainMenuBar();
    }
    if (ImGui::Shortcut(CONFIGURATION_EDITOR_KEY_CHORD, EDITOR_SHORTCUT_FLAGS))
    {
        window->isDefinitionActive = FALSE;
    }
    if (ImGui::Shortcut(DEFINITION_EDITOR_KEY_CHORD, EDITOR_SHORTCUT_FLAGS))
    {
        window->isDefinitionActive = TRUE;
    }
    if (window->isDefinitionActive)
    {
        Arinc661_Definition_DrawEditor(window->definitionEditor);
    }
    else
    {
        Arinc661_Configuration_DrawEditor(window->configurationEditor);
    }
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
