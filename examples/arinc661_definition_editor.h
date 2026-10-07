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
#define ARINC661_DEFINITION_MAX_HISTORY            (64)
#define ARINC661_DEFINITION_MAX_FILTER_BYTES       (64)

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
    void*                   undoStack[ARINC661_DEFINITION_MAX_HISTORY];               ///< Model snapshots older than the shadow, oldest first.
    void*                   redoStack[ARINC661_DEFINITION_MAX_HISTORY];               ///< Model snapshots undone, oldest first.
    void*                   shadow;                                                   ///< Snapshot of the model as last committed.
    float32_t               viewZoom;                                                 ///< Scene zoom factor over the fitted scale; zero until first drawn.
    float32_t               viewPanX;                                                 ///< Scene pan in screen pixels.
    float32_t               viewPanY;                                                 ///< Scene pan in screen pixels.
    uint32_t                undoCount;                                                ///< Snapshots on the undo stack.
    uint32_t                redoCount;                                                ///< Snapshots on the redo stack.
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
    uint32_t                         sceneBoundsCount;                                            ///< Bounds recorded by the last scene.
    int32_t                          dragMode;                                                    ///< Scene drag in progress: none, move, or a resize corner.
    uint32_t                         dragWidget;                                                  ///< Widget being dragged.
    float32_t                        dragStartX;                                                  ///< Mouse X at drag start.
    float32_t                        dragStartY;                                                  ///< Mouse Y at drag start.
    float32_t                        dragOriginX;                                                 ///< PosX at drag start.
    float32_t                        dragOriginY;                                                 ///< PosY at drag start.
    float32_t                        dragOriginWidth;                                             ///< SizeX at drag start.
    float32_t                        dragOriginHeight;                                            ///< SizeY at drag start.
    float32_t                        dragScale;                                                   ///< Screen pixels per layer unit during the drag.
    bool_t                           hasDragged;                                                  ///< TRUE once the drag moved the mouse.
    bool_t                           isSnapping;                                                  ///< TRUE to snap moves and sizes to whole millimetres.
    bool_t                           showsGrid;                                                   ///< TRUE to draw a millimetre grid behind the layer.
    uint32_t                         dropWidget;                                                  ///< Widget dropped onto a hierarchy node this frame, or UINT32_MAX.
    uint32_t                         dropParent;                                                  ///< Node it was dropped on, or UINT32_MAX for the layer.
    uint32_t                         dropLayer;                                                   ///< Layer of that node.
    uint32_t                         createConfiguration;                                         ///< Configuration slot chosen for a new user application.
    uint32_t                         createWindow;                                                ///< Window array index chosen for it.
    char_t                           dropType[ARINC661_MAX_NAME_BYTES];                           ///< Palette type dropped on a hierarchy node this frame, or empty.
    uint32_t                         dropTypeParent;                                              ///< Node it was dropped on, or UINT32_MAX for the layer.
    uint32_t                         dropTypeLayer;                                               ///< Layer of that node.
    char_t                           addFilter[ARINC661_DEFINITION_MAX_FILTER_BYTES];             ///< Type filter typed in the add pop-up.
    uint32_t                         addLayer;                                                    ///< Layer the add pop-up targets.
    uint32_t                         addParent;                                                   ///< Parent widget the add pop-up targets.
    uint32_t                         addHit;                                                      ///< Widget under the cursor when the add pop-up opened.
    float32_t                        addX;                                                        ///< Layer X where the add pop-up opened.
    float32_t                        addY;                                                        ///< Layer Y where the add pop-up opened.
    int32_t                          status;                                                      ///< File browser status message.
    uint32_t                         activeDocumentIndex;                                         ///< Selected document slot.
    uint32_t                         requestedDocumentIndex;                                      ///< Tab to select after opening a file.
    uint32_t                         pendingCloseDocumentIndex;                                   ///< Document awaiting close confirmation.
    uint32_t                         pendingSaveDocumentIndex;                                    ///< Untitled document awaiting a filename.
    bool_t                           isSaveAllPending;                                            ///< TRUE while Save All is naming untitled documents.
    char_t                           renamedFrom[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];         ///< Name a document had before its last Save As, or empty.
    char_t                           renamedTo[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];           ///< Name it has now.
    char_t                           statusNote[ARINC661_DEFINITION_MAX_XML_NAME_BYTES];          ///< Host note appended to the status text, or empty.
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

