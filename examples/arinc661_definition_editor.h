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
#include "arinc661/arinc661_scene.h"
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
#define ARINC661_DEFINITION_MAX_SCENE_COMMANDS     (65536)
#define ARINC661_DEFINITION_MAX_SCENE_POINTS       (524288)
#define ARINC661_DEFINITION_MAX_SCENE_TEXT_BYTES   (1048576)
#define ARINC661_DEFINITION_NO_DOCUMENT            (UINT32_MAX)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief One open Definition File and its conversion workspace.
 */
struct Arinc661_Definition_Editor_s;

typedef struct Arinc661_Definition_Document_s
{
    struct Arinc661_Definition_Editor_s* owner;                                        ///< Editor holding this document.
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
struct Arinc661_Context_s;

typedef struct Arinc661_Definition_Editor_s
{
    struct Arinc661_Context_s*       context;                                                     ///< Shared application context, or NULL.
    FACEOE_Gfx_DrawList_t            drawList;                                                    ///< Scene draw list.
    FACEOE_Gfx_Command_t             sceneCommands[ARINC661_DEFINITION_MAX_SCENE_COMMANDS];       ///< Scene command storage.
    FACEOE_Gfx_Point_t               scenePoints[ARINC661_DEFINITION_MAX_SCENE_POINTS];           ///< Scene point storage.
    char_t                           sceneText[ARINC661_DEFINITION_MAX_SCENE_TEXT_BYTES];         ///< Scene text storage.
    Arinc661_Scene_Bounds_t          sceneBounds[ARINC661_DEFINITION_MAX_WIDGETS];                ///< Widget bounds of the last scene.
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

/*************************************************************************************************/
/**
 * \brief Sets the folder the file list and new documents use.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     folderPath   Zero-terminated folder path
 *
 * \retval        None
 */
void Arinc661_Definition_SetFolderPath(Arinc661_Definition_Editor_t* editor, const char_t* folderPath);

/*************************************************************************************************/
/**
 * \brief Opens a Definition File from the folder, or selects it if it is already open.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     fileName   Filename inside the folder
 *
 * \retval        None
 */
void Arinc661_Definition_LoadFile(Arinc661_Definition_Editor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Starts a new untitled Definition File.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_CreateDocument(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves the active document, asking for a filename when it is untitled.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_SaveActiveDocument(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves every changed document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_SaveAllDocuments(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Closes the active document, asking first when it has unsaved changes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_CloseActiveDocument(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reports whether a document is active, whether any is changed, and whether a slot is free.
 *
 * \param[in]  editor    Editor state
 * \param[out] isDirty   TRUE when any open document has unsaved changes; may be NULL
 * \param[out] hasFree   TRUE when another document can be opened; may be NULL
 *
 * \retval     TRUE      if a document is active.
 * \retval     FALSE     otherwise.
 */
bool_t Arinc661_Definition_HasActiveDocument(const Arinc661_Definition_Editor_t* editor, bool_t* isDirty, bool_t* hasFree);

/*************************************************************************************************/
/**
 * \brief Draws the document tab bar, including the unsaved-changes prompt.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DrawDocumentTabs(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's hierarchy tree into the current window.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DrawHierarchy(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's scene as a child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DrawScene(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's inspector as a child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DrawInspector(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the save-as dialog when a filename is pending.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DrawDialogs(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Returns the latest status message, or an empty string.
 *
 * \param[in] editor   Editor state
 *
 * \retval    const char_t*   Zero-terminated status text.
 */
const char_t* Arinc661_Definition_StatusText(const Arinc661_Definition_Editor_t* editor);

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
void Arinc661_Definition_MeasurePanes(const Arinc661_Definition_Editor_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*************************************************************************************************/
/**
 * \brief Tells whether a file holds an ARINC 661 Definition File, in XML or binary form.
 *
 * \param[in] bytes       First bytes of the file
 * \param[in] byteCount   Number of bytes available
 *
 * \retval    TRUE        if the bytes start a binary Definition File or an a661_df document.
 * \retval    FALSE       otherwise.
 */
bool_t Arinc661_Definition_IsDefinitionFile(const byte_t* bytes, uint64_t byteCount);

/*************************************************************************************************/
/**
 * \brief Attaches the shared application context.
 *
 * \param[in,out] editor    Editor state
 * \param[in]     context   Context owned by the host
 *
 * \retval        None
 */
void Arinc661_Definition_SetContext(Arinc661_Definition_Editor_t* editor, struct Arinc661_Context_s* context);

/*************************************************************************************************/
/**
 * \brief Finds an open document by file name, ignoring case.
 *
 * \param[in] editor     Editor state
 * \param[in] fileName   File name as shown on its tab
 *
 * \retval    uint32_t   Document slot, or ARINC661_DEFINITION_NO_DOCUMENT.
 */
uint32_t Arinc661_Definition_FindDocument(const Arinc661_Definition_Editor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Returns an open document's Definition File values.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    const Arinc661_Definition_t*   Values, or NULL when the slot is empty.
 */
const Arinc661_Definition_t* Arinc661_Definition_Document(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Returns an open document's file name.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    const char_t*   Name, or NULL when the slot is empty.
 */
const char_t* Arinc661_Definition_DocumentName(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Reads an open document's ApplicationId model property.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[out] applicationId   Value
 *
 * \retval     TRUE            if the property exists and is numeric.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Definition_DocumentApplicationId(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, int64_t* applicationId);

/*************************************************************************************************/
/**
 * \brief Selects a document's tab.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 *
 * \retval        None
 */
void Arinc661_Definition_SelectDocument(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Records every layer of an open document into a draw list, in layer order.
 *
 * \par Details:
 *  The draw list's current transform maps 0.01 mm layer units to output units; hidden
 *  widgets, closed pop-ups, names, and chips are left out, as on a CDS.
 *
 * \param[in]     editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in,out] drawList        Target draw list
 * \param[in]     fontSize        Host font size in output units
 *
 * \retval        TRUE            if the document is open and at least one layer was recorded.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_Definition_RecordDocument(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize);

#endif /* ARINC661_DEFINITION_EDITOR_H */
