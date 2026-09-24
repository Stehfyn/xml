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
#include <fstream>
#include <iterator>
#include <limits>
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
#define VISUALIZER_WINDOW_NAME    ("Visualizer")
#define DOCKSPACE_NAME            ("ARINC661DockSpace")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

typedef std::unordered_map<Utl_Xml_Attribute_t*, std::string> AttributeValues_t;

/*************************************************************************************************/
/**
 * \brief Parsed configuration and editable copies of its attribute values.
 */
typedef struct ConfigurationEditor_s
{
    std::vector<byte_t>               inputBuffer;
    std::vector<Utl_Xml_Node_t>       nodes;
    std::vector<Utl_Xml_Attribute_t>  attributes;
    Utl_Xml_Document_t                document;
    AttributeValues_t                 attributeValues;
    Utl_Xml_Node_t*                   selectedNode;
    std::string                       status;
    bool_t                            isLoaded;
} ConfigurationEditor_t;

/*************************************************************************************************/
/**
 * \brief Screen dimensions read from the editable XML tree.
 */
typedef struct ScreenPreview_s
{
    Utl_Xml_Node_t* node;
    float           width;
    float           height;
} ScreenPreview_t;

/*************************************************************************************************/
/**
 * \brief Device context owned by an ImGui platform viewport.
 */
typedef struct WglWindowData_s
{
    HDC deviceContext;
} WglWindowData_t;

typedef HGLRC(WINAPI* WglCreateContextAttribsFunction_t)(HDC, HGLRC, const int*);

/*=================================================================================================
** 3.3 Static global variables
**===============================================================================================*/

static HGLRC           g_renderContext;
static WglWindowData_t g_mainWindow;
static int             g_renderWidth;
static int             g_renderHeight;
static const char_t* const c_unitOptions[] = {"pixel", "inch", "screen"};

/*=================================================================================================
** 3.4 Static function prototypes
**===============================================================================================*/

static void CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);
static void LoadConfiguration(ConfigurationEditor_t* editor);
static bool_t AddFormattingHeadroom(const Utl_Xml_Node_t* node, size_t depth, size_t* capacity);
static void BuildConfiguration(ConfigurationEditor_t* editor);
static bool_t IsHierarchyNode(const Utl_Xml_Node_t* node);
static void DrawConfigurationNode(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node);
static bool_t AttributeHasName(const Utl_Xml_Attribute_t* attribute, const char_t* name);
static bool_t ParseUnsignedValue(const std::string* text, uint32_t* value);
static void DrawAttributeControl(const Utl_Xml_Node_t* ownerNode, Utl_Xml_Attribute_t* attribute, std::string* value);
static void DrawAttributeSection(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* heading);
static void DrawSelectedNodeInspector(ConfigurationEditor_t* editor);
static void DrawConfigurationEditor(ConfigurationEditor_t* editor);
static bool_t NodeHasName(const Utl_Xml_Node_t* node, const char_t* name);
static Utl_Xml_Node_t* FindChild(Utl_Xml_Node_t* parent, const char_t* name);
static const std::string* FindAttributeValue(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name);
static bool_t ParsePixelValue(const std::string* text, float* value);
static bool_t ReadPixelRectangle(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, float* x, float* y, float* width, float* height);
static void DrawScreenWindows(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen, ImDrawList* drawList, const ImVec2* origin, float scale);
static void DrawVisualizer(const ConfigurationEditor_t* editor);
static void BuildDockLayout(ImGuiID dockspaceId);
static bool_t CreateDeviceWGL(HWND windowHandle, WglWindowData_t* windowData);
static void CleanupDeviceWGL(HWND windowHandle, WglWindowData_t* windowData);
static void HookRendererCreateWindow(ImGuiViewport* viewport);
static void HookRendererDestroyWindow(ImGuiViewport* viewport);
static void HookPlatformRenderWindow(ImGuiViewport* viewport, void* data);
static void HookRendererSwapBuffers(ImGuiViewport* viewport, void* data);
static LRESULT WINAPI WindowProcedure(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND windowHandle, UINT message, WPARAM wordParameter, LPARAM longParameter);

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Copies parsed attribute values into editable storage.
 */
