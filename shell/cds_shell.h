#ifndef CDS_SHELL_H
#define CDS_SHELL_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include <windows.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Registry string forms of the class identifiers the installer registers; they match the CLSIDs in cds_shell.c. */
#define ARINC661_SHELL_CLSID_DEFINITION_OVERLAY_TEXT    L"{B490275C-C09C-4CEC-91AD-3A9EEDE63F4E}"
#define ARINC661_SHELL_CLSID_CONFIGURATION_OVERLAY_TEXT L"{A484FF8A-8CAE-41D2-B056-0B0BE5D884CC}"
#define ARINC661_SHELL_CLSID_CONTEXT_MENU_TEXT          L"{FB3A3DF2-C819-45BF-9043-3E7E6C2BA68C}"

/** Registry key, under HKCU or HKLM, whose InstallDir value the installer sets to the editor's folder. */
#define ARINC661_SHELL_SETTINGS_KEY                     L"Software\\FACEOE\\CDS Editor"
#define ARINC661_SHELL_INSTALL_DIR_VALUE                L"InstallDir"
#define ARINC661_SHELL_EDITOR_EXECUTABLE                L"cds-editor.exe"

/** Icon resource indices inside the DLL: the product icon Explorer shows, then the overlay badges. */
#define ARINC661_SHELL_ICON_PRODUCT                     (0)
#define ARINC661_SHELL_ICON_DEFINITION                  (1)
#define ARINC661_SHELL_ICON_CONFIGURATION               (2)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/** What the first bytes of a file say it is. */
typedef enum Arinc661_Shell_Kind_e
{
    ARINC661_SHELL_KIND_NONE,          ///< Not one of ours
    ARINC661_SHELL_KIND_DEFINITION,    ///< ARINC 661 Definition File, XML or binary
    ARINC661_SHELL_KIND_CONFIGURATION  ///< FACE display management configuration
} Arinc661_Shell_Kind_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Tells what a file is from its extension and its first bytes.
 *
 * \par Details:
 *  Only .xml and .bin files are read, and only their first 4096 bytes: a .bin Definition File
 *  starts with A661_DF_MAGIC_NUMBER, an .xml one has a661_df as its document element, and an
 *  .xml FACE configuration has configuration as its document element. Files with other
 *  extensions are NONE without being opened, so Explorer's listing stays fast.
 *
 * \param[in] path   Non-NULL full path
 *
 * \retval    Arinc661_Shell_Kind_t   The kind.
 */
Arinc661_Shell_Kind_t Arinc661_Shell_Sniff(const wchar_t* path);

#endif /* CDS_SHELL_H */
