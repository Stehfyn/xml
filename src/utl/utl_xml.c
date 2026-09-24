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

#define INDENT_SPACES_PER_LEVEL (4)

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
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameStartChar
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
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameChar
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
 * \return Decoded code point, or zero for invalid UTF-8. Check decodedByteCount for success.
 */
static uint32_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t offset, uint32_t* decodedByteCount);


/*************************************************************************************************/
/**
 * \brief Finds child in parent's ancestor chain, including parent.
 *
 * \param[in] parent   Node whose parent chain is searched
 * \param[in] child    Node to find
 *
 * \return Matching ancestor, or NULL if child is absent.
 */
static const Utl_Xml_Node_t* FindAncestor(const Utl_Xml_Node_t* parent, const Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Finds an attribute with the requested name.
 *
 * \param[in] node            Element whose attributes are searched
 * \param[in] attribute       Attribute to be appended; must not already be linked
 * \param[in] name            Name bytes to find
 * \param[in] nameByteCount   Number of name bytes
 *
 * \return Matching attribute, or NULL if the name is absent.
 */
static const Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount);

/*************************************************************************************************/
/**
 * \brief Finds a top-level node of the requested type.
 *
 * \param[in] document   Document to search
 * \param[in] type       Node type to find
 *
 * \return    Matching node, or NULL if absent.
 */
static const Utl_Xml_Node_t* FindNode(const Utl_Xml_Document_t* document, Utl_Xml_NodeType_t type);

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
 * \retval        None
 */
static void WriteBytes(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* bytes, uint32_t numBytes);

/*************************************************************************************************/
/**
 * \brief Appends one byte to the caller's output buffer.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     value       Byte to append
 *
 * \retval        None
 */
static void WriteByte(byte_t* output, uint32_t capacity, uint64_t* byteCount, byte_t value);

/*************************************************************************************************/
/**
 * \brief Writes a null-terminated XML token.
 *
 * \param[in,out] output      Caller-provided byte storage
 * \param[in]     capacity    Number of bytes in output
 * \param[in,out] byteCount   Number of bytes already written
 * \param[in]     token       XML token from utl_xml.h
 *
 * \retval        None
 */
static void WriteToken(byte_t* output, uint32_t capacity, uint64_t* byteCount, const char_t* token);

/*************************************************************************************************/
/**
 * \brief Writes the requested number of indentation spaces.
 *
 * \param[in,out] output             Caller-provided byte storage
 * \param[in]     capacity           Number of bytes in output
 * \param[in,out] byteCount          Number of bytes already written
 * \param[in]     indentSpaceCount   Number of spaces to write
 *
 * \retval        None
 */
static void WriteIndent(byte_t* output, uint32_t capacity, uint64_t* byteCount, uint32_t indentSpaceCount);

/*************************************************************************************************/
/**
 * \brief Writes a document type declaration from its name and optional suffix.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-doctypedecl
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       DOCTYPE node to write
 *
 * \retval        None
 */
