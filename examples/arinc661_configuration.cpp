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
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
#include "os/os_file.h"
#include "utl/utl_defines.h"
}

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define CONFIGURATION_WINDOW_NAME ("ARINC 661 Configuration")
#define HIERARCHY_PANE_NAME       ("Hierarchy")
#define INSPECTOR_PANE_NAME       ("Inspector")
#define VISUALIZER_PANE_NAME      ("Visualizer")
#define CONFIGURATION_FILE_LABEL  ("arinc661_configuration.xml")
#define HIERARCHY_PANE_COLUMN      (0)
#define VISUALIZER_PANE_COLUMN     (1)
#define INSPECTOR_PANE_COLUMN      (2)
#define CONFIGURATION_PANE_COUNT  (3)

#define CONFIGURATION_ADD_SCREEN_LABEL    ("+ Add Screen")
#define CONFIGURATION_ADD_LAYOUT_LABEL    ("+ Add Layout")
#define CONFIGURATION_ADD_WINDOW_LABEL    ("+ Add Window")
#define CONFIGURATION_REMOVE_SCREEN_LABEL ("Remove Screen")
#define CONFIGURATION_REMOVE_LAYOUT_LABEL ("Remove Layout")
#define CONFIGURATION_REMOVE_WINDOW_LABEL ("Remove Window")
#define CONFIGURATION_RELOAD_XML_LABEL     ("Reload XML")
#define CONFIGURATION_BUILD_XML_LABEL      ("Build XML")
#define CONFIGURATION_BASE_UNIT_LABEL      ("Base Unit")

#define VISUALIZER_CANVAS_COLOR        (IM_COL32(22, 27, 34, 255))
#define VISUALIZER_SCREEN_COLOR        (IM_COL32(42, 50, 64, 255))
#define VISUALIZER_SCREEN_BORDER_COLOR (IM_COL32(220, 230, 245, 255))
#define VISUALIZER_WINDOW_COLOR        (IM_COL32(42, 154, 188, 80))
#define VISUALIZER_WINDOW_BORDER_COLOR (IM_COL32(97, 221, 245, 255))
#define VISUALIZER_TEXT_COLOR          (IM_COL32(240, 248, 255, 255))

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
 * \brief Recalculates the dimensions needed to fit drawable screens in the visualizer.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void UpdatePreviewMeasurements(ConfigurationEditor_t* editor);

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
 * \brief Chooses the next identifier for a screen, layout, or window.
 *
 * \param[in]  editor      Editor state
 * \param[in]  kind        Entity type
 * \param[in]  screenIndex Owning screen index for a layout
 * \param[out] id          Next available identifier
 *
 * \retval TRUE  An identifier is available.
 * \retval FALSE The identifier range is exhausted.
 */
static bool_t NextIdentifier(const ConfigurationEditor_t* editor, ConfigurationSelectionKind_t kind, uint32_t screenIndex, int64_t* id);

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
 * \brief Draws an X remove control beside the preceding tree row.
 *
 * \param[in] label     Button identifier and tooltip text
 * \param[in] rowHeight Height of the tree row
 *
 * \retval TRUE  The remove control was clicked.
 * \retval FALSE The remove control was not clicked.
 */
static bool_t DrawRemoveButton(const char_t* label, float32_t rowHeight);

/*************************************************************************************************/
/**
 * \brief Draws the selection tree and its add and remove controls.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawHierarchy(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Begins a two-column property table with the editor's standard columns.
 *
 * \param[in] name Table identifier
 *
 * \retval TRUE  The table is visible.
 * \retval FALSE The table is not visible.
 */
