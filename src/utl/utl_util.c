#include "utl/utl_util.h"

#include "os/os_assert.h"
#include "utl/utl_defines.h"

#include <stddef.h>
#include <ctype.h>

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
char_t* Utl_Strtok(char_t* str, const char_t* delems, char_t** intPos)
{
    uint64_t str_idx;
    uint64_t str_len;
    uint64_t del_idx;
    uint64_t del_len;
    char_t*  tok;

    del_len = strlen(delems) + 1;
    tok     = NULL;

    for (str_idx = 0; str_idx < str_len; ++str_idx)
    {
        for(del_idx = 0; del_idx < del_len; ++del_idx)
        {
            if(delems[del_idx] == (*intPos)[str_idx])
            {
                break;
            }
        }

        if (del_idx == del_len)
        {
            tok = *intPos + str_idx;
            break;
        }
    }

    for (; str_idx < str_len; ++str_idx)
    {
        for(del_idx = 0; del_idx < del_len; ++del_idx)
        {
            if (delems[del_idx] == (*intPos)[str_idx])
            {
                break;
            }
        }

        if (del_idx != del_len)
        {
            (*intPos) += str_idx;

            if (delems[del_idx] != '\0')
            {
                **intPos = '\0';
                (*intPos)++;
            }

            break;
        }
    }

    return tok;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
const char_t* Utl_StrStr(const char_t* str1, const char_t* str2, bool_t caseInsensitive)
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
                if (tolower(str2[search_idx]) != tolower(str1[str1_base_index + search_idx]))
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
 *  None
 */
uint64_t Utl_StrSplit(const char_t* str, char_t* outStr, char_t delim, uint64_t dstSize)
{
    const char_t* iterator;
    uint64_t      current_idx;

    LOGASSERT(NULL != str);
    LOGASSERT(NULL != outStr);
    LOGASSERT(2 <= dstSize);

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
 *  None
 */
void Utl_StrTrim(char_t* str)
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

    while (*str_trim_search && isspace(*str_trim_search))
    {
        ++str_trim_search;
    }

    str_trimmed_start = str_trim_search;

    while (*str_trim_search) 
    {
        if(!isspace(*str_trim_search))
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