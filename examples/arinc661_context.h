#ifndef ARINC661_CONTEXT_H
#define ARINC661_CONTEXT_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * FACE Technical Standard, Edition 3.1, Appendix H.3.1: a UserApplication names its window and
 * its Definition File (dFPath). ARINC 661 Part 1-7, section 6.5.1.2: the Definition File's
 * ApplicationId property identifies that User Application.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

extern "C"
{
#include "faceoe_gfx/faceoe_gfx_drawlist.h"
#include "os/os_types.h"
}

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_CONTEXT_MAX_DEFINITION_FILES (256)
#define ARINC661_CONTEXT_MAX_FILE_NAME_BYTES  (260)
#define ARINC661_CONTEXT_MAX_PROBLEMS         (256)
#define ARINC661_CONTEXT_MAX_PROBLEM_BYTES    (320)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

struct Arinc661_Configuration_Editor_s;
struct Arinc661_Definition_Editor_s;

/*************************************************************************************************/
/**
 * \brief Where a problem points.
 */
typedef enum Arinc661_Context_Target_e
{
    ARINC661_CONTEXT_TARGET_NONE,             ///< Nothing to select
    ARINC661_CONTEXT_TARGET_USER_APPLICATION, ///< A user application of a configuration
    ARINC661_CONTEXT_TARGET_WINDOW,           ///< A window of a configuration
    ARINC661_CONTEXT_TARGET_DEFINITION,       ///< A Definition File document
    ARINC661_CONTEXT_TARGET_LAYER             ///< A layer of a Definition File document
} Arinc661_Context_Target_t;

/*************************************************************************************************/
/**
 * \brief One cross-document problem.
 */
typedef struct Arinc661_Context_Problem_s
{
    char_t                    text[ARINC661_CONTEXT_MAX_PROBLEM_BYTES]; ///< What is wrong and where
    Arinc661_Context_Target_t target;                                   ///< Where it points
    uint32_t                  documentIndex;                            ///< Document slot of the target
    uint32_t                  itemIndex;                                ///< Array index of the target item
    bool_t                    isError;                                  ///< TRUE for a defect, FALSE for a warning
} Arinc661_Context_Problem_t;

/*************************************************************************************************/
/**
 * \brief Application state shared by the configuration and Definition File editors.
 *
 * \par Details:
 *  The host owns one context, points it at both editors, and keeps the folder's valid
 *  Definition Files in it. Either editor may ask the context about the other editor's
 *  documents, request that a document be opened and focused, or have a Definition File
 *  drawn into a rectangle.
 */
typedef struct Arinc661_Context_s
{
    struct Arinc661_Configuration_Editor_s* configurationEditor;                                                       ///< Configuration editor, never NULL once set up.
    struct Arinc661_Definition_Editor_s*    definitionEditor;                                                          ///< Definition File editor, never NULL once set up.
    uint32_t                                definitionFileCount;                                                       ///< Valid Definition Files in the shared folder.
    char_t                                  definitionFiles[ARINC661_CONTEXT_MAX_DEFINITION_FILES][ARINC661_CONTEXT_MAX_FILE_NAME_BYTES]; ///< Their names.
    bool_t                                  isDefinitionFocused;                                                       ///< TRUE when the last focus request named the Definition File section.
    bool_t                                  hasFocusRequest;                                                           ///< TRUE until the host moves the focus as requested.
    Arinc661_Context_Problem_t              problems[ARINC661_CONTEXT_MAX_PROBLEMS];                                   ///< Problems found by the last check.
    uint32_t                                problemCount;                                                              ///< Problems found.
    uint32_t                                errorCount;                                                                ///< Problems that are errors.
} Arinc661_Context_t;

/*************************************************************************************************/
/**
 * \brief One user application of an open configuration, as seen from the context.
 */
