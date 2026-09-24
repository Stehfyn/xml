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

#include "utl/utl_xml_document.h"
#include "utl/utl_xml.h"

#include "os/os_assert.h"

#include <string.h>

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

/*************************************************************************************************/
/**
 * \brief Finds a top-level node of the requested type.
 *
 * \param[in] document  Document to search
 * \param[in] type      Node type to find
 *
 * \return Matching node, or NULL if absent.
 */
static const Utl_Xml_Node_t* FindNode(const Utl_Xml_Document_t* document, Utl_Xml_NodeType_t type);

/*************************************************************************************************/
/**
 * \brief Appends an exact byte span to the caller's output buffer.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     bytes      Bytes to append
 * \param[in]     numBytes   Number of bytes to append
 *
 * \retval None
 */
static void WriteBytes(byte_t* output, uint32_t capacity, uint32_t* byteCount, const byte_t* bytes, uint32_t numBytes);

/*************************************************************************************************/
/**
 * \brief Appends one byte to the caller's output buffer.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     value      Byte to append
 *
 * \retval None
 */
static void WriteByte(byte_t* output, uint32_t capacity, uint32_t* byteCount, byte_t value);

/*************************************************************************************************/
/**
 * \brief Writes a one-digit hexadecimal XML character reference.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-CharRef
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     hexDigit   Hexadecimal digit
 *
 * \retval None
 */
static void WriteHexCharReference(byte_t* output, uint32_t capacity, uint32_t* byteCount, byte_t hexDigit);

/*************************************************************************************************/
/**
 * \brief Escapes XML text or a double-quoted attribute value.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-AttValue
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-CharData
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     utf8Text   UTF-8 byte span, or NULL for an empty span
 * \param[in]     numBytes   Number of input bytes
 * \param[in]     isAttribute  TRUE for an attribute value
 *
 * \retval None
 */
static void WriteEscaped(byte_t* output, uint32_t capacity, uint32_t* byteCount, const byte_t* utf8Text, uint32_t numBytes, bool_t isAttribute);

/*************************************************************************************************/
/**
 * \brief Writes a caller-supplied XML declaration or DOCTYPE span.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-XMLDecl
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-doctypedecl
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Declaration to write
 *
 * \retval None
 */
