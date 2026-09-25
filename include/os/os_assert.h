#ifndef XML_OS_ASSERT_H
#define XML_OS_ASSERT_H

/* Use the MSVC spelling for both the compiler and Visual Studio IntelliSense. */
#if defined(_MSC_VER)
#define OS_NORETURN __declspec(noreturn)
#else
#define OS_NORETURN _Noreturn
#endif

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_message.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define LOGASSERT(expression)                                                                                                                                                                          \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        if (!(expression))                                                                                                                                                                             \
        {                                                                                                                                                                                              \
            Os_AssertFailure(#expression, __FILE__, __LINE__);                                                                                                                                         \
        }                                                                                                                                                                                              \
    } while (0) ///< Makes a false invariant fatal and records its expression and source location.

/* Pair LOGSTART/LOGEND in one lexical scope. Do not return or jump out before LOGEND. */
#define LOGSTART(stream)                                                                                                                                                                               \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        Os_MessageBuilder_t os_log_builder;                                                                                                                                                            \
    Os_MessageBuilder_Begin(&os_log_builder, (stream))
#define LOGSTARTBUFFER(buffer, capacity)                                                                                                                                                               \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        Os_MessageBuilder_t os_log_builder;                                                                                                                                                            \
    Os_MessageBuilder_BeginBuffer(&os_log_builder, (buffer), (capacity))
#define LOGEND()                                                                                                                                                                                       \
    Os_MessageBuilder_End(&os_log_builder);                                                                                                                                                            \
    }                                                                                                                                                                                                  \
    while (0)

#define LOGPRINTS(string)                          (Os_MessageBuilder_AppendString(&os_log_builder, (string)))
#define LOGPRINTW(string)                          (Os_MessageBuilder_AppendWideString(&os_log_builder, (string)))
#define LOGPRINTI(value)                           (Os_MessageBuilder_AppendInt(&os_log_builder, (value)))
#define LOGPRINTU(value)                           (Os_MessageBuilder_AppendUint(&os_log_builder, (value)))
#define LOGPRINTX(value, minimumDigits, uppercase) (Os_MessageBuilder_AppendHex(&os_log_builder, (value), (minimumDigits), (uppercase)))
#define LOGPRINTP(value)                           (Os_MessageBuilder_AppendPointer(&os_log_builder, (value)))
#define LOGPRINTF(value)                           (Os_MessageBuilder_AppendFloat(&os_log_builder, (value)))

#define LOGFATAL(message) (Os_LogFatal((message), __FILE__, __LINE__)) ///< Emits a fatal diagnostic at the call site.

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
 * \brief Emits a failed-invariant diagnostic and terminates the process.
 *
 * \param[in] pExpression   Non-NULL null-terminated expression text
 * \param[in] pFile         Non-NULL null-terminated source filename
 * \param[in] line          One-based source line
 *
 * \retval    None
 */
OS_NORETURN void Os_AssertFailure(const char_t* pExpression, const char_t* pFile, int line);

/*************************************************************************************************/
/**
 * \brief Emits an unrecoverable-error diagnostic and terminates the process.
 *
 * \param[in] pMessage   Non-NULL null-terminated failure description
 * \param[in] pFile      Non-NULL null-terminated source filename
 * \param[in] line       One-based source line
 *
 * \retval    None
 */
OS_NORETURN void Os_LogFatal(const char_t* pMessage, const char_t* pFile, int line);

#endif /* XML_OS_ASSERT_H */
