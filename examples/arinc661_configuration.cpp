/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_configuration.h"
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
#include "utl/utl_defines.h"
}

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * \brief Advances within one XML subtree without recursion.
 *
 * \param[in] rootNode      Root of the subtree
 * \param[in] currentNode   Node just visited
 *
 * \return Next node in document order, or NULL at the end of the subtree.
 */
static Utl_Xml_Node_t* NextSubtreeNode(Utl_Xml_Node_t* rootNode, Utl_Xml_Node_t* currentNode);

/*************************************************************************************************/
/**
 * \brief Copies parsed attribute values into fixed storage and updates their pointers.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     node     XML node
 *
 * \retval TRUE    All attribute values fit the editor storage.
 * \retval FALSE   The editor storage limit was reached.
 */
static bool_t CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

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
 * \brief Updates and validates attribute lengths in a linked XML subtree.
 *
 * \param[in,out] editor   Editor state receiving an error message
 * \param[in,out] node     XML node to prepare
 *
 * \retval TRUE    All linked attribute values are valid.
 * \retval FALSE   An attribute value is invalid.
 */
static bool_t UpdateNodeAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

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
 * \brief Initializes the next fixed XML node slot for an editor addition.
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
 * \param[in]     node          XML node
 * \param[in]     elementName   Element name to match
 * \param[in,out] maximumId     Largest matching ID found
 * \param[in,out] foundId       Whether a matching ID was found
 *
 * \retval None
 */
static void FindMaximumId(Utl_Xml_Node_t* node, const char_t* elementName, uint32_t* maximumId, bool_t* foundId);

/*************************************************************************************************/
/**
 * \brief Chooses the next ID within the requested subtree.
 *
 * \param[in]     root          Root of the subtree
 * \param[in]     elementName   Element name to match
 * \param[out]    nextId        Generated next ID
 *
 * \retval TRUE    An ID is available.
 * \retval FALSE   The ID range is exhausted.
 */
static bool_t NextElementId(Utl_Xml_Node_t* root, const char_t* elementName, uint32_t* nextId);

/*************************************************************************************************/
/**
 * \brief Finds the configuration document element.
 *
 * \param[in]     editor   Editor state
 *
 * \return Document element, or NULL when absent.
 */
static Utl_Xml_Node_t* FindDocumentElement(ConfigurationEditor_t* editor);

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
 * \retval TRUE    The node is a screen, layout, or window.
 * \retval FALSE   The node is not in the hierarchy.
 */
