/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ISO/IEC 9899:2018, section 7.21.6.1, the fprintf conversions d, u, and f.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "faceoe_utl/faceoe_utl_print.h"

#include "os/os_assert.h"

#include <inttypes.h>
#include <stdio.h>

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

/*************************************************************************************************/
/**
 * \brief Checks an snprintf result against the destination it was printed into.
 *
 * \param[in,out] a          Destination text, emptied when the result does not fit
 * \param[in]     capacity   Destination bytes, including the terminator
 * \param[in]     printed    snprintf result
 *
 * \retval        -1             The text does not fit, or snprintf failed.
 * \retval        1..INT32_MAX   Number of characters printed.
 */
static int32_t CheckPrinted(char_t* a, uint32_t capacity, int printed);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  snprintf returns the length the complete text would have had, so a result
 *  of capacity or more means the text was cut off.
 */
static int32_t CheckPrinted(char_t* a, uint32_t capacity, int printed)
{
    int32_t length;

    length = (int32_t)printed;
    if ((0 > printed) || ((uint32_t)printed >= capacity))
    {
        a[0]   = '\0';
        length = -1;
    }

    return length;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The longest result, INT64_MIN, needs 21 bytes including the terminator.
 */
int32_t FACEOE_Utl_Print_Int64(char_t* a, uint32_t capacity, int64_t v)
{
    LOGASSERT(NULL != a);
    LOGASSERT(0 < capacity);

    return CheckPrinted(a, capacity, snprintf(a, capacity, "%" PRId64, v));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The longest result, UINT64_MAX, needs 21 bytes including the terminator.
 */
int32_t FACEOE_Utl_Print_Uint64(char_t* a, uint32_t capacity, uint64_t v)
{
    LOGASSERT(NULL != a);
    LOGASSERT(0 < capacity);

    return CheckPrinted(a, capacity, snprintf(a, capacity, "%" PRIu64, v));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rounding follows the C library's %.*f conversion.
 */
int32_t FACEOE_Utl_Print_Float(char_t* a, uint32_t capacity, float64_t v, uint32_t fractionalDigits)
{
    LOGASSERT(NULL != a);
    LOGASSERT(0 < capacity);
    LOGASSERT(INT32_MAX >= fractionalDigits);

    return CheckPrinted(a, capacity, snprintf(a, capacity, "%.*f", (int)fractionalDigits, v));
}
