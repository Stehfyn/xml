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
#include "faceoe/faceoe_displaymanagement.h"
}

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES     (262144)
#define ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES  (512)
#define ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES     (260)
#define ARINC661_CONFIGURATION_MAX_FILE_PATH_BYTES    (ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES + ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES + 1)
#define ARINC661_CONFIGURATION_MAX_SCREENS            (1024)
#define ARINC661_CONFIGURATION_MAX_LAYOUTS            (4096)
#define ARINC661_CONFIGURATION_MAX_WINDOWS            (4096)
#define ARINC661_CONFIGURATION_MAX_USER_APPLICATIONS  (1024)
#define ARINC661_CONFIGURATION_MAX_EXTERNAL_SOURCES   (1024)
#define ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS     (8)
#define ARINC661_CONFIGURATION_MAX_DEFINITION_FILES   (256)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief One open document and its conversion workspace.
 */
typedef struct Arinc661_Configuration_Document_s
{
    int32_t                                status;                                                           ///< Current status message.
    bool_t                                 isLoaded;                                                         ///< TRUE after a successful read.
    bool_t                                 isDirty;                                                          ///< TRUE when the loaded document has unsaved changes.
    int32_t                                selectedKind;                                                     ///< Selected entity type.
    uint32_t                               selectedIndex;                                                    ///< Selected array index.
    float32_t                              totalScreenWidth;                                                 ///< Sum of drawable screen widths.
    float32_t                              maximumScreenHeight;                                              ///< Largest drawable screen height.
    uint32_t                               drawableScreenCount;                                              ///< Screens with positive pixel size.
    char_t                                 documentName[ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES];          ///< Filename shown on the tab.
    char_t                                 documentPath[ARINC661_CONFIGURATION_MAX_FILE_PATH_BYTES];         ///< Path of the loaded document.
    byte_t                                 inputBuffer[ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES + 1];       ///< Input file bytes.
    byte_t                                 outputBuffer[ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES];          ///< Built XML bytes.
    uint64_t                               workspace[FACEOE_DISPLAYMANAGEMENT_WORKSPACE_BYTES / sizeof(uint64_t)];       ///< XML conversion scratch space.
    FACEOE_DisplayManagement_Screen_t                  screens[ARINC661_CONFIGURATION_MAX_SCREENS];                      ///< Screen values.
    FACEOE_DisplayManagement_Layout_t                  layouts[ARINC661_CONFIGURATION_MAX_LAYOUTS];                      ///< Layout values.
    FACEOE_DisplayManagement_Window_t                  windows[ARINC661_CONFIGURATION_MAX_WINDOWS];                      ///< Window values.
    FACEOE_DisplayManagement_UserApplication_t         userApplications[ARINC661_CONFIGURATION_MAX_USER_APPLICATIONS];   ///< User application values.
    FACEOE_DisplayManagement_ExternalSource_t          externalSources[ARINC661_CONFIGURATION_MAX_EXTERNAL_SOURCES];     ///< External source values.
    FACEOE_DisplayManagement_Configuration_t           configuration;                                                    ///< Display management configuration values.
} Arinc661_Configuration_Document_t;

/*************************************************************************************************/
/**
 * \brief File browser and fixed document slots for the editor.
 */
typedef struct Arinc661_Configuration_Editor_s
{
    int32_t                             status;                                                         ///< File browser status message.
    uint32_t                            activeDocumentIndex;                                            ///< Selected document slot.
    uint32_t                            requestedDocumentIndex;                                         ///< Tab to select after opening a file.
    uint32_t                            pendingCloseDocumentIndex;                                      ///< Document awaiting close confirmation.
    uint32_t                            pendingSaveDocumentIndex;                                       ///< Untitled document awaiting a filename.
    bool_t                              isSaveAllPending;                                               ///< TRUE while Save All is naming untitled documents.
    char_t                              folderPath[ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES];       ///< Folder containing XML files.
    char_t                              saveFolderPath[ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES];   ///< Folder chosen in the save dialog.
    char_t                              saveFileName[ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES];        ///< Filename chosen in the save dialog.
    uint32_t                            definitionFileCount;                                            ///< Definition Files offered for a user application's dFPath.
    char_t                              definitionFiles[ARINC661_CONFIGURATION_MAX_DEFINITION_FILES][ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES]; ///< Known Definition File names.
    Arinc661_Configuration_Document_t   documents[ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS];           ///< Document slots and XML workspaces.
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

/*************************************************************************************************/
/**
 * \brief Sets the folder the file list and new documents use.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     folderPath   Zero-terminated folder path
 *
 * \retval        None
 */
void Arinc661_Configuration_SetFolderPath(Arinc661_Configuration_Editor_t* editor, const char_t* folderPath);

/*************************************************************************************************/
/**
 * \brief Opens a configuration file from the folder, or selects it if it is already open.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     fileName   Filename inside the folder
 *
 * \retval        None
 */
void Arinc661_Configuration_LoadFile(Arinc661_Configuration_Editor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Starts a new untitled configuration document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_CreateDocument(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves the active document, asking for a filename when it is untitled.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_SaveActiveDocument(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves every changed document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_SaveAllDocuments(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Closes the active document, asking first when it has unsaved changes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_CloseActiveDocument(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reports whether a document is active, whether any is changed, and whether a slot is free.
 *
 * \param[in]  editor     Editor state
 * \param[out] isDirty    TRUE when any open document has unsaved changes; may be NULL
 * \param[out] hasFree    TRUE when another document can be opened; may be NULL
 *
 * \retval     TRUE       if a document is active.
 * \retval     FALSE      otherwise.
 */
bool_t Arinc661_Configuration_HasActiveDocument(const Arinc661_Configuration_Editor_t* editor, bool_t* isDirty, bool_t* hasFree);

/*************************************************************************************************/
/**
 * \brief Draws the document tab bar, including the unsaved-changes prompt.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawDocumentTabs(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's hierarchy tree into the current window.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawHierarchy(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's scene as a child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawScene(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's inspector as a child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawInspector(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the save-as dialog when a filename is pending.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawDialogs(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Returns the latest status message, or an empty string.
 *
 * \param[in] editor   Editor state
 *
 * \retval    const char_t*   Zero-terminated status text.
 */
const char_t* Arinc661_Configuration_StatusText(const Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Measures the hierarchy and inspector widths the active document needs.
 *
 * \param[in]  editor           Editor state
 * \param[out] hierarchyWidth   Minimum hierarchy width
 * \param[out] inspectorWidth   Inspector width
 *
 * \retval     None
 */
void Arinc661_Configuration_MeasurePanes(const Arinc661_Configuration_Editor_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*************************************************************************************************/
/**
 * \brief Replaces the list of Definition Files offered for a user application's dFPath.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_ClearDefinitionFiles(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a Definition File name to the dFPath choices.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     fileName   Filename of a valid ARINC 661 Definition File
 *
 * \retval        None
 */
void Arinc661_Configuration_AddDefinitionFile(Arinc661_Configuration_Editor_t* editor, const char_t* fileName);

#endif /* ARINC661_CONFIGURATION_H */
