#ifndef FACEOE_UTL_PRINT_H
#define FACEOE_UTL_PRINT_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ISO/IEC 9899:2018, section 7.21.6.1, the fprintf conversions d, u, and f.
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
 * \brief Prints a signed 64-bit value as decimal text, the inverse of FACEOE_Utl_Scan_Int64.
 *
 * \par Details:
 *  A negative value starts with a minus sign; a positive value has no sign,
 *  for example -20 prints "-20" and INT64_MIN prints "-9223372036854775808".
 *
 * \par Requirements:
 *  - 001 This function shall return the number of characters printed, excluding the zero terminator.
 *  - 002 This function shall print an empty string and return -1 when the text and its terminator do not fit.
 *  - 003 This function shall fail an assertion if a is NULL or capacity is zero.
 *
 * \param[out] a          Non-NULL destination text
 * \param[in]  capacity   Destination bytes, including the terminator
 * \param[in]  v          Value to print
 *
 * \retval     -1             The text does not fit.
 * \retval     1..INT32_MAX   Number of characters printed.
 */
int32_t FACEOE_Utl_Print_Int64(char_t* a, uint32_t capacity, int64_t v);

/*************************************************************************************************/
/**
 * \brief Prints an unsigned 64-bit value as decimal text.
 *
 * \par Details:
 *  For example 10000 prints "10000" and UINT64_MAX prints "18446744073709551615".
 *
 * \par Requirements:
 *  - 001 This function shall return the number of characters printed, excluding the zero terminator.
 *  - 002 This function shall print an empty string and return -1 when the text and its terminator do not fit.
 *  - 003 This function shall fail an assertion if a is NULL or capacity is zero.
 *
 * \param[out] a          Non-NULL destination text
 * \param[in]  capacity   Destination bytes, including the terminator
 * \param[in]  v          Value to print
 *
 * \retval     -1             The text does not fit.
 * \retval     1..INT32_MAX   Number of characters printed.
 */
int32_t FACEOE_Utl_Print_Uint64(char_t* a, uint32_t capacity, uint64_t v);

/*************************************************************************************************/
/**
 * \brief Prints a value as decimal text with a fixed number of fractional digits, the inverse of FACEOE_Utl_Scan_Float.
 *
 * \par Details:
 *  The value is rounded to the given number of fractional digits; zero
 *  digits prints no decimal point. For example, 1.5 with 3 digits prints
 *  "1.500", and -0.125 with 1 digit prints "-0.1".
 *
 * \par Requirements:
 *  - 001 This function shall return the number of characters printed, excluding the zero terminator.
 *  - 002 This function shall print an empty string and return -1 when the text and its terminator do not fit.
 *  - 003 This function shall fail an assertion if a is NULL or capacity is zero.
 *
 * \param[out] a                  Non-NULL destination text
 * \param[in]  capacity           Destination bytes, including the terminator
 * \param[in]  v                  Value to print
 * \param[in]  fractionalDigits   Digits after the decimal point
 *
 * \retval     -1             The text does not fit.
 * \retval     1..INT32_MAX   Number of characters printed.
 */
int32_t FACEOE_Utl_Print_Float(char_t* a, uint32_t capacity, float64_t v, uint32_t fractionalDigits);

#endif /* FACEOE_UTL_PRINT_H */
