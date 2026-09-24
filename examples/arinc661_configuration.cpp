/*=================================================================================================
** 1. References
**===============================================================================================*/

/* Win32 and OpenGL setup adapted from imgui-docking/examples/example_win32_opengl3/main.cpp. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#include "misc/cpp/imgui_stdlib.h"

extern "C"
{
#include "os/os_assert.h"
#include "utl/utl_xml.h"
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <iterator>
#include <limits.h>
#include <string>
#include <unordered_map>
#include <vector>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#ifndef ARINC661_CONFIGURATION_XML_PATH
#define ARINC661_CONFIGURATION_XML_PATH "examples/arinc661_configuration.xml"
#endif

#define CONFIGURATION_WINDOW_NAME ("ARINC 661 Configuration")
#define VISUALIZER_WINDOW_NAME ("Visualizer")
#define DOCKSPACE_NAME ("ARINC661DockSpace")

#define OPENGL_MAJOR_VERSION_PARAMETER (0x821B)
#define OPENGL_MINOR_VERSION_PARAMETER (0x821C)
#define OPENGL_VERSION_3_0 (300)
#define WGL_MAJOR_VERSION_ATTRIBUTE (0x2091)
#define WGL_MINOR_VERSION_ATTRIBUTE (0x2092)
#define WGL_PROFILE_MASK_ATTRIBUTE (0x9126)
#define WGL_CORE_PROFILE_BIT (0x0001)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Editable attribute values keyed by stable XML attribute addresses.
 */
typedef std::unordered_map<Utl_Xml_Attribute_t*, std::string> AttributeValues_t;

/*************************************************************************************************/
/**
 * \brief Parsed configuration and editable copies of its attribute values.
 */
typedef struct ConfigurationEditor_s
{
    std::vector<byte_t>              inputBuffer;         ///< Mutable XML bytes borrowed by parsed nodes.
    std::vector<Utl_Xml_Node_t>      nodes;               ///< Parsed node storage.
    std::vector<Utl_Xml_Attribute_t> attributes;          ///< Parsed attribute storage.
    std::deque<Utl_Xml_Node_t>       addedNodes;          ///< Stable storage for newly added nodes.
    std::deque<Utl_Xml_Attribute_t>  addedAttributes;     ///< Stable storage for newly added attributes.
    Utl_Xml_Document_t               document;            ///< Parsed and edited XML tree.
    AttributeValues_t                attributeValues;     ///< Editable values for linked attributes.
    Utl_Xml_Node_t*                  selectedNode;        ///< Selected screen, layout, or window.
    std::string                      status;              ///< Current load or build message.
    bool_t                           isLoaded;            ///< TRUE once the document has been read.
    bool_t                           isLayoutInitialized; ///< TRUE once the dock layout has been built.
} ConfigurationEditor_t;

/*************************************************************************************************/
/**
 * \brief Screen dimensions read from the editable XML tree.
 */
typedef struct ScreenPreview_s
{
    Utl_Xml_Node_t* node;   ///< Screen element in the XML tree.
    float           width;  ///< Pixel width shown in the preview.
    float           height; ///< Pixel height shown in the preview.
} ScreenPreview_t;

/*************************************************************************************************/
/**
 * \brief Device context owned by an ImGui platform viewport.
 */
typedef struct WglWindowData_s
{
    HDC deviceContext; ///< Win32 device context for the viewport.
} WglWindowData_t;

/*************************************************************************************************/
/**
 * \brief WGL extension signature for creating an OpenGL context.
 */
typedef HGLRC(WINAPI* WglCreateContextAttribsFunction_t)(HDC, HGLRC, const int*);

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Rendering state shared with Dear ImGui platform viewport callbacks.
 */
static HGLRC           g_renderContext;
static WglWindowData_t g_mainWindow;
static int             g_renderWidth;
static int             g_renderHeight;

/*************************************************************************************************/
/**
 * \brief Unit choices used by the configuration example.
 */
static const char_t* const c_unitOptions[] = {"pixel", "inch", "screen"};

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
static void CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);

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
 * \brief Adds a byte count without overflowing the output capacity.
 *
 * \param[in]     byteCount   Number of bytes to add
 * \param[in,out] capacity    Accumulated output capacity
 *
 * \return TRUE if the byte count fits; FALSE on overflow.
 */
static bool_t AddOutputBytes(uint64_t byteCount, uint64_t* capacity);

/*************************************************************************************************/
/**
 * \brief Bounds the writer's markup, values, and indentation for one subtree.
 *
 * \param[in]     node       XML node
 * \param[in]     depth      Depth of the current XML node
 * \param[in,out] capacity   Accumulated output capacity
 *
 * \return TRUE if the subtree fits; FALSE on overflow.
 */
