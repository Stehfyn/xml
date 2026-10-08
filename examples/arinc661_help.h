#ifndef ARINC661_HELP_H
#define ARINC661_HELP_H

/*=================================================================================================
** 1. References
**
** ARINC 661 Part 1-7, sections 3.3 to 3.10 and 8: the Categories, Description, Restriction, and
** Parameters table of every widget and extension.
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

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
 * \brief Returns the specification text of a widget or extension type.
 *
 * \param[in]  type          Widget or extension type name, such as A661_PUSH_BUTTON
 * \param[out] description   Description paragraph
 * \param[out] categories    Categories, comma separated
 * \param[out] restriction   Restriction paragraph, or empty
 *
 * \retval     TRUE          if the type has a section.
 * \retval     FALSE         otherwise.
 */
bool_t Arinc661_Help_GetType(const char_t* type, const char_t** description, const char_t** categories, const char_t** restriction);

/*************************************************************************************************/
/**
 * \brief Returns the specification text of one parameter of a widget or extension type.
 *
 * \param[in]  type            Widget or extension type name
 * \param[in]  parameterName   Parameter name
 * \param[out] change          D, R, or DR: settable at definition time, run time, or both; may be NULL
 *
 * \retval     const char_t*   Description, or an empty string.
 */
const char_t* Arinc661_Help_GetParameter(const char_t* type, const char_t* parameterName, const char_t** change);

#ifdef __cplusplus
}
#endif

#endif /* ARINC661_HELP_H */
