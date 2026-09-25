#ifndef UTL_SCAN_H
#define UTL_SCAN_H

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

#define UTL_SCAN_MAX_FLOAT32_BYTES (95) ///< Maximum counted decimal bytes accepted for float32_t.

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
 * \brief Scans a complete counted decimal integer into a signed 64-bit value.
 *
 * \par Details:
 *  An optional leading sign is followed by one or more ASCII digits. Whitespace,
 *  trailing data, and values outside the signed 64-bit range are rejected.
 *
 * \par Requirements:
 *  - 001 This function shall consume the complete counted span.
 *  - 002 This function shall reject values outside the signed 64-bit range.
 *  - 003 This function shall leave value unchanged on failure.
 *
 * \param[in]  text      Non-NULL byte span
 * \param[in]  byteCount Number of bytes in the span
 * \param[out] value     Non-NULL destination, written only on success
 *
 * \retval    TRUE      The entire span is a representable integer.
 * \retval    FALSE     The span is empty, malformed, or out of range.
 */
bool_t Utl_Scan_Int64(const byte_t* text, uint64_t byteCount, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Scans a complete counted XML Schema decimal into a finite float32_t.
 *
 * \par Details:
 *  XML Schema decimal permits an optional sign, decimal digits, and an optional
 *  decimal point. At least one digit is required. Exponents and surrounding
 *  whitespace are not part of its lexical form.
 *  https://www.w3.org/TR/xmlschema-2/#decimal
 *
 * \par Requirements:
 *  - 001 This function shall consume the complete counted span.
 *  - 002 This function shall reject exponent notation and non-finite results.
 *  - 003 This function shall leave value unchanged on failure.
 *
 * \param[in]  text      Non-NULL byte span
 * \param[in]  byteCount Number of bytes, at most UTL_SCAN_MAX_FLOAT32_BYTES
 * \param[out] value     Non-NULL destination, written only on success
 *
 * \retval    TRUE      The entire span is a finite decimal value.
 * \retval    FALSE     The span is empty, malformed, too long, or out of range.
 */
bool_t Utl_Scan_Float32(const byte_t* text, uint64_t byteCount, float32_t* value);

#endif /* UTL_SCAN_H */
