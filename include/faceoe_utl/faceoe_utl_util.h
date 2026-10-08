#ifndef FACEOE_UTL_UTIL_H
#define FACEOE_UTL_UTIL_H

#include "os/os_types.h"

/*************************************************************************************************/
/**
 * \brief Finds the first occurrence of one string in another.
 *
 * \param[in] str1              Zero-terminated text to search
 * \param[in] str2              Zero-terminated text to find; an empty string matches at str1
 * \param[in] caseInsensitive   TRUE to compare characters after tolower
 *
 * \retval    const char_t*   Pointer to the first match in str1, or NULL.
 */
const char_t* FACEOE_Utl_StrStr(const char_t* str1, const char_t* str2, bool_t caseInsensitive);

/*************************************************************************************************/
/**
 * \brief Finds the first occurrence of a character, searching no further than a bound.
 *
 * \param[in] str         Text to search
 * \param[in] character   Character to find; the zero terminator never matches
 * \param[in] sizeOfStr   Maximum number of characters to search
 *
 * \retval    const char_t*   Pointer to the first match, or NULL when none comes before the terminator or the bound.
 */
const char_t* FACEOE_Utl_StrChr(const char_t* str, char_t character, uint64_t sizeOfStr);

/*************************************************************************************************/
/**
 * \brief Copies the first part of a delimited string.
 *
 * \par Details:
 *  Copies str up to the first delim or zero terminator, cut to dstSize - 1
 *  characters, into outStr and zero-terminates it. When str is empty or
 *  starts with delim, outStr is not written.
 *
 * \param[in]  str       Zero-terminated text to split
 * \param[out] outStr    Destination for the part
 * \param[in]  delim     Delimiter character
 * \param[in]  dstSize   Bytes in outStr, at least 2
 *
 * \retval     0          str is empty or starts with delim.
 * \retval     uint64_t   Length of the part plus one; when the part ended at delim, the offset in str of the next part.
 */
uint64_t FACEOE_Utl_StrSplit(const char_t* str, char_t* outStr, char_t delim, uint64_t dstSize);

/*************************************************************************************************/
/**
 * \brief Removes leading and trailing white space in place.
 *
 * \par Details:
 *  White space is any character isspace accepts. A string of only white space
 *  is left unchanged.
 *
 * \param[in,out] str   Zero-terminated text to trim, or NULL to do nothing
 *
 * \retval        None
 */
void FACEOE_Utl_StrTrim(char_t* str);

#endif /* FACEOE_UTL_UTIL_H */
