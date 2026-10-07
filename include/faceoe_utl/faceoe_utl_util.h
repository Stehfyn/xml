#ifndef FACEOE_UTL_UTIL_H
#define FACEOE_UTL_UTIL_H

#include "os/os_types.h"

/*************************************************************************************************/
/**
 * \brief Splits a string at delimiters, like strtok but reentrant.
 *
 * \param str      Str
 * \param delems   Delems
 * \param intPos   Int pos
 *
 * \retval char_t*   Next token, or NULL when none remain.
 */
char_t* FACEOE_Utl_Strtok(char_t* str, const char_t* delems, char_t** intPos);

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
