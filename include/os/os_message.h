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

/** Begin/end must be paired on the same thread. Appends never interpret format specifiers. */
void Os_MessageBuilder_Begin(Os_MessageBuilder_t* pMessage, FILE* pStream);
/** Buffer output is always terminated when capacity is nonzero; truncation sets the truncated field. */
void Os_MessageBuilder_BeginBuffer(Os_MessageBuilder_t* pMessage, char_t* pBuffer, size_t capacity);
void Os_MessageBuilder_End(Os_MessageBuilder_t* pMessage);
void Os_MessageBuilder_AppendString(Os_MessageBuilder_t* pMessage, const char_t* pString);
/** Converts Unicode wide strings to UTF-8, replacing invalid code points with U+FFFD. */
void Os_MessageBuilder_AppendWideString(Os_MessageBuilder_t* pMessage, const wchar_t* pString);
void Os_MessageBuilder_AppendInt(Os_MessageBuilder_t* pMessage, int64_t value);
void Os_MessageBuilder_AppendUint(Os_MessageBuilder_t* pMessage, uint64_t value);
void Os_MessageBuilder_AppendHex(Os_MessageBuilder_t* pMessage, uint64_t value, uint32_t minimumDigits, bool_t uppercase);
void Os_MessageBuilder_AppendPointer(Os_MessageBuilder_t* pMessage, const void* pValue);
/** Prints a float in scientific notation with up to nine significant decimal digits, or nan/inf. */
void Os_MessageBuilder_AppendFloat(Os_MessageBuilder_t* pMessage, float value);

#endif /* XML_OS_MESSAGE_H */
