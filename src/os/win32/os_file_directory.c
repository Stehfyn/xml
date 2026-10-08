/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_file.h"

#include "os/os_assert.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define OS_FILE_SEARCH_PATH_CHARS (4096)
#define OS_FILE_UTF8_NAME_BYTES   (MAX_PATH * 4)

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
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Visits every non-directory entry. Converts the folder path to UTF-16 and each filename to
 *  UTF-8; a filename that fails conversion is skipped and makes the result FALSE.
 */
bool_t OS_File_EnumerateFiles(const char_t* folderPath, OS_File_Visitor_t visitor, void* context)
{
    WIN32_FIND_DATAW find_data;
    HANDLE           find_handle;
    wchar_t          search_path[OS_FILE_SEARCH_PATH_CHARS];
    char_t           file_name[OS_FILE_UTF8_NAME_BYTES];
    int32_t          path_chars;
    int32_t          name_bytes;
    DWORD            error;
    bool_t           result;
    bool_t           has_next;
    bool_t           ends_with_backslash;
    bool_t           ends_with_slash;
    bool_t           needs_separator;

    LOGASSERT(NULL != folderPath);
    LOGASSERT(NULL != visitor);
    result = FALSE;
    if ('\0' != folderPath[0])
    {
        path_chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, folderPath, -1, search_path, OS_FILE_SEARCH_PATH_CHARS - 2);
        if (0 < path_chars)
        {
            ends_with_backslash = (L'\\' == search_path[path_chars - 2]);
            ends_with_slash     = (L'/' == search_path[path_chars - 2]);
            needs_separator     = (FALSE == ends_with_backslash) && (FALSE == ends_with_slash);
            if (needs_separator)
            {
                search_path[path_chars - 1] = L'\\';
                path_chars += 1;
            }
            search_path[path_chars - 1] = L'*';
            search_path[path_chars]     = L'\0';

            find_handle = FindFirstFileW(search_path, &find_data);
            if (INVALID_HANDLE_VALUE == find_handle)
            {
                error  = GetLastError();
                result = (ERROR_FILE_NOT_FOUND == error);
            }
            else
            {
                result = TRUE;
                do
                {
                    if (0 == (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        name_bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, find_data.cFileName, -1, file_name, (int32_t)sizeof(file_name), NULL, NULL);
                        if (0 < name_bytes)
                        {
                            visitor(file_name, context);
                        }
                        else
                        {
                            result = FALSE;
                        }
                    }
                    has_next = (FALSE != FindNextFileW(find_handle, &find_data));
                } while (has_next);
                if (ERROR_NO_MORE_FILES != GetLastError())
                {
                    result = FALSE;
                }
                if (FALSE == FindClose(find_handle))
                {
                    result = FALSE;
                }
            }
        }
    }

    return result;
}