static bool_t AddNodeOutputCapacity(const Utl_Xml_Node_t* node, uint32_t depth, uint64_t* capacity);

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
static bool_t ParseUnsignedValue(const std::string* text, uint32_t* value);

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
static void DrawAttributeControl(const Utl_Xml_Node_t* ownerNode, Utl_Xml_Attribute_t* attribute, std::string* value);

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
static const std::string* FindAttributeValue(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Reads a nonnegative finite pixel coordinate from an edited value.
 *
 * \param[in]     text    Editable numeric text
 * \param[out]    value   Attribute value
 *
 * \return TRUE for a finite nonnegative value; FALSE otherwise.
 */
static bool_t ParsePixelValue(const std::string* text, float* value);

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

/*************************************************************************************************/
/**
 * \brief Starts a new Dear ImGui frame.
 *
 * \retval None
 */
static void BeginFrame(void);

/*************************************************************************************************/
/**
 * \brief Draws the dockspace, configuration editor, and visualizer.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
static void DrawApp(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Renders the Dear ImGui frame and presents the window.
 *
 * \retval None
 */
static void EndFrame(void);

/*************************************************************************************************/
/**
 * \brief Creates a Win32 OpenGL context for the main or platform window.
 *
 * \param[in]     windowHandle   Win32 window handle
 * \param[in,out] windowData     Window rendering data
 *
 * \return TRUE when rendering setup succeeds; FALSE otherwise.
 */
static bool_t CreateDeviceWGL(HWND windowHandle, WglWindowData_t* windowData);

/*************************************************************************************************/
/**
 * \brief Releases a Win32 window device context.
 *
 * \param[in]     windowHandle   Win32 window handle
 * \param[in,out] windowData     Window rendering data
 *
 * \retval None
 */
static void CleanupDeviceWGL(HWND windowHandle, WglWindowData_t* windowData);

/*************************************************************************************************/
/**
 * \brief Creates renderer data for an ImGui platform viewport.
 *
 * \param[in,out] viewport   ImGui platform viewport
 *
 * \retval None
 */
static void HookRendererCreateWindow(ImGuiViewport* viewport);

/*************************************************************************************************/
/**
 * \brief Releases renderer data for an ImGui platform viewport.
 *
 * \param[in,out] viewport   ImGui platform viewport
 *
 * \retval None
 */
static void HookRendererDestroyWindow(ImGuiViewport* viewport);

/*************************************************************************************************/
/**
 * \brief Activates the shared OpenGL context for a platform viewport.
 *
 * \param[in]     viewport   ImGui platform viewport
 * \param[in]     data       Unused renderer callback data
 *
 * \retval None
 */
static void HookPlatformRenderWindow(ImGuiViewport* viewport, void* data);

/*************************************************************************************************/
/**
 * \brief Presents a platform viewport.
 *
 * \param[in]     viewport   ImGui platform viewport
 * \param[in]     data       Unused renderer callback data
 *
 * \retval None
 */
static void HookRendererSwapBuffers(ImGuiViewport* viewport, void* data);

/*************************************************************************************************/
/**
 * \brief Routes Win32 input and window events to ImGui and the renderer.
 *
 * \param[in]     windowHandle    Win32 window handle
 * \param[in]     message         Win32 message identifier
 * \param[in]     wordParameter   Win32 message word parameter
 * \param[in]     longParameter   Win32 message long parameter
 *
 * \return Win32 message result.
 */
static LRESULT WINAPI WindowProcedure(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter);

/* The Win32 backend exposes this handler without declaring it in its public header. */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Copies parsed attribute values into editable storage.
 */
static void CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t* attribute;
    Utl_Xml_Node_t*      child;

    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        editor->attributeValues.emplace(attribute, std::string(reinterpret_cast<const char_t*>(attribute->value), attribute->valueByteCount));
    }
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        CollectAttributes(editor, child);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the configuration through Utl_Xml_ReadDocument.
 */
