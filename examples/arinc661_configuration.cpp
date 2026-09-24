/*=================================================================================================
** 1. References
**===============================================================================================*/

/* ARINC 661 configuration editor and visualizer. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
}

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define CONFIGURATION_WINDOW_NAME ("ARINC 661 Configuration")
#define VISUALIZER_WINDOW_NAME ("Visualizer")
#define DOCKSPACE_NAME ("ARINC661DockSpace")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Unit choices used by the configuration example.
 */
static const char_t* const c_unitOptions[] =
{
    "pixel",
    "inch",
    "screen"
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Copies parsed attribute values into editable storage.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     node     XML node
 *
 * \retval None
 */
static bool_t CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Finds an editable value by its XML attribute address.
 *
 * \param[in] editor      Editor state
 * \param[in] attribute   XML attribute
 *
 * \return Editable value index, or the array capacity when absent.
 */
static uint32_t FindEditableValueIndex(const ConfigurationEditor_t* editor, const Utl_Xml_Attribute_t* attribute);

/*************************************************************************************************/
/**
 * \brief Reads the configuration through Utl_Xml_ReadDocument.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void LoadConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Validates edited values and writes the document with Utl_Xml_WriteDocument.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void BuildConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Allocates a stable XML element for an editor addition.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     name     Null-terminated XML name
 *
 * \return Pointer to stable, initialized element storage.
 */
static Utl_Xml_Node_t* CreateElement(ConfigurationEditor_t* editor, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Adds a stable XML attribute and an editable value.
 *
 * \param[in,out] editor   Editor state
 * \param[in,out] node     XML node
 * \param[in]     name     Null-terminated XML name
 * \param[in]     value    Attribute value
 *
 * \retval None
 */
static void CreateAttribute(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Finds the largest numeric ID for one element type in a subtree.
 *
 * \param[in]     editor        Editor state
 * \param[in]     node          XML node
 * \param[in]     elementName   Element name to match
 * \param[in,out] maximumId     Largest matching ID found
 * \param[in,out] foundId       Whether a matching ID was found
 *
 * \retval None
 */
static void FindMaximumId(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* elementName, uint32_t* maximumId, bool_t* foundId);

/*************************************************************************************************/
/**
 * \brief Chooses the next ID within the requested subtree.
 *
 * \param[in]     editor        Editor state
 * \param[in]     root          Root of the subtree
 * \param[in]     elementName   Element name to match
 * \param[out]    nextId        Generated next ID
 *
 * \return TRUE if an ID is available; FALSE on overflow.
 */
static bool_t NextElementId(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* root, const char_t* elementName, uint32_t* nextId);

/*************************************************************************************************/
/**
 * \brief Finds the configuration document element.
 *
 * \param[in,out] editor   Editor state
 *
 * \return Document element, or NULL when absent.
 */
static Utl_Xml_Node_t* FindDocumentElement(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Removes editable values belonging to a detached subtree.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     node     XML node
 *
 * \retval None
 */
static void ForgetNodeValues(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Appends a screen with editable pixel and physical dimensions.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void AddScreen(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Appends a layout to a screen.
 *
 * \param[in,out] editor   Editor state
 * \param[in,out] screen   Screen element
 *
 * \retval None
 */
static void AddLayout(ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen);

/*************************************************************************************************/
/**
 * \brief Appends a window with a pixel area and scaling factor.
 *
 * \param[in,out] editor   Editor state
 * \param[in,out] layout   Layout element
 *
 * \retval None
 */
static void AddWindow(ConfigurationEditor_t* editor, Utl_Xml_Node_t* layout);

/*************************************************************************************************/
/**
 * \brief Detaches the selected screen, layout, or window from its parent.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void RemoveSelectedNode(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Includes only screens, layouts, and windows in the hierarchy.
 *
 * \param[in]     node   XML node
 *
 * \return TRUE for a screen, layout, or window; FALSE otherwise.
 */
static bool_t IsHierarchyNode(const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Shows one element in the configuration hierarchy.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     node     XML node
 *
 * \retval None
 */
static void DrawConfigurationNode(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Compares a counted attribute name with a literal.
 *
 * \param[in]     attribute   XML attribute
 * \param[in]     name        Null-terminated XML name
 *
 * \return TRUE if the attribute has the requested name; FALSE otherwise.
 */
static bool_t AttributeHasName(const Utl_Xml_Attribute_t* attribute, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Parses an unsigned integer for an inspector number field.
 *
 * \param[in]     text    Editable numeric text
 * \param[out]    value   Attribute value
 *
 * \return TRUE for a valid unsigned value; FALSE otherwise.
 */
static bool_t ParseUnsignedValue(const char_t* text, uint32_t* value);

/*************************************************************************************************/
/**
 * \brief Uses numeric fields and unit choices for known configuration attributes.
 *
 * \param[in]     ownerNode   Element that owns the attribute
 * \param[in]     attribute   XML attribute
 * \param[in,out] value       Attribute value
 *
 * \retval None
 */
static void DrawAttributeControl(const Utl_Xml_Node_t* ownerNode, Utl_Xml_Attribute_t* attribute, char_t* value);

/*************************************************************************************************/
/**
 * \brief Shows a node's attributes as one two-column property group.
 *
 * \param[in,out] editor    Editor state
 * \param[in]     node      XML node
 * \param[in]     heading   Inspector section heading
 *
 * \retval None
 */
static void DrawAttributeSection(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* heading);

/*************************************************************************************************/
/**
 * \brief Groups screen and window details under their owning selection.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void DrawSelectedNodeInspector(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Shows a hierarchy above the selected element's inspector.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void DrawConfigurationEditor(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Compares a counted XML node name with a literal.
 *
 * \param[in]     node   XML node
 * \param[in]     name   Null-terminated XML name
 *
 * \return TRUE if the node has the requested name; FALSE otherwise.
 */
static bool_t NodeHasName(const Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds a direct child with the requested element name.
 *
 * \param[in]     parent   Parent element
 * \param[in]     name     Null-terminated XML name
 *
 * \return Matching direct child, or NULL when absent.
 */
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds an edited value by its XML attribute name.
 *
 * \param[in]     editor   Editor state
 * \param[in]     node     XML node
 * \param[in]     name     Null-terminated XML name
 *
 * \return Editable value, or NULL when the attribute is absent.
 */
static const char_t* FindAttributeValue(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a nonnegative finite pixel coordinate from an edited value.
 *
 * \param[in]     text    Editable numeric text
 * \param[out]    value   Attribute value
 *
 * \return TRUE for a finite nonnegative value; FALSE otherwise.
 */
static bool_t ParsePixelValue(const char_t* text, float* value);

/*************************************************************************************************/
/**
 * \brief Reads a window pixel area from the current editor values.
 *
 * \param[in]     editor   Editor state
 * \param[in]     node     XML node
 * \param[out]    x        Pixel x coordinate
 * \param[out]    y        Pixel y coordinate
 * \param[out]    width    Pixel width
 * \param[out]    height   Pixel height
 *
 * \return TRUE if all four coordinates are valid; FALSE otherwise.
 */
static bool_t ReadPixelRectangle(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, float* x, float* y, float* width, float* height);

/*************************************************************************************************/
/**
 * \brief Reads positive screen dimensions from the current editor values.
 *
 * \param[in]  editor   Editor state
 * \param[in]  screen   Screen element
 * \param[out] width    Pixel width
 * \param[out] height   Pixel height
 *
 * \return TRUE when both dimensions are positive; FALSE otherwise.
 */
static bool_t ReadScreenDimensions(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen, float* width, float* height);

/*************************************************************************************************/
/**
 * \brief Draws configured windows inside a screen rectangle.
 *
 * \param[in]     editor     Editor state
 * \param[in]     screen     Screen element
 * \param[in,out] drawList   ImGui drawing list
 * \param[in]     origin     Screen origin in display coordinates
 * \param[in]     scale      Display pixels per configuration pixel
 *
 * \retval None
 */
static void DrawScreenWindows(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen, ImDrawList* drawList, const ImVec2* origin, float scale);

/*************************************************************************************************/
/**
 * \brief Shows screen sizes and window pixel areas from the current field values.
 *
 * \param[in]     editor   Editor state
 *
 * \retval None
 */
static void DrawVisualizer(const ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Places the configuration on the left and the visualizer in the center.
 *
 * \param[in]     dockspaceId   Viewport dockspace identifier
 *
 * \retval None
 */
static void BuildDockLayout(ImGuiID dockspaceId);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies parsed attribute values into editable storage.
 */
static bool_t CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t* attribute;
    Utl_Xml_Node_t*      child;
    AttributeValue_t*    editable_value;

    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        if ((CONFIGURATION_MAX_EDITABLE_ATTRIBUTES == editor->attributeValueCount) ||
            (CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES < attribute->valueByteCount))
        {
            strcpy(editor->status, "The XML file exceeds the editor's attribute limits.");
            return FALSE;
        }
        editable_value            = &editor->attributeValues[editor->attributeValueCount];
        ++editor->attributeValueCount;
        editable_value->attribute = attribute;
        memcpy(editable_value->value, attribute->value, attribute->valueByteCount);
        editable_value->value[attribute->valueByteCount] = '\0';
    }
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        if (FALSE == CollectAttributes(editor, child))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds an editable value by its XML attribute address.
 */
static uint32_t FindEditableValueIndex(const ConfigurationEditor_t* editor, const Utl_Xml_Attribute_t* attribute)
{
    uint32_t value_index;

    for (value_index = 0; value_index < editor->attributeValueCount; ++value_index)
    {
        if (attribute == editor->attributeValues[value_index].attribute)
        {
            return value_index;
        }
    }
    return CONFIGURATION_MAX_EDITABLE_ATTRIBUTES;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the configuration through Utl_Xml_ReadDocument.
 */
static void LoadConfiguration(ConfigurationEditor_t* editor)
{
    FILE*           xml_file;
    size_t          document_byte_count;
    int             read_error;
    int             close_result;
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* first_screen;

    editor->isLoaded     = FALSE;
    editor->selectedNode = NULL;
    editor->attributeValueCount = 0;
    editor->addedNodeCount = 0;
    editor->addedAttributeCount = 0;

    xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "rb");
    if (NULL == xml_file)
    {
        strcpy(editor->status, "Could not open arinc661_configuration.xml.");
        return;
    }
    document_byte_count = fread(editor->inputBuffer, 1, CONFIGURATION_MAX_DOCUMENT_BYTES + 1, xml_file);
    read_error          = ferror(xml_file);
    close_result        = fclose(xml_file);
    if ((0 != read_error) || (0 != close_result))
    {
        strcpy(editor->status, "Could not read arinc661_configuration.xml.");
        return;
    }
    if (CONFIGURATION_MAX_DOCUMENT_BYTES < document_byte_count)
    {
        strcpy(editor->status, "The XML file exceeds the reader's storage limits.");
        return;
    }
    if (0 == document_byte_count)
    {
        strcpy(editor->status, "The XML file is empty.");
        return;
    }
    editor->inputBuffer[document_byte_count] = 0;
    Utl_Xml_InitializeDocument(&editor->document);
    Utl_Xml_ReadDocument(
        &editor->document,
        editor->inputBuffer,
        (uint64_t)document_byte_count,
        editor->nodes,
        UTL_XML_DOCUMENT_MAX_NODES,
        editor->attributes,
        CONFIGURATION_MAX_PARSED_ATTRIBUTES);

    for (node = editor->document.firstNode; NULL != node; node = node->nextSibling)
    {
        if (FALSE == CollectAttributes(editor, node))
        {
            return;
        }
        if ((NULL == editor->selectedNode) && (UTL_XML_NODE_ELEMENT == node->type))
        {
            editor->selectedNode = node;
        }
    }
    if (NULL != editor->selectedNode)
    {
        first_screen = FindChild(editor->selectedNode, "screen");
        if (NULL != first_screen)
        {
            editor->selectedNode = first_screen;
        }
    }
    editor->isLoaded = TRUE;
    strcpy(editor->status, "Loaded arinc661_configuration.xml.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates edited values and writes the document with Utl_Xml_WriteDocument.
 */
static void BuildConfiguration(ConfigurationEditor_t* editor)
{
    AttributeValue_t* value;
    FILE*             xml_file;
    uint32_t          value_index;
    uint32_t          value_byte_count;
    uint64_t          output_byte_count;
    size_t            bytes_written;
    int               close_result;

    for (value_index = 0; value_index < editor->attributeValueCount; ++value_index)
    {
        value = &editor->attributeValues[value_index];
        if (NULL == value->attribute)
        {
            continue;
        }
        value_byte_count = (uint32_t)strlen(value->value);
        if (FALSE == Utl_Xml_IsAttributeValue((const byte_t*)value->value, value_byte_count))
        {
            snprintf(
                editor->status,
                sizeof(editor->status),
                "Invalid XML attribute value: %.*s",
                (int)value->attribute->nameByteCount,
                (const char_t*)value->attribute->name);
            return;
        }
    }

    for (value_index = 0; value_index < editor->attributeValueCount; ++value_index)
    {
        value = &editor->attributeValues[value_index];
        if (NULL != value->attribute)
        {
            value->attribute->value          = (const byte_t*)value->value;
            value->attribute->valueByteCount = (uint32_t)strlen(value->value);
        }
    }
    output_byte_count = Utl_Xml_WriteDocument(
        &editor->document,
        editor->outputBuffer,
        CONFIGURATION_MAX_DOCUMENT_BYTES);

    xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "wb");
    if (NULL == xml_file)
    {
        strcpy(editor->status, "Could not write arinc661_configuration.xml.");
        return;
    }
    bytes_written = fwrite(editor->outputBuffer, 1, (size_t)output_byte_count, xml_file);
    close_result  = fclose(xml_file);
    if ((output_byte_count != bytes_written) || (0 != close_result))
    {
        strcpy(editor->status, "Could not finish writing arinc661_configuration.xml.");
        return;
    }
    strcpy(editor->status, "Built arinc661_configuration.xml.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Allocates a stable XML element for an editor addition.
 */
static Utl_Xml_Node_t* CreateElement(ConfigurationEditor_t* editor, const char_t* name)
{
    Utl_Xml_Node_t* node;

    LOGASSERT(editor->addedNodeCount < CONFIGURATION_MAX_ADDED_NODES);
    node = &editor->addedNodes[editor->addedNodeCount];
    ++editor->addedNodeCount;
    Utl_Xml_InitializeNode(node, UTL_XML_NODE_ELEMENT, (const byte_t*)name, (uint32_t)strlen(name));
    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds a stable XML attribute and an editable value.
 */
static void CreateAttribute(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* attribute;
    AttributeValue_t*    editable_value;

    LOGASSERT(editor->addedAttributeCount < CONFIGURATION_MAX_ADDED_ATTRIBUTES);
    LOGASSERT(editor->attributeValueCount < CONFIGURATION_MAX_EDITABLE_ATTRIBUTES);
    LOGASSERT(strlen(value) <= CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES);
    attribute = &editor->addedAttributes[editor->addedAttributeCount];
    ++editor->addedAttributeCount;
    editable_value = &editor->attributeValues[editor->attributeValueCount];
    ++editor->attributeValueCount;
    editable_value->attribute = attribute;
    strcpy(editable_value->value, value);
    Utl_Xml_InitializeAttribute(
        attribute,
        (const byte_t*)name,
        (uint32_t)strlen(name),
        (const byte_t*)editable_value->value,
        (uint32_t)strlen(editable_value->value));
    Utl_Xml_AppendAttribute(node, attribute);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds the largest numeric ID for one element type in a subtree.
 */
static void FindMaximumId(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* elementName, uint32_t* maximumId, bool_t* foundId)
{
    const char_t*   id_text;
    Utl_Xml_Node_t* child;
    uint32_t        id;

    if (TRUE == NodeHasName(node, elementName))
    {
        id_text = FindAttributeValue(editor, node, "id");
        if ((NULL != id_text) && (TRUE == ParseUnsignedValue(id_text, &id)))
        {
            if ((FALSE == *foundId) || (*maximumId < id))
            {
                *maximumId = id;
            }
            *foundId = TRUE;
        }
    }
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        FindMaximumId(editor, child, elementName, maximumId, foundId);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Chooses the next ID within the requested subtree.
 */
static bool_t NextElementId(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* root, const char_t* elementName, uint32_t* nextId)
{
    uint32_t maximum_id;
    bool_t   found_id;

    maximum_id = 0;
    found_id   = FALSE;
    FindMaximumId(editor, root, elementName, &maximum_id, &found_id);
    if ((TRUE == found_id) && (UINT32_MAX == maximum_id))
    {
        return FALSE;
    }
    *nextId = (TRUE == found_id) ? maximum_id + 1 : 0;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds the configuration document element.
 */
static Utl_Xml_Node_t* FindDocumentElement(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* node;

    for (node = editor->document.firstNode; NULL != node; node = node->nextSibling)
    {
        if (UTL_XML_NODE_ELEMENT == node->type)
        {
            return node;
        }
    }
    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes editable values belonging to a detached subtree.
 */
static void ForgetNodeValues(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t* attribute;
    Utl_Xml_Node_t*      child;
    uint32_t             value_index;

    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        value_index = FindEditableValueIndex(editor, attribute);
        LOGASSERT(value_index < editor->attributeValueCount);
        editor->attributeValues[value_index].attribute = NULL;
    }
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        ForgetNodeValues(editor, child);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Appends a screen with editable pixel and physical dimensions.
 */
static void AddScreen(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* root;
    Utl_Xml_Node_t* screen;
    Utl_Xml_Node_t* pixel_size;
    Utl_Xml_Node_t* physical_dimensions;
    uint32_t        next_id;
    char_t          id_text[16];

    root = FindDocumentElement(editor);
    if ((3 > CONFIGURATION_MAX_ADDED_NODES - editor->addedNodeCount) ||
        (7 > CONFIGURATION_MAX_ADDED_ATTRIBUTES - editor->addedAttributeCount) ||
        (7 > CONFIGURATION_MAX_EDITABLE_ATTRIBUTES - editor->attributeValueCount))
    {
        strcpy(editor->status, "The editor's storage is full.");
        return;
    }
    if ((NULL == root) || (FALSE == NextElementId(editor, root, "screen", &next_id)))
    {
        strcpy(editor->status, "Could not allocate a screen ID.");
        return;
    }
    snprintf(id_text, sizeof(id_text), "%u", next_id);
    screen = CreateElement(editor, "screen");
    CreateAttribute(editor, screen, "id", id_text);

    pixel_size = CreateElement(editor, "pixelSize");
    CreateAttribute(editor, pixel_size, "width", "640");
    CreateAttribute(editor, pixel_size, "height", "480");
    CreateAttribute(editor, pixel_size, "units", "pixel");
    Utl_Xml_AppendChild(screen, pixel_size);

    physical_dimensions = CreateElement(editor, "physicalDimensions");
    CreateAttribute(editor, physical_dimensions, "width", "5");
    CreateAttribute(editor, physical_dimensions, "height", "3.75");
    CreateAttribute(editor, physical_dimensions, "units", "inch");
    Utl_Xml_AppendChild(screen, physical_dimensions);

    Utl_Xml_AppendChild(root, screen);
    editor->selectedNode = screen;
    strcpy(editor->status, "Added screen. Build XML to save.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Appends a layout to a screen.
 */
static void AddLayout(ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen)
{
    Utl_Xml_Node_t* layout;
    uint32_t        next_id;
    char_t          id_text[16];

    if ((1 > CONFIGURATION_MAX_ADDED_NODES - editor->addedNodeCount) ||
        (1 > CONFIGURATION_MAX_ADDED_ATTRIBUTES - editor->addedAttributeCount) ||
        (1 > CONFIGURATION_MAX_EDITABLE_ATTRIBUTES - editor->attributeValueCount))
    {
        strcpy(editor->status, "The editor's storage is full.");
        return;
    }
    if (FALSE == NextElementId(editor, screen, "layout", &next_id))
    {
        strcpy(editor->status, "Could not allocate a layout ID.");
        return;
    }
    snprintf(id_text, sizeof(id_text), "%u", next_id);
    layout = CreateElement(editor, "layout");
    CreateAttribute(editor, layout, "id", id_text);
    Utl_Xml_AppendChild(screen, layout);
    editor->selectedNode = layout;
    strcpy(editor->status, "Added layout. Build XML to save.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Appends a window with a pixel area and scaling factor.
 */
static void AddWindow(ConfigurationEditor_t* editor, Utl_Xml_Node_t* layout)
{
    Utl_Xml_Node_t*    root;
    Utl_Xml_Node_t*    screen;
    Utl_Xml_Node_t*    pixel_size;
    Utl_Xml_Node_t*    window;
    Utl_Xml_Node_t*    pixel_area;
    Utl_Xml_Node_t*    scaling_factor;
    const char_t*    edited_width;
    const char_t*    edited_height;
    const char_t*    width_text;
    const char_t*    height_text;
    uint32_t         next_id;
    uint32_t         width;
    uint32_t         height;
    char_t           id_text[16];
    char_t           name_text[32];

    if ((3 > CONFIGURATION_MAX_ADDED_NODES - editor->addedNodeCount) ||
        (10 > CONFIGURATION_MAX_ADDED_ATTRIBUTES - editor->addedAttributeCount) ||
        (10 > CONFIGURATION_MAX_EDITABLE_ATTRIBUTES - editor->attributeValueCount))
    {
        strcpy(editor->status, "The editor's storage is full.");
        return;
    }
    root = FindDocumentElement(editor);
    if ((NULL == root) || (FALSE == NextElementId(editor, root, "window", &next_id)))
    {
        strcpy(editor->status, "Could not allocate a window ID.");
        return;
    }
    screen      = layout->parent;
    pixel_size  = FindChild(screen, "pixelSize");
    width_text  = "640";
    height_text = "480";
    if (NULL != pixel_size)
    {
        edited_width  = FindAttributeValue(editor, pixel_size, "width");
        edited_height = FindAttributeValue(editor, pixel_size, "height");
        if ((NULL != edited_width) && (TRUE == ParseUnsignedValue(edited_width, &width)) && (0 < width))
        {
            width_text = edited_width;
        }
        if ((NULL != edited_height) && (TRUE == ParseUnsignedValue(edited_height, &height)) && (0 < height))
        {
            height_text = edited_height;
        }
    }

    snprintf(id_text, sizeof(id_text), "%u", next_id);
    snprintf(name_text, sizeof(name_text), "window%s", id_text);
    window = CreateElement(editor, "window");
    CreateAttribute(editor, window, "id", id_text);
    CreateAttribute(editor, window, "name", name_text);

    pixel_area = CreateElement(editor, "pixelArea");
    CreateAttribute(editor, pixel_area, "x", "0");
    CreateAttribute(editor, pixel_area, "y", "0");
    CreateAttribute(editor, pixel_area, "width", width_text);
    CreateAttribute(editor, pixel_area, "height", height_text);
    Utl_Xml_AppendChild(window, pixel_area);

    scaling_factor = CreateElement(editor, "scalingFactor");
    CreateAttribute(editor, scaling_factor, "xScale", "20.0");
    CreateAttribute(editor, scaling_factor, "yScale", "20.0");
    CreateAttribute(editor, scaling_factor, "baseUnit", "screen");
    CreateAttribute(editor, scaling_factor, "perUnit", "pixel");
    Utl_Xml_AppendChild(window, scaling_factor);

    Utl_Xml_AppendChild(layout, window);
    editor->selectedNode = window;
    strcpy(editor->status, "Added window. Build XML to save.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Detaches the selected screen, layout, or window from its parent.
 */
static void RemoveSelectedNode(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* parent;
    Utl_Xml_Node_t* previous;
    Utl_Xml_Node_t* current;
    Utl_Xml_Node_t* next_selection;

    node = editor->selectedNode;
    if ((NULL == node) || (FALSE == IsHierarchyNode(node)))
    {
        return;
    }
    parent = node->parent;
    LOGASSERT(NULL != parent);
    previous = NULL;
    current  = parent->firstChild;
    while ((NULL != current) && (node != current))
    {
        previous = current;
        current  = current->nextSibling;
    }
    LOGASSERT(node == current);

    next_selection = parent;
    if (TRUE == NodeHasName(node, "screen"))
    {
        next_selection = (NULL != node->nextSibling) ? node->nextSibling : previous;
    }
    ForgetNodeValues(editor, node);
    if (NULL == previous)
    {
        parent->firstChild = node->nextSibling;
    }
    else
    {
        previous->nextSibling = node->nextSibling;
    }
    if (parent->lastChild == node)
    {
        parent->lastChild = previous;
    }
    node->parent         = NULL;
    node->nextSibling    = NULL;
    editor->selectedNode = next_selection;
    strcpy(editor->status, "Removed selection. Build XML to save.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Includes only screens, layouts, and windows in the hierarchy.
 */
static bool_t IsHierarchyNode(const Utl_Xml_Node_t* node)
{
    return (TRUE == NodeHasName(node, "screen")) ||
           (TRUE == NodeHasName(node, "layout")) ||
           (TRUE == NodeHasName(node, "window"));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows one element in the configuration hierarchy.
 */
static void DrawConfigurationNode(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Node_t*    child;
    const char_t*      identifier;
    ImGuiTreeNodeFlags flags;
    bool_t             is_open;
    bool_t             has_hierarchy_child;

    if (FALSE == IsHierarchyNode(node))
    {
        return;
    }

    identifier = FindAttributeValue(editor, node, "name");
    if (NULL == identifier)
    {
        identifier = FindAttributeValue(editor, node, "id");
    }
    flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
    if (editor->selectedNode == node)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    has_hierarchy_child = FALSE;
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        if (TRUE == IsHierarchyNode(child))
        {
            has_hierarchy_child = TRUE;
            break;
        }
    }
    if (FALSE == has_hierarchy_child)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    ImGui::PushID(node);
    if (NULL != identifier)
    {
        is_open = ImGui::TreeNodeEx("node", flags, "%.*s (%s)", (int)node->nameByteCount, (const char_t*)node->name, identifier);
    }
    else
    {
        is_open = ImGui::TreeNodeEx("node", flags, "%.*s", (int)node->nameByteCount, (const char_t*)node->name);
    }
    if (ImGui::IsItemClicked())
    {
        editor->selectedNode = node;
    }
    if ((TRUE == is_open) && (TRUE == has_hierarchy_child))
    {
        for (child = node->firstChild; NULL != child; child = child->nextSibling)
        {
            DrawConfigurationNode(editor, child);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares a counted attribute name with a literal.
 */
static bool_t AttributeHasName(const Utl_Xml_Attribute_t* attribute, const char_t* name)
{
    size_t name_byte_count;

    name_byte_count = strlen(name);
    return (attribute->nameByteCount == name_byte_count) && (0 == memcmp(attribute->name, name, name_byte_count));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Parses an unsigned integer for an inspector number field.
 */
static bool_t ParseUnsignedValue(const char_t* text, uint32_t* value)
{
    char_t*            end;
    unsigned long long parsed_value;

    if ((NULL == text) || ('\0' == text[0]) || ('-' == text[0]))
    {
        return FALSE;
    }
    errno        = 0;
    parsed_value = std::strtoull(text, &end, 10);
    if ((ERANGE == errno) || ('\0' != *end) || (UINT32_MAX < parsed_value))
    {
        return FALSE;
    }
    *value = (uint32_t)parsed_value;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses numeric fields and unit choices for known configuration attributes.
 */
static void DrawAttributeControl(const Utl_Xml_Node_t* ownerNode, Utl_Xml_Attribute_t* attribute, char_t* value)
{
    uint32_t integer_value;
    float    float_value;
    size_t   option_index;
    bool_t   is_selected;

    if ((TRUE == AttributeHasName(attribute, "id")) ||
        (TRUE == AttributeHasName(attribute, "x")) ||
        (TRUE == AttributeHasName(attribute, "y")) ||
        (((TRUE == AttributeHasName(attribute, "width")) ||
          (TRUE == AttributeHasName(attribute, "height"))) &&
         (FALSE == NodeHasName(ownerNode, "physicalDimensions"))))
    {
        if (FALSE == ParseUnsignedValue(value, &integer_value))
        {
            ImGui::TextUnformatted("Invalid number");
            ImGui::SameLine();
            if (ImGui::SmallButton("Set 0"))
            {
                strcpy(value, "0");
            }
        }
        else if (ImGui::InputScalar("##value", ImGuiDataType_U32, &integer_value))
        {
            snprintf(value, CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 1, "%u", integer_value);
        }
    }
    else if ((TRUE == AttributeHasName(attribute, "xScale")) ||
             (TRUE == AttributeHasName(attribute, "yScale")) ||
             ((TRUE == NodeHasName(ownerNode, "physicalDimensions")) &&
              ((TRUE == AttributeHasName(attribute, "width")) || (TRUE == AttributeHasName(attribute, "height")))))
    {
        if (FALSE == ParsePixelValue(value, &float_value))
        {
            ImGui::TextUnformatted("Invalid number");
            ImGui::SameLine();
            if (ImGui::SmallButton("Set 0"))
            {
                strcpy(value, "0");
            }
        }
        else if (ImGui::InputFloat("##value", &float_value, 0.0f, 0.0f, "%.6g"))
        {
            snprintf(value, CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 1, "%.9g", float_value);
        }
    }
    else if ((TRUE == AttributeHasName(attribute, "units")) ||
             (TRUE == AttributeHasName(attribute, "baseUnit")) ||
             (TRUE == AttributeHasName(attribute, "perUnit")))
    {
        if (ImGui::BeginCombo("##value", value))
        {
            for (option_index = 0; option_index < (sizeof(c_unitOptions) / sizeof(c_unitOptions[0])); ++option_index)
            {
                is_selected = (0 == strcmp(value, c_unitOptions[option_index]));
                if (ImGui::Selectable(c_unitOptions[option_index], TRUE == is_selected))
                {
                    strcpy(value, c_unitOptions[option_index]);
                }
                if (TRUE == is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    else
    {
        ImGui::InputText("##value", value, CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 1);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows a node's attributes as one two-column property group.
 */
static void DrawAttributeSection(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* heading)
{
    Utl_Xml_Attribute_t* attribute;
    uint32_t             value_index;

    if (NULL == node)
    {
        return;
    }
    ImGui::Separator();
    ImGui::TextUnformatted(heading);
    if (NULL == node->firstAttribute)
    {
        ImGui::TextDisabled("No properties");
        return;
    }
    ImGui::PushID(node);
    if (ImGui::BeginTable("Properties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 125.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted((const char_t*)attribute->name, (const char_t*)attribute->name + attribute->nameByteCount);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(attribute);
            ImGui::SetNextItemWidth(-1.0f);
            value_index = FindEditableValueIndex(editor, attribute);
            LOGASSERT(value_index < editor->attributeValueCount);
            DrawAttributeControl(node, attribute, editor->attributeValues[value_index].value);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Groups screen and window details under their owning selection.
 */
static void DrawSelectedNodeInspector(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* node;

    node = editor->selectedNode;
    if (NULL == node)
    {
        ImGui::TextUnformatted("Select a screen, layout, or window.");
        return;
    }
    if (TRUE == NodeHasName(node, "screen"))
    {
        DrawAttributeSection(editor, node, "Screen");
        DrawAttributeSection(editor, FindChild(node, "pixelSize"), "Pixel Size");
        DrawAttributeSection(editor, FindChild(node, "physicalDimensions"), "Physical Dimensions");
    }
    else if (TRUE == NodeHasName(node, "layout"))
    {
        DrawAttributeSection(editor, node, "Layout");
    }
    else if (TRUE == NodeHasName(node, "window"))
    {
        DrawAttributeSection(editor, node, "Window");
        DrawAttributeSection(editor, FindChild(node, "pixelArea"), "Pixel Area");
        DrawAttributeSection(editor, FindChild(node, "scalingFactor"), "Scaling Factor");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows a hierarchy above the selected element's inspector.
 */
static void DrawConfigurationEditor(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* root;
    ImVec2          available_size;
    float           hierarchy_height;

    ImGui::Begin(CONFIGURATION_WINDOW_NAME);
    ImGui::TextUnformatted("arinc661_configuration.xml");
    if (ImGui::Button("Reload XML"))
    {
        LoadConfiguration(editor);
    }
    ImGui::SameLine();
    if ((TRUE == editor->isLoaded) && ImGui::Button("Build XML"))
    {
        BuildConfiguration(editor);
    }
    ImGui::TextWrapped("%s", editor->status);
    if (TRUE == editor->isLoaded)
    {
        root = FindDocumentElement(editor);
        ImGui::TextUnformatted("Hierarchy");
        if (ImGui::SmallButton("Add Screen"))
        {
            AddScreen(editor);
        }
        if ((NULL != editor->selectedNode) && (TRUE == NodeHasName(editor->selectedNode, "screen")))
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Add Layout"))
            {
                AddLayout(editor, editor->selectedNode);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove Screen"))
            {
                RemoveSelectedNode(editor);
            }
        }
        else if ((NULL != editor->selectedNode) && (TRUE == NodeHasName(editor->selectedNode, "layout")))
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Add Window"))
            {
                AddWindow(editor, editor->selectedNode);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove Layout"))
            {
                RemoveSelectedNode(editor);
            }
        }
        else if ((NULL != editor->selectedNode) && (TRUE == NodeHasName(editor->selectedNode, "window")))
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Add Window"))
            {
                AddWindow(editor, editor->selectedNode->parent);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Remove Window"))
            {
                RemoveSelectedNode(editor);
            }
        }
        available_size   = ImGui::GetContentRegionAvail();
        hierarchy_height = (std::max)(150.0f, available_size.y * 0.36f);
        ImGui::BeginChild("Hierarchy", ImVec2(0.0f, hierarchy_height), true);
        if (NULL != root)
        {
            for (node = root->firstChild; NULL != node; node = node->nextSibling)
            {
                DrawConfigurationNode(editor, node);
            }
        }
        ImGui::EndChild();
        ImGui::TextUnformatted("Inspector");
        DrawSelectedNodeInspector(editor);
        if ((NULL != root) && ImGui::CollapsingHeader("Document settings"))
        {
            DrawAttributeSection(editor, root, "Configuration");
        }
    }
    ImGui::End();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares a counted XML node name with a literal.
 */
static bool_t NodeHasName(const Utl_Xml_Node_t* node, const char_t* name)
{
    size_t name_byte_count;

    name_byte_count = strlen(name);
    return (node->nameByteCount == name_byte_count) && (0 == memcmp(node->name, name, name_byte_count));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds a direct child with the requested element name.
 */
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name)
{
    Utl_Xml_Node_t* child;

    for (child = parent->firstChild; NULL != child; child = child->nextSibling)
    {
        if (TRUE == NodeHasName(child, name))
        {
            return child;
        }
    }
    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds an edited value by its XML attribute name.
 */
static const char_t* FindAttributeValue(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* attribute;
    uint32_t             value_index;
    size_t               name_byte_count;

    name_byte_count = strlen(name);
    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        if ((attribute->nameByteCount == name_byte_count) && (0 == memcmp(attribute->name, name, name_byte_count)))
        {
            value_index = FindEditableValueIndex(editor, attribute);
            LOGASSERT(value_index < editor->attributeValueCount);
            return editor->attributeValues[value_index].value;
        }
    }
    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a nonnegative finite pixel coordinate from an edited value.
 */
static bool_t ParsePixelValue(const char_t* text, float* value)
{
    char_t* end;
    float   parsed_value;

    if ((NULL == text) || ('\0' == text[0]))
    {
        return FALSE;
    }
    errno        = 0;
    parsed_value = std::strtof(text, &end);
    if ((ERANGE == errno) || ('\0' != *end) || !std::isfinite(parsed_value) || (0.0f > parsed_value))
    {
        return FALSE;
    }
    *value = parsed_value;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a window pixel area from the current editor values.
 */
static bool_t ReadPixelRectangle(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, float* x, float* y, float* width, float* height)
{
    if (FALSE == ParsePixelValue(FindAttributeValue(editor, node, "x"), x))
    {
        return FALSE;
    }
    if (FALSE == ParsePixelValue(FindAttributeValue(editor, node, "y"), y))
    {
        return FALSE;
    }
    if (FALSE == ParsePixelValue(FindAttributeValue(editor, node, "width"), width))
    {
        return FALSE;
    }
    return ParsePixelValue(FindAttributeValue(editor, node, "height"), height);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads positive screen dimensions from the current editor values.
 */
static bool_t ReadScreenDimensions(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen, float* width, float* height)
{
    Utl_Xml_Node_t* pixel_size;

    pixel_size = FindChild(screen, "pixelSize");
    if (NULL == pixel_size)
    {
        return FALSE;
    }
    if (FALSE == ParsePixelValue(FindAttributeValue(editor, pixel_size, "width"), width))
    {
        return FALSE;
    }
    if (FALSE == ParsePixelValue(FindAttributeValue(editor, pixel_size, "height"), height))
    {
        return FALSE;
    }
    return (0.0f < *width) && (0.0f < *height);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws configured windows inside a screen rectangle.
 */
static void DrawScreenWindows(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen, ImDrawList* drawList, const ImVec2* origin, float scale)
{
    Utl_Xml_Node_t*    layout;
    Utl_Xml_Node_t*    window;
    Utl_Xml_Node_t*    pixel_area;
    const char_t*      window_name;
    ImVec2             area_min;
    ImVec2             area_max;
    float              x;
    float              y;
    float              width;
    float              height;

    for (layout = screen->firstChild; NULL != layout; layout = layout->nextSibling)
    {
        if (FALSE == NodeHasName(layout, "layout"))
        {
            continue;
        }
        for (window = layout->firstChild; NULL != window; window = window->nextSibling)
        {
            if (FALSE == NodeHasName(window, "window"))
            {
                continue;
            }
            pixel_area = FindChild(window, "pixelArea");
            if ((NULL == pixel_area) || (FALSE == ReadPixelRectangle(editor, pixel_area, &x, &y, &width, &height)))
            {
                continue;
            }
            area_min = ImVec2(origin->x + x * scale, origin->y + y * scale);
            area_max = ImVec2(area_min.x + width * scale, area_min.y + height * scale);
            drawList->AddRectFilled(area_min, area_max, IM_COL32(42, 154, 188, 80));
            drawList->AddRect(area_min, area_max, IM_COL32(97, 221, 245, 255), 0.0f, 0, 2.0f);
            window_name = FindAttributeValue(editor, window, "name");
            if ((NULL != window_name) && ('\0' != window_name[0]))
            {
                drawList->AddText(ImVec2(area_min.x + 6.0f, area_min.y + 6.0f), IM_COL32(240, 248, 255, 255), window_name);
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows screen sizes and window pixel areas from the current field values.
 */
static void DrawVisualizer(const ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* root;
    Utl_Xml_Node_t* screen;
    const char_t*   screen_id;
    ImDrawList*     draw_list;
    ImVec2          canvas_min;
    ImVec2          canvas_size;
    ImVec2          screen_min;
    ImVec2          screen_max;
    char_t          screen_label[CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 16];
    float           total_width;
    float           maximum_height;
    float           available_width;
    float           available_height;
    float           screen_width;
    float           screen_height;
    float           scale;
    float           x;
    float           y;
    uint32_t        screen_count;

    ImGui::Begin(VISUALIZER_WINDOW_NAME);
    if (FALSE == editor->isLoaded)
    {
        ImGui::TextUnformatted(editor->status);
        ImGui::End();
        return;
    }

    root = editor->document.firstNode;
    while ((NULL != root) && (UTL_XML_NODE_ELEMENT != root->type))
    {
        root = root->nextSibling;
    }
    if (NULL == root)
    {
        ImGui::TextUnformatted("No document element.");
        ImGui::End();
        return;
    }

    total_width    = 0.0f;
    maximum_height = 0.0f;
    screen_count   = 0;
    for (screen = root->firstChild; NULL != screen; screen = screen->nextSibling)
    {
        if ((FALSE == NodeHasName(screen, "screen")) ||
            (FALSE == ReadScreenDimensions(editor, screen, &screen_width, &screen_height)))
        {
            continue;
        }
        ++screen_count;
        total_width += screen_width;
        maximum_height = (std::max)(maximum_height, screen_height);
    }
    if (0 == screen_count)
    {
        ImGui::TextUnformatted("No screens with valid pixel dimensions.");
        ImGui::End();
        return;
    }

    canvas_min       = ImGui::GetCursorScreenPos();
    canvas_size      = ImGui::GetContentRegionAvail();
    available_width  = canvas_size.x - 40.0f - 20.0f * (float)(screen_count - 1);
    available_height = canvas_size.y - 60.0f;
    if ((0.0f >= available_width) || (0.0f >= available_height))
    {
        ImGui::TextUnformatted("Preview area is too small.");
        ImGui::End();
        return;
    }

    ImGui::InvisibleButton("Screen canvas", canvas_size);
    draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(canvas_min, ImVec2(canvas_min.x + canvas_size.x, canvas_min.y + canvas_size.y), true);
    draw_list->AddRectFilled(canvas_min, ImVec2(canvas_min.x + canvas_size.x, canvas_min.y + canvas_size.y), IM_COL32(22, 27, 34, 255));

    scale = (std::min)(available_width / total_width, available_height / maximum_height);
    x     = canvas_min.x + 20.0f;
    y     = canvas_min.y + 35.0f + (available_height - maximum_height * scale) * 0.5f;
    for (screen = root->firstChild; NULL != screen; screen = screen->nextSibling)
    {
        if ((FALSE == NodeHasName(screen, "screen")) ||
            (FALSE == ReadScreenDimensions(editor, screen, &screen_width, &screen_height)))
        {
            continue;
        }
        screen_min = ImVec2(x, y);
        screen_max = ImVec2(x + screen_width * scale, y + screen_height * scale);
        draw_list->AddRectFilled(screen_min, screen_max, IM_COL32(42, 50, 64, 255));
        draw_list->AddRect(screen_min, screen_max, IM_COL32(220, 230, 245, 255), 0.0f, 0, 2.0f);
        screen_id = FindAttributeValue(editor, screen, "id");
        if (NULL != screen_id)
        {
            snprintf(screen_label, sizeof(screen_label), "Screen %s", screen_id);
            draw_list->AddText(ImVec2(x, y - 22.0f), IM_COL32(240, 248, 255, 255), screen_label);
        }
        draw_list->PushClipRect(screen_min, screen_max, true);
        DrawScreenWindows(editor, screen, draw_list, &screen_min, scale);
        draw_list->PopClipRect();
        x = screen_max.x + 20.0f;
    }
    draw_list->PopClipRect();
    ImGui::End();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Places the configuration on the left and the visualizer in the center.
 */
static void BuildDockLayout(ImGuiID dockspaceId)
{
    const ImGuiViewport* viewport;
    ImGuiID              configuration_node_id;
    ImGuiID              visualizer_node_id;

    viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.30f, &configuration_node_id, &visualizer_node_id);
    ImGui::DockBuilderDockWindow(CONFIGURATION_WINDOW_NAME, configuration_node_id);
    ImGui::DockBuilderDockWindow(VISUALIZER_WINDOW_NAME, visualizer_node_id);
    ImGui::DockBuilderFinish(dockspaceId);
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Loads XML and prepares the default dock layout.
 */
void InitializeConfiguration(ConfigurationEditor_t* editor)
{
    editor->isLayoutInitialized = FALSE;
    LoadConfiguration(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the configuration editor and visualizer in their docked windows.
 */
void DrawConfiguration(ConfigurationEditor_t* editor)
{
    ImGuiID dockspace_id;

    dockspace_id = ImHashStr(DOCKSPACE_NAME);
    if (FALSE == editor->isLayoutInitialized)
    {
        BuildDockLayout(dockspace_id);
        editor->isLayoutInitialized = TRUE;
    }
    ImGui::DockSpaceOverViewport(dockspace_id);
    DrawConfigurationEditor(editor);
    DrawVisualizer(editor);
}
