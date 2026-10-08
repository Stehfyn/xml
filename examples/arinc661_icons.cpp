/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661_icons.h"
#include "faceoe_utl/faceoe_utl_defines.h"

#include "imgui_internal.h"

#include <math.h>
#include <string.h>

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
#define SEGOE_TEXT_PATH        ("C:\\Windows\\Fonts\\segoeui.ttf")
#define SEGOE_ICON_PATH        ("C:\\Windows\\Fonts\\segmdl2.ttf")
#define ICON_GLYPH_FIRST       (0xE700)
#define ICON_GLYPH_LAST        (0xF8FF)
#define ICON_SCALE             (0.80f)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/** Segoe MDL2 Assets code point per icon, in Arinc661_Icon_t order; vertical variants reuse the
    horizontal glyph, rotated. */
static const ImWchar c_glyphs[] =
{
    0xE8A5, /* NEW: Document */
    0xE74E, /* SAVE */
    0xE78C, /* SAVE_ALL: SaveLocal */
    0xE7A7, /* UNDO */
    0xE7A6, /* REDO */
    0xE81E, /* ADD_LAYER: MapLayers */
    0xE710, /* ADD_WIDGET: Add */
    0xE723, /* ADD_EXTENSION: Attach */
    0xE8C8, /* DUPLICATE: Copy */
    0xE74D, /* DELETE */
    0xE80A, /* GRID: TiltDown */
    0xE718, /* SNAP: Pin */
    0xE9A6, /* FIT: FitPage */
    0xE897, /* HELP */
    0xE8A3, /* ZOOM_IN */
    0xE71F, /* ZOOM_OUT */
    0xE7F4, /* SCREEN: TVMonitor */
    0xE8A9, /* LAYOUT: ViewAll */
    0xE7C4, /* WINDOW: TaskView */
    0xE8B7, /* APPLICATION: Folder */
    0xE8E4, /* ALIGN_LEFT: AlignLeft */
    0xE8E2, /* ALIGN_RIGHT: AlignRight */
    0xE8E4, /* ALIGN_TOP: rotated AlignLeft */
    0xE8E2, /* ALIGN_BOTTOM: rotated AlignRight */
    0xE8E3, /* ALIGN_CENTER_X: AlignCenter */
    0xE8E3, /* ALIGN_CENTER_Y: rotated AlignCenter */
    0xE8EB, /* DISTRIBUTE_X: Reshare */
    0xE8EB, /* DISTRIBUTE_Y: rotated Reshare */
    0xE71E, /* ZOOM_SELECTION: Zoom */
    0xE890, /* EYE: View */
    0xE72E, /* LOCK */
    0xE8C8, /* COPY */
    0xE77F, /* PASTE */
    0xE721, /* SEARCH */
    0xE768, /* PLAY */
    0xE90F  /* BUILD: Repair */
};

static bool_t s_hasIconFont;

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Draws the primitive version of an icon.
 *
 * \param[in,out] drawList   Target draw list
 * \param[in]     icon       Icon to draw
 * \param[in]     minimum    Top-left of the icon box
 * \param[in]     maximum    Bottom-right of the icon box
 * \param[in]     color      Icon color
 *
 * \retval        None
 */
static void DrawPrimitiveIcon(ImDrawList* drawList, Arinc661_Icon_t icon, ImVec2 minimum, ImVec2 maximum, ImU32 color);

/*************************************************************************************************/
/**
 * \brief Encodes a code point as UTF-8.
 *
 * \param[in]  codePoint   Unicode code point below 0x10000
 * \param[out] text        At least four bytes
 *
 * \retval     None
 */
static void EncodeUtf8(ImWchar codePoint, char_t* text);

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The icon font's private use area code points take three bytes.
 */