typedef struct Arinc661_Context_UserApplication_s
{
    const char_t* documentName;  ///< Configuration document holding the user application.
    uint32_t      documentIndex; ///< Configuration document slot.
    uint32_t      applicationIndex; ///< Index in that document's user application array.
    int64_t       applicationId; ///< FACE applicationId.
    int64_t       windowId;      ///< FACE window reference.
    const char_t* dFPath;        ///< FACE dFPath.
} Arinc661_Context_UserApplication_t;

/*************************************************************************************************/
/**
 * \brief What the context knows about one Definition File by name.
 */
typedef struct Arinc661_Context_Definition_s
{
    bool_t   isInFolder;    ///< TRUE when the shared folder holds a valid Definition File of that name.
    bool_t   isOpen;        ///< TRUE when the Definition File editor has it open.
    uint32_t documentIndex; ///< Definition File document slot when open.
    bool_t   hasApplicationId; ///< TRUE when the open document carries a numeric ApplicationId.
    int64_t  applicationId; ///< That ApplicationId.
    uint32_t layerCount;    ///< Layers of the open document.
    uint32_t widgetCount;   ///< Widgets and extensions of the open document.
} Arinc661_Context_Definition_t;

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
 * \brief Clears the context and attaches it to both editors.
 *
 * \param[out]    context               Context to initialize
 * \param[in,out] configurationEditor   Configuration editor
 * \param[in,out] definitionEditor      Definition File editor
 *
 * \retval        None
 */
void Arinc661_Context_Initialize(Arinc661_Context_t* context, struct Arinc661_Configuration_Editor_s* configurationEditor, struct Arinc661_Definition_Editor_s* definitionEditor);

/*************************************************************************************************/
/**
 * \brief Looks a Definition File up by name across the folder and the open documents.
 *
 * \param[in]  context      Context
 * \param[in]  fileName     Definition File name, as a user application's dFPath
 * \param[out] definition   Findings
 *
 * \retval     TRUE         if the name is known to the folder or is open.
 * \retval     FALSE        otherwise; the findings are cleared.
 */
bool_t Arinc661_Context_FindDefinition(const Arinc661_Context_t* context, const char_t* fileName, Arinc661_Context_Definition_t* definition);

/*************************************************************************************************/
/**
 * \brief Opens, or selects, a Definition File and moves the focus to it.
 *
 * \param[in,out] context    Context
 * \param[in]     fileName   Definition File name in the shared folder
 *
 * \retval        None
 */
void Arinc661_Context_ShowDefinition(Arinc661_Context_t* context, const char_t* fileName);

/*************************************************************************************************/
/**
 * \brief Counts the user applications of every open configuration.
 *
 * \param[in] context   Context
 *
 * \retval    uint32_t   Number of user applications.
 */
uint32_t Arinc661_Context_UserApplicationCount(const Arinc661_Context_t* context);

/*************************************************************************************************/
/**
 * \brief Describes one user application across the open configurations.
 *
 * \param[in]  context       Context
 * \param[in]  index         Index below Arinc661_Context_UserApplicationCount
 * \param[out] application   Findings; its strings stay valid while the configuration stays open
 *
 * \retval     TRUE          if the index exists.
 * \retval     FALSE         otherwise.
 */
