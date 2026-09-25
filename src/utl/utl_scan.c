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

#include "utl/utl_scan.h"

#include "os/os_assert.h"

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

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

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks each digit against the remaining range before multiplying.
 */
bool_t Utl_Scan_Int64(const byte_t* text, uint64_t byteCount, int64_t* value)
{
    uint64_t magnitude;
    uint64_t limit;
    uint64_t digit;
    uint64_t byte_index;
    byte_t   current_byte;
    bool_t   is_negative;
    bool_t   is_digit;
    bool_t   has_capacity;
    bool_t   is_valid;

    LOGASSERT(NULL != text);
    LOGASSERT(NULL != value);

    byte_index  = 0;
    is_negative = FALSE;
    if (0 < byteCount)
    {
        current_byte = text[0];
        if ('-' == current_byte)
        {
            is_negative = TRUE;
            byte_index  = 1;
        }
        else if ('+' == current_byte)
        {
            byte_index = 1;
        }
    }
    is_valid  = (byte_index < byteCount);
    magnitude = 0;
    limit     = (uint64_t)INT64_MAX;
    if (is_negative)
    {
        limit += 1;
    }
    for (; byte_index < byteCount; ++byte_index)
    {
        current_byte = text[byte_index];
        is_digit     = (0 != isdigit(current_byte)) && (current_byte <= '9');
        if (FALSE == is_digit)
        {
            is_valid = FALSE;
            break;
        }
        digit        = (uint64_t)(current_byte - '0');
        has_capacity = (magnitude <= ((limit - digit) / 10));
        if (FALSE == has_capacity)
        {
            is_valid = FALSE;
            break;
        }
        magnitude = (magnitude * 10) + digit;
    }
    if (is_valid)
    {
        if (is_negative)
        {
            if (limit == magnitude)
            {
                (*value) = INT64_MIN;
            }
            else
            {
                (*value) = -(int64_t)magnitude;
            }
        }
        else
        {
            (*value) = (int64_t)magnitude;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates the XML Schema decimal lexical form before using the C conversion.
 */
bool_t Utl_Scan_Float32(const byte_t* text, uint64_t byteCount, float32_t* value)
{
    char_t    number[(UTL_SCAN_MAX_FLOAT32_BYTES + 1)];
    char_t*   end;
    uint64_t  byte_index;
    float32_t parsed_value;
    bool_t    has_content;
    bool_t    fits_number;
    bool_t    has_digits;
    bool_t    has_point;
    bool_t    is_digit;
    bool_t    is_valid;

    LOGASSERT(NULL != text);
    LOGASSERT(NULL != value);

    has_content = (0 < byteCount);
    fits_number = (byteCount <= UTL_SCAN_MAX_FLOAT32_BYTES);
    is_valid    = has_content && fits_number;
    if (is_valid)
    {
        byte_index = 0;
        if (('+' == text[0]) || ('-' == text[0]))
        {
            byte_index = 1;
        }
        has_digits = FALSE;
        has_point  = FALSE;
        for (; byte_index < byteCount; ++byte_index)
        {
            is_digit = (0 != isdigit(text[byte_index])) && (text[byte_index] <= '9');
            if (is_digit)
            {
                has_digits = TRUE;
            }
            else if (('.' == text[byte_index]) && (FALSE == has_point))
            {
                has_point = TRUE;
            }
            else
            {
                is_valid = FALSE;
                break;
            }
        }
        if (FALSE == has_digits)
        {
            is_valid = FALSE;
        }
        if (is_valid)
        {
            (void)memcpy(number, text, byteCount);
            number[byteCount] = '\0';
            parsed_value = strtof(number, &end);
            is_valid     = ('\0' == (*end)) && (parsed_value <= FLT_MAX) && (parsed_value >= -FLT_MAX);
            if (is_valid)
            {
                (*value) = parsed_value;
            }
        }
    }

    return is_valid;
}
