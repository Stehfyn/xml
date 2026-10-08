#ifndef XML_OS_FILE_H
#define XML_OS_FILE_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

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
 * \brief Visits one filename while OS_File_EnumerateFiles is running.
 *
 * \param[in] fileName   UTF-8 filename without its folder
 * \param[in] context    Caller-owned context
 *
 * \retval    None
 */
typedef void (*OS_File_Visitor_t)(const char_t* fileName, void* context);

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
 * \brief Reads up to bufferSize bytes from a file into caller-owned storage.
 *
 * \par Details:
 *  The file is read in binary mode. A short read at end of file, including an empty file,
 *  succeeds. The buffer is not zero-terminated.
 *
 * \param[in]  filePath     Non-NULL UTF-8 path to the file
 * \param[out] buffer       Non-NULL destination buffer
 * \param[in]  bufferSize   Maximum number of bytes to read
 *
 * \retval     -1           The file could not be opened or read, or the byte count exceeds INT32_MAX.
 * \retval     0..INT32_MAX Number of bytes read successfully.
 */
int32_t OS_File_ReadFile(const char_t* filePath, byte_t* buffer, uint32_t bufferSize);

/*************************************************************************************************/
/**
 * \brief Writes a buffer to a file, replacing its previous contents.
 *
 * \par Details:
 *  The file is written in binary mode. An empty buffer creates or truncates the file.
 *
 * \param[in] dstPath    Non-NULL UTF-8 path to the file
 * \param[in] buf        Non-NULL source buffer
 * \param[in] numBytes   Number of bytes to write
 *
 * \retval    -1           The file could not be opened, fully written, or closed, or the byte count exceeds INT32_MAX.
 * \retval    0..INT32_MAX Number of bytes written successfully.
 */
int32_t OS_File_WriteFile(const char_t* dstPath, const byte_t* buf, uint32_t numBytes);

/*************************************************************************************************/
/**
 * \brief Visits regular files directly inside a folder.
 *
 * \par Details:
 *  The filename passed to visitor is valid only during that callback. An empty folder succeeds.
 *
 * \param[in] folderPath   Non-NULL UTF-8 folder path
 * \param[in] visitor      Non-NULL callback invoked for each file
 * \param[in] context      Caller-owned context passed to visitor
 *
 * \retval    TRUE         if the folder was enumerated successfully.
 * \retval    FALSE        if the folder could not be opened or fully enumerated.
 */
bool_t OS_File_EnumerateFiles(const char_t* folderPath, OS_File_Visitor_t visitor, void* context);

#endif /* XML_OS_FILE_H */
