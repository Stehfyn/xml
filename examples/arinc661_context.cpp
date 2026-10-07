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
#include "arinc661_configuration.h"
#include "arinc661_definition_editor.h"

extern "C"
{
#include "os/os_assert.h"
}

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
