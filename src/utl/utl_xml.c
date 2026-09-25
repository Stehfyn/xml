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
#include "utl/utl_util.h"

#include "os/os_assert.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

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
 * \brief Additional ranges from XML 1.0 (Fifth Edition), production [4a] NameChar.
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
 * \param[in]     offset             Current byte offset
 * \param[out]    decodedByteCount   Number of bytes decoded, or zero on failure
 *
 * \retval        0..0x10FFFF   decoded code point when decodedByteCount is nonzero; U+0000 is valid UTF-8.
 * \retval        0             if decoding fails; decodedByteCount is zero.
 */
static uint32_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t offset, uint32_t* decodedByteCount);

/*************************************************************************************************/
/**
 * \brief Finds an attribute with the requested name.
 *
 * \param[in] node            Element whose attributes are searched
 * \param[in] attribute       Attribute to be appended; must not already be linked
 * \param[in] name            Name bytes to find
 * \param[in] nameByteCount   Number of name bytes
 *
 * \retval    non-NULL   matching attribute.
 * \retval    NULL       if the name is absent.
 */
static const Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount);

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
static const Utl_Xml_Node_t* FindAncestor(const Utl_Xml_Node_t* parent, const Utl_Xml_Node_t* child);

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
static const Utl_Xml_Node_t* FindNode(const Utl_Xml_Document_t* document, Utl_Xml_NodeType_t type);

/*************************************************************************************************/
/**
 * \brief Reads an XML 1.0 token when it is present.
 *
 * \param[in]     bytes       UTF-8 bytes
 * \param[in]     byteCount   Number of bytes
 * \param[in,out] position    Offset of the next unread byte
 * \param[in]     token       XML token from utl_xml.h
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
 * \param[in]     bytes       UTF-8 bytes
 * \param[in]     byteCount   Number of bytes
 * \param[in,out] position    Offset of the next unread byte
 * \param[in]     mustReachEnd TRUE when only trailing whitespace is allowed
 *
 * \retval        TRUE        if the scan is in bounds and satisfies mustReachEnd.
 * \retval        FALSE       otherwise
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
 * \param[out]    name            Name bytes
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
 * \param[in,out] position    Offset of the next unread byte
 * \param[out]    value       Value bytes
 * \param[out]    numBytes    Number of value bytes
 *
 * \retval        TRUE        if the quoted value is valid.
 * \retval        FALSE       otherwise
 */
static bool_t ReadAttributeValue(const byte_t* bytes, uint64_t byteCount, uint64_t* position, const byte_t** value, uint32_t* numBytes);

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
static bool_t ReadElementStart(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity, uint32_t* attributeCount, bool_t* isStartTag);

/*************************************************************************************************/
/**
 * \brief Appends an exact byte span to the caller's output buffer.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     bytes       Bytes to append
 * \param[in]     numBytes    Number of bytes to append
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
 * \brief Writes a null-terminated XML token.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     token       XML token from utl_xml.h
 *
 * \retval        TRUE        if the token fits.
 * \retval        FALSE       otherwise
 */