static bool_t IsHierarchyNode(const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Shows one element in the configuration hierarchy.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     node     XML node
 *
 * \retval TRUE    Child nodes are visible and the caller must call ImGui::TreePop.
 * \retval FALSE   Child nodes are hidden or absent.
 */
static bool_t DrawConfigurationNode(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Shows screens, layouts, and windows without recursion.
 *
 * \param[in,out] editor   Editor state
 * \param[in]     root     Configuration element
 *
 * \retval None
 */
static void DrawConfigurationHierarchy(ConfigurationEditor_t* editor, Utl_Xml_Node_t* root);

/*************************************************************************************************/
/**
 * \brief Compares a counted attribute name with a literal.
 *
 * \param[in]     attribute   XML attribute
 * \param[in]     name        Null-terminated XML name
 *
 * \retval TRUE    The attribute has the requested name.
 * \retval FALSE   The attribute has another name.
 */
static bool_t AttributeHasName(const Utl_Xml_Attribute_t* attribute, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Chooses the inspector control for an XML attribute.
 *
 * \param[in] ownerNode   Element that owns the attribute
 * \param[in] attribute   XML attribute
 *
 * \return Control kind for the attribute.
 */
static ConfigurationAttributeKind_t ClassifyAttribute(const Utl_Xml_Node_t* ownerNode, const Utl_Xml_Attribute_t* attribute);

/*************************************************************************************************/
/**
 * \brief Parses the initial value of a numeric inspector control.
 *
 * \param[in,out] valueState   Editable value to initialize
 * \param[in]     ownerNode    Element that owns the attribute
 * \param[in]     attribute    XML attribute
 *
 * \retval None
 */
static void InitializeAttributeValue(ConfigurationAttributeValue_t* valueState, const Utl_Xml_Node_t* ownerNode, const Utl_Xml_Attribute_t* attribute);

/*************************************************************************************************/
/**
 * \brief Parses an unsigned integer for an inspector number field.
 *
 * \param[in]     text    Editable numeric text
 * \param[out]    value   Attribute value
 *
 * \retval TRUE    The text is a valid unsigned value.
 * \retval FALSE   The text is not a valid unsigned value.
 */
static bool_t ParseUnsignedValue(const char_t* text, uint32_t* value);

/*************************************************************************************************/
/**
 * \brief Uses numeric fields and unit choices for known configuration attributes.
 *
 * \param[in,out] editor      Editor state
 * \param[in]     attribute   XML attribute
 *
 * \retval None
 */
static void DrawAttributeControl(ConfigurationEditor_t* editor, Utl_Xml_Attribute_t* attribute);

/*************************************************************************************************/
/**
 * \brief Shows a node's attributes as one two-column property group.
 *
 * \param[in,out] editor    Editor state
 * \param[in,out] node      XML node
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
 * \retval TRUE    The node has the requested name.
 * \retval FALSE   The node has another name.
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
 * \param[in]     node     XML node
 * \param[in]     name     Null-terminated XML name
 *
 * \return Editable value, or NULL when the attribute is absent.
 */
static const char_t* FindAttributeValue(Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a nonnegative finite pixel coordinate from an edited value.
 *
 * \param[in]     text    Editable numeric text
 * \param[out]    value   Attribute value
 *
 * \retval TRUE    The text is a finite nonnegative value.
 * \retval FALSE   The text is not a finite nonnegative value.
 */
static bool_t ParsePixelValue(const char_t* text, float32_t* value);

/*************************************************************************************************/
/**
 * \brief Reads a window pixel area from the current editor values.
 *
 * \param[in]     node     XML node
 * \param[out]    x        Pixel x coordinate
 * \param[out]    y        Pixel y coordinate
 * \param[out]    width    Pixel width
 * \param[out]    height   Pixel height
 *
 * \retval TRUE    All four coordinates are valid.
 * \retval FALSE   At least one coordinate is invalid.
 */
static bool_t ReadPixelRectangle(Utl_Xml_Node_t* node, float32_t* x, float32_t* y, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Reads positive screen dimensions from the current editor values.
 *
 * \param[in]  screen   Screen element
 * \param[out] width    Pixel width
 * \param[out] height   Pixel height
 *
 * \retval TRUE    Both dimensions are positive.
 * \retval FALSE   A dimension is absent or invalid.
 */
static bool_t ReadScreenDimensions(Utl_Xml_Node_t* screen, float32_t* width, float32_t* height);

/*************************************************************************************************/
/**
 * \brief Collects cached window geometry for one screen.
 *
 * \param[in,out] editor       Editor state
 * \param[in]     screenNode   Screen element
 *
 * \retval None
 */
static void CollectPreviewWindows(ConfigurationEditor_t* editor, Utl_Xml_Node_t* screenNode);

/*************************************************************************************************/
/**
 * \brief Updates cached screen and window geometry from the XML tree.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void UpdatePreview(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws measured screens and their windows in the visualizer canvas.
 *
 * \param[in] editor   Editor state with prepared geometry
 *
 * \retval None
 */
static void DrawScreenCanvas(const ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Shows prepared screen sizes and window pixel areas.
 *
 * \param[in]     editor   Editor state
 *
 * \retval None
 */
static void DrawVisualizer(ConfigurationEditor_t* editor);

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
 *  Follows child, sibling, and parent links in bounded document order.
 */
static Utl_Xml_Node_t* NextSubtreeNode(Utl_Xml_Node_t* rootNode, Utl_Xml_Node_t* currentNode)
{
    Utl_Xml_Node_t* next_node;
    Utl_Xml_Node_t* ancestor;
    uint32_t        ancestor_index;

    next_node = currentNode->firstChild;
    ancestor  = currentNode;

    if (NULL == next_node)
    {
        for (ancestor_index = 0; ancestor_index < UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
        {
            if (rootNode == ancestor)
            {
                break;
            }

            next_node = ancestor->nextSibling;
            if (NULL != next_node)
            {
                break;
            }

            ancestor = ancestor->parent;
            LOGASSERT(NULL != ancestor);
        }
        LOGASSERT((NULL != next_node) || (rootNode == ancestor));
    }

    return next_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies parsed attribute values into editable storage.
 */
static bool_t CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t*           attribute;
    Utl_Xml_Node_t*                current_node;
    ConfigurationAttributeValue_t* value_state;
    uint32_t                       attribute_index;
    uint32_t                       node_index;
    bool_t                         is_storage_full;
    bool_t                         is_value_too_long;
    bool_t                         is_over_limit;
    bool_t                         is_valid;

    current_node = node;
    is_valid     = TRUE;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        attribute = current_node->firstAttribute;
        for (attribute_index = 0; attribute_index < current_node->attributeCount; ++attribute_index)
        {
            LOGASSERT(NULL != attribute);
            is_storage_full   = (CONFIGURATION_MAX_EDITABLE_ATTRIBUTES == editor->attributeValueCount);
            is_value_too_long = (CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES < attribute->valueByteCount);
            is_over_limit     = is_storage_full || is_value_too_long;
            if (is_over_limit)
            {
                strcpy(editor->status, "The XML file exceeds the editor's attribute limits.");
                is_valid = FALSE;
                break;
            }
            value_state = &editor->attributeValues[editor->attributeValueCount];
            memcpy(value_state->text, attribute->value, attribute->valueByteCount);
            value_state->text[attribute->valueByteCount] = '\0';
            attribute->value = (const byte_t*)value_state->text;
            InitializeAttributeValue(value_state, current_node, attribute);
            editor->attributeValueCount += 1;
            attribute = attribute->next;
        }
        if (FALSE == is_valid)
        {
            break;
        }
        LOGASSERT(NULL == attribute);

        current_node = NextSubtreeNode(node, current_node);
    }

    LOGASSERT((FALSE == is_valid) || (NULL == current_node));

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the configuration through Utl_Xml_ReadDocument.
 */
static void LoadConfiguration(ConfigurationEditor_t* editor)
{
    FILE*           xml_file;
    uint64_t        document_byte_count;
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* first_screen;
    uint32_t        node_index;

    editor->isLoaded            = FALSE;
    editor->selectedNode        = NULL;
    editor->attributeValueCount = 0;
    editor->addedNodeCount      = 0;
    editor->addedAttributeCount = 0;
    editor->previewScreenCount  = 0;
    editor->previewWindowCount  = 0;
    editor->isPreviewDirty      = FALSE;

    xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "rb");
    if (NULL == xml_file)
    {
        strcpy(editor->status, "Could not open arinc661_configuration.xml.");
        return;
    }
    document_byte_count = fread(editor->inputBuffer, 1, CONFIGURATION_MAX_DOCUMENT_BYTES + 1, xml_file);
    fclose(xml_file);
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
    Utl_Xml_InitializeDocument(&editor->document);
    Utl_Xml_ReadDocument(
        &editor->document,
        editor->inputBuffer,
        document_byte_count,
        editor->nodes,
        UTL_XML_DOCUMENT_MAX_NODES,
        editor->attributes,
        CONFIGURATION_MAX_PARSED_ATTRIBUTES);

    node = editor->document.firstNode;
    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == node)
        {
            break;
        }
        if (FALSE == CollectAttributes(editor, node))
        {
            return;
        }
        if (NULL == editor->selectedNode)
        {
            if (UTL_XML_NODE_ELEMENT == node->type)
            {
                editor->selectedNode = node;
            }
        }
        node = node->nextSibling;
    }
    LOGASSERT(NULL == node);
    if (NULL != editor->selectedNode)
    {
        first_screen = FindChild(editor->selectedNode, "screen");
        if (NULL != first_screen)
        {
            editor->selectedNode = first_screen;
        }
    }
    editor->isLoaded = TRUE;
    editor->isPreviewDirty = TRUE;
    strcpy(editor->status, "Loaded arinc661_configuration.xml.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Updates and validates attribute lengths in a linked XML subtree.
 */
static bool_t UpdateNodeAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t* attribute;
    Utl_Xml_Node_t*      current_node;
    uint32_t             value_byte_count;
    uint32_t             attribute_index;
    uint32_t             node_index;
    bool_t               is_valid;

    current_node = node;
    is_valid     = TRUE;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        attribute = current_node->firstAttribute;
        for (attribute_index = 0; attribute_index < current_node->attributeCount; ++attribute_index)
        {
            LOGASSERT(NULL != attribute);
            value_byte_count = (uint32_t)strlen((const char_t*)attribute->value);
            is_valid = Utl_Xml_IsAttributeValue(attribute->value, value_byte_count);
            if (FALSE == is_valid)
            {
                snprintf(
                    editor->status,
                    sizeof(editor->status),
                    "Invalid XML attribute value: %.*s",
                    (int)attribute->nameByteCount,
                    (const char_t*)attribute->name);
                break;
            }
            attribute->valueByteCount = value_byte_count;
            attribute = attribute->next;
        }
        if (FALSE == is_valid)
        {
            break;
        }
        LOGASSERT(NULL == attribute);

        current_node = NextSubtreeNode(node, current_node);
    }

    LOGASSERT((FALSE == is_valid) || (NULL == current_node));

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates edited values and writes the document with Utl_Xml_WriteDocument.
 */
static void BuildConfiguration(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* node;
    FILE*           xml_file;
    uint64_t        output_byte_count;
    uint64_t        bytes_written;
    uint32_t        node_index;
    bool_t          has_write_error;

    node = editor->document.firstNode;
    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == node)
        {
            break;
        }
        if (FALSE == UpdateNodeAttributes(editor, node))
        {
            return;
        }
        node = node->nextSibling;
    }
    LOGASSERT(NULL == node);
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
    bytes_written = fwrite(editor->outputBuffer, 1, output_byte_count, xml_file);
    fclose(xml_file);

    has_write_error = (output_byte_count != bytes_written);
    if (has_write_error)
    {
        strcpy(editor->status, "Could not finish writing arinc661_configuration.xml.");
        return;
    }
    strcpy(editor->status, "Built arinc661_configuration.xml.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Initializes the next fixed node slot for an editor addition.
 */
static Utl_Xml_Node_t* CreateElement(ConfigurationEditor_t* editor, const char_t* name)
{
    Utl_Xml_Node_t* node;

    LOGASSERT(editor->addedNodeCount < CONFIGURATION_MAX_ADDED_NODES);
    node = &editor->addedNodes[editor->addedNodeCount];
    Utl_Xml_InitializeNode(node, UTL_XML_NODE_ELEMENT, (const byte_t*)name, (uint32_t)strlen(name));
    editor->addedNodeCount += 1;
    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds a stable XML attribute and an editable value.
 */
static void CreateAttribute(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t*           attribute;
    ConfigurationAttributeValue_t* value_state;

    LOGASSERT(editor->addedAttributeCount < CONFIGURATION_MAX_ADDED_ATTRIBUTES);
    LOGASSERT(editor->attributeValueCount < CONFIGURATION_MAX_EDITABLE_ATTRIBUTES);
    LOGASSERT(strlen(value) <= CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES);
    attribute   = &editor->addedAttributes[editor->addedAttributeCount];
    value_state = &editor->attributeValues[editor->attributeValueCount];
    strcpy(value_state->text, value);
    Utl_Xml_InitializeAttribute(
        attribute,
        (const byte_t*)name,
        (uint32_t)strlen(name),
        (const byte_t*)value_state->text,
        (uint32_t)strlen(value_state->text));
    InitializeAttributeValue(value_state, node, attribute);
    Utl_Xml_AppendAttribute(node, attribute);
    editor->addedAttributeCount += 1;
    editor->attributeValueCount += 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds the largest numeric ID for one element type in a subtree.
 */
static void FindMaximumId(Utl_Xml_Node_t* node, const char_t* elementName, uint32_t* maximumId, bool_t* foundId)
{
    const char_t*   id_text;
    Utl_Xml_Node_t* current_node;
    uint32_t        id;
    uint32_t        node_index;
    bool_t          is_valid_id;
    bool_t          is_first_id;
    bool_t          is_larger_id;

    current_node = node;
    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }
        if (NodeHasName(current_node, elementName))
        {
            id_text = FindAttributeValue(current_node, "id");
            if (NULL != id_text)
            {
                is_valid_id = ParseUnsignedValue(id_text, &id);
                if (is_valid_id)
                {
                    is_first_id  = (FALSE == (*foundId));
                    is_larger_id = ((*maximumId) < id);
                    if (is_first_id || is_larger_id)
                    {
                        (*maximumId) = id;
                    }
                    (*foundId) = TRUE;
                }
            }
        }
        current_node = NextSubtreeNode(node, current_node);
    }
    LOGASSERT(NULL == current_node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Chooses the next ID within the requested subtree.
 */
static bool_t NextElementId(Utl_Xml_Node_t* root, const char_t* elementName, uint32_t* nextId)
{
    uint32_t maximum_id;
    bool_t   found_id;
    bool_t   has_exhausted_ids;

    maximum_id = 0;
    found_id   = FALSE;
    FindMaximumId(root, elementName, &maximum_id, &found_id);

    has_exhausted_ids = found_id && (UINT32_MAX == maximum_id);
    if (has_exhausted_ids)
    {
        return FALSE;
    }
    if (found_id)
    {
        (*nextId) = maximum_id + 1;
    }
    else
    {
        (*nextId) = 0;
    }
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
    Utl_Xml_Node_t* found_node;
    uint32_t        node_index;

    found_node = NULL;
    node       = editor->document.firstNode;
    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == node)
        {
            break;
        }
        if (UTL_XML_NODE_ELEMENT == node->type)
        {
            found_node = node;
            break;
        }
        node = node->nextSibling;
    }
    if (NULL == found_node)
    {
        LOGASSERT(NULL == node);
    }
    return found_node;
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
    Utl_Xml_Node_t* pixel_size_node;
    Utl_Xml_Node_t* physical_dimensions_node;
    uint32_t        next_id;
    char_t          id_text[16];
    bool_t          has_next_id;

    root = FindDocumentElement(editor);
    if (NULL == root)
    {
        strcpy(editor->status, "Could not find the configuration element.");
        return;
    }
    has_next_id = NextElementId(root, "screen", &next_id);
    if (FALSE == has_next_id)
    {
        strcpy(editor->status, "Could not allocate a screen ID.");
        return;
    }
    snprintf(id_text, sizeof(id_text), "%u", next_id);
    screen = CreateElement(editor, "screen");
    CreateAttribute(editor, screen, "id", id_text);

    pixel_size_node = CreateElement(editor, "pixelSize");
    CreateAttribute(editor, pixel_size_node, "width", "640");
    CreateAttribute(editor, pixel_size_node, "height", "480");
    CreateAttribute(editor, pixel_size_node, "units", "pixel");
    Utl_Xml_AppendChild(screen, pixel_size_node);

    physical_dimensions_node = CreateElement(editor, "physicalDimensions");
    CreateAttribute(editor, physical_dimensions_node, "width", "5");
    CreateAttribute(editor, physical_dimensions_node, "height", "3.75");
    CreateAttribute(editor, physical_dimensions_node, "units", "inch");
    Utl_Xml_AppendChild(screen, physical_dimensions_node);

    Utl_Xml_AppendChild(root, screen);
    editor->selectedNode = screen;
    editor->isPreviewDirty = TRUE;
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

    if (FALSE == NextElementId(screen, "layout", &next_id))
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
    Utl_Xml_Node_t* root;
    Utl_Xml_Node_t* screen;
    Utl_Xml_Node_t* pixel_size_node;
    Utl_Xml_Node_t* window;
    Utl_Xml_Node_t* pixel_area_node;
    Utl_Xml_Node_t* scaling_factor_node;
    const char_t*   edited_width;
    const char_t*   edited_height;
    const char_t*   width_text;
    const char_t*   height_text;
    uint32_t        next_id;
    uint32_t        width;
    uint32_t        height;
    char_t          id_text[16];
    char_t          name_text[32];
    bool_t          is_width_valid;
    bool_t          is_height_valid;
    bool_t          has_next_id;

    root = FindDocumentElement(editor);
    if (NULL == root)
    {
        strcpy(editor->status, "Could not find the configuration element.");
        return;
    }
    has_next_id = NextElementId(root, "window", &next_id);
    if (FALSE == has_next_id)
    {
        strcpy(editor->status, "Could not allocate a window ID.");
        return;
    }
    screen          = layout->parent;
    pixel_size_node = FindChild(screen, "pixelSize");
    width_text      = "640";
    height_text     = "480";
    if (NULL != pixel_size_node)
    {
        edited_width  = FindAttributeValue(pixel_size_node, "width");
        edited_height = FindAttributeValue(pixel_size_node, "height");
        if (NULL != edited_width)
        {
            is_width_valid = ParseUnsignedValue(edited_width, &width);
            if (is_width_valid)
            {
                if (0 < width)
                {
                    width_text = edited_width;
                }
            }
        }
        if (NULL != edited_height)
        {
            is_height_valid = ParseUnsignedValue(edited_height, &height);
            if (is_height_valid)
            {
                if (0 < height)
                {
                    height_text = edited_height;
                }
            }
        }
    }

    snprintf(id_text, sizeof(id_text), "%u", next_id);
    snprintf(name_text, sizeof(name_text), "window%s", id_text);
    window = CreateElement(editor, "window");
    CreateAttribute(editor, window, "id", id_text);
    CreateAttribute(editor, window, "name", name_text);

    pixel_area_node = CreateElement(editor, "pixelArea");
    CreateAttribute(editor, pixel_area_node, "x", "0");
    CreateAttribute(editor, pixel_area_node, "y", "0");
    CreateAttribute(editor, pixel_area_node, "width", width_text);
    CreateAttribute(editor, pixel_area_node, "height", height_text);
    Utl_Xml_AppendChild(window, pixel_area_node);

    scaling_factor_node = CreateElement(editor, "scalingFactor");
    CreateAttribute(editor, scaling_factor_node, "xScale", "20.0");
    CreateAttribute(editor, scaling_factor_node, "yScale", "20.0");
    CreateAttribute(editor, scaling_factor_node, "baseUnit", "screen");
    CreateAttribute(editor, scaling_factor_node, "perUnit", "pixel");
    Utl_Xml_AppendChild(window, scaling_factor_node);

    Utl_Xml_AppendChild(layout, window);
    editor->selectedNode = window;
    editor->isPreviewDirty = TRUE;
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
    uint32_t        node_index;
    bool_t          is_hierarchy_node;

    node = editor->selectedNode;
    if (NULL == node)
    {
        return;
    }
    is_hierarchy_node = IsHierarchyNode(node);
    if (FALSE == is_hierarchy_node)
    {
        return;
    }
    parent = node->parent;
    LOGASSERT(NULL != parent);
    previous = NULL;
    current  = parent->firstChild;
    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current)
        {
            break;
        }
        if (node == current)
        {
            break;
        }
        previous = current;
        current  = current->nextSibling;
    }
    LOGASSERT(node == current);

    next_selection = parent;
    if (NodeHasName(node, "screen"))
    {
        if (NULL != node->nextSibling)
        {
            next_selection = node->nextSibling;
        }
        else
        {
            next_selection = previous;
        }
    }
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
    editor->isPreviewDirty = TRUE;
    strcpy(editor->status, "Removed selection. Build XML to save.");
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Includes only screens, layouts, and windows in the hierarchy.
 */
static bool_t IsHierarchyNode(const Utl_Xml_Node_t* node)
{
    bool_t is_screen;
    bool_t is_layout;
    bool_t is_window;

    is_screen = NodeHasName(node, "screen");
    is_layout = NodeHasName(node, "layout");
    is_window = NodeHasName(node, "window");

    return is_screen || is_layout || is_window;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows one element in the configuration hierarchy.
 */
static bool_t DrawConfigurationNode(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Node_t*    child;
    const char_t*      identifier;
    ImGuiTreeNodeFlags flags;
    bool_t             is_open;
    bool_t             has_hierarchy_child;
    bool_t             should_draw_children;
    uint32_t           child_index;

    LOGASSERT(IsHierarchyNode(node));

    identifier = FindAttributeValue(node, "name");
    if (NULL == identifier)
    {
        identifier = FindAttributeValue(node, "id");
    }
    flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
    if (editor->selectedNode == node)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    has_hierarchy_child = FALSE;
    child = node->firstChild;
    for (child_index = 0; child_index < UTL_XML_DOCUMENT_MAX_NODES; ++child_index)
    {
        if (NULL == child)
        {
            break;
        }
        if (IsHierarchyNode(child))
        {
            has_hierarchy_child = TRUE;
            break;
        }
        child = child->nextSibling;
    }
    if (FALSE == has_hierarchy_child)
    {
        LOGASSERT(NULL == child);
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
    should_draw_children = is_open && has_hierarchy_child;
    ImGui::PopID();

    return should_draw_children;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The ARINC 661 hierarchy has three visible levels: screen, layout, and window.
 */
static void DrawConfigurationHierarchy(ConfigurationEditor_t* editor, Utl_Xml_Node_t* root)
{
    Utl_Xml_Node_t* screen_node;
    Utl_Xml_Node_t* layout_node;
    Utl_Xml_Node_t* window_node;
    uint32_t        screen_index;
    uint32_t        layout_index;
    uint32_t        window_index;
    bool_t          is_screen_open;
    bool_t          is_layout_open;
    bool_t          is_window_open;

    screen_node = root->firstChild;
    for (screen_index = 0; screen_index < UTL_XML_DOCUMENT_MAX_NODES; ++screen_index)
    {
        if (NULL == screen_node)
        {
            break;
        }
        if (NodeHasName(screen_node, "screen"))
        {
            is_screen_open = DrawConfigurationNode(editor, screen_node);
            if (is_screen_open)
            {
                layout_node = screen_node->firstChild;
                for (layout_index = 0; layout_index < UTL_XML_DOCUMENT_MAX_NODES; ++layout_index)
                {
                    if (NULL == layout_node)
                    {
                        break;
                    }
                    if (NodeHasName(layout_node, "layout"))
                    {
                        is_layout_open = DrawConfigurationNode(editor, layout_node);
                        if (is_layout_open)
                        {
                            window_node = layout_node->firstChild;
                            for (window_index = 0; window_index < UTL_XML_DOCUMENT_MAX_NODES; ++window_index)
                            {
                                if (NULL == window_node)
                                {
                                    break;
                                }
                                if (NodeHasName(window_node, "window"))
                                {
                                    is_window_open = DrawConfigurationNode(editor, window_node);
                                    if (is_window_open)
                                    {
                                        ImGui::TreePop();
                                    }
                                }
                                window_node = window_node->nextSibling;
                            }
                            LOGASSERT(NULL == window_node);
                            ImGui::TreePop();
                        }
                    }
                    layout_node = layout_node->nextSibling;
                }
                LOGASSERT(NULL == layout_node);
                ImGui::TreePop();
            }
        }
        screen_node = screen_node->nextSibling;
    }
    LOGASSERT(NULL == screen_node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares a counted attribute name with a literal.
 */
static bool_t AttributeHasName(const Utl_Xml_Attribute_t* attribute, const char_t* name)
{
    uint32_t name_byte_count;
    bool_t   has_same_name_byte_count;
    bool_t   has_same_name;

    name_byte_count = (uint32_t)strlen(name);
    has_same_name_byte_count = (attribute->nameByteCount == name_byte_count);
    has_same_name            = FALSE;

    if (has_same_name_byte_count)
    {
        has_same_name = (0 == memcmp(attribute->name, name, name_byte_count));
    }

    return has_same_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses numeric controls for geometry and IDs, and choices for units.
 */
static ConfigurationAttributeKind_t ClassifyAttribute(const Utl_Xml_Node_t* ownerNode, const Utl_Xml_Attribute_t* attribute)
{
    ConfigurationAttributeKind_t kind;
    bool_t                       is_width;
    bool_t                       is_height;
    bool_t                       is_id;
    bool_t                       is_x;
    bool_t                       is_y;
    bool_t                       is_x_scale;
    bool_t                       is_y_scale;
    bool_t                       is_units;
    bool_t                       is_base_unit;
    bool_t                       is_per_unit;
    bool_t                       is_width_or_height;
    bool_t                       is_physical_dimensions;
    bool_t                       is_pixel_dimension;
    bool_t                       is_physical_dimension;
    bool_t                       is_integer;
    bool_t                       is_float;
    bool_t                       is_unit;

    is_width               = AttributeHasName(attribute, "width");
    is_height              = AttributeHasName(attribute, "height");
    is_id                  = AttributeHasName(attribute, "id");
    is_x                   = AttributeHasName(attribute, "x");
    is_y                   = AttributeHasName(attribute, "y");
    is_x_scale             = AttributeHasName(attribute, "xScale");
    is_y_scale             = AttributeHasName(attribute, "yScale");
    is_units               = AttributeHasName(attribute, "units");
    is_base_unit           = AttributeHasName(attribute, "baseUnit");
    is_per_unit            = AttributeHasName(attribute, "perUnit");
    is_width_or_height     = is_width || is_height;
    is_physical_dimensions = NodeHasName(ownerNode, "physicalDimensions");
    is_pixel_dimension     = is_width_or_height && (FALSE == is_physical_dimensions);
    is_physical_dimension  = is_width_or_height && is_physical_dimensions;
    is_integer             = is_id || is_x || is_y || is_pixel_dimension;
    is_float               = is_x_scale || is_y_scale || is_physical_dimension;
    is_unit                = is_units || is_base_unit || is_per_unit;

    kind = CONFIGURATION_ATTRIBUTE_TEXT;
    if (is_integer)
    {
        kind = CONFIGURATION_ATTRIBUTE_INTEGER;
    }
    else if (is_float)
    {
        kind = CONFIGURATION_ATTRIBUTE_FLOAT;
    }
    else if (is_unit)
    {
        kind = CONFIGURATION_ATTRIBUTE_UNIT;
    }

    return kind;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Parses a numeric value once when it enters editor storage.
 */
static void InitializeAttributeValue(ConfigurationAttributeValue_t* valueState, const Utl_Xml_Node_t* ownerNode, const Utl_Xml_Attribute_t* attribute)
{
    valueState->kind    = ClassifyAttribute(ownerNode, attribute);
    valueState->isValid = TRUE;

    if (CONFIGURATION_ATTRIBUTE_INTEGER == valueState->kind)
    {
        valueState->isValid = ParseUnsignedValue(valueState->text, &valueState->integerValue);
    }
    else if (CONFIGURATION_ATTRIBUTE_FLOAT == valueState->kind)
    {
        valueState->isValid = ParsePixelValue(valueState->text, &valueState->floatValue);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Parses an unsigned integer for an inspector number field.
 */
static bool_t ParseUnsignedValue(const char_t* text, uint32_t* value)
{
    char_t*  end;
    uint64_t parsed_value;
    bool_t   is_empty;
    bool_t   is_negative;
    bool_t   is_out_of_range;
    bool_t   has_invalid_tail;
    bool_t   is_valid;

    is_valid = (NULL != text);

    if (is_valid)
    {
        is_empty    = ('\0' == text[0]);
        is_negative = ('-' == text[0]);
        is_valid    = (FALSE == is_empty) && (FALSE == is_negative);

        if (is_valid)
        {
            errno        = 0;
            parsed_value = strtoull(text, &end, 10);

            is_out_of_range  = (ERANGE == errno) || (UINT32_MAX < parsed_value);
            has_invalid_tail = ('\0' != (*end));
            is_valid         = (FALSE == is_out_of_range) && (FALSE == has_invalid_tail);

            if (is_valid)
            {
                (*value) = (uint32_t)parsed_value;
            }
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses numeric fields and unit choices for known configuration attributes.
 */
static void DrawAttributeControl(ConfigurationEditor_t* editor, Utl_Xml_Attribute_t* attribute)
{
    ConfigurationAttributeValue_t* value_state;
    uint32_t                       option_index;
    bool_t                         is_selected;
    bool_t                         is_finite;
    bool_t                         is_nonnegative;

    value_state = (ConfigurationAttributeValue_t*)attribute->value;
    if (CONFIGURATION_ATTRIBUTE_INTEGER == value_state->kind)
    {
        if (FALSE == value_state->isValid)
        {
            ImGui::TextUnformatted("Invalid number");
            ImGui::SameLine();
            if (ImGui::SmallButton("Set 0"))
            {
                value_state->integerValue = 0;
                value_state->isValid      = TRUE;
                strcpy(value_state->text, "0");
                editor->isPreviewDirty = TRUE;
            }
        }
        else if (ImGui::InputScalar("##value", ImGuiDataType_U32, &value_state->integerValue))
        {
            snprintf(value_state->text, sizeof(value_state->text), "%u", value_state->integerValue);
            editor->isPreviewDirty = TRUE;
        }
    }
    else if (CONFIGURATION_ATTRIBUTE_FLOAT == value_state->kind)
    {
        if (FALSE == value_state->isValid)
        {
            ImGui::TextUnformatted("Invalid number");
            ImGui::SameLine();
            if (ImGui::SmallButton("Set 0"))
            {
                value_state->floatValue = 0.0f;
                value_state->isValid    = TRUE;
                strcpy(value_state->text, "0");
                editor->isPreviewDirty = TRUE;
            }
        }
        else if (ImGui::InputFloat("##value", &value_state->floatValue, 0.0f, 0.0f, "%.6g"))
        {
            is_finite              = (0 != _finite(value_state->floatValue));
            is_nonnegative         = (0.0f <= value_state->floatValue);
            value_state->isValid   = is_finite && is_nonnegative;
            snprintf(value_state->text, sizeof(value_state->text), "%.9g", value_state->floatValue);
            editor->isPreviewDirty = TRUE;
        }
    }
    else if (CONFIGURATION_ATTRIBUTE_UNIT == value_state->kind)
    {
        if (ImGui::BeginCombo("##value", value_state->text))
        {
            for (option_index = 0; option_index < COUNT_OF(c_unitOptions); ++option_index)
            {
                is_selected = (0 == strcmp(value_state->text, c_unitOptions[option_index]));
                if (ImGui::Selectable(c_unitOptions[option_index], is_selected))
                {
                    strcpy(value_state->text, c_unitOptions[option_index]);
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    else
    {
        ImGui::InputText("##value", value_state->text, sizeof(value_state->text));
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
    uint32_t             attribute_index;

    if (NULL != node)
    {
        ImGui::Separator();
        ImGui::TextUnformatted(heading);
        if (NULL == node->firstAttribute)
        {
            ImGui::TextDisabled("No properties");
        }
        else
        {
            ImGui::PushID(node);
            if (ImGui::BeginTable("Properties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 125.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                attribute = node->firstAttribute;
                for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
                {
                    LOGASSERT(NULL != attribute);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted((const char_t*)attribute->name, (const char_t*)attribute->name + attribute->nameByteCount);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(attribute);
                    ImGui::SetNextItemWidth(-1.0f);
                    DrawAttributeControl(editor, attribute);
                    ImGui::PopID();
                    attribute = attribute->next;
                }
                LOGASSERT(NULL == attribute);
                ImGui::EndTable();
            }
            ImGui::PopID();
        }
    }
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
    }
    else if (NodeHasName(node, "screen"))
    {
        DrawAttributeSection(editor, node, "Screen");
        DrawAttributeSection(editor, FindChild(node, "pixelSize"), "Pixel Size");
        DrawAttributeSection(editor, FindChild(node, "physicalDimensions"), "Physical Dimensions");
    }
    else if (NodeHasName(node, "layout"))
    {
        DrawAttributeSection(editor, node, "Layout");
    }
    else if (NodeHasName(node, "window"))
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
    Utl_Xml_Node_t* root;
    ImVec2          available_size;
    float32_t       hierarchy_height;
    bool_t          is_screen_selected;
    bool_t          is_layout_selected;
    bool_t          is_window_selected;

    if (ImGui::Begin(CONFIGURATION_WINDOW_NAME))
    {
        ImGui::TextUnformatted("arinc661_configuration.xml");
        if (ImGui::Button("Reload XML"))
        {
            LoadConfiguration(editor);
        }
        if (editor->isLoaded)
        {
            ImGui::SameLine();
            if (ImGui::Button("Build XML"))
            {
                BuildConfiguration(editor);
            }
        }
        ImGui::TextWrapped("%s", editor->status);
        if (editor->isLoaded)
        {
            root = FindDocumentElement(editor);
            ImGui::TextUnformatted("Hierarchy");
            if (ImGui::SmallButton("Add Screen"))
            {
                AddScreen(editor);
            }
            is_screen_selected = FALSE;
            is_layout_selected = FALSE;
            is_window_selected = FALSE;
            if (NULL != editor->selectedNode)
            {
                is_screen_selected = NodeHasName(editor->selectedNode, "screen");
                is_layout_selected = NodeHasName(editor->selectedNode, "layout");
                is_window_selected = NodeHasName(editor->selectedNode, "window");
            }
            if (is_screen_selected)
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
            else if (is_layout_selected)
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
            else if (is_window_selected)
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
            hierarchy_height = available_size.y * 0.36f;
            if (150.0f > hierarchy_height)
            {
                hierarchy_height = 150.0f;
            }
            if (ImGui::BeginChild("Hierarchy", ImVec2(0.0f, hierarchy_height), TRUE))
            {
                if (NULL != root)
                {
                    DrawConfigurationHierarchy(editor, root);
                }
            }
            ImGui::EndChild();
            ImGui::TextUnformatted("Inspector");
            DrawSelectedNodeInspector(editor);
            if (NULL != root)
            {
                if (ImGui::CollapsingHeader("Document settings"))
                {
                    DrawAttributeSection(editor, root, "Configuration");
                }
            }
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
    uint32_t name_byte_count;
    bool_t   has_same_name_byte_count;
    bool_t   has_same_name;

    name_byte_count = (uint32_t)strlen(name);
    has_same_name_byte_count = (node->nameByteCount == name_byte_count);
    has_same_name            = FALSE;

    if (has_same_name_byte_count)
    {
        has_same_name = (0 == memcmp(node->name, name, name_byte_count));
    }

    return has_same_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds a direct child with the requested element name.
 */
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name)
{
    Utl_Xml_Node_t* child;
    Utl_Xml_Node_t* found_child;
    uint32_t        child_index;

    found_child = NULL;
    child       = parent->firstChild;
    for (child_index = 0; child_index < UTL_XML_DOCUMENT_MAX_NODES; ++child_index)
    {
        if (NULL == child)
        {
            break;
        }
        if (NodeHasName(child, name))
        {
            found_child = child;
            break;
        }
        child = child->nextSibling;
    }
    if (NULL == found_child)
    {
        LOGASSERT(NULL == child);
    }
    return found_child;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds an edited value by its XML attribute name.
 */
static const char_t* FindAttributeValue(Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* attribute;
    const char_t*        found_value;
    uint32_t             attribute_index;

    found_value = NULL;
    attribute   = node->firstAttribute;
    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != attribute);
        if (AttributeHasName(attribute, name))
        {
            found_value = (const char_t*)attribute->value;
            break;
        }
        attribute = attribute->next;
    }
    if (NULL == found_value)
    {
        LOGASSERT(NULL == attribute);
    }
    return found_value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a nonnegative finite pixel coordinate from an edited value.
 */
static bool_t ParsePixelValue(const char_t* text, float32_t* value)
{
    char_t*   end;
    float32_t parsed_value;
    bool_t    is_empty;
    bool_t    is_out_of_range;
    bool_t    has_invalid_tail;
    bool_t    is_not_finite;
    bool_t    is_negative;
    bool_t    is_valid;

    is_valid = (NULL != text);

    if (is_valid)
    {
        is_empty = ('\0' == text[0]);
        is_valid = (FALSE == is_empty);

        if (is_valid)
        {
            errno        = 0;
            parsed_value = strtof(text, &end);

            is_out_of_range  = (ERANGE == errno);
            has_invalid_tail = ('\0' != (*end));
            is_not_finite    = (0 == _finite(parsed_value));
            is_negative      = (0.0f > parsed_value);
            is_valid         = (FALSE == is_out_of_range) && (FALSE == has_invalid_tail) &&
                              (FALSE == is_not_finite) && (FALSE == is_negative);

            if (is_valid)
            {
                (*value) = parsed_value;
            }
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a window pixel area from the current editor values.
 */
static bool_t ReadPixelRectangle(Utl_Xml_Node_t* node, float32_t* x, float32_t* y, float32_t* width, float32_t* height)
{
    bool_t is_valid;

    is_valid = ParsePixelValue(FindAttributeValue(node, "x"), x);
    if (is_valid)
    {
        is_valid = ParsePixelValue(FindAttributeValue(node, "y"), y);
    }
    if (is_valid)
    {
        is_valid = ParsePixelValue(FindAttributeValue(node, "width"), width);
    }
    if (is_valid)
    {
        is_valid = ParsePixelValue(FindAttributeValue(node, "height"), height);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads positive screen dimensions from the current editor values.
 */
static bool_t ReadScreenDimensions(Utl_Xml_Node_t* screen, float32_t* width, float32_t* height)
{
    Utl_Xml_Node_t* pixel_size_node;
    bool_t          has_positive_width;
    bool_t          has_positive_height;
    bool_t          is_valid;

    pixel_size_node = FindChild(screen, "pixelSize");
    is_valid        = (NULL != pixel_size_node);

    if (is_valid)
    {
        is_valid = ParsePixelValue(FindAttributeValue(pixel_size_node, "width"), width);
    }
    if (is_valid)
    {
        is_valid = ParsePixelValue(FindAttributeValue(pixel_size_node, "height"), height);
    }
    if (is_valid)
    {
        has_positive_width  = (0.0f < (*width));
        has_positive_height = (0.0f < (*height));
        is_valid            = has_positive_width && has_positive_height;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Collects window rectangles from one screen after an XML change.
 */
static void CollectPreviewWindows(ConfigurationEditor_t* editor, Utl_Xml_Node_t* screenNode)
{
    Utl_Xml_Node_t*               layout_node;
    Utl_Xml_Node_t*               window_node;
    Utl_Xml_Node_t*               pixel_area_node;
    ConfigurationPreviewWindow_t* preview_window;
    float32_t                     x;
    float32_t                     y;
    float32_t                     width;
    float32_t                     height;
    uint32_t                      layout_index;
    uint32_t                      window_index;
    bool_t                        is_layout;
    bool_t                        is_window;
    bool_t                        has_rectangle;

    layout_node = screenNode->firstChild;
    for (layout_index = 0; layout_index < UTL_XML_DOCUMENT_MAX_NODES; ++layout_index)
    {
        if (NULL == layout_node)
        {
            break;
        }
        is_layout = NodeHasName(layout_node, "layout");
        if (is_layout)
        {
            window_node = layout_node->firstChild;
            for (window_index = 0; window_index < UTL_XML_DOCUMENT_MAX_NODES; ++window_index)
            {
                if (NULL == window_node)
                {
                    break;
                }
                is_window = NodeHasName(window_node, "window");
                if (is_window)
                {
                    pixel_area_node = FindChild(window_node, "pixelArea");
                    if (NULL != pixel_area_node)
                    {
                        has_rectangle = ReadPixelRectangle(pixel_area_node, &x, &y, &width, &height);
                        if (has_rectangle)
                        {
                            LOGASSERT(editor->previewWindowCount < CONFIGURATION_MAX_PREVIEW_NODES);
                            preview_window = &editor->previewWindows[editor->previewWindowCount];
                            preview_window->name   = FindAttributeValue(window_node, "name");
                            preview_window->x      = x;
                            preview_window->y      = y;
                            preview_window->width  = width;
                            preview_window->height = height;
                            editor->previewWindowCount += 1;
                        }
                    }
                }
                window_node = window_node->nextSibling;
            }
            LOGASSERT(NULL == window_node);
        }
        layout_node = layout_node->nextSibling;
    }
    LOGASSERT(NULL == layout_node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rebuilds the visualizer state only after an XML edit.
 */
static void UpdatePreview(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t*               root_node;
    Utl_Xml_Node_t*               screen_node;
    ConfigurationPreviewScreen_t* preview_screen;
    float32_t                     screen_width;
    float32_t                     screen_height;
    uint32_t                      screen_index;
    bool_t                        is_screen;
    bool_t                        has_dimensions;

    editor->previewScreenCount   = 0;
    editor->previewWindowCount   = 0;
    editor->previewTotalWidth    = 0.0f;
    editor->previewMaximumHeight = 0.0f;

    if (editor->isLoaded)
    {
        root_node = FindDocumentElement(editor);
        if (NULL != root_node)
        {
            screen_node = root_node->firstChild;
            for (screen_index = 0; screen_index < UTL_XML_DOCUMENT_MAX_NODES; ++screen_index)
            {
                if (NULL == screen_node)
                {
                    break;
                }
                is_screen = NodeHasName(screen_node, "screen");
                if (is_screen)
                {
                    has_dimensions = ReadScreenDimensions(screen_node, &screen_width, &screen_height);
                    if (has_dimensions)
                    {
                        LOGASSERT(editor->previewScreenCount < CONFIGURATION_MAX_PREVIEW_NODES);
                        preview_screen = &editor->previewScreens[editor->previewScreenCount];
                        preview_screen->id               = FindAttributeValue(screen_node, "id");
                        preview_screen->width            = screen_width;
                        preview_screen->height           = screen_height;
                        preview_screen->firstWindowIndex = editor->previewWindowCount;
                        CollectPreviewWindows(editor, screen_node);
                        preview_screen->windowCount = editor->previewWindowCount - preview_screen->firstWindowIndex;
                        editor->previewScreenCount += 1;
                        editor->previewTotalWidth += screen_width;
                        if (editor->previewMaximumHeight < screen_height)
                        {
                            editor->previewMaximumHeight = screen_height;
                        }
                    }
                }
                screen_node = screen_node->nextSibling;
            }
            LOGASSERT(NULL == screen_node);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws measured screens and their windows in the visualizer canvas.
 */
static void DrawScreenCanvas(const ConfigurationEditor_t* editor)
{
    const ConfigurationPreviewScreen_t* screen;
    const ConfigurationPreviewWindow_t* window;
    ImDrawList*                         draw_list;
    ImVec2                              canvas_min;
    ImVec2                              canvas_size;
    ImVec2                              screen_min;
    ImVec2                              screen_max;
    ImVec2                              area_min;
    ImVec2                              area_max;
    char_t                              screen_label[CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 16];
    float32_t                           available_width;
    float32_t                           available_height;
    float32_t                           scale;
    float32_t                           x;
    float32_t                           y;
    uint32_t                            screen_index;
    uint32_t                            window_index;
    bool_t                              has_canvas_width;
    bool_t                              has_canvas_height;
    bool_t                              has_canvas_space;

    canvas_min       = ImGui::GetCursorScreenPos();
    canvas_size      = ImGui::GetContentRegionAvail();
    available_width  = canvas_size.x - 40.0f - 20.0f * (float32_t)(editor->previewScreenCount - 1);
    available_height = canvas_size.y - 60.0f;
    has_canvas_width  = (0.0f < available_width);
    has_canvas_height = (0.0f < available_height);
    has_canvas_space  = has_canvas_width && has_canvas_height;
    if (FALSE == has_canvas_space)
    {
        ImGui::TextUnformatted("Preview area is too small.");
    }
    else
    {
        ImGui::InvisibleButton("Screen canvas", canvas_size);
        draw_list = ImGui::GetWindowDrawList();
        draw_list->PushClipRect(canvas_min, ImVec2(canvas_min.x + canvas_size.x, canvas_min.y + canvas_size.y), TRUE);
        draw_list->AddRectFilled(canvas_min, ImVec2(canvas_min.x + canvas_size.x, canvas_min.y + canvas_size.y), IM_COL32(22, 27, 34, 255));

        scale = available_width / editor->previewTotalWidth;
        if (available_height / editor->previewMaximumHeight < scale)
        {
            scale = available_height / editor->previewMaximumHeight;
        }
        x = canvas_min.x + 20.0f;
        y = canvas_min.y + 35.0f + (available_height - editor->previewMaximumHeight * scale) * 0.5f;
        for (screen_index = 0; screen_index < editor->previewScreenCount; ++screen_index)
        {
            screen = &editor->previewScreens[screen_index];
            screen_min = ImVec2(x, y);
            screen_max = ImVec2(x + screen->width * scale, y + screen->height * scale);
            draw_list->AddRectFilled(screen_min, screen_max, IM_COL32(42, 50, 64, 255));
            draw_list->AddRect(screen_min, screen_max, IM_COL32(220, 230, 245, 255), 0.0f, 0, 2.0f);
            if (NULL != screen->id)
            {
                snprintf(screen_label, sizeof(screen_label), "Screen %s", screen->id);
                draw_list->AddText(ImVec2(x, y - 22.0f), IM_COL32(240, 248, 255, 255), screen_label);
            }
            draw_list->PushClipRect(screen_min, screen_max, TRUE);
            for (window_index = screen->firstWindowIndex; window_index < screen->firstWindowIndex + screen->windowCount; ++window_index)
            {
                window = &editor->previewWindows[window_index];
                area_min = ImVec2(screen_min.x + window->x * scale, screen_min.y + window->y * scale);
                area_max = ImVec2(area_min.x + window->width * scale, area_min.y + window->height * scale);
                draw_list->AddRectFilled(area_min, area_max, IM_COL32(42, 154, 188, 80));
                draw_list->AddRect(area_min, area_max, IM_COL32(97, 221, 245, 255), 0.0f, 0, 2.0f);
                if (NULL != window->name)
                {
                    if ('\0' != window->name[0])
                    {
                        draw_list->AddText(ImVec2(area_min.x + 6.0f, area_min.y + 6.0f), IM_COL32(240, 248, 255, 255), window->name);
                    }
                }
            }
            draw_list->PopClipRect();
            x = screen_max.x + 20.0f;
        }
        draw_list->PopClipRect();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows screen sizes and window pixel areas from the current field values.
 */
static void DrawVisualizer(ConfigurationEditor_t* editor)
{
    if (ImGui::Begin(VISUALIZER_WINDOW_NAME))
    {
        if (FALSE == editor->isLoaded)
        {
            ImGui::TextUnformatted(editor->status);
        }
        else
        {
            if (0 == editor->previewScreenCount)
            {
                ImGui::TextUnformatted("No screens with valid pixel dimensions.");
            }
            else
            {
                DrawScreenCanvas(editor);
            }
        }
    }
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
    UpdateConfiguration(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Refreshes visualizer geometry only after the XML tree or a numeric value changes.
 */
void UpdateConfiguration(ConfigurationEditor_t* editor)
{
    if (editor->isPreviewDirty)
    {
        UpdatePreview(editor);
        editor->isPreviewDirty = FALSE;
    }
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
