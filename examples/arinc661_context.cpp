/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * FACE Technical Standard, Edition 3.1, Appendix H.3.1, UserApplication: applicationId, window,
 * and dFPath. ARINC 661 Part 1-7, section 6.5.1.2: ApplicationId property of a661_df.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_context.h"
#include "arinc661_definition_editor.h"

extern "C"
{
#include "arinc661/arinc661_scene.h"
}
#include "arinc661_configuration.h"
#include "arinc661_definition_editor.h"

extern "C"
{
#include "os/os_assert.h"
}

#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Appends a problem to the context's list when it has room.
 *
 * \param[in,out] context         Context
 * \param[in]     isError         TRUE for a defect, FALSE for a warning
 * \param[in]     target          Where the problem points
 * \param[in]     documentIndex   Document slot of the target
 * \param[in]     itemIndex       Array index of the target item
 * \param[in]     text            Message
 *
 * \retval        None
 */
static void AddProblem(Arinc661_Context_t* context, bool_t isError, Arinc661_Context_Target_t target, uint32_t documentIndex, uint32_t itemIndex, const char_t* text);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Both editors keep a pointer to the context so their inspectors and scenes can consult it.
 */
void Arinc661_Context_Initialize(Arinc661_Context_t* context, Arinc661_Configuration_Editor_t* configurationEditor, Arinc661_Definition_Editor_t* definitionEditor)
{
    LOGASSERT(NULL != context);
    LOGASSERT(NULL != configurationEditor);
    LOGASSERT(NULL != definitionEditor);
    (void)memset(context, 0, sizeof((*context)));
    context->configurationEditor = configurationEditor;
    context->definitionEditor    = definitionEditor;
    Arinc661_Configuration_SetContext(configurationEditor, context);
    Arinc661_Definition_SetContext(definitionEditor, context);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Names are compared ignoring case, as the file system does. An open document answers with
 *  its ApplicationId and counts even when the folder list has not been refreshed yet.
 */
bool_t Arinc661_Context_FindDefinition(const Arinc661_Context_t* context, const char_t* fileName, Arinc661_Context_Definition_t* definition)
{
    uint32_t file_index;

    LOGASSERT(NULL != context);
    LOGASSERT(NULL != fileName);
    LOGASSERT(NULL != definition);
    (void)memset(definition, 0, sizeof((*definition)));
    for (file_index = 0; file_index < context->definitionFileCount; ++file_index)
    {
        if (0 == _stricmp(fileName, context->definitionFiles[file_index]))
        {
            definition->isInFolder = TRUE;
            break;
        }
    }
    definition->documentIndex = Arinc661_Definition_FindDocument(context->definitionEditor, fileName);
    definition->isOpen        = (ARINC661_DEFINITION_NO_DOCUMENT != definition->documentIndex);
    if (definition->isOpen)
    {
        definition->hasApplicationId = Arinc661_Definition_DocumentApplicationId(context->definitionEditor, definition->documentIndex, &definition->applicationId);
        definition->layerCount       = Arinc661_Definition_Document(context->definitionEditor, definition->documentIndex)->layerCount;
        definition->widgetCount      = Arinc661_Definition_Document(context->definitionEditor, definition->documentIndex)->widgetCount;
    }

    return !!(definition->isInFolder || definition->isOpen);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Loading an already open file selects its tab.
 */
void Arinc661_Context_ShowDefinition(Arinc661_Context_t* context, const char_t* fileName)
{
    LOGASSERT(NULL != context);
    LOGASSERT(NULL != fileName);
    Arinc661_Definition_LoadFile(context->definitionEditor, fileName);
    if (ARINC661_DEFINITION_NO_DOCUMENT != Arinc661_Definition_FindDocument(context->definitionEditor, fileName))
    {
        Arinc661_Definition_SelectDocument(context->definitionEditor, Arinc661_Definition_FindDocument(context->definitionEditor, fileName));
    }
    context->isDefinitionFocused = TRUE;
    context->hasFocusRequest     = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks every open configuration document.
 */
uint32_t Arinc661_Context_UserApplicationCount(const Arinc661_Context_t* context)
{
    uint32_t document_index;
    uint32_t count;

    LOGASSERT(NULL != context);
    count = 0;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        count += Arinc661_Configuration_UserApplicationCount(context->configurationEditor, document_index);
    }

    return count;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Indexes run through the open configuration documents in slot order, then through each
 *  document's user applications in array order.
 */
bool_t Arinc661_Context_UserApplication(const Arinc661_Context_t* context, uint32_t index, Arinc661_Context_UserApplication_t* application)
{
    uint32_t document_index;
    uint32_t document_count;
    bool_t   is_found;

    LOGASSERT(NULL != context);
    LOGASSERT(NULL != application);
    (void)memset(application, 0, sizeof((*application)));
    is_found = FALSE;
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document_count = Arinc661_Configuration_UserApplicationCount(context->configurationEditor, document_index);
        if (index < document_count)
        {
            application->documentIndex    = document_index;
            application->applicationIndex = index;
            application->documentName     = Arinc661_Configuration_DocumentName(context->configurationEditor, document_index);
            is_found                      = Arinc661_Configuration_UserApplication(context->configurationEditor, document_index, index, &application->applicationId, &application->windowId, &application->dFPath);
            break;
        }
        index -= document_count;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Selecting the user application also makes its document the active configuration.
 */
void Arinc661_Context_ShowUserApplication(Arinc661_Context_t* context, const Arinc661_Context_UserApplication_t* application)
{
    LOGASSERT(NULL != context);
    LOGASSERT(NULL != application);
    Arinc661_Configuration_SelectUserApplication(context->configurationEditor, application->documentIndex, application->applicationIndex);
    context->isDefinitionFocused = FALSE;
    context->hasFocusRequest     = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A file that is not open records nothing, so the configuration scene stays honest about
 *  what the application knows.
 */
bool_t Arinc661_Context_RecordDefinition(const Arinc661_Context_t* context, const char_t* fileName, FACEOE_Gfx_DrawList_t* drawList, float32_t fontSize)
{
    uint32_t document_index;
    bool_t   has_recorded;

    LOGASSERT(NULL != context);
    LOGASSERT(NULL != fileName);
    LOGASSERT(NULL != drawList);
    has_recorded   = FALSE;
    document_index = Arinc661_Definition_FindDocument(context->definitionEditor, fileName);
    if (ARINC661_DEFINITION_NO_DOCUMENT != document_index)
    {
        has_recorded = Arinc661_Definition_RecordDocument(context->definitionEditor, document_index, drawList, fontSize);
    }

    return has_recorded;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Clears the request so the host moves the focus once.
 */
bool_t Arinc661_Context_TakeFocusRequest(Arinc661_Context_t* context, bool_t* isDefinitionFocused)
{
    bool_t has_request;

    LOGASSERT(NULL != context);
    LOGASSERT(NULL != isDefinitionFocused);
    has_request = context->hasFocusRequest;
    (*isDefinitionFocused) = context->isDefinitionFocused;
    context->hasFocusRequest = FALSE;

    return has_request;
}

/*************************************************************************************************/
/**
 * \brief Appends a problem when the list has room.
 */
static void AddProblem(Arinc661_Context_t* context, bool_t isError, Arinc661_Context_Target_t target, uint32_t documentIndex, uint32_t itemIndex, const char_t* text)
{
    Arinc661_Context_Problem_t* problem;

    if (context->problemCount < ARINC661_CONTEXT_MAX_PROBLEMS)
    {
        problem                = &context->problems[context->problemCount];
        problem->target        = target;
        problem->documentIndex = documentIndex;
        problem->itemIndex     = itemIndex;
        problem->isError       = isError;
        strncpy_s(problem->text, sizeof(problem->text), text, _TRUNCATE);
        context->problemCount += 1;
        if (isError)
        {
            context->errorCount += 1;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the user application checks first, then the window and Definition File checks.
 */
uint32_t Arinc661_Context_Check(Arinc661_Context_t* context)
{
    Arinc661_Context_UserApplication_t application;
    Arinc661_Context_UserApplication_t other;
    Arinc661_Context_Definition_t      definition;
    const Arinc661_Definition_t*       model;
    const char_t*                      document_name;
    const char_t*                      window_name;
    char_t                             text[ARINC661_CONTEXT_MAX_PROBLEM_BYTES];
    float32_t                          layer_width;
    float32_t                          layer_height;
    float32_t                          units_per_pixel_x;
    float32_t                          units_per_pixel_y;
    int64_t                            pixel_width;
    int64_t                            pixel_height;
    int64_t                            window_id;
    int64_t                            application_id;
    int64_t                            other_id;
    uint32_t                           application_index;
    uint32_t                           other_index;
    uint32_t                           document_index;
    uint32_t                           other_document;
    uint32_t                           window_index;
    uint32_t                           layer_index;
    bool_t                             has_application;
    bool_t                             has_window;

    LOGASSERT(NULL != context);
    context->problemCount = 0;
    context->errorCount   = 0;
    for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
    {
        if ('\0' == application.dFPath[0])
        {
            (void)snprintf(text, sizeof(text), "%s: application %lld has no Definition File (dFPath is empty).", application.documentName, (long long)application.applicationId);
            AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_USER_APPLICATION, application.documentIndex, application.applicationIndex, text);
        }
        else if (FALSE == Arinc661_Context_FindDefinition(context, application.dFPath, &definition))
        {
            (void)snprintf(text, sizeof(text), "%s: application %lld names %s, which is not a Definition File in the folder.", application.documentName, (long long)application.applicationId, application.dFPath);
            AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_USER_APPLICATION, application.documentIndex, application.applicationIndex, text);
        }
        else if (FALSE == definition.isOpen)
        {
            (void)snprintf(text, sizeof(text), "%s: application %lld references %s, which is not open; open it to preview and check it.", application.documentName, (long long)application.applicationId, application.dFPath);
            AddProblem(context, FALSE, ARINC661_CONTEXT_TARGET_USER_APPLICATION, application.documentIndex, application.applicationIndex, text);
        }
        else
        {
            if (FALSE == definition.hasApplicationId)
            {
                (void)snprintf(text, sizeof(text), "%s has no numeric ApplicationId; application %lld expects it.", application.dFPath, (long long)application.applicationId);
                AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_DEFINITION, definition.documentIndex, 0, text);
            }
            else if (definition.applicationId != application.applicationId)
            {
                (void)snprintf(text, sizeof(text), "%s carries ApplicationId %lld but %s application %lld references it (ARINC 661 6.5.1.2).", application.dFPath, (long long)definition.applicationId, application.documentName, (long long)application.applicationId);
                AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_DEFINITION, definition.documentIndex, 0, text);
            }
            if (0 == definition.layerCount)
            {
                (void)snprintf(text, sizeof(text), "%s has no layers; application %lld shows nothing.", application.dFPath, (long long)application.applicationId);
                AddProblem(context, FALSE, ARINC661_CONTEXT_TARGET_DEFINITION, definition.documentIndex, 0, text);
            }
            // Layer size against the window's size in layer units
            if (Arinc661_Configuration_Window(context->configurationEditor, application.documentIndex, application.windowId, &window_index, &pixel_width, &pixel_height, &units_per_pixel_x, &units_per_pixel_y))
            {
                model = Arinc661_Definition_Document(context->definitionEditor, definition.documentIndex);
                for (layer_index = 0; (NULL != model) && (layer_index < model->layerCount); ++layer_index)
                {
                    Arinc661_Scene_LayerSize(model, layer_index, &layer_width, &layer_height);
                    if ((layer_width > ((float32_t)pixel_width * units_per_pixel_x * 1.001f)) || (layer_height > ((float32_t)pixel_height * units_per_pixel_y * 1.001f)))
                    {
                        (void)snprintf(text, sizeof(text), "%s layer %u is %.0f x %.0f but window %lld of %s holds %.0f x %.0f (0.01 mm); the layer will be cut off.", application.dFPath, (unsigned)layer_index, (float64_t)layer_width, (float64_t)layer_height, (long long)application.windowId, application.documentName, (float64_t)((float32_t)pixel_width * units_per_pixel_x), (float64_t)((float32_t)pixel_height * units_per_pixel_y));
                        AddProblem(context, FALSE, ARINC661_CONTEXT_TARGET_LAYER, definition.documentIndex, layer_index, text);
                    }
                }
            }
        }
        if (FALSE == Arinc661_Configuration_Window(context->configurationEditor, application.documentIndex, application.windowId, NULL, NULL, NULL, NULL, NULL))
        {
            (void)snprintf(text, sizeof(text), "%s: application %lld targets window %lld, which does not exist.", application.documentName, (long long)application.applicationId, (long long)application.windowId);
            AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_USER_APPLICATION, application.documentIndex, application.applicationIndex, text);
        }
        // Duplicate applicationId inside one configuration
        for (other_index = application_index + 1; Arinc661_Context_UserApplication(context, other_index, &other); ++other_index)
        {
            if ((other.documentIndex == application.documentIndex) && (other.applicationId == application.applicationId))
            {
                (void)snprintf(text, sizeof(text), "%s: applicationId %lld is used twice.", application.documentName, (long long)application.applicationId);
                AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_USER_APPLICATION, other.documentIndex, other.applicationIndex, text);
            }
        }
    }
    // Windows nobody targets
    for (document_index = 0; document_index < ARINC661_CONFIGURATION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        for (window_index = 0; Arinc661_Configuration_WindowAt(context->configurationEditor, document_index, window_index, &window_id, &window_name); ++window_index)
        {
            has_application = FALSE;
            for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
            {
                if ((application.documentIndex == document_index) && (application.windowId == window_id))
                {
                    has_application = TRUE;
                }
            }
            if (FALSE == has_application)
            {
                document_name = Arinc661_Configuration_DocumentName(context->configurationEditor, document_index);
                (void)snprintf(text, sizeof(text), "%s: window %lld (%s) has no user application; nothing will be shown in it.", document_name, (long long)window_id, window_name);
                AddProblem(context, FALSE, ARINC661_CONTEXT_TARGET_WINDOW, document_index, window_index, text);
            }
        }
    }
    // Open Definition Files nobody references, and duplicate ApplicationIds across them
    for (document_index = 0; document_index < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++document_index)
    {
        document_name = Arinc661_Definition_DocumentName(context->definitionEditor, document_index);
        if (NULL == document_name)
        {
            continue;
        }
        has_application = FALSE;
        for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
        {
            if (0 == _stricmp(application.dFPath, document_name))
            {
                has_application = TRUE;
            }
        }
        has_window = (0 < Arinc661_Context_UserApplicationCount(context));
        if ((FALSE == has_application) && has_window)
        {
            (void)snprintf(text, sizeof(text), "%s is open but no user application references it; set a dFPath to it or create one from its inspector.", document_name);
            AddProblem(context, FALSE, ARINC661_CONTEXT_TARGET_DEFINITION, document_index, 0, text);
        }
        if (Arinc661_Definition_DocumentApplicationId(context->definitionEditor, document_index, &application_id))
        {
            for (other_document = document_index + 1; other_document < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS; ++other_document)
            {
                if (Arinc661_Definition_DocumentApplicationId(context->definitionEditor, other_document, &other_id) && (other_id == application_id))
                {
                    (void)snprintf(text, sizeof(text), "%s and %s both carry ApplicationId %lld; a CDS cannot tell them apart.", document_name, Arinc661_Definition_DocumentName(context->definitionEditor, other_document), (long long)application_id);
                    AddProblem(context, TRUE, ARINC661_CONTEXT_TARGET_DEFINITION, other_document, 0, text);
                }
            }
        }
    }

    return context->problemCount;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Each target kind maps to one editor selection call.
 */
void Arinc661_Context_ShowProblem(Arinc661_Context_t* context, const Arinc661_Context_Problem_t* problem)
{
    LOGASSERT(NULL != context);
    LOGASSERT(NULL != problem);
    switch (problem->target)
    {
        case ARINC661_CONTEXT_TARGET_USER_APPLICATION:
            Arinc661_Configuration_SelectUserApplication(context->configurationEditor, problem->documentIndex, problem->itemIndex);
            context->isDefinitionFocused = FALSE;
            context->hasFocusRequest     = TRUE;
            break;
        case ARINC661_CONTEXT_TARGET_WINDOW:
            Arinc661_Configuration_SelectWindow(context->configurationEditor, problem->documentIndex, problem->itemIndex);
            context->isDefinitionFocused = FALSE;
            context->hasFocusRequest     = TRUE;
            break;
        case ARINC661_CONTEXT_TARGET_DEFINITION:
            Arinc661_Definition_SelectDocument(context->definitionEditor, problem->documentIndex);
            context->isDefinitionFocused = TRUE;
            context->hasFocusRequest     = TRUE;
            break;
        case ARINC661_CONTEXT_TARGET_LAYER:
            Arinc661_Definition_SelectLayer(context->definitionEditor, problem->documentIndex, problem->itemIndex);
            context->isDefinitionFocused = TRUE;
            context->hasFocusRequest     = TRUE;
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Loads each referenced file present in the folder that is not open yet; the active
 *  document stays where it was.
 */
uint32_t Arinc661_Context_OpenReferencedDefinitions(Arinc661_Context_t* context)
{
    Arinc661_Context_UserApplication_t application;
    Arinc661_Context_Definition_t      definition;
    uint32_t                           application_index;
    uint32_t                           active;
    uint32_t                           opened;

    LOGASSERT(NULL != context);
    opened = 0;
    active = context->definitionEditor->activeDocumentIndex;
    for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
    {
        if (Arinc661_Context_FindDefinition(context, application.dFPath, &definition) && definition.isInFolder && (FALSE == definition.isOpen))
        {
            Arinc661_Definition_LoadFile(context->definitionEditor, application.dFPath);
            if (ARINC661_DEFINITION_NO_DOCUMENT != Arinc661_Definition_FindDocument(context->definitionEditor, application.dFPath))
            {
                opened += 1;
            }
        }
    }
    if ((0 < opened) && (UINT32_MAX != active) && (active < ARINC661_DEFINITION_MAX_OPEN_DOCUMENTS) && (NULL != Arinc661_Definition_DocumentName(context->definitionEditor, active)))
    {
        Arinc661_Definition_SelectDocument(context->definitionEditor, active);
    }

    return opened;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The layer takes the window's size converted to 0.01 mm; a user application without a
 *  dFPath gets one named after its applicationId.
 */
bool_t Arinc661_Context_CreateDefinitionFor(Arinc661_Context_t* context, const Arinc661_Context_UserApplication_t* application)
{
    char_t    file_name[ARINC661_CONTEXT_MAX_FILE_NAME_BYTES];
    float32_t units_per_pixel_x;
    float32_t units_per_pixel_y;
    float32_t layer_width;
    float32_t layer_height;
    int64_t   pixel_width;
    int64_t   pixel_height;
    bool_t    is_created;

    LOGASSERT(NULL != context);
    LOGASSERT(NULL != application);
    if ('\0' != application->dFPath[0])
    {
        strncpy_s(file_name, sizeof(file_name), application->dFPath, _TRUNCATE);
    }
    else
    {
        (void)snprintf(file_name, sizeof(file_name), "application_%lld.xml", (long long)application->applicationId);
    }
    layer_width  = 10000.0f;
    layer_height = 10000.0f;
    if (Arinc661_Configuration_Window(context->configurationEditor, application->documentIndex, application->windowId, NULL, &pixel_width, &pixel_height, &units_per_pixel_x, &units_per_pixel_y))
    {
        layer_width  = ((float32_t)pixel_width * units_per_pixel_x);
        layer_height = ((float32_t)pixel_height * units_per_pixel_y);
    }
    is_created = Arinc661_Definition_CreateFor(context->definitionEditor, file_name, application->applicationId, layer_width, layer_height);
    if (is_created)
    {
        if ('\0' == application->dFPath[0])
        {
            (void)Arinc661_Configuration_RepointDefinition(context->configurationEditor, "", file_name);
        }
        context->isDefinitionFocused = TRUE;
        context->hasFocusRequest     = TRUE;
    }

    return is_created;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The user application takes the Definition File's ApplicationId, or the next free id.
 */
bool_t Arinc661_Context_CreateUserApplicationFor(Arinc661_Context_t* context, uint32_t definitionDocument, uint32_t configurationDocument, int64_t windowId)
{
    Arinc661_Context_UserApplication_t application;
    const char_t*                      file_name;
    int64_t                            application_id;
    uint32_t                           application_index;
    bool_t                             is_created;

    LOGASSERT(NULL != context);
    is_created = FALSE;
    file_name  = Arinc661_Definition_DocumentName(context->definitionEditor, definitionDocument);
    if (NULL != file_name)
    {
        if (FALSE == Arinc661_Definition_DocumentApplicationId(context->definitionEditor, definitionDocument, &application_id))
        {
            application_id = 1;
            for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
            {
                if ((application.documentIndex == configurationDocument) && (application.applicationId >= application_id))
                {
                    application_id = (application.applicationId + 1);
                }
            }
        }
        is_created = Arinc661_Configuration_AddUserApplicationFor(context->configurationEditor, configurationDocument, application_id, windowId, file_name);
        if (is_created)
        {
            context->isDefinitionFocused = FALSE;
            context->hasFocusRequest     = TRUE;
        }
    }

    return is_created;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Counts through the user applications that name the file and returns the index-th one's
 *  window in layer units.
 */
bool_t Arinc661_Context_WindowFrame(const Arinc661_Context_t* context, const char_t* fileName, uint32_t index, float32_t* widthUnits, float32_t* heightUnits, char_t* label, uint32_t labelCapacity)
{
    Arinc661_Context_UserApplication_t application;
    float32_t                          units_per_pixel_x;
    float32_t                          units_per_pixel_y;
    int64_t                            pixel_width;
    int64_t                            pixel_height;
    uint32_t                           application_index;
    uint32_t                           seen;
    bool_t                             is_found;

    LOGASSERT(NULL != context);
    LOGASSERT(NULL != fileName);
    LOGASSERT(NULL != widthUnits);
    LOGASSERT(NULL != heightUnits);
    LOGASSERT(NULL != label);
    is_found = FALSE;
    seen     = 0;
    for (application_index = 0; Arinc661_Context_UserApplication(context, application_index, &application); ++application_index)
    {
        if (0 != _stricmp(application.dFPath, fileName))
        {
            continue;
        }
        if (seen == index)
        {
            if (Arinc661_Configuration_Window(context->configurationEditor, application.documentIndex, application.windowId, NULL, &pixel_width, &pixel_height, &units_per_pixel_x, &units_per_pixel_y))
            {
                (*widthUnits)  = ((float32_t)pixel_width * units_per_pixel_x);
                (*heightUnits) = ((float32_t)pixel_height * units_per_pixel_y);
                (void)snprintf(label, labelCapacity, "%s: application %lld, window %lld", application.documentName, (long long)application.applicationId, (long long)application.windowId);
                is_found = TRUE;
            }
            break;
        }
        seen += 1;
    }

    return is_found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Consumes the Definition File editor's rename record.
 */
uint32_t Arinc661_Context_FollowRenames(Arinc661_Context_t* context)
{
    const char_t* previous_name;
    const char_t* current_name;
    uint32_t      count;

    LOGASSERT(NULL != context);
    count = 0;
    if (Arinc661_Definition_TakeRename(context->definitionEditor, &previous_name, &current_name))
    {
        count = Arinc661_Configuration_RepointDefinition(context->configurationEditor, previous_name, current_name);
        context->definitionEditor->renamedFrom[0] = '\0';
        context->definitionEditor->renamedTo[0]   = '\0';
    }

    return count;
}
