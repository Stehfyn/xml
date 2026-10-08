#ifndef XML_OS_DIAGNOSTICS_H
#define XML_OS_DIAGNOSTICS_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

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
 * \brief Initializes process-wide operating-system diagnostics.
 *
 * On Windows, suppresses error dialogs, routes CRT reports to stderr, makes invalid CRT
 * parameters and SIGABRT exit with EXIT_FAILURE, and logs unhandled exceptions with their
 * stack frames. Other platforms need no initialization.
 *
 * \retval        None
 */
void Os_Diagnostics_Initialize(void);

#if defined(_WIN32)
/*************************************************************************************************/
/**
 * \brief Initializes diagnostics and calls entry inside a structured-exception handler.
 *
 * \param[in] entry   Non-NULL program entry point
 * \param[in] argc    Argument count passed to entry
 * \param[in] argv    Argument vector passed to entry
 *
 * \retval    int     entry's result, or EXIT_FAILURE after an unhandled exception is logged.
 */
int Os_Diagnostics_Run(int (*entry)(int, char_t**), int argc, char_t** argv);
#endif

#endif /* XML_OS_DIAGNOSTICS_H */
