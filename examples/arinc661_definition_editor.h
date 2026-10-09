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
#include "arinc661_renderer.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_DEFINITION_EDITOR_MAX_DOCUMENT_BYTES     (1048576)
#define ARINC661_DEFINITION_EDITOR_MAX_BLOCK_BYTES        (524288)
#define ARINC661_DEFINITION_EDITOR_MAX_FOLDER_PATH_BYTES  (512)
#define ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES     (260)
#define ARINC661_DEFINITION_EDITOR_MAX_FILE_PATH_BYTES    (ARINC661_DEFINITION_EDITOR_MAX_FOLDER_PATH_BYTES + ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES + 1)
#define ARINC661_DEFINITION_EDITOR_MAX_LAYERS             (128)
#define ARINC661_DEFINITION_EDITOR_MAX_WIDGETS            (2048)
#define ARINC661_DEFINITION_EDITOR_MAX_PROPERTIES         (16384)
#define ARINC661_DEFINITION_EDITOR_MAX_OPEN_DOCUMENTS     (8)
#define ARINC661_DEFINITION_EDITOR_MAX_SCENE_COMMANDS     (65536)
#define ARINC661_DEFINITION_EDITOR_MAX_SCENE_POINTS       (524288)
#define ARINC661_DEFINITION_EDITOR_MAX_SCENE_TEXT_BYTES   (1048576)
#define ARINC661_DEFINITION_EDITOR_NO_DOCUMENT            (UINT32_MAX)
#define ARINC661_DEFINITION_EDITOR_MAX_HISTORY            (64)
#define ARINC661_DEFINITION_EDITOR_MAX_FILTER_BYTES       (64)
#define ARINC661_DEFINITION_EDITOR_CLIPBOARD_WIDGETS      (512)
#define ARINC661_DEFINITION_EDITOR_CLIPBOARD_PROPERTIES   (4096)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief One document slot: a definition with its storage, file buffers, undo history, and view.
 */
struct Arinc661_DefinitionEditor_s;

typedef struct Arinc661_DefinitionEditor_Document_s
{
    struct Arinc661_DefinitionEditor_s* owner;                                                          ///< Editor holding this document.
    void*                               undoStack[ARINC661_DEFINITION_EDITOR_MAX_HISTORY];              ///< Model snapshots older than the shadow, oldest first.
    void*                               redoStack[ARINC661_DEFINITION_EDITOR_MAX_HISTORY];              ///< Model snapshots undone; the last is the next redo.
    void*                               shadow;                                                         ///< Snapshot of the model as last committed.
    float32_t                           viewZoom;                                                       ///< Scene zoom factor over the fitted scale; zero until first drawn.
    float32_t                           viewPanX;                                                       ///< Horizontal scene pan in screen pixels.
    float32_t                           viewPanY;                                                       ///< Vertical scene pan in screen pixels.
    uint32_t                            undoCount;                                                      ///< Snapshots on the undo stack.
    uint32_t                            redoCount;                                                      ///< Snapshots on the redo stack.
    int32_t                             status;                                                         ///< Document status message code.
    bool_t                              isLoaded;                                                       ///< TRUE while the slot holds a document, read or new.
    bool_t                              isDirty;                                                        ///< TRUE when the loaded document has unsaved changes.
    bool_t                              isBinary;                                                       ///< TRUE when the document is saved in the binary form.
    int32_t                             selectedKind;                                                   ///< Kind of the primary selection: none, definition, layer, or widget.
    uint32_t                            selectedIndex;                                                  ///< Layer or widget array index of the primary selection.
    byte_t                              isSelected[ARINC661_DEFINITION_EDITOR_MAX_WIDGETS];             ///< Widgets in the multi-selection, primary included.
    byte_t                              isHiddenInEditor[ARINC661_DEFINITION_EDITOR_MAX_WIDGETS];       ///< Widgets hidden from the scene by the author (editor only).
    byte_t                              isLocked[ARINC661_DEFINITION_EDITOR_MAX_WIDGETS];               ///< Widgets the scene does not pick or move (editor only).
    char_t                              documentName[ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES];    ///< Filename shown on the tab.
    char_t                              documentPath[ARINC661_DEFINITION_EDITOR_MAX_FILE_PATH_BYTES];   ///< Path of the document's file, or empty when untitled.
    byte_t                              inputBuffer[ARINC661_DEFINITION_EDITOR_MAX_DOCUMENT_BYTES + 1]; ///< Bytes read from a file, plus the byte the XML reader needs.
    byte_t                              outputBuffer[ARINC661_DEFINITION_EDITOR_MAX_DOCUMENT_BYTES];    ///< Bytes to write to a file.
    uint64_t                            workspace[ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)];         ///< XML conversion scratch space.
    Arinc661_Layer_t                    layers[ARINC661_DEFINITION_EDITOR_MAX_LAYERS];                  ///< Layer storage of the definition.
    Arinc661_Widget_t                   widgets[ARINC661_DEFINITION_EDITOR_MAX_WIDGETS];                ///< Widget and extension storage of the definition.
    Arinc661_Property_t                 properties[ARINC661_DEFINITION_EDITOR_MAX_PROPERTIES];          ///< Property storage of the definition.
    byte_t                              blocks[ARINC661_DEFINITION_EDITOR_MAX_BLOCK_BYTES];             ///< Block storage of the definition: binary symbol, picture, and animation law blocks.
    Arinc661_Definition_t               definition;                                                     ///< The document's definition, attached to the storage above.
} Arinc661_DefinitionEditor_Document_t;