bool_t Arinc661_Context_UserApplication(const Arinc661_Context_t* context, uint32_t index, Arinc661_Context_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief Selects a user application in its configuration and moves the focus to it.
 *
 * \param[in,out] context       Context
 * \param[in]     application   A user application returned by Arinc661_Context_UserApplication
 *
 * \retval        None
 */
void Arinc661_Context_ShowUserApplication(Arinc661_Context_t* context, const Arinc661_Context_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief Records every layer of an open Definition File into a draw list.
 *
 * \param[in]     context    Context
 * \param[in]     fileName   Definition File name; nothing is recorded unless it is open
 * \param[in,out] drawList   Target; its current transform maps 0.01 mm to output units
 * \param[in]     fontSize   Host font size in output units
 *
 * \retval        TRUE       if at least one layer was recorded.
 * \retval        FALSE      otherwise.
 */
bool_t Arinc661_Context_RecordDefinition(const Arinc661_Context_t* context, const char_t* fileName, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize);

/*************************************************************************************************/
/**
 * \brief Takes a pending request to move the focus between the two editors.
 *
 * \param[in,out] context               Context
 * \param[out]    isDefinitionFocused   TRUE when the Definition File section should take the focus
 *
 * \retval        TRUE                  if a request was pending; it is cleared.
 * \retval        FALSE                 otherwise.
 */
bool_t Arinc661_Context_TakeFocusRequest(Arinc661_Context_t* context, bool_t* isDefinitionFocused);

/*************************************************************************************************/
/**
 * \brief Checks every open document against the others and fills the problem list.
 *
 * \par Details:
 *  Errors: a user application whose dFPath names no known file; an ApplicationId that differs
 *  from the applicationId; two Definition Files carrying the same ApplicationId; a user
 *  application whose window does not exist. Warnings: a referenced file that is not open; a
 *  layer larger than its window; a window no user application targets; an open Definition
 *  File no user application references.
 *
 * \param[in,out] context   Context
 *
 * \retval        uint32_t   Number of problems.
 */
uint32_t Arinc661_Context_Check(Arinc661_Context_t* context);

/*************************************************************************************************/
/**
 * \brief Selects the item a problem points at and moves the focus there.
 *
 * \param[in,out] context   Context
 * \param[in]     problem   A problem from the last check
 *
 * \retval        None
 */
void Arinc661_Context_ShowProblem(Arinc661_Context_t* context, const Arinc661_Context_Problem_t* problem);

/*************************************************************************************************/
/**
 * \brief Opens every Definition File in the folder that a user application references.
 *
 * \param[in,out] context   Context
 *
 * \retval        uint32_t   Number of files opened.
 */
uint32_t Arinc661_Context_OpenReferencedDefinitions(Arinc661_Context_t* context);

/*************************************************************************************************/
/**
 * \brief Creates a Definition File for a user application, sized to its window.
 *
 * \param[in,out] context       Context
 * \param[in]     application   The user application
 *
 * \retval        TRUE          if the document was created and focused.
 * \retval        FALSE         otherwise.
 */
bool_t Arinc661_Context_CreateDefinitionFor(Arinc661_Context_t* context, const Arinc661_Context_UserApplication_t* application);

/*************************************************************************************************/
/**
 * \brief Creates a user application for an open Definition File in a configuration window.
 *
 * \param[in,out] context                 Context
 * \param[in]     definitionDocument      Definition File document slot
 * \param[in]     configurationDocument   Configuration document slot
 * \param[in]     windowId                Window the application targets
 *
 * \retval        TRUE                    if the user application was created and focused.
 * \retval        FALSE                   otherwise.
 */
bool_t Arinc661_Context_CreateUserApplicationFor(Arinc661_Context_t* context, uint32_t definitionDocument, uint32_t configurationDocument, int64_t windowId);

/*************************************************************************************************/
/**
 * \brief Reports the window frames of the user applications referencing a Definition File.
 *
 * \param[in]  context          Context
 * \param[in]  fileName         Definition File name
 * \param[in]  index            Which referencing user application
 * \param[out] widthUnits       Window width in 0.01 mm
 * \param[out] heightUnits      Window height in 0.01 mm
 * \param[out] label            Label naming the configuration, application, and window
 * \param[in]  labelCapacity    Bytes in label
 *
 * \retval     TRUE             if such a user application exists and its window was found.
 * \retval     FALSE            otherwise.
 */
bool_t Arinc661_Context_WindowFrame(const Arinc661_Context_t* context, const char_t* fileName, uint32_t index, float32_t* widthUnits, float32_t* heightUnits, char_t* label, uint32_t labelCapacity);

/*************************************************************************************************/
/**
 * \brief Follows a Definition File rename by repointing user applications that named it.
 *
 * \param[in,out] context   Context
 *
 * \retval        uint32_t   Number of user applications changed.
 */
uint32_t Arinc661_Context_FollowRenames(Arinc661_Context_t* context);

#endif /* ARINC661_CONTEXT_H */