static void WriteDoctype(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an XML 1.0 declaration with optional encoding and standalone fields.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-XMLDecl
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       XML declaration node to write
 *
 * \retval        None
 */
static void WriteDeclaration(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes escaped XML character data.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-CharData
 * https://www.w3.org/TR/2008/REC-xml-20081126/#sec-line-ends
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     utf8Text   UTF-8 byte span, or NULL for an empty span
 * \param[in]     numBytes   Number of input bytes
 *
 * \retval        None
 */
static void WriteEscapedCharData(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* utf8Text, uint32_t numBytes);

/*************************************************************************************************/
/**
 * \brief Writes an escaped double-quoted XML attribute value.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-AttValue
 * https://www.w3.org/TR/2008/REC-xml-20081126/#AVNormalize
 * https://www.w3.org/TR/2008/REC-xml-20081126/#sec-line-ends
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     utf8Text   UTF-8 byte span, or NULL for an empty span
 * \param[in]     numBytes   Number of input bytes
 *
 * \retval        None
 */
static void WriteEscapedAttributeValue(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* utf8Text, uint32_t numBytes);

/*************************************************************************************************/
/**
 * \brief Writes an element's start tag or empty-element tag with attributes.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-STag
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-EmptyElemTag
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Element to write
 *
 * \retval        None
 */
static void WriteElementStart(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element's closing tag.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-ETag
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Element to close
 *
 * \retval        None
 */
static void WriteElementEnd(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element and its descendants without recursion.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     root       Root element to write
 *
 * \retval        None
 */
static void WriteElement(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* root);

/*************************************************************************************************/
/**
 * \brief Writes a text node as XML character data.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-CharData
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Text node to write
 *
 * \retval        None
 */
static void WriteText(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an XML comment.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Comment
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Comment node to write
 *
 * \retval        None
 */
static void WriteComment(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an XML processing instruction.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-PI
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Processing-instruction node to write
 *
 * \retval        None
 */
static void WriteProcessingInstruction(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes a text, comment, or processing-instruction node.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Node to write
 *
 * \retval        None
 */
static void WriteLeafNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes one top-level XML node, including an element's descendants.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Node to write
 *
 * \retval        None
 */
static void WriteRootNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

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
    bool_t   is_valid;

    code_point = 0;
    (*decodedByteCount) = 0;
    is_valid = (offset < numBytes);

    if (is_valid)
    {
        position = offset;
        first    = bytes[position];

        if (first <= 0x7F)
        {
            decoded          = first;
            minimum          = 0;
            num_continuations = 0;
        }
        else if ((0xC2 <= first) && (first <= 0xDF))
        {
            decoded          = first & 0x1F;
            minimum          = 0x80;
            num_continuations = 1;
        }
        else if ((0xE0 <= first) && (first <= 0xEF))
        {
            decoded          = first & 0x0F;
            minimum          = 0x800;
            num_continuations = 2;
        }
        else if ((0xF0 <= first) && (first <= 0xF4))
        {
            decoded          = first & 0x07;
            minimum          = 0x10000;
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
                is_valid = (position < numBytes) && (0x80 == (bytes[position] & 0xC0));

                if (FALSE == is_valid)
                {
                    break;
                }

                decoded = (decoded << 6) | (bytes[position] & 0x3F);
            }
        }

        if (is_valid)
        {
            is_valid = (minimum <= decoded) && (decoded <= 0x10FFFF) &&
                      ((decoded < 0xD800) || (0xDFFF < decoded));
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
 *  None
 */
static const Utl_Xml_Node_t* FindAncestor(const Utl_Xml_Node_t* parent, const Utl_Xml_Node_t* child)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* matching_ancestor;
    bool_t                is_ancestor;
    uint32_t              node_depth;

    current_node = parent;
    matching_ancestor = NULL;

    for (node_depth = 0; node_depth < UTL_XML_NODE_MAX_DEPTH; ++node_depth)
    {
        if (NULL == current_node)
        {
            break;
        }

        is_ancestor = (child == current_node);

        if (is_ancestor)
        {
            matching_ancestor = current_node;
            break;
        }

        current_node = current_node->parent;
    }

    LOGASSERT((NULL != matching_ancestor) || (NULL == current_node));

    return matching_ancestor;
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
 *  Checks the output bound before copying a nonempty span.
 */
static void WriteBytes(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* bytes, uint32_t numBytes)
{
    bool_t fits_capacity;

    LOGASSERT(NULL != output);
    LOGASSERT(NULL != byteCount);
    LOGASSERT(NULL != bytes);
    LOGASSERT(0 < numBytes);
    LOGASSERT((*byteCount) <= capacity);

    fits_capacity = (numBytes <= capacity - (*byteCount));
    LOGASSERT(fits_capacity);

    (void)memcpy(output + (*byteCount), bytes, numBytes);

    (*byteCount) += numBytes;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteByte(byte_t* output, uint32_t capacity, uint64_t* byteCount, byte_t value)
{
    WriteBytes(output, capacity, byteCount, &value, 1);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteToken(byte_t* output, uint32_t capacity, uint64_t* byteCount, const char_t* token)
{
    uint32_t byte_index;

    LOGASSERT(NULL != token);

    for (byte_index = 0; '\0' != token[byte_index]; ++byte_index)
    {
        WriteByte(output, capacity, byteCount, token[byte_index]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes only spaces; callers write line breaks separately.
 */
static void WriteIndent(byte_t* output, uint32_t capacity, uint64_t* byteCount, uint32_t indentSpaceCount)
{
    uint32_t space_index;

    for (space_index = 0; space_index < indentSpaceCount; ++space_index)
    {
        WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller supplies an ExternalID, an internal subset, or both in value, without a leading space.
 */
static void WriteDoctype(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t is_name;

    LOGASSERT(NULL != node->name);
    LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

    is_name = Utl_Xml_IsName(node->name, node->nameByteCount);
    LOGASSERT(is_name);

    WriteToken(output, capacity, byteCount, UTL_XML_DOCTYPE_OPEN);
    WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
    WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);

    if (0 < node->valueByteCount)
    {
        WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_DOCTYPE_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller supplies any encoding and standalone fields in value, without a leading space.
 */
static void WriteDeclaration(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    LOGASSERT(NULL == node->name);
    LOGASSERT(0 == node->nameByteCount);
    LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

    WriteToken(output, capacity, byteCount, UTL_XML_DECL_OPEN);
    WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
    WriteToken(output, capacity, byteCount, UTL_XML_VERSION_NAME);
    WriteByte(output, capacity, byteCount, UTL_XML_EQ);
    WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
    WriteToken(output, capacity, byteCount, UTL_XML_VERSION_1_0);
    WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);

    if (0 < node->valueByteCount)
    {
        WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_DECL_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves character data except for XML delimiters and carriage returns.
 */
static void WriteEscapedCharData(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* utf8Text, uint32_t numBytes)
{
    bool_t   is_xml_string;
    uint32_t byte_index;

    LOGASSERT((NULL != utf8Text) || (0 == numBytes));

    if (NULL != utf8Text)
    {
        is_xml_string = Utl_Xml_IsString(utf8Text, numBytes);
        LOGASSERT(is_xml_string);
    }

    for (byte_index = 0; byte_index < numBytes; ++byte_index)
    {
        switch (utf8Text[byte_index])
        {
            case UTL_XML_CHAR_CR:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_CHAR_REF_CR);
                break;
            }
            case UTL_XML_CHAR_AMP:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_AMP);
                break;
            }
            case UTL_XML_CHAR_LT:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_LT);
                break;
            }
            case UTL_XML_CHAR_GT:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_GT);
                break;
            }
            default:
            {
                WriteByte(output, capacity, byteCount, utf8Text[byte_index]);
                break;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses character references for whitespace that attribute normalization would change.
 */
static void WriteEscapedAttributeValue(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* utf8Text, uint32_t numBytes)
{
    bool_t   is_xml_string;
    uint32_t byte_index;

    LOGASSERT((NULL != utf8Text) || (0 == numBytes));

    if (NULL != utf8Text)
    {
        is_xml_string = Utl_Xml_IsString(utf8Text, numBytes);
        LOGASSERT(is_xml_string);
    }

    for (byte_index = 0; byte_index < numBytes; ++byte_index)
    {
        switch (utf8Text[byte_index])
        {
            case UTL_XML_CHAR_TAB:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_CHAR_REF_TAB);
                break;
            }
            case UTL_XML_CHAR_LF:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_CHAR_REF_LF);
                break;
            }
            case UTL_XML_CHAR_CR:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_CHAR_REF_CR);
                break;
            }
            case UTL_XML_CHAR_AMP:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_AMP);
                break;
            }
            case UTL_XML_CHAR_LT:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_LT);
                break;
            }
            case UTL_XML_CHAR_GT:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_GT);
                break;
            }
            case UTL_XML_QUOTE_QUOT:
            {
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_QUOT);
                break;
            }
            default:
            {
                WriteByte(output, capacity, byteCount, utf8Text[byte_index]);
                break;
            }
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteElementStart(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t                     is_name;
    const Utl_Xml_Attribute_t* current_attribute;
    uint32_t                   attribute_index;

    LOGASSERT(NULL != node->name);
    LOGASSERT(NULL == node->value);
    LOGASSERT(0 == node->valueByteCount);

    is_name = Utl_Xml_IsName(node->name, node->nameByteCount);
    LOGASSERT(is_name);

    WriteToken(output, capacity, byteCount, UTL_XML_STAG_OPEN);
    WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);

    current_attribute = node->firstAttribute;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != current_attribute);
        LOGASSERT(NULL != current_attribute->name);

        is_name = Utl_Xml_IsName(current_attribute->name, current_attribute->nameByteCount);
        LOGASSERT(is_name);

        WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        WriteBytes(output, capacity, byteCount, current_attribute->name, current_attribute->nameByteCount);
        WriteByte(output, capacity, byteCount, UTL_XML_EQ);
        WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
        WriteEscapedAttributeValue(output, capacity, byteCount, current_attribute->value, current_attribute->valueByteCount);
        WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);

        current_attribute = current_attribute->next;
    }

    LOGASSERT(NULL == current_attribute);

    if (NULL == node->firstChild)
    {
        WriteToken(output, capacity, byteCount, UTL_XML_EMPTY_ELEM_TAG_CLOSE);
    }
    else
    {
        WriteToken(output, capacity, byteCount, UTL_XML_TAG_CLOSE);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteElementEnd(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    WriteToken(output, capacity, byteCount, UTL_XML_ETAG_OPEN);
    WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);
    WriteToken(output, capacity, byteCount, UTL_XML_TAG_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteElement(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* root)
{
    const Utl_Xml_Node_t* current_node;
    uint32_t              depth;
    uint32_t              node_index;
    bool_t                should_close_parent;
    uint32_t              ancestor_index;

    current_node = root;
    depth        = 0;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        if (current_node != root)
        {
            WriteByte(output, capacity, byteCount, UTL_XML_CHAR_LF);
            WriteIndent(output, capacity, byteCount, depth * INDENT_SPACES_PER_LEVEL);
        }

        if (UTL_XML_NODE_ELEMENT == current_node->type)
        {
            WriteElementStart(output, capacity, byteCount, current_node);

            if (NULL != current_node->firstChild)
            {
                LOGASSERT(depth < UTL_XML_NODE_MAX_DEPTH);
                current_node = current_node->firstChild;
                ++depth;
                continue;
            }
        }
        else
        {
            WriteLeafNode(output, capacity, byteCount, current_node);
        }

        should_close_parent = (current_node != root) && (NULL == current_node->nextSibling);

        for (ancestor_index = 0; ancestor_index < UTL_XML_NODE_MAX_DEPTH; ++ancestor_index)
        {
            if (FALSE == should_close_parent)
            {
                break;
            }

            current_node = current_node->parent;
            LOGASSERT(NULL != current_node);
            LOGASSERT(0 < depth);
            --depth;

            WriteByte(output, capacity, byteCount, UTL_XML_CHAR_LF);
            WriteIndent(output, capacity, byteCount, depth * INDENT_SPACES_PER_LEVEL);
            WriteElementEnd(output, capacity, byteCount, current_node);

            should_close_parent = (current_node != root) && (NULL == current_node->nextSibling);
        }

        LOGASSERT(FALSE == should_close_parent);

        if (current_node == root)
        {
            current_node = NULL;
        }
        else
        {
            current_node = current_node->nextSibling;
        }
    }

    LOGASSERT(NULL == current_node);
    LOGASSERT(0 == depth);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Escapes the text without adding markup.
 */
static void WriteText(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    LOGASSERT(NULL == node->name);
    LOGASSERT(0 == node->nameByteCount);

    WriteEscapedCharData(output, capacity, byteCount, node->value, node->valueByteCount);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteComment(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t is_comment_content;

    LOGASSERT(NULL == node->name);
    LOGASSERT(0 == node->nameByteCount);
    LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

    if (NULL != node->value)
    {
        is_comment_content = Utl_Xml_IsComment(node->value, node->valueByteCount);
        LOGASSERT(is_comment_content);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_COMMENT_OPEN);

    if (0 < node->valueByteCount)
    {
        WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_COMMENT_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteProcessingInstruction(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t is_pi_target;
    bool_t is_pi_data;

    LOGASSERT(NULL != node->name);
    LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

    is_pi_target = Utl_Xml_IsPITarget(node->name, node->nameByteCount);
    LOGASSERT(is_pi_target);

    if (NULL != node->value)
    {
        is_pi_data = Utl_Xml_IsPI(node->value, node->valueByteCount);
        LOGASSERT(is_pi_data);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_PI_OPEN);
    WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);

    if (0 < node->valueByteCount)
    {
        WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_PI_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches text, comment, and processing-instruction nodes.
 */
static void WriteLeafNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    switch (node->type)
    {
        case UTL_XML_NODE_TEXT:
        {
            WriteText(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_COMMENT:
        {
            WriteComment(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_PROCESSING_INSTRUCTION:
        {
            WriteProcessingInstruction(output, capacity, byteCount, node);
            break;
        }
        default:
            LOGFATAL("Invalid XML node type!");
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches top-level nodes; element descendants use the iterative element writer.
 */
static void WriteRootNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    switch (node->type)
    {
        case UTL_XML_NODE_XML_DECLARATION:
        {
            WriteDeclaration(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_DOCTYPE:
        {
            WriteDoctype(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_ELEMENT:
        {
            WriteElement(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_TEXT:
        {
            WriteText(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_COMMENT:
        {
            WriteComment(output, capacity, byteCount, node);
            break;
        }
        case UTL_XML_NODE_PROCESSING_INSTRUCTION:
        {
            WriteProcessingInstruction(output, capacity, byteCount, node);
            break;
        }
        default:
            LOGFATAL("Invalid XML node type!");
    }
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
 *  Empty strings are permitted as text and attribute values.
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

    is_name_char = Utl_Xml_IsNameStartChar(codePoint);

    if (FALSE == is_name_char)
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
 *  None
 */
bool_t Utl_Xml_IsNmtoken(const uint32_t* codePoints, uint32_t numCodePoints)
{
    bool_t   is_nmtoken;
    uint32_t code_point_index;

    LOGASSERT(NULL != codePoints);
    LOGASSERT(0 < numCodePoints);

    is_nmtoken = TRUE;

    for (code_point_index = 0; code_point_index < numCodePoints; ++code_point_index)
    {
        is_nmtoken = Utl_Xml_IsNameChar(codePoints[code_point_index]);

        if (FALSE == is_nmtoken)
        {
            break;
        }
    }

    return is_nmtoken;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Comment content cannot contain a double hyphen or end with a hyphen.
 */
bool_t Utl_Xml_IsComment(const byte_t* utf8Text, uint32_t byteCount)
{
    uint32_t byte_index;
    bool_t   is_comment_content;
    bool_t   has_delimiter;

    LOGASSERT(NULL != utf8Text);

    is_comment_content = Utl_Xml_IsString(utf8Text, byteCount);

    if (is_comment_content)
    {
        for (byte_index = 1; byte_index < byteCount; ++byte_index)
        {
            has_delimiter = ('-' == utf8Text[byte_index - 1]) &&
                            ('-' == utf8Text[byte_index]);

            if (has_delimiter)
            {
                is_comment_content = FALSE;
                break;
            }
        }
    }

    if (is_comment_content)
    {
        if (0 < byteCount)
        {
            is_comment_content = ('-' != utf8Text[byteCount - 1]);
        }
    }

    return is_comment_content;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  PI data cannot contain its closing delimiter.
 */
bool_t Utl_Xml_IsPI(const byte_t* utf8Text, uint32_t byteCount)
{
    uint32_t byte_index;
    bool_t   is_pi_data;
    bool_t   has_delimiter;

    LOGASSERT(NULL != utf8Text);

    is_pi_data = Utl_Xml_IsString(utf8Text, byteCount);

    if (is_pi_data)
    {
        for (byte_index = 1; byte_index < byteCount; ++byte_index)
        {
            has_delimiter = ('?' == utf8Text[byte_index - 1]) &&
                            ('>' == utf8Text[byte_index]);

            if (has_delimiter)
            {
                is_pi_data = FALSE;
                break;
            }
        }
    }

    return is_pi_data;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  PITarget excludes xml in every ASCII letter case after checking Name.
 */
bool_t Utl_Xml_IsPITarget(const byte_t* utf8Text, uint32_t byteCount)
{
    bool_t is_pi_target;
    bool_t is_reserved_target;

    LOGASSERT(NULL != utf8Text);

    is_pi_target = Utl_Xml_IsName(utf8Text, byteCount);

    if (is_pi_target)
    {
        is_reserved_target = (3 == byteCount) &&
                             (('x' == utf8Text[0]) || ('X' == utf8Text[0])) &&
                             (('m' == utf8Text[1]) || ('M' == utf8Text[1])) &&
                             (('l' == utf8Text[2]) || ('L' == utf8Text[2]));
        is_pi_target = (FALSE == is_reserved_target);
    }

    return is_pi_target;
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
void Utl_Xml_InitializeNode(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount)
{
    LOGASSERT(NULL != node);
    LOGASSERT((NULL != name) || (0 == nameByteCount));
    LOGASSERT((NULL != value) || (0 == valueByteCount));

    node->type           = type;
    node->name           = name;
    node->nameByteCount  = nameByteCount;
    node->value          = value;
    node->valueByteCount = valueByteCount;
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
    LOGASSERT(UTL_XML_NODE_XML_DECLARATION != child->type);
    LOGASSERT(UTL_XML_NODE_DOCTYPE != child->type);

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
 *  Validates the caller's attribute and appends it in source order.
 */
void Utl_Xml_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute)
{
    bool_t                     is_name;
    bool_t                     is_xml_string;
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

    is_xml_string = Utl_Xml_IsString(attribute->value, attribute->valueByteCount);
    LOGASSERT(is_xml_string);

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
 *  Links one unlinked node at the end of the top-level list.
 */
void Utl_Xml_AppendNode(Utl_Xml_Document_t* document, Utl_Xml_Node_t* node)
{
    const Utl_Xml_Node_t* found_node;
    bool_t                is_element;
    bool_t                is_comment;
    bool_t                is_processing_instruction;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != node);
    LOGASSERT(NULL == node->parent);
    LOGASSERT(NULL == node->nextSibling);
    LOGASSERT(document->lastNode != node);

    if (UTL_XML_NODE_XML_DECLARATION == node->type)
    {
        LOGASSERT(NULL == document->firstNode);
    }
    else if (UTL_XML_NODE_DOCTYPE == node->type)
    {
        found_node = FindNode(document, UTL_XML_NODE_DOCTYPE);
        LOGASSERT(NULL == found_node);

        found_node = FindNode(document, UTL_XML_NODE_ELEMENT);
        LOGASSERT(NULL == found_node);
    }
    else
    {
        is_element                = (UTL_XML_NODE_ELEMENT == node->type);
        is_comment                = (UTL_XML_NODE_COMMENT == node->type);
        is_processing_instruction = (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type);
        LOGASSERT(is_element || is_comment || is_processing_instruction);

        if (is_element)
        {
            found_node = FindNode(document, UTL_XML_NODE_ELEMENT);
            LOGASSERT(NULL == found_node);
        }
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
 *  None
 */
uint64_t Utl_Xml_WriteDocument(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity)
{
    const Utl_Xml_Node_t* current_node;
    uint64_t              byte_count;
    uint32_t              node_index;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != output);

    current_node = document->firstNode;
    byte_count   = 0;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        WriteRootNode(output, capacity, &byte_count, current_node);
        WriteByte(output, capacity, &byte_count, UTL_XML_CHAR_LF);

        current_node = current_node->nextSibling;
    }

    LOGASSERT(NULL == current_node);

    return byte_count;
}
