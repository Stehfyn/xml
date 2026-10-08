/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_assert.h"
#include "os/os_time.h"

#include <windows.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

#define NANOSECONDS_PER_SECOND      (1000000000ull)
#define NANOSECONDS_PER_MILLISECOND (1000000ull)

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
uint64_t Os_Time_GetNanoseconds(void)
{
    BOOL          query_succeeded;
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    uint64_t      seconds;
    uint64_t      remainder;

    query_succeeded = QueryPerformanceFrequency(&frequency);
    LOGASSERT(query_succeeded);
    query_succeeded = QueryPerformanceCounter(&counter);
    LOGASSERT(query_succeeded);
    LOGASSERT(0 < frequency.QuadPart);
    LOGASSERT(0 <= counter.QuadPart);

    seconds   = (uint64_t)counter.QuadPart / (uint64_t)frequency.QuadPart;
    remainder = (uint64_t)counter.QuadPart % (uint64_t)frequency.QuadPart;
    return seconds * NANOSECONDS_PER_SECOND + remainder * NANOSECONDS_PER_SECOND / (uint64_t)frequency.QuadPart;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Os_Time_SleepUntil(uint64_t deadlineNanoseconds)
{
    uint64_t now;

    now = Os_Time_GetNanoseconds();
    while (now < deadlineNanoseconds)
    {
        uint64_t remaining;
        uint64_t milliseconds;

        remaining    = deadlineNanoseconds - now;
        milliseconds = (remaining + NANOSECONDS_PER_MILLISECOND - 1u) / NANOSECONDS_PER_MILLISECOND;
        LOGASSERT(milliseconds <= MAXDWORD);
        Sleep((DWORD)milliseconds);
        now = Os_Time_GetNanoseconds();
    }
}
