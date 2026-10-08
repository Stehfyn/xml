#ifndef FACEOE_UTL_PRINT_H
#define FACEOE_UTL_PRINT_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ISO/IEC 9899:2018, section 7.21.6.12, the vsnprintf function.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#if defined(__GNUC__)
#define FACEOE_UTL_PRINT_FORMAT_CHECK(formatIndex, firstArgument) __attribute__((format(printf, formatIndex, firstArgument))) ///< Lets GCC and Clang check arguments against the format
#else
#define FACEOE_UTL_PRINT_FORMAT_CHECK(formatIndex, firstArgument) ///< No format checking on this compiler
#endif

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Formats text into a caller buffer as printf does, always zero-terminated.
 *
 * \par Details:
 *  Output that does not fit is cut off at capacity - 1 characters. The return
 *  value counts only the characters stored, so a caller appending piece after
 *  piece can add it to its position without passing the end of the buffer:
 *
 * \code{.c}
 * used += FACEOE_Utl_Print_Format(&report[used], capacity - used, "Wrote %s\n", path);
 * \endcode
 *
 * \par Requirements:
 *  - 001 This function shall store at most capacity bytes, including the zero terminator.
 *  - 002 This function shall zero-terminate the buffer whenever capacity is nonzero.
 *  - 003 This function shall return the number of characters stored, excluding the terminator.
 *  - 004 This function shall store an empty string and return zero on an encoding error.
 *  - 005 This function shall fail an assertion if format is NULL, or buffer is NULL with a nonzero capacity.
 *
 * \param[out] buffer     Destination text
 * \param[in]  capacity   Destination bytes, including the terminator
 * \param[in]  format     printf format
 * \param[in]  ...        Arguments of the format
 *
 * \retval     uint32_t   Characters stored, excluding the terminator; less than capacity, or zero when capacity is zero.
 */
uint32_t FACEOE_Utl_Print_Format(char_t* buffer, uint32_t capacity, const char_t* format, ...) FACEOE_UTL_PRINT_FORMAT_CHECK(3, 4);

#endif /* FACEOE_UTL_PRINT_H */
