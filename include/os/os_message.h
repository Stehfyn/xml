#ifndef XML_OS_MESSAGE_H
#define XML_OS_MESSAGE_H

#include "os/os_types.h"

#include <stdio.h>
#include <wchar.h>

/** Builds a message in a caller buffer or writes it to a locked stream, without allocation. */
typedef struct Os_MessageBuilder
{
    FILE*   stream;
    char_t* buffer;
    size_t  capacity;
    size_t  length;
    bool_t  truncated;
} Os_MessageBuilder_t;

/*************************************************************************************************/
/**
 * \brief Starts a message written directly to a stream, which stays locked until Os_MessageBuilder_End.
 *
 * \param[out] pMessage   Builder to start
 * \param[in]  pStream    Non-NULL output stream
 *
 * \retval None
 */
void Os_MessageBuilder_Begin(Os_MessageBuilder_t* pMessage, FILE* pStream);
/*************************************************************************************************/
/**
 * \brief Starts a message in a caller buffer, kept zero-terminated; text that does not fit sets truncated.
 *
 * \param[out] pMessage   Builder to start
 * \param[out] pBuffer    Destination buffer; may be NULL only when capacity is 0
 * \param[in]  capacity   Size of pBuffer in bytes, including the terminator
 *
 * \retval None
 */
void Os_MessageBuilder_BeginBuffer(Os_MessageBuilder_t* pMessage, char_t* pBuffer, size_t capacity);

/*************************************************************************************************/
/**
 * \brief Ends a message, unlocking the stream of a stream message.
 *
 * \param[in,out] pMessage   Builder to end
 *
 * \retval None
 */
void Os_MessageBuilder_End(Os_MessageBuilder_t* pMessage);

/*************************************************************************************************/
/**
 * \brief Appends a string.
 *
 * \param[in,out] pMessage   Started builder
 * \param[in]     pString    Non-NULL zero-terminated string
 *
 * \retval None
 */
void Os_MessageBuilder_AppendString(Os_MessageBuilder_t* pMessage, const char_t* pString);
/*************************************************************************************************/
/**
 * \brief Appends a wide string converted to UTF-8; invalid code points become U+FFFD.
 *
 * \param[in,out] pMessage   Started builder
 * \param[in]     pString    Non-NULL zero-terminated UTF-16 or UTF-32 string
 *
 * \retval None
 */
void Os_MessageBuilder_AppendWideString(Os_MessageBuilder_t* pMessage, const wchar_t* pString);

/*************************************************************************************************/
/**
 * \brief Appends a signed integer in decimal.
 *
 * \param[in,out] pMessage   Started builder
 * \param[in]     value      Integer to append
 *
 * \retval None
 */
void Os_MessageBuilder_AppendInt(Os_MessageBuilder_t* pMessage, int64_t value);

/*************************************************************************************************/
/**
 * \brief Appends an unsigned integer in decimal.
 *
 * \param[in,out] pMessage   Started builder
 * \param[in]     value      Integer to append
 *
 * \retval None
 */
void Os_MessageBuilder_AppendUint(Os_MessageBuilder_t* pMessage, uint64_t value);

/*************************************************************************************************/
/**
 * \brief Appends an unsigned integer in hexadecimal, without a "0x" prefix.
 *
 * \param[in,out] pMessage        Started builder
 * \param[in]     value           Integer to append
 * \param[in]     minimumDigits   Minimum digit count; shorter values are zero-padded
 * \param[in]     uppercase       TRUE for A-F, FALSE for a-f
 *
 * \retval None
 */
void Os_MessageBuilder_AppendHex(Os_MessageBuilder_t* pMessage, uint64_t value, uint32_t minimumDigits, bool_t uppercase);

/*************************************************************************************************/
/**
 * \brief Appends a pointer as "0x" and zero-padded lowercase hexadecimal.
 *
 * \param[in,out] pMessage   Started builder
 * \param[in]     pValue     Pointer to append; may be NULL
 *
 * \retval None
 */
void Os_MessageBuilder_AppendPointer(Os_MessageBuilder_t* pMessage, const void* pValue);
/*************************************************************************************************/
/**
 * \brief Appends a float in scientific notation with up to nine significant digits, e.g. 1.5e+03.
 *
 * \param[in,out] pMessage   Started builder
 * \param[in]     value      Number to append; zero prints "0", NaN "nan", an infinity "inf"; a negative value, -0 and -inf included, gets a leading minus
 *
 * \retval None
 */
void Os_MessageBuilder_AppendFloat(Os_MessageBuilder_t* pMessage, float value);

#endif /* XML_OS_MESSAGE_H */
