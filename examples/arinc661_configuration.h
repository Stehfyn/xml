#ifndef ARINC661_CONFIGURATION_H
#define ARINC661_CONFIGURATION_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

extern "C"
{
#include "utl/utl_arinc661.h"
}

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES     (262144)
#define ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES  (512)
#define ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES     (260)
#define ARINC661_CONFIGURATION_MAX_SCREENS            (1024)
#define ARINC661_CONFIGURATION_MAX_LAYOUTS            (4096)
#define ARINC661_CONFIGURATION_MAX_WINDOWS            (4096)
#define ARINC661_CONFIGURATION_MAX_USER_APPLICATIONS  (1024)
#define ARINC661_CONFIGURATION_MAX_EXTERNAL_SOURCES   (1024)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Message shown in the editor status bar.
 */
typedef enum Arinc661_Configuration_Status_e
{
    ARINC661_CONFIGURATION_STATUS_NONE,
    ARINC661_CONFIGURATION_STATUS_FOLDER_PATH_REQUIRED,
    ARINC661_CONFIGURATION_STATUS_XML_FILE_REQUIRED,
    ARINC661_CONFIGURATION_STATUS_BUILD_FAILED,
    ARINC661_CONFIGURATION_STATUS_BUILT,
    ARINC661_CONFIGURATION_STATUS_WRITE_FAILED,
    ARINC661_CONFIGURATION_STATUS_SCREEN_ADDED,
    ARINC661_CONFIGURATION_STATUS_SCREEN_ADD_FAILED,
    ARINC661_CONFIGURATION_STATUS_LAYOUT_ADDED,
    ARINC661_CONFIGURATION_STATUS_LAYOUT_ADD_FAILED,
    ARINC661_CONFIGURATION_STATUS_WINDOW_ADDED,
    ARINC661_CONFIGURATION_STATUS_WINDOW_ADD_FAILED,
    ARINC661_CONFIGURATION_STATUS_SCREEN_REMOVED,
    ARINC661_CONFIGURATION_STATUS_LAYOUT_REMOVED,
    ARINC661_CONFIGURATION_STATUS_WINDOW_REMOVED,
    ARINC661_CONFIGURATION_STATUS_READ_FAILED,
    ARINC661_CONFIGURATION_STATUS_INPUT_TOO_LARGE,
    ARINC661_CONFIGURATION_STATUS_INPUT_EMPTY,
    ARINC661_CONFIGURATION_STATUS_LOADED,
    ARINC661_CONFIGURATION_STATUS_INVALID_XML
} Arinc661_Configuration_Status_t;

/*************************************************************************************************/
/**
 * \brief Kind of entity selected in the configuration hierarchy.
 */
typedef enum Arinc661_Configuration_SelectionKind_e
{
    ARINC661_CONFIGURATION_SELECTION_NONE,
    ARINC661_CONFIGURATION_SELECTION_SCREEN,
    ARINC661_CONFIGURATION_SELECTION_LAYOUT,
    ARINC661_CONFIGURATION_SELECTION_WINDOW
} Arinc661_Configuration_SelectionKind_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned document, conversion workspace, and editor presentation state.
 */
typedef struct Arinc661_Configuration_Editor_s
{
    Arinc661_Configuration_Status_t        status;                                                           ///< Current status message.
    bool_t                                 isLoaded;                                                         ///< TRUE after a successful read.
    Arinc661_Configuration_SelectionKind_t selectedKind;                                                     ///< Selected entity type.
    uint32_t                               selectedIndex;                                                    ///< Selected array index.
    float32_t                              totalScreenWidth;                                                 ///< Sum of drawable screen widths.
    float32_t                              maximumScreenHeight;                                              ///< Largest drawable screen height.
    uint32_t                               drawableScreenCount;                                              ///< Screens with positive pixel size.
    char_t                                 folderPath[ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES];         ///< Folder containing XML files.
    char_t                                 selectedXmlFileName[ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES];   ///< Selected XML filename.
    byte_t                                 inputBuffer[ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES + 1];       ///< Input file bytes.
    byte_t                                 outputBuffer[ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES];          ///< Built XML bytes.
    uint64_t                               workspace[UTL_ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)];       ///< XML conversion scratch space.
    Utl_Arinc661_Screen_t                  screens[ARINC661_CONFIGURATION_MAX_SCREENS];                      ///< Screen values.
    Utl_Arinc661_Layout_t                  layouts[ARINC661_CONFIGURATION_MAX_LAYOUTS];                      ///< Layout values.
    Utl_Arinc661_Window_t                  windows[ARINC661_CONFIGURATION_MAX_WINDOWS];                      ///< Window values.
    Utl_Arinc661_UserApplication_t         userApplications[ARINC661_CONFIGURATION_MAX_USER_APPLICATIONS];   ///< User application values.
    Utl_Arinc661_ExternalSource_t          externalSources[ARINC661_CONFIGURATION_MAX_EXTERNAL_SOURCES];     ///< External source values.
    Utl_Arinc661_Configuration_t           configuration;                                                    ///< Display management configuration values.
} Arinc661_Configuration_Editor_t;

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
 * \brief Initializes every editor field and its owned array storage.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_Initialize(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the hierarchy, scene, and inspector in one ImGui window.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawEditor(Arinc661_Configuration_Editor_t* editor);

#endif /* ARINC661_CONFIGURATION_H */
