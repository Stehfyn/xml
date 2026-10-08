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

#include "faceoe_utl/faceoe_utl_xml.h"
#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_util.h"

#include "os/os_assert.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* XML 1.0 sections 2.2 and 2.3, productions [2] Char and [3] S. */
#define CHAR_TAB   ('\t') ///< Horizontal tab.
#define CHAR_LF    ('\n') ///< Line feed; normalized line ending (section 2.11).
#define CHAR_CR    ('\r') ///< Carriage return.
#define CHAR_SPACE (' ')  ///< Space.

/* XML 1.0 section 2.4, production [14] CharData. */
#define CHAR_AMP ('&') ///< Ampersand.
#define CHAR_LT  ('<') ///< Less-than sign.
#define CHAR_GT  ('>') ///< Greater-than sign.

/* XML 1.0 section 2.3, production [10] AttValue, and section 2.8, [25] Eq. */
#define QUOTE_QUOT ('"')  ///< Double quote, the attribute value delimiter.
#define EQ         ('=')  ///< Equals sign; Eq also permits surrounding S, which the reader rejects.

/* XML 1.0 section 2.8, productions [23] XMLDecl, [24] VersionInfo, [26] VersionNum. */
#define DECL_OPEN    ("<?xml")   ///< XMLDecl opening delimiter; S follows.
#define DECL_CLOSE   ("?>")      ///< XMLDecl closing delimiter.
#define VERSION_NAME ("version") ///< VersionInfo keyword.
#define VERSION_1_0  ("1.0")     ///< XML 1.0 version value.
#define VERSION_1_0_BYTE_COUNT (sizeof(VERSION_1_0) - 1)

/* XML 1.0 section 3.1, productions [40] STag, [42] ETag, [44] EmptyElemTag. */
#define STAG_OPEN            ("<")  ///< STag and EmptyElemTag opening delimiter.
#define ETAG_OPEN            ("</") ///< ETag opening delimiter.
#define TAG_CLOSE            (">")  ///< STag and ETag closing delimiter.
#define EMPTY_ELEM_TAG_CLOSE ("/>") ///< EmptyElemTag closing delimiter.

#define TOKEN_MAX_BYTES (sizeof(VERSION_NAME)) ///< Longest supported token, including its zero terminator.
#define INDENT_SPACES_PER_LEVEL (4)

/* UTF-8 leading-byte ranges and their payload bits. */
#define UTF8_ASCII_MAX_BYTE               (0x7F)
#define UTF8_TWO_BYTE_LEAD_MIN            (0xC2)
#define UTF8_TWO_BYTE_LEAD_MAX            (0xDF)
#define UTF8_THREE_BYTE_LEAD_MIN          (0xE0)
#define UTF8_THREE_BYTE_LEAD_MAX          (0xEF)
#define UTF8_FOUR_BYTE_LEAD_MIN           (0xF0)
#define UTF8_FOUR_BYTE_LEAD_MAX           (0xF4)
#define UTF8_TWO_BYTE_LEAD_PAYLOAD_MASK   (0x1F)
#define UTF8_THREE_BYTE_LEAD_PAYLOAD_MASK (0x0F)
#define UTF8_FOUR_BYTE_LEAD_PAYLOAD_MASK  (0x07)

/* A continuation byte has the form 10xxxxxx. */
#define UTF8_CONTINUATION_PREFIX        (0x80)
#define UTF8_CONTINUATION_PREFIX_MASK   (0xC0)
#define UTF8_CONTINUATION_PAYLOAD_MASK  (0x3F)
#define UTF8_CONTINUATION_PAYLOAD_BITS  (6)

/* Shortest-form minima and Unicode scalar-value bounds. */
#define UTF8_TWO_BYTE_MIN_CODE_POINT   (0x80)
#define UTF8_THREE_BYTE_MIN_CODE_POINT (0x800)
#define UTF8_FOUR_BYTE_MIN_CODE_POINT  (0x10000)
#define UTF8_MAX_CODE_POINT            (0x10FFFF)
#define UTF8_SURROGATE_MIN_CODE_POINT  (0xD800)
#define UTF8_SURROGATE_MAX_CODE_POINT  (0xDFFF)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Inclusive range of Unicode code points used by XML character productions.
 */
typedef struct XmlCodePointRange_s
{
    uint32_t first; ///< Lowest code point in the range.
    uint32_t last;  ///< Highest code point in the range.
} XmlCodePointRange_t;

/*************************************************************************************************/
/**
 * \brief Input position and caller-owned storage shared by the document reader.
 */
typedef struct XmlReader_s
{
    FACEOE_Utl_Xml_Document_t*  document;          ///< Document receiving parsed nodes.
    byte_t*                     bytes;             ///< Input with one writable terminating byte.
    uint64_t                    byteCount;         ///< Number of input bytes.
    uint64_t                    position;          ///< Offset of the next unread byte.
    FACEOE_Utl_Xml_Node_t*      nodes;             ///< Caller-owned node storage.
    uint32_t                    nodeCapacity;      ///< Number of nodes available.
    uint32_t                    nodeCount;         ///< Number of nodes used.
    FACEOE_Utl_Xml_Attribute_t* attributes;        ///< Caller-owned attribute storage.
    uint32_t                    attributeCapacity; ///< Number of attributes available.
    uint32_t                    attributeCount;    ///< Number of attributes used.
    uint32_t                    markupCount;       ///< Number of markup spans read across the document.
} XmlReader_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Allowed first-character ranges from XML 1.0 (Fifth Edition), production [4] NameStartChar.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameStartChar
 */
static const XmlCodePointRange_t c_xmlNameStartCharRanges[] =
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
    {0x10000, 0xEFFFF}
};

/*************************************************************************************************/
/**
 * \brief Ranges that XML 1.0 (Fifth Edition) production [4a] NameChar adds to NameStartChar.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameChar
 */
static const XmlCodePointRange_t c_xmlNameCharRanges[] =
{
    {'-',    '-'},
    {'.',    '.'},
    {'0',    '9'},
    {0xB7,   0xB7},
    {0x300,  0x36F},
    {0x203F, 0x2040}
};

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Checks whether a code point is in an inclusive range.
 *
 * \param[in] codePoint   Code point to check.
 * \param[in] first       Inclusive lower bound.
 * \param[in] last        Inclusive upper bound.
 *
 * \retval    TRUE        if code point is within the range.
 * \retval    FALSE       otherwise
 */
static bool_t InRange(uint32_t codePoint, uint32_t first, uint32_t last);

/*************************************************************************************************/
/**
 * \brief Decodes one strictly encoded UTF-8 code point within a byte span.
 *
 * \param[in]     bytes              Non-NULL UTF-8 byte span
 * \param[in]     numBytes           Number of bytes in the span
 * \param[in]     offset             Offset of the code point's first byte
 * \param[out]    decodedByteCount   Number of bytes decoded, or zero on failure
 *
 * \retval        0..0x10FFFF   decoded code point when decodedByteCount is nonzero; U+0000 is valid UTF-8.
 * \retval        0             if decoding fails; decodedByteCount is zero.
 */
static uint32_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t offset, uint32_t* decodedByteCount);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code point is a permitted XML 1.0 character.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [2] Char:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Char
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 character.
 * \retval    FALSE       otherwise
 */
static bool_t IsChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span against XML 1.0 Char.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [2] Char:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Char
 *
 * \param[in] utf8Text    Non-NULL UTF-8 byte span
 * \param[in] byteCount   Number of bytes in the span
 *
 * \retval    TRUE        if the span is valid UTF-8 and every code point is a Char.
 * \retval    FALSE       otherwise
 */
