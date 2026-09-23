#ifndef XML_OS_ASSERT_H
#define XML_OS_ASSERT_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#if defined(_MSC_VER)
#define OS_NORETURN __declspec(noreturn)
#else
#define OS_NORETURN _Noreturn
#endif

#define LOGASSERT(expression)                                                                                                                                                                          \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        if (!(expression))                                                                                                                                                                             \
        {                                                                                                                                                                                              \
            Os_AssertFailure(#expression, __FILE__, __LINE__);                                                                                                                                         \
        }                                                                                                                                                                                              \
    } while (0) ///< Records a failed invariant and terminates, including in Release builds.

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
 * \brief Reports a failed assertion and terminates the process.
 *
 * \par Requirements:
 *  - 001 This function shall write the expression, source filename, and line number to stderr.
 *  - 002 This function shall terminate the process by calling abort.
 *
 * \param[in] pExpression       Non-NULL null-terminated expression text
 * \param[in] pFile             Non-NULL null-terminated source filename
 * \param[in] line              One-based source line
 *
 * \retval    None
 */
OS_NORETURN void Os_AssertFailure(const char* pExpression, const char* pFile, int line);

#endif /* XML_OS_ASSERT_H */
