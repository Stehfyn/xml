#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "os/os_message.h"

#include <math.h>

static const char_t c_decimalDigits[]  = "0123456789";
static const char_t c_lowerHexDigits[] = "0123456789abcdef";
static const char_t c_upperHexDigits[] = "0123456789ABCDEF";

static void AppendCharacter(Os_MessageBuilder_t* pMessage, int character)
{
    if (NULL != pMessage->stream)
    {
        (void)fputc(character, pMessage->stream);
    }
    else if (pMessage->capacity && pMessage->length < pMessage->capacity - 1u)
    {
        pMessage->buffer[pMessage->length++] = (char_t)character;
        pMessage->buffer[pMessage->length]   = '\0';
    }
    else
    {
        pMessage->truncated = TRUE;
    }
}

static void AppendDigits(Os_MessageBuilder_t* pMessage, uint64_t value, uint32_t base, uint32_t minimumDigits, const char_t* pDigits)
{
    char_t digits[64];
    size_t count;

    count = 0;
    do
    {
        digits[count++] = pDigits[value % base];
        value /= base;
    } while (value);
    while (minimumDigits > count)
    {
        AppendCharacter(pMessage, '0');
        --minimumDigits;
    }
    while (count)
    {
        AppendCharacter(pMessage, digits[--count]);
    }
}

void Os_MessageBuilder_Begin(Os_MessageBuilder_t* pMessage, FILE* pStream)
{
    pMessage->stream    = pStream;
    pMessage->buffer    = NULL;
    pMessage->capacity  = 0;
    pMessage->length    = 0;
    pMessage->truncated = FALSE;
#if defined(_WIN32)
    _lock_file(pStream);
#else
    flockfile(pStream);
#endif
}

void Os_MessageBuilder_BeginBuffer(Os_MessageBuilder_t* pMessage, char_t* pBuffer, size_t capacity)
{
    pMessage->stream    = NULL;
    pMessage->buffer    = pBuffer;
    pMessage->capacity  = capacity;
    pMessage->length    = 0;
    pMessage->truncated = FALSE;
    if (capacity)
    {
        pBuffer[0] = '\0';
    }
}

void Os_MessageBuilder_End(Os_MessageBuilder_t* pMessage)
{
    if (NULL != pMessage->stream)
    {
#if defined(_WIN32)
        _unlock_file(pMessage->stream);
#else
        funlockfile(pMessage->stream);
#endif
        pMessage->stream = NULL;
    }
}

void Os_MessageBuilder_AppendString(Os_MessageBuilder_t* pMessage, const char_t* pString)
{
    if (NULL != pMessage->stream)
    {
        (void)fputs(pString, pMessage->stream);
    }
    else
    {
        while ((*pString))
        {
            AppendCharacter(pMessage, (int)(byte_t)(*pString));
            ++pString;
        }
    }
}

void Os_MessageBuilder_AppendWideString(Os_MessageBuilder_t* pMessage, const wchar_t* pString)
{
    uint32_t code_point;
    uint32_t low_surrogate;

    while ((*pString))
    {
        code_point = (uint32_t)(*pString);
        ++pString;
        if (0xd800u <= code_point && 0xdbffu >= code_point)
        {
            low_surrogate = (uint32_t)(*pString);
            if (0xdc00u <= low_surrogate && 0xdfffu >= low_surrogate)
            {
                ++pString;
                code_point = 0x10000u + ((code_point - 0xd800u) << 10) + low_surrogate - 0xdc00u;
            }
            else
            {
                code_point = 0xfffdu;
            }
        }
        if ((0xdc00u <= code_point && 0xdfffu >= code_point) || 0x10ffffu < code_point)
        {
            code_point = 0xfffdu;
        }
        if (0x80u > code_point)
        {
            AppendCharacter(pMessage, (int)code_point);
        }
        else if (0x800u > code_point)
        {
            AppendCharacter(pMessage, (int)(0xc0u | (code_point >> 6)));
            AppendCharacter(pMessage, (int)(0x80u | (code_point & 0x3fu)));
        }
        else if (0x10000u > code_point)
        {
            AppendCharacter(pMessage, (int)(0xe0u | (code_point >> 12)));
            AppendCharacter(pMessage, (int)(0x80u | ((code_point >> 6) & 0x3fu)));
            AppendCharacter(pMessage, (int)(0x80u | (code_point & 0x3fu)));
        }
        else
        {
            AppendCharacter(pMessage, (int)(0xf0u | (code_point >> 18)));
            AppendCharacter(pMessage, (int)(0x80u | ((code_point >> 12) & 0x3fu)));
            AppendCharacter(pMessage, (int)(0x80u | ((code_point >> 6) & 0x3fu)));
            AppendCharacter(pMessage, (int)(0x80u | (code_point & 0x3fu)));
        }
    }
}