static void EncodeUtf8(ImWchar codePoint, char_t* text)
{
    if (codePoint < 0x80)
    {
        text[0] = (char_t)codePoint;
        text[1] = '\0';
    }
    else if (codePoint < 0x800)
    {
        text[0] = (char_t)(0xC0 | (codePoint >> 6));
        text[1] = (char_t)(0x80 | (codePoint & 0x3F));
        text[2] = '\0';
    }
    else
    {
        text[0] = (char_t)(0xE0 | (codePoint >> 12));
        text[1] = (char_t)(0x80 | ((codePoint >> 6) & 0x3F));
        text[2] = (char_t)(0x80 | (codePoint & 0x3F));
        text[3] = '\0';
    }
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The text font becomes the default; the icon font is merged at ICON_SCALE of the text size,
 *  one pixel lower so glyphs sit on the text baseline.
 */
bool_t Arinc661_Icons_LoadFonts(float32_t textSize)
{
    static const ImWchar c_iconRanges[] = {ICON_GLYPH_FIRST, ICON_GLYPH_LAST, 0};
    ImGuiIO*     io;
    ImFontConfig text_config;
    ImFontConfig icon_config;
    ImFont*      text_font;
    ImFont*      icon_font;

    io = &ImGui::GetIO();
    io->Fonts->Clear();
    text_font = io->Fonts->AddFontFromFileTTF(SEGOE_TEXT_PATH, textSize, &text_config);
    if (NULL == text_font)
    {
        text_font = io->Fonts->AddFontDefault();
    }
    icon_config.MergeMode   = true;
    icon_config.GlyphOffset = ImVec2(0.0f, 1.0f);
    icon_config.GlyphRanges = c_iconRanges;
    icon_font = io->Fonts->AddFontFromFileTTF(SEGOE_ICON_PATH, textSize * ICON_SCALE, &icon_config, c_iconRanges);
    s_hasIconFont = (NULL != icon_font);
    io->FontDefault = text_font;

    return s_hasIconFont;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The glyph is centered in the box; vertical align and distribute icons reuse the horizontal
 *  glyph, rotated a quarter turn. Missing glyphs fall back to primitives.
 */
void Arinc661_Icons_Draw(ImDrawList* drawList, Arinc661_Icon_t icon, ImVec2 minimum, ImVec2 maximum, ImU32 color)
{
    ImFont*   font;
    char_t    text[4];
    ImVec2    size;
    ImVec2    position;
    ImVec2    center;
    float32_t font_size;
    float32_t dx;
    float32_t dy;
    int32_t   vertex_start;
    int32_t   vertex_index;
    bool_t    is_rotated;

    font = ImGui::GetFont();
    if (s_hasIconFont && ((uint32_t)icon < COUNT_OF(c_glyphs)) && font->IsGlyphInFont(c_glyphs[icon]))
    {
        EncodeUtf8(c_glyphs[icon], text);
        font_size    = ((maximum.y - minimum.y) * 1.45f);
        size         = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);
        center       = ImVec2((minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f);
        position     = ImVec2(center.x - (size.x * 0.5f), center.y - (size.y * 0.5f));
        is_rotated   = !!((ARINC661_ICON_ALIGN_TOP == icon) || (ARINC661_ICON_ALIGN_BOTTOM == icon) || (ARINC661_ICON_ALIGN_CENTER_Y == icon) || (ARINC661_ICON_DISTRIBUTE_Y == icon));
        vertex_start = drawList->VtxBuffer.Size;
        drawList->AddText(font, font_size, position, color, text);
        if (is_rotated)
        {
            // Quarter turn clockwise about the box center
            for (vertex_index = vertex_start; vertex_index < drawList->VtxBuffer.Size; ++vertex_index)
            {
                dx = (drawList->VtxBuffer[vertex_index].pos.x - center.x);
                dy = (drawList->VtxBuffer[vertex_index].pos.y - center.y);
                drawList->VtxBuffer[vertex_index].pos.x = (center.x - dy);
                drawList->VtxBuffer[vertex_index].pos.y = (center.y + dx);
            }
        }
    }
    else
    {
        DrawPrimitiveIcon(drawList, icon, minimum, maximum, color);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Shapes are scaled to the box width; only the help icon uses the current font, for its "?".
 */
static void DrawPrimitiveIcon(ImDrawList* drawList, Arinc661_Icon_t icon, ImVec2 minimum, ImVec2 maximum, ImU32 color)
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
        case ARINC661_ICON_ALIGN_LEFT:
        case ARINC661_ICON_ALIGN_RIGHT:
        case ARINC661_ICON_ALIGN_CENTER_X:
            // A vertical guide with two bars aligned to it
            if (ARINC661_ICON_ALIGN_LEFT == icon)
            {
                drawList->AddLine(ImVec2(minimum.x, minimum.y), ImVec2(minimum.x, maximum.y), color, 1.5f);
                drawList->AddRectFilled(ImVec2(minimum.x + unit, minimum.y + unit), ImVec2(maximum.x, minimum.y + (3.0f * unit)), color);
                drawList->AddRectFilled(ImVec2(minimum.x + unit, maximum.y - (3.0f * unit)), ImVec2(center.x + unit, maximum.y - unit), color);
            }
            else if (ARINC661_ICON_ALIGN_RIGHT == icon)
            {
                drawList->AddLine(ImVec2(maximum.x, minimum.y), ImVec2(maximum.x, maximum.y), color, 1.5f);
                drawList->AddRectFilled(ImVec2(minimum.x, minimum.y + unit), ImVec2(maximum.x - unit, minimum.y + (3.0f * unit)), color);
                drawList->AddRectFilled(ImVec2(center.x - unit, maximum.y - (3.0f * unit)), ImVec2(maximum.x - unit, maximum.y - unit), color);
            }
            else
            {
                drawList->AddLine(ImVec2(center.x, minimum.y), ImVec2(center.x, maximum.y), color, 1.5f);
                drawList->AddRectFilled(ImVec2(minimum.x, minimum.y + unit), ImVec2(maximum.x, minimum.y + (3.0f * unit)), color);
                drawList->AddRectFilled(ImVec2(center.x - (2.0f * unit), maximum.y - (3.0f * unit)), ImVec2(center.x + (2.0f * unit), maximum.y - unit), color);
            }
            break;
        case ARINC661_ICON_ALIGN_TOP:
        case ARINC661_ICON_ALIGN_BOTTOM:
        case ARINC661_ICON_ALIGN_CENTER_Y:
            if (ARINC661_ICON_ALIGN_TOP == icon)
            {
                drawList->AddLine(ImVec2(minimum.x, minimum.y), ImVec2(maximum.x, minimum.y), color, 1.5f);
                drawList->AddRectFilled(ImVec2(minimum.x + unit, minimum.y + unit), ImVec2(minimum.x + (3.0f * unit), maximum.y), color);
                drawList->AddRectFilled(ImVec2(maximum.x - (3.0f * unit), minimum.y + unit), ImVec2(maximum.x - unit, center.y + unit), color);
            }
            else if (ARINC661_ICON_ALIGN_BOTTOM == icon)
            {
                drawList->AddLine(ImVec2(minimum.x, maximum.y), ImVec2(maximum.x, maximum.y), color, 1.5f);
                drawList->AddRectFilled(ImVec2(minimum.x + unit, minimum.y), ImVec2(minimum.x + (3.0f * unit), maximum.y - unit), color);
                drawList->AddRectFilled(ImVec2(maximum.x - (3.0f * unit), center.y - unit), ImVec2(maximum.x - unit, maximum.y - unit), color);
            }
            else
            {
                drawList->AddLine(ImVec2(minimum.x, center.y), ImVec2(maximum.x, center.y), color, 1.5f);
                drawList->AddRectFilled(ImVec2(minimum.x + unit, minimum.y), ImVec2(minimum.x + (3.0f * unit), maximum.y), color);
                drawList->AddRectFilled(ImVec2(maximum.x - (3.0f * unit), center.y - (2.0f * unit)), ImVec2(maximum.x - unit, center.y + (2.0f * unit)), color);
            }
            break;
        case ARINC661_ICON_DISTRIBUTE_X:
            drawList->AddRectFilled(ImVec2(minimum.x, minimum.y + unit), ImVec2(minimum.x + (1.5f * unit), maximum.y - unit), color);
            drawList->AddRectFilled(ImVec2(center.x - (0.75f * unit), minimum.y + unit), ImVec2(center.x + (0.75f * unit), maximum.y - unit), color);
            drawList->AddRectFilled(ImVec2(maximum.x - (1.5f * unit), minimum.y + unit), ImVec2(maximum.x, maximum.y - unit), color);
            break;
        case ARINC661_ICON_DISTRIBUTE_Y:
            drawList->AddRectFilled(ImVec2(minimum.x + unit, minimum.y), ImVec2(maximum.x - unit, minimum.y + (1.5f * unit)), color);
            drawList->AddRectFilled(ImVec2(minimum.x + unit, center.y - (0.75f * unit)), ImVec2(maximum.x - unit, center.y + (0.75f * unit)), color);
            drawList->AddRectFilled(ImVec2(minimum.x + unit, maximum.y - (1.5f * unit)), ImVec2(maximum.x - unit, maximum.y), color);
            break;
        case ARINC661_ICON_ZOOM_SELECTION:
            // Brackets around a filled box
            drawList->AddRectFilled(ImVec2(center.x - (2.0f * unit), center.y - (2.0f * unit)), ImVec2(center.x + (2.0f * unit), center.y + (2.0f * unit)), color);
            drawList->AddLine(ImVec2(minimum.x, minimum.y + (2.0f * unit)), minimum, color, 1.5f);
            drawList->AddLine(minimum, ImVec2(minimum.x + (2.0f * unit), minimum.y), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x - (2.0f * unit), minimum.y), ImVec2(maximum.x, minimum.y), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x, minimum.y), ImVec2(maximum.x, minimum.y + (2.0f * unit)), color, 1.5f);
            drawList->AddLine(ImVec2(minimum.x, maximum.y - (2.0f * unit)), ImVec2(minimum.x, maximum.y), color, 1.5f);
            drawList->AddLine(ImVec2(minimum.x, maximum.y), ImVec2(minimum.x + (2.0f * unit), maximum.y), color, 1.5f);
            drawList->AddLine(ImVec2(maximum.x - (2.0f * unit), maximum.y), maximum, color, 1.5f);
            drawList->AddLine(maximum, ImVec2(maximum.x, maximum.y - (2.0f * unit)), color, 1.5f);
            break;
        case ARINC661_ICON_EYE:
            drawList->AddEllipse(center, ImVec2(size.x * 0.5f, size.y * 0.3f), color, 0.0f, 0, 1.5f);
            drawList->AddCircleFilled(center, 1.5f * unit, color);
            break;
        case ARINC661_ICON_LOCK:
            drawList->AddRectFilled(ImVec2(minimum.x + unit, center.y - unit), ImVec2(maximum.x - unit, maximum.y), color);
            drawList->PathArcTo(ImVec2(center.x, center.y - unit), 2.0f * unit, 3.14159f, 6.28318f, 10);
            drawList->PathStroke(color, 0, 1.5f);
            break;
        case ARINC661_ICON_COPY:
            drawList->AddRect(ImVec2(minimum.x, minimum.y + (2.0f * unit)), ImVec2(maximum.x - (2.0f * unit), maximum.y), color, 1.0f, 0, 1.5f);
            drawList->AddRect(ImVec2(minimum.x + (2.0f * unit), minimum.y), ImVec2(maximum.x, maximum.y - (2.0f * unit)), color);
            break;
        case ARINC661_ICON_PASTE:
            drawList->AddRect(ImVec2(minimum.x + unit, minimum.y + unit), ImVec2(maximum.x - unit, maximum.y), color, 1.0f, 0, 1.5f);
            drawList->AddRectFilled(ImVec2(center.x - (2.0f * unit), minimum.y), ImVec2(center.x + (2.0f * unit), minimum.y + (2.0f * unit)), color);
            drawList->AddLine(ImVec2(center.x - (2.0f * unit), center.y), ImVec2(center.x + (2.0f * unit), center.y), color);
            drawList->AddLine(ImVec2(center.x - (2.0f * unit), center.y + (2.0f * unit)), ImVec2(center.x + (2.0f * unit), center.y + (2.0f * unit)), color);
            break;
        case ARINC661_ICON_SEARCH:
            drawList->AddCircle(ImVec2(center.x - unit, center.y - unit), 2.8f * unit, color, 0, 1.5f);
            drawList->AddLine(ImVec2(center.x + unit, center.y + unit), maximum, color, 2.0f);
            break;
        case ARINC661_ICON_PLAY:
            drawList->AddTriangleFilled(ImVec2(minimum.x + unit, minimum.y), ImVec2(maximum.x, center.y), ImVec2(minimum.x + unit, maximum.y), color);
            break;
        case ARINC661_ICON_BUILD:
            drawList->AddRect(minimum, maximum, color, 1.0f, 0, 1.5f);
            drawList->AddLine(ImVec2(minimum.x, center.y), ImVec2(maximum.x, center.y), color);
            drawList->AddLine(ImVec2(center.x, minimum.y), ImVec2(center.x, maximum.y), color);
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Wraps to a new row when the button does not fit. The tooltip shows the label and the
 *  shortcut, if any, also while disabled.
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

    // Wrap rather than clip
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
 *  A short vertical line with spacing on both sides; wraps to a new row when the gap and the
 *  next button do not fit.
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