/*************************************************************************************************/
/**
 * \brief Reverts the active document to the state before its latest change.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_Undo(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reapplies the active document's latest undone change.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_Redo(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reports whether the active document can undo or redo.
 *
 * \param[in]  editor    Editor state
 * \param[out] canUndo   TRUE when an older state exists
 * \param[out] canRedo   TRUE when an undone state exists
 *
 * \retval     None
 */
void Arinc661_Definition_History(const Arinc661_Definition_Editor_t* editor, bool_t* canUndo, bool_t* canRedo);

/*************************************************************************************************/
/**
 * \brief Removes the active document's selected layer, widget, or extension.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DeleteSelection(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Duplicates the active document's selected widget with its subtree and extensions.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_DuplicateSelection(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Toggles grid snapping and reports its state.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     toggle   TRUE to flip the setting
 *
 * \retval        TRUE     if snapping is on.
 * \retval        FALSE    otherwise.
 */
bool_t Arinc661_Definition_Snap(Arinc661_Definition_Editor_t* editor, bool_t toggle);

/*************************************************************************************************/
/**
 * \brief Toggles the scene grid and reports its state.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     toggle   TRUE to flip the setting
 *
 * \retval        TRUE     if the grid is shown.
 * \retval        FALSE    otherwise.
 */
bool_t Arinc661_Definition_Grid(Arinc661_Definition_Editor_t* editor, bool_t toggle);

/*************************************************************************************************/
/**
 * \brief Resets the active document's scene view to fit the layer.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_FitView(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a layer to the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_AddLayer(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a widget under the active document's selection: into the selected container,
 *  beside a selected leaf, or into the selected layer.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_AddWidget(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds an extension to the active document's selected widget.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_AddExtension(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Returns the type of the active document's selected widget, for contextual help.
 *
 * \param[in] editor   Editor state
 *
 * \retval    const char_t*   Type name, or NULL when no widget is selected.
 */
