#ifndef FACEOE_GFX_DRAWLIST_H
#define FACEOE_GFX_DRAWLIST_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * Retained 2-D draw list: a flat command buffer of filled polygons, polylines, text boxes,
 * and clip rectangles in caller-owned storage. Producers record through a transform stack;
 * renderers replay the commands with their own drawing API. Points are stored already
 * transformed, in output units with Y growing downward.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define FACEOE_GFX_MAX_TRANSFORM_DEPTH (64)
#define FACEOE_GFX_MAX_CLIP_DEPTH      (64)
#define FACEOE_GFX_MAX_TEXT_BYTES      (256)

/** Packs a colour as 0xAABBGGRR. */
#define FACEOE_GFX_RGBA(r, g, b, a) ((FACEOE_Gfx_Color_t)(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r)))
#define FACEOE_GFX_COLOR_RED(c)     ((uint32_t)(c) & 0xFFU)
#define FACEOE_GFX_COLOR_GREEN(c)   (((uint32_t)(c) >> 8) & 0xFFU)
#define FACEOE_GFX_COLOR_BLUE(c)    (((uint32_t)(c) >> 16) & 0xFFU)
#define FACEOE_GFX_COLOR_ALPHA(c)   (((uint32_t)(c) >> 24) & 0xFFU)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

typedef uint32_t FACEOE_Gfx_Color_t; ///< 0xAABBGGRR colour

/*************************************************************************************************/
/**
 * \brief Point in output units, Y growing downward.
 */
typedef struct FACEOE_Gfx_Point_s
{
    float32_t x; ///< Horizontal position
    float32_t y; ///< Vertical position
} FACEOE_Gfx_Point_t;

/*************************************************************************************************/
/**
 * \brief Affine transform: output = (a*x + c*y + e, b*x + d*y + f).
 */
typedef struct FACEOE_Gfx_Transform_s
{
    float32_t a; ///< X scale and rotation
    float32_t b; ///< X shear and rotation
    float32_t c; ///< Y shear and rotation
    float32_t d; ///< Y scale and rotation
    float32_t e; ///< X translation
    float32_t f; ///< Y translation
} FACEOE_Gfx_Transform_t;

/*************************************************************************************************/
/**
 * \brief Kind of one recorded command.
 */
typedef enum FACEOE_Gfx_CommandKind_e
{
    FACEOE_GFX_COMMAND_FILL_POLYGON,    ///< Fill the points as a closed polygon, which may be concave
    FACEOE_GFX_COMMAND_STROKE_POLYLINE, ///< Stroke the points, closed when isClosed is TRUE
    FACEOE_GFX_COMMAND_TEXT,            ///< Lay text out in the box of the first two points and rotate it about the box centre
    FACEOE_GFX_COMMAND_PUSH_CLIP,       ///< Intersect the clip rectangle with the first two points
    FACEOE_GFX_COMMAND_POP_CLIP         ///< Restore the previous clip rectangle
} FACEOE_Gfx_CommandKind_t;

/*************************************************************************************************/
/**
 * \brief Horizontal and vertical text alignment inside a text box.
 */
typedef enum FACEOE_Gfx_Align_e
{
    FACEOE_GFX_ALIGN_START,  ///< Left or top
    FACEOE_GFX_ALIGN_CENTER, ///< Centred
    FACEOE_GFX_ALIGN_END     ///< Right or bottom
} FACEOE_Gfx_Align_t;

/*************************************************************************************************/
/**
 * \brief One recorded command.
 *
 * \par Details:
 *  firstPoint and pointCount index the draw list's point storage. Text commands use two
 *  points for the axis-aligned box in output units before rotation, plus angle, font size,
 *  alignment, and a text offset into the draw list's text storage.
 */
typedef struct FACEOE_Gfx_Command_s
{
    FACEOE_Gfx_CommandKind_t kind;       ///< Command kind
    uint32_t                 firstPoint; ///< Index of the first point
    uint32_t                 pointCount; ///< Number of points
    FACEOE_Gfx_Color_t       color;      ///< Colour
    float32_t                thickness;  ///< Stroke thickness in output units
    bool_t                   isClosed;   ///< TRUE for a closed polyline
    float32_t                angle;      ///< Text rotation in radians, clockwise on screen
    float32_t                fontSize;   ///< Text size in output units
    FACEOE_Gfx_Align_t       horizontal; ///< Text horizontal alignment
    FACEOE_Gfx_Align_t       vertical;   ///< Text vertical alignment
    uint32_t                 textOffset; ///< Offset of the zero-terminated text
} FACEOE_Gfx_Command_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned draw list.
 */
