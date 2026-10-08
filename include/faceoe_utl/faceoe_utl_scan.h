#ifndef FACEOE_UTL_SCAN_H
#define FACEOE_UTL_SCAN_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * W3C XML Schema Part 2: Datatypes, decimal lexical representation:
 * https://www.w3.org/TR/xmlschema-2/#decimal
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

#define FACEOE_UTL_SCAN_MAX_FLOAT_BYTES (2047) ///< Maximum decimal prefix length accepted by FACEOE_Utl_Scan_Float.

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
 * \brief Scans a decimal integer prefix into a signed 64-bit value.
 *
 * \par Details:
 *  An optional leading sign is followed by one or more ASCII digits. Scanning stops
 *  at the first non-digit. Leading whitespace and values outside the signed 64-bit
 *  range are rejected.
 *
 * \par Requirements:
 *  - 001 This function shall return the number of characters in the numeric prefix.
 *  - 002 This function shall reject values outside the signed 64-bit range.
 *  - 003 This function shall leave v unchanged on failure.
 *
 * \param[in]  a   Non-NULL text, readable through the first non-digit or NUL
 * \param[out] v   Non-NULL destination, written only on success
 *
 * \retval     -1             The prefix has no digits, is too long, or is out of range.
 * \retval     1..INT32_MAX   Number of characters consumed.
 */
int32_t FACEOE_Utl_Scan_Int64(const char_t* a, int64_t* v);

/*************************************************************************************************/
/**
 * \brief Scans an XML Schema decimal prefix into a finite float64_t.
 *
 * \par Details:
 *  XML Schema decimal permits an optional sign, decimal digits, and an optional
 *  decimal point. At least one digit is required. Scanning stops at the first
 *  character outside this form, so an exponent is not consumed. Leading whitespace is rejected.
 *  https://www.w3.org/TR/xmlschema-2/#decimal
 *
 * \par Requirements:
 *  - 001 This function shall return the number of characters in the decimal prefix.
 *  - 002 This function shall reject non-finite results and prefixes longer than FACEOE_UTL_SCAN_MAX_FLOAT_BYTES.
 *  - 003 This function shall leave v unchanged on failure.
 *
 * \param[in]  a   Non-NULL text, readable through the first non-decimal character or NUL
 * \param[out] v   Non-NULL destination, written only on success
 *
 * \retval     -1             The prefix has no digits, is too long, or is out of range.
 * \retval     1..INT32_MAX   Number of characters consumed.
 */
int32_t FACEOE_Utl_Scan_Float(const char_t* a, float64_t* v);

#endif /* FACEOE_UTL_SCAN_H */
