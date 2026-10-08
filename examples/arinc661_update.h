#ifndef ARINC661_UPDATE_H
#define ARINC661_UPDATE_H

/*=================================================================================================
** 1. References
**
** Windows Installer: msiexec /i with a MajorUpgrade package replaces the installed product.
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_UPDATE_MAX_URL_BYTES       (1024)
#define ARINC661_UPDATE_MAX_NOTE_BYTES      (512)
#define ARINC661_UPDATE_MAX_VERSION_BYTES   (32)
#define ARINC661_UPDATE_SHA256_BYTES        (32)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/** A four-part product version. */
typedef struct Arinc661_Update_Version_s
{
    uint32_t major; ///< First number
    uint32_t minor; ///< Second number
    uint32_t patch; ///< Third number
    uint32_t build; ///< Fourth number
} Arinc661_Update_Version_t;

/** What an update manifest says. */
typedef struct Arinc661_Update_Manifest_s
{
    Arinc661_Update_Version_t version;                                        ///< Version offered
    char_t                    versionText[ARINC661_UPDATE_MAX_VERSION_BYTES]; ///< Version as written
    char_t                    packageUrl[ARINC661_UPDATE_MAX_URL_BYTES];      ///< Where the MSI is
    char_t                    notes[ARINC661_UPDATE_MAX_NOTE_BYTES];          ///< One line of release notes, or empty
    byte_t                    sha256[ARINC661_UPDATE_SHA256_BYTES];           ///< Expected digest of the MSI
    bool_t                    hasSha256;                                      ///< TRUE when sha256 was given
} Arinc661_Update_Manifest_t;

/** Outcome of a check or a download. */
typedef enum Arinc661_Update_Result_e
{
    ARINC661_UPDATE_RESULT_UP_TO_DATE,   ///< Installed version is the newest
    ARINC661_UPDATE_RESULT_AVAILABLE,    ///< A newer version is offered
    ARINC661_UPDATE_RESULT_UNREACHABLE,  ///< The manifest could not be fetched
    ARINC661_UPDATE_RESULT_MALFORMED,    ///< The manifest could not be read
    ARINC661_UPDATE_RESULT_DOWNLOADED,   ///< The package is on disk and verified
    ARINC661_UPDATE_RESULT_CORRUPT       ///< The package failed its digest
} Arinc661_Update_Result_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Parses a dotted version of up to four numbers.
 *
 * \param[in]  text      Version text
 * \param[out] version   Parsed numbers; missing parts are zero
 *
 * \retval     TRUE      if at least one number was read and nothing else followed.
 * \retval     FALSE     otherwise.
 */
bool_t Arinc661_Update_ParseVersion(const char_t* text, Arinc661_Update_Version_t* version);

/*************************************************************************************************/
/**
 * \brief Orders two versions.
 *
 * \param[in] left    One version
 * \param[in] right   Another
 *
 * \retval    int32_t   Negative, zero, or positive as left is below, equal to, or above right.
 */
int32_t Arinc661_Update_CompareVersions(const Arinc661_Update_Version_t* left, const Arinc661_Update_Version_t* right);

/*************************************************************************************************/
/**
 * \brief Reads a manifest from its text.
 *
 * \par Details:
 *  Lines of key=value: version (required), msi (required; a URL, a UNC path, or a path relative
 *  to the manifest), sha256 (optional, 64 hex digits), notes (optional). Other lines are ignored.
 *
 * \param[in]  text       Manifest text
 * \param[in]  byteCount  Bytes in text
 * \param[in]  baseUrl    Where the manifest came from, for a relative msi
 * \param[out] manifest   Parsed manifest
 *
 * \retval     TRUE       if version and msi were present and well formed.
 * \retval     FALSE      otherwise.
 */
bool_t Arinc661_Update_ParseManifest(const char_t* text, uint32_t byteCount, const char_t* baseUrl, Arinc661_Update_Manifest_t* manifest);

/*************************************************************************************************/
/**
 * \brief Fetches a URL, a UNC path, or a local path into a buffer.
 *
 * \par Details:
 *  http and https go through WinHTTP with the system proxy; anything else is read as a file.
 *
 * \param[in]  url        Source
 * \param[out] buffer     Bytes read
 * \param[in]  capacity   Bytes in buffer
 *
 * \retval     int32_t    Bytes read, or negative when the source could not be read or was too big.
 */
int32_t Arinc661_Update_Fetch(const char_t* url, byte_t* buffer, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Fetches the manifest and compares it with the running version.
 *
 * \param[in]  manifestUrl   Manifest source
 * \param[in]  installed     Running version
 * \param[out] manifest      What was offered, valid when AVAILABLE or UP_TO_DATE
 *
 * \retval     Arinc661_Update_Result_t   UP_TO_DATE, AVAILABLE, UNREACHABLE, or MALFORMED.
 */
Arinc661_Update_Result_t Arinc661_Update_Check(const char_t* manifestUrl, const Arinc661_Update_Version_t* installed, Arinc661_Update_Manifest_t* manifest);

/*************************************************************************************************/
/**
 * \brief Downloads the package of a manifest to the temporary folder and verifies its digest.
 *
 * \param[in]  manifest   What to fetch
 * \param[out] path       Where the package was written
 * \param[in]  capacity   Bytes in path
 *
 * \retval     Arinc661_Update_Result_t   DOWNLOADED, UNREACHABLE, or CORRUPT.
 */
Arinc661_Update_Result_t Arinc661_Update_Download(const Arinc661_Update_Manifest_t* manifest, char_t* path, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Starts the installer on a downloaded package and arranges a relaunch.
 *
 * \par Details:
 *  Runs msiexec /i passively in a detached command shell that relaunches the executable when
 *  the install succeeds. The caller must exit right after so the installer can replace files.
 *
 * \param[in] packagePath   Downloaded MSI
 * \param[in] relaunchPath  Executable to start afterwards, or NULL
 *
 * \retval    TRUE          if the shell was started.
 * \retval    FALSE         otherwise.
 */
bool_t Arinc661_Update_Apply(const char_t* packagePath, const char_t* relaunchPath);

/*************************************************************************************************/
/**
 * \brief Computes SHA-256 of a buffer.
 *
 * \param[in]  bytes       Data
 * \param[in]  byteCount   Bytes in data
 * \param[out] digest      ARINC661_UPDATE_SHA256_BYTES written
 *
 * \retval     TRUE        if the digest was computed.
 * \retval     FALSE       otherwise.
 */
bool_t Arinc661_Update_Sha256(const byte_t* bytes, uint32_t byteCount, byte_t* digest);

#ifdef __cplusplus
}
#endif

#endif /* ARINC661_UPDATE_H */
