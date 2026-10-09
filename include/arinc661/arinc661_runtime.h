#ifndef ARINC661_RUNTIME_H
#define ARINC661_RUNTIME_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, sections 3 and 8: each widget and extension's Runtime Modifiable
 * Parameters table. Section 4.5.4.5: the parameter structures of A661_CMD_SET_PARAMETER.
 * Table 4.6-8: the ParameterIdent values and the structure each uses.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief The parameter structure carrying a run-time value (section 4.5.4.5).
 */
typedef enum Arinc661_RunTimeStructure_e
{
    ARINC661_RUNTIME_1BYTE,       ///< A661_ParameterStructure_1Byte: ParameterIdent, a 1-byte value, a pad
    ARINC661_RUNTIME_2BYTES,      ///< A661_ParameterStructure_2Bytes: ParameterIdent, a 2-byte value
    ARINC661_RUNTIME_4BYTES,      ///< A661_ParameterStructure_4Bytes: ParameterIdent, a pad, a 4-byte value
    ARINC661_RUNTIME_8BYTES,      ///< A661_ParameterStructure_8Bytes: ParameterIdent, a pad, two 4-byte values
    ARINC661_RUNTIME_STRING,      ///< A661_ParameterStructure_String: ParameterIdent, a size, a padded string
    ARINC661_RUNTIME_STRING_ARRAY ///< A661_ParameterStructure_StringArray: ParameterIdent, a count, indexed padded strings
} Arinc661_RunTimeStructure_t;

/*************************************************************************************************/
/**
 * \brief One parameter a widget or extension type lets the UA set at run time.
 *
 * \par Details:
 *  A pair such as A661_POS_XY sets two parameters at once; secondName names the second.
 *  A string array sets entries of an array parameter, one property per entry.
 */
typedef struct Arinc661_RunTimeParameter_s
{
    uint32_t                    ident;      ///< ParameterIdent, such as 0xB530 for A661_VISIBLE
    Arinc661_RunTimeStructure_t structure;  ///< Structure carrying the value
    Arinc661_ParameterKind_t    kind;       ///< Type of the value, or of the first value of a pair
    const char_t*               name;       ///< Parameter set, which is also the name of the property holding its value
    Arinc661_ParameterKind_t    secondKind; ///< Type of the second value of a pair; unused otherwise
    const char_t*               secondName; ///< Second parameter set by a pair, or NULL
} Arinc661_RunTimeParameter_t;

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
 * \brief Describes a parameter a type lets the UA set at run time.
 *
 * \par Details:
 *  Only parameters carried by an Arinc661_RunTimeStructure_t structure are listed; arrays of
 *  numbers, buffers, and map items are not.
 *
 * \par Requirements:
 *  - 001 This function shall describe the row of parameterIdent in the Runtime Modifiable Parameters table of typeName.
 *  - 002 This function shall return FALSE, leaving parameter unchanged, if the type or the row is not listed.
 *  - 003 This function shall fail an assertion if typeName or parameter is NULL.
 *
 * \param[in]  typeName         Zero-terminated widget or extension type name
 * \param[in]  parameterIdent   ParameterIdent
 * \param[out] parameter        Parameter description; its names stay valid for the program lifetime
 *
 * \retval     TRUE             if the type lets the UA set the parameter at run time.
 * \retval     FALSE            otherwise.
 */
bool_t Arinc661_RunTime_GetParameter(const char_t* typeName, uint32_t parameterIdent, Arinc661_RunTimeParameter_t* parameter);

#endif /* ARINC661_RUNTIME_H */
