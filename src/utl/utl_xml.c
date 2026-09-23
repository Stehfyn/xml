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

#include "os/os_assert.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

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
    return (codePoint == (uint32_t)UTL_XML_CHAR_TAB) || (codePoint == (uint32_t)UTL_XML_CHAR_LF) || (codePoint == (uint32_t)UTL_XML_CHAR_CR) ||
           ((codePoint >= (uint32_t)0x20) && (codePoint <= (uint32_t)0xD7FF)) || ((codePoint >= (uint32_t)0xE000) && (codePoint <= (uint32_t)0xFFFD)) ||
           ((codePoint >= (uint32_t)0x10000) && (codePoint <= (uint32_t)0x10FFFF));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsWhitespace(uint32_t codePoint)
{
    return (codePoint == (uint32_t)UTL_XML_CHAR_SPACE) || (codePoint == (uint32_t)UTL_XML_CHAR_TAB) || (codePoint == (uint32_t)UTL_XML_CHAR_CR) || (codePoint == (uint32_t)UTL_XML_CHAR_LF);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsNameStartChar(uint32_t codePoint)
{
    return (codePoint == (uint32_t)0x3A) || ((codePoint >= (uint32_t)0x41) && (codePoint <= (uint32_t)0x5A)) || (codePoint == (uint32_t)0x5F) ||
           ((codePoint >= (uint32_t)0x61) && (codePoint <= (uint32_t)0x7A)) || ((codePoint >= (uint32_t)0xC0) && (codePoint <= (uint32_t)0xD6)) ||
           ((codePoint >= (uint32_t)0xD8) && (codePoint <= (uint32_t)0xF6)) || ((codePoint >= (uint32_t)0xF8) && (codePoint <= (uint32_t)0x2FF)) ||
           ((codePoint >= (uint32_t)0x370) && (codePoint <= (uint32_t)0x37D)) || ((codePoint >= (uint32_t)0x37F) && (codePoint <= (uint32_t)0x1FFF)) ||
           ((codePoint >= (uint32_t)0x200C) && (codePoint <= (uint32_t)0x200D)) || ((codePoint >= (uint32_t)0x2070) && (codePoint <= (uint32_t)0x218F)) ||
           ((codePoint >= (uint32_t)0x2C00) && (codePoint <= (uint32_t)0x2FEF)) || ((codePoint >= (uint32_t)0x3001) && (codePoint <= (uint32_t)0xD7FF)) ||
           ((codePoint >= (uint32_t)0xF900) && (codePoint <= (uint32_t)0xFDCF)) || ((codePoint >= (uint32_t)0xFDF0) && (codePoint <= (uint32_t)0xFFFD)) ||
           ((codePoint >= (uint32_t)0x10000) && (codePoint <= (uint32_t)0xEFFFF));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_IsNameChar(uint32_t codePoint)
{
    return (Utl_Xml_IsNameStartChar(codePoint)) || (codePoint == (uint32_t)0x2D) || (codePoint == (uint32_t)0x2E) || ((codePoint >= (uint32_t)0x30) && (codePoint <= (uint32_t)0x39)) ||
           (codePoint == (uint32_t)0xB7) || ((codePoint >= (uint32_t)0x300) && (codePoint <= (uint32_t)0x36F)) || ((codePoint >= (uint32_t)0x203F) && (codePoint <= (uint32_t)0x2040));
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

    is_name = Utl_Xml_IsNameStartChar(codePoints[0]);

    for (code_point_index = 1; code_point_index < count; ++code_point_index)
    {
        is_name = (is_name && (Utl_Xml_IsNameChar(codePoints[code_point_index])));
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
