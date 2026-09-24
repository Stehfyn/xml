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
static void WriteBytes(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* bytes, uint32_t numBytes);

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
static void WriteByte(byte_t* output, uint32_t capacity, uint64_t* byteCount, byte_t value);

/*************************************************************************************************/
/**
 * \brief Writes a null-terminated XML token.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     token      XML token from utl_xml.h
 *
 * \retval None
 */
static void WriteToken(byte_t* output, uint32_t capacity, uint64_t* byteCount, const char_t* token);

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
 * \retval None
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
 * \retval None
 */
static void WriteDeclaration(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

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
static void WriteHexCharReference(byte_t* output, uint32_t capacity, uint64_t* byteCount, byte_t hexDigit);

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
static void WriteEscaped(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* utf8Text, uint32_t numBytes, bool_t isAttribute);

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
 * \retval None
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
 * \retval None
 */
static void WriteElementEnd(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

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
 * \retval None
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
 * \retval None
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
 * \retval None
 */
static void WriteProcessingInstruction(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes an element and its descendants without recursion.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     root       Root element to write
 *
 * \retval None
 */
static void WriteElement(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* root);

/*************************************************************************************************/
/**
 * \brief Writes a text, comment, or processing-instruction node.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Node to write
 *
 * \retval None
 */
static void WriteNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Writes one top-level XML node, including an element's descendants.
 *
 * \param[in,out] output     Caller-provided byte storage
 * \param[in]     capacity   Number of bytes in output
 * \param[in,out] byteCount  Number of bytes already written
 * \param[in]     node       Node to write
 *
 * \retval None
 */
static void WriteTopLevelNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node);

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
 *  Checks the output bound before copying. A zero-length span copies nothing.
 */
static void WriteBytes(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* bytes, uint32_t numBytes)
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
        (void)memcpy(output + (*byteCount), bytes, numBytes);
    }

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
 *  The caller supplies an ExternalID, an internal subset, or both in value,
 *  without a leading space.
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
 *  Uses the character-reference delimiters in utl_xml.h.
 */
static void WriteHexCharReference(byte_t* output, uint32_t capacity, uint64_t* byteCount, byte_t hexDigit)
{
    WriteToken(output, capacity, byteCount, UTL_XML_CHAR_REF_HEX_OPEN);
    WriteByte(output, capacity, byteCount, hexDigit);
    WriteToken(output, capacity, byteCount, UTL_XML_REFERENCE_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Preserves text bytes except for XML delimiters and carriage returns.
 */
static void WriteEscaped(byte_t* output, uint32_t capacity, uint64_t* byteCount, const byte_t* utf8Text, uint32_t numBytes, bool_t isAttribute)
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
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_AMP);
                break;
            case '<':
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_LT);
                break;
            case '>':
                WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_GT);
                break;
            case UTL_XML_QUOTE_QUOT:
                if (isAttribute)
                {
                    WriteToken(output, capacity, byteCount, UTL_XML_ENTITY_REF_QUOT);
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
 *  Attributes are appended and checked for uniqueness by utl_xml_node.
 */
static void WriteElementStart(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    const Utl_Xml_Attribute_t* current_attribute;
    bool_t                     is_name;
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
        WriteEscaped(output, capacity, byteCount, current_attribute->value, current_attribute->valueByteCount, TRUE);
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
 *  Escapes the text without adding markup.
 */
static void WriteText(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    LOGASSERT(NULL == node->name);
    LOGASSERT(0 == node->nameByteCount);
    WriteEscaped(output, capacity, byteCount, node->value, node->valueByteCount, FALSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks content with Utl_Xml_IsCommentContent.
 */
static void WriteComment(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
{
    bool_t is_comment_content;

    LOGASSERT(NULL == node->name);
    LOGASSERT(0 == node->nameByteCount);
    LOGASSERT((NULL != node->value) || (0 == node->valueByteCount));

    if (NULL != node->value)
    {
        is_comment_content = Utl_Xml_IsCommentContent(node->value, node->valueByteCount);
        LOGASSERT(is_comment_content);
    }

    WriteToken(output, capacity, byteCount, UTL_XML_COMMENT_OPEN);
    WriteBytes(output, capacity, byteCount, node->value, node->valueByteCount);
    WriteToken(output, capacity, byteCount, UTL_XML_COMMENT_CLOSE);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the target and data with the XML 1.0 validators.
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
        is_pi_data = Utl_Xml_IsPIData(node->value, node->valueByteCount);
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
 *  Traverses element children without allocating a stack.
 */
static void WriteElement(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* root)
{
    const Utl_Xml_Node_t* current_node;
    uint32_t              node_index;
    uint32_t              ancestor_index;
    bool_t                should_close_parent;

    current_node = root;

    for (node_index = 0; node_index < UTL_XML_DOCUMENT_MAX_NODES; ++node_index)
    {
        if (NULL == current_node)
        {
            break;
        }

        if (UTL_XML_NODE_ELEMENT == current_node->type)
        {
            WriteElementStart(output, capacity, byteCount, current_node);

            if (NULL != current_node->firstChild)
            {
                current_node = current_node->firstChild;
                continue;
            }
        }
        else
        {
            WriteNode(output, capacity, byteCount, current_node);
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
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Dispatches text, comment, and processing-instruction nodes.
 */
static void WriteNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
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
static void WriteTopLevelNode(byte_t* output, uint32_t capacity, uint64_t* byteCount, const Utl_Xml_Node_t* node)
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
void Utl_Xml_Document_Initialize(Utl_Xml_Document_t* document)
{
    LOGASSERT(NULL != document);

    document->firstNode = NULL;
    document->lastNode  = NULL;
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
uint64_t Utl_Xml_Document_Write(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity)
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

        WriteTopLevelNode(output, capacity, &byte_count, current_node);

        current_node = current_node->nextSibling;
    }

    LOGASSERT(NULL == current_node);

    return byte_count;
}
