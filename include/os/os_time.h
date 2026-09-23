#ifndef XML_OS_TIME_H
#define XML_OS_TIME_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include <stdint.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Returns monotonic time in nanoseconds.
 *
 * \return Nanoseconds from an unspecified monotonic epoch.
 */
uint64_t Os_Time_GetNanoseconds(void);

/*************************************************************************************************/
/**
 * \brief Suspends execution until a monotonic nanosecond deadline.
 *
 * \param[in] deadlineNanoseconds Absolute deadline returned by the monotonic clock
 *
 * \retval None
 */
void Os_Time_SleepUntil(uint64_t deadlineNanoseconds);

#endif /* XML_OS_TIME_H */
