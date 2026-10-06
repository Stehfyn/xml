#ifndef ARINC661_DEFINITION_EDITOR_H
#define ARINC661_DEFINITION_EDITOR_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

extern "C"
{
#include "arinc661/arinc661_definition.h"
}

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_DEFINITION_MAX_DOCUMENT_BYTES     (1048576)
#define ARINC661_DEFINITION_MAX_BLOCK_BYTES        (524288)
#define ARINC661_DEFINITION_MAX_FOLDER_PATH_BYTES  (512)
#define ARINC661_DEFINITION_MAX_XML_NAME_BYTES     (260)
#define ARINC661_DEFINITION_MAX_FILE_PATH_BYTES    (ARINC661_DEFINITION_MAX_FOLDER_PATH_BYTES + ARINC661_DEFINITION_MAX_XML_NAME_BYTES + 1)
#define ARINC661_DEFINITION_MAX_LAYERS             (128)
#define ARINC661_DEFINITION_MAX_WIDGETS            (2048)
#define ARINC661_DEFINITION_MAX_PROPERTIES         (16384)
#define ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS     (8)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief One open Definition File and its conversion workspace.
 */
typedef struct Arinc661_Definition_Document_s
{
    int32_t                 status;                                                   ///< Current status message.
    bool_t                  isLoaded;                                                 ///< TRUE after a successful read.
    bool_t                  isDirty;                                                  ///< TRUE when the loaded document has unsaved changes.
    bool_t                  isBinary;                                                 ///< TRUE when the document is saved in the binary form.
    int32_t                 selectedKind;                                             ///< Selected entity type.
    uint32_t                selectedIndex;                                            ///< Selected array index.
    char_t                  documentName[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];     ///< Filename shown on the tab.
    char_t                  documentPath[ARINC661_DEFINITION_MAX_FILE_PATH_BYTES];    ///< Path of the loaded document.
    byte_t                  inputBuffer[ARINC661_DEFINITION_MAX_DOCUMENT_BYTES + 1];  ///< Input file bytes.
    byte_t                  outputBuffer[ARINC661_DEFINITION_MAX_DOCUMENT_BYTES];     ///< Built file bytes.
    uint64_t                workspace[ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)];   ///< XML conversion scratch space.
    Arinc661_Layer_t        layers[ARINC661_DEFINITION_MAX_LAYERS];                   ///< Layer values.
    Arinc661_Widget_t       widgets[ARINC661_DEFINITION_MAX_WIDGETS];                 ///< Widget and extension values.
    Arinc661_Property_t     properties[ARINC661_DEFINITION_MAX_PROPERTIES];           ///< Property values.
    byte_t                  blocks[ARINC661_DEFINITION_MAX_BLOCK_BYTES];              ///< Binary symbol, picture, and animation law blocks.
    Arinc661_Definition_t   definition;                                               ///< Definition File values.
} Arinc661_Definition_Document_t;

/*************************************************************************************************/
/**
 * \brief File browser and fixed document slots for the editor.
 */
typedef struct Arinc661_Definition_Editor_s
{
    int32_t                          status;                                                      ///< File browser status message.
    uint32_t                         activeDocumentIndex;                                         ///< Selected document slot.
    uint32_t                         requestedDocumentIndex;                                      ///< Tab to select after opening a file.
    uint32_t                         pendingCloseDocumentIndex;                                   ///< Document awaiting close confirmation.
    uint32_t                         pendingSaveDocumentIndex;                                    ///< Untitled document awaiting a filename.
    bool_t                           isSaveAllPending;                                            ///< TRUE while Save All is naming untitled documents.
    char_t                           folderPath[ARINC661_DEFINITION_MAX_FOLDER_PATH_BYTES];       ///< Folder containing Definition Files.
    char_t                           saveFolderPath[ARINC661_DEFINITION_MAX_FOLDER_PATH_BYTES];   ///< Folder chosen in the save dialog.
    char_t                           saveFileName[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];        ///< Filename chosen in the save dialog.
    Arinc661_Definition_Document_t   documents[ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS];           ///< Document slots and workspaces.
} Arinc661_Definition_Editor_t;

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
void Arinc661_Definition_Initialize(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the hierarchy, scene, and inspector in one ImGui window.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DrawEditor(Arinc661_Definition_Editor_t* editor);

#endif /* ARINC661_DEFINITION_EDITOR_H */
