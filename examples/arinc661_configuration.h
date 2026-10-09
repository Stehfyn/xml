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
#include "faceoe_gfx/faceoe_gfx_drawlist.h"
}
#include "arinc661_renderer.h"

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
#define ARINC661_CONFIGURATION_MAX_HISTORY            (64)
#define ARINC661_CONFIGURATION_MAX_PREVIEW_BOUNDS     (4096)
#define ARINC661_CONFIGURATION_MAX_SCENE_COMMANDS     (65536)
#define ARINC661_CONFIGURATION_MAX_SCENE_POINTS       (524288)
#define ARINC661_CONFIGURATION_MAX_SCENE_TEXT_BYTES   (1048576)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief One open document and its conversion workspace.
 */
struct Arinc661_Configuration_Editor_s;

typedef struct Arinc661_Configuration_Document_s
{
    struct Arinc661_Configuration_Editor_s*    owner;                                                                  ///< Editor holding this document.
    int32_t                                    status;                                                                 ///< Current status message.
    void*                                      undoStack[ARINC661_CONFIGURATION_MAX_HISTORY];                          ///< Model snapshots older than the shadow, oldest first.
    void*                                      redoStack[ARINC661_CONFIGURATION_MAX_HISTORY];                          ///< Model snapshots undone, oldest first.
    void*                                      shadow;                                                                 ///< Snapshot of the model as last committed.
    uint32_t                                   undoCount;                                                              ///< Snapshots on the undo stack.
    uint32_t                                   redoCount;                                                              ///< Snapshots on the redo stack.
    bool_t                                     isLoaded;                                                               ///< TRUE while the slot holds a document, read or new.
    bool_t                                     isDirty;                                                                ///< TRUE when the loaded document has unsaved changes.
    int32_t                                    selectedKind;                                                           ///< Kind of the selected entity.
    uint32_t                                   selectedIndex;                                                          ///< Array index of the selected entity.
    float32_t                                  totalScreenWidth;                                                       ///< Sum of drawable screen widths in pixels.
    float32_t                                  maximumScreenHeight;                                                    ///< Largest drawable screen height in pixels.
    uint32_t                                   drawableScreenCount;                                                    ///< Screens with positive pixel size.
    char_t                                     documentName[ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES];                ///< Filename shown on the tab.
    char_t                                     documentPath[ARINC661_CONFIGURATION_MAX_FILE_PATH_BYTES];               ///< File path, or empty while untitled.
    byte_t                                     inputBuffer[ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES + 1];             ///< Input file bytes; the extra byte detects an oversized file.
    byte_t                                     outputBuffer[ARINC661_CONFIGURATION_MAX_DOCUMENT_BYTES];                ///< Built XML bytes.
    uint64_t                                   workspace[FACEOE_DISPLAYMANAGEMENT_WORKSPACE_BYTES / sizeof(uint64_t)]; ///< XML conversion scratch space.
    FACEOE_DisplayManagement_Screen_t          screens[ARINC661_CONFIGURATION_MAX_SCREENS];                            ///< Screen values.
    FACEOE_DisplayManagement_Layout_t          layouts[ARINC661_CONFIGURATION_MAX_LAYOUTS];                            ///< Layout values.
    FACEOE_DisplayManagement_Window_t          windows[ARINC661_CONFIGURATION_MAX_WINDOWS];                            ///< Window values.
    FACEOE_DisplayManagement_UserApplication_t userApplications[ARINC661_CONFIGURATION_MAX_USER_APPLICATIONS];         ///< User application values.
    FACEOE_DisplayManagement_ExternalSource_t  externalSources[ARINC661_CONFIGURATION_MAX_EXTERNAL_SOURCES];           ///< External source values.
    FACEOE_DisplayManagement_Configuration_t   configuration;                                                          ///< Configuration over the arrays above.
} Arinc661_Configuration_Document_t;

/*************************************************************************************************/
/**
 * \brief Configuration editor state: the document slots, the file browser, the preview draw list
 *  and bounds, and the scene drag.
 */
struct Arinc661_Context_s;

