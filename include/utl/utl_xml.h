#ifndef UTL_XML_H
#define UTL_XML_H

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

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Maximum accepted UTF-8 name and value byte counts. */
#define UTL_XML_MAX_NAME_BYTES  (1024)
#define UTL_XML_MAX_VALUE_BYTES (1048576)

/* XML 1.0 sections 2.2 and 2.3, productions [2] Char and [3] S. */
#define UTL_XML_CHAR_TAB   ('\t') ///< Horizontal tab.
#define UTL_XML_CHAR_LF    ('\n') ///< Line feed; normalized line ending (section 2.11).
#define UTL_XML_CHAR_CR    ('\r') ///< Carriage return.
#define UTL_XML_CHAR_SPACE (' ')  ///< Space.

/* XML 1.0 section 2.4, production [14] CharData. */
#define UTL_XML_CHAR_AMP ('&') ///< Ampersand.
#define UTL_XML_CHAR_LT  ('<') ///< Less-than sign.
#define UTL_XML_CHAR_GT  ('>') ///< Greater-than sign.

/* XML 1.0 section 2.3, production [10] AttValue, and section 2.8, [25] Eq. */
#define UTL_XML_QUOTE_APOS ('\'') ///< Single-quoted literal delimiter.
#define UTL_XML_QUOTE_QUOT ('"')  ///< Double-quoted literal delimiter.
#define UTL_XML_EQ         ('=')  ///< Eq also permits surrounding S.

/* XML 1.0 section 2.5, production [15] Comment. */
#define UTL_XML_COMMENT_OPEN  ("<!--") ///< Comment opening delimiter.
#define UTL_XML_COMMENT_CLOSE ("-->")  ///< Comment closing delimiter.

/* XML 1.0 section 2.6, production [16] PI. */
#define UTL_XML_PI_OPEN  ("<?") ///< Processing-instruction opening delimiter.
#define UTL_XML_PI_CLOSE ("?>") ///< Processing-instruction closing delimiter.

/* XML 1.0 section 2.7, productions [19] CDStart and [21] CDEnd. */
#define UTL_XML_CD_START ("<![CDATA[") ///< CDStart.
#define UTL_XML_CD_END   ("]]>")       ///< CDEnd; also forbidden inside CharData ([14]).

/* XML 1.0 section 2.8, productions [23] XMLDecl, [24] VersionInfo, [26] VersionNum. */
#define UTL_XML_DECL_OPEN    ("<?xml")   ///< XMLDecl opening delimiter; S follows.
#define UTL_XML_DECL_CLOSE   ("?>")      ///< XMLDecl closing delimiter.
#define UTL_XML_VERSION_NAME ("version") ///< VersionInfo keyword.
#define UTL_XML_VERSION_1_0  ("1.0")     ///< XML 1.0 version value.

/* XML 1.0 section 2.8, production [28] doctypedecl. */
#define UTL_XML_DOCTYPE_OPEN          ("<!DOCTYPE") ///< doctypedecl opening delimiter; S follows.
#define UTL_XML_DOCTYPE_CLOSE         (">")         ///< doctypedecl closing delimiter.
#define UTL_XML_INTERNAL_SUBSET_OPEN  ("[")         ///< Begins intSubset.
#define UTL_XML_INTERNAL_SUBSET_CLOSE ("]")         ///< Ends intSubset.

/* XML 1.0 section 2.9, production [32] SDDecl. */
#define UTL_XML_STANDALONE_NAME ("standalone") ///< SDDecl keyword.
#define UTL_XML_STANDALONE_YES  ("yes")        ///< No external declarations affect document information.
#define UTL_XML_STANDALONE_NO   ("no")         ///< External declarations may affect document information.

/* XML 1.0 section 3.1, productions [40] STag, [42] ETag, [44] EmptyElemTag. */
#define UTL_XML_STAG_OPEN            ("<")  ///< STag and EmptyElemTag opening delimiter.
#define UTL_XML_ETAG_OPEN            ("</") ///< ETag opening delimiter.
#define UTL_XML_TAG_CLOSE            (">")  ///< STag and ETag closing delimiter.
#define UTL_XML_EMPTY_ELEM_TAG_CLOSE ("/>") ///< EmptyElemTag closing delimiter.

