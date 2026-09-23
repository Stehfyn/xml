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

typedef struct XmlOutput_s
{
    byte_t*  bytes;
    uint32_t capacity;
    uint32_t length;
} XmlOutput_t;

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

/*************************************************************************************************/
/**
 * \brief Appends bytes, or measures them when output storage is NULL.
 */
static bool_t WriteBytes(XmlOutput_t* output, const byte_t* bytes, uint32_t byteCount)
{
    bool_t can_write;

    can_write = (byteCount <= UINT32_MAX - output->length);

    if (can_write && (NULL != output->bytes))
    {
        can_write = (byteCount <= output->capacity - output->length);

        if (can_write && (0 < byteCount))
        {
            memcpy(output->bytes + output->length, bytes, byteCount);
        }
    }

    if (can_write)
    {
        output->length += byteCount;
    }

    return can_write;
}

/*************************************************************************************************/
/**
 * \brief Appends one byte.
 */
static bool_t WriteByte(XmlOutput_t* output, byte_t value)
{
    return WriteBytes(output, &value, 1);
}

/*************************************************************************************************/
/**
 * \brief Escapes valid XML character data or a double-quoted attribute value.
 */
static bool_t WriteEscaped(XmlOutput_t* output, const byte_t* utf8Text, uint32_t byteCount, bool_t isAttribute)
{
    uint32_t byte_index;
    bool_t   is_valid;
    bool_t   is_written;

    is_valid = (NULL != utf8Text);

    if (is_valid)
    {
        is_valid = Utl_Xml_IsString(utf8Text, byteCount);
    }
    else
    {
        is_valid = (0 == byteCount);
    }

    if (FALSE == is_valid)
    {
        return FALSE;
    }

    for (byte_index = 0; byte_index < byteCount; ++byte_index)
    {
        switch (utf8Text[byte_index])
        {
            case '&':
                is_written = WriteBytes(output, (const byte_t*)UTL_XML_ENTITY_REF_AMP,
                                        (uint32_t)(sizeof(UTL_XML_ENTITY_REF_AMP) - 1));
                break;
            case '<':
                is_written = WriteBytes(output, (const byte_t*)UTL_XML_ENTITY_REF_LT,
                                        (uint32_t)(sizeof(UTL_XML_ENTITY_REF_LT) - 1));
                break;
            case '>':
                is_written = WriteBytes(output, (const byte_t*)UTL_XML_ENTITY_REF_GT,
                                        (uint32_t)(sizeof(UTL_XML_ENTITY_REF_GT) - 1));
                break;
            case '"':
                is_written = isAttribute ? WriteBytes(output, (const byte_t*)UTL_XML_ENTITY_REF_QUOT,
                                                      (uint32_t)(sizeof(UTL_XML_ENTITY_REF_QUOT) - 1)) : WriteByte(output, '"');
                break;
            case '\t':
                is_written = isAttribute ? WriteBytes(output, (const byte_t*)"&#x9;", (uint32_t)(sizeof("&#x9;") - 1)) :
                                           WriteByte(output, '\t');
                break;
            case '\n':
                is_written = isAttribute ? WriteBytes(output, (const byte_t*)"&#xA;", (uint32_t)(sizeof("&#xA;") - 1)) :
                                           WriteByte(output, '\n');
                break;
            case '\r':
                is_written = WriteBytes(output, (const byte_t*)"&#xD;", (uint32_t)(sizeof("&#xD;") - 1));
                break;
            default:
                is_written = WriteByte(output, utf8Text[byte_index]);
                break;
        }

        if (FALSE == is_written)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \brief Validates the byte span and framing of caller-supplied raw markup.
 */
static bool_t WriteRawMarkup(XmlOutput_t* output, const byte_t* markup, uint32_t byteCount, const byte_t* prefix, uint32_t prefixByteCount, const byte_t* suffix, uint32_t suffixByteCount)
{
    bool_t is_valid;

    is_valid = (NULL != markup) && (prefixByteCount <= byteCount) &&
               (suffixByteCount <= byteCount - prefixByteCount);

    if (is_valid)
    {
        is_valid = Utl_Xml_IsString(markup, byteCount) &&
                   (0 == memcmp(markup, prefix, prefixByteCount)) &&
                   (0 == memcmp(markup + byteCount - suffixByteCount, suffix, suffixByteCount));
    }

    if (is_valid)
    {
        is_valid = WriteBytes(output, markup, byteCount);
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \brief Writes a previously recorded XML declaration.
 */
static bool_t WriteXmlDeclaration(XmlOutput_t* output, const Utl_Xml_Document_t* document)
{
    static const byte_t prefix[] = "<?xml";
    static const byte_t suffix[] = "?>";

    return WriteRawMarkup(output, document->xmlDeclaration, document->xmlDeclarationByteCount,
                          prefix, (uint32_t)(sizeof(prefix) - 1), suffix, (uint32_t)(sizeof(suffix) - 1));
}

/*************************************************************************************************/
/**
 * \brief Writes a previously recorded DOCTYPE declaration.
 */
static bool_t WriteDoctype(XmlOutput_t* output, const Utl_Xml_Document_t* document)
{
    static const byte_t prefix[] = "<!DOCTYPE";
    static const byte_t suffix[] = ">";

    return WriteRawMarkup(output, document->doctype, document->doctypeByteCount,
                          prefix, (uint32_t)(sizeof(prefix) - 1), suffix, (uint32_t)(sizeof(suffix) - 1));
}

/*************************************************************************************************/
/**
 * \brief Writes and validates an element's opening tag and attributes.
 */
static bool_t WriteElementStart(XmlOutput_t* output, const Utl_Xml_Node_t* node)
{
    const Utl_Xml_Attribute_t* current_attribute;
    const Utl_Xml_Attribute_t* previous_attribute;
    const Utl_Xml_Attribute_t* last_attribute;
    uint32_t                   attribute_index;
    bool_t                     has_same_name_byte_count;
    bool_t                     has_same_name;

    if ((NULL == node->name) || (FALSE == Utl_Xml_IsName(node->name, node->nameByteCount)) ||
        (NULL != node->value) || (0 != node->valueByteCount) ||
        ((NULL == node->firstChild) != (NULL == node->lastChild)) ||
        (UTL_XML_NODE_MAX_ATTRIBUTES < node->attributeCount))
    {
        return FALSE;
    }

    if ((FALSE == WriteByte(output, '<')) ||
        (FALSE == WriteBytes(output, node->name, node->nameByteCount)))
    {
        return FALSE;
    }

    current_attribute = node->firstAttribute;
    last_attribute = NULL;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        if ((NULL == current_attribute) || (NULL == current_attribute->name) ||
            (FALSE == Utl_Xml_IsName(current_attribute->name, current_attribute->nameByteCount)))
        {
            return FALSE;
        }

        for (previous_attribute = node->firstAttribute;
             previous_attribute != current_attribute;
             previous_attribute = previous_attribute->next)
        {
            if (NULL == previous_attribute)
            {
                return FALSE;
            }

            has_same_name_byte_count = (previous_attribute->nameByteCount == current_attribute->nameByteCount);

            if (has_same_name_byte_count)
            {
                has_same_name = (0 == memcmp(previous_attribute->name, current_attribute->name,
                                             current_attribute->nameByteCount));

                if (has_same_name)
                {
                    return FALSE;
                }
            }
        }

        if ((FALSE == WriteByte(output, ' ')) ||
            (FALSE == WriteBytes(output, current_attribute->name, current_attribute->nameByteCount)) ||
            (FALSE == WriteByte(output, UTL_XML_EQ)) ||
            (FALSE == WriteByte(output, UTL_XML_QUOTE_QUOT)) ||
            (FALSE == WriteEscaped(output, current_attribute->value, current_attribute->valueByteCount, TRUE)) ||
            (FALSE == WriteByte(output, '"')))
        {
            return FALSE;
        }

        last_attribute = current_attribute;
        current_attribute = current_attribute->next;
    }

    if ((NULL != current_attribute) || (last_attribute != node->lastAttribute))
    {
        return FALSE;
    }

    if (NULL == node->firstChild)
    {
        return WriteBytes(output, (const byte_t*)UTL_XML_EMPTY_ELEM_TAG_CLOSE,
                          (uint32_t)(sizeof(UTL_XML_EMPTY_ELEM_TAG_CLOSE) - 1));
    }

    return WriteByte(output, '>');
}

/*************************************************************************************************/
/**
 * \brief Writes an element's closing tag after its children.
 */
static bool_t WriteElementEnd(XmlOutput_t* output, const Utl_Xml_Node_t* node)
{
    return WriteBytes(output, (const byte_t*)UTL_XML_ETAG_OPEN, (uint32_t)(sizeof(UTL_XML_ETAG_OPEN) - 1)) &&
           WriteBytes(output, node->name, node->nameByteCount) &&
           WriteByte(output, '>');
}

/*************************************************************************************************/
/**
 * \brief Writes a text, comment, or processing-instruction node.
 */
static bool_t WriteLeaf(XmlOutput_t* output, const Utl_Xml_Node_t* node)
{
    uint32_t byte_index;
    bool_t   is_valid;
    bool_t   is_reserved_target;

    if ((NULL != node->firstChild) || (NULL != node->lastChild) ||
        (NULL != node->firstAttribute) || (NULL != node->lastAttribute) ||
        (0 != node->attributeCount))
    {
        return FALSE;
    }

    if (UTL_XML_NODE_TEXT == node->type)
    {
        if ((NULL != node->name) || (0 != node->nameByteCount))
        {
            return FALSE;
        }

        return WriteEscaped(output, node->value, node->valueByteCount, FALSE);
    }

    is_valid = (NULL != node->value);

    if (is_valid)
    {
        is_valid = Utl_Xml_IsString(node->value, node->valueByteCount);
    }
    else
    {
        is_valid = (0 == node->valueByteCount);
    }

    if (FALSE == is_valid)
    {
        return FALSE;
    }

    if (UTL_XML_NODE_COMMENT == node->type)
    {
        if ((NULL != node->name) || (0 != node->nameByteCount))
        {
            return FALSE;
        }

        for (byte_index = 1; byte_index < node->valueByteCount; ++byte_index)
        {
            if (('-' == node->value[byte_index - 1]) && ('-' == node->value[byte_index]))
            {
                return FALSE;
            }
        }

        if ((0 < node->valueByteCount) && ('-' == node->value[node->valueByteCount - 1]))
        {
            return FALSE;
        }

        return WriteBytes(output, (const byte_t*)UTL_XML_COMMENT_OPEN, (uint32_t)(sizeof(UTL_XML_COMMENT_OPEN) - 1)) &&
               WriteBytes(output, node->value, node->valueByteCount) &&
               WriteBytes(output, (const byte_t*)UTL_XML_COMMENT_CLOSE, (uint32_t)(sizeof(UTL_XML_COMMENT_CLOSE) - 1));
    }

    if (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type)
    {
        if ((NULL == node->name) || (FALSE == Utl_Xml_IsName(node->name, node->nameByteCount)))
        {
            return FALSE;
        }

        is_reserved_target = (3 == node->nameByteCount) &&
                             (('x' == node->name[0]) || ('X' == node->name[0])) &&
                             (('m' == node->name[1]) || ('M' == node->name[1])) &&
                             (('l' == node->name[2]) || ('L' == node->name[2]));

        if (is_reserved_target)
        {
            return FALSE;
        }

        for (byte_index = 1; byte_index < node->valueByteCount; ++byte_index)
        {
            if (('?' == node->value[byte_index - 1]) && ('>' == node->value[byte_index]))
            {
                return FALSE;
            }
        }

        if ((FALSE == WriteBytes(output, (const byte_t*)UTL_XML_PI_OPEN, (uint32_t)(sizeof(UTL_XML_PI_OPEN) - 1))) ||
            (FALSE == WriteBytes(output, node->name, node->nameByteCount)))
        {
            return FALSE;
        }

        if (0 < node->valueByteCount)
        {
            if ((FALSE == WriteByte(output, ' ')) ||
                (FALSE == WriteBytes(output, node->value, node->valueByteCount)))
            {
                return FALSE;
            }
        }

        return WriteBytes(output, (const byte_t*)UTL_XML_PI_CLOSE, (uint32_t)(sizeof(UTL_XML_PI_CLOSE) - 1));
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \brief Writes a subtree iteratively so allowed nesting depth does not consume call stack.
 */
static bool_t WriteSubtree(XmlOutput_t* output, const Utl_Xml_Node_t* root)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* parent_node;
    const Utl_Xml_Node_t* sibling_node;
    uint32_t              node_depth;
    bool_t                is_element;
    bool_t                is_written;

    current_node = root;
    node_depth = 0;

    for (;;)
    {
        is_element = (UTL_XML_NODE_ELEMENT == current_node->type);

        if (is_element)
        {
            is_written = WriteElementStart(output, current_node);

            if (FALSE == is_written)
            {
                return FALSE;
            }

            if (NULL != current_node->firstChild)
            {
                if ((UTL_XML_NODE_MAX_DEPTH <= node_depth) ||
                    (current_node->firstChild->parent != current_node))
                {
                    return FALSE;
                }

                current_node = current_node->firstChild;
                ++node_depth;
                continue;
            }
        }
        else if (FALSE == WriteLeaf(output, current_node))
        {
            return FALSE;
        }

        while ((current_node != root) && (NULL == current_node->nextSibling))
        {
            parent_node = current_node->parent;

            if ((NULL == parent_node) || (UTL_XML_NODE_ELEMENT != parent_node->type))
            {
                return FALSE;
            }

            if (parent_node->lastChild != current_node)
            {
                return FALSE;
            }

            current_node = parent_node;
            --node_depth;

            if (FALSE == WriteElementEnd(output, current_node))
            {
                return FALSE;
            }
        }

        if (current_node == root)
        {
            break;
        }

        sibling_node = current_node->nextSibling;

        if ((NULL == sibling_node) || (sibling_node->parent != current_node->parent))
        {
            return FALSE;
        }

        current_node = sibling_node;
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \brief Writes the declaration, ordered top-level nodes, and DOCTYPE at its recorded position.
 */
static bool_t WriteDocument(XmlOutput_t* output, const Utl_Xml_Document_t* document)
{
    const Utl_Xml_Node_t* current_node;
    const Utl_Xml_Node_t* last_node;
    bool_t                is_root;
    bool_t                is_misc;
    bool_t                has_root;
    bool_t                has_doctype;
    bool_t                wrote_doctype;

    if ((NULL == document->root) || (NULL == document->firstNode) || (NULL == document->lastNode))
    {
        return FALSE;
    }

    if (NULL != document->xmlDeclaration)
    {
        if (FALSE == WriteXmlDeclaration(output, document))
        {
            return FALSE;
        }
    }
    else if (0 != document->xmlDeclarationByteCount)
    {
        return FALSE;
    }

    has_doctype = (NULL != document->doctype);
    wrote_doctype = (FALSE == has_doctype);

    if (has_doctype && (NULL == document->nodeBeforeDoctype))
    {
        if (FALSE == WriteDoctype(output, document))
        {
            return FALSE;
        }

        wrote_doctype = TRUE;
    }
    else if ((FALSE == has_doctype) &&
             ((0 != document->doctypeByteCount) || (NULL != document->nodeBeforeDoctype)))
    {
        return FALSE;
    }

    last_node = NULL;
    has_root = FALSE;

    for (current_node = document->firstNode; NULL != current_node; current_node = current_node->nextSibling)
    {
        if (NULL != current_node->parent)
        {
            return FALSE;
        }

        is_root = (UTL_XML_NODE_ELEMENT == current_node->type);
        is_misc = (UTL_XML_NODE_COMMENT == current_node->type) ||
                  (UTL_XML_NODE_PROCESSING_INSTRUCTION == current_node->type);

        if (is_root)
        {
            if (has_root || (current_node != document->root))
            {
                return FALSE;
            }

            has_root = TRUE;
        }
        else if (FALSE == is_misc)
        {
            return FALSE;
        }

        if (FALSE == WriteSubtree(output, current_node))
        {
            return FALSE;
        }

        if (current_node == document->nodeBeforeDoctype)
        {
            if ((FALSE == has_doctype) || has_root || wrote_doctype ||
                (FALSE == WriteDoctype(output, document)))
            {
                return FALSE;
            }

            wrote_doctype = TRUE;
        }

        last_node = current_node;
    }

    return has_root && wrote_doctype && (last_node == document->lastNode);
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

    document->xmlDeclaration = NULL;
    document->xmlDeclarationByteCount = 0;
    document->doctype = NULL;
    document->doctypeByteCount = 0;
    document->nodeBeforeDoctype = NULL;
    document->firstNode = NULL;
    document->lastNode = NULL;
    document->root = NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Stores markup that will precede every other document item.
 */
void Utl_Xml_Document_SetXmlDeclaration(Utl_Xml_Document_t* document, const byte_t* utf8Text, uint32_t byteCount)
{
    LOGASSERT(NULL != document);
    LOGASSERT(NULL != utf8Text);
    LOGASSERT(0 < byteCount);
    LOGASSERT(NULL == document->xmlDeclaration);
    LOGASSERT(NULL == document->doctype);
    LOGASSERT(NULL == document->firstNode);

    document->xmlDeclaration = utf8Text;
    document->xmlDeclarationByteCount = byteCount;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Records the last prolog node so later nodes follow the DOCTYPE.
 */
void Utl_Xml_Document_SetDoctype(Utl_Xml_Document_t* document, const byte_t* utf8Text, uint32_t byteCount)
{
    LOGASSERT(NULL != document);
    LOGASSERT(NULL != utf8Text);
    LOGASSERT(0 < byteCount);
    LOGASSERT(NULL == document->doctype);
    LOGASSERT(NULL == document->root);

    document->doctype = utf8Text;
    document->doctypeByteCount = byteCount;
    document->nodeBeforeDoctype = document->lastNode;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Appends one node and records the sole element as root.
 */
void Utl_Xml_Document_AppendNode(Utl_Xml_Document_t* document, Utl_Xml_Node_t* node)
{
    bool_t is_element;
    bool_t is_misc;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != node);
    LOGASSERT(NULL == node->parent);
    LOGASSERT(NULL == node->nextSibling);
    LOGASSERT(document->lastNode != node);

    is_element = (UTL_XML_NODE_ELEMENT == node->type);
    is_misc = (UTL_XML_NODE_COMMENT == node->type) ||
              (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type);
    LOGASSERT(is_element || is_misc);

    if (is_element)
    {
        LOGASSERT(NULL == document->root);
        document->root = node;
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
 *  Measures and validates before writing, so invalid input or insufficient capacity
 *  does not modify the caller's output buffer.
 */
bool_t Utl_Xml_Document_Serialize(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity, uint32_t* byteCount)
{
    XmlOutput_t measured_output;
    XmlOutput_t written_output;
    bool_t      is_valid;

    LOGASSERT(NULL != document);
    LOGASSERT(NULL != byteCount);
    LOGASSERT((NULL != output) || (0 == capacity));

    (*byteCount) = 0;
    measured_output.bytes = NULL;
    measured_output.capacity = 0;
    measured_output.length = 0;

    is_valid = WriteDocument(&measured_output, document);

    if (FALSE == is_valid)
    {
        return FALSE;
    }

    (*byteCount) = measured_output.length;

    if (NULL == output)
    {
        return TRUE;
    }

    if (capacity < measured_output.length)
    {
        return FALSE;
    }

    written_output.bytes = output;
    written_output.capacity = capacity;
    written_output.length = 0;

    is_valid = WriteDocument(&written_output, document);

    if ((FALSE == is_valid) || (written_output.length != measured_output.length))
    {
        (*byteCount) = 0;
        return FALSE;
    }

    return TRUE;
}