typedef struct Arinc661_Configuration_Editor_s
{
    struct Arinc661_Context_s*        context;                                                                                                 ///< Shared application context, or NULL.
    FACEOE_Gfx_DrawList_t             drawList;                                                                                                ///< Draw list for definition previews.
    FACEOE_Gfx_Command_t              sceneCommands[ARINC661_CONFIGURATION_MAX_SCENE_COMMANDS];                                                ///< Preview command storage.
    FACEOE_Gfx_Point_t                scenePoints[ARINC661_CONFIGURATION_MAX_SCENE_POINTS];                                                    ///< Preview point storage.
    char_t                            sceneText[ARINC661_CONFIGURATION_MAX_SCENE_TEXT_BYTES];                                                  ///< Preview text storage.
    int32_t                           dragMode;                                                                                                ///< Scene drag in progress: none, move, or a resize corner.
    uint32_t                          dragWindow;                                                                                              ///< Array index of the window being dragged.
    float32_t                         dragStartX;                                                                                              ///< Mouse X at drag start.
    float32_t                         dragStartY;                                                                                              ///< Mouse Y at drag start.
    int64_t                           dragOriginX;                                                                                             ///< pixelArea.x at drag start.
    int64_t                           dragOriginY;                                                                                             ///< pixelArea.y at drag start.
    int64_t                           dragOriginWidth;                                                                                         ///< pixelArea.width at drag start.
    int64_t                           dragOriginHeight;                                                                                        ///< pixelArea.height at drag start.
    float32_t                         dragScale;                                                                                               ///< Canvas pixels per configured screen pixel at drag start.
    bool_t                            hasDragged;                                                                                              ///< TRUE once the mouse moved past the drag threshold.
    Arinc661_Renderer_Bounds_t        previewBounds[ARINC661_CONFIGURATION_MAX_PREVIEW_BOUNDS];                                                ///< Widget bounds of this frame's previews; each widget points into the definition of its previewDocuments slot.
    uint32_t                          previewBoundsCount;                                                                                      ///< Bounds recorded this frame.
    uint32_t                          previewDocuments[ARINC661_CONFIGURATION_MAX_PREVIEW_BOUNDS];                                             ///< Definition editor document slot of each bound.
    int32_t                           status;                                                                                                  ///< Editor status message; shown in place of the active document's.
    uint32_t                          activeDocumentIndex;                                                                                     ///< Selected document slot.
    uint32_t                          requestedDocumentIndex;                                                                                  ///< Tab to select on the next tab bar draw.
    uint32_t                          pendingCloseDocumentIndex;                                                                               ///< Document awaiting close confirmation.
    uint32_t                          pendingSaveDocumentIndex;                                                                                ///< Untitled document awaiting a filename.
    bool_t                            isSaveAllPending;                                                                                        ///< TRUE while Save All is naming untitled documents.
    char_t                            folderPath[ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES];                                                ///< Folder listed by the file browser.
    char_t                            saveFolderPath[ARINC661_CONFIGURATION_MAX_FOLDER_PATH_BYTES];                                            ///< Folder chosen in the save dialog.
    char_t                            saveFileName[ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES];                                                 ///< Filename chosen in the save dialog.
    uint32_t                          definitionFileCount;                                                                                     ///< Names in definitionFiles.
    char_t                            definitionFiles[ARINC661_CONFIGURATION_MAX_DEFINITION_FILES][ARINC661_CONFIGURATION_MAX_XML_NAME_BYTES]; ///< Definition File names offered for a user application's dFPath.
    Arinc661_Configuration_Document_t documents[ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS];                                                    ///< Document slots and XML workspaces.
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
 * \brief Initializes a zero-filled editor and its document slots.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_Initialize(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the whole editor in one full-viewport ImGui window, then commits the frame's changes
 *  to the undo history.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DrawEditor(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Sets the folder the file list and the save-as dialog start from.
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
 * \brief Empties the list of Definition Files offered for a user application's dFPath.
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

/*************************************************************************************************/
/**
 * \brief Attaches the shared application context.
 *
 * \param[in,out] editor    Editor state
 * \param[in]     context   Context owned by the host
 *
 * \retval        None
 */
void Arinc661_Configuration_SetContext(Arinc661_Configuration_Editor_t* editor, struct Arinc661_Context_s* context);

/*************************************************************************************************/
/**
 * \brief Returns an open document's file name.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    const char_t*   Name, or NULL when the slot is empty.
 */
const char_t* Arinc661_Configuration_DocumentName(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Counts an open document's user applications.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    uint32_t   Count, zero for an empty slot.
 */
uint32_t Arinc661_Configuration_UserApplicationCount(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Reads one user application of an open document.
 *
 * \param[in]  editor             Editor state
 * \param[in]  documentIndex      Document slot
 * \param[in]  applicationIndex   Index below the count
 * \param[out] applicationId      FACE applicationId
 * \param[out] windowId           FACE window reference
 * \param[out] dFPath             FACE dFPath; stays valid while the document stays open
 *
 * \retval     TRUE               if the user application exists.
 * \retval     FALSE              otherwise.
 */
bool_t Arinc661_Configuration_UserApplication(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex, int64_t* applicationId, int64_t* windowId, const char_t** dFPath);

/*************************************************************************************************/
/**
 * \brief Makes a document active and selects one of its user applications.
 *
 * \param[in,out] editor             Editor state
 * \param[in]     documentIndex      Document slot
 * \param[in]     applicationIndex   Index below the count
 *
 * \retval        None
 */
void Arinc661_Configuration_SelectUserApplication(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex);

/*************************************************************************************************/
/**
 * \brief Reverts the active document to the state before its latest change.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_Undo(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reapplies the active document's latest undone change.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_Redo(Arinc661_Configuration_Editor_t* editor);

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
void Arinc661_Configuration_History(const Arinc661_Configuration_Editor_t* editor, bool_t* canUndo, bool_t* canRedo);

/*************************************************************************************************/
/**
 * \brief Removes the active document's selected element.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DeleteSelection(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Duplicates the active document's selected window or user application.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_DuplicateSelection(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a screen to the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_AddScreen(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a layout to the active document, on the selection's screen or else the first screen.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_AddLayout(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a window to the active document: to the selected layout, the selected window's
 *  layout, or the selected screen's first layout, else to the first layout.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_AddWindow(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Adds a user application to the active document, targeting the selected window, else the
 *  first window.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_AddUserApplication(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Names the active document's selected element kind, for contextual help.
 *
 * \param[in] editor   Editor state
 *
 * \retval    const char_t*   "Screen", "Layout", "Window", "UserApplication", "ExternalSource", or NULL.
 */
const char_t* Arinc661_Configuration_SelectedElement(const Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Commits the frame's changes to the undo history; call once per frame after drawing,
 *  unless Arinc661_Configuration_DrawEditor already did.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_EndFrame(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Finds a window of an open document by id and reports its pixel size and density.
 *
 * \param[in]  editor            Editor state
 * \param[in]  documentIndex     Document slot
 * \param[in]  windowId          FACE window id
 * \param[out] windowIndex       Window array index; may be NULL
 * \param[out] pixelWidth        Width in pixels; may be NULL
 * \param[out] pixelHeight       Height in pixels; may be NULL
 * \param[out] unitsPerPixelX    0.01 mm per pixel along X; may be NULL
 * \param[out] unitsPerPixelY    0.01 mm per pixel along Y; may be NULL
 *
 * \retval     TRUE              if the window exists.
 * \retval     FALSE             otherwise.
 */
bool_t Arinc661_Configuration_Window(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, int64_t windowId, uint32_t* windowIndex, int64_t* pixelWidth, int64_t* pixelHeight, float32_t* unitsPerPixelX, float32_t* unitsPerPixelY);

/*************************************************************************************************/
/**
 * \brief Counts an open document's windows.
 *
 * \param[in] editor          Editor state
 * \param[in] documentIndex   Document slot
 *
 * \retval    uint32_t   Count, zero for an empty slot.
 */
uint32_t Arinc661_Configuration_WindowCount(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex);

/*************************************************************************************************/
/**
 * \brief Reads one window's id and name by array index.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  windowIndex     Window array index
 * \param[out] windowId        FACE window id
 * \param[out] name            Window name; stays valid while the document stays open
 *
 * \retval     TRUE            if the window exists.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Configuration_WindowAt(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t windowIndex, int64_t* windowId, const char_t** name);

/*************************************************************************************************/
/**
 * \brief Makes a document active and selects one of its windows.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in]     windowIndex     Window array index
 *
 * \retval        None
 */
void Arinc661_Configuration_SelectWindow(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t windowIndex);

/*************************************************************************************************/
/**
 * \brief Adds a visible user application with the given values to an open document, selects it,
 *  and makes the document active.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in]     applicationId   FACE applicationId
 * \param[in]     windowId        FACE window id
 * \param[in]     dFPath          Definition File name
 *
 * \retval        TRUE            if the user application was added.
 * \retval        FALSE           otherwise.
 */
bool_t Arinc661_Configuration_AddUserApplicationFor(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, int64_t applicationId, int64_t windowId, const char_t* dFPath);

/*************************************************************************************************/
/**
 * \brief Repoints every user application naming one Definition File at another name.
 *
 * \param[in,out] editor         Editor state
 * \param[in]     previousName   Name to replace, compared ignoring case
 * \param[in]     newName        Replacement
 *
 * \retval        uint32_t       Number of user applications changed.
 */
uint32_t Arinc661_Configuration_RepointDefinition(Arinc661_Configuration_Editor_t* editor, const char_t* previousName, const char_t* newName);

/*************************************************************************************************/
/**
 * \brief Points one user application at a Definition File.
 *
 * \param[in,out] editor             Editor state
 * \param[in]     documentIndex      Document slot
 * \param[in]     applicationIndex   User application array index
 * \param[in]     dFPath             Definition File name
 *
 * \retval        TRUE               if the user application exists and now names the file.
 * \retval        FALSE              otherwise.
 */
bool_t Arinc661_Configuration_SetDefinitionPath(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex, const char_t* dFPath);

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
void Arinc661_Configuration_SelectionText(const Arinc661_Configuration_Editor_t* editor, char_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Reads one external source of an open document.
 *
 * \param[in]  editor          Editor state
 * \param[in]  documentIndex   Document slot
 * \param[in]  sourceIndex     External source array index
 * \param[out] sourceId        FACE external source id
 * \param[out] name            Name; stays valid while the document stays open
 * \param[out] type            Type; stays valid while the document stays open
 *
 * \retval     TRUE            if the source exists.
 * \retval     FALSE           otherwise.
 */
bool_t Arinc661_Configuration_ExternalSourceAt(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t sourceIndex, uint32_t* sourceId, const char_t** name, const char_t** type);

/*************************************************************************************************/
/**
 * \brief Makes a document active and selects one of its external sources.
 *
 * \param[in,out] editor          Editor state
 * \param[in]     documentIndex   Document slot
 * \param[in]     sourceIndex     External source array index
 *
 * \retval        None
 */
void Arinc661_Configuration_SelectExternalSource(Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t sourceIndex);

/*************************************************************************************************/
/**
 * \brief Adds an external source to the active document.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval        None
 */
void Arinc661_Configuration_AddExternalSource(Arinc661_Configuration_Editor_t* editor);

/*************************************************************************************************/
/**
 * \brief Reads a user application's mapSourceLayer, visible, and styleFilePath values.
 *
 * \param[in]  editor             Editor state
 * \param[in]  documentIndex      Document slot
 * \param[in]  applicationIndex   User application array index
 * \param[out] mapSourceLayer     FACE mapSourceLayer flag
 * \param[out] isVisible          FACE visible flag
 * \param[out] styleFilePath      FACE styleFilePath; stays valid while the document stays open
 *
 * \retval     TRUE               if the user application exists.
 * \retval     FALSE              otherwise.
 */
bool_t Arinc661_Configuration_UserApplicationDetails(const Arinc661_Configuration_Editor_t* editor, uint32_t documentIndex, uint32_t applicationIndex, bool_t* mapSourceLayer, bool_t* isVisible, const char_t** styleFilePath);

#endif /* ARINC661_CONFIGURATION_H */
