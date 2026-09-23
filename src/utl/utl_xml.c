/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * W3C Extensible Markup Language (XML) 1.0 (Fifth Edition):
 * https://www.w3.org/TR/2008/REC-xml-20081126/
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "utl/utl_xml.h"
#include "utl/utl_defines.h"

#include "os/os_assert.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

typedef struct XmlCodePointRange_s
{
    uint32_t first;
    uint32_t last;
} XmlCodePointRange_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Allowed first-character ranges from XML 1.0 (Fifth Edition), production [4] NameStartChar.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameStartChar
 */
static const XmlCodePointRange_t c_xmlNameStartRanges[] =
{
    {':',     ':'},
    {'A',     'Z'},
    {'_',     '_'},
    {'a',     'z'},
    {0xC0,    0xD6},
    {0xD8,    0xF6},
    {0xF8,    0x2FF},
    {0x370,   0x37D},
    {0x37F,   0x1FFF},
    {0x200C,  0x200D},
    {0x2070,  0x218F},
    {0x2C00,  0x2FEF},
    {0x3001,  0xD7FF},
    {0xF900,  0xFDCF},
    {0xFDF0,  0xFFFD},
    {0x10000, 0xEFFFF},
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsChar(uint32_t codePoint)
{
    bool_t is_tab                   = (UTL_XML_CHAR_TAB == codePoint);
    bool_t is_line_feed             = (UTL_XML_CHAR_LF == codePoint);
    bool_t is_carriage_return       = (UTL_XML_CHAR_CR == codePoint);
    bool_t is_bmp_before_surrogates = ((0x20 <= codePoint) && (codePoint <= 0xD7FF));
    bool_t is_bmp_after_surrogates  = ((0xE000 <= codePoint) && (codePoint <= 0xFFFD));
    bool_t is_supplementary         = ((0x10000 <= codePoint) && (codePoint <= 0x10FFFF));

    return is_tab || is_line_feed || is_carriage_return || is_bmp_before_surrogates || is_bmp_after_surrogates || is_supplementary;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsWhitespace(uint32_t codePoint)
{
    bool_t is_space           = (UTL_XML_CHAR_SPACE == codePoint);
    bool_t is_tab             = (UTL_XML_CHAR_TAB == codePoint);
    bool_t is_carriage_return = (UTL_XML_CHAR_CR == codePoint);
    bool_t is_line_feed       = (UTL_XML_CHAR_LF == codePoint);

    return is_space || is_tab || is_carriage_return || is_line_feed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsNameStartChar(uint32_t codePoint)
{
    uint32_t range_index;

    for (range_index = 0; range_index < COUNT_OF(c_xmlNameStartRanges); ++range_index)
    {
        if (codePoint <= c_xmlNameStartRanges[range_index].last)
        {
            return codePoint >= c_xmlNameStartRanges[range_index].first;
        }
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsNameChar(uint32_t codePoint)
{
    bool_t is_name_start_char       = Utl_Xml_IsNameStartChar(codePoint);
    bool_t is_hyphen                = ('-' == codePoint);
    bool_t is_period                = ('.' == codePoint);
    bool_t is_digit                 = (('0' <= codePoint) && (codePoint <= '9'));
    bool_t is_middle_dot            = (0xB7 == codePoint);
    bool_t is_combining_mark        = ((0x300 <= codePoint) && (codePoint <= 0x36F));
    bool_t is_connector_punctuation = ((0x203F <= codePoint) && (codePoint <= 0x2040));

    return is_name_start_char || is_hyphen || is_period || is_digit || is_middle_dot || is_combining_mark || is_connector_punctuation;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsName(const uint32_t* codePoints, uint32_t count)
{
    uint32_t code_point_index;
    bool_t   is_name;

    LOGASSERT(NULL != codePoints);
    LOGASSERT(0 < count);

    is_name = TRUE;

    for (code_point_index = 0; code_point_index < count; ++code_point_index)
    {
        if (0 == code_point_index)
        {
            is_name = Utl_Xml_IsNameStartChar(codePoints[code_point_index]);
        }
        else
        {
            is_name = Utl_Xml_IsNameChar(codePoints[code_point_index]);
        }

        if (FALSE == is_name)
        {
            break;
        }
    }

    return is_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsNmtoken(const uint32_t* codePoints, uint32_t count)
{
    bool_t   is_nmtoken;
    uint32_t code_point_index;

    LOGASSERT(NULL != codePoints);
    LOGASSERT(0 < count);

    is_nmtoken = TRUE;

    for (code_point_index = 0; code_point_index < count; ++code_point_index)
    {
        is_nmtoken = Utl_Xml_IsNameChar(codePoints[code_point_index]);

        if (FALSE == is_nmtoken)
        {
            break;
        }
    }

    return is_nmtoken;
}