void Os_MessageBuilder_AppendInt(Os_MessageBuilder_t* pMessage, int64_t value)
{
    uint64_t magnitude;

    magnitude = (uint64_t)value;
    if (value < 0)
    {
        AppendCharacter(pMessage, '-');
        magnitude = (uint64_t)0 - magnitude;
    }
    Os_MessageBuilder_AppendUint(pMessage, magnitude);
}

void Os_MessageBuilder_AppendUint(Os_MessageBuilder_t* pMessage, uint64_t value)
{
    AppendDigits(pMessage, value, 10, 1, c_decimalDigits);
}

void Os_MessageBuilder_AppendHex(Os_MessageBuilder_t* pMessage, uint64_t value, uint32_t minimumDigits, bool_t uppercase)
{
    const char_t* digits;

    digits = c_lowerHexDigits;
    if (uppercase)
    {
        digits = c_upperHexDigits;
    }
    AppendDigits(pMessage, value, 16, minimumDigits, digits);
}

void Os_MessageBuilder_AppendPointer(Os_MessageBuilder_t* pMessage, const void* pValue)
{
    Os_MessageBuilder_AppendString(pMessage, "0x");
    Os_MessageBuilder_AppendHex(pMessage, (uint64_t)(uintptr_t)pValue, (uint32_t)(2u * sizeof(uintptr_t)), FALSE);
}

void Os_MessageBuilder_AppendFloat(Os_MessageBuilder_t* pMessage, float value)
{
    double   scaled;
    uint32_t significand;
    uint32_t divisor;
    int32_t  exponent;

    if (isnan(value))
    {
        Os_MessageBuilder_AppendString(pMessage, "nan");
        return;
    }
    if (signbit(value))
    {
        AppendCharacter(pMessage, '-');
    }
    if (isinf(value))
    {
        Os_MessageBuilder_AppendString(pMessage, "inf");
        return;
    }
    scaled = (double)value;
    if (scaled < 0.0)
    {
        scaled = -scaled;
    }
    if (scaled == 0.0)
    {
        AppendCharacter(pMessage, '0');
        return;
    }
    exponent = 0;
    while (scaled >= 10.0)
    {
        scaled /= 10.0;
        ++exponent;
    }
    while (scaled < 1.0)
    {
        scaled *= 10.0;
        --exponent;
    }
    significand = (uint32_t)(scaled * 100000000.0 + 0.5);
    if (significand == 1000000000u)
    {
        significand = 100000000u;
        ++exponent;
    }
    divisor = 100000000u;
    AppendCharacter(pMessage, c_decimalDigits[significand / divisor]);
    significand %= divisor;
    if (significand)
    {
        AppendCharacter(pMessage, '.');
        while (significand)
        {
            divisor /= 10u;
            AppendCharacter(pMessage, c_decimalDigits[significand / divisor]);
            significand %= divisor;
        }
    }
    AppendCharacter(pMessage, 'e');
    if (exponent < 0)
    {
        AppendCharacter(pMessage, '-');
        exponent = -exponent;
    }
    else
    {
        AppendCharacter(pMessage, '+');
    }
    AppendDigits(pMessage, (uint64_t)exponent, 10, 2, c_decimalDigits);
}