static bool_t BeginPropertyTable(const char_t* name);

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
 * \brief Draws the configuration actions and hierarchy in the left child pane.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawHierarchyPane(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the selected configuration item's properties in the right child pane.
 *
 * \param[in,out] editor Editor state
 *
 * \retval None
 */
static void DrawInspector(ConfigurationEditor_t* editor);

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
 * \brief Limits pending pane drags before ImGui applies them.
 *
 * \retval None
 */
static void ClampPaneResize(void);

/*************************************************************************************************/
/**
 * \brief Measures the default widths of the hierarchy and inspector panes.
 *
 * \param[out] hierarchyWidth Width needed for the file actions
 * \param[out] inspectorWidth Width needed for the property fields
 *
 * \retval None
 */
static void MeasurePaneWidths(float32_t* hierarchyWidth, float32_t* inspectorWidth);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Sums valid screen widths and finds the tallest valid screen in the current configuration.
 */
static void UpdatePreviewMeasurements(ConfigurationEditor_t* editor)
{
    const Utl_Arinc661_Screen_t* screen;
    uint32_t                     screen_index;
    bool_t                       has_screen_width;
    bool_t                       has_screen_height;
    bool_t                       is_drawable_screen;

    editor->totalScreenWidth    = 0.0f;
    editor->maximumScreenHeight = 0.0f;
    editor->drawableScreenCount = 0;
    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        screen             = &editor->screens[screen_index];
        has_screen_width   = (0 < screen->pixelSize.width);
        has_screen_height  = (0 < screen->pixelSize.height);
        is_drawable_screen = !!(has_screen_width && has_screen_height);
        if (is_drawable_screen)
        {
            editor->drawableScreenCount += 1;
            editor->totalScreenWidth    += (float32_t)screen->pixelSize.width;
            editor->maximumScreenHeight = ImMax(editor->maximumScreenHeight, (float32_t)screen->pixelSize.height);
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
    FILE*    xml_file;
    uint64_t output_byte_count;
    uint64_t bytes_written;

    output_byte_count = Utl_Arinc661_WriteConfiguration(&editor->configuration, editor->outputBuffer, sizeof(editor->outputBuffer), editor->workspace, sizeof(editor->workspace));
    if (0 == output_byte_count)
    {
        (void)strcpy(editor->status, "Could not build the display configuration XML.");
    }
    else
    {
        xml_file = fopen(ARINC661_CONFIGURATION_XML_PATH, "wb");
        if (NULL == xml_file)
        {
            (void)strcpy(editor->status, "Could not write arinc661_configuration.xml.");
        }
        else
        {
            bytes_written = fwrite(editor->outputBuffer, 1, output_byte_count, xml_file);
            (void)fclose(xml_file);
            if (output_byte_count == bytes_written)
            {
                (void)strcpy(editor->status, "Built arinc661_configuration.xml.");
            }
            else
            {
                (void)strcpy(editor->status, "Could not finish writing arinc661_configuration.xml.");
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Layout identifiers are local to a screen. Screen and window identifiers are global.
 */
static bool_t NextIdentifier(const ConfigurationEditor_t* editor, ConfigurationSelectionKind_t kind, uint32_t screenIndex, int64_t* id)
{
    int64_t  maximum_id;
    uint32_t index;
    bool_t   has_next_id;

    maximum_id = -1;
    if (CONFIGURATION_SELECTION_SCREEN == kind)
    {
        for (index = 0; index < editor->configuration.screenCount; ++index)
        {
            maximum_id = ImMax(maximum_id, editor->screens[index].id);
        }
    }
    else if (CONFIGURATION_SELECTION_LAYOUT == kind)
    {
        for (index = 0; index < editor->configuration.layoutCount; ++index)
        {
            if (screenIndex == editor->layouts[index].screenIndex)
            {
                maximum_id = ImMax(maximum_id, editor->layouts[index].id);
            }
        }
    }
    else if (CONFIGURATION_SELECTION_WINDOW == kind)
    {
        for (index = 0; index < editor->configuration.windowCount; ++index)
        {
            maximum_id = ImMax(maximum_id, editor->windows[index].id);
        }
    }
    has_next_id = (INT64_MAX != maximum_id);
    if (has_next_id)
    {
        (*id) = maximum_id + 1;
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
    int64_t                id;
    bool_t                 has_capacity;
    bool_t                 has_id;

    has_capacity = (editor->configuration.screenCount < editor->configuration.screenCapacity);
    has_id       = NextIdentifier(editor, CONFIGURATION_SELECTION_SCREEN, 0, &id);
    if (has_capacity && has_id)
    {
        screen = &editor->screens[editor->configuration.screenCount];
        (void)memset(screen, 0, sizeof((*screen)));
        screen->id                        = id;
        screen->pixelSize.width           = 640;
        screen->pixelSize.height          = 480;
        screen->pixelSize.units           = UTL_ARINC661_UNIT_PIXEL;
        screen->physicalDimensions.width  = 5;
        screen->physicalDimensions.height = 4;
        screen->physicalDimensions.units  = UTL_ARINC661_UNIT_INCH;
        editor->selectedKind              = CONFIGURATION_SELECTION_SCREEN;
        editor->selectedIndex             = editor->configuration.screenCount;
        editor->configuration.screenCount += 1;
        UpdatePreviewMeasurements(editor);
        (void)strcpy(editor->status, "Added screen. Build XML to save.");
    }
    else
    {
        (void)strcpy(editor->status, "Could not add another screen.");
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
    int64_t                id;
    bool_t                 has_capacity;
    bool_t                 has_id;

    has_capacity = (editor->configuration.layoutCount < editor->configuration.layoutCapacity);
    has_id       = NextIdentifier(editor, CONFIGURATION_SELECTION_LAYOUT, screenIndex, &id);
    if (has_capacity && has_id)
    {
        layout = &editor->layouts[editor->configuration.layoutCount];
        (void)memset(layout, 0, sizeof((*layout)));
        layout->id            = id;
        layout->screenIndex   = screenIndex;
        editor->selectedKind  = CONFIGURATION_SELECTION_LAYOUT;
        editor->selectedIndex = editor->configuration.layoutCount;
        editor->configuration.layoutCount += 1;
        (void)strcpy(editor->status, "Added layout. Build XML to save.");
    }
    else
    {
        (void)strcpy(editor->status, "Could not add another layout.");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The window begins at the origin and uses its screen's current pixel size.
 */
static void AddWindow(ConfigurationEditor_t* editor, uint32_t layoutIndex)
{
    Utl_Arinc661_Window_t*       window;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Screen_t* screen;
    int64_t                      id;
    bool_t                       has_capacity;
    bool_t                       has_id;

    has_capacity = (editor->configuration.windowCount < editor->configuration.windowCapacity);
    has_id       = NextIdentifier(editor, CONFIGURATION_SELECTION_WINDOW, 0, &id);
    if (has_capacity && has_id)
    {
        layout = &editor->layouts[layoutIndex];
        screen = &editor->screens[layout->screenIndex];
        window = &editor->windows[editor->configuration.windowCount];
        (void)memset(window, 0, sizeof((*window)));
        window->id = id;
        (void)snprintf(window->name, sizeof(window->name), "window%" PRId64, id);
        window->layoutIndex            = layoutIndex;
        window->pixelArea.width        = screen->pixelSize.width;
        window->pixelArea.height       = screen->pixelSize.height;
        window->scalingFactor.xScale   = 20.0f;
        window->scalingFactor.yScale   = 20.0f;
        window->scalingFactor.baseUnit = UTL_ARINC661_UNIT_SCREEN;
        window->scalingFactor.perUnit  = UTL_ARINC661_UNIT_PIXEL;
        editor->selectedKind           = CONFIGURATION_SELECTION_WINDOW;
        editor->selectedIndex          = editor->configuration.windowCount;
        editor->configuration.windowCount += 1;
        (void)strcpy(editor->status, "Added window. Build XML to save.");
    }
    else
    {
        (void)strcpy(editor->status, "Could not add another window.");
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
        (void)memmove(
            &editor->windows[windowIndex],
            &editor->windows[windowIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->windows[0])));
    }
    editor->configuration.windowCount -= 1;
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
        (void)memmove(
            &editor->layouts[layoutIndex],
            &editor->layouts[layoutIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->layouts[0])));
    }
    editor->configuration.layoutCount -= 1;
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
        (void)memmove(
            &editor->screens[screenIndex],
            &editor->screens[screenIndex + 1],
            ((uint64_t)remaining_count * sizeof(editor->screens[0])));
    }
    editor->configuration.screenCount -= 1;
    UpdatePreviewMeasurements(editor);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fits the X to the current font and centers it in a tree-row-height hit area.
 */
static bool_t DrawRemoveButton(const char_t* label, float32_t rowHeight)
{
    ImDrawList* draw_list;
    ImVec2      button_size;
    ImVec2      cross_half_size;
    ImVec2      cross_center;
    ImVec2      cross_min;
    ImVec2      cross_max;
    ImVec2      cross_left_bottom;
    ImVec2      cross_right_top;
    ImU32       cross_color;
    float32_t   cross_size;
    bool_t      is_clicked;

    draw_list       = ImGui::GetWindowDrawList();
    button_size     = ImVec2(ImGui::GetTextLineHeight(), rowHeight);
    cross_size      = ImMin(ImGui::CalcTextSize("x").x, ImGui::GetTextLineHeight());
    cross_half_size = (ImVec2(cross_size, cross_size) * 0.5f);

    ImGui::SameLine();
    is_clicked = ImGui::InvisibleButton(label, button_size);
    ImGui::SetItemTooltip("%s", label);
    cross_center      = ((ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f);
    cross_min         = (cross_center - cross_half_size);
    cross_max         = (cross_center + cross_half_size);
    cross_left_bottom = ImVec2(cross_min.x, cross_max.y);
    cross_right_top   = ImVec2(cross_max.x, cross_min.y);
    cross_color       = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    if (ImGui::IsItemHovered())
    {
        cross_color = ImGui::GetColorU32(ImGuiCol_Text);
    }
    draw_list->AddLine(cross_min, cross_max, cross_color);
    draw_list->AddLine(cross_left_bottom, cross_right_top, cross_color);

    return is_clicked;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Iterates the typed parent indices and applies requested additions or removals after traversal.
 */
static void DrawHierarchy(ConfigurationEditor_t* editor)
{
    const Utl_Arinc661_Screen_t* screen;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Window_t* window;
    ImGuiTreeNodeFlags           flags;
    float32_t                    row_height;
    uint32_t                     screen_index;
    uint32_t                     layout_index;
    uint32_t                     window_index;
    uint32_t                     parent_index;
    uint32_t                     add_layout_screen_index;
    uint32_t                     add_window_layout_index;
    uint32_t                     remove_screen_index;
    uint32_t                     remove_layout_index;
    uint32_t                     remove_window_index;
    bool_t                       is_screen_open;
    bool_t                       is_layout_open;

    add_layout_screen_index = UINT32_MAX;
    add_window_layout_index = UINT32_MAX;
    remove_screen_index     = UINT32_MAX;
    remove_layout_index     = UINT32_MAX;
    remove_window_index     = UINT32_MAX;
    for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
    {
        screen = &editor->screens[screen_index];
        flags  = (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
        if ((CONFIGURATION_SELECTION_SCREEN == editor->selectedKind) && (screen_index == editor->selectedIndex))
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushID(screen);
        is_screen_open = ImGui::TreeNodeEx("screen", flags, "Screen %" PRId64, screen->id);
        if (ImGui::IsItemClicked())
        {
            editor->selectedKind  = CONFIGURATION_SELECTION_SCREEN;
            editor->selectedIndex = screen_index;
        }
        row_height = ImGui::GetItemRectSize().y;
        if (DrawRemoveButton(CONFIGURATION_REMOVE_SCREEN_LABEL, row_height))
        {
            remove_screen_index = screen_index;
        }
        ImGui::PopID();
        if (is_screen_open)
        {
            for (layout_index = 0; layout_index < editor->configuration.layoutCount; ++layout_index)
            {
                layout = &editor->layouts[layout_index];
                if (screen_index == layout->screenIndex)
                {
                    flags = (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
                    if ((CONFIGURATION_SELECTION_LAYOUT == editor->selectedKind) && (layout_index == editor->selectedIndex))
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    ImGui::PushID(layout);
                    is_layout_open = ImGui::TreeNodeEx("layout", flags, "Layout %" PRId64, layout->id);
                    if (ImGui::IsItemClicked())
                    {
                        editor->selectedKind  = CONFIGURATION_SELECTION_LAYOUT;
                        editor->selectedIndex = layout_index;
                    }
                    row_height = ImGui::GetItemRectSize().y;
                    if (DrawRemoveButton(CONFIGURATION_REMOVE_LAYOUT_LABEL, row_height))
                    {
                        remove_layout_index = layout_index;
                    }
                    ImGui::PopID();
                    if (is_layout_open)
                    {
                        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                        {
                            window = &editor->windows[window_index];
                            if (layout_index == window->layoutIndex)
                            {
                                flags = (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
                                if ((CONFIGURATION_SELECTION_WINDOW == editor->selectedKind) && (window_index == editor->selectedIndex))
                                {
                                    flags |= ImGuiTreeNodeFlags_Selected;
                                }
                                ImGui::PushID(window);
                                (void)ImGui::TreeNodeEx("window", flags, "Window %" PRId64 " (%s)", window->id, window->name);
                                if (ImGui::IsItemClicked())
                                {
                                    editor->selectedKind  = CONFIGURATION_SELECTION_WINDOW;
                                    editor->selectedIndex = window_index;
                                }
                                row_height = ImGui::GetItemRectSize().y;
                                if (DrawRemoveButton(CONFIGURATION_REMOVE_WINDOW_LABEL, row_height))
                                {
                                    remove_window_index = window_index;
                                }
                                ImGui::PopID();
                            }
                        }
                        flags = (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        (void)ImGui::TreeNodeEx(CONFIGURATION_ADD_WINDOW_LABEL, flags);
                        if (ImGui::IsItemClicked())
                        {
                            add_window_layout_index = layout_index;
                        }
                        ImGui::TreePop();
                    }
                }
            }
            flags = (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            (void)ImGui::TreeNodeEx(CONFIGURATION_ADD_LAYOUT_LABEL, flags);
            if (ImGui::IsItemClicked())
            {
                add_layout_screen_index = screen_index;
            }
            ImGui::TreePop();
        }
    }
    flags = (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    (void)ImGui::TreeNodeEx(CONFIGURATION_ADD_SCREEN_LABEL, flags);
    if (ImGui::IsItemClicked())
    {
        AddScreen(editor);
    }
    if (UINT32_MAX != remove_screen_index)
    {
        RemoveScreen(editor, remove_screen_index);
        editor->selectedKind  = CONFIGURATION_SELECTION_NONE;
        editor->selectedIndex = 0;
        (void)strcpy(editor->status, "Removed screen. Build XML to save.");
    }
    else if (UINT32_MAX != remove_layout_index)
    {
        parent_index = editor->layouts[remove_layout_index].screenIndex;
        RemoveLayout(editor, remove_layout_index);
        editor->selectedKind  = CONFIGURATION_SELECTION_SCREEN;
        editor->selectedIndex = parent_index;
        (void)strcpy(editor->status, "Removed layout. Build XML to save.");
    }
    else if (UINT32_MAX != remove_window_index)
    {
        parent_index = editor->windows[remove_window_index].layoutIndex;
        RemoveWindow(editor, remove_window_index);
        editor->selectedKind  = CONFIGURATION_SELECTION_LAYOUT;
        editor->selectedIndex = parent_index;
        (void)strcpy(editor->status, "Removed window. Build XML to save.");
    }
    else if (UINT32_MAX != add_layout_screen_index)
    {
        AddLayout(editor, add_layout_screen_index);
    }
    else if (UINT32_MAX != add_window_layout_index)
    {
        AddWindow(editor, add_window_layout_index);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  All inspector groups use the same property and value columns.
 */
static bool_t BeginPropertyTable(const char_t* name)
{
    const ImGuiStyle* style;
    ImGuiTableFlags   flags;
    ImVec2            label_size;
    float32_t         property_width;
    bool_t            is_visible;

    style          = &ImGui::GetStyle();
    label_size     = ImGui::CalcTextSize(CONFIGURATION_BASE_UNIT_LABEL);
    property_width = (label_size.x + (style->FramePadding.x * 2.0f));
    flags          = (ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp);
    is_visible     = ImGui::BeginTable(name, 2, flags);
    if (is_visible)
    {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, property_width);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    }

    return is_visible;
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
    bool_t   is_selected;
    bool_t   has_changed;

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
                (*unit)     = (Utl_Arinc661_Unit_t)unit_index;
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
    bool_t                 has_width_changed;
    bool_t                 has_height_changed;
    bool_t                 has_pixel_size_changed;

    screen = &editor->screens[editor->selectedIndex];
    ImGui::TextUnformatted("Screen");
    if (BeginPropertyTable("ScreenProperties"))
    {
        (void)DrawIntegerField("ID", &screen->id);
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Pixel Size");
    if (BeginPropertyTable("PixelSizeProperties"))
    {
        has_width_changed      = DrawIntegerField("Width", &screen->pixelSize.width);
        has_height_changed     = DrawIntegerField("Height", &screen->pixelSize.height);
        has_pixel_size_changed = !!(has_width_changed || has_height_changed);
        (void)DrawUnitField("Units", &screen->pixelSize.units);
        ImGui::EndTable();
        if (has_pixel_size_changed)
        {
            UpdatePreviewMeasurements(editor);
        }
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Physical Dimensions");
    if (BeginPropertyTable("PhysicalDimensionProperties"))
    {
        (void)DrawIntegerField("Width", &screen->physicalDimensions.width);
        (void)DrawIntegerField("Height", &screen->physicalDimensions.height);
        (void)DrawUnitField("Units", &screen->physicalDimensions.units);
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
    if (BeginPropertyTable("LayoutProperties"))
    {
        (void)DrawIntegerField("ID", &layout->id);
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

    window = &editor->windows[editor->selectedIndex];
    ImGui::TextUnformatted("Window");
    if (BeginPropertyTable("WindowProperties"))
    {
        (void)DrawIntegerField("ID", &window->id);
        (void)DrawTextField("Name", window->name, sizeof(window->name));
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Pixel Area");
    if (BeginPropertyTable("PixelAreaProperties"))
    {
        (void)DrawIntegerField("X", &window->pixelArea.x);
        (void)DrawIntegerField("Y", &window->pixelArea.y);
        (void)DrawIntegerField("Width", &window->pixelArea.width);
        (void)DrawIntegerField("Height", &window->pixelArea.height);
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Scaling Factor");
    if (BeginPropertyTable("ScalingFactorProperties"))
    {
        (void)DrawDecimalField("X Scale", &window->scalingFactor.xScale);
        (void)DrawDecimalField("Y Scale", &window->scalingFactor.yScale);
        (void)DrawUnitField(CONFIGURATION_BASE_UNIT_LABEL, &window->scalingFactor.baseUnit);
        (void)DrawUnitField("Per Unit", &window->scalingFactor.perUnit);
        ImGui::EndTable();
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  File actions and the hierarchy share the left child pane.
 */
static void DrawHierarchyPane(ConfigurationEditor_t* editor)
{
    if (ImGui::BeginChild(HIERARCHY_PANE_NAME, ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding))
    {
        ImGui::TextUnformatted(HIERARCHY_PANE_NAME);
        ImGui::Separator();
        ImGui::TextUnformatted(CONFIGURATION_FILE_LABEL);
        if (ImGui::Button(CONFIGURATION_RELOAD_XML_LABEL))
        {
            ReloadConfiguration(editor);
        }
        if (editor->isLoaded)
        {
            ImGui::SameLine();
            if (ImGui::Button(CONFIGURATION_BUILD_XML_LABEL))
            {
                BuildConfigurationFile(editor);
            }
        }
        ImGui::TextWrapped("%s", editor->status);
        if (editor->isLoaded)
        {
            DrawHierarchy(editor);
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shows the selected item's typed fields in the right child pane.
 */
static void DrawInspector(ConfigurationEditor_t* editor)
{
    if (ImGui::BeginChild(INSPECTOR_PANE_NAME, ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding))
    {
        ImGui::TextUnformatted(INSPECTOR_PANE_NAME);
        ImGui::Separator();
        if (editor->isLoaded)
        {
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
        else
        {
            ImGui::TextUnformatted(editor->status);
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fits the combined screen widths and tallest screen inside the available canvas.
 */
static void DrawVisualizer(const ConfigurationEditor_t* editor)
{
    const ImGuiStyle*            style;
    const Utl_Arinc661_Screen_t* screen;
    const Utl_Arinc661_Layout_t* layout;
    const Utl_Arinc661_Window_t* window;
    ImDrawList*                  draw_list;
    ImVec2                       canvas_min;
    ImVec2                       canvas_size;
    ImVec2                       canvas_max;
    ImVec2                       content_size;
    ImVec2                       screen_min;
    ImVec2                       screen_position;
    ImVec2                       screen_size;
    ImVec2                       screen_max;
    ImVec2                       screen_label_size;
    ImVec2                       screen_label_position;
    ImVec2                       area_offset;
    ImVec2                       area_size;
    ImVec2                       area_min;
    ImVec2                       area_max;
    char_t                       screen_label[64];
    char_t                       window_label[UTL_ARINC661_MAX_NAME_BYTES + 64];
    float32_t                    label_height;
    float32_t                    unused_height;
    float32_t                    width_scale;
    float32_t                    height_scale;
    float32_t                    scale;
    uint32_t                     screen_index;
    uint32_t                     window_index;
    bool_t                       has_canvas_width;
    bool_t                       has_canvas_height;
    bool_t                       has_canvas_space;
    bool_t                       has_valid_layout;
    bool_t                       is_window_on_screen;
    bool_t                       has_screen_width;
    bool_t                       has_screen_height;
    bool_t                       is_drawable_screen;

    if (ImGui::BeginChild(VISUALIZER_PANE_NAME, ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding))
    {
        ImGui::TextUnformatted(VISUALIZER_PANE_NAME);
        ImGui::Separator();
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
            style        = &ImGui::GetStyle();
            canvas_min   = ImGui::GetCursorScreenPos();
            canvas_size  = ImGui::GetContentRegionAvail();
            label_height = ImGui::GetTextLineHeightWithSpacing();

            content_size = (canvas_size - (style->WindowPadding + style->WindowPadding));
            content_size.x -= (style->ItemSpacing.x * (float32_t)(editor->drawableScreenCount - 1));
            content_size.y -= label_height;

            has_canvas_width  = (0.0f < content_size.x);
            has_canvas_height = (0.0f < content_size.y);
            has_canvas_space  = !!(has_canvas_width && has_canvas_height);
            if (FALSE == has_canvas_space)
            {
                ImGui::TextUnformatted("Preview area is too small.");
            }
            else
            {
                (void)ImGui::InvisibleButton("Screen canvas", canvas_size);
                draw_list  = ImGui::GetWindowDrawList();
                canvas_max = (canvas_min + canvas_size);
                draw_list->PushClipRect(canvas_min, canvas_max, TRUE);
                draw_list->AddRectFilled(canvas_min, canvas_max, VISUALIZER_CANVAS_COLOR);
                width_scale   = (content_size.x / editor->totalScreenWidth);
                height_scale  = (content_size.y / editor->maximumScreenHeight);
                scale         = ImMin(width_scale, height_scale);
                unused_height = (content_size.y - (editor->maximumScreenHeight * scale));

                screen_position = (canvas_min + style->WindowPadding);
                screen_position.y += (label_height + (unused_height / 2.0f));
                for (screen_index = 0; screen_index < editor->configuration.screenCount; ++screen_index)
                {
                    screen             = &editor->screens[screen_index];
                    has_screen_width   = (0 < screen->pixelSize.width);
                    has_screen_height  = (0 < screen->pixelSize.height);
                    is_drawable_screen = !!(has_screen_width && has_screen_height);
                    if (is_drawable_screen)
                    {
                        screen_min  = screen_position;
                        screen_size = ImVec2((float32_t)screen->pixelSize.width, (float32_t)screen->pixelSize.height);
                        screen_max  = (screen_min + (screen_size * scale));
                        draw_list->AddRectFilled(screen_min, screen_max, VISUALIZER_SCREEN_COLOR);
                        draw_list->AddRect(screen_min, screen_max, VISUALIZER_SCREEN_BORDER_COLOR);
                        (void)snprintf(screen_label, sizeof(screen_label), "Screen %" PRId64 " (%" PRId64 " x %" PRId64 ")", screen->id, screen->pixelSize.width, screen->pixelSize.height);
                        screen_label_size       = ImGui::CalcTextSize(screen_label);
                        screen_label_position   = screen_min;
                        screen_label_position.y = (screen_label_position.y - (screen_label_size.y + style->ItemSpacing.y));
                        draw_list->AddText(screen_label_position, VISUALIZER_TEXT_COLOR, screen_label);
                        draw_list->PushClipRect(screen_min, screen_max, TRUE);
                        for (window_index = 0; window_index < editor->configuration.windowCount; ++window_index)
                        {
                            window              = &editor->windows[window_index];
                            has_valid_layout    = (window->layoutIndex < editor->configuration.layoutCount);
                            is_window_on_screen = FALSE;
                            if (has_valid_layout)
                            {
                                layout              = &editor->layouts[window->layoutIndex];
                                is_window_on_screen = (screen_index == layout->screenIndex);
                            }
                            if (is_window_on_screen)
                            {
                                area_offset = ImVec2((float32_t)window->pixelArea.x, (float32_t)window->pixelArea.y);
                                area_size   = ImVec2((float32_t)window->pixelArea.width, (float32_t)window->pixelArea.height);
                                area_min    = (screen_min + (area_offset * scale));
                                area_max    = (area_min + (area_size * scale));
                                draw_list->AddRectFilled(area_min, area_max, VISUALIZER_WINDOW_COLOR);
                                draw_list->AddRect(area_min, area_max, VISUALIZER_WINDOW_BORDER_COLOR);
                                if ('\0' != window->name[0])
                                {
                                    (void)snprintf(window_label, sizeof(window_label), "%s (%" PRId64 " x %" PRId64 ")", window->name, window->pixelArea.width, window->pixelArea.height);
                                }
                                else
                                {
                                    (void)snprintf(window_label, sizeof(window_label), "Window %" PRId64 " (%" PRId64 " x %" PRId64 ")", window->id, window->pixelArea.width, window->pixelArea.height);
                                }
                                draw_list->AddText((area_min + style->FramePadding), VISUALIZER_TEXT_COLOR, window_label);
                            }
                        }
                        draw_list->PopClipRect();
                        screen_position.x = (screen_max.x + style->ItemSpacing.x);
                    }
                }
                draw_list->PopClipRect();
            }
        }
    }
    ImGui::EndChild();
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Stops the left divider at the right divider.
 *  Stops the right divider when the inspector reaches the hierarchy's width.
 */
static void ClampPaneResize(void)
{
    ImGuiTable* table;
    float32_t   maximum_hierarchy_width;
    float32_t   maximum_visualizer_width;

    table = ImGui::GetCurrentTable();
    if (HIERARCHY_PANE_COLUMN == table->ResizedColumn)
    {
        maximum_hierarchy_width  = table->Columns[INSPECTOR_PANE_COLUMN].MinX;
        maximum_hierarchy_width -= table->Columns[HIERARCHY_PANE_COLUMN].MaxX;
        maximum_hierarchy_width += table->Columns[HIERARCHY_PANE_COLUMN].WidthGiven;
        table->ResizedColumnNextWidth = ImMin(table->ResizedColumnNextWidth, maximum_hierarchy_width);
    }
    else if (VISUALIZER_PANE_COLUMN == table->ResizedColumn)
    {
        maximum_visualizer_width = (table->Columns[VISUALIZER_PANE_COLUMN].WidthRequest + table->Columns[INSPECTOR_PANE_COLUMN].WidthRequest);
        maximum_visualizer_width -= table->Columns[HIERARCHY_PANE_COLUMN].WidthGiven;
        table->ResizedColumnNextWidth = ImMin(table->ResizedColumnNextWidth, maximum_visualizer_width);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the visible file actions and property fields for the side pane defaults.
 */
static void MeasurePaneWidths(float32_t* hierarchyWidth, float32_t* inspectorWidth)
{
    const ImGuiStyle* style;
    ImVec2            button_padding;
    ImVec2            file_label_size;
    ImVec2            toolbar_size;
    char_t            integer_text[32];
    float32_t         property_width;
    float32_t         value_width;

    style = &ImGui::GetStyle();

    (void)snprintf(integer_text, sizeof(integer_text), "%" PRId64, INT64_MIN);
    button_padding  = (style->FramePadding + style->FramePadding);
    file_label_size = ImGui::CalcTextSize(CONFIGURATION_FILE_LABEL);
    toolbar_size    = (ImGui::CalcTextSize(CONFIGURATION_RELOAD_XML_LABEL) + ImGui::CalcTextSize(CONFIGURATION_BUILD_XML_LABEL));

    toolbar_size += (button_padding + button_padding + style->ItemSpacing + style->WindowPadding + style->WindowPadding);

    property_width    = (ImGui::CalcTextSize(CONFIGURATION_BASE_UNIT_LABEL).x + button_padding.x);
    value_width       = (ImGui::CalcTextSize(integer_text).x + button_padding.x);
    (*hierarchyWidth) = ImMax(toolbar_size.x, (file_label_size.x + (style->WindowPadding.x * 2.0f)));
    (*inspectorWidth) = (property_width + value_width);
    (*inspectorWidth) += (style->CellPadding.x * 4.0f);
    (*inspectorWidth) += (style->WindowPadding.x * 2.0f);
    (*inspectorWidth) += style->ScrollbarSize;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Binds the fixed arrays and their capacities in the zero-initialized editor.
 */
void InitializeConfiguration(ConfigurationEditor_t* editor)
{
    editor->configuration.screens                 = editor->screens;
    editor->configuration.layouts                 = editor->layouts;
    editor->configuration.windows                 = editor->windows;
    editor->configuration.userApplications        = editor->userApplications;
    editor->configuration.externalSources         = editor->externalSources;
    editor->configuration.screenCapacity          = CONFIGURATION_MAX_SCREENS;
    editor->configuration.layoutCapacity          = CONFIGURATION_MAX_LAYOUTS;
    editor->configuration.windowCapacity          = CONFIGURATION_MAX_WINDOWS;
    editor->configuration.userApplicationCapacity = CONFIGURATION_MAX_USER_APPLICATIONS;
    editor->configuration.externalSourceCapacity  = CONFIGURATION_MAX_EXTERNAL_SOURCES;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads the example file and replaces the editor's typed configuration and selection.
 */
void ReloadConfiguration(ConfigurationEditor_t* editor)
{
    int32_t document_byte_count;
    bool_t  is_configuration;

    editor->isLoaded      = FALSE;
    editor->selectedKind  = CONFIGURATION_SELECTION_NONE;
    editor->selectedIndex = 0;

    editor->configuration.screenCount          = 0;
    editor->configuration.layoutCount          = 0;
    editor->configuration.windowCount          = 0;
    editor->configuration.userApplicationCount = 0;
    editor->configuration.externalSourceCount  = 0;
    editor->configuration.hasMetaData          = FALSE;

    editor->totalScreenWidth    = 0.0f;
    editor->maximumScreenHeight = 0.0f;
    editor->drawableScreenCount = 0;

    document_byte_count = OS_File_ReadFile(ARINC661_CONFIGURATION_XML_PATH, editor->inputBuffer, sizeof(editor->inputBuffer));
    if (0 > document_byte_count)
    {
        (void)strcpy(editor->status, "Could not read arinc661_configuration.xml.");
    }
    else
    {
        if (CONFIGURATION_MAX_DOCUMENT_BYTES < document_byte_count)
        {
            (void)strcpy(editor->status, "The XML file exceeds the input buffer.");
        }
        else if (0 == document_byte_count)
        {
            (void)strcpy(editor->status, "The XML file is empty.");
        }
        else
        {
            is_configuration = Utl_Arinc661_ReadConfiguration(&editor->configuration, editor->inputBuffer, (uint64_t)document_byte_count, editor->workspace, sizeof(editor->workspace));
            if (is_configuration)
            {
                editor->isLoaded = TRUE;
                if (0 < editor->configuration.screenCount)
                {
                    editor->selectedKind = CONFIGURATION_SELECTION_SCREEN;
                }
                UpdatePreviewMeasurements(editor);
                (void)strcpy(editor->status, "Loaded arinc661_configuration.xml.");
            }
            else
            {
                (void)strcpy(editor->status, "Invalid or unsupported display configuration XML.");
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Draws one viewport-sized window containing resizable hierarchy, visualizer, and inspector panes.
 */
void DrawConfiguration(ConfigurationEditor_t* editor)
{
    const ImGuiViewport* viewport;
    ImGuiWindowFlags     window_flags;
    ImGuiTableFlags      table_flags;
    ImVec2               table_size;
    float32_t            hierarchy_width;
    float32_t            inspector_width;

    viewport     = ImGui::GetMainViewport();
    window_flags = (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking);
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    if (ImGui::Begin(CONFIGURATION_WINDOW_NAME, NULL, window_flags))
    {
        MeasurePaneWidths(&hierarchy_width, &inspector_width);
        table_size  = ImGui::GetContentRegionAvail();
        table_flags = (ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX);
        if (ImGui::BeginTable("ConfigurationPanes", CONFIGURATION_PANE_COUNT, table_flags, table_size))
        {
            ImGui::TableSetupColumn(HIERARCHY_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, hierarchy_width);
            ImGui::TableSetupColumn(VISUALIZER_PANE_NAME, ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(INSPECTOR_PANE_NAME, ImGuiTableColumnFlags_WidthFixed, inspector_width);
            ClampPaneResize();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
            ImGui::TableNextRow();
            ImGui::PopStyleVar();
            ImGui::TableSetColumnIndex(HIERARCHY_PANE_COLUMN);
            DrawHierarchyPane(editor);
            ImGui::TableSetColumnIndex(INSPECTOR_PANE_COLUMN);
            DrawInspector(editor);
            ImGui::TableSetColumnIndex(VISUALIZER_PANE_COLUMN);
            DrawVisualizer(editor);
            ImGui::EndTable();
        }
    }
    ImGui::End();
}