typedef struct FACEOE_Gfx_DrawList_s
{
    FACEOE_Gfx_Command_t*  commands;                                      ///< Caller's command storage
    FACEOE_Gfx_Point_t*    points;                                        ///< Caller's point storage
    char_t*                text;                                          ///< Caller's text storage
    uint32_t               commandCapacity;                               ///< Command slots
    uint32_t               pointCapacity;                                 ///< Point slots
    uint32_t               textCapacity;                                  ///< Text bytes
    uint32_t               commandCount;                                  ///< Commands recorded
    uint32_t               pointCount;                                    ///< Points recorded
    uint32_t               textByteCount;                                 ///< Text bytes recorded
    bool_t                 isOverflowed;                                  ///< TRUE once a command was dropped for lack of storage
    uint32_t               transformDepth;                                ///< Transforms pushed
    FACEOE_Gfx_Transform_t transforms[FACEOE_GFX_MAX_TRANSFORM_DEPTH];    ///< Transform stack; the top is current
    uint32_t               clipDepth;                                     ///< Clips pushed
} FACEOE_Gfx_DrawList_t;

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Attaches storage and starts an empty list with the identity transform.
 *
 * \param[out] drawList          Draw list
 * \param[in]  commands          Command storage
 * \param[in]  commandCapacity   Command slots
 * \param[in]  points            Point storage
 * \param[in]  pointCapacity     Point slots
 * \param[in]  text              Text storage
 * \param[in]  textCapacity      Text bytes
 *
 * \retval     None
 */
void FACEOE_Gfx_DrawList_Initialize(FACEOE_Gfx_DrawList_t* drawList, FACEOE_Gfx_Command_t* commands, uint32_t commandCapacity, FACEOE_Gfx_Point_t* points, uint32_t pointCapacity, char_t* text, uint32_t textCapacity);

/*************************************************************************************************/
/**
 * \brief Discards every command and resets the stacks to the identity transform.
 *
 * \param[in,out] drawList   Draw list
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Clear(FACEOE_Gfx_DrawList_t* drawList);

/*************************************************************************************************/
/**
 * \brief Replaces the current transform.
 *
 * \param[in,out] drawList    Draw list
 * \param[in]     transform   New current transform
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_SetTransform(FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Transform_t* transform);

/*************************************************************************************************/
/**
 * \brief Returns the current transform.
 *
 * \param[in] drawList   Draw list
 *
 * \retval    const FACEOE_Gfx_Transform_t*   Current transform.
 */
const FACEOE_Gfx_Transform_t* FACEOE_Gfx_DrawList_Transform(const FACEOE_Gfx_DrawList_t* drawList);

/*************************************************************************************************/
/**
 * \brief Pushes a copy of the current transform.
 *
 * \param[in,out] drawList   Draw list
 *
 * \retval        TRUE       if the stack had room.
 * \retval        FALSE      otherwise; nothing is pushed.
 */
bool_t FACEOE_Gfx_DrawList_PushTransform(FACEOE_Gfx_DrawList_t* drawList);

/*************************************************************************************************/
/**
 * \brief Pops the current transform.
 *
 * \param[in,out] drawList   Draw list
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_PopTransform(FACEOE_Gfx_DrawList_t* drawList);

/*************************************************************************************************/
/**
 * \brief Translates the current transform in its own local units.
 *
 * \param[in,out] drawList   Draw list
 * \param[in]     x          Local X offset
 * \param[in]     y          Local Y offset
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Translate(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y);

/*************************************************************************************************/
/**
 * \brief Rotates the current transform about a local point.
 *
 * \param[in,out] drawList   Draw list
 * \param[in]     centerX    Local X of the rotation centre
 * \param[in]     centerY    Local Y of the rotation centre
 * \param[in]     radians    Angle, counter-clockwise in local units
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Rotate(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t radians);

/*************************************************************************************************/
/**
 * \brief Scales the current transform about a local point.
 *
 * \param[in,out] drawList   Draw list
 * \param[in]     centerX    Local X of the scaling centre
 * \param[in]     centerY    Local Y of the scaling centre
 * \param[in]     scaleX     Local X factor
 * \param[in]     scaleY     Local Y factor
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Scale(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t scaleX, float32_t scaleY);

/*************************************************************************************************/
/**
 * \brief Maps a local point through the current transform.
 *
 * \param[in] drawList   Draw list
 * \param[in] x          Local X
 * \param[in] y          Local Y
 *
 * \retval    FACEOE_Gfx_Point_t   Output point.
 */
