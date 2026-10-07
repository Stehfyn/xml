/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_icons.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define TOOLBAR_BUTTON_SIZE    (ARINC661_ICON_BUTTON_SIZE)
#define TOOLBAR_ICON_INSET     (6.0f)
#define TOOLBAR_GAP            (10.0f)
#define TOOLBAR_COLOR          (IM_COL32(220, 228, 240, 255))
#define TOOLBAR_DISABLED_COLOR (IM_COL32(120, 126, 134, 255))
#define TOOLBAR_ACTIVE_COLOR   (IM_COL32(97, 221, 245, 255))
#define TOOLBAR_HOVER_COLOR    (IM_COL32(255, 255, 255, 30))
#define TOOLBAR_GAP_COLOR      (IM_COL32(255, 255, 255, 40))
#define TOOLTIP_FORMAT         ("%s (%s)")

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
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Every icon is drawn from lines and rectangles inside the button, so no icon font is needed.
 */
void Arinc661_Icons_Draw(ImDrawList* drawList, Arinc661_Icon_t icon, ImVec2 minimum, ImVec2 maximum, ImU32 color)
{
    ImVec2    center;
    ImVec2    size;
    ImVec2    points[6];
    float32_t unit;

    center = ImVec2((minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f);
    size   = ImVec2(maximum.x - minimum.x, maximum.y - minimum.y);
    unit   = (size.x / 8.0f);
    switch (icon)
    {
        case ARINC661_ICON_NEW:
            // Page with a folded corner and a plus
            points[0] = ImVec2(minimum.x + unit, minimum.y);
            points[1] = ImVec2(maximum.x - (2.0f * unit), minimum.y);
            points[2] = ImVec2(maximum.x - unit, minimum.y + unit);
            points[3] = ImVec2(maximum.x - unit, maximum.y);
            points[4] = ImVec2(minimum.x + unit, maximum.y);
            drawList->AddPolyline(points, 5, color, ImDrawFlags_Closed, 1.5f);
            drawList->AddLine(ImVec2(center.x - unit, center.y + unit), ImVec2(center.x + unit, center.y + unit), color, 1.5f);
            drawList->AddLine(ImVec2(center.x, center.y), ImVec2(center.x, center.y + (2.0f * unit)), color, 1.5f);
            break;
        case ARINC661_ICON_SAVE:
        case ARINC661_ICON_SAVE_ALL:
            drawList->AddRect(minimum, maximum, color, 1.0f, 0, 1.5f);
            drawList->AddRectFilled(ImVec2(minimum.x + (2.0f * unit), minimum.y), ImVec2(maximum.x - (2.0f * unit), minimum.y + (2.5f * unit)), color);
            drawList->AddRect(ImVec2(minimum.x + (1.5f * unit), maximum.y - (3.0f * unit)), ImVec2(maximum.x - (1.5f * unit), maximum.y), color);
            if (ARINC661_ICON_SAVE_ALL == icon)
            {
                drawList->AddRect(ImVec2(minimum.x + (2.0f * unit), minimum.y - (2.0f * unit)), ImVec2(maximum.x + (2.0f * unit), maximum.y - (2.0f * unit)), color);
            }
            break;
        case ARINC661_ICON_UNDO:
        case ARINC661_ICON_REDO:
            // Curved arrow: an arc with a head on the left or the right
            drawList->PathArcTo(ImVec2(center.x, center.y + unit), 3.0f * unit, 3.4f, 6.0f, 12);
            drawList->PathStroke(color, 0, 1.5f);
            if (ARINC661_ICON_UNDO == icon)
            {
                drawList->AddTriangleFilled(ImVec2(minimum.x, center.y), ImVec2(minimum.x + (2.5f * unit), center.y - (2.0f * unit)), ImVec2(minimum.x + (2.5f * unit), center.y + (1.5f * unit)), color);
            }
            else
            {
                drawList->AddTriangleFilled(ImVec2(maximum.x, center.y), ImVec2(maximum.x - (2.5f * unit), center.y - (2.0f * unit)), ImVec2(maximum.x - (2.5f * unit), center.y + (1.5f * unit)), color);
            }
            break;
        case ARINC661_ICON_ADD_LAYER:
            // Two stacked sheets and a plus
            drawList->AddRect(ImVec2(minimum.x, minimum.y + (2.0f * unit)), ImVec2(maximum.x - (2.0f * unit), maximum.y), color);
            drawList->AddRect(ImVec2(minimum.x + (2.0f * unit), minimum.y), ImVec2(maximum.x, maximum.y - (2.0f * unit)), color);
            drawList->AddLine(ImVec2(maximum.x - (2.0f * unit), maximum.y - unit), ImVec2(maximum.x + unit, maximum.y - unit), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x - (0.5f * unit), maximum.y - (2.5f * unit)), ImVec2(maximum.x - (0.5f * unit), maximum.y + (0.5f * unit)), color, 1.5f);
            break;
        case ARINC661_ICON_ADD_WIDGET:
            drawList->AddRect(minimum, maximum, color, 1.0f, 0, 1.5f);
            drawList->AddLine(ImVec2(center.x - (2.0f * unit), center.y), ImVec2(center.x + (2.0f * unit), center.y), color, 1.5f);
            drawList->AddLine(ImVec2(center.x, center.y - (2.0f * unit)), ImVec2(center.x, center.y + (2.0f * unit)), color, 1.5f);
            break;
        case ARINC661_ICON_ADD_EXTENSION:
            // Box with a tab attached on its right side
            drawList->AddRect(ImVec2(minimum.x, minimum.y + unit), ImVec2(maximum.x - (2.5f * unit), maximum.y), color, 1.0f, 0, 1.5f);
            drawList->AddRectFilled(ImVec2(maximum.x - (2.5f * unit), center.y - unit), ImVec2(maximum.x, center.y + (1.5f * unit)), color);
            break;
        case ARINC661_ICON_DUPLICATE:
            drawList->AddRect(ImVec2(minimum.x, minimum.y + (2.0f * unit)), ImVec2(maximum.x - (2.0f * unit), maximum.y), color, 1.0f, 0, 1.5f);
            drawList->AddRect(ImVec2(minimum.x + (2.0f * unit), minimum.y), ImVec2(maximum.x, maximum.y - (2.0f * unit)), color);
            break;
        case ARINC661_ICON_DELETE:
            // Bin with a lid
            drawList->AddLine(ImVec2(minimum.x, minimum.y + unit), ImVec2(maximum.x, minimum.y + unit), color, 1.5f);
            drawList->AddLine(ImVec2(center.x - unit, minimum.y), ImVec2(center.x + unit, minimum.y), color, 1.5f);
            points[0] = ImVec2(minimum.x + unit, minimum.y + unit);
            points[1] = ImVec2(maximum.x - unit, minimum.y + unit);
            points[2] = ImVec2(maximum.x - (1.5f * unit), maximum.y);
            points[3] = ImVec2(minimum.x + (1.5f * unit), maximum.y);
            drawList->AddPolyline(points, 4, color, ImDrawFlags_Closed, 1.5f);
            drawList->AddLine(ImVec2(center.x, minimum.y + (2.5f * unit)), ImVec2(center.x, maximum.y - (1.5f * unit)), color);
            break;
        case ARINC661_ICON_GRID:
            drawList->AddRect(minimum, maximum, color);
            drawList->AddLine(ImVec2(minimum.x + (size.x / 3.0f), minimum.y), ImVec2(minimum.x + (size.x / 3.0f), maximum.y), color);
            drawList->AddLine(ImVec2(minimum.x + (2.0f * size.x / 3.0f), minimum.y), ImVec2(minimum.x + (2.0f * size.x / 3.0f), maximum.y), color);
            drawList->AddLine(ImVec2(minimum.x, minimum.y + (size.y / 3.0f)), ImVec2(maximum.x, minimum.y + (size.y / 3.0f)), color);
            drawList->AddLine(ImVec2(minimum.x, minimum.y + (2.0f * size.y / 3.0f)), ImVec2(maximum.x, minimum.y + (2.0f * size.y / 3.0f)), color);
            break;
        case ARINC661_ICON_SNAP:
            // Magnet: a U with two pole caps
            drawList->PathArcTo(ImVec2(center.x, center.y - unit), 3.0f * unit, 0.0f, 3.14159f, 12);
            drawList->PathStroke(color, 0, 2.5f);
            drawList->AddLine(ImVec2(center.x - (3.0f * unit), center.y - unit), ImVec2(center.x - (3.0f * unit), maximum.y), color, 2.5f);
            drawList->AddLine(ImVec2(center.x + (3.0f * unit), center.y - unit), ImVec2(center.x + (3.0f * unit), maximum.y), color, 2.5f);
            drawList->AddRectFilled(ImVec2(center.x - (4.0f * unit), maximum.y - (1.5f * unit)), ImVec2(center.x - (2.0f * unit), maximum.y), color);
            drawList->AddRectFilled(ImVec2(center.x + (2.0f * unit), maximum.y - (1.5f * unit)), ImVec2(center.x + (4.0f * unit), maximum.y), color);
            break;
        case ARINC661_ICON_FIT:
            // Four corner brackets
            drawList->AddLine(ImVec2(minimum.x, minimum.y + (2.0f * unit)), minimum, color, 1.5f);
            drawList->AddLine(minimum, ImVec2(minimum.x + (2.0f * unit), minimum.y), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x - (2.0f * unit), minimum.y), ImVec2(maximum.x, minimum.y), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x, minimum.y), ImVec2(maximum.x, minimum.y + (2.0f * unit)), color, 1.5f);
            drawList->AddLine(ImVec2(minimum.x, maximum.y - (2.0f * unit)), ImVec2(minimum.x, maximum.y), color, 1.5f);
            drawList->AddLine(ImVec2(minimum.x, maximum.y), ImVec2(minimum.x + (2.0f * unit), maximum.y), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x - (2.0f * unit), maximum.y), maximum, color, 1.5f);
            drawList->AddLine(maximum, ImVec2(maximum.x, maximum.y - (2.0f * unit)), color, 1.5f);
            break;
        case ARINC661_ICON_HELP:
            drawList->AddCircle(center, size.x * 0.5f, color, 0, 1.5f);
            drawList->AddText(ImVec2(center.x - (ImGui::CalcTextSize("?").x * 0.5f), center.y - (ImGui::GetTextLineHeight() * 0.5f)), color, "?");
            break;
        case ARINC661_ICON_ZOOM_IN:
        case ARINC661_ICON_ZOOM_OUT:
            // Magnifier with a plus or a minus
            drawList->AddCircle(ImVec2(center.x - unit, center.y - unit), 2.8f * unit, color, 0, 1.5f);
            drawList->AddLine(ImVec2(center.x + unit, center.y + unit), maximum, color, 2.0f);
            drawList->AddLine(ImVec2(center.x - (2.3f * unit), center.y - unit), ImVec2(center.x + (0.3f * unit), center.y - unit), color, 1.5f);
            if (ARINC661_ICON_ZOOM_IN == icon)
            {
                drawList->AddLine(ImVec2(center.x - unit, center.y - (2.3f * unit)), ImVec2(center.x - unit, center.y + (0.3f * unit)), color, 1.5f);
            }
            break;
        case ARINC661_ICON_SCREEN:
            // Monitor on a stand
            drawList->AddRect(ImVec2(minimum.x, minimum.y), ImVec2(maximum.x, maximum.y - (2.5f * unit)), color, 1.0f, 0, 1.5f);
            drawList->AddLine(ImVec2(center.x, maximum.y - (2.5f * unit)), ImVec2(center.x, maximum.y), color, 1.5f);
            drawList->AddLine(ImVec2(center.x - (2.0f * unit), maximum.y), ImVec2(center.x + (2.0f * unit), maximum.y), color, 1.5f);
            break;
        case ARINC661_ICON_LAYOUT:
            // Screen split into panes
            drawList->AddRect(minimum, maximum, color, 1.0f, 0, 1.5f);
            drawList->AddLine(ImVec2(center.x, minimum.y), ImVec2(center.x, maximum.y), color);
            drawList->AddLine(ImVec2(center.x, center.y), ImVec2(maximum.x, center.y), color);
            break;
        case ARINC661_ICON_WINDOW:
            // Frame with a title bar
            drawList->AddRect(minimum, maximum, color, 1.0f, 0, 1.5f);
            drawList->AddRectFilled(minimum, ImVec2(maximum.x, minimum.y + (2.0f * unit)), color);
            break;
        case ARINC661_ICON_APPLICATION:
            // Window with a cog-like dot and a link line
            drawList->AddRect(ImVec2(minimum.x, minimum.y + unit), ImVec2(maximum.x - (2.0f * unit), maximum.y), color, 1.0f, 0, 1.5f);
            drawList->AddCircleFilled(ImVec2(maximum.x - unit, minimum.y + unit), 1.5f * unit, color);
            drawList->AddLine(ImVec2(maximum.x - unit, minimum.y + (2.5f * unit)), ImVec2(maximum.x - unit, center.y + unit), color, 1.5f);
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A square button drawn with an icon; the tooltip names the action and its shortcut.
 *
 * \retval TRUE    if the button was pressed.
 * \retval FALSE   otherwise.
 */
