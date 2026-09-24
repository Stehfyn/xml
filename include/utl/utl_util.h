#ifndef UTL_UTIL_H
#define UTL_UTIL_H

#include "os/os_types.h"

char_t* Utl_Strtok(char_t* str, const char_t* delems, char_t** intPos);
const char_t* Utl_StrStr(const char_t* str1, const char_t* str2, bool_t caseInsensitive);
uint64_t Utl_StrSplit(const char_t* str, char_t* outStr, char_t delim, uint64_t dstSize);
void Utl_StrTrim(char_t* str);

#endif /* UTL_UTIL_H */