static bool_t WriteToken(byte_t* output, uint64_t capacity, uint64_t* byteCount, const char_t* token);

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
static bool_t WriteDeclaration(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

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
static bool_t WriteElementStart(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

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
static bool_t WriteElementEnd(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element and its descendants without recursion.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     root       Root element to write
 *
 * \retval        TRUE       if the element is valid and fits.
 * \retval        FALSE      otherwise
 */
static bool_t WriteElement(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* root);

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
static bool_t WriteRootNode(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
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
 *  Rejects truncated sequences, overlong forms, surrogates, and code points above U+10FFFF.
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

    code_point = 0;
    (*decodedByteCount) = 0;
    is_valid = (offset < numBytes);

    if (is_valid)
    {
        position = offset;
        first    = bytes[position];

        if (first <= UTF8_ASCII_MAX_BYTE)
        {
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
            is_valid = FALSE;
        }

        if (is_valid)
        {
            for (index = 0; index < num_continuations; ++index)
            {
                ++position;

                is_position_valid = (position < numBytes);

                is_valid = is_position_valid;

                if (is_valid)
                {
                    is_continuation_byte = (UTF8_CONTINUATION_PREFIX == (bytes[position] & UTF8_CONTINUATION_PREFIX_MASK));
                    is_valid = is_continuation_byte;
                }

                if (FALSE == is_valid)
                {
                    break;
                }

                decoded = (decoded << UTF8_CONTINUATION_PAYLOAD_BITS) | (bytes[position] & UTF8_CONTINUATION_PAYLOAD_MASK);
            }
        }

        if (is_valid)
        {
            is_code_point_in_range = InRange(decoded, minimum, UTF8_MAX_CODE_POINT);
            is_valid = is_code_point_in_range;
        }

        if (is_valid)
        {
            is_surrogate = InRange(decoded, UTF8_SURROGATE_MIN_CODE_POINT, UTF8_SURROGATE_MAX_CODE_POINT);
            is_valid = (FALSE == is_surrogate);
        }

        if (is_valid)
        {
            code_point = decoded;
            (*decodedByteCount) = num_continuations + 1;
        }
    }

    return code_point;
}


/*************************************************************************************************/
/**
 * \par Details:
 *  Compares byte contents only when the names have the same byte count.
 */
static const Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount)
{
    const Utl_Xml_Attribute_t* current_attribute;
    const Utl_Xml_Attribute_t* found_attribute;
    uint32_t                   attribute_index;
    bool_t                     has_same_name_byte_count;
    bool_t                     has_same_name;

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
 *  None
 */
static const Utl_Xml_Node_t* FindAncestor(const Utl_Xml_Node_t* parent, const Utl_Xml_Node_t* child)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* ancestor;
    bool_t                is_ancestor;
    uint32_t              node_depth;

    current_node = parent;
    ancestor     = NULL;

    for (node_depth = 0; node_depth < UTL_XML_NODE_MAX_DEPTH; ++node_depth)
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
 *  None
 */
static const Utl_Xml_Node_t* FindNode(const Utl_Xml_Document_t* document, Utl_Xml_NodeType_t type)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* found_node;
    uint32_t              node_index;
    bool_t                has_node;

    found_node = NULL;
    current_node = document->firstNode;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
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
 *  Advances position only when the token is present.
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

    for (byte_index = 0; '\0' != token[byte_index]; ++byte_index)
    {
        token_byte = token[byte_index];

        is_position_valid = ((*position) < byteCount);

        is_valid = is_position_valid;

        if (is_valid)
        {
            is_token_byte_available = (byte_index < (byteCount - (*position)));

            is_valid = is_token_byte_available;
        }
        if (is_valid)
        {
            is_matching_byte = (token_byte == bytes[(*position) + byte_index]);

            is_valid = is_matching_byte;
        }

        if (FALSE == is_valid)
        {
            break;
        }
    }

    if (is_valid)
    {
        (*position) += byte_index;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t ReadWhitespace(const byte_t* bytes, uint64_t byteCount, uint64_t* position, bool_t mustReachEnd)
{
    uint64_t byte_index;
    bool_t   is_whitespace;
    bool_t   is_valid;

    is_valid = ((*position) <= byteCount);

    if (is_valid)
    {
        for (byte_index = (*position); byte_index < byteCount; ++byte_index)
        {
            is_whitespace = Utl_Xml_IsWhitespace(bytes[byte_index]);

            if (FALSE == is_whitespace)
            {
                break;
            }
        }

        (*position) = byte_index;

        if (mustReachEnd)
        {
            is_valid = (byte_index == byteCount);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The name ends at whitespace, =, /, or >.
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

    parsed_name = bytes + (*position);

    for (byte_index = (*position); byte_index < byteCount; ++byte_index)
    {
        is_whitespace = Utl_Xml_IsWhitespace(bytes[byte_index]);
        is_equals     = (UTL_XML_EQ == bytes[byte_index]);
        is_slash      = ('/' == bytes[byte_index]);
        is_tag_close  = (UTL_XML_CHAR_GT == bytes[byte_index]);
        is_name_end   = is_whitespace || is_equals || is_slash || is_tag_close;

        if (is_name_end)
        {
            break;
        }
    }

    span_byte_count      = byte_index - (*position);
    has_name_bytes       = (0 < span_byte_count);
    is_within_name_limit = (span_byte_count <= UTL_XML_MAX_NAME_BYTES);

    is_valid = has_name_bytes && is_within_name_limit;

    if (is_valid)
    {
        is_valid = Utl_Xml_IsName(parsed_name, (uint32_t)span_byte_count);
    }
    if (is_valid)
    {
        (*nameByteCount) = (uint32_t)span_byte_count;
        (*position)      = byte_index;
        (*name)          = parsed_name;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller checks the value's characters before appending an attribute.
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
        is_equals = (UTL_XML_EQ == bytes[(*position)]);

        is_valid = is_equals;
    }
    if (is_valid)
    {
        is_open_quote = (UTL_XML_QUOTE_QUOT == bytes[(*position) + 1]);

        is_valid = is_open_quote;
    }
    if (is_valid)
    {
        (*position) += 2;

        parsed_value = bytes + (*position);

        for (byte_index = (*position); byte_index < byteCount; ++byte_index)
        {
            is_value_close = (UTL_XML_QUOTE_QUOT == bytes[byte_index]);

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
            is_within_value_limit = (span_byte_count <= UTL_XML_MAX_VALUE_BYTES);

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
 *  Utl_Xml_AppendAttribute checks attribute names, values, and uniqueness.
 */
static bool_t ReadElementStart(const byte_t* markup, uint64_t markupByteCount, uint64_t* position, Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity, uint32_t* attributeCount, bool_t* isStartTag)
{
    Utl_Xml_Attribute_t* attribute;
    const byte_t*        name;
    const byte_t*        value;
    uint32_t             name_byte_count;
    uint32_t             value_byte_count;
    uint32_t             attribute_index;
    uint64_t             whitespace_start;
    bool_t               has_whitespace;
    bool_t               is_empty_elem_tag_close;
    bool_t               is_tag_close;
    bool_t               is_start_tag;
    bool_t               is_valid;

    is_valid = ReadToken(markup, markupByteCount, position, UTL_XML_STAG_OPEN);
    if (is_valid)
    {
        is_valid = ReadName(markup, markupByteCount, position, &name, &name_byte_count);
    }
    if (is_valid)
    {
        Utl_Xml_InitializeNode(node, UTL_XML_NODE_ELEMENT, name, name_byte_count);
    }

    for (attribute_index = 0; attribute_index < UTL_XML_NODE_MAX_ATTRIBUTES; ++attribute_index)
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
            is_valid = Utl_Xml_IsAttributeValue(value, value_byte_count);
        }
        if (is_valid)
        {
            is_valid = (NULL == FindAttribute(node, NULL, name, name_byte_count));
        }
        if (is_valid)
        {
            attribute = &attributes[(*attributeCount)];
            Utl_Xml_InitializeAttribute(attribute, name, name_byte_count, value, value_byte_count);
            Utl_Xml_AppendAttribute(node, attribute);
            (*attributeCount) += 1;
        }

        if (FALSE == is_valid)
        {
            break;
        }
    }

    if (is_valid)
    {
        is_empty_elem_tag_close = ReadToken(markup, markupByteCount, position, UTL_XML_EMPTY_ELEM_TAG_CLOSE);

        is_start_tag = (FALSE == is_empty_elem_tag_close);

        if (is_start_tag)
        {
            is_tag_close = ReadToken(markup, markupByteCount, position, UTL_XML_TAG_CLOSE);

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
 *  None
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
 *  None
 */
static bool_t WriteToken(byte_t* output, uint64_t capacity, uint64_t* byteCount, const char_t* token)
{
    uint32_t byte_index;
    bool_t   is_valid;

    LOGASSERT(NULL != token);

    is_valid = TRUE;
    for (byte_index = 0; '\0' != token[byte_index]; ++byte_index)
    {
        is_valid = WriteByte(output, capacity, byteCount, token[byte_index]);
        if (FALSE == is_valid)
        {
            break;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes only spaces; callers write line breaks separately.
 */
static bool_t WriteIndent(byte_t* output, uint64_t capacity, uint64_t* byteCount, uint32_t indentSpaceCount)
{
    uint32_t space_index;
    bool_t   is_valid;

    is_valid = TRUE;
    for (space_index = 0; space_index < indentSpaceCount; ++space_index)
    {
        is_valid = WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        if (FALSE == is_valid)
        {
            break;
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteDeclaration(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t has_no_name;
    bool_t has_no_name_bytes;
    bool_t is_valid;

    has_no_name       = (NULL == node->name);
    has_no_name_bytes = (0 == node->nameByteCount);

    is_valid = has_no_name && has_no_name_bytes;

    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, UTL_XML_DECL_OPEN);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, UTL_XML_VERSION_NAME);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, UTL_XML_EQ);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, UTL_XML_VERSION_1_0);
    }
    if (is_valid)
    {
        is_valid = WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, UTL_XML_DECL_CLOSE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteElementStart(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    const Utl_Xml_Attribute_t* current_attribute;
    uint32_t                   attribute_index;
    bool_t                     has_name;
    bool_t                     has_name_bytes;
    bool_t                     is_name_within_limit;
    bool_t                     is_attribute_count_within_limit;
    bool_t                     has_value;
    bool_t                     is_value_within_limit;
    bool_t                     has_value_bytes;
    bool_t                     is_valid;

    has_name                        = (NULL != node->name);
    has_name_bytes                  = (0 < node->nameByteCount);
    is_name_within_limit            = (node->nameByteCount <= UTL_XML_MAX_NAME_BYTES);
    is_attribute_count_within_limit = (node->attributeCount <= UTL_XML_NODE_MAX_ATTRIBUTES);

    is_valid = has_name && has_name_bytes && is_name_within_limit && is_attribute_count_within_limit;

    if (is_valid)
    {
        is_valid = Utl_Xml_IsName(node->name, node->nameByteCount);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, UTL_XML_STAG_OPEN);
    }
    if (is_valid)
    {
        is_valid = WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);
    }

    current_attribute = node->firstAttribute;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        if (FALSE == is_valid)
        {
            break;
        }

        is_valid = (NULL != current_attribute);

        if (is_valid)
        {
            has_name              = (NULL != current_attribute->name);
            has_name_bytes        = (0 < current_attribute->nameByteCount);
            is_name_within_limit  = (current_attribute->nameByteCount <= UTL_XML_MAX_NAME_BYTES);
            has_value             = (NULL != current_attribute->value);
            is_value_within_limit = (current_attribute->valueByteCount <= UTL_XML_MAX_VALUE_BYTES);

            is_valid = has_name && has_name_bytes && is_name_within_limit && has_value && is_value_within_limit;
        }
        if (is_valid)
        {
            is_valid = Utl_Xml_IsName(current_attribute->name, current_attribute->nameByteCount);
        }
        if (is_valid)
        {
            is_valid = Utl_Xml_IsAttributeValue(current_attribute->value, current_attribute->valueByteCount);
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        }
        if (is_valid)
        {
            is_valid = WriteBytes(output, capacity, byteCount, current_attribute->name, current_attribute->nameByteCount);
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, UTL_XML_EQ);
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
        }
        if (is_valid)
        {
            has_value_bytes = (0 < current_attribute->valueByteCount);
            if (has_value_bytes)
            {
                is_valid = WriteBytes(output, capacity, byteCount, current_attribute->value, current_attribute->valueByteCount);
            }
        }
        if (is_valid)
        {
            is_valid = WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
        }

        if (is_valid)
        {
            current_attribute = current_attribute->next;
        }

        if(FALSE == is_valid)
        {
            break;
        }
    }

    if (is_valid)
    {
        is_valid = (NULL == current_attribute);
    }
    if (is_valid)
    {
        if (NULL == node->firstChild)
        {
            is_valid = WriteToken(output, capacity, byteCount, UTL_XML_EMPTY_ELEM_TAG_CLOSE);
        }
        else
        {
            is_valid = WriteToken(output, capacity, byteCount, UTL_XML_TAG_CLOSE);
        }
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteElementEnd(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t is_valid;

    is_valid = WriteToken(output, capacity, byteCount, UTL_XML_ETAG_OPEN);
    if (is_valid)
    {
        is_valid = WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);
    }
    if (is_valid)
    {
        is_valid = WriteToken(output, capacity, byteCount, UTL_XML_TAG_CLOSE);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static bool_t WriteElement(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* root)
{
    const Utl_Xml_Node_t* current_node;
    uint32_t              depth;
    uint32_t              node_index;
    uint32_t              ancestor_index;
    bool_t                is_non_root;
    bool_t                has_no_next_sibling;
    bool_t                has_current_node;
    bool_t                has_depth;
    bool_t                is_at_end;
    bool_t                is_at_root_depth;
    bool_t                should_close_parent;
    bool_t                is_valid;

    current_node = root;
    depth        = 0;
    is_valid     = TRUE;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        if (current_node != root)
        {
            is_valid = WriteByte(output, capacity, byteCount, UTL_XML_CHAR_LF);
            if (is_valid)
            {
                is_valid = WriteIndent(output, capacity, byteCount, depth * INDENT_SPACES_PER_LEVEL);
            }
        }

        if (is_valid)
        {
            is_valid = WriteElementStart(output, capacity, byteCount, current_node);
        }
        if (FALSE == is_valid)
        {
            break;
        }

        if (NULL != current_node->firstChild)
        {
            is_valid = (depth < UTL_XML_NODE_MAX_DEPTH);
            if (FALSE == is_valid)
            {
                break;
            }
            current_node = current_node->firstChild;
            ++depth;
            continue;
        }

        is_non_root         = (current_node != root);
        has_no_next_sibling = (NULL == current_node->nextSibling);
        should_close_parent = is_non_root && has_no_next_sibling;

        for (ancestor_index = 0; ancestor_index < UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
        {
            if (FALSE == should_close_parent)
            {
                break;
            }

            current_node = current_node->parent;
            has_current_node = (NULL != current_node);
            has_depth        = (0 < depth);
            is_valid        = has_current_node && has_depth;
            if (FALSE == is_valid)
            {
                break;
            }
            --depth;

            is_valid = WriteByte(output, capacity, byteCount, UTL_XML_CHAR_LF);
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

            is_non_root         = (current_node != root);
            has_no_next_sibling = (NULL == current_node->nextSibling);
            should_close_parent = is_non_root && has_no_next_sibling;
        }

        if (is_valid)
        {
            is_valid = (FALSE == should_close_parent);
        }
        if (is_valid)
        {
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
        is_at_end        = (NULL == current_node);
        is_at_root_depth = (0 == depth);
        is_valid         = is_at_end && is_at_root_depth;
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches top-level nodes; element descendants use the iterative element writer.
 */
static bool_t WriteRootNode(byte_t* output, uint64_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t is_valid;

    switch (node->type)
    {
        case UTL_XML_NODE_XML_DECLARATION:
        {
            is_valid = WriteDeclaration(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_ELEMENT:
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
    bool_t is_bmp_before_surrogates = InRange(codePoint, 0x20, 0xD7FF);
    bool_t is_bmp_after_surrogates  = InRange(codePoint, 0xE000, 0xFFFD);
    bool_t is_supplementary         = InRange(codePoint, 0x10000, 0x10FFFF);

    return is_tab || is_line_feed || is_carriage_return || is_bmp_before_surrogates || is_bmp_after_surrogates || is_supplementary;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Empty strings are permitted as attribute values.
 */
bool_t Utl_Xml_IsString(const byte_t* utf8Text, uint32_t byteCount)
{
    bool_t   is_xml_string;
    uint32_t byte_offset;
    uint32_t code_point;
    uint32_t decoded_byte_count;
    bool_t   decode_failed;
    bool_t   is_invalid_code_point;

    LOGASSERT(NULL != utf8Text);
    LOGASSERT(UTL_XML_MAX_VALUE_BYTES >= byteCount);

    is_xml_string = TRUE;

    for (byte_offset = 0; byte_offset < byteCount; byte_offset += decoded_byte_count)
    {
        code_point = DecodeUtf8(utf8Text, byteCount, byte_offset, &decoded_byte_count);

        decode_failed         = (0 == decoded_byte_count);
        is_invalid_code_point = (FALSE == Utl_Xml_IsChar(code_point));

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
 *  Checks the literal-character branch of AttValue for double quotes.
 */
bool_t Utl_Xml_IsAttributeValueChar(uint32_t codePoint)
{
    bool_t is_xml_char;
    bool_t is_less_than;
    bool_t is_ampersand;
    bool_t is_double_quote;
    bool_t is_reserved;
    bool_t is_attribute_value_char;

    is_xml_char              = Utl_Xml_IsChar(codePoint);
    is_less_than             = (UTL_XML_CHAR_LT == codePoint);
    is_ampersand             = (UTL_XML_CHAR_AMP == codePoint);
    is_double_quote          = (UTL_XML_QUOTE_QUOT == codePoint);
    is_reserved              = is_less_than || is_ampersand || is_double_quote;
    is_attribute_value_char = is_xml_char && (FALSE == is_reserved);

    return is_attribute_value_char;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks XML characters first, then excludes characters that cannot be written
 *  literally or would be changed by attribute-value normalization.
 */
bool_t Utl_Xml_IsAttributeValue(const byte_t* utf8Text, uint32_t byteCount)
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

    if (Utl_Xml_IsString(utf8Text, byteCount))
    {
        for (byte_index = 0; byte_index < byteCount; ++byte_index)
        {
            value_byte = utf8Text[byte_index];
            is_ascii   = (value_byte <= UTF8_ASCII_MAX_BYTE);
            if (is_ascii)
            {
                is_attribute_value_char  = Utl_Xml_IsAttributeValueChar(value_byte);
                is_whitespace            = Utl_Xml_IsWhitespace(value_byte);
                is_space                 = (UTL_XML_CHAR_SPACE == value_byte);
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
    bool_t   is_name_start_char;
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
 *  None
 */
bool_t Utl_Xml_IsNameChar(uint32_t codePoint)
{
    bool_t   is_name_char;
    uint32_t range_index;
    uint32_t first;
    uint32_t last;

    is_name_char = FALSE;

    if (Utl_Xml_IsNameStartChar(codePoint))
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
bool_t Utl_Xml_IsName(const byte_t* utf8Text, uint32_t byteCount)
{
    bool_t   is_name;
    uint32_t byte_offset;
    uint32_t code_point;
    uint32_t decoded_byte_count;
    bool_t   decode_failed;

    LOGASSERT(NULL != utf8Text);
    LOGASSERT(0 < byteCount);
    LOGASSERT(UTL_XML_MAX_NAME_BYTES >= byteCount);

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
                is_name = Utl_Xml_IsNameStartChar(code_point);
            }
            else
            {
                is_name = Utl_Xml_IsNameChar(code_point);
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
 *  Stores borrowed byte spans and clears the attribute link.
 */
void Utl_Xml_InitializeAttribute(Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount)
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
 *  None
 */
void Utl_Xml_InitializeNode(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const byte_t* name, uint32_t nameByteCount)
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
 *  None
 */
void Utl_Xml_InitializeDocument(Utl_Xml_Document_t* document)
{
    LOGASSERT(NULL != document);

    document->firstNode = NULL;
    document->lastNode  = NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates the caller's attribute and appends it in source order. The writer copies values
 *  without references, so a value must not contain <, &, ", or whitespace other than spaces.
 */
void Utl_Xml_AppendAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute)
{
    bool_t                     is_name;
    bool_t                     is_attribute_value;
    const Utl_Xml_Attribute_t* found_attribute;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != attribute);
    LOGASSERT(NULL != attribute->name);
    LOGASSERT(NULL != attribute->value);
    LOGASSERT(NULL == attribute->next);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);
    LOGASSERT(node->attributeCount < UTL_XML_NODE_MAX_ATTRIBUTES);

    is_name = Utl_Xml_IsName(attribute->name, attribute->nameByteCount);
    LOGASSERT(is_name);

    is_attribute_value = Utl_Xml_IsAttributeValue(attribute->value, attribute->valueByteCount);
    LOGASSERT(is_attribute_value);

    found_attribute = FindAttribute(node, attribute, attribute->name, attribute->nameByteCount);
    LOGASSERT(NULL == found_attribute);

    if (NULL != node->lastAttribute)
    {
        node->lastAttribute->next = attribute;
    }
    else
    {
        node->firstAttribute = attribute;
    }

    node->lastAttribute = attribute;

    node->attributeCount += 1;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Links an unlinked child at the end of the element's child list.
 */
void Utl_Xml_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child)
{
    const Utl_Xml_Node_t* ancestor;

    LOGASSERT(NULL != parent);
    LOGASSERT(NULL != child);
    LOGASSERT(UTL_XML_NODE_ELEMENT == parent->type);
    LOGASSERT(NULL == child->parent);
    LOGASSERT(NULL == child->nextSibling);
    LOGASSERT(UTL_XML_NODE_ELEMENT == child->type);

    ancestor = FindAncestor(parent, child);
    LOGASSERT(NULL == ancestor);

    if (NULL != parent->lastChild)
    {
        parent->lastChild->nextSibling = child;
    }
    else
    {
        parent->firstChild = child;
    }

    parent->lastChild = child;
    child->parent     = parent;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Links one unlinked node at the end of the top-level list.
 */
void Utl_Xml_AppendNode(Utl_Xml_Document_t* document, Utl_Xml_Node_t* node)
{
    const Utl_Xml_Node_t* found_node;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != node);
    LOGASSERT(NULL == node->parent);
    LOGASSERT(NULL == node->nextSibling);
    LOGASSERT(document->lastNode != node);

    if (UTL_XML_NODE_XML_DECLARATION == node->type)
    {
        LOGASSERT(NULL == document->firstNode);
    }
    else
    {
        LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

        found_node = FindNode(document, UTL_XML_NODE_ELEMENT);
        LOGASSERT(NULL == found_node);
    }

    if (NULL != document->lastNode)
    {
        document->lastNode->nextSibling = node;
    }
    else
    {
        document->firstNode = node;
    }

    document->lastNode = node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Utl_StrStr finds each <, so every token holds one tag and the whitespace after it.
 *  The open element is the parent of the elements that follow it.
 */
uint64_t Utl_Xml_ReadDocument(Utl_Xml_Document_t* document, byte_t* bufferToReadDocumentFrom, uint64_t sizeOfDocumentBuffer, Utl_Xml_Node_t* nodes, uint32_t nodeCapacity, Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity)
{
    Utl_Xml_Node_t*       node;
    Utl_Xml_Node_t*       parent;
    const Utl_Xml_Node_t* root_node;
    byte_t*               document_cursor;
    byte_t*               next_markup;
    const byte_t*         name;
    const byte_t*         version;
    uint64_t              bytes_read;
    uint64_t              document_byte_count;
    uint64_t              offset;
    uint64_t              whitespace_start;
    size_t                remaining_byte_count;
    uint32_t              name_byte_count;
    uint32_t              version_byte_count;
    uint32_t              node_count;
    uint32_t              attribute_count;
    uint32_t              token_index;
    bool_t                is_end_tag;
    bool_t                is_declaration;
    bool_t                is_start_tag;
    bool_t                is_first_markup;
    bool_t                is_end_of_document;
    bool_t                is_top_level;
    bool_t                is_empty_document;
    bool_t                is_valid;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != bufferToReadDocumentFrom);
    LOGASSERT(NULL != nodes);
    LOGASSERT(NULL != attributes);
    LOGASSERT(NULL == document->firstNode);

    bufferToReadDocumentFrom[sizeOfDocumentBuffer] = '\0';
    bytes_read       = 0;
    offset           = 0;
    node_count       = 0;
    attribute_count  = 0;
    parent           = NULL;

    is_valid = ReadWhitespace(bufferToReadDocumentFrom, sizeOfDocumentBuffer, &offset, FALSE);

    if (is_valid)
    {
        is_valid = (offset < sizeOfDocumentBuffer);
    }

    if (is_valid)
    {
        document_cursor = bufferToReadDocumentFrom + offset;
        for (token_index = 0; token_index < UTL_XML_DOCUMENT_MAX_NODES; ++token_index)
        {
            is_valid = (UTL_XML_CHAR_LT == document_cursor[0]);
            if (FALSE == is_valid)
            {
                break;
            }

            next_markup = (byte_t*)Utl_StrStr((const char_t*)(document_cursor + 1), UTL_XML_STAG_OPEN, FALSE);

            if (NULL != next_markup)
            {
                next_markup[0] = '\0';
            }

            remaining_byte_count = (size_t)(bufferToReadDocumentFrom + sizeOfDocumentBuffer - document_cursor);
            document_byte_count  = (uint64_t)strnlen((const char_t*)document_cursor, remaining_byte_count);
            offset               = 0;
            is_declaration       = FALSE;

            is_end_tag = ReadToken(document_cursor, document_byte_count, &offset, UTL_XML_ETAG_OPEN);

            if (FALSE == is_end_tag)
            {
                is_declaration = ReadToken(document_cursor, document_byte_count, &offset, UTL_XML_DECL_OPEN);
            }

            if (is_end_tag)
            {
                is_valid = (NULL != parent);
                if (is_valid)
                {
                    is_valid = ReadName(document_cursor, document_byte_count, &offset, &name, &name_byte_count);
                }
                if (is_valid)
                {
                    is_valid = (parent->nameByteCount == name_byte_count);
                }
                if (is_valid)
                {
                    is_valid = (0 == memcmp(parent->name, name, name_byte_count));
                }
                if (is_valid)
                {
                    is_valid = ReadWhitespace(document_cursor, document_byte_count, &offset, FALSE);
                }
                if (is_valid)
                {
                    is_valid = ReadToken(document_cursor, document_byte_count, &offset, UTL_XML_TAG_CLOSE);
                }
                if (is_valid)
                {
                    parent = parent->parent;
                }
            }
            else
            {
                is_valid     = (node_count < nodeCapacity);
                is_start_tag = FALSE;

                if (is_valid)
                {
                    node = &nodes[node_count];
                    if (is_declaration)
                    {
                        is_first_markup   = (bufferToReadDocumentFrom == document_cursor);
                        is_top_level      = (NULL == parent);
                        is_empty_document = (NULL == document->firstNode);
                        is_valid          = is_first_markup && is_top_level && is_empty_document;
                        if (is_valid)
                        {
                            whitespace_start = offset;
                            is_valid = ReadWhitespace(document_cursor, document_byte_count, &offset, FALSE);
                        }
                        if (is_valid)
                        {
                            is_valid = (whitespace_start < offset);
                        }
                        if (is_valid)
                        {
                            is_valid = ReadToken(document_cursor, document_byte_count, &offset, UTL_XML_VERSION_NAME);
                        }
                        if (is_valid)
                        {
                            is_valid = ReadAttributeValue(document_cursor, document_byte_count, &offset, &version, &version_byte_count);
                        }
                        if (is_valid)
                        {
                            is_valid = (UTL_XML_VERSION_1_0_BYTE_COUNT == version_byte_count);
                        }
                        if (is_valid)
                        {
                            is_valid = (0 == memcmp(version, UTL_XML_VERSION_1_0, version_byte_count));
                        }
                        if (is_valid)
                        {
                            is_valid = ReadToken(document_cursor, document_byte_count, &offset, UTL_XML_DECL_CLOSE);
                        }
                        if (is_valid)
                        {
                            Utl_Xml_InitializeNode(node, UTL_XML_NODE_XML_DECLARATION, NULL, 0);
                        }
                    }
                    else
                    {
                        is_valid = ReadElementStart(document_cursor, document_byte_count, &offset, node, attributes, attributeCapacity, &attribute_count, &is_start_tag);
                        if (is_valid)
                        {
                            is_top_level = (NULL == parent);
                            if (is_top_level)
                            {
                                is_valid = (NULL == FindNode(document, UTL_XML_NODE_ELEMENT));
                            }
                        }
                    }
                }

                if (is_valid)
                {
                    if (NULL == parent)
                    {
                        Utl_Xml_AppendNode(document, node);
                    }
                    else
                    {
                        Utl_Xml_AppendChild(parent, node);
                    }

                    node_count += 1;

                    if (is_start_tag)
                    {
                        parent = node;
                    }
                }
            }

            if (is_valid)
            {
                is_valid = ReadWhitespace(document_cursor, document_byte_count, &offset, TRUE);
            }

            if (NULL != next_markup)
            {
                next_markup[0] = UTL_XML_CHAR_LT;
            }

            if (FALSE == is_valid)
            {
                break;
            }

            document_cursor += document_byte_count;
            if (NULL == next_markup)
            {
                break;
            }
        }
    }

    if (is_valid)
    {
        is_end_of_document = ((bufferToReadDocumentFrom + sizeOfDocumentBuffer) == document_cursor);
        is_valid           = is_end_of_document;
    }

    if (is_valid)
    {
        is_top_level = (NULL == parent);
        is_valid     = is_top_level;
    }

    if (is_valid)
    {
        root_node = FindNode(document, UTL_XML_NODE_ELEMENT);
        is_valid  = (NULL != root_node);
    }

    if (is_valid)
    {
        bytes_read = (uint64_t)(document_cursor - bufferToReadDocumentFrom);
    }

    return bytes_read;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
uint64_t Utl_Xml_WriteDocument(const Utl_Xml_Document_t* document, byte_t* bufferToWriteDocumentIn, uint64_t sizeOfDocumentBuffer)
{
    const Utl_Xml_Node_t* current_node;
    uint64_t              bytes_written;
    bool_t                is_valid;
    uint32_t              node_index;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != bufferToWriteDocumentIn);

    current_node  = document->firstNode;
    bytes_written = 0;
    is_valid      = TRUE;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        is_valid = WriteRootNode(bufferToWriteDocumentIn, sizeOfDocumentBuffer, &bytes_written, current_node);

        if (is_valid)
        {
            is_valid = WriteByte(bufferToWriteDocumentIn, sizeOfDocumentBuffer, &bytes_written, UTL_XML_CHAR_LF);
        }

        if (FALSE == is_valid)
        {
            break;
        }

        current_node = current_node->nextSibling;
    }

    if ((FALSE == is_valid) || (NULL != current_node))
    {
        bytes_written = 0;
    }

    return bytes_written;
}
