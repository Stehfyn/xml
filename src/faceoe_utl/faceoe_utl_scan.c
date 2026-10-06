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

#include "faceoe_utl/faceoe_utl_scan.h"

#include "os/os_assert.h"

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
int32_t FACEOE_Utl_Scan_Int64(const char_t* a, int64_t* v)
{
    uint64_t magnitude;
    uint64_t limit;
    uint64_t digit;
    uint32_t byte_index;
    char_t   current_byte;
    int32_t  result;
    bool_t   is_negative;
    bool_t   is_digit;
    bool_t   has_digits;
    bool_t   is_valid;

    LOGASSERT(NULL != a);
    LOGASSERT(NULL != v);

    byte_index  = 0;
    is_negative = FALSE;
    if ('-' == a[0])
    {
        is_negative = TRUE;
        byte_index  = 1;
    }
    else if ('+' == a[0])
    {
        byte_index = 1;
    }
    has_digits = FALSE;
    is_valid   = TRUE;
    magnitude = 0;
    limit     = (uint64_t)INT64_MAX;
    if (is_negative)
    {
        limit += 1;
    }
    for (;;)
    {
        current_byte = a[byte_index];
        is_digit     = ('0' <= current_byte) && (current_byte <= '9');
        if (FALSE == is_digit)
        {
            break;
        }
        has_digits = TRUE;
        digit      = (uint64_t)(current_byte - '0');
        if ((INT32_MAX == byte_index) || (magnitude > ((limit - digit) / 10)))
        {
            is_valid = FALSE;
            break;
        }
        magnitude = (magnitude * 10) + digit;
        byte_index += 1;
    }
    result = -1;
    if (is_valid && has_digits)
    {
        if (is_negative)
        {
            if (limit == magnitude)
            {
                (*v) = INT64_MIN;
            }
            else
            {
                (*v) = -(int64_t)magnitude;
            }
        }
        else
        {
            (*v) = (int64_t)magnitude;
        }
        result = (int32_t)byte_index;
    }

    return result;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates the XML Schema decimal lexical form before using the C conversion.
 */
int32_t FACEOE_Utl_Scan_Float(const char_t* a, float64_t* v)
{
    char_t    number[FACEOE_UTL_SCAN_MAX_FLOAT_BYTES + 1];
    char_t*   end;
    uint32_t  byte_index;
    float64_t parsed_value;
    int32_t   result;
    bool_t    has_digits;
    bool_t    has_point;
    bool_t    is_digit;
    bool_t    is_valid;

    LOGASSERT(NULL != a);
    LOGASSERT(NULL != v);

    byte_index = 0;
    if (('+' == a[0]) || ('-' == a[0]))
    {
        byte_index = 1;
    }
    has_digits = FALSE;
    has_point  = FALSE;
    is_valid   = TRUE;
    for (;;)
    {
        is_digit = ('0' <= a[byte_index]) && (a[byte_index] <= '9');
        if (is_digit)
        {
            has_digits = TRUE;
        }
        else if (('.' == a[byte_index]) && (FALSE == has_point))
        {
            has_point = TRUE;
        }
        else
        {
            break;
        }
        if (FACEOE_UTL_SCAN_MAX_FLOAT_BYTES == byte_index)
        {
            is_valid = FALSE;
            break;
        }
        byte_index += 1;
    }

    result = -1;
    if (is_valid && has_digits)
    {
        (void)memcpy(number, a, byte_index);
        number[byte_index] = '\0';
        parsed_value = strtod(number, &end);
        if ((end == (number + byte_index)) && (parsed_value <= DBL_MAX) && (parsed_value >= -DBL_MAX))
        {
            (*v)  = parsed_value;
            result = (int32_t)byte_index;
        }
    }

    return result;
}