FACEOE_Gfx_Point_t FACEOE_Gfx_DrawList_Map(const FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y);

/*************************************************************************************************/
/**
 * \brief Returns the output length of one local unit along the local X and Y axes.
 *
 * \param[in]  drawList   Draw list
 * \param[out] unitX      Output units per local X unit
 * \param[out] unitY      Output units per local Y unit
 *
 * \retval     None
 */
void FACEOE_Gfx_DrawList_UnitLengths(const FACEOE_Gfx_DrawList_t* drawList, float32_t* unitX, float32_t* unitY);

/*************************************************************************************************/
/**
 * \brief Returns the screen angle of the local X axis in radians, clockwise on screen.
 *
 * \param[in] drawList   Draw list
 *
 * \retval    float32_t   Angle.
 */
float32_t FACEOE_Gfx_DrawList_Angle(const FACEOE_Gfx_DrawList_t* drawList);

/*************************************************************************************************/
/**
 * \brief Records a filled polygon from local points.
 *
 * \param[in,out] drawList   Draw list
 * \param[in]     points     Local points
 * \param[in]     count      Number of points, at least 3
 * \param[in]     color      Fill colour
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_FillPolygon(FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Point_t* points, uint32_t count, FACEOE_Gfx_Color_t color);

/*************************************************************************************************/
/**
 * \brief Records a stroked polyline from local points.
 *
 * \param[in,out] drawList    Draw list
 * \param[in]     points      Local points
 * \param[in]     count       Number of points, at least 2
 * \param[in]     isClosed    TRUE to join the last point to the first
 * \param[in]     color       Stroke colour
 * \param[in]     thickness   Thickness in output units
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_StrokePolyline(FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Point_t* points, uint32_t count, bool_t isClosed, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records a filled local rectangle.
 *
 * \param[in,out] drawList   Draw list
 * \param[in]     x          Local X of one corner
 * \param[in]     y          Local Y of that corner
 * \param[in]     width      Local width
 * \param[in]     height     Local height
 * \param[in]     color      Fill colour
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_FillRectangle(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color);

/*************************************************************************************************/
/**
 * \brief Records a stroked local rectangle.
 *
 * \param[in,out] drawList    Draw list
 * \param[in]     x           Local X of one corner
 * \param[in]     y           Local Y of that corner
 * \param[in]     width       Local width
 * \param[in]     height      Local height
 * \param[in]     color       Stroke colour
 * \param[in]     thickness   Thickness in output units
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_StrokeRectangle(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records a local line segment.
 *
 * \param[in,out] drawList    Draw list
 * \param[in]     x1          Local X of the start
 * \param[in]     y1          Local Y of the start
 * \param[in]     x2          Local X of the end
 * \param[in]     y2          Local Y of the end
 * \param[in]     color       Stroke colour
 * \param[in]     thickness   Thickness in output units
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Line(FACEOE_Gfx_DrawList_t* drawList, float32_t x1, float32_t y1, float32_t x2, float32_t y2, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records a dashed local line segment.
 *
 * \param[in,out] drawList     Draw list
 * \param[in]     x1           Local X of the start
 * \param[in]     y1           Local Y of the start
 * \param[in]     x2           Local X of the end
 * \param[in]     y2           Local Y of the end
 * \param[in]     dashLength   Local length of each dash and gap
 * \param[in]     color        Stroke colour
 * \param[in]     thickness    Thickness in output units
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_DashedLine(FACEOE_Gfx_DrawList_t* drawList, float32_t x1, float32_t y1, float32_t x2, float32_t y2, float32_t dashLength, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records a dashed local rectangle outline.
 *
 * \param[in,out] drawList     Draw list
 * \param[in]     x            Local X of one corner
 * \param[in]     y            Local Y of that corner
 * \param[in]     width        Local width
 * \param[in]     height       Local height
 * \param[in]     dashLength   Local length of each dash and gap
 * \param[in]     color        Stroke colour
 * \param[in]     thickness    Thickness in output units
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_DashedRectangle(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, float32_t dashLength, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records an elliptical arc, or a filled sector, in local units.
 *
 * \param[in,out] drawList      Draw list
 * \param[in]     centerX       Local X of the centre
 * \param[in]     centerY       Local Y of the centre
 * \param[in]     radiusX       Local X radius
 * \param[in]     radiusY       Local Y radius
 * \param[in]     startRadians  Start angle, counter-clockwise from +X
 * \param[in]     endRadians    End angle; a full turn or more closes the ellipse
 * \param[in]     isFilled      TRUE to fill the sector, FALSE to stroke the arc
 * \param[in]     color         Colour
 * \param[in]     thickness     Thickness in output units when stroking
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Arc(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t radiusX, float32_t radiusY, float32_t startRadians, float32_t endRadians, bool_t isFilled, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records a ring sector between two radii in local units.
 *
 * \param[in,out] drawList      Draw list
 * \param[in]     centerX       Local X of the centre
 * \param[in]     centerY       Local Y of the centre
 * \param[in]     innerRadius   Local inner radius
 * \param[in]     outerRadius   Local outer radius
 * \param[in]     startRadians  Start angle, counter-clockwise from +X
 * \param[in]     endRadians    End angle
 * \param[in]     isFilled      TRUE to fill, FALSE to stroke the outline
 * \param[in]     color         Colour
 * \param[in]     thickness     Thickness in output units when stroking
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Crown(FACEOE_Gfx_DrawList_t* drawList, float32_t centerX, float32_t centerY, float32_t innerRadius, float32_t outerRadius, float32_t startRadians, float32_t endRadians, bool_t isFilled, FACEOE_Gfx_Color_t color, float32_t thickness);

/*************************************************************************************************/
/**
 * \brief Records text laid out in a local box.
 *
 * \par Details:
 *  The box is mapped through the current transform; the renderer aligns the text inside the
 *  mapped box's axis-aligned bounds and rotates it about the box centre by the transform's
 *  angle plus the extra angle. A zero font size asks the renderer for its default size.
 *
 * \param[in,out] drawList     Draw list
 * \param[in]     x            Local X of one corner
 * \param[in]     y            Local Y of that corner
 * \param[in]     width        Local width
 * \param[in]     height       Local height
 * \param[in]     text         Zero-terminated text
 * \param[in]     horizontal   Horizontal alignment
 * \param[in]     vertical     Vertical alignment
 * \param[in]     fontSize     Font size in output units, or zero
 * \param[in]     radians      Extra rotation, counter-clockwise in local units
 * \param[in]     color        Text colour
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_Text(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height, const char_t* text, FACEOE_Gfx_Align_t horizontal, FACEOE_Gfx_Align_t vertical, float32_t fontSize, float32_t radians, FACEOE_Gfx_Color_t color);

/*************************************************************************************************/
/**
 * \brief Records a clip rectangle from a local box's mapped bounds.
 *
 * \param[in,out] drawList   Draw list
 * \param[in]     x          Local X of one corner
 * \param[in]     y          Local Y of that corner
 * \param[in]     width      Local width
 * \param[in]     height     Local height
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_PushClip(FACEOE_Gfx_DrawList_t* drawList, float32_t x, float32_t y, float32_t width, float32_t height);

/*************************************************************************************************/
/**
 * \brief Records the end of the innermost clip rectangle.
 *
 * \param[in,out] drawList   Draw list
 *
 * \retval        None
 */
void FACEOE_Gfx_DrawList_PopClip(FACEOE_Gfx_DrawList_t* drawList);

/*************************************************************************************************/
/**
 * \brief Returns the text of a text command.
 *
 * \param[in] drawList   Draw list
 * \param[in] command    Text command of that list
 *
 * \retval    const char_t*   Zero-terminated text.
 */
const char_t* FACEOE_Gfx_DrawList_CommandText(const FACEOE_Gfx_DrawList_t* drawList, const FACEOE_Gfx_Command_t* command);

/*************************************************************************************************/
/**
 * \brief Scales a colour's alpha.
 *
 * \param[in] color   Colour
 * \param[in] alpha   Factor from 0 to 1
 *
 * \retval    FACEOE_Gfx_Color_t   Faded colour.
 */
FACEOE_Gfx_Color_t FACEOE_Gfx_Color_Fade(FACEOE_Gfx_Color_t color, float32_t alpha);

#endif /* FACEOE_GFX_DRAWLIST_H */
