/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * Retained 2-D draw list; see faceoe_gfx_drawlist.h.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "faceoe_gfx/faceoe_gfx_drawlist.h"

#include "os/os_assert.h"

#include <math.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARC_MAX_POINTS      (130)
#define ARC_SEGMENT_RADIANS (0.0873f) /* about 5 degrees */
#define TWO_PI              (6.28318530718f)

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

static FACEOE_Gfx_Transform_t* CurrentTransform(FACEOE_Gfx_DrawList_t* drawList);
static FACEOE_Gfx_Command_t* AppendCommand(FACEOE_Gfx_DrawList_t* drawList, FACEOE_Gfx_CommandKind_t kind, const FACEOE_Gfx_Point_t* points, uint32_t count);
static uint32_t ArcPoints(float32_t centerX, float32_t centerY, float32_t radiusX, float32_t radiusY, float32_t startRadians, float32_t endRadians, FACEOE_Gfx_Point_t* points, uint32_t capacity);
static void BoxPoints(float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Point_t* points);
static void MappedBox(const FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Point_t* box);
static FACEOE_Gfx_Command_t* AppendMappedCommand(FACEOE_Gfx_DrawList_t* drawList, FACEOE_Gfx_CommandKind_t kind, const FACEOE_Gfx_Point_t* points, uint32_t count);

/*=================================================================================================
** 4. STATIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Returns the top of the transform stack.
 */
static FACEOE_Gfx_Transform_t* CurrentTransform(FACEOE_Gfx_DrawList_t* drawList)
{
    return &drawList->transforms[drawList->transformDepth];
}

/*************************************************************************************************/
/**
 * \brief Maps the points through the current transform and appends a command.
 *
 * \retval FACEOE_Gfx_Command_t*   New command, or NULL when storage is exhausted.
 */
static FACEOE_Gfx_Command_t* AppendCommand(FACEOE_Gfx_DrawList_t* drawList, FACEOE_Gfx_CommandKind_t kind, const FACEOE_Gfx_Point_t* points, uint32_t count)
{
    FACEOE_Gfx_Command_t* command;
    uint32_t              point_index;

    command = NULL;
    if ((drawList->commandCount < drawList->commandCapacity) && ((drawList->pointCount + count) <= drawList->pointCapacity))
    {
        command = &drawList->commands[drawList->commandCount];
        (void)memset(command, 0, sizeof((*command)));
        command->kind       = kind;
        command->firstPoint = drawList->pointCount;
        command->pointCount = count;
        for (point_index = 0; point_index < count; ++point_index)
        {
            drawList->points[drawList->pointCount + point_index] = FACEOE_Gfx_DrawList_Map(drawList, points[point_index].x, points[point_index].y);
        }
        drawList->pointCount += count;
        drawList->commandCount += 1;
    }
    else
    {
        drawList->isOverflowed = TRUE;
    }

    return command;
}

/*************************************************************************************************/
/**
 * \brief Samples an elliptical arc counter-clockwise from the start to the end angle.
 *
 * \retval uint32_t   Points written.
 */
static uint32_t ArcPoints(float32_t centerX, float32_t centerY, float32_t radiusX, float32_t radiusY, float32_t startRadians, float32_t endRadians, FACEOE_Gfx_Point_t* points, uint32_t capacity)
{
    float32_t sweep;
    float32_t angle;
    uint32_t  segment_count;
    uint32_t  point_index;

    sweep = (endRadians - startRadians);
    if (sweep < 0.0f)
    {
        sweep += TWO_PI;
    }
    if (TWO_PI < sweep)
    {
        sweep = TWO_PI;
    }
    segment_count = (uint32_t)ceilf(sweep / ARC_SEGMENT_RADIANS);
    if (segment_count < 1)
    {
        segment_count = 1;
    }
    if ((capacity - 1) < segment_count)
    {
        segment_count = (capacity - 1);
    }
    for (point_index = 0; point_index <= segment_count; ++point_index)
    {
        angle                 = startRadians + (sweep * ((float32_t)point_index / (float32_t)segment_count));
        points[point_index].x = centerX + (radiusX * cosf(angle));
        points[point_index].y = centerY + (radiusY * sinf(angle));
    }

    return (segment_count + 1);
}

