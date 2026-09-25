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

#define CONFIGURATION_MAX_DOCUMENT_BYTES   (262144)
#define CONFIGURATION_MAX_SCREENS           (1024)
#define CONFIGURATION_MAX_LAYOUTS           (4096)
#define CONFIGURATION_MAX_WINDOWS           (4096)
#define CONFIGURATION_MAX_USER_APPLICATIONS (1024)
#define CONFIGURATION_MAX_EXTERNAL_SOURCES  (1024)
#define CONFIGURATION_STATUS_BYTES          (256)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Kind of entity selected in the configuration hierarchy.
 */
typedef enum ConfigurationSelectionKind_e
{
    CONFIGURATION_SELECTION_NONE,
    CONFIGURATION_SELECTION_SCREEN,
    CONFIGURATION_SELECTION_LAYOUT,
    CONFIGURATION_SELECTION_WINDOW
} ConfigurationSelectionKind_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned document, conversion workspace, and editor presentation state.
 */
typedef struct ConfigurationEditor_s
{
    byte_t                             inputBuffer[CONFIGURATION_MAX_DOCUMENT_BYTES + 1];       ///< Input file bytes.
    byte_t                             outputBuffer[CONFIGURATION_MAX_DOCUMENT_BYTES];          ///< Built XML bytes.
    uint64_t                           workspace[UTL_ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)]; ///< XML conversion scratch space.
    Utl_Arinc661_Screen_t              screens[CONFIGURATION_MAX_SCREENS];                      ///< Screen values.
    Utl_Arinc661_Layout_t              layouts[CONFIGURATION_MAX_LAYOUTS];                      ///< Layout values.
    Utl_Arinc661_Window_t              windows[CONFIGURATION_MAX_WINDOWS];                      ///< Window values.
    Utl_Arinc661_UserApplication_t     userApplications[CONFIGURATION_MAX_USER_APPLICATIONS];  ///< User application values.
    Utl_Arinc661_ExternalSource_t      externalSources[CONFIGURATION_MAX_EXTERNAL_SOURCES];    ///< External source values.
    Utl_Arinc661_Configuration_t       configuration;                                          ///< Display management configuration values.
    ConfigurationSelectionKind_t       selectedKind;                                             ///< Selected entity type.
    uint32_t                           selectedIndex;                                            ///< Selected array index.
    float32_t                          totalScreenWidth;                                         ///< Sum of drawable screen widths.
    float32_t                          maximumScreenHeight;                                      ///< Largest drawable screen height.
    uint32_t                           drawableScreenCount;                                      ///< Screens with positive pixel size.
    char_t                             status[CONFIGURATION_STATUS_BYTES];                      ///< Current message.
    bool_t                             isLoaded;                                                ///< TRUE after a successful read.
} ConfigurationEditor_t;

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
 * \brief Binds fixed arrays and their capacities in a zero-initialized editor.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
void InitializeConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reloads the example file into the editor's typed configuration.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
void ReloadConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the hierarchy, visualizer, and inspector in one ImGui window.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
void DrawConfiguration(ConfigurationEditor_t* editor);

#endif /* ARINC661_CONFIGURATION_H */