static void LoadConfiguration(ConfigurationEditor_t* editor)
{
    std::ifstream   xml_file;
    std::string     xml_text;
    size_t          tag_count;
    size_t          equals_count;
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* first_screen;

    editor->isLoaded     = FALSE;
    editor->selectedNode = NULL;
    editor->attributeValues.clear();
    editor->nodes.clear();
    editor->attributes.clear();
    editor->addedNodes.clear();
    editor->addedAttributes.clear();
    editor->inputBuffer.clear();

    xml_file.open(ARINC661_CONFIGURATION_XML_PATH, std::ios::binary);
    if (!xml_file)
    {
        editor->status = "Could not open arinc661_configuration.xml.";
        return;
    }
    xml_text.assign(std::istreambuf_iterator<char_t>(xml_file), std::istreambuf_iterator<char_t>());
    if (xml_file.bad())
    {
        editor->status = "Could not read arinc661_configuration.xml.";
        return;
    }

    tag_count    = std::count(xml_text.begin(), xml_text.end(), '<');
    equals_count = std::count(xml_text.begin(), xml_text.end(), '=');
    if ((0 == tag_count) || (UTL_XML_DOCUMENT_MAX_NODES < tag_count) || (UINT32_MAX <= equals_count))
    {
        editor->status = "The XML file exceeds the reader's storage limits.";
        return;
    }

    editor->inputBuffer.assign(xml_text.begin(), xml_text.end());
    editor->inputBuffer.push_back(0);
    editor->nodes.resize(tag_count);
    editor->attributes.resize(equals_count + 1);
    Utl_Xml_InitializeDocument(&editor->document);
    Utl_Xml_ReadDocument(&editor->document, editor->inputBuffer.data(), static_cast<uint64_t>(xml_text.size()),
                         editor->nodes.data(), static_cast<uint32_t>(editor->nodes.size()),
                         editor->attributes.data(), static_cast<uint32_t>(editor->attributes.size()));

    for (node = editor->document.firstNode; NULL != node; node = node->nextSibling)
    {
        CollectAttributes(editor, node);
        if ((NULL == editor->selectedNode) && (UTL_XML_NODE_ELEMENT == node->type))
        {
            editor->selectedNode = node;
        }
    }
    first_screen = FindChild(editor->selectedNode, "screen");
    if (NULL != first_screen)
    {
        editor->selectedNode = first_screen;
    }
    editor->isLoaded = TRUE;
    editor->status   = "Loaded arinc661_configuration.xml.";
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds a byte count without overflowing the output capacity.
 */
static bool_t AddOutputBytes(uint64_t byteCount, uint64_t* capacity)
{
    if (byteCount > UINT64_MAX - *capacity)
    {
        return FALSE;
    }
    *capacity += byteCount;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Bounds the writer's markup, values, and indentation for one subtree.
 */
static bool_t AddNodeOutputCapacity(const Utl_Xml_Node_t* node, uint32_t depth, uint64_t* capacity)
{
    const Utl_Xml_Node_t*      child;
    const Utl_Xml_Attribute_t* attribute;

    if ((UTL_XML_NODE_MAX_DEPTH < depth) ||
        (FALSE == AddOutputBytes(64 + 8 * static_cast<uint64_t>(depth), capacity)) ||
        (FALSE == AddOutputBytes(2 * static_cast<uint64_t>(node->nameByteCount), capacity)))
    {
        return FALSE;
    }
    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        if ((FALSE == AddOutputBytes(static_cast<uint64_t>(attribute->nameByteCount) + 4, capacity)) ||
            (FALSE == AddOutputBytes(attribute->valueByteCount, capacity)))
        {
            return FALSE;
        }
    }
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        if (FALSE == AddNodeOutputCapacity(child, depth + 1, capacity))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates edited values and writes the document with Utl_Xml_WriteDocument.
 */
static void BuildConfiguration(ConfigurationEditor_t* editor)
{
    AttributeValues_t::iterator value_iterator;
    const std::string*          value;
    const Utl_Xml_Attribute_t*  attribute;
    const Utl_Xml_Node_t*       node;
    std::vector<byte_t>         output_buffer;
    FILE*                       xml_file;
    uint64_t                    output_capacity;
    uint64_t                    output_byte_count;
    size_t                      bytes_written;
    int                         close_result;

    output_capacity = 0;
    for (value_iterator = editor->attributeValues.begin(); value_iterator != editor->attributeValues.end(); ++value_iterator)
    {
        value = &value_iterator->second;
        if ((UTL_XML_MAX_VALUE_BYTES < value->size()) ||
            (FALSE == Utl_Xml_IsAttributeValue(reinterpret_cast<const byte_t*>(value->c_str()), static_cast<uint32_t>(value->size()))))
        {
            attribute      = value_iterator->first;
            editor->status = "Invalid XML attribute value: " +
                             std::string(reinterpret_cast<const char_t*>(attribute->name), attribute->nameByteCount);
            return;
        }
    }

    for (value_iterator = editor->attributeValues.begin(); value_iterator != editor->attributeValues.end(); ++value_iterator)
    {
        value_iterator->first->value          = reinterpret_cast<const byte_t*>(value_iterator->second.c_str());
        value_iterator->first->valueByteCount = static_cast<uint32_t>(value_iterator->second.size());
    }
    for (node = editor->document.firstNode; NULL != node; node = node->nextSibling)
    {
        if (FALSE == AddNodeOutputCapacity(node, 0, &output_capacity))
        {
            editor->status = "The XML output is too large.";
            return;
        }
    }
    if (output_capacity > SIZE_MAX)
    {
        editor->status = "The XML output is too large.";
        return;
    }

    output_buffer.resize(static_cast<size_t>(output_capacity));
    output_byte_count = Utl_Xml_WriteDocument(&editor->document, output_buffer.data(), output_capacity);

    xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "wb");
    if (NULL == xml_file)
    {
        editor->status = "Could not write arinc661_configuration.xml.";
        return;
    }
    bytes_written = fwrite(output_buffer.data(), 1, static_cast<size_t>(output_byte_count), xml_file);
    close_result  = fclose(xml_file);
    if ((output_byte_count != bytes_written) || (0 != close_result))
    {
        editor->status = "Could not finish writing arinc661_configuration.xml.";
        return;
    }
    editor->status = "Built arinc661_configuration.xml.";
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Allocates a stable XML element for an editor addition.
 */
static Utl_Xml_Node_t* CreateElement(ConfigurationEditor_t* editor, const char_t* name)
{
    Utl_Xml_Node_t* node;

    editor->addedNodes.emplace_back();
    node = &editor->addedNodes.back();
    Utl_Xml_InitializeNode(node, UTL_XML_NODE_ELEMENT, reinterpret_cast<const byte_t*>(name), static_cast<uint32_t>(strlen(name)));
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
    std::string*         stored_value;

    editor->addedAttributes.emplace_back();
    attribute     = &editor->addedAttributes.back();
    stored_value  = &editor->attributeValues[attribute];
    *stored_value = value;
    Utl_Xml_InitializeAttribute(attribute, reinterpret_cast<const byte_t*>(name), static_cast<uint32_t>(strlen(name)),
                                reinterpret_cast<const byte_t*>(stored_value->c_str()), static_cast<uint32_t>(stored_value->size()));
    Utl_Xml_AppendAttribute(node, attribute);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Finds the largest numeric ID for one element type in a subtree.
 */
static void FindMaximumId(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* elementName, uint32_t* maximumId, bool_t* foundId)
{
    const std::string* id_text;
    Utl_Xml_Node_t*    child;
    uint32_t           id;

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

    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        editor->attributeValues.erase(attribute);
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
    std::string     id_text;

    root = FindDocumentElement(editor);
    if ((NULL == root) || (FALSE == NextElementId(editor, root, "screen", &next_id)))
    {
        editor->status = "Could not allocate a screen ID.";
        return;
    }
    id_text = std::to_string(next_id);
    screen  = CreateElement(editor, "screen");
    CreateAttribute(editor, screen, "id", id_text.c_str());

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
    editor->status       = "Added screen. Build XML to save.";
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
    std::string     id_text;

    if (FALSE == NextElementId(editor, screen, "layout", &next_id))
    {
        editor->status = "Could not allocate a layout ID.";
        return;
    }
    id_text = std::to_string(next_id);
    layout  = CreateElement(editor, "layout");
    CreateAttribute(editor, layout, "id", id_text.c_str());
    Utl_Xml_AppendChild(screen, layout);
    editor->selectedNode = layout;
    editor->status       = "Added layout. Build XML to save.";
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
    const std::string* edited_width;
    const std::string* edited_height;
    uint32_t           next_id;
    uint32_t           width;
    uint32_t           height;
    std::string        id_text;
    std::string        name_text;
    std::string        width_text;
    std::string        height_text;

    root = FindDocumentElement(editor);
    if ((NULL == root) || (FALSE == NextElementId(editor, root, "window", &next_id)))
    {
        editor->status = "Could not allocate a window ID.";
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
            width_text = *edited_width;
        }
        if ((NULL != edited_height) && (TRUE == ParseUnsignedValue(edited_height, &height)) && (0 < height))
        {
            height_text = *edited_height;
        }
    }

    id_text   = std::to_string(next_id);
    name_text = "window" + id_text;
    window    = CreateElement(editor, "window");
    CreateAttribute(editor, window, "id", id_text.c_str());
    CreateAttribute(editor, window, "name", name_text.c_str());

    pixel_area = CreateElement(editor, "pixelArea");
    CreateAttribute(editor, pixel_area, "x", "0");
    CreateAttribute(editor, pixel_area, "y", "0");
    CreateAttribute(editor, pixel_area, "width", width_text.c_str());
    CreateAttribute(editor, pixel_area, "height", height_text.c_str());
    Utl_Xml_AppendChild(window, pixel_area);

    scaling_factor = CreateElement(editor, "scalingFactor");
    CreateAttribute(editor, scaling_factor, "xScale", "20.0");
    CreateAttribute(editor, scaling_factor, "yScale", "20.0");
    CreateAttribute(editor, scaling_factor, "baseUnit", "screen");
    CreateAttribute(editor, scaling_factor, "perUnit", "pixel");
    Utl_Xml_AppendChild(window, scaling_factor);

    Utl_Xml_AppendChild(layout, window);
    editor->selectedNode = window;
    editor->status       = "Added window. Build XML to save.";
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
    editor->status       = "Removed selection. Build XML to save.";
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
    const std::string* identifier;
    std::string        node_name;
    std::string        label;
    ImGuiTreeNodeFlags flags;
    bool_t             is_open;
    bool_t             has_hierarchy_child;

    if (FALSE == IsHierarchyNode(node))
    {
        return;
    }

    node_name.assign(reinterpret_cast<const char_t*>(node->name), node->nameByteCount);
    label      = node_name;
    identifier = FindAttributeValue(editor, node, "name");
    if (NULL == identifier)
    {
        identifier = FindAttributeValue(editor, node, "id");
    }
    if (NULL != identifier)
    {
        label += " (" + *identifier + ")";
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
    is_open = ImGui::TreeNodeEx("node", flags, "%s", label.c_str());
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
static bool_t ParseUnsignedValue(const std::string* text, uint32_t* value)
{
    char_t*            end;
    unsigned long long parsed_value;

    if (text->empty() || ('-' == (*text)[0]))
    {
        return FALSE;
    }
    errno        = 0;
    parsed_value = std::strtoull(text->c_str(), &end, 10);
    if ((ERANGE == errno) || ('\0' != *end) || (UINT32_MAX < parsed_value))
    {
        return FALSE;
    }
    *value = static_cast<uint32_t>(parsed_value);
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses numeric fields and unit choices for known configuration attributes.
 */
static void DrawAttributeControl(const Utl_Xml_Node_t* ownerNode, Utl_Xml_Attribute_t* attribute, std::string* value)
{
    uint32_t integer_value;
    float    float_value;
    char_t   formatted_value[64];
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
                *value = "0";
            }
        }
        else if (ImGui::InputScalar("##value", ImGuiDataType_U32, &integer_value))
        {
            *value = std::to_string(integer_value);
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
                *value = "0";
            }
        }
        else if (ImGui::InputFloat("##value", &float_value, 0.0f, 0.0f, "%.6g"))
        {
            std::snprintf(formatted_value, sizeof(formatted_value), "%.9g", float_value);
            *value = formatted_value;
        }
    }
    else if ((TRUE == AttributeHasName(attribute, "units")) ||
             (TRUE == AttributeHasName(attribute, "baseUnit")) ||
             (TRUE == AttributeHasName(attribute, "perUnit")))
    {
        if (ImGui::BeginCombo("##value", value->c_str()))
        {
            for (option_index = 0; option_index < (sizeof(c_unitOptions) / sizeof(c_unitOptions[0])); ++option_index)
            {
                is_selected = (*value == c_unitOptions[option_index]);
                if (ImGui::Selectable(c_unitOptions[option_index], TRUE == is_selected))
                {
                    *value = c_unitOptions[option_index];
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
        ImGui::InputText("##value", value);
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
    std::string          attribute_name;

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
            attribute_name.assign(reinterpret_cast<const char_t*>(attribute->name), attribute->nameByteCount);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(attribute_name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(attribute);
            ImGui::SetNextItemWidth(-1.0f);
            DrawAttributeControl(node, attribute, &editor->attributeValues.at(attribute));
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
    ImGui::TextWrapped("%s", editor->status.c_str());
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
static const std::string* FindAttributeValue(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* attribute;
    size_t               name_byte_count;

    name_byte_count = strlen(name);
    for (attribute = node->firstAttribute; NULL != attribute; attribute = attribute->next)
    {
        if ((attribute->nameByteCount == name_byte_count) && (0 == memcmp(attribute->name, name, name_byte_count)))
        {
            return &editor->attributeValues.at(attribute);
        }
    }
    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads a nonnegative finite pixel coordinate from an edited value.
 */
static bool_t ParsePixelValue(const std::string* text, float* value)
{
    char_t* end;
    float   parsed_value;

    if ((NULL == text) || text->empty())
    {
        return FALSE;
    }
    errno        = 0;
    parsed_value = std::strtof(text->c_str(), &end);
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
    return (TRUE == ParsePixelValue(FindAttributeValue(editor, node, "x"), x)) &&
           (TRUE == ParsePixelValue(FindAttributeValue(editor, node, "y"), y)) &&
           (TRUE == ParsePixelValue(FindAttributeValue(editor, node, "width"), width)) &&
           (TRUE == ParsePixelValue(FindAttributeValue(editor, node, "height"), height));
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
    const std::string* window_name;
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
            if ((NULL != window_name) && !window_name->empty())
            {
                drawList->AddText(ImVec2(area_min.x + 6.0f, area_min.y + 6.0f), IM_COL32(240, 248, 255, 255), window_name->c_str());
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
    Utl_Xml_Node_t*              root;
    Utl_Xml_Node_t*              screen;
    Utl_Xml_Node_t*              pixel_size;
    const std::string*           screen_id;
    std::vector<ScreenPreview_t> screens;
    ScreenPreview_t              preview;
    ImDrawList*                  draw_list;
    ImVec2                       canvas_min;
    ImVec2                       canvas_size;
    ImVec2                       screen_min;
    ImVec2                       screen_max;
    std::string                  screen_label;
    float                        total_width;
    float                        maximum_height;
    float                        available_width;
    float                        available_height;
    float                        scale;
    float                        x;
    float                        y;
    size_t                       screen_index;

    ImGui::Begin(VISUALIZER_WINDOW_NAME);
    if (FALSE == editor->isLoaded)
    {
        ImGui::TextUnformatted(editor->status.c_str());
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
    for (screen = root->firstChild; NULL != screen; screen = screen->nextSibling)
    {
        if (FALSE == NodeHasName(screen, "screen"))
        {
            continue;
        }
        pixel_size     = FindChild(screen, "pixelSize");
        preview.node   = screen;
        preview.width  = 0.0f;
        preview.height = 0.0f;
        if ((NULL == pixel_size) ||
            (FALSE == ParsePixelValue(FindAttributeValue(editor, pixel_size, "width"), &preview.width)) ||
            (FALSE == ParsePixelValue(FindAttributeValue(editor, pixel_size, "height"), &preview.height)) ||
            (0.0f == preview.width) || (0.0f == preview.height))
        {
            continue;
        }
        screens.push_back(preview);
        total_width += preview.width;
        maximum_height = (std::max)(maximum_height, preview.height);
    }
    if (screens.empty())
    {
        ImGui::TextUnformatted("No screens with valid pixel dimensions.");
        ImGui::End();
        return;
    }

    canvas_min       = ImGui::GetCursorScreenPos();
    canvas_size      = ImGui::GetContentRegionAvail();
    available_width  = canvas_size.x - 40.0f - 20.0f * static_cast<float>(screens.size() - 1);
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
    for (screen_index = 0; screen_index < screens.size(); ++screen_index)
    {
        screen_min = ImVec2(x, y);
        screen_max = ImVec2(x + screens[screen_index].width * scale, y + screens[screen_index].height * scale);
        draw_list->AddRectFilled(screen_min, screen_max, IM_COL32(42, 50, 64, 255));
        draw_list->AddRect(screen_min, screen_max, IM_COL32(220, 230, 245, 255), 0.0f, 0, 2.0f);
        screen_id = FindAttributeValue(editor, screens[screen_index].node, "id");
        if (NULL != screen_id)
        {
            screen_label = "Screen " + *screen_id;
            draw_list->AddText(ImVec2(x, y - 22.0f), IM_COL32(240, 248, 255, 255), screen_label.c_str());
        }
        draw_list->PushClipRect(screen_min, screen_max, true);
        DrawScreenWindows(editor, screens[screen_index].node, draw_list, &screen_min, scale);
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

/*************************************************************************************************/
/**
 * \par Details:
 *  Starts the OpenGL, Win32, and Dear ImGui frame state.
 */
static void BeginFrame(void)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws the dockspace and both application windows for the current frame.
 */
static void DrawApp(ConfigurationEditor_t* editor)
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

/*************************************************************************************************/
/**
 * \par Details:
 *  Renders Dear ImGui draw data and presents the main and platform windows.
 */
static void EndFrame(void)
{
    ImGui::Render();
    glViewport(0, 0, g_renderWidth, g_renderHeight);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (0 != (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        wglMakeCurrent(g_mainWindow.deviceContext, g_renderContext);
    }
    ::SwapBuffers(g_mainWindow.deviceContext);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates a Win32 OpenGL context for the main or platform window.
 */
static bool_t CreateDeviceWGL(HWND windowHandle, WglWindowData_t* windowData)
{
    HDC                               device_context;
    PIXELFORMATDESCRIPTOR             pixel_format_description;
    int                               pixel_format;
    HGLRC                             temporary_context;
    HGLRC                             new_context;
    GLint                             major;
    GLint                             minor;
    const char_t*                     version_string;
    GLuint                            version;
    WglCreateContextAttribsFunction_t create_context_attribs;
    int                               context_attributes[7];

    windowData->deviceContext = NULL;
    device_context            = ::GetDC(windowHandle);
    if (NULL == device_context)
    {
        return FALSE;
    }
    memset(&pixel_format_description, 0, sizeof(pixel_format_description));
    pixel_format_description.nSize      = sizeof(pixel_format_description);
    pixel_format_description.nVersion   = 1;
    pixel_format_description.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format_description.iPixelType = PFD_TYPE_RGBA;
    pixel_format_description.cColorBits = 32;

    pixel_format = ::ChoosePixelFormat(device_context, &pixel_format_description);
    if ((0 == pixel_format) || (FALSE == ::SetPixelFormat(device_context, pixel_format, &pixel_format_description)))
    {
        ::ReleaseDC(windowHandle, device_context);
        return FALSE;
    }
    ::ReleaseDC(windowHandle, device_context);

    windowData->deviceContext = ::GetDC(windowHandle);
    if (NULL == windowData->deviceContext)
    {
        return FALSE;
    }
    if (NULL != g_renderContext)
    {
        return TRUE;
    }

    temporary_context = wglCreateContext(windowData->deviceContext);
    if (NULL == temporary_context)
    {
        CleanupDeviceWGL(windowHandle, windowData);
        return FALSE;
    }
    if (FALSE == wglMakeCurrent(windowData->deviceContext, temporary_context))
    {
        wglDeleteContext(temporary_context);
        CleanupDeviceWGL(windowHandle, windowData);
        return FALSE;
    }

    major = 0;
    minor = 0;
    glGetIntegerv(OPENGL_MAJOR_VERSION_PARAMETER, &major);
    glGetIntegerv(OPENGL_MINOR_VERSION_PARAMETER, &minor);
    version_string = reinterpret_cast<const char_t*>(glGetString(GL_VERSION));
    if ((0 == major) && (0 == minor) && (NULL != version_string))
    {
        sscanf_s(version_string, "%d.%d", &major, &minor);
    }
    version         = static_cast<GLuint>(major * 100 + minor * 10);
    g_renderContext = temporary_context;
    if (OPENGL_VERSION_3_0 <= version)
    {
        return TRUE;
    }

    create_context_attribs = reinterpret_cast<WglCreateContextAttribsFunction_t>(wglGetProcAddress("wglCreateContextAttribsARB"));
    context_attributes[0]  = WGL_MAJOR_VERSION_ATTRIBUTE;
    context_attributes[1]  = 3;
    context_attributes[2]  = WGL_MINOR_VERSION_ATTRIBUTE;
    context_attributes[3]  = 0;
    context_attributes[4]  = WGL_PROFILE_MASK_ATTRIBUTE;
    context_attributes[5]  = WGL_CORE_PROFILE_BIT;
    context_attributes[6]  = 0;
    new_context            = NULL;
    if (NULL != create_context_attribs)
    {
        new_context = create_context_attribs(windowData->deviceContext, NULL, context_attributes);
    }
    if (NULL != new_context)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(temporary_context);
        g_renderContext = new_context;
        wglMakeCurrent(windowData->deviceContext, g_renderContext);
    }
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Releases a Win32 window device context.
 */
static void CleanupDeviceWGL(HWND windowHandle, WglWindowData_t* windowData)
{
    if (NULL != windowData->deviceContext)
    {
        wglMakeCurrent(NULL, NULL);
        ::ReleaseDC(windowHandle, windowData->deviceContext);
        windowData->deviceContext = NULL;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Creates renderer data for an ImGui platform viewport.
 */
static void HookRendererCreateWindow(ImGuiViewport* viewport)
{
    WglWindowData_t* window_data;

    LOGASSERT(NULL == viewport->RendererUserData);
    window_data = IM_NEW(WglWindowData_t);
    LOGASSERT(TRUE == CreateDeviceWGL(static_cast<HWND>(viewport->PlatformHandle), window_data));
    viewport->RendererUserData = window_data;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Releases renderer data for an ImGui platform viewport.
 */
static void HookRendererDestroyWindow(ImGuiViewport* viewport)
{
    WglWindowData_t* window_data;

    if (NULL != viewport->RendererUserData)
    {
        window_data = static_cast<WglWindowData_t*>(viewport->RendererUserData);
        CleanupDeviceWGL(static_cast<HWND>(viewport->PlatformHandle), window_data);
        IM_DELETE(window_data);
        viewport->RendererUserData = NULL;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Activates the shared OpenGL context for a platform viewport.
 */
static void HookPlatformRenderWindow(ImGuiViewport* viewport, void* data)
{
    WglWindowData_t* window_data;

    (void)data;
    window_data = static_cast<WglWindowData_t*>(viewport->RendererUserData);
    if (NULL != window_data)
    {
        wglMakeCurrent(window_data->deviceContext, g_renderContext);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Presents a platform viewport.
 */
static void HookRendererSwapBuffers(ImGuiViewport* viewport, void* data)
{
    WglWindowData_t* window_data;

    (void)data;
    window_data = static_cast<WglWindowData_t*>(viewport->RendererUserData);
    if (NULL != window_data)
    {
        ::SwapBuffers(window_data->deviceContext);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Routes Win32 input and window events to ImGui and the renderer.
 */
static LRESULT WINAPI WindowProcedure(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter)
{
    if (0 != ImGui_ImplWin32_WndProcHandler(windowHandle, message, wordParameter, longParameter))
    {
        return TRUE;
    }
    switch (message)
    {
    case WM_SIZE:
    {
        if (SIZE_MINIMIZED != wordParameter)
        {
            g_renderWidth  = LOWORD(longParameter);
            g_renderHeight = HIWORD(longParameter);
        }
        return 0;
    }
    case WM_SYSCOMMAND:
    {
        if (SC_KEYMENU == (wordParameter & 0xfff0))
        {
            return 0;
        }
        break;
    }
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        return 0;
    }
    default:
    {
        break;
    }
    }
    return ::DefWindowProcW(windowHandle, message, wordParameter, longParameter);
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Opens the configuration editor and its live screen visualizer.
 */
int main(void)
{
    float                 main_scale;
    WNDCLASSEXW           window_class;
    HWND                  window_handle;
    ImGuiIO*              io;
    ImGuiStyle*           style;
    ImGuiPlatformIO*      platform_io;
    ConfigurationEditor_t configuration_editor;
    bool_t                is_done;
    MSG                   message;

    ImGui_ImplWin32_EnableDpiAwareness();
    main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    memset(&window_class, 0, sizeof(window_class));
    window_class.cbSize        = sizeof(window_class);
    window_class.style         = CS_OWNDC;
    window_class.lpfnWndProc   = WindowProcedure;
    window_class.hInstance     = GetModuleHandle(NULL);
    window_class.lpszClassName = L"ARINC661Configuration";
    ::RegisterClassExW(&window_class);
    window_handle = ::CreateWindowW(window_class.lpszClassName, L"ARINC 661 Configuration", WS_OVERLAPPEDWINDOW,
                                    100, 100, static_cast<int>(1280 * main_scale), static_cast<int>(800 * main_scale),
                                    NULL, NULL, window_class.hInstance, NULL);
    if (FALSE == CreateDeviceWGL(window_handle, &g_mainWindow))
    {
        CleanupDeviceWGL(window_handle, &g_mainWindow);
        ::DestroyWindow(window_handle);
        ::UnregisterClassW(window_class.lpszClassName, window_class.hInstance);
        return 1;
    }
    wglMakeCurrent(g_mainWindow.deviceContext, g_renderContext);
    ::ShowWindow(window_handle, SW_SHOWDEFAULT);
    ::UpdateWindow(window_handle);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    style = &ImGui::GetStyle();
    style->ScaleAllSizes(main_scale);
    style->FontScaleDpi         = main_scale;
    io->ConfigDpiScaleFonts     = TRUE;
    io->ConfigDpiScaleViewports = TRUE;
    if (0 != (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        style->WindowRounding              = 0.0f;
        style->Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_InitForOpenGL(window_handle);
    ImGui_ImplOpenGL3_Init();
    if (0 != (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        platform_io = &ImGui::GetPlatformIO();
        LOGASSERT(NULL == platform_io->Renderer_CreateWindow);
        LOGASSERT(NULL == platform_io->Renderer_DestroyWindow);
        LOGASSERT(NULL == platform_io->Renderer_SwapBuffers);
        LOGASSERT(NULL == platform_io->Platform_RenderWindow);
        platform_io->Renderer_CreateWindow  = HookRendererCreateWindow;
        platform_io->Renderer_DestroyWindow = HookRendererDestroyWindow;
        platform_io->Renderer_SwapBuffers   = HookRendererSwapBuffers;
        platform_io->Platform_RenderWindow  = HookPlatformRenderWindow;
    }

    configuration_editor.isLayoutInitialized = FALSE;
    LoadConfiguration(&configuration_editor);
    is_done = FALSE;
    while (FALSE == is_done)
    {
        while (::PeekMessage(&message, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
            if (WM_QUIT == message.message)
            {
                is_done = TRUE;
            }
        }
        if (TRUE == is_done)
        {
            break;
        }
        if (::IsIconic(window_handle))
        {
            ::Sleep(10);
            continue;
        }

        BeginFrame();

        DrawApp(&configuration_editor);
        
        EndFrame();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceWGL(window_handle, &g_mainWindow);
    wglDeleteContext(g_renderContext);
    ::DestroyWindow(window_handle);
    ::UnregisterClassW(window_class.lpszClassName, window_class.hInstance);
    return 0;
}