/*************************************************************************************************/
/**
 * \brief Fills the four corners of a local box.
 */
static void BoxPoints(float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Point_t* points)
{
    points[0].x = x;
    points[0].y = y;
    points[1].x = (x + width);
    points[1].y = y;
    points[2].x = (x + width);
    points[2].y = (y + height);
    points[3].x = x;
    points[3].y = (y + height);
}

/*************************************************************************************************/
/**
 * rief Maps a local box and returns its axis-aligned bounds as two points.
 */
static void MappedBox(const FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Point_t* box)
{
    FACEOE_Gfx_Point_t corners[4];
    uint32_t           corner_index;

    BoxPoints(x, y, width, height, corners);
    for (corner_index = 0; corner_index < 4; ++corner_index)
    {
        corners[corner_index] = FACEOE_Gfx_DrawList_Map(drawList, corners[corner_index].x, corners[corner_index].y);
    }
    box[0] = corners[0];
    box[1] = corners[0];
    for (corner_index = 1; corner_index < 4; ++corner_index)
    {
        if (corners[corner_index].x < box[0].x)
        {
            box[0].x = corners[corner_index].x;
        }
        if (corners[corner_index].y < box[0].y)
        {
            box[0].y = corners[corner_index].y;
        }
        if (box[1].x < corners[corner_index].x)
        {
            box[1].x = corners[corner_index].x;
        }
        if (box[1].y < corners[corner_index].y)
        {
            box[1].y = corners[corner_index].y;
        }
    }
}

/*************************************************************************************************/
/**
 * rief Appends a command whose points are already in output units.
 */
