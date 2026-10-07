/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "../examples/arinc661_update.h"

#include "os/os_file.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

#define MANIFEST_FILE ("arinc661_update_test_manifest.txt")
#define PACKAGE_FILE  ("arinc661_update_test_package.bin")

/*************************************************************************************************/
/**
 * \brief Runs the updater tests.
 *
 * \retval None
 */
void RunArinc661UpdateTests(void);

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Version parsing and ordering, manifest parsing with a relative package and a digest, then a
 *  check and a download through the file path branch against files written here, including a
 *  digest mismatch. The network branch is not exercised.
 */
void RunArinc661UpdateTests(void)
{
    Arinc661_Update_Version_t  left;
    Arinc661_Update_Version_t  right;
    Arinc661_Update_Manifest_t manifest;
    byte_t                     package[64];
    byte_t                     digest[ARINC661_UPDATE_SHA256_BYTES];
    char_t                     manifest_text[512];
    char_t                     hex[(ARINC661_UPDATE_SHA256_BYTES * 2) + 1];
    char_t                     path[512];
    uint32_t                   index;
    int32_t                    byte_count;

    /* Versions */
    assert(Arinc661_Update_ParseVersion("1.2.3.4", &left));
    assert((1 == left.major) && (2 == left.minor) && (3 == left.patch) && (4 == left.build));
    assert(Arinc661_Update_ParseVersion("2", &right));
    assert((2 == right.major) && (0 == right.minor) && (0 == right.patch) && (0 == right.build));
    assert(0 > Arinc661_Update_CompareVersions(&left, &right));
    assert(0 < Arinc661_Update_CompareVersions(&right, &left));
    assert(0 == Arinc661_Update_CompareVersions(&left, &left));
    assert(Arinc661_Update_ParseVersion("1.2.3.5", &right));
    assert(0 > Arinc661_Update_CompareVersions(&left, &right));
    assert(FALSE == Arinc661_Update_ParseVersion("", &right));
    assert(FALSE == Arinc661_Update_ParseVersion("1.", &right));
    assert(FALSE == Arinc661_Update_ParseVersion("1.2.3.4.5", &right));
    assert(FALSE == Arinc661_Update_ParseVersion("1.a", &right));

    /* Package bytes and their digest */
    for (index = 0; index < sizeof(package); ++index)
    {
        package[index] = (byte_t)(index * 7);
    }
    assert(Arinc661_Update_Sha256(package, sizeof(package), digest));
    for (index = 0; index < ARINC661_UPDATE_SHA256_BYTES; ++index)
    {
        (void)snprintf(&hex[index * 2], 3, "%02x", (unsigned)digest[index]);
    }
    assert(0 <= OS_File_WriteFile(PACKAGE_FILE, package, sizeof(package)));

    /* Manifest with a relative package name resolves against the manifest's folder */
    (void)snprintf(manifest_text, sizeof(manifest_text), "# comment\nversion = 9.1.0.0\r\nmsi=%s\nSHA256=%s\nnotes=Faster layers\n", PACKAGE_FILE, hex);
    assert(Arinc661_Update_ParseManifest(manifest_text, (uint32_t)strlen(manifest_text), "C:\\share\\updates\\manifest.txt", &manifest));
    assert((9 == manifest.version.major) && (1 == manifest.version.minor));
    assert(0 == strcmp("9.1.0.0", manifest.versionText));
    assert(0 == strcmp("C:\\share\\updates\\arinc661_update_test_package.bin", manifest.packageUrl));
    assert(manifest.hasSha256);
    assert(0 == memcmp(digest, manifest.sha256, sizeof(digest)));
    assert(0 == strcmp("Faster layers", manifest.notes));
    assert(Arinc661_Update_ParseManifest("version=1\nmsi=https://host/a.msi\n", 33, "https://host/m.txt", &manifest));
    assert(0 == strcmp("https://host/a.msi", manifest.packageUrl));
    assert(FALSE == manifest.hasSha256);
    assert(FALSE == Arinc661_Update_ParseManifest("msi=a.msi\n", 10, "x", &manifest));
    assert(FALSE == Arinc661_Update_ParseManifest("version=1\n", 10, "x", &manifest));
    assert(FALSE == Arinc661_Update_ParseManifest("version=1\nmsi=a\nsha256=zz\n", 26, "x", &manifest));

    /* Check through a file manifest beside the package in the working folder */
    assert(0 <= OS_File_WriteFile(MANIFEST_FILE, (const byte_t*)manifest_text, (uint32_t)strlen(manifest_text)));
    assert(Arinc661_Update_ParseVersion("1.0.0.0", &left));
    assert(ARINC661_UPDATE_RESULT_AVAILABLE == Arinc661_Update_Check(MANIFEST_FILE, &left, &manifest));
    assert(Arinc661_Update_ParseVersion("9.1.0.0", &left));
    assert(ARINC661_UPDATE_RESULT_UP_TO_DATE == Arinc661_Update_Check(MANIFEST_FILE, &left, &manifest));
    assert(ARINC661_UPDATE_RESULT_UNREACHABLE == Arinc661_Update_Check("no_such_manifest.txt", &left, &manifest));

    /* Download verifies the digest and writes the package to the temporary folder */
    assert(ARINC661_UPDATE_RESULT_DOWNLOADED == Arinc661_Update_Download(&manifest, path, sizeof(path)));
    byte_count = OS_File_ReadFile(path, digest, sizeof(digest));
    assert((int32_t)sizeof(digest) == byte_count);
    assert(0 == memcmp(digest, package, sizeof(digest)));
    (void)remove(path);
    manifest.sha256[0] ^= 0xFF;
    assert(ARINC661_UPDATE_RESULT_CORRUPT == Arinc661_Update_Download(&manifest, path, sizeof(path)));
    (void)snprintf(manifest.packageUrl, sizeof(manifest.packageUrl), "no_such_package.msi");
    assert(ARINC661_UPDATE_RESULT_UNREACHABLE == Arinc661_Update_Download(&manifest, path, sizeof(path)));

    (void)remove(MANIFEST_FILE);
    (void)remove(PACKAGE_FILE);
}