/* XML 1.0 section 4.1, productions [66] CharRef, [68] EntityRef, [69] PEReference. */
#define UTL_XML_CHAR_REF_DEC_OPEN ("&#")  ///< Decimal CharRef prefix.
#define UTL_XML_CHAR_REF_HEX_OPEN ("&#x") ///< Hexadecimal CharRef prefix; x is lowercase.
#define UTL_XML_ENTITY_REF_OPEN   ("&")   ///< EntityRef prefix.
#define UTL_XML_PE_REFERENCE_OPEN ("%")   ///< PEReference prefix.
#define UTL_XML_REFERENCE_CLOSE   (";")   ///< CharRef, EntityRef, and PEReference terminator.

/* XML 1.0 [66] CharRef; sections 2.11 and 3.3.3. */
#define UTL_XML_CHAR_REF_TAB ("&#x9;") ///< Preserves tab in attribute values.
#define UTL_XML_CHAR_REF_LF  ("&#xA;") ///< Preserves line feed in attribute values.
#define UTL_XML_CHAR_REF_CR  ("&#xD;") ///< Preserves carriage return through line-end normalization.

/* XML 1.0 section 4.3.3, productions [80] EncodingDecl and [81] EncName. */
#define UTL_XML_ENCODING_NAME  ("encoding") ///< EncodingDecl keyword.
#define UTL_XML_ENCODING_UTF_8 ("UTF-8")    ///< Encoding name for UTF-8 input.

/* XML 1.0 section 4.6: references to the five predefined general entities. */
#define UTL_XML_ENTITY_REF_LT   ("&lt;")   ///< Expands to <.
#define UTL_XML_ENTITY_REF_GT   ("&gt;")   ///< Expands to >.
#define UTL_XML_ENTITY_REF_AMP  ("&amp;")  ///< Expands to &.
#define UTL_XML_ENTITY_REF_APOS ("&apos;") ///< Expands to '.
#define UTL_XML_ENTITY_REF_QUOT ("&quot;") ///< Expands to ".

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
 * \brief Checks whether a decoded code point is a permitted XML 1.0 character.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [2] Char:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Char
 *
 * \par Requirements:
 *  - 001 This function shall return TRUE for XML 1.0 section 2.2, production [2] Char.
 *  - 002 This function shall accept Char values even when the section 2.2 note discourages their use.
 *  - 003 This function shall return FALSE otherwise, including for surrogates and values above U+10FFFF.
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 character.
 * \retval    FALSE       otherwise
 */
bool_t Utl_Xml_IsChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span against XML 1.0 Char.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [2] Char:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Char
 *
 * \par Requirements:
 *  - 001 This function shall accept an empty span and reject invalid UTF-8 or non-Char code points, including zero bytes.
 *  - 002 This function shall check exactly byteCount bytes and reject spans longer than UTL_XML_MAX_VALUE_BYTES.
 *  - 003 This function shall fail an assertion if utf8Text is NULL.
 *
 * \param[in] utf8Text   Non-NULL UTF-8 byte span
 * \param[in] byteCount  Number of bytes in the span
 *
 * \retval TRUE          if the complete string is valid XML character data.
 * \retval FALSE         otherwise
 */
