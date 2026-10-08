/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "faceoe_gfx/faceoe_gfx_drawlist.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Runs the FACEOE graphics draw list tests.
 *
 * \retval None
 */
void RunFACEOEGfxTests(void);

static FACEOE_Gfx_Command_t s_commands[256];
static FACEOE_Gfx_Point_t   s_points[4096];
static char_t               s_text[4096];

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks that two floats differ by less than 0.001.
 */
static bool_t Near(float32_t a, float32_t b)
{
    return (fabsf(a - b) < 0.001f);
}

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks transform composition, mapped command points, text boxes, clip depth, color fading,
 *  and that running out of storage sets isOverflowed without writing past the end.
 */
void RunFACEOEGfxTests(void)
{
    FACEOE_Gfx_DrawList_t  list;
    FACEOE_Gfx_Transform_t base;
    FACEOE_Gfx_Point_t     point;
    FACEOE_Gfx_Point_t     triangle[3];
    float32_t              unit_x;
    float32_t              unit_y;

    FACEOE_Gfx_DrawList_Initialize(&list, s_commands, 256, s_points, 4096, s_text, sizeof(s_text));
    // Layer units to screen: 0.1 px per unit, Y flipped, origin at (100, 500)
    memset(&base, 0, sizeof(base));
    base.a = 0.1f;
    base.d = -0.1f;
    base.e = 100.0f;
    base.f = 500.0f;
    FACEOE_Gfx_DrawList_SetTransform(&list, &base);
    point = FACEOE_Gfx_DrawList_Map(&list, 1000.0f, 2000.0f);
    assert(Near(point.x, 200.0f) && Near(point.y, 300.0f));
    FACEOE_Gfx_DrawList_UnitLengths(&list, &unit_x, &unit_y);
    assert(Near(unit_x, 0.1f) && Near(unit_y, 0.1f));

    // Translate then rotate 90 degrees counter-clockwise about the local origin
    assert(FACEOE_Gfx_DrawList_PushTransform(&list));
    FACEOE_Gfx_DrawList_Translate(&list, 1000.0f, 1000.0f);
    FACEOE_Gfx_DrawList_Rotate(&list, 0.0f, 0.0f, 1.5707963f);
    point = FACEOE_Gfx_DrawList_Map(&list, 100.0f, 0.0f);
    // Local +X turns into local +Y, which is screen up: (1000, 1100) -> (200, 390)
    assert(Near(point.x, 200.0f) && Near(point.y, 390.0f));
    assert(Near(FACEOE_Gfx_DrawList_Angle(&list), -1.5707963f));
    FACEOE_Gfx_DrawList_PopTransform(&list);
    assert(Near(FACEOE_Gfx_DrawList_Angle(&list), 0.0f));

    // Scale about a center keeps the center fixed
    assert(FACEOE_Gfx_DrawList_PushTransform(&list));
    FACEOE_Gfx_DrawList_Scale(&list, 500.0f, 500.0f, 2.0f, 2.0f);
    point = FACEOE_Gfx_DrawList_Map(&list, 500.0f, 500.0f);
    assert(Near(point.x, 150.0f) && Near(point.y, 450.0f));
    point = FACEOE_Gfx_DrawList_Map(&list, 600.0f, 500.0f);
    assert(Near(point.x, 170.0f));
    FACEOE_Gfx_DrawList_PopTransform(&list);

    // Commands
    FACEOE_Gfx_DrawList_FillRectangle(&list, 0.0f, 0.0f, 1000.0f, 1000.0f, FACEOE_GFX_RGBA(1, 2, 3, 255));
    assert(1 == list.commandCount);
    assert(FACEOE_GFX_COMMAND_FILL_POLYGON == list.commands[0].kind);
    assert(4 == list.commands[0].pointCount);
    assert(Near(list.points[2].x, 200.0f) && Near(list.points[2].y, 400.0f));
    triangle[0].x = 0.0f;
    triangle[0].y = 0.0f;
    triangle[1].x = 100.0f;
    triangle[1].y = 0.0f;
    triangle[2].x = 0.0f;
    triangle[2].y = 100.0f;
    FACEOE_Gfx_DrawList_StrokePolyline(&list, triangle, 3, TRUE, FACEOE_GFX_RGBA(9, 9, 9, 9), 2.0f);
    assert(2 == list.commandCount);
    assert(list.commands[1].isClosed);
    assert(Near(list.commands[1].thickness, 2.0f));
    FACEOE_Gfx_DrawList_DashedLine(&list, 0.0f, 0.0f, 1000.0f, 0.0f, 100.0f, 0xFFFFFFFFU, 1.0f);
    assert(7 == list.commandCount);
    FACEOE_Gfx_DrawList_Arc(&list, 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, 6.2831853f, FALSE, 0xFFFFFFFFU, 1.0f);
    assert(list.commands[7].isClosed);
    FACEOE_Gfx_DrawList_Arc(&list, 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, 1.0f, TRUE, 0xFFFFFFFFU, 1.0f);
    assert(FACEOE_GFX_COMMAND_FILL_POLYGON == list.commands[8].kind);
    FACEOE_Gfx_DrawList_Crown(&list, 0.0f, 0.0f, 50.0f, 100.0f, 0.0f, 3.0f, TRUE, 0xFFFFFFFFU, 1.0f);
    assert(FACEOE_GFX_COMMAND_FILL_POLYGON == list.commands[9].kind);
    FACEOE_Gfx_DrawList_Text(&list, 0.0f, 0.0f, 1000.0f, 200.0f, "Hello", FACEOE_GFX_ALIGN_END, FACEOE_GFX_ALIGN_START, 12.0f, 0.0f, 0xFFFFFFFFU);
    assert(FACEOE_GFX_COMMAND_TEXT == list.commands[10].kind);
    assert(0 == strncmp("Hello", FACEOE_Gfx_DrawList_CommandText(&list, &list.commands[10]), sizeof("Hello")));
    assert(FACEOE_GFX_ALIGN_END == list.commands[10].horizontal);
    // Box is reported top-left to bottom-right in output space
    assert(Near(list.points[list.commands[10].firstPoint].x, 100.0f) && Near(list.points[list.commands[10].firstPoint].y, 480.0f));
    assert(Near(list.points[list.commands[10].firstPoint + 1].x, 200.0f) && Near(list.points[list.commands[10].firstPoint + 1].y, 500.0f));
    FACEOE_Gfx_DrawList_PushClip(&list, 0.0f, 0.0f, 500.0f, 500.0f);
    assert(1 == list.clipDepth);
    FACEOE_Gfx_DrawList_PopClip(&list);
    assert(0 == list.clipDepth);
    FACEOE_Gfx_DrawList_PopClip(&list);
    assert(13 == list.commandCount);
    assert(FALSE == list.isOverflowed);
    assert(Near(FACEOE_Gfx_Color_Fade(FACEOE_GFX_RGBA(1, 2, 3, 200), 0.5f) >> 24, 100.0f));

    // Overflow is reported, not written past the end
    FACEOE_Gfx_DrawList_Initialize(&list, s_commands, 1, s_points, 4, s_text, 4);
    FACEOE_Gfx_DrawList_FillRectangle(&list, 0.0f, 0.0f, 1.0f, 1.0f, 0U);
    FACEOE_Gfx_DrawList_FillRectangle(&list, 0.0f, 0.0f, 1.0f, 1.0f, 0U);
    assert(1 == list.commandCount);
    assert(list.isOverflowed);
    FACEOE_Gfx_DrawList_Clear(&list);
    FACEOE_Gfx_DrawList_Text(&list, 0.0f, 0.0f, 1.0f, 1.0f, "toolong", FACEOE_GFX_ALIGN_START, FACEOE_GFX_ALIGN_START, 0.0f, 0.0f, 0U);
    assert(0 == list.commandCount);
    assert(list.isOverflowed);
}
