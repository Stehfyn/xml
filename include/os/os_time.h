#ifndef XML_OS_TIME_H
#define XML_OS_TIME_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Returns monotonic time in nanoseconds.
 *
 * \retval    uint64_t   Nanoseconds from an unspecified monotonic epoch.
 */
uint64_t Os_Time_GetNanoseconds(void);

/*************************************************************************************************/
/**
 * \brief Suspends execution until a monotonic nanosecond deadline.
 *
 * \param[in] deadlineNanoseconds   Absolute deadline on the Os_Time_GetNanoseconds clock; a past deadline returns at once
 *
 * \retval    None
 */
void Os_Time_SleepUntil(uint64_t deadlineNanoseconds);

#endif /* XML_OS_TIME_H */
