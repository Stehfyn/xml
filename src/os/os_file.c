/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_file.h"

#include "os/os_assert.h"

#include <stdio.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#ifdef _WIN32
#define OS_FILE_WIDE_PATH_CHARS (4096)
#define OS_FILE_WIDE_MODE_CHARS (16)
#endif

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
 * \brief Opens a file using the UTF-8 path supplied by the caller.
 *
 * \param[in] filePath   File path
 * \param[in] mode       fopen mode
 *
 * \retval    non-NULL   Open file.
 * \retval    NULL       if opening the file fails.
 */
static FILE* OpenUtf8File(const char_t* filePath, const char_t* mode);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Converts UTF-8 paths at the Win32 boundary.
 */
static FILE* OpenUtf8File(const char_t* filePath, const char_t* mode)
{
    FILE* file;

#ifdef _WIN32
    wchar_t wide_path[OS_FILE_WIDE_PATH_CHARS];
    wchar_t wide_mode[OS_FILE_WIDE_MODE_CHARS];
    int32_t path_chars;
    int32_t mode_chars;

    file       = NULL;
    path_chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filePath, -1, wide_path, OS_FILE_WIDE_PATH_CHARS);
    mode_chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mode, -1, wide_mode, OS_FILE_WIDE_MODE_CHARS);
    if ((0 < path_chars) && (0 < mode_chars))
    {
        file = _wfopen(wide_path, wide_mode);
    }
#else
    file = fopen(filePath, mode);
#endif

    return file;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns a negative value for an open or read error; otherwise returns the exact byte count.
 */
int32_t OS_File_ReadFile(const char_t* filePath, byte_t* buffer, uint32_t bufferSize)
{
    FILE*    file;
    uint64_t bytes_read;
    int32_t  result;

    LOGASSERT(NULL != filePath);
    LOGASSERT(NULL != buffer);
    result = -1;
    file   = OpenUtf8File(filePath, "rb");
    if (NULL != file)
    {
        bytes_read = fread(buffer, 1, bufferSize, file);
        if (0 == ferror(file))
        {
            if (bytes_read <= INT32_MAX)
            {
                result = (int32_t)bytes_read;
            }
        }
        (void)fclose(file);
    }

    return result;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns a negative value for an open, write, or close error; otherwise returns the exact byte count.
 */
int32_t OS_File_WriteFile(const char_t* dstPath, const byte_t* buf, uint32_t numBytes)
{
    FILE*   file;
    size_t  bytes_written;
    int32_t result;

    LOGASSERT(NULL != dstPath);
    LOGASSERT(NULL != buf);
    result = -1;
    if (numBytes <= INT32_MAX)
    {
        file = OpenUtf8File(dstPath, "wb");
        if (NULL != file)
        {
            bytes_written = fwrite(buf, 1, numBytes, file);
            if (numBytes == bytes_written)
            {
                result = (int32_t)bytes_written;
            }
            if (0 != fclose(file))
            {
                result = -1;
            }
        }
    }

    return result;
}
