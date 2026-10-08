#ifndef FACEOE_UTL_UTIL_H
#define FACEOE_UTL_UTIL_H

#include "os/os_types.h"

/*************************************************************************************************/
/**
 * \brief Finds a substring.
 *
 * \param str1              Str1
 * \param str2              Str2
 * \param caseInsensitive   Case insensitive
 *
 * \retval const char_t*   Pointer to the first match, or NULL.
 */
const char_t* FACEOE_Utl_StrStr(const char_t* str1, const char_t* str2, bool_t caseInsensitive);

/*************************************************************************************************/
/**
 * \brief Finds the first occurrence of a character, searching no further than a bound.
 *
 * \param[in] str         Text to search
 * \param[in] character   Character to find; the zero terminator never matches
 * \param[in] sizeOfStr   Most characters to search
 *
 * \retval    const char_t*   Pointer to the first match, or NULL when none comes before the terminator or the bound.
 */
const char_t* FACEOE_Utl_StrChr(const char_t* str, char_t character, uint64_t sizeOfStr);

/*************************************************************************************************/
/**
 * \brief Splits a string into parts at a delimiter.
 *
 * \param str       Str
 * \param outStr    Out str
 * \param delim     Delim
 * \param dstSize   Dst size
 *
 * \retval uint64_t   Number of parts written.
 */
uint64_t FACEOE_Utl_StrSplit(const char_t* str, char_t* outStr, char_t delim, uint64_t dstSize);

/*************************************************************************************************/
/**
 * \brief Trims leading and trailing white space.
 *
 * \param str   Str
 *
 * \retval None
 */
void FACEOE_Utl_StrTrim(char_t* str);

#endif /* FACEOE_UTL_UTIL_H */
