/*=================================================================================================
** 1. References
**
** Windows Installer: msiexec /i /passive /norestart on a MajorUpgrade package.
** WinHTTP: WinHttpCrackUrl, WinHttpOpen, WinHttpConnect, WinHttpOpenRequest, WinHttpReadData.
** CNG: BCryptOpenAlgorithmProvider, BCryptCreateHash, BCryptHashData, BCryptFinishHash.
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_update.h"

#include "os/os_assert.h"
#include "os/os_file.h"

#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define MANIFEST_MAX_BYTES        (65536)
#define PACKAGE_MAX_BYTES         (256u * 1024u * 1024u)
#define KEY_VERSION               ("version")
#define KEY_PACKAGE               ("msi")
#define KEY_SHA256                ("sha256")
#define KEY_NOTES                 ("notes")
#define HTTP_PREFIX               ("http://")
#define HTTPS_PREFIX              ("https://")
#define USER_AGENT                (L"ARINC661Editor/1.0")
#define PACKAGE_FILE_NAME         ("arinc661_editor_update.msi")
#define APPLY_COMMAND_FORMAT      ("/c \"msiexec.exe /i \"%s\" /passive /norestart && start \"\" \"%s\"\"")
#define APPLY_COMMAND_NO_RELAUNCH ("/c \"msiexec.exe /i \"%s\" /passive /norestart\"")
#define VERSION_PARTS             (4)
#define HEX_DIGITS_PER_BYTE       (2)
#define NT_SUCCESS_STATUS(status) (((LONG)(status)) >= 0)

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
 * \brief Fetches an http or https URL through WinHTTP.
 *
 * \param[in]  url        Source
 * \param[out] buffer     Bytes read
 * \param[in]  capacity   Bytes in buffer
 *
 * \retval     int32_t    Bytes read, or negative on any failure, a status other than 200, or a body that fills the buffer.
 */