static bool_t IsString(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Checks whether a code point or byte is XML 1.0 whitespace.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [3] S:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-S
 *
 * \param[in] codePoint   Code point or byte to classify
 *
 * \retval    TRUE        if the code point is space, tab, CR, or LF.
 * \retval    FALSE       otherwise
 */
static bool_t IsWhitespace(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code point is a permitted XML 1.0 NameStartChar.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [4] NameStartChar:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameStartChar
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 NameStartChar.
 * \retval    FALSE       otherwise
 */
static bool_t IsNameStartChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks whether a decoded code point is a permitted XML 1.0 NameChar.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [4a] NameChar:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameChar
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is a permitted XML 1.0 NameChar.
 * \retval    FALSE       otherwise
 */
static bool_t IsNameChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Checks a counted UTF-8 byte span against XML 1.0 Name.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [5] Name:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Name
 *
 * \param[in] utf8Text    Non-NULL UTF-8 byte span
 * \param[in] byteCount   Number of bytes in the span, 1 to FACEOE_UTL_XML_MAX_NAME_BYTES
 *
 * \retval    TRUE        if the span is a permitted XML 1.0 Name.
 * \retval    FALSE       otherwise
 */
static bool_t IsName(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Checks whether a code point is permitted literally in a double-quoted XML attribute value.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [10] AttValue:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-AttValue
 *  Tab, LF, and CR are permitted literally; section 3.3.3 normalizes them to spaces.
 *
 * \param[in] codePoint   Decoded Unicode code point to classify
 *
 * \retval    TRUE        if the code point is permitted literally in a double-quoted attribute value.
 * \retval    FALSE       otherwise
 */
static bool_t IsAttributeValueChar(uint32_t codePoint);

/*************************************************************************************************/
/**
 * \brief Finds an attribute with the requested name.
 *
 * \param[in] node            Element whose attributes are searched
 * \param[in] attribute       Attribute being appended, asserted absent from the list, or NULL
 * \param[in] name            Name bytes to find
 * \param[in] nameByteCount   Number of name bytes
 *
 * \retval    non-NULL   matching attribute.
 * \retval    NULL       if the name is absent.
 */
static const FACEOE_Utl_Xml_Attribute_t* FindAttribute(const FACEOE_Utl_Xml_Node_t* node, const FACEOE_Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount);

/*************************************************************************************************/
/**
 * \brief Finds child in parent's ancestor chain, including parent.
 *
 * \param[in] parent   Node whose parent chain is searched
 * \param[in] child    Node to find
 *
 * \retval    non-NULL   matching ancestor.
 * \retval    NULL       if child is absent from the ancestor chain.
 */
static const FACEOE_Utl_Xml_Node_t* FindAncestor(const FACEOE_Utl_Xml_Node_t* parent, const FACEOE_Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Finds a top-level node of the requested type.
 *
 * \param[in] document   Document to search
 * \param[in] type       Node type to find
 *
 * \retval    non-NULL   matching top-level node.
 * \retval    NULL       if no top-level node has the requested type.
 */
static const FACEOE_Utl_Xml_Node_t* FindRootNode(const FACEOE_Utl_Xml_Document_t* document, FACEOE_Utl_Xml_NodeType_t type);

/*************************************************************************************************/
/**
 * \brief Reads an XML 1.0 token when it is present.
 *
 * \param[in]     bytes       UTF-8 bytes
 * \param[in]     byteCount   Number of bytes
 * \param[in,out] position    Offset of the next unread byte
 * \param[in]     token       Zero-terminated token of fewer than TOKEN_MAX_BYTES bytes
 *
 * \retval        TRUE        if the token was read.
 * \retval        FALSE       otherwise
 */
static bool_t ReadToken(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const char_t* token);

/*************************************************************************************************/
/**
 * \brief Reads optional XML 1.0 whitespace.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-S
 *
 * \param[in]     bytes          UTF-8 bytes
 * \param[in]     byteCount      Number of bytes
 * \param[in,out] position       Offset of the next unread byte
 * \param[in]     mustReachEnd   TRUE when only whitespace may remain
 *
 * \retval        TRUE           if position does not start beyond byteCount and mustReachEnd is satisfied.
 * \retval        FALSE          otherwise
 */
static bool_t ReadWhitespace(const byte_t* bytes, uint64_t byteCount, uint64_t* position, bool_t mustReachEnd);

/*************************************************************************************************/
/**
 * \brief Reads an XML 1.0 name.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Name
 *
 * \param[in]     bytes           UTF-8 bytes
 * \param[in]     byteCount       Number of bytes
 * \param[in,out] position        Offset of the next unread byte
 * \param[out]    name            Pointer to the name within bytes
 * \param[out]    nameByteCount   Number of name bytes
 *
 * \retval        TRUE            if the name is valid.
 * \retval        FALSE           otherwise
 */
static bool_t ReadName(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const byte_t** name, uint32_t* nameByteCount);

/*************************************************************************************************/
/**
 * \brief Reads = and a double-quoted XML 1.0 attribute value.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Eq
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-AttValue
 *
 * \param[in]     bytes       UTF-8 bytes
 * \param[in]     byteCount   Number of bytes
 * \param[in,out] position    Offset of the =
 * \param[out]    value       Pointer to the value within bytes, without quotes
 * \param[out]    numBytes    Number of value bytes
 *
 * \retval        TRUE        if the quoted value is valid.
 * \retval        FALSE       otherwise
 */
static bool_t ReadAttributeValue(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const byte_t** value, uint32_t* numBytes);

/*************************************************************************************************/
/**
 * \brief Reads, validates, and appends one attribute to an element.
 *
 * \param[in]     markup              Markup containing the attribute
 * \param[in]     markupByteCount     Number of markup bytes
 * \param[in,out] position            Offset of the attribute name
 * \param[in,out] node                Element receiving the attribute
 * \param[out]    attributes          Caller-owned attribute storage
 * \param[in]     attributeCapacity   Number of attributes in attributes
 * \param[in,out] attributeCount      Number of attributes used
 *
 * \retval        TRUE                if the attribute was appended.
 * \retval        FALSE               if invalid, duplicated, or storage is exhausted.
 */
static bool_t ReadAttribute(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity, uint32_t* attributeCount);

/*************************************************************************************************/
/**
 * \brief Reads an XML declaration with version="1.0" and initializes its node.
 *
 * \param[in]     markup           Markup beginning with <?xml
 * \param[in]     markupByteCount  Number of markup bytes
 * \param[in,out] position         Offset of the next unread byte
 * \param[out]    node             Node storage to initialize
 *
 * \retval        TRUE             if the declaration is supported.
 * \retval        FALSE            otherwise
 */
static bool_t ReadDeclaration(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Reads an element's start tag or empty-element tag with attributes.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-STag
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-EmptyElemTag
 *
 * \param[in]     markup              Markup that begins with <
 * \param[in]     markupByteCount     Number of markup bytes
 * \param[in,out] position            Offset of the next unread byte
 * \param[out]    node                Node storage to initialize
 * \param[out]    attributes          Caller-owned attribute storage
 * \param[in]     attributeCapacity   Number of attributes in attributes
 * \param[in,out] attributeCount      Number of attributes used
 * \param[out]    isStartTag          TRUE for a start tag, FALSE for an empty-element tag
 *
 * \retval        TRUE                if the element tag is supported.
 * \retval        FALSE               otherwise
 */
static bool_t ReadElementStart(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity, uint32_t* attributeCount, bool_t* isStartTag);

/*************************************************************************************************/
/**
 * \brief Reads an end tag and checks that its name matches the open element.
 *
 * \param[in]     markup           Markup beginning with </
 * \param[in]     markupByteCount  Number of markup bytes
 * \param[in,out] position         Offset of the next unread byte
 * \param[in]     node             Open element to match
 *
 * \retval        TRUE             if the end tag matches.
 * \retval        FALSE            otherwise
 */
static bool_t ReadElementEnd(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, const FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Reads the next markup span: the bytes from a < up to the next < or zero byte.
 *
 * \param[in,out] reader           Shared input position and markup count
 * \param[out]    markup           Pointer to the span's <
 * \param[out]    markupByteCount  Number of bytes in the span
 *
 * \retval        TRUE             if a span was read.
 * \retval        FALSE            if the markup limit is reached, the input is exhausted, or the next byte is not <.
 */
static bool_t ReadMarkup(XmlReader_t* reader, const byte_t** markup, uint64_t* markupByteCount);

/*************************************************************************************************/
/**
 * \brief Reads and appends one complete element tree using iterative traversal.
 *
 * \param[in,out] reader   Shared input position, document, and caller-owned storage
 *
 * \retval        TRUE     if the element and all descendants were read and closed.
 * \retval        FALSE    otherwise; the document may contain partial results.
 */
static bool_t ReadElement(XmlReader_t* reader);

/*************************************************************************************************/
/**
 * \brief Reads a top-level declaration or element, enforcing their placement.
 *
 * \param[in,out] reader   Shared input position, document, and caller-owned storage
 *
 * \retval        TRUE     if the top-level node was read.
 * \retval        FALSE    otherwise; the document may contain partial results.
 */
static bool_t ReadRootNode(XmlReader_t* reader);

/*************************************************************************************************/
/**
 * \brief Writes a zero-terminated XML token without its terminator.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     token       Zero-terminated token of fewer than TOKEN_MAX_BYTES bytes
 *
 * \retval        TRUE        if the token fits.
 * \retval        FALSE       otherwise
 */
static bool_t WriteToken(byte_t* output, uint64_t capacity, uint64_t* byteCount, const char_t* token);

/*************************************************************************************************/
/**
 * \brief Writes an XML 1.0 declaration.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-XMLDecl
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       XML declaration node to write
 *
 * \retval        TRUE       if the declaration is valid and fits.
 * \retval        FALSE      otherwise
 */
static bool_t WriteDeclaration(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element's start tag or empty-element tag with attributes.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-STag
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-EmptyElemTag
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Element to write
 *
 * \retval        TRUE       if the tag is valid and fits.
 * \retval        FALSE      otherwise
 */
static bool_t WriteElementStart(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element's closing tag.
 *
 * \par Details:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-ETag
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Element to close
 *
 * \retval        TRUE       if the tag fits.
 * \retval        FALSE      otherwise
 */
static bool_t WriteElementEnd(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element and its descendants without recursion.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     root       Root element to write
 *
 * \retval        TRUE       if the subtree is valid, within the node and depth limits, and fits.
 * \retval        FALSE      otherwise
 */
static bool_t WriteElement(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* root);

/*************************************************************************************************/
/**
 * \brief Writes one top-level XML 1.0 node, including an element's descendants.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Node to write
 *
 * \retval        TRUE       if the node is valid and fits.
 * \retval        FALSE      otherwise
 */
static bool_t WriteRootNode(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Appends a byte span to the caller's output buffer.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     bytes       Bytes to append
 * \param[in]     numBytes    Nonzero number of bytes to append
 *
 * \retval        TRUE        if the bytes fit.
 * \retval        FALSE       otherwise
 */
static bool_t WriteBytes(byte_t* output, uint64_t capacity, uint64_t* byteCount, const byte_t* bytes, uint32_t numBytes);

/*************************************************************************************************/
/**
 * \brief Appends one byte to the caller's output buffer.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     value       Byte to append
 *
 * \retval        TRUE        if the byte fits.
 * \retval        FALSE       otherwise
 */
static bool_t WriteByte(byte_t* output, uint64_t capacity, uint64_t* byteCount, byte_t value);

/*************************************************************************************************/
/**
 * \brief Writes the requested number of indentation spaces.
 *
 * \param[in,out] output             Caller-provided byte storage
 * \param[in]     capacity           Number of bytes in output
 * \param[in,out] byteCount          Number of bytes already written
 * \param[in]     indentSpaceCount   Number of spaces to write
 *
 * \retval        TRUE               if the indentation fits.
 * \retval        FALSE              otherwise
 */
static bool_t WriteIndent(byte_t* output, uint64_t capacity, uint64_t* byteCount, uint32_t indentSpaceCount);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Also classifies UTF-8 leading bytes.
 */
static bool_t InRange(uint32_t codePoint, uint32_t first, uint32_t last)
{
    bool_t is_first_or_greater;
    bool_t is_last_or_less;

    is_first_or_greater = (first <= codePoint);
    is_last_or_less     = (codePoint <= last);

    return is_first_or_greater && is_last_or_less;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Decodes one code point at offset. For example, the three-byte EURO SIGN
 *  between the ASCII letters A and B is decoded as follows:
 *
 * \code{.txt}
 * Byte index:       0  1  2  3  4
 * Input bytes:     41 E2 82 AC 42   (hexadecimal)
 * Offset:              1
 *
 * E2 = 1110 0010  -> payload 0010
 * 82 = 10 000010  -> payload 000010
 * AC = 10 101100  -> payload 101100
 * Combined payload: 0010 000010 101100 = U+20AC
 *
 * Return value: 0x20AC; decodedByteCount: 3
 * The caller advances from offset 1 to offset 4 to decode B next.
 * \endcode
 *
 *  Rejects invalid leading or continuation bytes, truncated sequences, overlong
 *  forms, surrogates, and code points above U+10FFFF. Failure leaves
 *  decodedByteCount at zero. A valid U+0000 also returns zero, but consumes one
 *  byte; callers use decodedByteCount to distinguish it from failure.
 */
static uint32_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t offset, uint32_t* decodedByteCount)
{
    byte_t   first;
    uint32_t decoded;
    uint32_t code_point;
    uint32_t minimum;
    uint32_t num_continuations;
    uint32_t index;
    uint32_t position;
    bool_t   is_position_valid;
    bool_t   is_continuation_byte;
    bool_t   is_code_point_in_range;
    bool_t   is_surrogate;
    bool_t   is_valid;

    // Keep the failure result until the complete sequence has passed validation
    code_point = 0;
    (*decodedByteCount) = 0;
    // Check the starting offset before accessing the leading byte
    is_valid = (offset < numBytes);

    if (is_valid)
    {
        position = offset;
        first    = bytes[position];

        // The leading byte selects the payload bits, sequence length, and minimum value
        if (first <= UTF8_ASCII_MAX_BYTE)
        {
            // ASCII, including U+0000, needs no continuation bytes
            decoded           = first;
            minimum           = 0;
            num_continuations = 0;
        }
        else if (InRange(first, UTF8_TWO_BYTE_LEAD_MIN, UTF8_TWO_BYTE_LEAD_MAX))
        {
            decoded           = first & UTF8_TWO_BYTE_LEAD_PAYLOAD_MASK;
            minimum           = UTF8_TWO_BYTE_MIN_CODE_POINT;
            num_continuations = 1;
        }
        else if (InRange(first, UTF8_THREE_BYTE_LEAD_MIN, UTF8_THREE_BYTE_LEAD_MAX))
        {
            decoded           = first & UTF8_THREE_BYTE_LEAD_PAYLOAD_MASK;
            minimum           = UTF8_THREE_BYTE_MIN_CODE_POINT;
            num_continuations = 2;
        }
        else if (InRange(first, UTF8_FOUR_BYTE_LEAD_MIN, UTF8_FOUR_BYTE_LEAD_MAX))
        {
            decoded           = first & UTF8_FOUR_BYTE_LEAD_PAYLOAD_MASK;
            minimum           = UTF8_FOUR_BYTE_MIN_CODE_POINT;
            num_continuations = 3;
        }
        else
        {
            // Reject continuation bytes, overlong leads C0 and C1, and leads F5 through FF
            is_valid = FALSE;
        }

        if (is_valid)
        {
            for (index = 0; index < num_continuations; ++index)
            {
                ++position;

                // Reject a truncated sequence before accessing the next byte
                is_position_valid = (position < numBytes);

                is_valid = is_position_valid;

                if (is_valid)
                {
                    // Each continuation byte must begin with the binary prefix 10
                    is_continuation_byte = (UTF8_CONTINUATION_PREFIX == (bytes[position] & UTF8_CONTINUATION_PREFIX_MASK));
                    is_valid = is_continuation_byte;
                }

                if (FALSE == is_valid)
                {
                    break;
                }

                // Append the continuation byte's six payload bits to the accumulated value
                decoded = (decoded << UTF8_CONTINUATION_PAYLOAD_BITS) | (bytes[position] & UTF8_CONTINUATION_PAYLOAD_MASK);
            }
        }

        if (is_valid)
        {
            // Enforce the shortest encoding for this length and the Unicode upper bound
            is_code_point_in_range = InRange(decoded, minimum, UTF8_MAX_CODE_POINT);
            is_valid = is_code_point_in_range;
        }

        if (is_valid)
        {
            // Surrogate code points are not Unicode scalar values and cannot appear in UTF-8
            is_surrogate = InRange(decoded, UTF8_SURROGATE_MIN_CODE_POINT, UTF8_SURROGATE_MAX_CODE_POINT);
            is_valid = (FALSE == is_surrogate);
        }

        if (is_valid)
        {
            // Publish the code point and consumed byte count only after every check succeeds
            code_point = decoded;
            (*decodedByteCount) = num_continuations + 1;
        }
    }

    return code_point;
}


/*************************************************************************************************/
/**
 * \par Details:
 *  Excludes surrogates, U+FFFE, U+FFFF, and C0 controls other than tab, LF, and CR.
 */
static bool_t IsChar(uint32_t codePoint)
{
    bool_t is_tab                   = (CHAR_TAB == codePoint);
    bool_t is_line_feed             = (CHAR_LF == codePoint);
    bool_t is_carriage_return       = (CHAR_CR == codePoint);
    bool_t is_bmp_before_surrogates = InRange(codePoint, 0x20, 0xD7FF);
    bool_t is_bmp_after_surrogates  = InRange(codePoint, 0xE000, 0xFFFD);
    bool_t is_supplementary         = InRange(codePoint, 0x10000, 0x10FFFF);

    return is_tab || is_line_feed || is_carriage_return || is_bmp_before_surrogates || is_bmp_after_surrogates || is_supplementary;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  An empty span is valid, since attribute values may be empty.
 */
static bool_t IsString(const byte_t* utf8Text, uint32_t byteCount)
{
    bool_t   is_xml_string;
    uint32_t byte_offset;
    uint32_t code_point;
    uint32_t decoded_byte_count;
    bool_t   decode_failed;
    bool_t   is_invalid_code_point;

    LOGASSERT(NULL != utf8Text);
    LOGASSERT(FACEOE_UTL_XML_MAX_VALUE_BYTES >= byteCount);

    is_xml_string = TRUE;

    for (byte_offset = 0; byte_offset < byteCount; byte_offset += decoded_byte_count)
    {
        code_point = DecodeUtf8(utf8Text, byteCount, byte_offset, &decoded_byte_count);

        decode_failed         = (0 == decoded_byte_count);
        is_invalid_code_point = (FALSE == IsChar(code_point));

        if (decode_failed || is_invalid_code_point)
        {
            is_xml_string = FALSE;
            break;
        }
    }

    return is_xml_string;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Other Unicode spaces, such as U+00A0, are not XML whitespace.
 */
static bool_t IsWhitespace(uint32_t codePoint)
{
    bool_t is_space           = (CHAR_SPACE == codePoint);
    bool_t is_tab             = (CHAR_TAB == codePoint);
    bool_t is_carriage_return = (CHAR_CR == codePoint);
    bool_t is_line_feed       = (CHAR_LF == codePoint);

    return is_space || is_tab || is_carriage_return || is_line_feed;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Searches c_xmlNameStartCharRanges.
 */
static bool_t IsNameStartChar(uint32_t codePoint)
{
    bool_t   is_name_start_char;
    uint32_t range_index;
    uint32_t first;
    uint32_t last;

    is_name_start_char = FALSE;

    for (range_index = 0; range_index < COUNT_OF(c_xmlNameStartCharRanges); ++range_index)
    {
        first = c_xmlNameStartCharRanges[range_index].first;
        last  = c_xmlNameStartCharRanges[range_index].last;

        is_name_start_char = InRange(codePoint, first, last);

        if (is_name_start_char)
        {
            break;
        }
    }

    return is_name_start_char;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts any NameStartChar, then searches c_xmlNameCharRanges.
 */
static bool_t IsNameChar(uint32_t codePoint)
{
    bool_t   is_name_char;
    uint32_t range_index;
    uint32_t first;
    uint32_t last;

    is_name_char = FALSE;

    if (IsNameStartChar(codePoint))
    {
        is_name_char = TRUE;
    }
    else
    {
        for (range_index = 0; range_index < COUNT_OF(c_xmlNameCharRanges); ++range_index)
        {
            first = c_xmlNameCharRanges[range_index].first;
            last  = c_xmlNameCharRanges[range_index].last;

            is_name_char = InRange(codePoint, first, last);

            if (is_name_char)
            {
                break;
            }
        }
    }

    return is_name_char;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses XML 1.0 NameStartChar and NameChar after strict UTF-8 decoding.
 */
static bool_t IsName(const byte_t* utf8Text, uint32_t byteCount)
{
    bool_t   is_name;
    uint32_t byte_offset;
    uint32_t code_point;
    uint32_t decoded_byte_count;
    bool_t   decode_failed;

    LOGASSERT(NULL != utf8Text);
    LOGASSERT(0 < byteCount);
    LOGASSERT(FACEOE_UTL_XML_MAX_NAME_BYTES >= byteCount);

    is_name = TRUE;

    for (byte_offset = 0; byte_offset < byteCount; byte_offset += decoded_byte_count)
    {
        code_point = DecodeUtf8(utf8Text, byteCount, byte_offset, &decoded_byte_count);

        decode_failed = (0 == decoded_byte_count);

        if (decode_failed)
        {
            is_name = FALSE;
        }
        else
        {
            if (0 == byte_offset)
            {
                is_name = IsNameStartChar(code_point);
            }
            else
            {
                is_name = IsNameChar(code_point);
            }
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
 *  Implements the [^<&"] branch of AttValue, limited to Char.
 */
static bool_t IsAttributeValueChar(uint32_t codePoint)
{
    bool_t is_xml_char;
    bool_t is_less_than;
    bool_t is_ampersand;
    bool_t is_double_quote;
    bool_t is_reserved;
    bool_t is_attribute_value_char;

    is_xml_char              = IsChar(codePoint);
    is_less_than             = (CHAR_LT == codePoint);
    is_ampersand             = (CHAR_AMP == codePoint);
    is_double_quote          = (QUOTE_QUOT == codePoint);
    is_reserved              = is_less_than || is_ampersand || is_double_quote;
    is_attribute_value_char  = is_xml_char && (FALSE == is_reserved);

    return is_attribute_value_char;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Compares bytes only when the names have the same byte count. Asserts that the
 *  list holds exactly attributeCount attributes when the name is absent.
 */
static const FACEOE_Utl_Xml_Attribute_t* FindAttribute(const FACEOE_Utl_Xml_Node_t* node, const FACEOE_Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount)
{
    const FACEOE_Utl_Xml_Attribute_t* current_attribute;
    const FACEOE_Utl_Xml_Attribute_t* found_attribute;
    uint32_t                          attribute_index;
    bool_t                            has_same_name_byte_count;
    bool_t                            has_same_name;

    current_attribute = node->firstAttribute;
    found_attribute   = NULL;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != current_attribute);
        LOGASSERT(attribute != current_attribute);

        has_same_name_byte_count = (nameByteCount == current_attribute->nameByteCount);

        if (has_same_name_byte_count)
        {
            has_same_name = (0 == memcmp(current_attribute->name, name, nameByteCount));

            if (has_same_name)
            {
                found_attribute = current_attribute;
                break;
            }
        }

        current_attribute = current_attribute->next;
    }

    LOGASSERT((NULL != found_attribute) || (NULL == current_attribute));

    return found_attribute;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Asserts if the chain from parent holds more than FACEOE_UTL_XML_NODE_MAX_DEPTH nodes without child.
 */
static const FACEOE_Utl_Xml_Node_t* FindAncestor(const FACEOE_Utl_Xml_Node_t* parent, const FACEOE_Utl_Xml_Node_t* child)
{
    const FACEOE_Utl_Xml_Node_t* current_node;
    const FACEOE_Utl_Xml_Node_t* ancestor;
    bool_t                       is_ancestor;
    uint32_t                     node_depth;

    current_node = parent;
    ancestor     = NULL;

    for (node_depth = 0; node_depth < FACEOE_UTL_XML_NODE_MAX_DEPTH; ++node_depth)
    {
        if (NULL == current_node)
        {
            break;
        }

        is_ancestor = (child == current_node);

        if (is_ancestor)
        {
            ancestor = current_node;
            break;
        }

        current_node = current_node->parent;
    }

    LOGASSERT((NULL != ancestor) || (NULL == current_node));

    return ancestor;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Asserts if the type is absent and more than FACEOE_UTL_XML_DOCUMENT_MAX_ROOT_NODES nodes are linked.
 */
static const FACEOE_Utl_Xml_Node_t* FindRootNode(const FACEOE_Utl_Xml_Document_t* document, FACEOE_Utl_Xml_NodeType_t type)
{
    const FACEOE_Utl_Xml_Node_t* current_node;
    const FACEOE_Utl_Xml_Node_t* found_node;
    uint32_t                     node_index;
    bool_t                       has_node;

    found_node   = NULL;
    current_node = document->firstNode;

    for (node_index = 0; node_index < FACEOE_UTL_XML_DOCUMENT_MAX_ROOT_NODES; ++node_index)
    {
        has_node = (NULL != current_node);

        if (FALSE == has_node)
        {
            break;
        }

        if (type == current_node->type)
        {
            found_node = current_node;

            break;
        }

        current_node = current_node->nextSibling;
    }

    if (NULL == found_node)
    {
        LOGASSERT(NULL == current_node);
    }

    return found_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Matches a fixed XML token at the current position. For example, reading the
 *  empty-element closing delimiter after the name in <panel/>:
 *
 * \verbatim
 * Byte index:   0 1 2 3 4 5 6 7
 * Input bytes:  < p a n e l / >
 * byteCount:    8
 * token:        "/>"
 * position:     6 -> 8
 * Return:       TRUE
 * \endverbatim
 *
 *  With <panel> instead, the same token fails at position 6 and leaves position
 *  unchanged, so the caller can try ">" at that offset.
 *
 *  The token's zero terminator must occur within TOKEN_MAX_BYTES and is not matched.
 *  Input is bounded by byteCount and needs no terminator. A mismatch, short input,
 *  or missing token terminator returns FALSE with position unchanged, even after a
 *  partial match.
 */
static bool_t ReadToken(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const char_t* token)
{
    uint32_t byte_index;
    byte_t   token_byte;
    bool_t   is_position_valid;
    bool_t   is_token_byte_available;
    bool_t   is_matching_byte;
    bool_t   is_valid;

    is_valid = TRUE;

    for (byte_index = 0; byte_index < TOKEN_MAX_BYTES; ++byte_index)
    {
        // The terminator ends the token and is not part of the input to match
        if ('\0' == token[byte_index])
        {
            break;
        }

        token_byte = token[byte_index];

        // Validate the starting offset before subtracting it from the input size
        is_position_valid = ((*position) < byteCount);

        is_valid = is_position_valid;

        if (is_valid)
        {
            // Check the remaining input before forming the byte's absolute offset
            is_token_byte_available = (byte_index < (byteCount - (*position)));

            is_valid = is_token_byte_available;
        }
        if (is_valid)
        {
            // Compare the token literally, without skipping whitespace or changing case
            is_matching_byte = (token_byte == bytes[(*position) + byte_index]);

            is_valid = is_matching_byte;
        }

        if (FALSE == is_valid)
        {
            // Stop at the first unavailable or mismatched byte
            break;
        }
    }

    if (is_valid)
    {
        // Exhausting the token bound without seeing a terminator is a failure
        is_valid = (byte_index < TOKEN_MAX_BYTES);
    }
    if (is_valid)
    {
        // Consume the token only after every byte and its terminator have been checked
        (*position) += byte_index;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Skips XML whitespace (space, tab, carriage return, and line feed) from the
 *  current position, stopping at the first other byte or the end of the input.
 *  For example, skipping a line feed and indentation before an element:
 *
 * \verbatim
 * Input bytes:       "\n    <button/>"  (\n represents one line-feed byte)
 * byteCount:         14
 * Initial position:  0
 *
 * mustReachEnd   Return   Final position
 * FALSE          TRUE     5 (the '<' before button)
 * TRUE           FALSE    5 (the '<' before button)
 * \endverbatim
 *
 *  With mustReachEnd set, all remaining bytes must be whitespace. For example,
 *  "\r\n  " advances position from 0 to 4 and returns TRUE in either mode.
 *  Position still advances past any whitespace when mustReachEnd rejects a
 *  following non-whitespace byte.
 *
 *  Whitespace is optional: starting at byteCount succeeds in either mode.
 *  Starting beyond byteCount returns FALSE without changing position.
 */
static bool_t ReadWhitespace(const byte_t* bytes, uint64_t byteCount, uint64_t* position, bool_t mustReachEnd)
{
    bool_t   is_valid;
    uint64_t byte_index;

    // Allow an empty remaining span, but reject an offset beyond the input
    is_valid = ((*position) <= byteCount);

    if (is_valid)
    {
        for (byte_index = (*position); byte_index < byteCount; ++byte_index)
        {
            // Leave the first non-whitespace byte for the caller to read
            if (FALSE == IsWhitespace(bytes[byte_index]))
            {
                break;
            }
        }

        // Consume the whitespace even if the trailing-content check below fails
        (*position) = byte_index;

        if (mustReachEnd)
        {
            // Reject any remaining content when only trailing whitespace is allowed
            is_valid = (byte_index == byteCount);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads an element or attribute name at the current position. For example,
 *  the caller can read both names in this empty-element tag:
 *
 * \code{.xml}
 * <button id="start"/>
 * \endcode
 *
 * \verbatim
 * byteCount: 20
 *
 * Initial position   Name bytes   nameByteCount   Final position   Return
 * 1                  button       6               7 (space)        TRUE
 * 8                  id           2              10 ('=')          TRUE
 * \endverbatim
 *
 *  These are separate calls after the caller consumes the preceding delimiter
 *  or whitespace. The scan ends at XML whitespace, =, /, >, or byteCount and
 *  leaves any delimiter unread. Leading whitespace is not skipped.
 *
 *  The name must be nonempty, fit within FACEOE_UTL_XML_MAX_NAME_BYTES, and be a
 *  valid UTF-8 XML Name. On success, name points into bytes; nothing is copied.
 *  On failure, position, name, and nameByteCount are unchanged.
 */
static bool_t ReadName(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const byte_t** name, uint32_t* nameByteCount)
{
    const byte_t* parsed_name;
    uint64_t      byte_index;
    uint64_t      span_byte_count;
    bool_t        is_whitespace;
    bool_t        is_equals;
    bool_t        is_slash;
    bool_t        is_tag_close;
    bool_t        is_name_end;
    bool_t        has_name_bytes;
    bool_t        is_within_name_limit;
    bool_t        is_valid;

    // Retain the start of the name so the result can reference the original input
    parsed_name = bytes + (*position);

    // Find the candidate span before validating its UTF-8 code points
    for (byte_index = (*position); byte_index < byteCount; ++byte_index)
    {
        is_whitespace = IsWhitespace(bytes[byte_index]);
        is_equals     = (EQ == bytes[byte_index]);
        is_slash      = ('/' == bytes[byte_index]);
        is_tag_close  = (CHAR_GT == bytes[byte_index]);
        is_name_end   = is_whitespace || is_equals || is_slash || is_tag_close;

        if (is_name_end)
        {
            // Leave the delimiter for the caller to process after the name
            break;
        }
    }

    // Reject empty or oversized spans before narrowing the length for validation
    span_byte_count      = byte_index - (*position);
    has_name_bytes       = (0 < span_byte_count);
    is_within_name_limit = (span_byte_count <= FACEOE_UTL_XML_MAX_NAME_BYTES);

    is_valid = has_name_bytes && is_within_name_limit;

    if (is_valid)
    {
        // Validate the first code point as a name start and the rest as name characters
        is_valid = IsName(parsed_name, (uint32_t)span_byte_count);
    }
    if (is_valid)
    {
        // Publish the name and advance only after the complete span is valid
        (*nameByteCount) = (uint32_t)span_byte_count;
        (*position)      = byte_index;
        (*name)          = parsed_name;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Whitespace around = is rejected. The value's characters are not checked;
 *  ReadAttribute checks them before appending.
 */
static bool_t ReadAttributeValue(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const byte_t** value, uint32_t* numBytes)
{
    const byte_t* parsed_value;
    uint64_t      byte_index;
    uint64_t      span_byte_count;
    bool_t        is_position_valid;
    bool_t        has_delimiter_bytes;
    bool_t        is_equals;
    bool_t        is_open_quote;
    bool_t        has_closing_quote;
    bool_t        is_within_value_limit;
    bool_t        is_valid;
    bool_t        is_value_close;

    is_position_valid = ((*position) < byteCount);

    is_valid = is_position_valid;

    if (is_valid)
    {
        has_delimiter_bytes = ((byteCount - (*position)) >= 2);

        is_valid = has_delimiter_bytes;
    }
    if (is_valid)
    {
        is_equals = (EQ == bytes[(*position)]);

        is_valid = is_equals;
    }
    if (is_valid)
    {
        is_open_quote = (QUOTE_QUOT == bytes[(*position) + 1]);

        is_valid = is_open_quote;
    }
    if (is_valid)
    {
        (*position) += 2;

        parsed_value = bytes + (*position);

        for (byte_index = (*position); byte_index < byteCount; ++byte_index)
        {
            is_value_close = (QUOTE_QUOT == bytes[byte_index]);

            if (is_value_close)
            {
                break;
            }
        }

        span_byte_count   = byte_index - (*position);
        has_closing_quote = (byte_index < byteCount);

        is_valid = has_closing_quote;

        if (is_valid)
        {
            is_within_value_limit = (span_byte_count <= FACEOE_UTL_XML_MAX_VALUE_BYTES);

            is_valid = is_within_value_limit;
        }
        if (is_valid)
        {
            (*numBytes) = (uint32_t)span_byte_count;
            (*position) = byte_index + 1;
            (*value)    = parsed_value;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the value and uniqueness first, so invalid input returns FALSE instead of
 *  failing an assertion in FACEOE_Utl_Xml_AppendAttribute.
 */
static bool_t ReadAttribute(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity, uint32_t* attributeCount)
{
    FACEOE_Utl_Xml_Attribute_t* attribute;
    const byte_t*               name;
    const byte_t*               value;
    uint32_t                    name_byte_count;
    uint32_t                    value_byte_count;
    bool_t                      is_valid;

    is_valid = ((*attributeCount) < attributeCapacity);
    if (is_valid)
    {
        is_valid = ReadName(markup, markupByteCount, position, &name, &name_byte_count);
    }
    if (is_valid)
    {
        is_valid = ReadAttributeValue(markup, markupByteCount, position, &value, &value_byte_count);
    }
    if (is_valid)
    {
        is_valid = FACEOE_Utl_Xml_IsAttributeValue(value, value_byte_count);
    }
    if (is_valid)
    {
        is_valid = (NULL == FindAttribute(node, NULL, name, name_byte_count));
    }
    if (is_valid)
    {
        attribute = &attributes[(*attributeCount)];
        FACEOE_Utl_Xml_InitializeAttribute(attribute, name, name_byte_count, value, value_byte_count);
        FACEOE_Utl_Xml_AppendAttribute(node, attribute);
        (*attributeCount) += 1;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts only <?xml, whitespace, then version="1.0"?>. ReadRootNode checks placement.
 */
static bool_t ReadDeclaration(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, FACEOE_Utl_Xml_Node_t* node)
{
    const byte_t* version;
    uint32_t      version_byte_count;
    uint64_t      whitespace_start;
    bool_t        is_valid;

    is_valid = ReadToken(markup, markupByteCount, position, DECL_OPEN);
    if (is_valid)
    {
        whitespace_start = (*position);
        is_valid = ReadWhitespace(markup, markupByteCount, position, FALSE);
    }
    if (is_valid)
    {
        is_valid = (whitespace_start < (*position));
    }
    if (is_valid)
    {
        is_valid = ReadToken(markup, markupByteCount, position, VERSION_NAME);
    }
    if (is_valid)
    {
        is_valid = ReadAttributeValue(markup, markupByteCount, position, &version, &version_byte_count);
    }
    if (is_valid)
    {
        is_valid = (VERSION_1_0_BYTE_COUNT == version_byte_count);
    }
    if (is_valid)
    {
        is_valid = (0 == memcmp(version, VERSION_1_0, version_byte_count));
    }
    if (is_valid)
    {
        is_valid = ReadToken(markup, markupByteCount, position, DECL_CLOSE);
    }
    if (is_valid)
    {
        FACEOE_Utl_Xml_InitializeNode(node, FACEOE_UTL_XML_NODE_XML_DECLARATION, NULL, 0);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads attributes in source order. Whitespace is accepted only before each
 *  attribute, not before > or />.
 */
static bool_t ReadElementStart(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity, uint32_t* attributeCount, bool_t* isStartTag)
{
    const byte_t*        name;
    uint32_t             name_byte_count;
    uint32_t             attribute_index;
    uint64_t             whitespace_start;
    bool_t               has_whitespace;
    bool_t               is_empty_elem_tag_close;
    bool_t               is_tag_close;
    bool_t               is_start_tag;
    bool_t               is_valid;

    is_valid = ReadToken(markup, markupByteCount, position, STAG_OPEN);
    if (is_valid)
    {
        is_valid = ReadName(markup, markupByteCount, position, &name, &name_byte_count);
    }
    if (is_valid)
    {
        FACEOE_Utl_Xml_InitializeNode(node, FACEOE_UTL_XML_NODE_ELEMENT, name, name_byte_count);
    }

    for (attribute_index = 0; attribute_index < FACEOE_UTL_XML_NODE_MAX_ATTRIBUTES; ++attribute_index)
    {
        if (FALSE == is_valid)
        {
            break;
        }

        whitespace_start = (*position);

        is_valid = ReadWhitespace(markup, markupByteCount, position, FALSE);

        if (FALSE == is_valid)
        {
            break;
        }

        has_whitespace = (whitespace_start < (*position));

        if (FALSE == has_whitespace)
        {
            break;
        }

        is_valid = ReadAttribute(markup, markupByteCount, position, node, attributes, attributeCapacity, attributeCount);

        if (FALSE == is_valid)
        {
            break;
        }
    }

    if (is_valid)
    {
        is_empty_elem_tag_close = ReadToken(markup, markupByteCount, position, EMPTY_ELEM_TAG_CLOSE);

        is_start_tag = (FALSE == is_empty_elem_tag_close);

        if (is_start_tag)
        {
            is_tag_close = ReadToken(markup, markupByteCount, position, TAG_CLOSE);

            is_valid = is_tag_close;
        }
        if (is_valid)
        {
            (*isStartTag) = is_start_tag;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts whitespace before >, as ETag allows. The caller moves to the parent
 *  element only after the end tag matches.
 */
static bool_t ReadElementEnd(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, const FACEOE_Utl_Xml_Node_t* node)
{
    const byte_t* name;
    uint32_t      name_byte_count;
    bool_t        is_valid;

    is_valid = ReadToken(markup, markupByteCount, position, ETAG_OPEN);
    if (is_valid)
    {
        is_valid = ReadName(markup, markupByteCount, position, &name, &name_byte_count);
    }
    if (is_valid)
    {
        is_valid = (node->nameByteCount == name_byte_count);
    }
    if (is_valid)
    {
        is_valid = (0 == memcmp(node->name, name, name_byte_count));
    }
    if (is_valid)
    {
        is_valid = ReadWhitespace(markup, markupByteCount, position, FALSE);
    }
    if (is_valid)
    {
        is_valid = ReadToken(markup, markupByteCount, position, TAG_CLOSE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  FACEOE_Utl_StrStr finds the next <, which is replaced by a zero byte while strnlen
 *  measures the span and restored before returning.
 */
static bool_t ReadMarkup(XmlReader_t* reader, const byte_t** markup, uint64_t* markupByteCount)
{
    byte_t*  document_cursor;
    byte_t*  next_markup;
    size_t   remaining_byte_count;
    uint64_t markup_byte_count;
    bool_t   is_valid;

    is_valid = (reader->markupCount < FACEOE_UTL_XML_DOCUMENT_MAX_NODES);
    if (is_valid)
    {
        is_valid = (reader->position < reader->byteCount);
    }
    if (is_valid)
    {
        document_cursor = reader->bytes + reader->position;
        is_valid = (CHAR_LT == document_cursor[0]);
    }
    if (is_valid)
    {
        next_markup = (byte_t*)FACEOE_Utl_StrStr((const char_t*)(document_cursor + 1), STAG_OPEN, FALSE);
        if (NULL != next_markup)
        {
            next_markup[0] = '\0';
        }

        remaining_byte_count = (size_t)(reader->byteCount - reader->position);
        markup_byte_count = (uint64_t)strnlen((const char_t*)document_cursor, remaining_byte_count);

        if (NULL != next_markup)
        {
            next_markup[0] = CHAR_LT;
        }

        (*markup) = document_cursor;
        (*markupByteCount) = markup_byte_count;
        reader->position += markup_byte_count;
        reader->markupCount += 1;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads one root element and its descendants without recursion. For example,
 *  this subtree combines nested start/end tags with empty-element tags:
 *
 * \code{.xml}
 * <menu>
 *     <group>
 *         <item name="open"/>
 *     </group>
 *     <separator/>
 * </menu>
 * \endcode
 *
 * \verbatim
 * Markup                 Parent after reading
 * <menu>                 menu
 * <group>                group
 * <item name="open"/>    group
 * </group>               menu
 * <separator/>           menu
 * </menu>                NULL (root complete)
 * \endverbatim
 *
 *  The first element is appended to the document; later elements are appended
 *  to the current parent. A start tag makes its node the new parent. An empty
 *  element leaves the parent unchanged, while a matching end tag moves back to
 *  the enclosing parent. An empty root completes the traversal immediately.
 *
 *  Each markup span includes the bytes after its tag up to the next markup or
 *  end of input. Only XML whitespace is accepted there. Once the root is closed,
 *  any following markup is left for the document reader to handle.
 *
 *  Node storage, attribute storage, and markup counts are shared across the
 *  document. This example uses four nodes, one attribute, and six markup spans;
 *  closing tags count toward FACEOE_UTL_XML_DOCUMENT_MAX_NODES but need no node storage.
 *  Invalid input, exhausted storage, or reaching the markup limit with an open
 *  element returns FALSE. Input position, counts, and partial results are not
 *  rolled back on failure.
 */
static bool_t ReadElement(XmlReader_t* reader)
{
    FACEOE_Utl_Xml_Node_t* node;
    FACEOE_Utl_Xml_Node_t* parent;
    const byte_t*          markup;
    uint64_t               markup_byte_count;
    uint64_t               position;
    uint64_t               token_position;
    uint32_t               markup_index;
    bool_t                 is_end_tag;
    bool_t                 is_start_tag;
    bool_t                 is_valid;

    // No element is open yet; reading no markup must leave the result invalid
    parent   = NULL;
    is_valid = FALSE;

    // Use the remaining document-wide markup budget, including closing tags
    for (markup_index = reader->markupCount; markup_index < FACEOE_UTL_XML_DOCUMENT_MAX_NODES; ++markup_index)
    {
        // Obtain the next markup span and advance the shared input position
        is_valid = ReadMarkup(reader, &markup, &markup_byte_count);
        if (FALSE == is_valid)
        {
            break;
        }

        // Probe with a separate offset so the tag reader still starts at byte zero
        position       = 0;
        token_position = 0;
        is_end_tag = ReadToken(markup, markup_byte_count, &token_position, ETAG_OPEN);

        if (is_end_tag)
        {
            // A closing tag requires an open element and must match its name
            is_valid = (NULL != parent);
            if (is_valid)
            {
                is_valid = ReadElementEnd(markup, markup_byte_count, &position, parent);
            }
            if (is_valid)
            {
                // The enclosing element becomes the parent for subsequent siblings
                parent = parent->parent;
            }
        }
        else
        {
            // Each start tag or empty-element tag needs one caller-provided node
            is_valid = (reader->nodeCount < reader->nodeCapacity);
            if (is_valid)
            {
                node = &reader->nodes[reader->nodeCount];
                is_valid = ReadElementStart(markup, markup_byte_count, &position, node, reader->attributes, reader->attributeCapacity, &reader->attributeCount, &is_start_tag);
            }
            if (is_valid)
            {
                if (NULL == parent)
                {
                    // The first element belongs to the document's top-level list
                    FACEOE_Utl_Xml_AppendNode(reader->document, node);
                }
                else
                {
                    // Preserve source order within the open parent's child list
                    FACEOE_Utl_Xml_AppendChild(parent, node);
                }
                reader->nodeCount += 1;

                if (is_start_tag)
                {
                    // Descend only when this element still needs a matching end tag
                    parent = node;
                }
            }
        }

        if (is_valid)
        {
            // Reject text or other content after the tag; only whitespace may remain
            is_valid = ReadWhitespace(markup, markup_byte_count, &position, TRUE);
        }
        // Stop on a parsing failure or after the root has been completely closed
        if ((FALSE == is_valid) || (NULL == parent))
        {
            break;
        }
    }

    if (is_valid)
    {
        // Exhausting the markup budget while an element is still open is a failure
        is_valid = (NULL == parent);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Declarations must begin at byte zero in an empty document. Element descendants
 *  are consumed by ReadElement, so only top-level placement is checked here.
 */
static bool_t ReadRootNode(XmlReader_t* reader)
{
    FACEOE_Utl_Xml_Node_t* node;
    const byte_t*          markup;
    uint64_t               markup_byte_count;
    uint64_t               position;
    bool_t                 is_declaration;
    bool_t                 is_first_markup;
    bool_t                 is_empty_document;
    bool_t                 is_valid;

    position = reader->position;
    is_declaration = ReadToken(reader->bytes, reader->byteCount, &position, DECL_OPEN);

    if (is_declaration)
    {
        is_first_markup   = (0 == reader->position);
        is_empty_document = (NULL == reader->document->firstNode);
        is_valid = is_first_markup && is_empty_document;
        if (is_valid)
        {
            is_valid = (reader->nodeCount < reader->nodeCapacity);
        }
        if (is_valid)
        {
            is_valid = ReadMarkup(reader, &markup, &markup_byte_count);
        }
        if (is_valid)
        {
            position = 0;
            node = &reader->nodes[reader->nodeCount];
            is_valid = ReadDeclaration(markup, markup_byte_count, &position, node);
        }
        if (is_valid)
        {
            FACEOE_Utl_Xml_AppendNode(reader->document, node);
            reader->nodeCount += 1;
            is_valid = ReadWhitespace(markup, markup_byte_count, &position, TRUE);
        }
    }
    else
    {
        is_valid = (NULL == FindRootNode(reader->document, FACEOE_UTL_XML_NODE_ELEMENT));
        if (is_valid)
        {
            is_valid = ReadElement(reader);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes bytes up to the token's zero terminator. Returns FALSE if the terminator
 *  is not found within TOKEN_MAX_BYTES or the output buffer runs out of space.
 */
static bool_t WriteToken(byte_t* output, uint64_t capacity, uint64_t* byteCount, const char_t* token)
{
    uint32_t byte_index;
    bool_t   is_valid;

    LOGASSERT(NULL != token);

    is_valid = TRUE;
    for (byte_index = 0; byte_index < TOKEN_MAX_BYTES; ++byte_index)
    {
        // The terminator ends the token and must not be written to the output
        if ('\0' == token[byte_index])
        {
            break;
        }

        is_valid = WriteByte(output, capacity, byteCount, token[byte_index]);
        if (FALSE == is_valid)
        {
            break;
        }
    }

    if (is_valid)
    {
        // Exhausting the token bound without seeing a terminator is a failure
        is_valid = (byte_index < TOKEN_MAX_BYTES);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the fixed XML 1.0 declaration:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * \endcode
 *
 *  Returns FALSE if the node has a name or a nonzero name byte count.
 *  FACEOE_Utl_Xml_WriteDocument appends the line feed after this declaration.
 */
static bool_t WriteDeclaration(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node)
{
    bool_t has_no_name;
    bool_t has_no_name_bytes;
    bool_t is_valid;

    has_no_name       = (NULL == node->name);
    has_no_name_bytes = (0 == node->nameByteCount);

    is_valid = has_no_name && has_no_name_bytes;

    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, DECL_OPEN);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, CHAR_SPACE);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, VERSION_NAME);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, EQ);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, QUOTE_QUOT);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, VERSION_1_0);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, QUOTE_QUOT);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, DECL_CLOSE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes an opening tag when the element has children, for example:
 *
 * \code{.xml}
 * <settings name="display">
 * \endcode
 *
 *  An element without children is written as a complete empty-element tag:
 *
 * \code{.xml}
 * <setting name="brightness" value="80"/>
 * \endcode
 *
 *  The name and each attribute are validated before they are written. Attributes
 *  keep their append order, with values copied literally inside double quotes.
 *  WriteElement supplies indentation, line feeds, and any descendants.
 */
static bool_t WriteElementStart(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node)
{
    const FACEOE_Utl_Xml_Attribute_t* current_attribute;
    uint32_t                          attribute_index;
    bool_t                            has_name;
    bool_t                            has_name_bytes;
    bool_t                            is_name_within_limit;
    bool_t                            is_attribute_count_within_limit;
    bool_t                            has_value;
    bool_t                            is_value_within_limit;
    bool_t                            has_value_bytes;
    bool_t                            is_valid;

    // Check storage pointers and limits before validating or writing the element name
    has_name                        = (NULL != node->name);
    has_name_bytes                  = (0 < node->nameByteCount);
    is_name_within_limit            = (node->nameByteCount <= FACEOE_UTL_XML_MAX_NAME_BYTES);
    is_attribute_count_within_limit = (node->attributeCount <= FACEOE_UTL_XML_NODE_MAX_ATTRIBUTES);

    is_valid = has_name && has_name_bytes && is_name_within_limit && is_attribute_count_within_limit;

    if (is_valid)
    {
        is_valid = IsName(node->name, node->nameByteCount);
    }
    if (is_valid)
    {
        // Begin the tag with < and the validated element name
        is_valid = WriteToken(output, capacity, byteCount, STAG_OPEN);
    }
    if (is_valid)
    {
        is_valid = WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);
    }

    current_attribute = node->firstAttribute;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        // Preserve any failure from the element name or previous output
        if (FALSE == is_valid)
        {
            break;
        }

        // The list must hold at least attributeCount attributes
        is_valid = (NULL != current_attribute);

        if (is_valid)
        {
            has_name              = (NULL != current_attribute->name);
            has_name_bytes        = (0 < current_attribute->nameByteCount);
            is_name_within_limit  = (current_attribute->nameByteCount <= FACEOE_UTL_XML_MAX_NAME_BYTES);
            has_value             = (NULL != current_attribute->value);
            is_value_within_limit = (current_attribute->valueByteCount <= FACEOE_UTL_XML_MAX_VALUE_BYTES);

            is_valid = has_name && has_name_bytes && is_name_within_limit && has_value && is_value_within_limit;
        }
        if (is_valid)
        {
            is_valid = IsName(current_attribute->name, current_attribute->nameByteCount);
        }
        if (is_valid)
        {
            // Values must be writable literally because this writer does not escape them
            is_valid = FACEOE_Utl_Xml_IsAttributeValue(current_attribute->value, current_attribute->valueByteCount);
        }
        if (is_valid)
        {
            // Separate attributes with one space, then emit name="value"
            is_valid = WriteByte(output, capacity, byteCount, CHAR_SPACE);
        }
        if (is_valid)
        {
            is_valid = WriteBytes(output, capacity, byteCount, current_attribute->name, current_attribute->nameByteCount);
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, EQ);
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, QUOTE_QUOT);
        }
        if (is_valid)
        {
            // Empty values still receive both quotes; WriteBytes requires a nonempty span
            has_value_bytes = (0 < current_attribute->valueByteCount);
            if (has_value_bytes)
            {
                is_valid = WriteBytes(output, capacity, byteCount, current_attribute->value, current_attribute->valueByteCount);
            }
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, QUOTE_QUOT);
        }

        if (is_valid)
        {
            current_attribute = current_attribute->next;
        }

        // Stop on invalid attribute content or insufficient output space
        if(FALSE == is_valid)
        {
            break;
        }
    }

    if (is_valid)
    {
        // Reject attributes linked beyond attributeCount
        is_valid = (NULL == current_attribute);
    }
    if (is_valid)
    {
        // Leaves close here; elements with children get their closing tag from WriteElement
        if (NULL == node->firstChild)
        {
            is_valid = WriteToken(output, capacity, byteCount, EMPTY_ELEM_TAG_CLOSE);
        }
        else
        {
            is_valid = WriteToken(output, capacity, byteCount, TAG_CLOSE);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the closing tag for an element whose children have been written:
 *
 * \code{.xml}
 * </settings>
 * \endcode
 *
 *  Uses the same name as the opening tag. WriteElement supplies the preceding
 *  line feed and indentation; this function writes only the closing tag.
 */
static bool_t WriteElementEnd(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node)
{
    bool_t is_valid;

    // Append the closing tag after the indentation already written by the caller
    is_valid = WriteToken(output, capacity, byteCount, ETAG_OPEN);
    if (is_valid)
    {
        // Reuse the name validated when the opening tag was written
        is_valid = WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, TAG_CLOSE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the supplied element's entire subtree. For a panel containing a group
 *  followed by a label, the output is:
 *
 * \code{.xml}
 * <panel id="main">
 *     <group id="controls">
 *         <button id="start"/>
 *         <button id="stop"/>
 *     </group>
 *     <label text="Ready"/>
 * </panel>
 * \endcode
 *
 *  Starting at panel, descends into group, writes the two sibling buttons, then
 *  closes group before moving to its sibling label. After label, closes panel
 *  and stops; siblings of panel are left to the caller.
 *  Uses four spaces per nesting level. Leaf elements such as button and label
 *  use empty-element tags. FACEOE_Utl_Xml_WriteDocument appends the final line feed.
 */
static bool_t WriteElement(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* root)
{
    const FACEOE_Utl_Xml_Node_t* current_node;
    uint32_t                     depth;
    uint32_t                     node_index;
    uint32_t                     ancestor_index;
    bool_t                       is_non_root;
    bool_t                       has_no_next_sibling;
    bool_t                       has_current_node;
    bool_t                       has_depth;
    bool_t                       is_at_end;
    bool_t                       is_at_root_depth;
    bool_t                       should_close_parent;
    bool_t                       is_valid;

    current_node = root;
    depth        = 0;
    is_valid     = TRUE;

    for (node_index = 0; node_index < FACEOE_UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        // A NULL cursor marks completion of this element and all its descendants
        if (NULL == current_node)
        {
            break;
        }

        // Start descendants on indented lines; the caller controls spacing before the root
        if (current_node != root)
        {
            is_valid = WriteByte(output, capacity, byteCount, CHAR_LF);
            if (is_valid)
            {
                is_valid = WriteIndent(output, capacity, byteCount, depth * INDENT_SPACES_PER_LEVEL);
            }
        }

        if (is_valid)
        {
            // Leaves use empty-element tags and need no separate closing tag
            is_valid = WriteElementStart(output, capacity, byteCount, current_node);
        }
        // Stop on invalid content or insufficient output space, preserving the failure
        if (FALSE == is_valid)
        {
            break;
        }

        // Descend into children before visiting siblings, subject to the depth limit
        if (NULL != current_node->firstChild)
        {
            is_valid = (depth < FACEOE_UTL_XML_NODE_MAX_DEPTH);
            if (FALSE == is_valid)
            {
                break;
            }

            current_node = current_node->firstChild;

            ++depth;

            continue;
        }

        // Finishing the last child completes its parent, which now needs a closing tag
        is_non_root         = (current_node != root);
        has_no_next_sibling = (NULL == current_node->nextSibling);
        should_close_parent = is_non_root && has_no_next_sibling;

        for (ancestor_index = 0; ancestor_index < FACEOE_UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
        {
            if (FALSE == should_close_parent)
            {
                break;
            }

            // Validate the parent link and depth before writing the ancestor's closing tag
            current_node = current_node->parent;
            has_current_node = (NULL != current_node);
            has_depth        = (0 < depth);

            is_valid = has_current_node && has_depth;
            if (FALSE == is_valid)
            {
                break;
            }
            --depth;

            // Align the closing tag with this ancestor's opening tag
            is_valid = WriteByte(output, capacity, byteCount, CHAR_LF);
            if (is_valid)
            {
                is_valid = WriteIndent(output, capacity, byteCount, depth * INDENT_SPACES_PER_LEVEL);
            }
            if (is_valid)
            {
                is_valid = WriteElementEnd(output, capacity, byteCount, current_node);
            }
            if (FALSE == is_valid)
            {
                break;
            }

            // Keep closing ancestors while no sibling remains and the root is not reached
            is_non_root         = (current_node != root);
            has_no_next_sibling = (NULL == current_node->nextSibling);
            should_close_parent = is_non_root && has_no_next_sibling;
        }

        if (is_valid)
        {
            // Reaching the ancestor limit must not leave a parent waiting to be closed
            is_valid = (FALSE == should_close_parent);
        }
        if (is_valid)
        {
            // Finish at this subtree's root; its siblings belong to the caller's traversal
            if (current_node == root)
            {
                current_node = NULL;
            }
            else
            {
                current_node = current_node->nextSibling;
            }
        }
        else
        {
            break;
        }
    }

    if (is_valid)
    {
        // Reaching the node limit must not leave unfinished nodes or unclosed ancestors
        is_at_end        = (NULL == current_node);
        is_at_root_depth = (0 == depth);
        is_valid         = is_at_end && is_at_root_depth;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches on node type; an unknown type returns FALSE.
 */
static bool_t WriteRootNode(byte_t* output, uint64_t capacity, uint64_t* byteCount, const FACEOE_Utl_Xml_Node_t* node)
{
    bool_t is_valid;

    switch (node->type)
    {
        case FACEOE_UTL_XML_NODE_XML_DECLARATION:
        {
            is_valid = WriteDeclaration(output, capacity, byteCount, node);
            break;
        }
        case FACEOE_UTL_XML_NODE_ELEMENT:
        {
            is_valid = WriteElement(output, capacity, byteCount, node);
            break;
        }
        default:
        {
            is_valid = FALSE;
            break;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the output bound before copying a nonempty span.
 */
static bool_t WriteBytes(byte_t* output, uint64_t capacity, uint64_t* byteCount, const byte_t* bytes, uint32_t numBytes)
{
    bool_t is_byte_count_valid;
    bool_t has_capacity_for_bytes;
    bool_t is_valid;

    LOGASSERT(NULL != output);
    LOGASSERT(NULL != byteCount);
    LOGASSERT(NULL != bytes);
    LOGASSERT(0 < numBytes);

    is_byte_count_valid = ((*byteCount) <= capacity);

    is_valid = is_byte_count_valid;

    if (is_valid)
    {
        has_capacity_for_bytes = (numBytes <= (capacity - (*byteCount)));

        is_valid = has_capacity_for_bytes;
    }

    if (is_valid)
    {
        (void)memcpy(output + (*byteCount), bytes, numBytes);
        (*byteCount) += numBytes;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes through WriteBytes with a one-byte span.
 */
static bool_t WriteByte(byte_t* output, uint64_t capacity, uint64_t* byteCount, byte_t value)
{
    bool_t is_valid;

    is_valid = WriteBytes(output, capacity, byteCount, &value, 1);

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes only spaces; callers write line breaks separately.
 */
static bool_t WriteIndent(byte_t* output, uint64_t capacity, uint64_t* byteCount, uint32_t indentSpaceCount)
{
    bool_t   is_valid;
    uint32_t space_index;

    is_valid = TRUE;

    for (space_index = 0; space_index < indentSpaceCount; ++space_index)
    {
        is_valid = WriteByte(output, capacity, byteCount, CHAR_SPACE);

        if (FALSE == is_valid)
        {
            break;
        }
    }

    return is_valid;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks XML characters first, then excludes characters that cannot be written
 *  literally or would be changed by attribute-value normalization.
 */
bool_t FACEOE_Utl_Xml_IsAttributeValue(const byte_t* utf8Text, uint32_t byteCount)
{
    bool_t   is_attribute_value;
    bool_t   is_ascii;
    bool_t   is_attribute_value_char;
    bool_t   is_whitespace;
    bool_t   is_space;
    bool_t   is_normalized_whitespace;
    uint32_t byte_index;
    byte_t   value_byte;

    is_attribute_value = TRUE;

    if (IsString(utf8Text, byteCount))
    {
        for (byte_index = 0; byte_index < byteCount; ++byte_index)
        {
            value_byte = utf8Text[byte_index];
            is_ascii   = (value_byte <= UTF8_ASCII_MAX_BYTE);
            if (is_ascii)
            {
                is_attribute_value_char  = IsAttributeValueChar(value_byte);
                is_whitespace            = IsWhitespace(value_byte);
                is_space                 = (CHAR_SPACE == value_byte);
                is_normalized_whitespace = is_whitespace && (FALSE == is_space);
                is_attribute_value       = is_attribute_value_char && (FALSE == is_normalized_whitespace);

                if (FALSE == is_attribute_value)
                {
                    break;
                }
            }
        }
    }
    else
    {
        is_attribute_value = FALSE;
    }

    return is_attribute_value;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  FACEOE_Utl_Xml_AppendAttribute validates the name and value.
 */
void FACEOE_Utl_Xml_InitializeAttribute(FACEOE_Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount)
{
    LOGASSERT(NULL != attribute);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);

    attribute->name           = name;
    attribute->nameByteCount  = nameByteCount;
    attribute->value          = value;
    attribute->valueByteCount = valueByteCount;
    attribute->next           = NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  FACEOE_Utl_Xml_WriteDocument validates element names.
 */
void FACEOE_Utl_Xml_InitializeNode(FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_NodeType_t type, const byte_t* name, uint32_t nameByteCount)
{
    LOGASSERT(NULL != node);
    LOGASSERT((NULL != name) || (0 == nameByteCount));

    node->type           = type;
    node->name           = name;
    node->nameByteCount  = nameByteCount;
    node->parent         = NULL;
    node->firstChild     = NULL;
    node->lastChild      = NULL;
    node->nextSibling    = NULL;
    node->firstAttribute = NULL;
    node->lastAttribute  = NULL;
    node->attributeCount = 0;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  FACEOE_Utl_Xml_ReadDocument requires a document in this empty state.
 */
void FACEOE_Utl_Xml_InitializeDocument(FACEOE_Utl_Xml_Document_t* document)
{
    LOGASSERT(NULL != document);

    document->firstNode = NULL;
    document->lastNode  = NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, appending id="start" and then enabled="true" to a childless
 *  button element produces this tag when the document is written:
 *
 * \code{.xml}
 * <button id="start" enabled="true"/>
 * \endcode
 *
 * \verbatim
 * button.firstAttribute -> id -> enabled -> NULL  (next links)
 * button.lastAttribute  -> enabled
 * button.attributeCount = 2
 * \endverbatim
 *
 *  The name and value are checked here, not only when written, so an invalid
 *  attribute fails an assertion at the call that adds it. Nothing is copied.
 */
void FACEOE_Utl_Xml_AppendAttribute(FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_Attribute_t* attribute)
{
    bool_t                            is_name;
    bool_t                            is_attribute_value;
    const FACEOE_Utl_Xml_Attribute_t* found_attribute;

    // Require an element with room for an initialized, unlinked attribute
    LOGASSERT(NULL != node);
    LOGASSERT(NULL != attribute);
    LOGASSERT(NULL != attribute->name);
    LOGASSERT(NULL != attribute->value);
    LOGASSERT(NULL == attribute->next);
    LOGASSERT(FACEOE_UTL_XML_NODE_ELEMENT == node->type);
    LOGASSERT(node->attributeCount < FACEOE_UTL_XML_NODE_MAX_ATTRIBUTES);

    // Check the UTF-8 name before adding it to the element
    is_name = IsName(attribute->name, attribute->nameByteCount);
    LOGASSERT(is_name);

    // Require a value that the writer can copy literally inside double quotes
    is_attribute_value = FACEOE_Utl_Xml_IsAttributeValue(attribute->value, attribute->valueByteCount);
    LOGASSERT(is_attribute_value);

    // Reject duplicate names and reusing an attribute already linked to this element
    found_attribute = FindAttribute(node, attribute, attribute->name, attribute->nameByteCount);
    LOGASSERT(NULL == found_attribute);

    if (NULL != node->lastAttribute)
    {
        // Extend the existing list in insertion order
        node->lastAttribute->next = attribute;
    }
    else
    {
        // The new attribute is also the first attribute of an empty list
        node->firstAttribute = attribute;
    }

    // Record the new tail and include it in the element's attribute count
    node->lastAttribute = attribute;

    node->attributeCount += 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, appending a button and then a label to an empty panel
 *  produces this subtree when the document is written:
 *
 * \code{.xml}
 * <panel>
 *     <button/>
 *     <label/>
 * </panel>
 * \endcode
 *
 * \verbatim
 * panel.firstChild -> button -> label -> NULL  (nextSibling links)
 * panel.lastChild  -> label
 * button.parent    -> panel
 * label.parent     -> panel
 * \endverbatim
 *
 *  The child keeps its attributes and descendants. Searching the parent's ancestor
 *  chain rejects appending the parent itself or one of its ancestors, either of
 *  which would create a cycle.
 */
void FACEOE_Utl_Xml_AppendChild(FACEOE_Utl_Xml_Node_t* parent, FACEOE_Utl_Xml_Node_t* child)
{
    const FACEOE_Utl_Xml_Node_t* ancestor;

    // Require two elements, with no parent or next sibling already set on the child
    LOGASSERT(NULL != parent);
    LOGASSERT(NULL != child);
    LOGASSERT(FACEOE_UTL_XML_NODE_ELEMENT == parent->type);
    LOGASSERT(NULL == child->parent);
    LOGASSERT(NULL == child->nextSibling);
    LOGASSERT(FACEOE_UTL_XML_NODE_ELEMENT == child->type);

    // Reject the parent itself or any of its ancestors as the new child
    ancestor = FindAncestor(parent, child);
    LOGASSERT(NULL == ancestor);

    if (NULL != parent->lastChild)
    {
        // Place the new child after the existing children
        parent->lastChild->nextSibling = child;
    }
    else
    {
        // The new child is also the first child of an empty parent
        parent->firstChild = child;
    }

    // Record the new tail and establish the child's link back to its parent
    parent->lastChild = child;
    child->parent     = parent;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  For example, appending an XML declaration and then a childless panel element
 *  to an empty document produces this output when the document is written:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * <panel/>
 * \endcode
 *
 * \verbatim
 * document.firstNode -> declaration -> panel -> NULL  (nextSibling links)
 * document.lastNode  -> panel
 * declaration.parent = NULL
 * panel.parent       = NULL
 * \endverbatim
 *
 *  The element keeps its subtree. The declaration is optional; appending only the
 *  element leaves firstNode and lastNode both pointing to it.
 */
void FACEOE_Utl_Xml_AppendNode(FACEOE_Utl_Xml_Document_t* document, FACEOE_Utl_Xml_Node_t* node)
{
    const FACEOE_Utl_Xml_Node_t* found_node;

    // Require an unlinked top-level node that is not already the document's tail
    LOGASSERT(NULL != document);
    LOGASSERT(NULL != node);
    LOGASSERT(NULL == node->parent);
    LOGASSERT(NULL == node->nextSibling);
    LOGASSERT(document->lastNode != node);

    if (FACEOE_UTL_XML_NODE_XML_DECLARATION == node->type)
    {
        // A declaration may only be appended to an empty document
        LOGASSERT(NULL == document->firstNode);
    }
    else
    {
        // The only other supported top-level node is a single root element
        LOGASSERT(FACEOE_UTL_XML_NODE_ELEMENT == node->type);

        found_node = FindRootNode(document, FACEOE_UTL_XML_NODE_ELEMENT);
        LOGASSERT(NULL == found_node);
    }

    if (NULL != document->lastNode)
    {
        // Link the root element after the existing declaration
        document->lastNode->nextSibling = node;
    }
    else
    {
        // The new node is also the first node of an empty document
        document->firstNode = node;
    }

    // Record the appended node as the document's final top-level node
    document->lastNode = node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Accepts an optional XML declaration followed by one root element, for example:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * <configuration name="example">
 *     <settings>
 *         <setting name="mode" value="normal"/>
 *         <setting name="brightness" value="80"/>
 *     </settings>
 * </configuration>
 * \endcode
 *
 *  Attribute values are double-quoted and contain no references or tab, LF, or CR.
 *  Between tags, only XML whitespace is accepted.
 *  Top-level nodes are dispatched individually; descendants are read iteratively.
 */
uint64_t FACEOE_Utl_Xml_ReadDocument(FACEOE_Utl_Xml_Document_t* document, byte_t* bufferToReadDocumentFrom, uint64_t sizeOfDocumentBuffer, FACEOE_Utl_Xml_Node_t* nodes, uint32_t nodeCapacity, FACEOE_Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity)
{
    XmlReader_t reader;
    uint64_t    bytes_read;
    bool_t      is_valid;
    uint32_t    node_index;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != bufferToReadDocumentFrom);
    LOGASSERT(NULL != nodes);
    LOGASSERT(NULL != attributes);
    LOGASSERT(NULL == document->firstNode);

    bufferToReadDocumentFrom[sizeOfDocumentBuffer] = '\0';
    reader.document          = document;
    reader.bytes             = bufferToReadDocumentFrom;
    reader.byteCount         = sizeOfDocumentBuffer;
    reader.position          = 0;
    reader.nodes             = nodes;
    reader.nodeCapacity      = nodeCapacity;
    reader.nodeCount         = 0;
    reader.attributes        = attributes;
    reader.attributeCapacity = attributeCapacity;
    reader.attributeCount    = 0;
    reader.markupCount       = 0;
    bytes_read               = 0;

    is_valid = ReadWhitespace(reader.bytes, reader.byteCount, &reader.position, FALSE);

    if (is_valid)
    {
        for (node_index = 0; node_index < FACEOE_UTL_XML_DOCUMENT_MAX_ROOT_NODES; ++node_index)
        {
            // Stop when all input, including trailing whitespace, has been consumed
            if (reader.position == reader.byteCount)
            {
                break;
            }

            is_valid = ReadRootNode(&reader);

            // Stop on a parsing failure so a later read cannot overwrite the error
            if (FALSE == is_valid)
            {
                break;
            }
        }
    }

    if (is_valid)
    {
        // Reject unread input when the top-level node limit ends the loop
        is_valid = (reader.position == reader.byteCount);
    }
    if (is_valid)
    {
        // Whitespace or an XML declaration alone does not form a complete document
        is_valid = (NULL != FindRootNode(document, FACEOE_UTL_XML_NODE_ELEMENT));
    }
    if (is_valid)
    {
        bytes_read = reader.position;
    }

    return bytes_read;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes the declaration and element tree in append order, for example:
 *
 * \code{.xml}
 * <?xml version="1.0"?>
 * <configuration name="example">
 *     <settings>
 *         <setting name="mode" value="normal"/>
 *         <setting name="brightness" value="80"/>
 *     </settings>
 * </configuration>
 * \endcode
 *
 *  Uses four spaces per nesting level and LF line endings, including a final LF.
 *  Elements without children use empty-element tags; attribute values are copied
 *  literally inside double quotes. The output is not zero-terminated.
 *  Top-level nodes are dispatched individually; descendants are written iteratively.
 */
uint64_t FACEOE_Utl_Xml_WriteDocument(const FACEOE_Utl_Xml_Document_t* document, byte_t* bufferToWriteDocumentIn, uint64_t sizeOfDocumentBuffer)
{
    const FACEOE_Utl_Xml_Node_t* current_node;
    uint64_t                     bytes_written;
    bool_t                       is_valid;
    uint32_t                     node_index;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != bufferToWriteDocumentIn);

    current_node  = document->firstNode;
    bytes_written = 0;
    is_valid      = TRUE;

    for (node_index = 0; node_index < FACEOE_UTL_XML_DOCUMENT_MAX_ROOT_NODES; ++node_index)
    {
        // Stop when every top-level node and its descendants have been written
        if (NULL == current_node)
        {
            break;
        }

        is_valid = WriteRootNode(bufferToWriteDocumentIn, sizeOfDocumentBuffer, &bytes_written, current_node);

        if (is_valid)
        {
            // End each top-level node with a line feed, including the last one
            is_valid = WriteByte(bufferToWriteDocumentIn, sizeOfDocumentBuffer, &bytes_written, CHAR_LF);
        }

        // Stop on invalid content or insufficient output space, preserving the failure
        if (FALSE == is_valid)
        {
            break;
        }

        current_node = current_node->nextSibling;
    }

    // Report failure if writing failed or the top-level node limit left nodes unwritten
    if ((FALSE == is_valid) || (NULL != current_node))
    {
        bytes_written = 0;
    }

    return bytes_written;
}