/*************************************************************************************************/
/**
 * \brief Editor state: file browser, fixed document slots, scene, clipboard, and pending UI actions.
 */
struct Arinc661_Context_s;

typedef struct Arinc661_DefinitionEditor_s
{
    struct Arinc661_Context_s*           context;                                                              ///< Shared application context, or NULL.
    FACEOE_Gfx_DrawList_t                drawList;                                                             ///< Scene draw list.
    FACEOE_Gfx_Command_t                 sceneCommands[ARINC661_DEFINITION_EDITOR_MAX_SCENE_COMMANDS];         ///< Scene command storage.
    FACEOE_Gfx_Point_t                   scenePoints[ARINC661_DEFINITION_EDITOR_MAX_SCENE_POINTS];             ///< Scene point storage.
    char_t                               sceneText[ARINC661_DEFINITION_EDITOR_MAX_SCENE_TEXT_BYTES];           ///< Scene text storage.
    Arinc661_Renderer_Bounds_t           sceneBounds[ARINC661_DEFINITION_EDITOR_MAX_WIDGETS];                  ///< Widget bounds of the last scene.
    uint32_t                             sceneBoundsCount;                                                     ///< Bounds recorded by the last scene.
    int32_t                              dragMode;                                                             ///< Scene drag in progress: none, move, or a resize corner.
    uint32_t                             dragWidget;                                                           ///< Widget being dragged.
    float32_t                            dragStartX;                                                           ///< Mouse X at drag start.
    float32_t                            dragStartY;                                                           ///< Mouse Y at drag start.
    float32_t                            dragOriginX;                                                          ///< PosX at drag start.
    float32_t                            dragOriginY;                                                          ///< PosY at drag start.
    float32_t                            dragOriginWidth;                                                      ///< SizeX at drag start.
    float32_t                            dragOriginHeight;                                                     ///< SizeY at drag start.
    float32_t                            dragOriginAngle;                                                      ///< RotationAngle at drag start.
    float32_t                            dragCenterX;                                                          ///< Screen X of the rotation pivot at drag start.
    float32_t                            dragCenterY;                                                          ///< Screen Y of the rotation pivot at drag start.
    float32_t                            dragScale;                                                            ///< Screen pixels per layer unit during the drag.
    bool_t                               hasDragged;                                                           ///< TRUE once the mouse has moved during the drag.
    bool_t                               isMarquee;                                                            ///< TRUE while a rubber-band selection is drawn.
    bool_t                               zoomsToSelection;                                                     ///< TRUE to fit the view to the selection next frame.
    float32_t                            animationTime;                                                        ///< Seconds into the animations shown, or negative for the rest state.
    bool_t                               isPlaying;                                                            ///< TRUE while play mode holds snapshots of the documents.
    void*                                playSnapshots[ARINC661_DEFINITION_EDITOR_MAX_OPEN_DOCUMENTS];         ///< Document states to restore when play stops.
    bool_t                               playWasDirty[ARINC661_DEFINITION_EDITOR_MAX_OPEN_DOCUMENTS];          ///< Dirty flags to restore when play stops.
    Arinc661_Widget_t                    clipboardWidgets[ARINC661_DEFINITION_EDITOR_CLIPBOARD_WIDGETS];       ///< Copied subtrees; parent indices are relative to the clipboard.
    Arinc661_Property_t                  clipboardProperties[ARINC661_DEFINITION_EDITOR_CLIPBOARD_PROPERTIES]; ///< Their properties; ownerIndex is relative to the clipboard.
    uint32_t                             clipboardWidgetCount;                                                 ///< Widgets in the clipboard.
    uint32_t                             clipboardPropertyCount;                                               ///< Properties in the clipboard.
    uint32_t                             renamingWidget;                                                       ///< Widget whose name is being edited inline, or UINT32_MAX.
    char_t                               renameBuffer[ARINC661_MAX_NAME_BYTES];                                ///< Inline rename text.
    char_t                               hierarchyFilter[ARINC661_DEFINITION_EDITOR_MAX_FILTER_BYTES];         ///< Hierarchy filter text.
    char_t                               inspectorFilter[ARINC661_DEFINITION_EDITOR_MAX_FILTER_BYTES];         ///< Inspector parameter filter text.
    float32_t                            marqueeStartX;                                                        ///< Screen X where the marquee started.
    float32_t                            marqueeStartY;                                                        ///< Screen Y where the marquee started.
    float32_t                            dragOrigins[ARINC661_DEFINITION_EDITOR_MAX_WIDGETS][2];               ///< PosX and PosY of every selected widget at drag start.
    bool_t                               isSnapping;                                                           ///< TRUE to snap moves, sizes, and added widgets to whole millimeters.
    bool_t                               showsGrid;                                                            ///< TRUE to draw a millimeter grid behind the layer.
    uint32_t                             dropWidget;                                                           ///< Widget dropped onto a hierarchy node this frame, or UINT32_MAX.
    uint32_t                             dropParent;                                                           ///< Widget it was dropped on, or UINT32_MAX for a layer.
    uint32_t                             dropLayer;                                                            ///< Layer it was dropped into.
    uint32_t                             createConfiguration;                                                  ///< Configuration slot chosen for a new user application.
    uint32_t                             createWindow;                                                         ///< Index of the window chosen for it in that configuration.
    char_t                               dropType[ARINC661_MAX_NAME_BYTES];                                    ///< Palette type dropped on a hierarchy node this frame, or empty.
    uint32_t                             dropTypeParent;                                                       ///< Widget it was dropped on, or UINT32_MAX for a layer.
    uint32_t                             dropTypeLayer;                                                        ///< Layer it was dropped into.
    char_t                               addFilter[ARINC661_DEFINITION_EDITOR_MAX_FILTER_BYTES];               ///< Type filter typed in the add pop-up.
    uint32_t                             addLayer;                                                             ///< Layer the add pop-up targets.
    uint32_t                             addParent;                                                            ///< Parent widget the add pop-up targets, or UINT32_MAX for the layer.
    uint32_t                             addHit;                                                               ///< Widget under the cursor when the add pop-up opened, or UINT32_MAX.
    float32_t                            addX;                                                                 ///< Layer X where the add pop-up opened.
    float32_t                            addY;                                                                 ///< Layer Y where the add pop-up opened.
    int32_t                              status;                                                               ///< Editor status message code; when set, shown instead of the active document's.
    uint32_t                             activeDocumentIndex;                                                  ///< Active document slot, or UINT32_MAX.
    uint32_t                             requestedDocumentIndex;                                               ///< Tab the tab bar selects on its next draw, or UINT32_MAX.
    uint32_t                             pendingCloseDocumentIndex;                                            ///< Document awaiting close confirmation, or UINT32_MAX.
    uint32_t                             pendingSaveDocumentIndex;                                             ///< Untitled document awaiting a filename, or UINT32_MAX.
    bool_t                               isSaveAllPending;                                                     ///< TRUE while Save All is naming untitled documents.
    char_t                               renamedFrom[ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES];           ///< Name a document had before its last Save As, or empty; the host clears it.
    char_t                               renamedTo[ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES];             ///< Name it has now.
    char_t                               statusNote[ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES];            ///< Note appended to the status text, or empty.
    char_t                               folderPath[ARINC661_DEFINITION_EDITOR_MAX_FOLDER_PATH_BYTES];         ///< Folder containing Definition Files.
    char_t                               saveFolderPath[ARINC661_DEFINITION_EDITOR_MAX_FOLDER_PATH_BYTES];     ///< Folder chosen in the save dialog.
    char_t                               saveFileName[ARINC661_DEFINITION_EDITOR_MAX_XML_NAME_BYTES];          ///< Filename chosen in the save dialog.
    Arinc661_DefinitionEditor_Document_t documents[ARINC661_DEFINITION_EDITOR_MAX_OPEN_DOCUMENTS];             ///< Document slots and workspaces.
} Arinc661_DefinitionEditor_t;

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
 * \brief Initializes the editor: empty document slots, each definition attached to its slot's storage, and the scene draw list.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_Initialize(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the whole editor in one ImGui window over the main viewport, then calls
 *  Arinc661_DefinitionEditor_EndFrame.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DrawEditor(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Sets the folder the file list and new documents use.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     folderPath   Zero-terminated folder path
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SetFolderPath(Arinc661_DefinitionEditor_t* editor, const char_t* folderPath);

/*************************************************************************************************/
/**
 * \brief Opens a Definition File from the folder, or selects it if it is already open.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     fileName   Filename inside the folder
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_LoadFile(Arinc661_DefinitionEditor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Opens a new untitled document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_CreateDocument(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves the active document, asking for a filename when it is untitled.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SaveActiveDocument(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves every changed document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SaveAllDocuments(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Closes the active document, asking first when it has unsaved changes.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_CloseActiveDocument(Arinc661_DefinitionEditor_t* editor);

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
bool_t Arinc661_DefinitionEditor_HasActiveDocument(const Arinc661_DefinitionEditor_t* editor, bool_t* isDirty, bool_t* hasFree);

/*************************************************************************************************/
/**
 * \brief Draws the document tab bar, including the unsaved-changes prompt.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DrawDocumentTabs(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's hierarchy tree into the current window.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DrawHierarchy(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's scene as a child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DrawScene(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the active document's inspector as a child pane.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DrawInspector(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the save-as dialog when a filename is pending.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DrawDialogs(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Returns the editor's status message, or else the active document's; may be empty.
 *
 * \param[in] editor   Editor state
 *
 * \retval    const char_t*   Zero-terminated status text.
 */
const char_t* Arinc661_DefinitionEditor_StatusText(const Arinc661_DefinitionEditor_t* editor);

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
void Arinc661_DefinitionEditor_MeasurePanes(const Arinc661_DefinitionEditor_t* editor, float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*************************************************************************************************/
/**
 * \brief Tells whether a file holds an ARINC 661 Definition File, in XML or binary form.
 *
 * \param[in] bytes         First bytes of the file
 * \param[in] sizeOfBytes   Number of bytes available
 *
 * \retval    TRUE          if the bytes start a binary Definition File or an a661_df document.
 * \retval    FALSE         otherwise.
 */
bool_t Arinc661_DefinitionEditor_IsDefinitionFile(const byte_t* bytes, uint64_t sizeOfBytes);

/*************************************************************************************************/
/**
 * \brief Attaches the shared application context.
 *
 * \param[in,out] editor    Editor state
 * \param[in]     context   Context owned by the host
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SetContext(Arinc661_DefinitionEditor_t* editor, struct Arinc661_Context_s* context);

/*************************************************************************************************/
/**
 * \brief Finds an open document by file name, ignoring case.
 *
 * \param[in] editor     Editor state
 * \param[in] fileName   File name as shown on its tab
 *
 * \retval    uint32_t   Document slot, or ARINC661_DEFINITION_EDITOR_NO_DOCUMENT.
 */
uint32_t Arinc661_DefinitionEditor_FindDocument(const Arinc661_DefinitionEditor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Returns an open document's definition.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    const Arinc661_Definition_t*   Definition, or NULL when the slot is empty.
 */
const Arinc661_Definition_t* Arinc661_DefinitionEditor_Document(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Returns an open document's file name.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    const char_t*   Name, or NULL when the slot is empty.
 */
const char_t* Arinc661_DefinitionEditor_DocumentName(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Reads the ApplicationId property of an open document's definition.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[out] applicationId   Value
 *
 * \retval     TRUE            if the property exists and is numeric.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_DocumentApplicationId(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, int64_t* applicationId);

/*************************************************************************************************/
/**
 * \brief Selects a document's tab.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SelectDocument(Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Records every layer of an open document into a draw list, in layer order.
 *
 * \par Details:
 *  The draw list's current transform maps 0.01 mm layer units to output units. Invisible
 *  widgets, closed pop-ups, names, and chips are left out, as on a CDS; widgets hidden only
 *  in the editor are drawn.
 *
 * \param[in]     editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in,out] drawList        Target draw list
 * \param[in]     fontSize        Host font size in output units
 *
 * \retval        TRUE            if the document is open and at least one layer was recorded.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_RecordDocument(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize);

/*************************************************************************************************/
/**
 * \brief Reverts the active document to the state before its latest change.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_Undo(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reapplies the active document's latest undone change.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_Redo(Arinc661_DefinitionEditor_t* editor);

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
void Arinc661_DefinitionEditor_History(const Arinc661_DefinitionEditor_t* editor, bool_t* canUndo, bool_t* canRedo);

/*************************************************************************************************/
/**
 * \brief Removes the active document's selection: a layer, or the selected widgets and extensions with everything below them.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DeleteSelection(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Duplicates the active document's selected widget with its subtree and extensions.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_DuplicateSelection(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reports whether moves and sizes snap to whole millimeters, after flipping the setting when asked.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     toggle   TRUE to flip the setting first
 *
 * \retval        TRUE     if snapping is on.
 * \retval        FALSE    otherwise.
 */
bool_t Arinc661_DefinitionEditor_Snap(Arinc661_DefinitionEditor_t* editor, bool_t toggle);

/*************************************************************************************************/
/**
 * \brief Reports whether the scene grid is shown, after flipping the setting when asked.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     toggle   TRUE to flip the setting first
 *
 * \retval        TRUE     if the grid is shown.
 * \retval        FALSE    otherwise.
 */
bool_t Arinc661_DefinitionEditor_Grid(Arinc661_DefinitionEditor_t* editor, bool_t toggle);

/*************************************************************************************************/
/**
 * \brief Resets the active document's scene view to fit the layer.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_FitView(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a layer to the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_AddLayer(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a widget under the active document's selection: into the selected container,
 *  beside a selected leaf or extension, or into the selected layer or else the first layer.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_AddWidget(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds an extension to the active document's selected widget.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_AddExtension(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Returns the type of the active document's selected widget or extension, for contextual help.
 *
 * \param[in] editor   Editor state
 *
 * \retval    const char_t*   Type name, or NULL when no widget or extension is selected.
 */
const char_t* Arinc661_DefinitionEditor_SelectedType(const Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Selects the active document's widgets with the given names; the first match in array order is primary.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     name     Widget name, or a comma-separated list of names
 *
 * \retval        TRUE     if a widget was selected.
 * \retval        FALSE    otherwise.
 */
bool_t Arinc661_DefinitionEditor_SelectByName(Arinc661_DefinitionEditor_t* editor, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Commits the frame's changes to the undo history; call once per frame after drawing,
 *  unless Arinc661_DefinitionEditor_DrawEditor drew the frame.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_EndFrame(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Creates an unsaved document for a user application: named after its dFPath, with its
 *  applicationId as ApplicationId and one layer sized to its window.
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
bool_t Arinc661_DefinitionEditor_CreateFor(Arinc661_DefinitionEditor_t* editor, const char_t* fileName, int64_t applicationId, float32_t layerWidth, float32_t layerHeight);

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
void Arinc661_DefinitionEditor_SelectLayer(Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t layerIndex);

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
void Arinc661_DefinitionEditor_SelectWidget(Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t widgetIndex);

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
bool_t Arinc661_DefinitionEditor_RecordDocumentBounds(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize, Arinc661_Renderer_Bounds_t* bounds, uint32_t boundsCapacity, uint32_t* boundsCount);

/*************************************************************************************************/
/**
 * \brief Reports the last Save As rename, so references to the old file name can follow it.
 *
 * \param[in]  editor          Editor state
 * \param[out] previousName    Name before the last Save As, or an empty string
 * \param[out] currentName     Name after it
 *
 * \retval     TRUE            if a rename is recorded; it stays recorded until the caller clears renamedFrom.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_TakeRename(Arinc661_DefinitionEditor_t* editor, const char_t** previousName, const char_t** currentName);

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
bool_t Arinc661_DefinitionEditor_DrawDocumentHierarchy(Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex);

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
void Arinc661_DefinitionEditor_SelectionText(const Arinc661_DefinitionEditor_t* editor, char_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Reads one layer's LayerId, name, and size from an open document.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  layerIndex      Layer array index
 * \param[out] layerId         LayerId text, or an empty string
 * \param[out] name            Layer name
 * \param[out] width           Width in 0.01 mm, as Arinc661_Renderer_GetLayerSize reads it
 * \param[out] height          Height in 0.01 mm, as Arinc661_Renderer_GetLayerSize reads it
 *
 * \retval     TRUE            if the layer exists.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_LayerAt(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t layerIndex, const char_t** layerId, const char_t** name, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Reports the index-th consistency problem inside one open document.
 *
 * \par Details:
 *  Problems are dangling identifier references, widgets under a parent the specification
 *  forbids, extensions on a widget Table 8.4 does not list, duplicate WidgetIdent values
 *  within a layer, and duplicate LayerId values.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  index           Which problem
 * \param[out] text            Message
 * \param[in]  capacity        Bytes in text
 * \param[out] widgetIndex     Widget to select, or UINT32_MAX
 * \param[out] layerIndex      Layer to select, or UINT32_MAX
 *
 * \retval     TRUE            if such a problem exists.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_ProblemAt(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t index, char_t* text, uint32_t capacity, uint32_t* widgetIndex, uint32_t* layerIndex);

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
bool_t Arinc661_DefinitionEditor_HasMapSource(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex);

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
bool_t Arinc661_DefinitionEditor_LayerIdAt(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t layerIndex, int64_t* layerId);

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
bool_t Arinc661_DefinitionEditor_SelectedWidgetIndex(const Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t* widgetIndex);

/*************************************************************************************************/
/**
 * \brief Sets the note appended to the status text.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     text     Note, or NULL to clear it
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SetStatusNote(Arinc661_DefinitionEditor_t* editor, const char_t* text);

/*************************************************************************************************/
/**
 * \brief Adds a widget of a type under the active document's selection.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     type     Widget type name
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_AddWidgetOfType(Arinc661_DefinitionEditor_t* editor, const char_t* type);

/*************************************************************************************************/
/**
 * \brief Attaches an extension of a type to the active document's selected widget.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     type     Extension type name
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_AddExtensionOfType(Arinc661_DefinitionEditor_t* editor, const char_t* type);

/*************************************************************************************************/
/**
 * \brief Copies the selected widgets' subtrees to the editor clipboard.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        uint32_t   Widgets copied.
 */
uint32_t Arinc661_DefinitionEditor_Copy(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Pastes the clipboard under the active document's selection.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        uint32_t   Widgets pasted.
 */
uint32_t Arinc661_DefinitionEditor_Paste(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Starts an inline rename of the selected widget in the hierarchy.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_BeginRename(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Sets the animation time the scenes and previews show.
 *
 * \param[in,out] editor    Editor state
 * \param[in]     seconds   Seconds into the animations, or negative for the rest state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_SetAnimationTime(Arinc661_DefinitionEditor_t* editor, float32_t seconds);

/*************************************************************************************************/
/**
 * \brief Reports one animation widget of the active document; AnimationGroup widgets are not counted.
 *
 * \param[in]  editor            Editor state
 * \param[in]  index             Which animation, in array order
 * \param[out] name              Widget name
 * \param[out] type              Widget type
 * \param[out] delaySeconds      Delay in seconds
 * \param[out] durationSeconds   Duration in seconds
 *
 * \retval     TRUE              if such an animation exists.
 * \retval     FALSE             otherwise.
 */
bool_t Arinc661_DefinitionEditor_AnimationAt(const Arinc661_DefinitionEditor_t* editor, uint32_t index, const char_t** name, const char_t** type, float32_t* delaySeconds, float32_t* durationSeconds);

/*************************************************************************************************/
/**
 * \brief Enters play mode: snapshots every open document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_BeginPlay(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Leaves play mode: restores the snapshots.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_EndPlay(Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Applies a crew member's click to a widget as a CDS would.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in]     widgetIndex     Widget array index
 * \param[in]     fractionX       Click position across the widget, 0 to 1
 * \param[in]     fractionY       Click position up the widget, 0 to 1
 *
 * \retval        TRUE            if a state changed.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_Interact(Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, uint32_t widgetIndex, float32_t fractionX, float32_t fractionY);

/*************************************************************************************************/
/**
 * \brief Writes an open document's binary form beside its file.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[out]    path            Path written
 * \param[in]     capacity        Bytes in path
 *
 * \retval        TRUE            if the file was written.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_DefinitionEditor_ExportBinary(Arinc661_DefinitionEditor_t* editor, uint32_t documentIndex, char_t* path, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Counts the active document's widgets.
 *
 * \param[in] editor   Editor state
 *
 * \retval    uint32_t   Widget count, zero without an active document.
 */
uint32_t Arinc661_DefinitionEditor_WidgetCount(const Arinc661_DefinitionEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Saves the clipboard as a prefab file in the folder.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     name       Prefab name without extension
 * \param[out]    path       Path written
 * \param[in]     capacity   Bytes in path
 *
 * \retval        TRUE       if the file was written.
 * \retval        FALSE      otherwise.
 */
bool_t Arinc661_DefinitionEditor_SavePrefab(Arinc661_DefinitionEditor_t* editor, const char_t* name, char_t* path, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Loads a prefab file into the clipboard and pastes it at the selection.
 *
 * \param[in,out] editor     Editor state
 * \param[in]     fileName   Prefab file name inside the folder
 *
 * \retval        TRUE       if widgets were added.
 * \retval        FALSE      otherwise.
 */
bool_t Arinc661_DefinitionEditor_LoadPrefab(Arinc661_DefinitionEditor_t* editor, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Counts the history states of the active document.
 *
 * \param[in]  editor         Editor state
 * \param[out] currentIndex   Index of the current state
 *
 * \retval     uint32_t       States, zero without an active document.
 */
uint32_t Arinc661_DefinitionEditor_HistoryCount(const Arinc661_DefinitionEditor_t* editor, uint32_t* currentIndex);

/*************************************************************************************************/
/**
 * \brief Names a history state by the change that produced it.
 *
 * \param[in] editor   Editor state
 * \param[in] index    State index
 *
 * \retval    const char_t*   Label, empty when out of range.
 */
const char_t* Arinc661_DefinitionEditor_HistoryLabel(const Arinc661_DefinitionEditor_t* editor, uint32_t index);

/*************************************************************************************************/
/**
 * \brief Makes a history state current by undoing or redoing.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     index    State index
 *
 * \retval        None
 */
void Arinc661_DefinitionEditor_GoToHistory(Arinc661_DefinitionEditor_t* editor, uint32_t index);

#endif /* ARINC661_DEFINITION_EDITOR_H */