static int32_t FetchHttp(const char_t* url, byte_t* buffer, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Copies the trimmed value of a key=value line.
 *
 * \param[in]  line       Start of the line
 * \param[in]  lineEnd    One past the last character of the line
 * \param[in]  key        Key wanted
 * \param[out] value      Value written, or untouched
 * \param[in]  capacity   Bytes in value
 *
 * \retval     TRUE       if the line carried the key.
 * \retval     FALSE      otherwise.
 */
static bool_t ReadLine(const char_t* line, const char_t* lineEnd, const char_t* key, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Reads 64 hex digits into 32 bytes.
 *
 * \param[in]  text     Hex text
 * \param[out] digest   Bytes written
 *
 * \retval     TRUE     if the text is exactly 64 hex digits.
 * \retval     FALSE    otherwise.
 */
static bool_t ReadHex(const char_t* text, byte_t* digest);

/*************************************************************************************************/
/**
 * \brief Resolves a package name against the manifest's location.
 *
 * \param[in]  baseUrl    Manifest URL or path
 * \param[in]  name       Package name, URL, or path
 * \param[out] result     Joined text
 * \param[in]  capacity   Bytes in result
 *
 * \retval     None
 */
static void JoinUrl(const char_t* baseUrl, const char_t* name, char_t* result, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Tells whether a source is an http or https URL.
 *
 * \param[in] url   Source
 *
 * \retval    TRUE    if it starts with http:// or https://, ignoring case.
 * \retval    FALSE   otherwise.
 */
static bool_t IsHttp(const char_t* url);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t IsHttp(const char_t* url)
{
    return (bool_t)((0 == _strnicmp(url, HTTP_PREFIX, strnlen(HTTP_PREFIX, sizeof(HTTP_PREFIX)))) || (0 == _strnicmp(url, HTTPS_PREFIX, strnlen(HTTPS_PREFIX, sizeof(HTTPS_PREFIX)))));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Every handle is closed on every path; the WinHTTP default proxy configuration applies.
 */
static int32_t FetchHttp(const char_t* url, byte_t* buffer, uint32_t capacity)
{
    wchar_t               wide_url[ARINC661_UPDATE_MAX_URL_BYTES];
    wchar_t               host[256];
    wchar_t               path[ARINC661_UPDATE_MAX_URL_BYTES];
    URL_COMPONENTS        parts;
    HINTERNET             session;
    HINTERNET             connection;
    HINTERNET             request;
    DWORD                 status;
    DWORD                 status_size;
    DWORD                 read_count;
    DWORD                 flags;
    uint32_t              used;
    int32_t               result;

    result = -1;
    if (0 >= MultiByteToWideChar(CP_UTF8, 0, url, -1, wide_url, (int)(sizeof(wide_url) / sizeof(wide_url[0]))))
    {
        return -1;
    }
    (void)memset(&parts, 0, sizeof(parts));
    parts.dwStructSize     = sizeof(parts);
    parts.lpszHostName     = host;
    parts.dwHostNameLength = (DWORD)(sizeof(host) / sizeof(host[0]));
    parts.lpszUrlPath      = path;
    parts.dwUrlPathLength  = (DWORD)(sizeof(path) / sizeof(path[0]));
    if (FALSE == WinHttpCrackUrl(wide_url, 0, 0, &parts))
    {
        return -1;
    }
    session = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (NULL == session)
    {
        return -1;
    }
    connection = WinHttpConnect(session, host, parts.nPort, 0);
    if (NULL != connection)
    {
        flags = 0;
        if (INTERNET_SCHEME_HTTPS == parts.nScheme)
        {
            flags = WINHTTP_FLAG_SECURE;
        }
        request = WinHttpOpenRequest(connection, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (NULL != request)
        {
            if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, NULL))
            {
                status      = 0;
                status_size = sizeof(status);
                if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX) && (HTTP_STATUS_OK == status))
                {
                    used   = 0;
                    result = 0;
                    while (used < capacity)
                    {
                        read_count = 0;
                        if (FALSE == WinHttpReadData(request, &buffer[used], capacity - used, &read_count))
                        {
                            result = -1;
                            break;
                        }
                        if (0 == read_count)
                        {
                            break;
                        }
                        used += read_count;
                    }
                    if ((0 <= result) && (used >= capacity))
                    {
                        // A full buffer may hide more data, so it is a failure, not a truncation
                        result = -1;
                    }
                    if (0 <= result)
                    {
                        result = (int32_t)used;
                    }
                }
            }
            WinHttpCloseHandle(request);
        }
        WinHttpCloseHandle(connection);
    }
    WinHttpCloseHandle(session);

    return result;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keys are case-insensitive. Spaces and tabs around key and value, and a trailing carriage
 *  return, are dropped; a value longer than capacity - 1 bytes is truncated.
 */
static bool_t ReadLine(const char_t* line, const char_t* lineEnd, const char_t* key, char_t* value, uint32_t capacity)
{
    const char_t* equals;
    const char_t* key_end;
    const char_t* value_start;
    const char_t* value_end;
    size_t        length;
    size_t        key_length;
    bool_t        is_trimmed;

    equals = line;
    while (equals < lineEnd)
    {
        if ('=' == (*equals))
        {
            break;
        }
        equals += 1;
    }
    if (equals >= lineEnd)
    {
        return FALSE;
    }
    key_end = equals;
    while (key_end > line)
    {
        is_trimmed = (' ' == key_end[-1]) || ('\t' == key_end[-1]);
        if (FALSE == is_trimmed)
        {
            break;
        }
        key_end -= 1;
    }
    while (line < key_end)
    {
        is_trimmed = (' ' == (*line)) || ('\t' == (*line));
        if (FALSE == is_trimmed)
        {
            break;
        }
        line += 1;
    }
    // A key longer than the text before the equals sign stops one past it and fails the length check
    key_length = strnlen(key, (size_t)(key_end - line) + 1);
    if (((size_t)(key_end - line) != key_length) || (0 != _strnicmp(line, key, key_length)))
    {
        return FALSE;
    }
    value_start = (equals + 1);
    while (value_start < lineEnd)
    {
        is_trimmed = (' ' == (*value_start)) || ('\t' == (*value_start));
        if (FALSE == is_trimmed)
        {
            break;
        }
        value_start += 1;
    }
    value_end = lineEnd;
    while (value_end > value_start)
    {
        is_trimmed = (' ' == value_end[-1]) || ('\t' == value_end[-1]) || ('\r' == value_end[-1]);
        if (FALSE == is_trimmed)
        {
            break;
        }
        value_end -= 1;
    }
    length = (size_t)(value_end - value_start);
    if (length >= capacity)
    {
        length = (capacity - 1);
    }
    (void)memcpy(value, value_start, length);
    value[length] = '\0';

    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadHex(const char_t* text, byte_t* digest)
{
    uint32_t index;
    uint32_t nibble;
    uint32_t accumulated;
    char_t   character;

    for (index = 0; index < (ARINC661_UPDATE_SHA256_BYTES * HEX_DIGITS_PER_BYTE); ++index)
    {
        character = text[index];
        if (('0' <= character) && (character <= '9'))
        {
            nibble = (uint32_t)(character - '0');
        }
        else if (('a' <= character) && (character <= 'f'))
        {
            nibble = (uint32_t)(character - 'a' + 10);
        }
        else if (('A' <= character) && (character <= 'F'))
        {
            nibble = (uint32_t)(character - 'A' + 10);
        }
        else
        {
            return FALSE;
        }
        if (0 == (index % HEX_DIGITS_PER_BYTE))
        {
            accumulated = (nibble << 4);
        }
        else
        {
            digest[index / HEX_DIGITS_PER_BYTE] = (byte_t)(accumulated | nibble);
        }
    }

    return (bool_t)('\0' == text[ARINC661_UPDATE_SHA256_BYTES * HEX_DIGITS_PER_BYTE]);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An http or https URL, a UNC path, or a path with a drive letter is taken as is; any other
 *  name replaces the last segment of the base.
 */
static void JoinUrl(const char_t* baseUrl, const char_t* name, char_t* result, uint32_t capacity)
{
    size_t base_length;
    size_t cut;

    if (IsHttp(name) || (('\\' == name[0]) && ('\\' == name[1])) || ((0 != name[0]) && (':' == name[1])))
    {
        (void)snprintf(result, capacity, "%s", name);
        return;
    }
    base_length = strnlen(baseUrl, ARINC661_UPDATE_MAX_URL_BYTES);
    cut         = base_length;
    while (0 < cut)
    {
        if ('/' == baseUrl[cut - 1])
        {
            break;
        }
        if ('\\' == baseUrl[cut - 1])
        {
            break;
        }
        cut -= 1;
    }
    (void)snprintf(result, capacity, "%.*s%s", (int)cut, baseUrl, name);
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Arinc661_Update_ParseVersion(const char_t* text, Arinc661_Update_Version_t* version)
{
    uint32_t parts[VERSION_PARTS];
    uint32_t part;
    uint32_t value;
    bool_t   has_digit;

    LOGASSERT(NULL != text);
    LOGASSERT(NULL != version);
    (void)memset(parts, 0, sizeof(parts));
    part      = 0;
    value     = 0;
    has_digit = FALSE;
    while ('\0' != (*text))
    {
        if (('0' <= (*text)) && ((*text) <= '9'))
        {
            value     = ((value * 10) + (uint32_t)((*text) - '0'));
            has_digit = TRUE;
        }
        else if (('.' == (*text)) && has_digit && (part < (VERSION_PARTS - 1)))
        {
            parts[part] = value;
            part       += 1;
            value       = 0;
            has_digit   = FALSE;
        }
        else
        {
            return FALSE;
        }
        text += 1;
    }
    if (FALSE == has_digit)
    {
        return FALSE;
    }
    parts[part]    = value;
    version->major = parts[0];
    version->minor = parts[1];
    version->patch = parts[2];
    version->build = parts[3];

    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
int32_t Arinc661_Update_CompareVersions(const Arinc661_Update_Version_t* left, const Arinc661_Update_Version_t* right)
{
    uint32_t left_parts[VERSION_PARTS];
    uint32_t right_parts[VERSION_PARTS];
    uint32_t index;

    LOGASSERT(NULL != left);
    LOGASSERT(NULL != right);
    left_parts[0]  = left->major;
    left_parts[1]  = left->minor;
    left_parts[2]  = left->patch;
    left_parts[3]  = left->build;
    right_parts[0] = right->major;
    right_parts[1] = right->minor;
    right_parts[2] = right->patch;
    right_parts[3] = right->build;
    for (index = 0; index < VERSION_PARTS; ++index)
    {
        if (left_parts[index] < right_parts[index])
        {
            return -1;
        }
        if (left_parts[index] > right_parts[index])
        {
            return 1;
        }
    }

    return 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Arinc661_Update_ParseManifest(const char_t* text, uint32_t sizeOfText, const char_t* baseUrl, Arinc661_Update_Manifest_t* manifest)
{
    const char_t* line;
    const char_t* line_end;
    const char_t* end;
    char_t        value[ARINC661_UPDATE_MAX_URL_BYTES];
    bool_t        has_version;
    bool_t        has_package;

    LOGASSERT(NULL != text);
    LOGASSERT(NULL != baseUrl);
    LOGASSERT(NULL != manifest);
    (void)memset(manifest, 0, sizeof(*manifest));
    has_version = FALSE;
    has_package = FALSE;
    line        = text;
    end         = (text + sizeOfText);
    while (line < end)
    {
        line_end = line;
        while (line_end < end)
        {
            if ('\n' == (*line_end))
            {
                break;
            }
            line_end += 1;
        }
        if (ReadLine(line, line_end, KEY_VERSION, value, sizeof(value)))
        {
            has_version = Arinc661_Update_ParseVersion(value, &manifest->version);
            (void)snprintf(manifest->versionText, sizeof(manifest->versionText), "%s", value);
        }
        else if (ReadLine(line, line_end, KEY_PACKAGE, value, sizeof(value)))
        {
            JoinUrl(baseUrl, value, manifest->packageUrl, sizeof(manifest->packageUrl));
            has_package = (bool_t)('\0' != value[0]);
        }
        else if (ReadLine(line, line_end, KEY_SHA256, value, sizeof(value)))
        {
            manifest->hasSha256 = ReadHex(value, manifest->sha256);
            if (FALSE == manifest->hasSha256)
            {
                return FALSE;
            }
        }
        else if (ReadLine(line, line_end, KEY_NOTES, value, sizeof(value)))
        {
            (void)snprintf(manifest->notes, sizeof(manifest->notes), "%s", value);
        }
        line = (line_end + 1);
    }

    return (bool_t)(has_version && has_package);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
int32_t Arinc661_Update_Fetch(const char_t* url, byte_t* buffer, uint32_t capacity)
{
    LOGASSERT(NULL != url);
    LOGASSERT(NULL != buffer);
    if (IsHttp(url))
    {
        return FetchHttp(url, buffer, capacity);
    }

    return OS_File_ReadFile(url, buffer, capacity);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The manifest buffer is on the heap so the function can run on a thread with a small stack.
 */
Arinc661_Update_Result_t Arinc661_Update_Check(const char_t* manifestUrl, const Arinc661_Update_Version_t* installed, Arinc661_Update_Manifest_t* manifest)
{
    byte_t*                  text;
    int32_t                  byte_count;
    Arinc661_Update_Result_t result;

    LOGASSERT(NULL != manifestUrl);
    LOGASSERT(NULL != installed);
    LOGASSERT(NULL != manifest);
    text = (byte_t*)malloc(MANIFEST_MAX_BYTES);
    if (NULL == text)
    {
        return ARINC661_UPDATE_RESULT_UNREACHABLE;
    }
    byte_count = Arinc661_Update_Fetch(manifestUrl, text, MANIFEST_MAX_BYTES - 1);
    if (0 >= byte_count)
    {
        free(text);
        return ARINC661_UPDATE_RESULT_UNREACHABLE;
    }
    text[byte_count] = 0;
    if (FALSE == Arinc661_Update_ParseManifest((const char_t*)text, (uint32_t)byte_count, manifestUrl, manifest))
    {
        free(text);
        return ARINC661_UPDATE_RESULT_MALFORMED;
    }
    free(text);
    result = ARINC661_UPDATE_RESULT_UP_TO_DATE;
    if (0 < Arinc661_Update_CompareVersions(&manifest->version, installed))
    {
        result = ARINC661_UPDATE_RESULT_AVAILABLE;
    }

    return result;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The package is held in memory and written only after it matches the manifest's digest, if any.
 */
Arinc661_Update_Result_t Arinc661_Update_Download(const Arinc661_Update_Manifest_t* manifest, char_t* path, uint32_t capacity)
{
    byte_t* package;
    byte_t  digest[ARINC661_UPDATE_SHA256_BYTES];
    char_t  temp_folder[MAX_PATH];
    int32_t byte_count;

    LOGASSERT(NULL != manifest);
    LOGASSERT(NULL != path);
    path[0] = '\0';
    if (0 == GetTempPathA((DWORD)sizeof(temp_folder), temp_folder))
    {
        return ARINC661_UPDATE_RESULT_UNREACHABLE;
    }
    (void)snprintf(path, capacity, "%s%s", temp_folder, PACKAGE_FILE_NAME);
    package = (byte_t*)malloc(PACKAGE_MAX_BYTES);
    if (NULL == package)
    {
        return ARINC661_UPDATE_RESULT_UNREACHABLE;
    }
    byte_count = Arinc661_Update_Fetch(manifest->packageUrl, package, PACKAGE_MAX_BYTES);
    if (0 >= byte_count)
    {
        free(package);
        return ARINC661_UPDATE_RESULT_UNREACHABLE;
    }
    if (manifest->hasSha256)
    {
        if ((FALSE == Arinc661_Update_Sha256(package, (uint32_t)byte_count, digest)) || (0 != memcmp(digest, manifest->sha256, sizeof(digest))))
        {
            free(package);
            return ARINC661_UPDATE_RESULT_CORRUPT;
        }
    }
    if (0 > OS_File_WriteFile(path, package, (uint32_t)byte_count))
    {
        free(package);
        return ARINC661_UPDATE_RESULT_UNREACHABLE;
    }
    free(package);

    return ARINC661_UPDATE_RESULT_DOWNLOADED;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  cmd.exe runs the installer and, when it exits with zero, starts relaunchPath if given. The
 *  shell runs without a console and keeps running after this process exits.
 */
bool_t Arinc661_Update_Apply(const char_t* packagePath, const char_t* relaunchPath)
{
    char_t              arguments[(ARINC661_UPDATE_MAX_URL_BYTES * 2) + 128];
    STARTUPINFOA        startup;
    PROCESS_INFORMATION process;
    char_t              command[64];
    bool_t              is_started;

    LOGASSERT(NULL != packagePath);
    if ((NULL != relaunchPath) && ('\0' != relaunchPath[0]))
    {
        (void)snprintf(arguments, sizeof(arguments), APPLY_COMMAND_FORMAT, packagePath, relaunchPath);
    }
    else
    {
        (void)snprintf(arguments, sizeof(arguments), APPLY_COMMAND_NO_RELAUNCH, packagePath);
    }
    (void)snprintf(command, sizeof(command), "cmd.exe");
    (void)memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    (void)memset(&process, 0, sizeof(process));
    // CreateProcess wants a writable command line: the program name and the arguments together
    (void)memmove(&arguments[strnlen(command, sizeof(command)) + 1], arguments, strnlen(arguments, sizeof(arguments)) + 1);
    (void)memcpy(arguments, command, strnlen(command, sizeof(command)));
    arguments[strnlen(command, sizeof(command))] = ' ';
    is_started = (bool_t)CreateProcessA(NULL, arguments, NULL, NULL, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &startup, &process);
    if (is_started)
    {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }

    return is_started;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Arinc661_Update_Sha256(const byte_t* bytes, uint32_t sizeOfBytes, byte_t* digest)
{
    BCRYPT_ALG_HANDLE  algorithm;
    BCRYPT_HASH_HANDLE hash;
    NTSTATUS           status;
    bool_t             is_done;

    LOGASSERT(NULL != bytes);
    LOGASSERT(NULL != digest);
    is_done   = FALSE;
    algorithm = NULL;
    hash      = NULL;
    status    = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (NT_SUCCESS_STATUS(status))
    {
        status = BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0);
        if (NT_SUCCESS_STATUS(status))
        {
            status = BCryptHashData(hash, (PUCHAR)bytes, (ULONG)sizeOfBytes, 0);
            if (NT_SUCCESS_STATUS(status))
            {
                status  = BCryptFinishHash(hash, digest, ARINC661_UPDATE_SHA256_BYTES, 0);
                is_done = (bool_t)NT_SUCCESS_STATUS(status);
            }
            (void)BCryptDestroyHash(hash);
        }
        (void)BCryptCloseAlgorithmProvider(algorithm, 0);
    }

    return is_done;
}
