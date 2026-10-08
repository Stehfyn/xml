/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_assert.h"
#include "os/os_time.h"

#include <errno.h>
#include <time.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

#define NANOSECONDS_PER_SECOND (1000000000ull)

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
    int             clock_result;
    struct timespec now;

    clock_result = clock_gettime(CLOCK_MONOTONIC, &now);
    LOGASSERT(0 == clock_result);
    LOGASSERT(0 <= now.tv_sec);
    LOGASSERT(0 <= now.tv_nsec);
    return (uint64_t)now.tv_sec * NANOSECONDS_PER_SECOND + (uint64_t)now.tv_nsec;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Os_Time_SleepUntil(uint64_t deadlineNanoseconds)
{
    struct timespec deadline;
    int             result;

    deadline.tv_sec  = (time_t)(deadlineNanoseconds / NANOSECONDS_PER_SECOND);
    deadline.tv_nsec = (long)(deadlineNanoseconds % NANOSECONDS_PER_SECOND);
    do
    {
        result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
    } while (EINTR == result);
    LOGASSERT(0 == result);
}