const char_t* Arinc661_Definition_SelectedType(const Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Selects the first widget of the active document with a name.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     name     Widget name
 *
 * \retval        TRUE     if a widget was selected.
 * \retval        FALSE    otherwise.
 */
bool_t Arinc661_Definition_SelectByName(Arinc661_Definition_Editor_t* editor, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Commits the frame's changes to the undo history; call once per frame after drawing.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Definition_EndFrame(Arinc661_Definition_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Creates a Definition File for a user application: named after its dFPath, carrying
 *  its applicationId, with one layer sized to its window.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     fileName        File name the user application references
 * \param[in]     applicationId   FACE applicationId to store as ApplicationId
 * \param[in]     layerWidth      Layer width in 0.01 mm
 * \param[in]     layerHeight     Layer height in 0.01 mm
 *
 * \retval        TRUE            if the document was created and selected.
 * \retval        FALSE           when no slot is free.
 */
bool_t Arinc661_Definition_CreateFor(Arinc661_Definition_Editor_t* editor, const char_t* fileName, int64_t applicationId, float32_t layerWidth, float32_t layerHeight);

/*************************************************************************************************/
/**
 * \brief Selects a layer of an open document and makes the document active.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in]     layerIndex      Layer array index
 *
 * \retval        None
 */
void Arinc661_Definition_SelectLayer(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t layerIndex);

/*************************************************************************************************/
/**
 * \brief Selects a widget of an open document and makes the document active.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in]     widgetIndex     Widget array index
 *
 * \retval        None
 */
void Arinc661_Definition_SelectWidget(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t widgetIndex);

/*************************************************************************************************/
/**
 * \brief Records every layer of an open document into a draw list and reports widget bounds.
 *
 * \param[in]     editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in,out] drawList        Target draw list; its transform maps 0.01 mm to output units
 * \param[in]     fontSize        Host font size in output units
 * \param[out]    bounds          Bounds storage, or NULL
 * \param[in]     boundsCapacity  Bounds slots
 * \param[out]    boundsCount     Bounds written; may be NULL
 *
 * \retval        TRUE            if at least one layer was recorded.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_Definition_RecordDocumentBounds(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize, Arinc661_Scene_Bounds_t* bounds, uint32_t boundsCapacity, uint32_t* boundsCount);

/*************************************************************************************************/
/**
 * \brief Renames an open document's file name without saving, so references can follow a Save As.
 *
 * \param[in]  editor          Editor state
 * \param[out] previousName    Name before the last Save As, or an empty string
 * \param[out] currentName     Name after it
 *
 * \retval     TRUE            if a Save As happened since the last call; the record is cleared.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Definition_TakeRename(Arinc661_Definition_Editor_t* editor, const char_t** previousName, const char_t** currentName);

/*************************************************************************************************/
/**
 * \brief Draws one open document's hierarchy tree, wherever the caller is; a click inside
 *  selects in that document and makes it active.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 *
 * \retval        TRUE            if the selection changed through this tree.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_Definition_DrawDocumentHierarchy(Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Describes the active document's selection as a breadcrumb.
 *
 * \param[in]  editor     Editor state
 * \param[out] text       Breadcrumb, or an empty string
 * \param[in]  capacity   Bytes in text
 *
 * \retval     None
 */
void Arinc661_Definition_SelectionText(const Arinc661_Definition_Editor_t* editor, char_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Counts an open document's layers and reads one layer's id, name, and size.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  layerIndex      Layer array index
 * \param[out] layerId         LayerId text, or an empty string
 * \param[out] name            Layer name
 * \param[out] width           Width in 0.01 mm
 * \param[out] height          Height in 0.01 mm
 *
 * \retval     TRUE            if the layer exists.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Definition_LayerAt(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t layerIndex, const char_t** layerId, const char_t** name, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Reports the index-th consistency problem inside one open document.
 *
 * \par Details:
 *  Dangling identifier references, widgets under a parent the specification forbids,
 *  duplicate WidgetIdent values within a layer, and duplicate LayerId values.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  index           Which problem
 * \param[out] text            Message
 * \param[in]  capacity        Bytes in text
 * \param[out] widgetIndex     Widget to select, or INVALID index
 * \param[out] layerIndex      Layer to select, or INVALID index
 *
 * \retval     TRUE            if such a problem exists.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Definition_ProblemAt(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t index, char_t* text, uint32_t capacity, uint32_t* widgetIndex, uint32_t* layerIndex);

/*************************************************************************************************/
/**
 * \brief Tells whether an open document holds a MapHorz_Source or MapVert_Source widget.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    TRUE            if it does.
 * \retval    FALSE           otherwise.
 */
bool_t Arinc661_Definition_HasMapSource(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Reads one layer's LayerId.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  layerIndex      Layer array index
 * \param[out] layerId         LayerId value
 *
 * \retval     TRUE            if the layer exists and has a numeric LayerId.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Definition_LayerIdAt(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t layerIndex, int64_t* layerId);

/*************************************************************************************************/
/**
 * \brief Reports the widget selected in an open document.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[out] widgetIndex     Selected widget array index
 *
 * \retval     TRUE            if a widget is selected in that document.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Definition_SelectedWidgetIndex(const Arinc661_Definition_Editor_t* editor, uint32_t documentIndex, uint32_t* widgetIndex);

/*************************************************************************************************/
/**
 * \brief Reports whether the active document's errors should be shown before saving.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     text     Message to show in the status bar after the save, or NULL to clear
 *
 * \retval        None
 */
void Arinc661_Definition_SetStatusNote(Arinc661_Definition_Editor_t* editor, const char_t* text);

/*************************************************************************************************/
/**
 * \brief Adds a widget of a type under the active document's selection.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     type     Widget type name
 *
 * \retval        None
 */
void Arinc661_Definition_AddWidgetOfType(Arinc661_Definition_Editor_t* editor, const char_t* type);

/*************************************************************************************************/
/**
 * \brief Attaches an extension of a type to the active document's selected widget.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     type     Extension type name
 *
 * \retval        None
 */
void Arinc661_Definition_AddExtensionOfType(Arinc661_Definition_Editor_t* editor, const char_t* type);

#endif /* ARINC661_DEFINITION_EDITOR_H */
