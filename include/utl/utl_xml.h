#ifndef UTL_XML_H
#define UTL_XML_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * W3C Extensible Markup Language (XML) 1.0 (Fifth Edition):
 * https://www.w3.org/TR/2008/REC-xml-20081126/
 * Section and production numbers below refer to this edition.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* XML 1.0 sections 2.2 and 2.3, productions [2] Char and [3] S. */
#define UTL_XML_CHAR_TAB   (0x09u) ///< Horizontal tab.
#define UTL_XML_CHAR_LF    (0x0Au) ///< Line feed; normalized line ending (section 2.11).
#define UTL_XML_CHAR_CR    (0x0Du) ///< Carriage return.
#define UTL_XML_CHAR_SPACE (0x20u) ///< Space.

/* XML 1.0 section 2.3, production [10] AttValue, and section 2.8, [25] Eq. */
#define UTL_XML_QUOTE_APOS ('\'') ///< Single-quoted literal delimiter.
#define UTL_XML_QUOTE_QUOT ('"')  ///< Double-quoted literal delimiter.
#define UTL_XML_EQ         ('=')  ///< Eq also permits surrounding S.

/* XML 1.0 section 2.5, production [15] Comment. */
#define UTL_XML_COMMENT_OPEN  "<!--" ///< Comment opening delimiter.
#define UTL_XML_COMMENT_CLOSE "-->" ///< Comment closing delimiter.

/* XML 1.0 section 2.6, production [16] PI. */
#define UTL_XML_PI_OPEN  "<?" ///< Processing-instruction opening delimiter.
#define UTL_XML_PI_CLOSE "?>" ///< Processing-instruction closing delimiter.

/* XML 1.0 section 2.7, productions [19] CDStart and [21] CDEnd. */
#define UTL_XML_CD_START "<![CDATA[" ///< CDStart.
#define UTL_XML_CD_END   "]]>"      ///< CDEnd; also forbidden inside CharData ([14]).

/* XML 1.0 section 2.8, productions [23] XMLDecl, [24] VersionInfo, [26] VersionNum. */
#define UTL_XML_DECL_OPEN    "<?xml"  ///< XMLDecl opening delimiter; S follows.
#define UTL_XML_DECL_CLOSE   "?>"     ///< XMLDecl closing delimiter.
#define UTL_XML_VERSION_NAME "version" ///< VersionInfo keyword.
#define UTL_XML_VERSION_1_0  "1.0"     ///< XML 1.0 version value.

/* XML 1.0 section 2.8, production [28] doctypedecl. */
#define UTL_XML_DOCTYPE_OPEN          "<!DOCTYPE" ///< doctypedecl opening delimiter; S follows.
#define UTL_XML_DOCTYPE_CLOSE         ">"         ///< doctypedecl closing delimiter.
#define UTL_XML_INTERNAL_SUBSET_OPEN  "["         ///< Begins intSubset.
#define UTL_XML_INTERNAL_SUBSET_CLOSE "]"         ///< Ends intSubset.

/* XML 1.0 section 2.9, production [32] SDDecl. */
#define UTL_XML_STANDALONE_NAME "standalone" ///< SDDecl keyword.
#define UTL_XML_STANDALONE_YES  "yes"        ///< No external declarations affect document information.
#define UTL_XML_STANDALONE_NO   "no"         ///< External declarations may affect document information.

/* XML 1.0 section 3.1, productions [40] STag, [42] ETag, [44] EmptyElemTag. */
#define UTL_XML_STAG_OPEN            "<"  ///< STag and EmptyElemTag opening delimiter.
#define UTL_XML_ETAG_OPEN            "</" ///< ETag opening delimiter.
#define UTL_XML_TAG_CLOSE            ">"  ///< STag and ETag closing delimiter.
#define UTL_XML_EMPTY_ELEM_TAG_CLOSE "/>" ///< EmptyElemTag closing delimiter.

/* XML 1.0 section 4.1, productions [66] CharRef, [68] EntityRef, [69] PEReference. */
#define UTL_XML_CHAR_REF_DEC_OPEN "&#"  ///< Decimal CharRef prefix.
#define UTL_XML_CHAR_REF_HEX_OPEN "&#x" ///< Hexadecimal CharRef prefix; x is lowercase.
#define UTL_XML_ENTITY_REF_OPEN   "&"   ///< EntityRef prefix.
#define UTL_XML_PE_REFERENCE_OPEN "%"   ///< PEReference prefix.
#define UTL_XML_REFERENCE_CLOSE   ";"   ///< CharRef, EntityRef, and PEReference terminator.

/* XML 1.0 section 4.3.3, productions [80] EncodingDecl and [81] EncName. */
#define UTL_XML_ENCODING_NAME   "encoding" ///< EncodingDecl keyword.
#define UTL_XML_ENCODING_UTF_8  "UTF-8"    ///< Required processor encoding.
#define UTL_XML_ENCODING_UTF_16 "UTF-16"   ///< Required processor encoding.

/* XML 1.0 section 4.6: references to the five predefined general entities. */
#define UTL_XML_ENTITY_REF_LT   "&lt;"   ///< Expands to <.
#define UTL_XML_ENTITY_REF_GT   "&gt;"   ///< Expands to >.
#define UTL_XML_ENTITY_REF_AMP  "&amp;"  ///< Expands to &.
#define UTL_XML_ENTITY_REF_APOS "&apos;" ///< Expands to '.
#define UTL_XML_ENTITY_REF_QUOT "&quot;" ///< Expands to ".

#endif /* UTL_XML_H */
