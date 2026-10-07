#ifndef ARINC661_ICONS_H
#define ARINC661_ICONS_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

extern "C"
{
#include "os/os_types.h"
}

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define ARINC661_ICON_BUTTON_SIZE (26.0f)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Icons drawn from primitives; no icon font is needed.
 */
typedef enum Arinc661_Icon_e
{
    ARINC661_ICON_NEW,
    ARINC661_ICON_SAVE,
    ARINC661_ICON_SAVE_ALL,
    ARINC661_ICON_UNDO,
    ARINC661_ICON_REDO,
    ARINC661_ICON_ADD_LAYER,
    ARINC661_ICON_ADD_WIDGET,
    ARINC661_ICON_ADD_EXTENSION,
    ARINC661_ICON_DUPLICATE,
    ARINC661_ICON_DELETE,
    ARINC661_ICON_GRID,
    ARINC661_ICON_SNAP,
    ARINC661_ICON_FIT,
    ARINC661_ICON_HELP,
    ARINC661_ICON_ZOOM_IN,
    ARINC661_ICON_ZOOM_OUT,
    ARINC661_ICON_SCREEN,
    ARINC661_ICON_LAYOUT,
    ARINC661_ICON_WINDOW,
    ARINC661_ICON_APPLICATION
} Arinc661_Icon_t;

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
 * \brief Draws one icon from primitives into a box.
 *
 * \param[in,out] drawList   Target draw list
 * \param[in]     icon       Icon to draw
 * \param[in]     minimum    Top-left of the icon box
 * \param[in]     maximum    Bottom-right of the icon box
 * \param[in]     color      Icon colour
 *
 * \retval        None
 */
void Arinc661_Icons_Draw(ImDrawList* drawList, Arinc661_Icon_t icon, ImVec2 minimum, ImVec2 maximum, ImU32 color);

/*************************************************************************************************/
/**
 * \brief Draws a square icon button with a tooltip, leaving the cursor on the same line.
 *
 * \param[in] icon        Icon to draw
 * \param[in] label       Tooltip text; also the ImGui id
 * \param[in] shortcut    Shortcut text, or NULL
 * \param[in] isEnabled   FALSE to draw it disabled
 * \param[in] isActive    TRUE to highlight a toggled state
 *
 * \retval    TRUE        if the button was pressed.
 * \retval    FALSE       otherwise.
 */
bool_t Arinc661_Icons_Button(Arinc661_Icon_t icon, const char_t* label, const char_t* shortcut, bool_t isEnabled, bool_t isActive);

/*************************************************************************************************/
/**
 * \brief Draws a thin vertical separator between toolbar groups, on the same line.
 *
 * \retval None
 */
void Arinc661_Icons_Gap(void);

#endif /* ARINC661_ICONS_H */