static void WriteMarkup(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element's opening tag and attributes.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-STag
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Element to write
 *
 * \retval None
 */
static void WriteElementStart(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node);

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
 * \retval None
 */
static void WriteElementEnd(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes a text, comment, or processing-instruction node.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-Comment
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-PI
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Leaf node to write
 *
 * \retval None
 */
static void WriteLeaf(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes a node and its descendants without recursion.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     root       Root of the subtree
 *
 * \retval None
 */
static void WriteSubtree(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* root);

/*************************************************************************************************/
/**
 * \brief Writes top-level nodes in document order.
 * https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     document   Document to write
 *
 * \retval None
 */
static void WriteDocument(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Document_t* document);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static const Utl_Xml_Node_t* FindNode(const Utl_Xml_Document_t* document, Utl_Xml_NodeType_t type)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* found_node;

    found_node = NULL;

    for (current_node = document->firstNode; NULL != current_node; current_node = current_node->nextSibling)
    {
        if (type == current_node->type)
        {
            found_node = current_node;
            break;
        }
    }

    return found_node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the output bound before copying. A zero-length span copies nothing.
 */
static void WriteBytes(byte_t* output, uint32_t capacity, uint32_t* byteCount, const byte_t* bytes, uint32_t numBytes)
{
    bool_t fits_capacity;

    LOGASSERT(NULL != output);
    LOGASSERT(NULL != byteCount);
    LOGASSERT((NULL != bytes) || (0 == numBytes));
    LOGASSERT((*byteCount) <= capacity);

    fits_capacity = (numBytes <= capacity - (*byteCount));
    LOGASSERT(fits_capacity);

    if (0 < numBytes)
    {
        memcpy(output + (*byteCount), bytes, numBytes);
    }

    (*byteCount) += numBytes;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteByte(byte_t* output, uint32_t capacity, uint32_t* byteCount, byte_t value)
{
    WriteBytes(output, capacity, byteCount, &value, 1);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the character-reference delimiters in utl_xml.h.
 */
static void WriteHexCharReference(byte_t* output, uint32_t capacity, uint32_t* byteCount, byte_t hexDigit)
{
    WriteBytes(
        output,
        capacity,
        byteCount,
        (const byte_t*)UTL_XML_CHAR_REF_HEX_OPEN,
        (uint32_t)(sizeof(UTL_XML_CHAR_REF_HEX_OPEN) - 1));
    WriteByte(output, capacity, byteCount, hexDigit);
    WriteBytes(
        output,
        capacity,
        byteCount,
        (const byte_t*)UTL_XML_REFERENCE_CLOSE,
        (uint32_t)(sizeof(UTL_XML_REFERENCE_CLOSE) - 1));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves text bytes except for XML delimiters and carriage returns.
 */
static void WriteEscaped(byte_t* output, uint32_t capacity, uint32_t* byteCount, const byte_t* utf8Text, uint32_t numBytes, bool_t isAttribute)
{
    uint32_t byte_index;
    bool_t   is_xml_string;

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
            case '&':
                WriteBytes(
                    output,
                    capacity,
                    byteCount,
                    (const byte_t*)UTL_XML_ENTITY_REF_AMP,
                    (uint32_t)(sizeof(UTL_XML_ENTITY_REF_AMP) - 1));
                break;
            case '<':
                WriteBytes(
                    output,
                    capacity,
                    byteCount,
                    (const byte_t*)UTL_XML_ENTITY_REF_LT,
                    (uint32_t)(sizeof(UTL_XML_ENTITY_REF_LT) - 1));
                break;
            case '>':
                WriteBytes(
                    output,
                    capacity,
                    byteCount,
                    (const byte_t*)UTL_XML_ENTITY_REF_GT,
                    (uint32_t)(sizeof(UTL_XML_ENTITY_REF_GT) - 1));
                break;
            case UTL_XML_QUOTE_QUOT:
                if (isAttribute)
                {
                    WriteBytes(
                        output,
                        capacity,
                        byteCount,
                        (const byte_t*)UTL_XML_ENTITY_REF_QUOT,
                        (uint32_t)(sizeof(UTL_XML_ENTITY_REF_QUOT) - 1));
                }
                else
                {
                    WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
                }
                break;
            case UTL_XML_CHAR_TAB:
                if (isAttribute)
                {
                    WriteHexCharReference(output, capacity, byteCount, '9');
                }
                else
                {
                    WriteByte(output, capacity, byteCount, UTL_XML_CHAR_TAB);
                }
                break;
            case UTL_XML_CHAR_LF:
                if (isAttribute)
                {
                    WriteHexCharReference(output, capacity, byteCount, 'A');
                }
                else
                {
                    WriteByte(output, capacity, byteCount, UTL_XML_CHAR_LF);
                }
                break;
            case UTL_XML_CHAR_CR:
                WriteHexCharReference(output, capacity, byteCount, 'D');
                break;
            default:
                WriteByte(output, capacity, byteCount, utf8Text[byte_index]);
                break;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the framing and XML characters. The caller supplies valid internal grammar.
 */
static void WriteMarkup(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node)
{
    const byte_t* open;
    const byte_t* close;
    uint32_t      open_byte_count;
    uint32_t      close_byte_count;
    bool_t        is_xml_string;
    bool_t        has_open;
    bool_t        has_close;

    if (UTL_XML_NODE_XML_DECLARATION == node->type)
    {
        open = (const byte_t*)UTL_XML_DECL_OPEN;
        close = (const byte_t*)UTL_XML_DECL_CLOSE;
        open_byte_count = (uint32_t)(sizeof(UTL_XML_DECL_OPEN) - 1);
        close_byte_count = (uint32_t)(sizeof(UTL_XML_DECL_CLOSE) - 1);
    }
    else
    {
        LOGASSERT(UTL_XML_NODE_DOCTYPE == node->type);
        open = (const byte_t*)UTL_XML_DOCTYPE_OPEN;
        close = (const byte_t*)UTL_XML_DOCTYPE_CLOSE;
        open_byte_count = (uint32_t)(sizeof(UTL_XML_DOCTYPE_OPEN) - 1);
        close_byte_count = (uint32_t)(sizeof(UTL_XML_DOCTYPE_CLOSE) - 1);
    }

    LOGASSERT(NULL == node->name);
    LOGASSERT(0 == node->nameByteCount);
    LOGASSERT(NULL == node->firstChild);
    LOGASSERT(NULL == node->lastChild);
    LOGASSERT(NULL == node->firstAttribute);
    LOGASSERT(NULL == node->lastAttribute);
    LOGASSERT(0 == node->attributeCount);
    LOGASSERT(NULL != node->value);
    LOGASSERT(open_byte_count <= node->valueByteCount);
    LOGASSERT(close_byte_count <= node->valueByteCount - open_byte_count);

    is_xml_string = Utl_Xml_IsString(node->value, node->valueByteCount);
    LOGASSERT(is_xml_string);

    has_open = (0 == memcmp(node->value, open, open_byte_count));
    has_close = (0 == memcmp(
        node->value + node->valueByteCount - close_byte_count,
        close,
        close_byte_count));
    LOGASSERT(has_open);
    LOGASSERT(has_close);

    WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attributes are appended and checked for uniqueness by utl_xml_node.
 */
static void WriteElementStart(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node)
{
    const Utl_Xml_Attribute_t* current_attribute;
    const Utl_Xml_Attribute_t* last_attribute;
    uint32_t                   attribute_index;
    bool_t                     is_name;

    LOGASSERT(NULL != node->name);
    is_name = Utl_Xml_IsName(node->name, node->nameByteCount);
    LOGASSERT(is_name);
    LOGASSERT(NULL == node->value);
    LOGASSERT(0 == node->valueByteCount);
    LOGASSERT((NULL == node->firstChild) == (NULL == node->lastChild));
    LOGASSERT(node->attributeCount <= UTL_XML_NODE_MAX_ATTRIBUTES);

    WriteBytes(
        output,
        capacity,
        byteCount,
        (const byte_t*)UTL_XML_STAG_OPEN,
        (uint32_t)(sizeof(UTL_XML_STAG_OPEN) - 1));
    WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);

    current_attribute = node->firstAttribute;
    last_attribute = NULL;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != current_attribute);
        LOGASSERT(NULL != current_attribute->name);

        is_name = Utl_Xml_IsName(current_attribute->name, current_attribute->nameByteCount);
        LOGASSERT(is_name);

        WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
        WriteBytes(
            output,
            capacity,
            byteCount,
            current_attribute->name,
            current_attribute->nameByteCount);
        WriteByte(output, capacity, byteCount, UTL_XML_EQ);
        WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);
        WriteEscaped(
            output,
            capacity,
            byteCount,
            current_attribute->value,
            current_attribute->valueByteCount,
            TRUE);
        WriteByte(output, capacity, byteCount, UTL_XML_QUOTE_QUOT);

        last_attribute = current_attribute;
        current_attribute = current_attribute->next;
    }

    LOGASSERT(NULL == current_attribute);
    LOGASSERT(last_attribute == node->lastAttribute);

    if (NULL == node->firstChild)
    {
        WriteBytes(
            output,
            capacity,
            byteCount,
            (const byte_t*)UTL_XML_EMPTY_ELEM_TAG_CLOSE,
            (uint32_t)(sizeof(UTL_XML_EMPTY_ELEM_TAG_CLOSE) - 1));
    }
    else
    {
        WriteBytes(
            output,
            capacity,
            byteCount,
            (const byte_t*)UTL_XML_TAG_CLOSE,
            (uint32_t)(sizeof(UTL_XML_TAG_CLOSE) - 1));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void WriteElementEnd(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node)
{
    WriteBytes(
        output,
        capacity,
        byteCount,
        (const byte_t*)UTL_XML_ETAG_OPEN,
        (uint32_t)(sizeof(UTL_XML_ETAG_OPEN) - 1));
    WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);
    WriteBytes(
        output,
        capacity,
        byteCount,
        (const byte_t*)UTL_XML_TAG_CLOSE,
        (uint32_t)(sizeof(UTL_XML_TAG_CLOSE) - 1));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Text is escaped; comment and processing-instruction content is written verbatim.
 */
static void WriteLeaf(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* node)
{
    uint32_t byte_index;
    bool_t   is_xml_string;
    bool_t   is_name;
    bool_t   is_reserved_target;
    bool_t   has_delimiter;

    LOGASSERT(NULL == node->firstChild);
    LOGASSERT(NULL == node->lastChild);
    LOGASSERT(NULL == node->firstAttribute);
    LOGASSERT(NULL == node->lastAttribute);
    LOGASSERT(0 == node->attributeCount);

    if (UTL_XML_NODE_TEXT == node->type)
    {
        LOGASSERT(NULL == node->name);
        LOGASSERT(0 == node->nameByteCount);
        WriteEscaped(output, capacity, byteCount, node->value, node->valueByteCount, FALSE);
    }
    else if (UTL_XML_NODE_COMMENT == node->type)
    {
        LOGASSERT(NULL == node->name);
        LOGASSERT(0 == node->nameByteCount);
        LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

        if (NULL != node->value)
        {
            is_xml_string = Utl_Xml_IsString(node->value, node->valueByteCount);
            LOGASSERT(is_xml_string);
        }

        for (byte_index = 1; byte_index < node->valueByteCount; ++byte_index)
        {
            has_delimiter = ('-' == node->value[byte_index - 1]) &&
                            ('-' == node->value[byte_index]);
            LOGASSERT(FALSE == has_delimiter);
        }

        if (0 < node->valueByteCount)
        {
            LOGASSERT('-' != node->value[node->valueByteCount - 1]);
        }

        WriteBytes(
            output,
            capacity,
            byteCount,
            (const byte_t*)UTL_XML_COMMENT_OPEN,
            (uint32_t)(sizeof(UTL_XML_COMMENT_OPEN) - 1));
        WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
        WriteBytes(
            output,
            capacity,
            byteCount,
            (const byte_t*)UTL_XML_COMMENT_CLOSE,
            (uint32_t)(sizeof(UTL_XML_COMMENT_CLOSE) - 1));
    }
    else
    {
        LOGASSERT(UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type);
        LOGASSERT(NULL != node->name);
        LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

        is_name = Utl_Xml_IsName(node->name, node->nameByteCount);
        LOGASSERT(is_name);

        is_reserved_target = (3 == node->nameByteCount) &&
                             (('x' == node->name[0]) || ('X' == node->name[0])) &&
                             (('m' == node->name[1]) || ('M' == node->name[1])) &&
                             (('l' == node->name[2]) || ('L' == node->name[2]));
        LOGASSERT(FALSE == is_reserved_target);

        if (NULL != node->value)
        {
            is_xml_string = Utl_Xml_IsString(node->value, node->valueByteCount);
            LOGASSERT(is_xml_string);
        }

        for (byte_index = 1; byte_index < node->valueByteCount; ++byte_index)
        {
            has_delimiter = ('?' == node->value[byte_index - 1]) &&
                            ('>' == node->value[byte_index]);
            LOGASSERT(FALSE == has_delimiter);
        }

        WriteBytes(
            output,
            capacity,
            byteCount,
            (const byte_t*)UTL_XML_PI_OPEN,
            (uint32_t)(sizeof(UTL_XML_PI_OPEN) - 1));
        WriteBytes(output, capacity, byteCount, node->name, node->nameByteCount);

        if (0 < node->valueByteCount)
        {
            WriteByte(output, capacity, byteCount, UTL_XML_CHAR_SPACE);
            WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
        }

        WriteBytes(
            output,
            capacity,
            byteCount,
            (const byte_t*)UTL_XML_PI_CLOSE,
            (uint32_t)(sizeof(UTL_XML_PI_CLOSE) - 1));
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Traverses element children without allocating a stack.
 */
static void WriteSubtree(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Node_t* root)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* parent_node;
    const Utl_Xml_Node_t* sibling_node;
    uint32_t              node_depth;

    current_node = root;
    node_depth = 0;

    while (NULL != current_node)
    {
        if (UTL_XML_NODE_ELEMENT == current_node->type)
        {
            WriteElementStart(output, capacity, byteCount, current_node);

            if (NULL != current_node->firstChild)
            {
                LOGASSERT(node_depth < UTL_XML_NODE_MAX_DEPTH);
                LOGASSERT(current_node->firstChild->parent == current_node);
                current_node = current_node->firstChild;
                ++node_depth;
                continue;
            }
        }
        else
        {
            WriteLeaf(output, capacity, byteCount, current_node);
        }

        while ((current_node != root) && (NULL == current_node->nextSibling))
        {
            parent_node = current_node->parent;
            LOGASSERT(NULL != parent_node);
            LOGASSERT(0 < node_depth);
            LOGASSERT(UTL_XML_NODE_ELEMENT == parent_node->type);
            LOGASSERT(parent_node->lastChild == current_node);

            current_node = parent_node;
            --node_depth;
            WriteElementEnd(output, capacity, byteCount, current_node);
        }

        if (current_node == root)
        {
            current_node = NULL;
        }
        else
        {
            sibling_node = current_node->nextSibling;
            LOGASSERT(NULL != sibling_node);
            LOGASSERT(sibling_node->parent == current_node->parent);
            current_node = sibling_node;
        }
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The document grammar allows one root element with prolog and trailing Misc.
 */
static void WriteDocument(byte_t* output, uint32_t capacity, uint32_t* byteCount, const Utl_Xml_Document_t* document)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* last_node;
    bool_t                has_root;
    bool_t                has_doctype;

    LOGASSERT(NULL != document->firstNode);
    LOGASSERT(NULL != document->lastNode);

    has_root = FALSE;
    has_doctype = FALSE;
    last_node = NULL;

    for (current_node = document->firstNode; NULL != current_node; current_node = current_node->nextSibling)
    {
        LOGASSERT(NULL == current_node->parent);

        if (UTL_XML_NODE_XML_DECLARATION == current_node->type)
        {
            LOGASSERT(current_node == document->firstNode);
            WriteMarkup(output, capacity, byteCount, current_node);
        }
        else if (UTL_XML_NODE_DOCTYPE == current_node->type)
        {
            LOGASSERT(FALSE == has_doctype);
            LOGASSERT(FALSE == has_root);
            WriteMarkup(output, capacity, byteCount, current_node);
            has_doctype = TRUE;
        }
        else if (UTL_XML_NODE_ELEMENT == current_node->type)
        {
            LOGASSERT(FALSE == has_root);
            WriteSubtree(output, capacity, byteCount, current_node);
            has_root = TRUE;
        }
        else
        {
            LOGASSERT((UTL_XML_NODE_COMMENT == current_node->type) ||
                      (UTL_XML_NODE_PROCESSING_INSTRUCTION == current_node->type));
            WriteLeaf(output, capacity, byteCount, current_node);
        }

        last_node = current_node;
    }

    LOGASSERT(has_root);
    LOGASSERT(last_node == document->lastNode);
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Document_Initialize(Utl_Xml_Document_t* document)
{
    LOGASSERT(NULL != document);

    document->firstNode = NULL;
    document->lastNode = NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Links one unlinked node at the end of the top-level list.
 */
void Utl_Xml_Document_AppendNode(Utl_Xml_Document_t* document, Utl_Xml_Node_t* node)
{
    const Utl_Xml_Node_t* found_node;
    bool_t                is_element;
    bool_t                is_misc;

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
        is_element = (UTL_XML_NODE_ELEMENT == node->type);
        is_misc = (UTL_XML_NODE_COMMENT == node->type) ||
                  (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type);
        LOGASSERT(is_element || is_misc);

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
 *  Writes the document once and asserts before any write that exceeds capacity.
 */
void Utl_Xml_Document_Serialize(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity, uint32_t* byteCount)
{
    LOGASSERT(NULL != document);
    LOGASSERT(NULL != output);
    LOGASSERT(NULL != byteCount);

    (*byteCount) = 0;
    WriteDocument(output, capacity, byteCount, document);
}
