/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * FACE Technical Standard, Appendix H.3, DisplayManagement.xsd:
 * https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 */

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

#include <limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define CONFIGURATION_WINDOW_NAME ("ARINC 661 Configuration")
#define VISUALIZER_WINDOW_NAME    ("Visualizer")
#define DOCKSPACE_NAME            ("ARINC661DockSpace")

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
 * \brief FACE DisplayManagement.xsd unit spellings in enum order.
 */
static const char_t* const c_unitNames[] =
{
    "inch",
    "mm",
    "screen",
    "pixel"
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Reads example file bytes through the in-memory ARINC 661 reader.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void ReadConfigurationFile(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Writes the in-memory display configuration to the example file.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void BuildConfigurationFile(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Chooses the next screen identifier from current screen values.
 *
 * \param[in]  editor Editor state
 * \param[out] id     Next available identifier
 *
 * \retval TRUE  An identifier is available.
 * \retval FALSE The identifier range is exhausted.
 */
static bool_t NextScreenId(const ConfigurationEditor_t* editor, int64_t* id);

/*************************************************************************************************/
/**
 * \brief Chooses the next layout identifier on one screen.
 *
 * \param[in]  editor      Editor state
 * \param[in]  screenIndex Owning screen index
 * \param[out] id          Next available identifier
 *
 * \retval TRUE  An identifier is available.
 * \retval FALSE The identifier range is exhausted.
 */
static bool_t NextLayoutId(const ConfigurationEditor_t* editor, uint32_t screenIndex, int64_t* id);

/*************************************************************************************************/
/**
 * \brief Chooses the next window identifier from current window values.
 *
 * \param[in]  editor Editor state
 * \param[out] id     Next available identifier
 *
 * \retval TRUE  An identifier is available.
 * \retval FALSE The identifier range is exhausted.
 */
static bool_t NextWindowId(const ConfigurationEditor_t* editor, int64_t* id);

/*************************************************************************************************/
/**
 * \brief Appends a screen with default dimensions.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void AddScreen(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Appends a layout to the selected screen.
 *
 * \param[in,out] editor      Editor state
 * \param[in]     screenIndex Owning screen index
 *
 * \retval None
 */
static void AddLayout(ConfigurationEditor_t* editor, uint32_t screenIndex);

/*************************************************************************************************/
/**
 * \brief Appends a window to the selected layout.
 *
 * \param[in,out] editor      Editor state
 * \param[in]     layoutIndex Owning layout index
 *
 * \retval None
 */
static void AddWindow(ConfigurationEditor_t* editor, uint32_t layoutIndex);

/*************************************************************************************************/
/**
 * \brief Removes one window and updates its array.
 *
 * \param[in,out] editor      Editor state
 * \param[in]     windowIndex Window index to remove
 *
 * \retval None
 */
static void RemoveWindow(ConfigurationEditor_t* editor, uint32_t windowIndex);

/*************************************************************************************************/
/**
 * \brief Removes one layout and all of its windows.
 *
 * \param[in,out] editor      Editor state
 * \param[in]     layoutIndex Layout index to remove
 *
 * \retval None
 */
static void RemoveLayout(ConfigurationEditor_t* editor, uint32_t layoutIndex);

/*************************************************************************************************/
/**
 * \brief Removes one screen and all of its layouts and windows.
 *
 * \param[in,out] editor      Editor state
 * \param[in]     screenIndex Screen index to remove
 *
 * \retval None
 */
static void RemoveScreen(ConfigurationEditor_t* editor, uint32_t screenIndex);

/*************************************************************************************************/
/**
 * \brief Draws the screen, layout, and window selection tree.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawHierarchy(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws one signed integer in the two-column inspector.
 *
 * \param[in]     label Visible field label
 * \param[in,out] value Integer value
 *
 * \retval TRUE  The value changed.
 * \retval FALSE The value did not change.
 */
static bool_t DrawIntegerField(const char_t* label, int64_t* value);

/*************************************************************************************************/
/**
 * \brief Draws one decimal in the two-column inspector.
 *
 * \param[in]     label Visible field label
 * \param[in,out] value Decimal value
 *
 * \retval TRUE  The value changed.
 * \retval FALSE The value did not change.
 */
static bool_t DrawDecimalField(const char_t* label, float32_t* value);

/*************************************************************************************************/
/**
 * \brief Draws one short text value in the two-column inspector.
 *
 * \param[in]     label    Visible field label
 * \param[in,out] value    Terminated text storage
 * \param[in]     capacity Number of text bytes
 *
 * \retval TRUE  The value changed.
 * \retval FALSE The value did not change.
 */
static bool_t DrawTextField(const char_t* label, char_t* value, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Draws a unit value as a choice field.
 *
 * \param[in]     label Visible field label
 * \param[in,out] unit  Unit value
 *
 * \retval TRUE  The value changed.
 * \retval FALSE The value did not change.
 */
static bool_t DrawUnitField(const char_t* label, Utl_Arinc661_Unit_t* unit);

/*************************************************************************************************/
/**
 * \brief Draws the selected screen's properties.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawScreenInspector(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected layout's properties.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawLayoutInspector(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected window's properties.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawWindowInspector(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the side configuration window.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawConfigurationEditor(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws screen and window geometry in the center panel.
 *
 * \param[in] editor Editor state
 *
 * \retval None
 */
static void DrawVisualizer(const ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Docks the configuration on the side and the visualizer centrally.
 *
 * \param[in] dockspaceId ImGui dockspace identifier
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
 *  Reads file bytes only; utl_arinc661 owns XML interpretation.
 */
static void ReadConfigurationFile(ConfigurationEditor_t* editor)
{
    FILE* xml_file;
    uint64_t document_byte_count;
    bool_t is_configuration;

    editor->isLoaded = FALSE;
    editor->selectedKind = CONFIGURATION_SELECTION_NONE;
    editor->configuration.screenCount = 0;
    editor->configuration.layoutCount = 0;
    editor->configuration.windowCount = 0;
    editor->configuration.userApplicationCount = 0;
    editor->configuration.externalSourceCount = 0;
    editor->configuration.hasMetaData = FALSE;
    xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "rb");
    if (NULL == xml_file)
    {
        strcpy(editor->status, "Could not open arinc661_configuration.xml.");
    }
    else
    {
        document_byte_count = fread(editor->inputBuffer, 1, CONFIGURATION_MAX_DOCUMENT_BYTES + 1, xml_file);
        fclose(xml_file);
        if (CONFIGURATION_MAX_DOCUMENT_BYTES < document_byte_count)
        {
            strcpy(editor->status, "The XML file exceeds the input buffer.");
        }
        else if (0 == document_byte_count)
        {
            strcpy(editor->status, "The XML file is empty.");
        }
        else
        {
            is_configuration = Utl_Arinc661_ReadDisplayManagementConfiguration(
                &editor->configuration,
                editor->inputBuffer,
                document_byte_count,
                editor->workspace,
                sizeof(editor->workspace));
            if (is_configuration)
            {
                editor->isLoaded = TRUE;
                editor->isPreviewDirty = TRUE;
                if (0 < editor->configuration.screenCount)
                {
                    editor->selectedKind = CONFIGURATION_SELECTION_SCREEN;
                    editor->selectedIndex = 0;
                }
                strcpy(editor->status, "Loaded arinc661_configuration.xml.");
            }
            else
            {
                strcpy(editor->status, "Invalid or unsupported display configuration XML.");
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps file output outside the in-memory utl_arinc661 module.
 */
static void BuildConfigurationFile(ConfigurationEditor_t* editor)
{
    FILE* xml_file;
    uint64_t output_byte_count;
    uint64_t bytes_written;

    output_byte_count = Utl_Arinc661_WriteDisplayManagementConfiguration(
        &editor->configuration,
        editor->outputBuffer,
        sizeof(editor->outputBuffer),
        editor->workspace,
        sizeof(editor->workspace));
    if (0 == output_byte_count)
    {
        strcpy(editor->status, "Could not build the display configuration XML.");
    }
    else
    {
        xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "wb");
        if (NULL == xml_file)
        {
            strcpy(editor->status, "Could not write arinc661_configuration.xml.");
        }
        else
        {
            bytes_written = fwrite(editor->outputBuffer, 1, output_byte_count, xml_file);
            fclose(xml_file);
            if (output_byte_count == bytes_written)
            {
                strcpy(editor->status, "Built arinc661_configuration.xml.");
            }
            else
            {
                strcpy(editor->status, "Could not finish writing arinc661_configuration.xml.");
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Screen identifiers are selected from the typed values.
 */
static bool_t NextScreenId(const ConfigurationEditor_t* editor, int64_t* id)
{
    int64_t maximum_id;
    uint32_t screen_index;
    bool_t has_id;
    bool_t has_next_id;

    maximum_id = -1;
    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        if (maximum_id < editor->screens[screen_index].id)
        {
            maximum_id = editor->screens[screen_index].id;
        }
    }
    has_id = (0 <= maximum_id);
    has_next_id = (INT64_MAX != maximum_id);
    if (has_next_id)
    {
        (*id) = 0;
        if (has_id)
        {
            (*id) = maximum_id + 1;
        }
    }

    return has_next_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layout identifiers are local to their owning screen.
 */
static bool_t NextLayoutId(const ConfigurationEditor_t* editor, uint32_t screenIndex, int64_t* id)
{
    int64_t maximum_id;
    uint32_t layout_index;
    bool_t has_id;
    bool_t has_next_id;

    maximum_id = -1;
    for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
    {
        if (screenIndex == editor->layouts[layout_index].screenIndex)
        {
            if (maximum_id < editor->layouts[layout_index].id)
            {
                maximum_id = editor->layouts[layout_index].id;
            }
        }
    }
    has_id = (0 <= maximum_id);
    has_next_id = (INT64_MAX != maximum_id);
    if (has_next_id)
    {
        (*id) = 0;
        if (has_id)
        {
            (*id) = maximum_id + 1;
        }
    }

    return has_next_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Window identifiers are selected across the configuration.
 */
static bool_t NextWindowId(const ConfigurationEditor_t* editor, int64_t* id)
{
    int64_t maximum_id;
    uint32_t window_index;
    bool_t has_id;
    bool_t has_next_id;

    maximum_id = -1;
    for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
    {
        if (maximum_id < editor->windows[window_index].id)
        {
            maximum_id = editor->windows[window_index].id;
        }
    }
    has_id = (0 <= maximum_id);
    has_next_id = (INT64_MAX != maximum_id);
    if (has_next_id)
    {
        (*id) = 0;
        if (has_id)
        {
            (*id) = maximum_id + 1;
        }
    }

    return has_next_id;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  New screens receive pixel and physical dimensions that can be edited immediately.
 */
static void AddScreen(ConfigurationEditor_t* editor)
{
    Utl_Arinc661_Screen_t* screen;
    int64_t id;
    bool_t has_capacity;
    bool_t has_id;

    has_capacity = (editor->configuration.screenCount < editor->configuration.screenCapacity);
    has_id = NextScreenId(editor, &id);
    if (has_capacity && has_id)
    {
        screen = &editor->screens[editor->configuration.screenCount];
        memset(screen, 0, sizeof(*screen));
        screen->id = id;
        screen->pixelSize.width = 640;
        screen->pixelSize.height = 480;
        screen->pixelSize.units = UTL_ARINC661_UNIT_PIXEL;
        screen->physicalDimensions.width = 5;
        screen->physicalDimensions.height = 4;
        screen->physicalDimensions.units = UTL_ARINC661_UNIT_INCH;
        editor->selectedKind = CONFIGURATION_SELECTION_SCREEN;
        editor->selectedIndex = editor->configuration.screenCount;
        editor->configuration.screenCount += 1;
        editor->isPreviewDirty = TRUE;
        strcpy(editor->status, "Added screen. Build XML to save.");
    }
    else
    {
        strcpy(editor->status, "Could not add another screen.");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A layout stores its owning screen index and no XML location.
 */
static void AddLayout(ConfigurationEditor_t* editor, uint32_t screenIndex)
{
    Utl_Arinc661_Layout_t* layout;
    int64_t id;
    bool_t has_capacity;
    bool_t has_id;

    has_capacity = (editor->configuration.layoutCount < editor->configuration.layoutCapacity);
    has_id = NextLayoutId(editor, screenIndex, &id);
    if (has_capacity && has_id)
    {
        layout = &editor->layouts[editor->configuration.layoutCount];
        memset(layout, 0, sizeof(*layout));
        layout->id = id;
        layout->screenIndex = screenIndex;
        editor->selectedKind = CONFIGURATION_SELECTION_LAYOUT;
        editor->selectedIndex = editor->configuration.layoutCount;
        editor->configuration.layoutCount += 1;
        strcpy(editor->status, "Added layout. Build XML to save.");
    }
    else
    {
        strcpy(editor->status, "Could not add another layout.");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The window begins at the origin and uses its screen's current pixel size.
 */
static void AddWindow(ConfigurationEditor_t* editor, uint32_t layoutIndex)
{
    Utl_Arinc661_Window_t* window;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Screen_t* screen;
    int64_t id;
    bool_t has_capacity;
    bool_t has_id;

    has_capacity = (editor->configuration.windowCount < editor->configuration.windowCapacity);
    has_id = NextWindowId(editor, &id);
    if (has_capacity && has_id)
    {
        layout = &editor->layouts[layoutIndex];
        screen = &editor->screens[layout->screenIndex];
        window = &editor->windows[editor->configuration.windowCount];
        memset(window, 0, sizeof(*window));
        window->id = id;
        snprintf(window->name, sizeof(window->name), "window%" PRId64, id);
        window->layoutIndex = layoutIndex;
        window->pixelArea.width = screen->pixelSize.width;
        window->pixelArea.height = screen->pixelSize.height;
        window->scalingFactor.xScale = 20.0f;
        window->scalingFactor.yScale = 20.0f;
        window->scalingFactor.baseUnit = UTL_ARINC661_UNIT_SCREEN;
        window->scalingFactor.perUnit = UTL_ARINC661_UNIT_PIXEL;
        editor->selectedKind = CONFIGURATION_SELECTION_WINDOW;
        editor->selectedIndex = editor->configuration.windowCount;
        editor->configuration.windowCount += 1;
        editor->isPreviewDirty = TRUE;
        strcpy(editor->status, "Added window. Build XML to save.");
    }
    else
    {
        strcpy(editor->status, "Could not add another window.");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compacts the window array after the chosen entry.
 */
static void RemoveWindow(ConfigurationEditor_t* editor, uint32_t windowIndex)
{
    uint32_t remaining_count;

    LOGASSERT(windowIndex < editor->configuration.windowCount);
    remaining_count = (editor->configuration.windowCount - (windowIndex + 1));
    if (0 < remaining_count)
    {
        memmove(&editor->windows[windowIndex], &editor->windows[windowIndex + 1], (uint64_t)remaining_count * sizeof(editor->windows[0]));
    }
    editor->configuration.windowCount -= 1;
    editor->isPreviewDirty = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes associated windows before compacting layout indices.
 */
static void RemoveLayout(ConfigurationEditor_t* editor, uint32_t layoutIndex)
{
    uint32_t window_index;
    uint32_t remaining_count;

    LOGASSERT(layoutIndex < editor->configuration.layoutCount);
    window_index = 0;
    while (window_index < editor->configuration.windowCount)
    {
        if (layoutIndex == editor->windows[window_index].layoutIndex)
        {
            RemoveWindow(editor, window_index);
        }
        else
        {
            if (layoutIndex < editor->windows[window_index].layoutIndex)
            {
                editor->windows[window_index].layoutIndex -= 1;
            }
            window_index += 1;
        }
    }
    remaining_count = (editor->configuration.layoutCount - (layoutIndex + 1));
    if (0 < remaining_count)
    {
        memmove(&editor->layouts[layoutIndex], &editor->layouts[layoutIndex + 1], (uint64_t)remaining_count * sizeof(editor->layouts[0]));
    }
    editor->configuration.layoutCount -= 1;
    editor->isPreviewDirty = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Removes child layouts and windows before compacting screen indices.
 */
static void RemoveScreen(ConfigurationEditor_t* editor, uint32_t screenIndex)
{
    uint32_t layout_index;
    uint32_t remaining_count;

    LOGASSERT(screenIndex < editor->configuration.screenCount);
    layout_index = 0;
    while (layout_index < editor->configuration.layoutCount)
    {
        if (screenIndex == editor->layouts[layout_index].screenIndex)
        {
            RemoveLayout(editor, layout_index);
        }
        else
        {
            if (screenIndex < editor->layouts[layout_index].screenIndex)
            {
                editor->layouts[layout_index].screenIndex -= 1;
            }
            layout_index += 1;
        }
    }
    remaining_count = (editor->configuration.screenCount - (screenIndex + 1));
    if (0 < remaining_count)
    {
        memmove(&editor->screens[screenIndex], &editor->screens[screenIndex + 1], (uint64_t)remaining_count * sizeof(editor->screens[0]));
    }
    editor->configuration.screenCount -= 1;
    editor->isPreviewDirty = TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Iterates the typed parent indices; it does not inspect XML names or attributes.
 */
static void DrawHierarchy(ConfigurationEditor_t* editor)
{
    const Utl_Arinc661_Screen_t* screen;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Window_t* window;
    ImGuiTreeNodeFlags flags;
    uint32_t screen_index;
    uint32_t layout_index;
    uint32_t window_index;
    bool_t has_layouts;
    bool_t has_windows;
    bool_t is_screen_open;
    bool_t is_layout_open;

    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        screen = &editor->screens[screen_index];
        has_layouts = FALSE;
        for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
        {
            if (screen_index == editor->layouts[layout_index].screenIndex)
            {
                has_layouts = TRUE;
                break;
            }
        }
        flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
        if ((CONFIGURATION_SELECTION_SCREEN == editor->selectedKind) && (screen_index == editor->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (FALSE == has_layouts)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        ImGui::PushID((const void*)screen);
        is_screen_open = ImGui::TreeNodeEx("screen", flags, "Screen %" PRId64, screen->id);
        if (ImGui::IsItemClicked())
        {
            editor->selectedKind = CONFIGURATION_SELECTION_SCREEN;
            editor->selectedIndex = screen_index;
        }
        ImGui::PopID();
        if (is_screen_open && has_layouts)
        {
            for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
            {
                layout = &editor->layouts[layout_index];
                if (screen_index == layout->screenIndex)
                {
                    has_windows = FALSE;
                    for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                    {
                        if (layout_index == editor->windows[window_index].layoutIndex)
                        {
                            has_windows = TRUE;
                            break;
                        }
                    }
                    flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
                    if ((CONFIGURATION_SELECTION_LAYOUT == editor->selectedKind) && (layout_index == editor->selectedIndex))
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    if (FALSE == has_windows)
                    {
                        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    }
                    ImGui::PushID((const void*)layout);
                    is_layout_open = ImGui::TreeNodeEx("layout", flags, "Layout %" PRId64, layout->id);
                    if (ImGui::IsItemClicked())
                    {
                        editor->selectedKind = CONFIGURATION_SELECTION_LAYOUT;
                        editor->selectedIndex = layout_index;
                    }
                    ImGui::PopID();
                    if (is_layout_open && has_windows)
                    {
                        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                        {
                            window = &editor->windows[window_index];
                            if (layout_index == window->layoutIndex)
                            {
                                flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                                if ((CONFIGURATION_SELECTION_WINDOW == editor->selectedKind) && (window_index == editor->selectedIndex))
                                {
                                    flags |= ImGuiTreeNodeFlags_Selected;
                                }
                                ImGui::PushID((const void*)window);
                                ImGui::TreeNodeEx("window", flags, "Window %" PRId64 " (%s)", window->id, window->name);
                                if (ImGui::IsItemClicked())
                                {
                                    editor->selectedKind = CONFIGURATION_SELECTION_WINDOW;
                                    editor->selectedIndex = window_index;
                                }
                                ImGui::PopID();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws a label and a signed numeric input in the current two-column table.
 */
static bool_t DrawIntegerField(const char_t* label, int64_t* value)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputScalar("##value", ImGuiDataType_S64, value);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws a label and a floating-point input in the current two-column table.
 */
static bool_t DrawDecimalField(const char_t* label, float32_t* value)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputFloat("##value", value, 0.0f, 0.0f, "%.6g");
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Text input is used only for the spec's string-valued name field.
 */
static bool_t DrawTextField(const char_t* label, char_t* value, uint32_t capacity)
{
    bool_t has_changed;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    has_changed = ImGui::InputText("##value", value, capacity);
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Restricts the choice to the FACE Units enumeration.
 */
static bool_t DrawUnitField(const char_t* label, Utl_Arinc661_Unit_t* unit)
{
    uint32_t unit_index;
    bool_t is_selected;
    bool_t has_changed;

    has_changed = FALSE;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(label);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##value", c_unitNames[(uint32_t)(*unit)]))
    {
        for (unit_index = 0; unit_index < COUNT_OF(c_unitNames); ++unit_index)
        {
            is_selected = (unit_index == (uint32_t)(*unit));
            if (ImGui::Selectable(c_unitNames[unit_index], is_selected))
            {
                (*unit) = (Utl_Arinc661_Unit_t)unit_index;
                has_changed = TRUE;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();

    return has_changed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Screen pixel and physical dimensions remain grouped under their screen selection.
 */
static void DrawScreenInspector(ConfigurationEditor_t* editor)
{
    Utl_Arinc661_Screen_t* screen;
    bool_t has_changed;

    screen = &editor->screens[editor->selectedIndex];
    ImGui::TextUnformatted("Screen");
    if (ImGui::BeginTable("ScreenProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        DrawIntegerField("ID", &screen->id);
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Pixel Size");
    if (ImGui::BeginTable("PixelSizeProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        has_changed = DrawIntegerField("Width", &screen->pixelSize.width);
        if (has_changed)
        {
            editor->isPreviewDirty = TRUE;
        }
        has_changed = DrawIntegerField("Height", &screen->pixelSize.height);
        if (has_changed)
        {
            editor->isPreviewDirty = TRUE;
        }
        DrawUnitField("Units", &screen->pixelSize.units);
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Physical Dimensions");
    if (ImGui::BeginTable("PhysicalDimensionProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        DrawIntegerField("Width", &screen->physicalDimensions.width);
        DrawIntegerField("Height", &screen->physicalDimensions.height);
        DrawUnitField("Units", &screen->physicalDimensions.units);
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layouts have an identifier; their screen relationship is shown by the hierarchy.
 */
static void DrawLayoutInspector(ConfigurationEditor_t* editor)
{
    Utl_Arinc661_Layout_t* layout;

    layout = &editor->layouts[editor->selectedIndex];
    ImGui::TextUnformatted("Layout");
    if (ImGui::BeginTable("LayoutProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        DrawIntegerField("ID", &layout->id);
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Window geometry and scaling values are edited as typed fields.
 */
static void DrawWindowInspector(ConfigurationEditor_t* editor)
{
    Utl_Arinc661_Window_t* window;
    bool_t has_changed;

    window = &editor->windows[editor->selectedIndex];
    ImGui::TextUnformatted("Window");
    if (ImGui::BeginTable("WindowProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        DrawIntegerField("ID", &window->id);
        DrawTextField("Name", window->name, sizeof(window->name));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Pixel Area");
    if (ImGui::BeginTable("PixelAreaProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        has_changed = DrawIntegerField("X", &window->pixelArea.x);
        if (has_changed)
        {
            editor->isPreviewDirty = TRUE;
        }
        has_changed = DrawIntegerField("Y", &window->pixelArea.y);
        if (has_changed)
        {
            editor->isPreviewDirty = TRUE;
        }
        has_changed = DrawIntegerField("Width", &window->pixelArea.width);
        if (has_changed)
        {
            editor->isPreviewDirty = TRUE;
        }
        has_changed = DrawIntegerField("Height", &window->pixelArea.height);
        if (has_changed)
        {
            editor->isPreviewDirty = TRUE;
        }
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Scaling Factor");
    if (ImGui::BeginTable("ScalingFactorProperties", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        DrawDecimalField("X Scale", &window->scalingFactor.xScale);
        DrawDecimalField("Y Scale", &window->scalingFactor.yScale);
        DrawUnitField("Base Unit", &window->scalingFactor.baseUnit);
        DrawUnitField("Per Unit", &window->scalingFactor.perUnit);
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Selection controls mutate the typed configuration before subsequent frames are drawn.
 */
static void DrawConfigurationEditor(ConfigurationEditor_t* editor)
{
    ImVec2 available_size;
    float32_t hierarchy_height;
    uint32_t parent_index;

    if (ImGui::Begin(CONFIGURATION_WINDOW_NAME))
    {
        ImGui::TextUnformatted("arinc661_configuration.xml");
        if (ImGui::Button("Reload XML"))
        {
            ReadConfigurationFile(editor);
        }
        if (editor->isLoaded)
        {
            ImGui::SameLine();
            if (ImGui::Button("Build XML"))
            {
                BuildConfigurationFile(editor);
            }
        }
        ImGui::TextWrapped("%s", editor->status);
        if (editor->isLoaded)
        {
            ImGui::TextUnformatted("Hierarchy");
            if (ImGui::SmallButton("Add Screen"))
            {
                AddScreen(editor);
            }
            if (CONFIGURATION_SELECTION_SCREEN == editor->selectedKind)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Add Layout"))
                {
                    AddLayout(editor, editor->selectedIndex);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove Screen"))
                {
                    RemoveScreen(editor, editor->selectedIndex);
                    editor->selectedKind = CONFIGURATION_SELECTION_NONE;
                    strcpy(editor->status, "Removed screen. Build XML to save.");
                }
            }
            else if (CONFIGURATION_SELECTION_LAYOUT == editor->selectedKind)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Add Window"))
                {
                    AddWindow(editor, editor->selectedIndex);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove Layout"))
                {
                    parent_index = editor->layouts[editor->selectedIndex].screenIndex;
                    RemoveLayout(editor, editor->selectedIndex);
                    editor->selectedKind = CONFIGURATION_SELECTION_SCREEN;
                    editor->selectedIndex = parent_index;
                    strcpy(editor->status, "Removed layout. Build XML to save.");
                }
            }
            else if (CONFIGURATION_SELECTION_WINDOW == editor->selectedKind)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Add Window"))
                {
                    AddWindow(editor, editor->windows[editor->selectedIndex].layoutIndex);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove Window"))
                {
                    parent_index = editor->windows[editor->selectedIndex].layoutIndex;
                    RemoveWindow(editor, editor->selectedIndex);
                    editor->selectedKind = CONFIGURATION_SELECTION_LAYOUT;
                    editor->selectedIndex = parent_index;
                    strcpy(editor->status, "Removed window. Build XML to save.");
                }
            }
            available_size = ImGui::GetContentRegionAvail();
            hierarchy_height = available_size.y * 0.36f;
            if (150.0f > hierarchy_height)
            {
                hierarchy_height = 150.0f;
            }
            if (ImGui::BeginChild("Hierarchy", ImVec2(0.0f, hierarchy_height), TRUE))
            {
                DrawHierarchy(editor);
            }
            ImGui::EndChild();
            ImGui::TextUnformatted("Inspector");
            if (CONFIGURATION_SELECTION_SCREEN == editor->selectedKind)
            {
                DrawScreenInspector(editor);
            }
            else if (CONFIGURATION_SELECTION_LAYOUT == editor->selectedKind)
            {
                DrawLayoutInspector(editor);
            }
            else if (CONFIGURATION_SELECTION_WINDOW == editor->selectedKind)
            {
                DrawWindowInspector(editor);
            }
            else
            {
                ImGui::TextUnformatted("Select a screen, layout, or window.");
            }
        }
    }
    ImGui::End();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses only typed screen and window dimensions prepared by utl_arinc661.
 */
static void DrawVisualizer(const ConfigurationEditor_t* editor)
{
    const Utl_Arinc661_Screen_t* screen;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Window_t* window;
    ImDrawList* draw_list;
    ImVec2 canvas_min;
    ImVec2 canvas_size;
    ImVec2 screen_min;
    ImVec2 screen_max;
    ImVec2 area_min;
    ImVec2 area_max;
    char_t screen_label[64];
    float32_t available_width;
    float32_t available_height;
    float32_t scale;
    float32_t x;
    float32_t y;
    uint32_t screen_index;
    uint32_t window_index;
    bool_t has_canvas_width;
    bool_t has_canvas_height;
    bool_t has_canvas_space;
    bool_t has_layout;

    if (ImGui::Begin(VISUALIZER_WINDOW_NAME))
    {
        if (FALSE == editor->isLoaded)
        {
            ImGui::TextUnformatted(editor->status);
        }
        else if (0 == editor->drawableScreenCount)
        {
            ImGui::TextUnformatted("No screens with positive pixel dimensions.");
        }
        else
        {
            canvas_min = ImGui::GetCursorScreenPos();
            canvas_size = ImGui::GetContentRegionAvail();
            available_width = ((canvas_size.x - 40.0f) - (20.0f * (float32_t)(editor->drawableScreenCount - 1)));
            available_height = canvas_size.y - 60.0f;
            has_canvas_width = (0.0f < available_width);
            has_canvas_height = (0.0f < available_height);
            has_canvas_space = has_canvas_width && has_canvas_height;
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
                scale = available_width / editor->totalScreenWidth;
                if ((available_height / editor->maximumScreenHeight) < scale)
                {
                    scale = available_height / editor->maximumScreenHeight;
                }
                x = canvas_min.x + 20.0f;
                y = ((canvas_min.y + 35.0f) + ((available_height - (editor->maximumScreenHeight * scale)) * 0.5f));
                for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
                {
                    screen = &editor->screens[screen_index];
                    if ((0 < screen->pixelSize.width) && (0 < screen->pixelSize.height))
                    {
                        screen_min = ImVec2(x, y);
                        screen_max = ImVec2((x + ((float32_t)screen->pixelSize.width * scale)), (y + ((float32_t)screen->pixelSize.height * scale)));
                        draw_list->AddRectFilled(screen_min, screen_max, IM_COL32(42, 50, 64, 255));
                        draw_list->AddRect(screen_min, screen_max, IM_COL32(220, 230, 245, 255), 0.0f, 0, 2.0f);
                        snprintf(screen_label, sizeof(screen_label), "Screen %" PRId64, screen->id);
                        draw_list->AddText(ImVec2(x, y - 22.0f), IM_COL32(240, 248, 255, 255), screen_label);
                        draw_list->PushClipRect(screen_min, screen_max, TRUE);
                        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                        {
                            window = &editor->windows[window_index];
                            has_layout = (window->layoutIndex < editor->configuration.layoutCount);
                            if (has_layout)
                            {
                                layout = &editor->layouts[window->layoutIndex];
                                has_layout = (screen_index == layout->screenIndex);
                            }
                            if (has_layout)
                            {
                                area_min = ImVec2((screen_min.x + ((float32_t)window->pixelArea.x * scale)), (screen_min.y + ((float32_t)window->pixelArea.y * scale)));
                                area_max = ImVec2((area_min.x + ((float32_t)window->pixelArea.width * scale)), (area_min.y + ((float32_t)window->pixelArea.height * scale)));
                                draw_list->AddRectFilled(area_min, area_max, IM_COL32(42, 154, 188, 80));
                                draw_list->AddRect(area_min, area_max, IM_COL32(97, 221, 245, 255), 0.0f, 0, 2.0f);
                                if ('\0' != window->name[0])
                                {
                                    draw_list->AddText(ImVec2(area_min.x + 6.0f, area_min.y + 6.0f), IM_COL32(240, 248, 255, 255), window->name);
                                }
                            }
                        }
                        draw_list->PopClipRect();
                        x = screen_max.x + 20.0f;
                    }
                }
                draw_list->PopClipRect();
            }
        }
    }
    ImGui::End();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The configuration occupies the left dock and the visualizer the central dock.
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

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Binds fixed caller-owned arrays before reading the example file.
 */
void InitializeConfiguration(ConfigurationEditor_t* editor)
{
    editor->configuration.screens = editor->screens;
    editor->configuration.layouts = editor->layouts;
    editor->configuration.windows = editor->windows;
    editor->configuration.userApplications = editor->userApplications;
    editor->configuration.externalSources = editor->externalSources;
    editor->configuration.screenCapacity = CONFIGURATION_MAX_SCREENS;
    editor->configuration.layoutCapacity = CONFIGURATION_MAX_LAYOUTS;
    editor->configuration.windowCapacity = CONFIGURATION_MAX_WINDOWS;
    editor->configuration.userApplicationCapacity = CONFIGURATION_MAX_USER_APPLICATIONS;
    editor->configuration.externalSourceCapacity = CONFIGURATION_MAX_EXTERNAL_SOURCES;
    editor->isLayoutInitialized = FALSE;
    editor->isPreviewDirty = FALSE;
    ReadConfigurationFile(editor);
    UpdateConfiguration(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Refreshes only presentation measurements; XML was interpreted at file read time.
 */
void UpdateConfiguration(ConfigurationEditor_t* editor)
{
    const Utl_Arinc661_Screen_t* screen;
    uint32_t screen_index;

    if (editor->isPreviewDirty)
    {
        editor->totalScreenWidth = 0.0f;
        editor->maximumScreenHeight = 0.0f;
        editor->drawableScreenCount = 0;
        for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
        {
            screen = &editor->screens[screen_index];
            if ((0 < screen->pixelSize.width) && (0 < screen->pixelSize.height))
            {
                editor->drawableScreenCount += 1;
                editor->totalScreenWidth += (float32_t)screen->pixelSize.width;
                if (editor->maximumScreenHeight < (float32_t)screen->pixelSize.height)
                {
                    editor->maximumScreenHeight = (float32_t)screen->pixelSize.height;
                }
            }
        }
        editor->isPreviewDirty = FALSE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws both docked windows from current typed configuration values.
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
