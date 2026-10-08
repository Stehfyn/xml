#ifndef ARINC661_RENDERER_H
#define ARINC661_RENDERER_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, section 2.3.4: origins at the lower-left corner, X right, Y up, angles in
 * degrees counter-clockwise from +X, screen units of 0.01 mm. Section 3.2.3: a container's
 * children are positioned from its reference point and inherit its Visible and Enable states.
 * Section 3.2.6: formatted numeric values. Sections 3.3 to 3.10: the Creation Structure of
 * every widget drawn here.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#include "arinc661/arinc661_scene.h"
#include "faceoe_gfx/faceoe_gfx_drawlist.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Output-space bounds of one drawn widget.
 */
typedef struct Arinc661_Renderer_Bounds_s
{
    const Arinc661_Widget_t* widget;  ///< Widget the scene node stands for
    FACEOE_Gfx_Point_t       minimum; ///< Top-left in output units
    FACEOE_Gfx_Point_t       maximum; ///< Bottom-right in output units
} Arinc661_Renderer_Bounds_t;

/*************************************************************************************************/
/**
 * \brief How a layer is drawn, and where its widget bounds are reported.
 */
typedef struct Arinc661_Renderer_Options_s
{
    bool_t                      showsHidden;       ///< TRUE to draw invisible subtrees, inactive pages, and closed pop-ups faded instead of skipping them
    bool_t                      showsNames;        ///< TRUE to label every widget with its name
    bool_t                      showsNonGraphical; ///< TRUE to draw widgets without geometry as chips along the layer's left edge
    bool_t                      fillsLayerArea;    ///< TRUE to fill the layer's Width by Height area
    float32_t                   fontSize;          ///< Host font size in output units
    Arinc661_Renderer_Bounds_t* bounds;            ///< Caller's bounds storage, or NULL
    uint32_t                    boundsCapacity;    ///< Bounds slots
    uint32_t                    boundsCount;       ///< Result: bounds written, in drawing order
    float32_t                   animationTime;     ///< Seconds into the layer's animations, or negative to draw the rest state
} Arinc661_Renderer_Options_t;

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
 * \brief Records one layer of a scene, with everything below it, into a draw list.
 *
 * \par Details:
 *  The draw list's current transform maps layer units (0.01 mm, Y up) to output units; the
 *  caller sets it before the call. Widgets are recorded in scene order, children after their
 *  container, so later widgets overdraw earlier ones as in section 3.1.4.
 *
 * \param[in]     scene        Scene to draw
 * \param[in]     layerIndex   Layer to draw
 * \param[in,out] drawList     Target draw list
 * \param[in,out] options      Drawing options and bounds output
 *
 * \retval        None
 */
void Arinc661_Renderer_DrawLayer(const Arinc661_Scene_t* scene, uint32_t layerIndex, FACEOE_Gfx_DrawList_t* drawList, Arinc661_Renderer_Options_t* options);

/*************************************************************************************************/
/**
 * \brief Finds the smallest recorded bounds containing a point.
 *
 * \param[in] bounds   Bounds written by Arinc661_Renderer_DrawLayer
 * \param[in] count    Number of bounds
 * \param[in] point    Output-space point
 *
 * \retval    const Arinc661_Widget_t*   Widget, or NULL.
 */
const Arinc661_Widget_t* Arinc661_Renderer_Pick(const Arinc661_Renderer_Bounds_t* bounds, uint32_t count, FACEOE_Gfx_Point_t point);

/*************************************************************************************************/
/**
 * \brief Formats a numeric value through an ARINC 661 format string.
 *
 * \par Details:
 *  Part 1-7, section 3.2.6: `#` always shows a digit, `_` shows only significant digits,
 *  `~` replaces insignificant digits with a figure space, `+` places the sign, `.` separates
 *  the parts, and any other character is a mask character.
 *
 * \param[in]  format     Format string
 * \param[in]  value      Value to format
 * \param[out] text       Formatted text
 * \param[in]  capacity   Bytes available in text
 *
 * \retval     None
 */
void Arinc661_Renderer_FormatNumber(const char_t* format, float64_t value, char_t* text, uint32_t capacity);

/*************************************************************************************************/
/**
 * \brief Reads a layer's Width and Height in 0.01 mm.
 *
 * \param[in]  scene        Scene
 * \param[in]  layerIndex   Layer
 * \param[out] width        Width, at least 1
 * \param[out] height       Height, at least 1
 *
 * \retval     None
 */
void Arinc661_Renderer_GetLayerSize(const Arinc661_Scene_t* scene, uint32_t layerIndex, float32_t* width, float32_t* height);

#ifdef __cplusplus
}
#endif

#endif /* ARINC661_RENDERER_H */