static FACEOE_Gfx_Command_t* AppendMappedCommand(FACEOE_Gfx_DrawList_t* drawList, FACEOE_Gfx_CommandKind_t kind, const FACEOE_Gfx_Point_t* points, uint32_t count)
{
    FACEOE_Gfx_Command_t* command;
    uint32_t              point_index;

    command = NULL;
    if ((drawList->commandCount < drawList->commandCapacity) && ((drawList->pointCount + count) <= drawList->pointCapacity))
    {
        command = &drawList->commands[drawList->commandCount];
        (void)memset(command, 0, sizeof((*command)));
        command->kind       = kind;
        command->firstPoint = drawList->pointCount;
        command->pointCount = count;
        for (point_index = 0; point_index < count; ++point_index)
        {
            drawList->points[drawList->pointCount + point_index] = points[point_index];
        }
        drawList->pointCount += count;
        drawList->commandCount += 1;
    }
    else
    {
        drawList->isOverflowed = TRUE;
    }

    return command;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Storage pointers may be NULL only with a zero capacity.
 */
void FACEOE_Gfx_DrawList_Initialize(FACEOE_Gfx_DrawList_t* drawList, FACEOE_Gfx_Command_t* commands, uint32_t commandCapacity, FACEOE_Gfx_Point_t* points, uint32_t pointCapacity, char_t* text, uint32_t textCapacity)
{
    LOGASSERT(NULL != drawList);
    LOGASSERT((NULL != commands) || (0 == commandCapacity));
    LOGASSERT((NULL != points) || (0 == pointCapacity));
    LOGASSERT((NULL != text) || (0 == textCapacity));
    (void)memset(drawList, 0, sizeof((*drawList)));
    drawList->commands        = commands;
    drawList->points          = points;
    drawList->text            = text;
    drawList->commandCapacity = commandCapacity;
    drawList->pointCapacity   = pointCapacity;
    drawList->textCapacity    = textCapacity;
    FACEOE_Gfx_DrawList_Clear(drawList);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Keeps the attached storage.
 */
void FACEOE_Gfx_DrawList_Clear(FACEOE_Gfx_DrawList_t* drawList)
{
    LOGASSERT(NULL != drawList);
    drawList->commandCount   = 0;
    drawList->pointCount     = 0;
    drawList->textByteCount  = 0;
    drawList->isOverflowed   = FALSE;
    drawList->transformDepth = 0;
    drawList->clipDepth      = 0;
    (void)memset(&drawList->transforms[0], 0, sizeof(drawList->transforms[0]));
    drawList->transforms[0].a = 1.0f;
    drawList->transforms[0].d = 1.0f;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Replaces only the top of the stack.
 */
void FACEOE_Gfx_DrawList_SetTransform(FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Transform_t* transform)
{
    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != transform);
    (*CurrentTransform(drawList)) = (*transform);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns the top of the stack.
 */
const FACEOE_Gfx_Transform_t* FACEOE_Gfx_DrawList_Transform(const FACEOE_Gfx_DrawList_t* drawList)
{
    LOGASSERT(NULL != drawList);

    return &drawList->transforms[drawList->transformDepth];
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fails when the stack is full so a producer can skip the matching pop.
 */
bool_t FACEOE_Gfx_DrawList_PushTransform(FACEOE_Gfx_DrawList_t* drawList)
{
    bool_t has_room;

    LOGASSERT(NULL != drawList);
    has_room = ((drawList->transformDepth + 1) < FACEOE_GFX_MAX_TRANSFORM_DEPTH);
    if (has_room)
    {
        drawList->transforms[drawList->transformDepth + 1] = drawList->transforms[drawList->transformDepth];
        drawList->transformDepth += 1;
    }

    return has_room;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The base transform is never popped.
 */
void FACEOE_Gfx_DrawList_PopTransform(FACEOE_Gfx_DrawList_t* drawList)
{
    LOGASSERT(NULL != drawList);
    LOGASSERT(0 < drawList->transformDepth);
    drawList->transformDepth -= 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  M' = M * T(x, y).
 */
void FACEOE_Gfx_DrawList_Translate(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y)
{
    FACEOE_Gfx_Transform_t* transform;

    LOGASSERT(NULL != drawList);
    transform    = CurrentTransform(drawList);
    transform->e = (transform->a * x) + (transform->c * y) + transform->e;
    transform->f = (transform->b * x) + (transform->d * y) + transform->f;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  M' = M * T(c) * R(angle) * T(-c), with R counter-clockwise in local units.
 */
void FACEOE_Gfx_DrawList_Rotate(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t radians)
{
    FACEOE_Gfx_Transform_t* transform;
    float32_t               cosine;
    float32_t               sine;
    float32_t               a;
    float32_t               b;
    float32_t               c;
    float32_t               d;

    LOGASSERT(NULL != drawList);
    FACEOE_Gfx_DrawList_Translate(drawList, centerX, centerY);
    transform    = CurrentTransform(drawList);
    cosine       = cosf(radians);
    sine         = sinf(radians);
    a            = transform->a;
    b            = transform->b;
    c            = transform->c;
    d            = transform->d;
    transform->a = (a * cosine) + (c * sine);
    transform->b = (b * cosine) + (d * sine);
    transform->c = (c * cosine) - (a * sine);
    transform->d = (d * cosine) - (b * sine);
    FACEOE_Gfx_DrawList_Translate(drawList, -centerX, -centerY);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  M' = M * T(c) * S * T(-c).
 */
void FACEOE_Gfx_DrawList_Scale(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t scaleX, float32_t scaleY)
{
    FACEOE_Gfx_Transform_t* transform;

    LOGASSERT(NULL != drawList);
    FACEOE_Gfx_DrawList_Translate(drawList, centerX, centerY);
    transform = CurrentTransform(drawList);
    transform->a *= scaleX;
    transform->b *= scaleX;
    transform->c *= scaleY;
    transform->d *= scaleY;
    FACEOE_Gfx_DrawList_Translate(drawList, -centerX, -centerY);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Applies the top transform.
 */
FACEOE_Gfx_Point_t FACEOE_Gfx_DrawList_Map(const FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y)
{
    const FACEOE_Gfx_Transform_t* transform;
    FACEOE_Gfx_Point_t            point;

    LOGASSERT(NULL != drawList);
    transform = &drawList->transforms[drawList->transformDepth];
    point.x   = (transform->a * x) + (transform->c * y) + transform->e;
    point.y   = (transform->b * x) + (transform->d * y) + transform->f;

    return point;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Lengths of the mapped unit vectors.
 */
void FACEOE_Gfx_DrawList_UnitLengths(const FACEOE_Gfx_DrawList_t* drawList, float32_t* unitX, float32_t* unitY)
{
    const FACEOE_Gfx_Transform_t* transform;

    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != unitX);
    LOGASSERT(NULL != unitY);
    transform = &drawList->transforms[drawList->transformDepth];
    (*unitX)  = sqrtf((transform->a * transform->a) + (transform->b * transform->b));
    (*unitY)  = sqrtf((transform->c * transform->c) + (transform->d * transform->d));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Direction of the mapped local X axis.
 */
float32_t FACEOE_Gfx_DrawList_Angle(const FACEOE_Gfx_DrawList_t* drawList)
{
    const FACEOE_Gfx_Transform_t* transform;

    LOGASSERT(NULL != drawList);
    transform = &drawList->transforms[drawList->transformDepth];

    return atan2f(transform->b, transform->a);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fewer than three points record nothing.
 */
void FACEOE_Gfx_DrawList_FillPolygon(FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Point_t* points, uint32_t count, FACEOE_Gfx_Color_t color)
{
    FACEOE_Gfx_Command_t* command;

    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != points);
    if (3 <= count)
    {
        command = AppendCommand(drawList, FACEOE_GFX_COMMAND_FILL_POLYGON, points, count);
        if (NULL != command)
        {
            command->color = color;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Fewer than two points record nothing.
 */
void FACEOE_Gfx_DrawList_StrokePolyline(FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Point_t* points, uint32_t count, bool_t isClosed, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    FACEOE_Gfx_Command_t* command;

    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != points);
    if (2 <= count)
    {
        command = AppendCommand(drawList, FACEOE_GFX_COMMAND_STROKE_POLYLINE, points, count);
        if (NULL != command)
        {
            command->color     = color;
            command->thickness = thickness;
            command->isClosed  = isClosed;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Four corners, counter-clockwise in local units.
 */
void FACEOE_Gfx_DrawList_FillRectangle(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color)
{
    FACEOE_Gfx_Point_t points[4];

    BoxPoints(x, y, width, height, points);
    FACEOE_Gfx_DrawList_FillPolygon(drawList, points, 4, color);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Four corners, closed.
 */
void FACEOE_Gfx_DrawList_StrokeRectangle(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    FACEOE_Gfx_Point_t points[4];

    BoxPoints(x, y, width, height, points);
    FACEOE_Gfx_DrawList_StrokePolyline(drawList, points, 4, TRUE, color, thickness);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Two-point open polyline.
 */
void FACEOE_Gfx_DrawList_Line(FACEOE_Gfx_DrawList_t* drawList, float32_t x1, float32_t y1, float32_t x2, float32_t y2, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    FACEOE_Gfx_Point_t points[2];

    points[0].x = x1;
    points[0].y = y1;
    points[1].x = x2;
    points[1].y = y2;
    FACEOE_Gfx_DrawList_StrokePolyline(drawList, points, 2, FALSE, color, thickness);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks the segment in dash-length steps; a non-positive dash length draws a solid line.
 */
void FACEOE_Gfx_DrawList_DashedLine(FACEOE_Gfx_DrawList_t* drawList, float32_t x1, float32_t y1, float32_t x2, float32_t y2, float32_t dashLength, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    float32_t length;
    float32_t direction_x;
    float32_t direction_y;
    float32_t start;
    float32_t end;

    LOGASSERT(NULL != drawList);
    length = sqrtf(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)));
    if ((dashLength <= 0.0f) || (length <= dashLength))
    {
        FACEOE_Gfx_DrawList_Line(drawList, x1, y1, x2, y2, color, thickness);
    }
    else
    {
        direction_x = ((x2 - x1) / length);
        direction_y = ((y2 - y1) / length);
        for (start = 0.0f; start < length; start += (2.0f * dashLength))
        {
            end = (start + dashLength);
            if (length < end)
            {
                end = length;
            }
            FACEOE_Gfx_DrawList_Line(drawList, x1 + (direction_x * start), y1 + (direction_y * start), x1 + (direction_x * end), y1 + (direction_y * end), color, thickness);
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Four dashed edges.
 */
void FACEOE_Gfx_DrawList_DashedRectangle(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, float32_t dashLength, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    FACEOE_Gfx_DrawList_DashedLine(drawList, x, y, x + width, y, dashLength, color, thickness);
    FACEOE_Gfx_DrawList_DashedLine(drawList, x + width, y, x + width, y + height, dashLength, color, thickness);
    FACEOE_Gfx_DrawList_DashedLine(drawList, x + width, y + height, x, y + height, dashLength, color, thickness);
    FACEOE_Gfx_DrawList_DashedLine(drawList, x, y + height, x, y, dashLength, color, thickness);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A filled sector adds the centre unless the arc is a full turn.
 */
void FACEOE_Gfx_DrawList_Arc(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t radiusX, float32_t radiusY, float32_t startRadians, float32_t endRadians, bool_t isFilled, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    FACEOE_Gfx_Point_t points[ARC_MAX_POINTS + 1];
    uint32_t           count;
    bool_t             is_full_turn;

    LOGASSERT(NULL != drawList);
    count        = ArcPoints(centerX, centerY, radiusX, radiusY, startRadians, endRadians, points, ARC_MAX_POINTS);
    is_full_turn = ((TWO_PI - 0.0001f) <= (endRadians - startRadians));
    if (isFilled)
    {
        if (FALSE == is_full_turn)
        {
            points[count].x = centerX;
            points[count].y = centerY;
            count += 1;
        }
        FACEOE_Gfx_DrawList_FillPolygon(drawList, points, count, color);
    }
    else
    {
        FACEOE_Gfx_DrawList_StrokePolyline(drawList, points, count, is_full_turn, color, thickness);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Outer arc forward, inner arc backward.
 */
void FACEOE_Gfx_DrawList_Crown(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t innerRadius, float32_t outerRadius, float32_t startRadians, float32_t endRadians, bool_t isFilled, FACEOE_Gfx_Color_t color, float32_t thickness)
{
    FACEOE_Gfx_Point_t points[(2 * ARC_MAX_POINTS)];
    uint32_t           outer_count;
    uint32_t           inner_count;
    uint32_t           point_index;
    FACEOE_Gfx_Point_t swap;

    LOGASSERT(NULL != drawList);
    outer_count = ArcPoints(centerX, centerY, outerRadius, outerRadius, startRadians, endRadians, points, ARC_MAX_POINTS);
    inner_count = ArcPoints(centerX, centerY, innerRadius, innerRadius, startRadians, endRadians, &points[outer_count], ARC_MAX_POINTS);
    // Reverse the inner arc so the outline runs around the ring
    for (point_index = 0; point_index < (inner_count / 2); ++point_index)
    {
        swap                                                   = points[outer_count + point_index];
        points[outer_count + point_index]                      = points[outer_count + inner_count - 1 - point_index];
        points[outer_count + inner_count - 1 - point_index]    = swap;
    }
    if (isFilled)
    {
        FACEOE_Gfx_DrawList_FillPolygon(drawList, points, outer_count + inner_count, color);
    }
    else
    {
        FACEOE_Gfx_DrawList_StrokePolyline(drawList, points, outer_count + inner_count, TRUE, color, thickness);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Stores the mapped box corners as two points plus the text bytes; text longer than the
 *  storage is truncated at the last whole byte.
 */
void FACEOE_Gfx_DrawList_Text(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* text, FACEOE_Gfx_Align_t horizontal, FACEOE_Gfx_Align_t vertical, float32_t fontSize, float32_t radians, FACEOE_Gfx_Color_t color)
{
    FACEOE_Gfx_Command_t* command;
    FACEOE_Gfx_Point_t    box[2];
    size_t                length;

    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != text);
    length = strlen(text);
    if ((FACEOE_GFX_MAX_TEXT_BYTES - 1) < length)
    {
        length = (FACEOE_GFX_MAX_TEXT_BYTES - 1);
    }
    if ((drawList->textByteCount + length + 1) <= drawList->textCapacity)
    {
        MappedBox(drawList, x, y, width, height, box);
        command = AppendMappedCommand(drawList, FACEOE_GFX_COMMAND_TEXT, box, 2);
        if (NULL != command)
        {
            command->color      = color;
            command->fontSize   = fontSize;
            command->horizontal = horizontal;
            command->vertical   = vertical;
            command->angle      = (FACEOE_Gfx_DrawList_Angle(drawList) - radians);
            command->textOffset = drawList->textByteCount;
            (void)memcpy(&drawList->text[drawList->textByteCount], text, length);
            drawList->text[drawList->textByteCount + length] = '\0';
            drawList->textByteCount += (uint32_t)(length + 1);
        }
    }
    else
    {
        drawList->isOverflowed = TRUE;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Records the mapped bounds of the box; a renderer intersects with its current clip.
 */
void FACEOE_Gfx_DrawList_PushClip(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height)
{
    FACEOE_Gfx_Point_t box[2];

    LOGASSERT(NULL != drawList);
    MappedBox(drawList, x, y, width, height, box);
    if (NULL != AppendMappedCommand(drawList, FACEOE_GFX_COMMAND_PUSH_CLIP, box, 2))
    {
        drawList->clipDepth += 1;
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Records nothing when no clip is open.
 */
void FACEOE_Gfx_DrawList_PopClip(FACEOE_Gfx_DrawList_t* drawList)
{
    LOGASSERT(NULL != drawList);
    if (0 < drawList->clipDepth)
    {
        if (NULL != AppendCommand(drawList, FACEOE_GFX_COMMAND_POP_CLIP, NULL, 0))
        {
            drawList->clipDepth -= 1;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Offsets into the list's text storage.
 */
const char_t* FACEOE_Gfx_DrawList_CommandText(const FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Command_t* command)
{
    LOGASSERT(NULL != drawList);
    LOGASSERT(NULL != command);
    LOGASSERT(FACEOE_GFX_COMMAND_TEXT == command->kind);

    return &drawList->text[command->textOffset];
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Clamps the factor to 0..1.
 */
FACEOE_Gfx_Color_t FACEOE_Gfx_Color_Fade(FACEOE_Gfx_Color_t color, float32_t alpha)
{
    uint32_t faded_alpha;

    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    if (1.0f < alpha)
    {
        alpha = 1.0f;
    }
    faded_alpha = (uint32_t)(((float32_t)FACEOE_GFX_COLOR_ALPHA(color) * alpha) + 0.5f);

    return (FACEOE_Gfx_Color_t)((color & 0x00FFFFFFU) | (faded_alpha << 24));
}
