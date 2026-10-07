#ifndef XML_OS_MESSAGE_H
#define XML_OS_MESSAGE_H

#include "os/os_types.h"

#include <stdio.h>
#include <wchar.h>

/** Builds a message in caller-owned storage or emits it under a stream lock, without allocation. */
typedef struct Os_MessageBuilder
{
    FILE*  stream;
    char_t* buffer;
    size_t capacity;
    size_t length;
    bool_t truncated;
} Os_MessageBuilder_t;

/*************************************************************************************************/
/**
 * \brief Starts a message in the builder's own storage.
 *
 * \param pMessage   P message
 * \param pStream    P stream
 *
 * \retval None
 */
void Os_MessageBuilder_Begin(Os_MessageBuilder_t* pMessage, FILE* pStream);
/*************************************************************************************************/
/**
 * \brief Starts a message in a caller buffer.
 *
 * \param pMessage   P message
 * \param pBuffer    P buffer
 * \param capacity   Bytes available
 *
 * \retval None
 */
void Os_MessageBuilder_BeginBuffer(Os_MessageBuilder_t* pMessage, char_t* pBuffer, size_t capacity);

/*************************************************************************************************/
/**
 * \brief Terminates the message and returns it.
 *
 * \param pMessage   P message
 *
 * \retval None
 */
void Os_MessageBuilder_End(Os_MessageBuilder_t* pMessage);

/*************************************************************************************************/
/**
 * \brief Appends a string.
 *
 * \param pMessage   P message
 * \param pString    P string
 *
 * \retval None
 */
void Os_MessageBuilder_AppendString(Os_MessageBuilder_t* pMessage, const char_t* pString);
/*************************************************************************************************/
/**
 * \brief Appends a wide string converted to UTF-8.
 *
 * \param pMessage   P message
 * \param pString    P string
 *
 * \retval None
 */
void Os_MessageBuilder_AppendWideString(Os_MessageBuilder_t* pMessage, const wchar_t* pString);

/*************************************************************************************************/
/**
 * \brief Appends a signed integer in decimal.
 *
 * \param pMessage   P message
 * \param value      Value
 *
 * \retval None
 */
void Os_MessageBuilder_AppendInt(Os_MessageBuilder_t* pMessage, int64_t value);

/*************************************************************************************************/
/**
 * \brief Appends an unsigned integer in decimal.
 *
 * \param pMessage   P message
 * \param value      Value
 *
 * \retval None
 */
void Os_MessageBuilder_AppendUint(Os_MessageBuilder_t* pMessage, uint64_t value);

/*************************************************************************************************/
/**
 * \brief Appends an unsigned integer in hexadecimal.
 *
 * \param pMessage        P message
 * \param value           Value
 * \param minimumDigits   Minimum digits
 * \param uppercase       Uppercase
 *
 * \retval None
 */
void Os_MessageBuilder_AppendHex(Os_MessageBuilder_t* pMessage, uint64_t value, uint32_t minimumDigits, bool_t uppercase);

/*************************************************************************************************/
/**
 * \brief Appends a pointer value.
 *
 * \param pMessage   P message
 * \param pValue     P value
 *
 * \retval None
 */
void Os_MessageBuilder_AppendPointer(Os_MessageBuilder_t* pMessage, const void* pValue);
/*************************************************************************************************/
/**
 * \brief Appends a floating-point number.
 *
 * \param pMessage   P message
 * \param value      Value
 *
 * \retval None
 */
void Os_MessageBuilder_AppendFloat(Os_MessageBuilder_t* pMessage, float value);

#endif /* XML_OS_MESSAGE_H */
