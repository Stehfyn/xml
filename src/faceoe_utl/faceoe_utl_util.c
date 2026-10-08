#include "faceoe_utl/faceoe_utl_util.h"

#include "os/os_assert.h"
#include "faceoe_utl/faceoe_utl_defines.h"

#include <stddef.h>
#include <ctype.h>

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares str2 at each position of str1 in turn, so the cost is O(length of str1 * length of str2).
 */
const char_t* FACEOE_Utl_StrStr(const char_t* str1, const char_t* str2, bool_t caseInsensitive)
{
    uint64_t str1_base_index;
    uint64_t search_idx;

    LOGASSERT(str1 != NULL);
    LOGASSERT(str2 != NULL);

    str1_base_index = 0;

    if (str2[str1_base_index] == '\0')
    {
        return str1;
    }

    while (str1[str1_base_index] != '\0')
    {
        search_idx = 0;

        while (str2[search_idx] != '\0')
        {
            if(TRUE == caseInsensitive)
            {
                if (tolower((unsigned char)str2[search_idx]) != tolower((unsigned char)str1[str1_base_index + search_idx]))
                {
                    break;
                }
            }
            else
            {
                if(str2[search_idx] != str1[str1_base_index + search_idx])
                {
                    break;
                }
            }

            search_idx++;
        }

        if (str2[search_idx] == '\0')
        {
            return &(str1[str1_base_index]);
        }

        str1_base_index++;
    }

    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Unlike strchr, the search stops at sizeOfStr characters and never returns the
 *  terminator; for example, searching "1.5" for '.' returns &str[1], and
 *  searching it for '\0' returns NULL.
 */
const char_t* FACEOE_Utl_StrChr(const char_t* str, char_t character, uint64_t sizeOfStr)
{
    uint64_t index;
    bool_t   is_end;
    bool_t   is_match;

    LOGASSERT(NULL != str);

    for (index = 0; index < sizeOfStr; ++index)
    {
        is_end = ('\0' == str[index]);

        if (is_end)
        {
            return NULL;
        }

        is_match = (character == str[index]);

        if (is_match)
        {
            return &str[index];
        }
    }

    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The scan stops at dstSize - 1 characters so the part and its terminator fit in outStr.
 */
uint64_t FACEOE_Utl_StrSplit(const char_t* str, char_t* outStr, char_t delim, uint64_t dstSize)
{
    const char_t* iterator;
    uint64_t      current_idx;

    LOGASSERT(NULL != str);
    LOGASSERT(NULL != outStr);
    LOGASSERT(2 <= dstSize);

    iterator = str;
    if ((delim == *iterator) || ('\0' == *iterator))
    {
        return 0;
    }

    for (current_idx = 1; current_idx < (dstSize - 1); ++current_idx)
    {
        char_t current_character;

        ++iterator;

        current_character = *iterator;

        if ((delim == current_character) || ('\0' == current_character))
        {
            break;
        }
    }

    (void) memcpy(outStr, str, (size_t)current_idx);
    outStr[current_idx] = '\0';

    return (size_t)current_idx + 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Moves the trimmed text to the start of str with memmove.
 */
void FACEOE_Utl_StrTrim(char_t* str)
{
    char_t* str_trimmed_start;
    char_t* str_trimmed_end;
    char_t* str_trim_search;

    str_trimmed_end = NULL;
    str_trim_search = str;

    if(!str)
    {
        return;
    }

    while ('\0' != (*str_trim_search))
    {
        if (0 == isspace((unsigned char)(*str_trim_search)))
        {
            break;
        }
        ++str_trim_search;
    }

    str_trimmed_start = str_trim_search;

    while (*str_trim_search) 
    {
        if(!isspace((unsigned char)(*str_trim_search)))
        {
            str_trimmed_end = str_trim_search;
        }

        ++str_trim_search;
    }

    if (*str_trimmed_start && ((uint64_t)str_trimmed_end >= (uint64_t)str_trimmed_start))
    {
        uint64_t str_size = ((uint64_t)str_trimmed_end - (uint64_t)str_trimmed_start) + 1;
        (void)memmove(str, str_trimmed_start, str_size);
        str[str_size] = 0;
    }
}