/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_assert.h"

#include "os/os_diagnostics.h"

#include <windows.h>

#include "os/os_types.h"

#include <crtdbg.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define STACK_FRAME_CAPACITY (32u) ///< Maximum stack frames reported for one unhandled exception.

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Writes an unhandled Win32 exception and its module-relative stack frames to stderr.
 *
 * \param[in] pException          Non-NULL exception record supplied by Win32
 *
 * \return EXCEPTION_EXECUTE_HANDLER so the boundary terminates without crash UI.
 */
static LONG WINAPI ReportUnhandledException(EXCEPTION_POINTERS* pException);

#if defined(_DEBUG)
/** Consume CRT reports without invoking the CRT's message-box handler. */
static int __cdecl ReportCrt(int reportType, char* message, int* result)
{
    (void)fprintf(stderr, "CRT report %d: %s\n", reportType, message ? message : "(no message)");
    (void)fflush(stderr);
    if (NULL != result) (*result) = 0;
    if (_CRT_WARN != reportType) _Exit(EXIT_FAILURE);
    return TRUE;
}

/** Wide-character counterpart of the CRT report hook. */
static int __cdecl ReportCrtWide(int reportType, wchar_t* message, int* result)
{
    (void)fprintf(stderr, "CRT report %d: %ls\n", reportType, message ? message : L"(no message)");
    (void)fflush(stderr);
    if (NULL != result) (*result) = 0;
    if (_CRT_WARN != reportType) _Exit(EXIT_FAILURE);
    return TRUE;
}
#endif

/** Invalid CRT parameters are fatal in unattended runs. */
static void __cdecl ReportInvalidParameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
{
    (void)reserved;
    (void)fprintf(stderr, "CRT invalid parameter: %ls in %ls (%ls:%u)\n", expression ? expression : L"(unknown)", function ? function : L"(unknown)", file ? file : L"(unknown)", line);
    (void)fflush(stderr);
    _Exit(EXIT_FAILURE);
}

/** Abort must never reach WER, the JIT debugger, or the CRT report dialog. */
static void ReportAbort(int signalNumber)
{
    (void)signalNumber;
    (void)fputs("fatal: SIGABRT\n", stderr);
    (void)fflush(stderr);
    _Exit(EXIT_FAILURE);
}

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static LONG WINAPI ReportUnhandledException(EXCEPTION_POINTERS* pException)
{
    void*                    frames[STACK_FRAME_CAPACITY];
    MEMORY_BASIC_INFORMATION memory_info;
    char                     module_path[MAX_PATH];
    USHORT                   frame_count;
    USHORT                   frame_index;

    LOGSTART(stderr);
    LOGPRINTS("unhandled exception 0x");
    LOGPRINTX((uint64_t)(pException->ExceptionRecord->ExceptionCode), 8, TRUE);
    LOGPRINTS(" at ");
    LOGPRINTP((const void*)(pException->ExceptionRecord->ExceptionAddress));
    LOGPRINTS("\n");
    LOGEND();
    frame_count = CaptureStackBackTrace(0, (DWORD)STACK_FRAME_CAPACITY, frames, NULL);
    for (frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        if (0 != VirtualQuery(frames[frame_index], &memory_info, sizeof(memory_info)) && 0 != GetModuleFileNameA((HMODULE)memory_info.AllocationBase, module_path, (DWORD)sizeof(module_path)))
        {
            LOGSTART(stderr);
            LOGPRINTS("stack ");
            LOGPRINTU((uint64_t)(frame_index));
            LOGPRINTS(": ");
            LOGPRINTS(module_path);
            LOGPRINTS("+0x");
            LOGPRINTX((uint64_t)((unsigned long long)((byte_t*)frames[frame_index] - (byte_t*)memory_info.AllocationBase)), 1, TRUE);
            LOGPRINTS("\n");
            LOGEND();
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Os_Diagnostics_Initialize(void)
{
    (void)SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    (void)_set_error_mode(_OUT_TO_STDERR);
    (void)_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    (void)_set_invalid_parameter_handler(ReportInvalidParameter);
    (void)signal(SIGABRT, ReportAbort);
#if defined(_DEBUG)
    (void)_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, ReportCrt);
    (void)_CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, ReportCrtWide);
#endif
    (void)_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    (void)_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    (void)_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    (void)SetUnhandledExceptionFilter(ReportUnhandledException);
}

int Os_Diagnostics_Run(int (*entry)(int, char**), int argc, char** argv)
{
    Os_Diagnostics_Initialize();
    __try
    {
        return entry(argc, argv);
    }
    __except (ReportUnhandledException(GetExceptionInformation()))
    {
        return EXIT_FAILURE;
    }
}