bool_t Arinc661_Icons_Button(Arinc661_Icon_t icon, const char_t* label, const char_t* shortcut, bool_t isEnabled, bool_t isActive)
{
    ImDrawList* draw_list;
    ImVec2      minimum;
    ImVec2      maximum;
    ImU32       color;
    bool_t      is_pressed;

    // Start a new row rather than clip when the button would not fit
    if (ImGui::GetContentRegionAvail().x < TOOLBAR_BUTTON_SIZE)
    {
        ImGui::NewLine();
    }
    ImGui::PushID(label);
    ImGui::BeginDisabled(FALSE == isEnabled);
    is_pressed = ImGui::InvisibleButton("##icon", ImVec2(TOOLBAR_BUTTON_SIZE, TOOLBAR_BUTTON_SIZE));
    ImGui::EndDisabled();
    draw_list = ImGui::GetWindowDrawList();
    minimum   = ImGui::GetItemRectMin();
    maximum   = ImGui::GetItemRectMax();
    color     = TOOLBAR_COLOR;
    if (FALSE == isEnabled)
    {
        color = TOOLBAR_DISABLED_COLOR;
    }
    else if (isActive)
    {
        color = TOOLBAR_ACTIVE_COLOR;
    }
    if (isEnabled && ImGui::IsItemHovered())
    {
        draw_list->AddRectFilled(minimum, maximum, TOOLBAR_HOVER_COLOR, 3.0f);
    }
    if (isActive)
    {
        draw_list->AddRect(minimum, maximum, TOOLBAR_ACTIVE_COLOR, 3.0f);
    }
    Arinc661_Icons_Draw(draw_list, icon, ImVec2(minimum.x + TOOLBAR_ICON_INSET, minimum.y + TOOLBAR_ICON_INSET), ImVec2(maximum.x - TOOLBAR_ICON_INSET, maximum.y - TOOLBAR_ICON_INSET), color);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if (NULL != shortcut)
        {
            ImGui::SetTooltip(TOOLTIP_FORMAT, label, shortcut);
        }
        else
        {
            ImGui::SetTooltip("%s", label);
        }
    }
    ImGui::PopID();
    ImGui::SameLine();

    return is_pressed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A short vertical line with spacing on both sides.
 */
void Arinc661_Icons_Gap(void)
{
    ImVec2 position;

    if (ImGui::GetContentRegionAvail().x < (TOOLBAR_GAP + TOOLBAR_BUTTON_SIZE))
    {
        ImGui::NewLine();
    }
    ImGui::Dummy(ImVec2(TOOLBAR_GAP * 0.5f, TOOLBAR_BUTTON_SIZE));
    ImGui::SameLine();
    position = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(position.x, position.y + 4.0f), ImVec2(position.x, position.y + TOOLBAR_BUTTON_SIZE - 4.0f), TOOLBAR_GAP_COLOR);
    ImGui::Dummy(ImVec2(TOOLBAR_GAP * 0.5f, TOOLBAR_BUTTON_SIZE));
    ImGui::SameLine();
}
