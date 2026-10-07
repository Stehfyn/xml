/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_gfx_imgui.h"

#include "imgui_internal.h"

extern "C"
{
#include "os/os_assert.h"
}

#include <math.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define IMGUI_MAX_POLYGON_POINTS (512)

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

static void RenderText(const FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Command_t* command, ImDrawList* target);

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Lays text out in the command's box and rotates its vertices about the box centre.
 */
static void RenderText(const FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Command_t* command, ImDrawList* target)
{
    const char_t* text;
    ImFont*       font;
    ImVec2        box_min;
    ImVec2        box_max;
    ImVec2        size;
    ImVec2        position;
    ImVec2        center;
    ImVec4        clip;
    float32_t     font_size;
    float32_t     cosine;
    float32_t     sine;
    float32_t     dx;
    float32_t     dy;
    int32_t       vertex_start;
    int32_t       vertex_index;

    text      = FACEOE_Gfx_DrawList_CommandText(drawList, command);
    font      = ImGui::GetFont();
    font_size = command->fontSize;
    if (font_size <= 0.0f)
    {
        font_size = ImGui::GetFontSize();
    }
    box_min = ImVec2(drawList->points[command->firstPoint].x, drawList->points[command->firstPoint].y);
    box_max = ImVec2(drawList->points[command->firstPoint + 1].x, drawList->points[command->firstPoint + 1].y);
    size    = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);
    // Lay rotated text out in an unrotated box of the same centre, then turn it
    center = ImVec2((box_min.x + box_max.x) * 0.5f, (box_min.y + box_max.y) * 0.5f);
    position = box_min;
    if (FACEOE_GFX_ALIGN_CENTER == command->horizontal)
    {
        position.x = (center.x - (size.x * 0.5f));
    }
    else if (FACEOE_GFX_ALIGN_END == command->horizontal)
    {
        position.x = (box_max.x - size.x);
    }
    if (FACEOE_GFX_ALIGN_CENTER == command->vertical)
    {
        position.y = (center.y - (size.y * 0.5f));
    }
    else if (FACEOE_GFX_ALIGN_END == command->vertical)
    {
        position.y = (box_max.y - size.y);
    }
    vertex_start = target->VtxBuffer.Size;
    if (fabsf(command->angle) < 0.0001f)
    {
        clip = ImVec4(box_min.x, box_min.y, box_max.x, box_max.y);
        target->AddText(font, font_size, position, command->color, text, NULL, 0.0f, &clip);
    }
    else
    {
        target->AddText(font, font_size, position, command->color, text);
        cosine = cosf(command->angle);
        sine   = sinf(command->angle);
        for (vertex_index = vertex_start; vertex_index < target->VtxBuffer.Size; ++vertex_index)
        {
            dx = (target->VtxBuffer[vertex_index].pos.x - center.x);
            dy = (target->VtxBuffer[vertex_index].pos.y - center.y);
            target->VtxBuffer[vertex_index].pos.x = (center.x + (dx * cosine) - (dy * sine));
            target->VtxBuffer[vertex_index].pos.y = (center.y + (dx * sine) + (dy * cosine));
        }
    }
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Polygons go through the concave filler; lines through the polyline path with the
 *  recorded thickness.
 */
void Arinc661_Gfx_ImGui_Render(const FACEOE_Gfx_DrawList_t* drawList, ImDrawList* target)
{
    const FACEOE_Gfx_Command_t* command;
    ImVec2                      points[IMGUI_MAX_POLYGON_POINTS];
    ImDrawFlags                 flags;
    uint32_t                    command_index;
    uint32_t                    point_index;
    uint32_t                    point_count;
    uint32_t                    clip_depth;

    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != target);
    clip_depth = 0;
    for (command_index = 0; command_index < drawList->commandCount; ++command_index)
    {
        command     = &drawList->commands[command_index];
        point_count = command->pointCount;
        if (IMGUI_MAX_POLYGON_POINTS < point_count)
        {
            point_count = IMGUI_MAX_POLYGON_POINTS;
        }
        for (point_index = 0; point_index < point_count; ++point_index)
        {
            points[point_index] = ImVec2(drawList->points[command->firstPoint + point_index].x, drawList->points[command->firstPoint + point_index].y);
        }
        switch (command->kind)
        {
            case FACEOE_GFX_COMMAND_FILL_POLYGON:
                target->AddConcavePolyFilled(points, (int)point_count, command->color);
                break;
            case FACEOE_GFX_COMMAND_STROKE_POLYLINE:
                flags = ImDrawFlags_None;
                if (command->isClosed)
                {
                    flags = ImDrawFlags_Closed;
                }
                target->AddPolyline(points, (int)point_count, command->color, flags, command->thickness);
                break;
            case FACEOE_GFX_COMMAND_TEXT:
                RenderText(drawList, command, target);
                break;
            case FACEOE_GFX_COMMAND_PUSH_CLIP:
                target->PushClipRect(points[0], points[1], true);
                clip_depth += 1;
                break;
            case FACEOE_GFX_COMMAND_POP_CLIP:
                if (0 < clip_depth)
                {
                    target->PopClipRect();
                    clip_depth -= 1;
                }
                break;
            default:
                break;
        }
    }
    while (0 < clip_depth)
    {
        target->PopClipRect();
        clip_depth -= 1;
    }
}