bool_t Utl_Xml_IsString(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code point is a permitted XML 1.0 whitespace character.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [3] S:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-S
 *
 * \par Requirements:
 *  - 001 This function shall return TRUE for space, tab, CR, and LF (XML 1.0 section 2.3, production [3] S).
 *  - 002 This function shall return FALSE for every other code point.
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 whitespace character.
 * \retval    FALSE       otherwise
 */
bool_t Utl_Xml_IsWhitespace(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code point is a permitted XML 1.0 NameStartChar.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [4] NameStartChar:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameStartChar
 *
 * \par Requirements:
 *  - 001 This function shall return TRUE for XML 1.0 section 2.3, production [4] NameStartChar.
 *  - 002 This function shall accept colon without applying namespace constraints.
 *  - 003 This function shall return FALSE for every other code point.
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 NameStartChar.
 * \retval    FALSE       otherwise
 */
bool_t Utl_Xml_IsNameStartChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code point is a permitted XML 1.0 NameChar.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [4a] NameChar:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameChar
 *
 * \par Requirements:
 *  - 001 This function shall return TRUE for XML 1.0 section 2.3, production [4a] NameChar.
 *  - 002 This function shall accept every NameStartChar and the additional NameChar values.
 *  - 003 This function shall return FALSE for every other code point.
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 NameChar.
 * \retval    FALSE       otherwise
 */
bool_t Utl_Xml_IsNameChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span against XML 1.0 Name.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [5] Name:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Name
 *
 * \par Requirements:
 *  - 001 This function shall require a non-empty NameStartChar followed by zero or more NameChar values.
 *  - 002 This function shall check exactly byteCount bytes and reject invalid UTF-8, zero bytes, and spans longer than UTL_XML_MAX_NAME_BYTES.
 *  - 003 This function shall fail an assertion if utf8Text is NULL.
 *
 * \param[in] utf8Text   Non-NULL UTF-8 byte span
 * \param[in] byteCount  Number of bytes in the span
 *
 * \retval TRUE          if the complete string is a permitted XML 1.0 Name.
 * \retval FALSE         otherwise
 */
bool_t Utl_Xml_IsName(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code-point span is a permitted XML 1.0 Nmtoken.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [7] Nmtoken:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Nmtoken
 *
 * \par Requirements:
 *  - 001 This function shall return TRUE only for XML 1.0 section 2.3, production [7] Nmtoken: one or more NameChar values.
 *  - 002 This function shall allow any NameChar first, without requiring NameStartChar.
 *  - 003 This function shall check exactly numCodePoints values, require no terminator, and reject zero within the span.
 *  - 004 This function shall fail an assertion if codePoints is NULL.
 *  - 005 This function shall fail an assertion if numCodePoints is zero.
 *
 * \param[in] codePoints      Non-NULL array of at least numCodePoints decoded Unicode code points
 * \param[in] numCodePoints   Nonzero number of code points in the span
 *
 * \retval    TRUE            if the complete span is a permitted XML 1.0 Nmtoken.
 * \retval    FALSE           otherwise
 */
bool_t Utl_Xml_IsNmtoken(const uint32_t* codePoints, uint32_t numCodePoints);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span for XML 1.0 comment content.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [15] Comment:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Comment
 *
 * \par Requirements:
 *  - 001 This function shall reject a double hyphen or trailing hyphen.
 *  - 002 This function shall check exactly byteCount bytes and reject invalid UTF-8, non-Char values, and spans longer than UTL_XML_MAX_VALUE_BYTES.
 *  - 003 This function shall fail an assertion if utf8Text is NULL.
 *
 * \param[in] utf8Text   Non-NULL UTF-8 byte span
 * \param[in] byteCount  Number of bytes in the span
 *
 * \retval TRUE          if the span is permitted XML 1.0 comment content.
 * \retval FALSE         otherwise
 */
bool_t Utl_Xml_IsCommentContent(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span for XML 1.0 processing-instruction data.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [16] PI:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-PI
 *
 * \par Requirements:
 *  - 001 This function shall reject the ?> delimiter within the data.
 *  - 002 This function shall check exactly byteCount bytes and reject invalid UTF-8, non-Char values, and spans longer than UTL_XML_MAX_VALUE_BYTES.
 *  - 003 This function shall fail an assertion if utf8Text is NULL.
 *
 * \param[in] utf8Text   Non-NULL UTF-8 byte span
 * \param[in] byteCount  Number of bytes in the span
 *
 * \retval TRUE          if the span is permitted XML 1.0 PI data.
 * \retval FALSE         otherwise
 */
bool_t Utl_Xml_IsPIData(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span against XML 1.0 PITarget.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [17] PITarget:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-PITarget
 *
 * \par Requirements:
 *  - 001 This function shall accept a Name except any case variant of xml.
 *  - 002 This function shall check exactly byteCount bytes and reject invalid UTF-8, zero bytes, and spans longer than UTL_XML_MAX_NAME_BYTES.
 *  - 003 This function shall fail an assertion if utf8Text is NULL.
 *
 * \param[in] utf8Text   Non-NULL UTF-8 byte span
 * \param[in] byteCount  Number of bytes in the span
 *
 * \retval TRUE          if the complete string is a permitted XML 1.0 PITarget.
 * \retval FALSE         otherwise
 */
bool_t Utl_Xml_IsPITarget(const byte_t* utf8Text, uint32_t byteCount);

#endif /* UTL_XML_H */