static void CollectAttributes(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t* attribute;
    Utl_Xml_Node_t* child;

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
 * \brief Reads the configuration through Utl_Xml_ReadDocument.
 */
static void LoadConfiguration(ConfigurationEditor_t* editor)
{
    std::ifstream xml_file;
    std::string xml_text;
    size_t tag_count;
    size_t equals_count;
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* first_screen;

    editor->isLoaded = FALSE;
    editor->selectedNode = NULL;
    editor->attributeValues.clear();
    editor->nodes.clear();
    editor->attributes.clear();
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

    tag_count = std::count(xml_text.begin(), xml_text.end(), '<');
    equals_count = std::count(xml_text.begin(), xml_text.end(), '=');
    if ((0 == tag_count) || (UTL_XML_DOCUMENT_MAX_NODES < tag_count) || (std::numeric_limits<uint32_t>::max() <= equals_count))
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
    editor->status = "Loaded arinc661_configuration.xml.";
}

/*************************************************************************************************/
/**
 * \brief Reserves space for the writer's indentation and line breaks.
 */
static bool_t AddFormattingHeadroom(const Utl_Xml_Node_t* node, size_t depth, size_t* capacity)
{
    const Utl_Xml_Node_t* child;
    size_t extra;

    extra = 8 * depth + 64;
    if (extra > std::numeric_limits<size_t>::max() - *capacity)
    {
        return FALSE;
    }
    *capacity += extra;
    for (child = node->firstChild; NULL != child; child = child->nextSibling)
    {
        if (FALSE == AddFormattingHeadroom(child, depth + 1, capacity))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*************************************************************************************************/
/**
 * \brief Validates edited values and writes the document with Utl_Xml_WriteDocument.
 */
static void BuildConfiguration(ConfigurationEditor_t* editor)
{
    AttributeValues_t::iterator value_iterator;
    const std::string* value;
    const Utl_Xml_Attribute_t* attribute;
    const Utl_Xml_Node_t* node;
    std::vector<byte_t> output_buffer;
    std::ofstream xml_file;
    size_t output_capacity;
    uint64_t output_byte_count;

    output_capacity = editor->inputBuffer.size();
    for (value_iterator = editor->attributeValues.begin(); value_iterator != editor->attributeValues.end(); ++value_iterator)
    {
        value = &value_iterator->second;
        if ((UTL_XML_MAX_VALUE_BYTES < value->size()) ||
            (FALSE == Utl_Xml_IsAttributeValue(reinterpret_cast<const byte_t*>(value->c_str()), static_cast<uint32_t>(value->size()))))
        {
            attribute = value_iterator->first;
            editor->status = "Invalid XML attribute value: " +
                std::string(reinterpret_cast<const char_t*>(attribute->name), attribute->nameByteCount);
            return;
        }
        if (value->size() > std::numeric_limits<size_t>::max() - output_capacity)
        {
            editor->status = "The XML output is too large.";
            return;
        }
        output_capacity += value->size();
    }

    for (node = editor->document.firstNode; NULL != node; node = node->nextSibling)
    {
        if (FALSE == AddFormattingHeadroom(node, 0, &output_capacity))
        {
            editor->status = "The XML output is too large.";
            return;
        }
    }
    if (output_capacity > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
    {
        editor->status = "The XML output is too large.";
        return;
    }

    for (value_iterator = editor->attributeValues.begin(); value_iterator != editor->attributeValues.end(); ++value_iterator)
    {
        value_iterator->first->value = reinterpret_cast<const byte_t*>(value_iterator->second.c_str());
        value_iterator->first->valueByteCount = static_cast<uint32_t>(value_iterator->second.size());
    }
    output_buffer.resize(output_capacity);
    output_byte_count = Utl_Xml_WriteDocument(&editor->document, output_buffer.data(), static_cast<uint64_t>(output_buffer.size()));

    xml_file.open(ARINC661_CONFIGURATION_XML_PATH, std::ios::binary | std::ios::trunc);
    if (!xml_file || !xml_file.write(reinterpret_cast<const char_t*>(output_buffer.data()), static_cast<std::streamsize>(output_byte_count)))
    {
        editor->status = "Could not write arinc661_configuration.xml.";
        return;
    }
    xml_file.flush();
    if (!xml_file)
    {
        editor->status = "Could not finish writing arinc661_configuration.xml.";
        return;
    }
    editor->status = "Built arinc661_configuration.xml.";
}

/*************************************************************************************************/
/**
 * \brief Includes only screens, layouts, and windows in the hierarchy.
 */
static bool_t IsHierarchyNode(const Utl_Xml_Node_t* node)
{
    return (TRUE == NodeHasName(node, "screen")) ||
           (TRUE == NodeHasName(node, "layout")) ||
           (TRUE == NodeHasName(node, "window"));
}

/*************************************************************************************************/
/**
 * \brief Shows one element in the configuration hierarchy.
 */
static void DrawConfigurationNode(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node)
{
    Utl_Xml_Node_t* child;
    const std::string* identifier;
    std::string node_name;
    std::string label;
    ImGuiTreeNodeFlags flags;
    bool_t is_open;
    bool_t has_hierarchy_child;

    if (FALSE == IsHierarchyNode(node))
    {
        return;
    }

    node_name.assign(reinterpret_cast<const char_t*>(node->name), node->nameByteCount);
    label = node_name;
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
 * \brief Compares a counted attribute name with a literal.
 */
static bool_t AttributeHasName(const Utl_Xml_Attribute_t* attribute, const char_t* name)
{
    size_t name_byte_count;

    name_byte_count = strlen(name);
    return (attribute->nameByteCount == name_byte_count) && (0 == memcmp(attribute->name, name, name_byte_count));
}

/*************************************************************************************************/
/**
 * \brief Parses an unsigned integer for an inspector number field.
 */
static bool_t ParseUnsignedValue(const std::string* text, uint32_t* value)
{
    char_t* end;
    unsigned long long parsed_value;

    if (text->empty() || ('-' == (*text)[0]))
    {
        return FALSE;
    }
    errno = 0;
    parsed_value = std::strtoull(text->c_str(), &end, 10);
    if ((ERANGE == errno) || ('\0' != *end) || (std::numeric_limits<uint32_t>::max() < parsed_value))
    {
        return FALSE;
    }
    *value = static_cast<uint32_t>(parsed_value);
    return TRUE;
}

/*************************************************************************************************/
/**
 * \brief Uses numeric fields and unit choices for known configuration attributes.
 */
static void DrawAttributeControl(const Utl_Xml_Node_t* ownerNode, Utl_Xml_Attribute_t* attribute, std::string* value)
{
    uint32_t integer_value;
    float float_value;
    char_t formatted_value[64];
    size_t option_index;
    bool_t is_selected;

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
 * \brief Shows a node's attributes as one two-column property group.
 */
static void DrawAttributeSection(ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* heading)
{
    Utl_Xml_Attribute_t* attribute;
    std::string attribute_name;

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
 * \brief Groups screen and window details under their owning selection.
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
 * \brief Shows a hierarchy above the selected element's inspector.
 */
static void DrawConfigurationEditor(ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* node;
    Utl_Xml_Node_t* root;
    ImVec2 available_size;
    float hierarchy_height;

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
        available_size = ImGui::GetContentRegionAvail();
        hierarchy_height = (std::max)(150.0f, available_size.y * 0.36f);
        ImGui::TextUnformatted("Hierarchy");
        ImGui::BeginChild("Hierarchy", ImVec2(0.0f, hierarchy_height), true);
        root = editor->document.firstNode;
        while ((NULL != root) && (UTL_XML_NODE_ELEMENT != root->type))
        {
            root = root->nextSibling;
        }
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
 * \brief Compares a counted XML node name with a literal.
 */
static bool_t NodeHasName(const Utl_Xml_Node_t* node, const char_t* name)
{
    size_t name_byte_count;

    name_byte_count = strlen(name);
    return (node->nameByteCount == name_byte_count) && (0 == memcmp(node->name, name, name_byte_count));
}

/*************************************************************************************************/
/**
 * \brief Finds a direct child with the requested element name.
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
 * \brief Finds an edited value by its XML attribute name.
 */
static const std::string* FindAttributeValue(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* attribute;
    size_t name_byte_count;

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
 * \brief Reads a nonnegative finite pixel coordinate from an edited value.
 */
static bool_t ParsePixelValue(const std::string* text, float* value)
{
    char_t* end;
    float parsed_value;

    if ((NULL == text) || text->empty())
    {
        return FALSE;
    }
    errno = 0;
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
 * \brief Reads a window pixel area from the current editor values.
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
 * \brief Draws configured windows inside a screen rectangle.
 */
static void DrawScreenWindows(const ConfigurationEditor_t* editor, Utl_Xml_Node_t* screen, ImDrawList* drawList, const ImVec2* origin, float scale)
{
    Utl_Xml_Node_t* layout;
    Utl_Xml_Node_t* window;
    Utl_Xml_Node_t* pixel_area;
    const std::string* window_name;
    ImVec2 area_min;
    ImVec2 area_max;
    float x;
    float y;
    float width;
    float height;

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
 * \brief Shows screen sizes and window pixel areas from the current field values.
 */
static void DrawVisualizer(const ConfigurationEditor_t* editor)
{
    Utl_Xml_Node_t* root;
    Utl_Xml_Node_t* screen;
    Utl_Xml_Node_t* pixel_size;
    const std::string* screen_id;
    std::vector<ScreenPreview_t> screens;
    ScreenPreview_t preview;
    ImDrawList* draw_list;
    ImVec2 canvas_min;
    ImVec2 canvas_size;
    ImVec2 screen_min;
    ImVec2 screen_max;
    std::string screen_label;
    float total_width;
    float maximum_height;
    float available_width;
    float available_height;
    float scale;
    float x;
    float y;
    size_t screen_index;

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

    total_width = 0.0f;
    maximum_height = 0.0f;
    for (screen = root->firstChild; NULL != screen; screen = screen->nextSibling)
    {
        if (FALSE == NodeHasName(screen, "screen"))
        {
            continue;
        }
        pixel_size = FindChild(screen, "pixelSize");
        preview.node = screen;
        preview.width = 0.0f;
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

    canvas_min = ImGui::GetCursorScreenPos();
    canvas_size = ImGui::GetContentRegionAvail();
    available_width = canvas_size.x - 40.0f - 20.0f * static_cast<float>(screens.size() - 1);
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
    x = canvas_min.x + 20.0f;
    y = canvas_min.y + 35.0f + (available_height - maximum_height * scale) * 0.5f;
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
 * \brief Places the configuration on the left and the visualizer in the center.
 */
static void BuildDockLayout(ImGuiID dockspaceId)
{
    const ImGuiViewport* viewport;
    ImGuiID configuration_node_id;
    ImGuiID visualizer_node_id;

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
 * \brief Creates a Win32 OpenGL context for the main or platform window.
 */
static bool_t CreateDeviceWGL(HWND windowHandle, WglWindowData_t* windowData)
{
    HDC device_context;
    PIXELFORMATDESCRIPTOR pixel_format_description;
    int pixel_format;
    HGLRC temporary_context;
    HGLRC new_context;
    GLint major;
    GLint minor;
    const char_t* version_string;
    GLuint version;
    WglCreateContextAttribsFunction_t create_context_attribs;
    int context_attributes[7];

    windowData->deviceContext = NULL;
    device_context = ::GetDC(windowHandle);
    if (NULL == device_context)
    {
        return FALSE;
    }
    memset(&pixel_format_description, 0, sizeof(pixel_format_description));
    pixel_format_description.nSize = sizeof(pixel_format_description);
    pixel_format_description.nVersion = 1;
    pixel_format_description.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
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
    glGetIntegerv(0x821B, &major);
    glGetIntegerv(0x821C, &minor);
    version_string = reinterpret_cast<const char_t*>(glGetString(GL_VERSION));
    if ((0 == major) && (0 == minor) && (NULL != version_string))
    {
        sscanf_s(version_string, "%d.%d", &major, &minor);
    }
    version = static_cast<GLuint>(major * 100 + minor * 10);
    g_renderContext = temporary_context;
    if (300 <= version)
    {
        return TRUE;
    }

    create_context_attribs = reinterpret_cast<WglCreateContextAttribsFunction_t>(wglGetProcAddress("wglCreateContextAttribsARB"));
    context_attributes[0] = 0x2091;
    context_attributes[1] = 3;
    context_attributes[2] = 0x2092;
    context_attributes[3] = 0;
    context_attributes[4] = 0x9126;
    context_attributes[5] = 0x0001;
    context_attributes[6] = 0;
    new_context = NULL;
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
 * \brief Releases a Win32 window device context.
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
 * \brief Creates renderer data for an ImGui platform viewport.
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
 * \brief Releases renderer data for an ImGui platform viewport.
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
 * \brief Activates the shared OpenGL context for a platform viewport.
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
 * \brief Presents a platform viewport.
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
 * \brief Routes Win32 input and window events to ImGui and the renderer.
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
                g_renderWidth = LOWORD(longParameter);
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
 * \brief Opens the configuration editor and its live screen visualizer.
 */
int main(void)
{
    float main_scale;
    WNDCLASSEXW window_class;
    HWND window_handle;
    ImGuiIO* io;
    ImGuiStyle* style;
    ImGuiPlatformIO* platform_io;
    ImVec4 clear_color;
    ConfigurationEditor_t configuration_editor;
    bool_t is_layout_initialized;
    bool_t is_done;
    MSG message;
    ImGuiID dockspace_id;

    ImGui_ImplWin32_EnableDpiAwareness();
    main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

    memset(&window_class, 0, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = WindowProcedure;
    window_class.hInstance = GetModuleHandle(NULL);
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
    style->FontScaleDpi = main_scale;
    io->ConfigDpiScaleFonts = TRUE;
    io->ConfigDpiScaleViewports = TRUE;
    if (0 != (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        style->WindowRounding = 0.0f;
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
        platform_io->Renderer_CreateWindow = HookRendererCreateWindow;
        platform_io->Renderer_DestroyWindow = HookRendererDestroyWindow;
        platform_io->Renderer_SwapBuffers = HookRendererSwapBuffers;
        platform_io->Platform_RenderWindow = HookPlatformRenderWindow;
    }

    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    LoadConfiguration(&configuration_editor);
    is_layout_initialized = FALSE;
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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        dockspace_id = ImHashStr(DOCKSPACE_NAME);
        if (FALSE == is_layout_initialized)
        {
            BuildDockLayout(dockspace_id);
            is_layout_initialized = TRUE;
        }
        ImGui::DockSpaceOverViewport(dockspace_id);
        DrawConfigurationEditor(&configuration_editor);
        DrawVisualizer(&configuration_editor);

        ImGui::Render();
        glViewport(0, 0, g_renderWidth, g_renderHeight);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (0 != (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            wglMakeCurrent(g_mainWindow.deviceContext, g_renderContext);
        }
        ::SwapBuffers(g_mainWindow.deviceContext);
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
