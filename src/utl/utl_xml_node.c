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

#include "utl/utl_xml_node.h"
#include "utl/utl_xml.h"

#include "os/os_assert.h"

#include <stdint.h>
#include <stdlib.h>
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
 * \brief Finds a null terminator within a documented byte limit.
 *
 * \param[in]  text       Non-NULL null-terminated UTF-8 string
 * \param[in]  maximum    Maximum permitted bytes before the terminator
 * \param[out] length     Number of bytes before the terminator
 *
 * \retval TRUE    if the string fits within maximum bytes.
 * \retval FALSE   otherwise
 */
static bool_t GetBoundedLength(const char_t* text, uint32_t maximum, uint32_t* length);

/*************************************************************************************************/
/**
 * \brief Decodes one strictly encoded UTF-8 code point within a byte span.
 *
 * \param[in]     bytes       Non-NULL UTF-8 byte span
 * \param[in]     numBytes    Number of bytes in the span
 * \param[in,out] offset      Current byte offset, advanced on success
 * \param[out]    codePoint   Decoded code point
 *
 * \retval TRUE    if one code point was decoded.
 * \retval FALSE   otherwise
 */
static bool_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t* offset, uint32_t* codePoint);

/*************************************************************************************************/
/**
 * \brief Checks a null-terminated UTF-8 string against XML 1.0 Char.
 */
static bool_t IsXmlString(const char_t* text);

/*************************************************************************************************/
/**
 * \brief Checks a null-terminated UTF-8 string against XML 1.0 Name.
 */
static bool_t IsName(const char_t* text);

/*************************************************************************************************/
/**
 * \brief Checks whether comment text can occur between XML comment delimiters.
 */
static bool_t IsComment(const char_t* text);

/*************************************************************************************************/
/**
 * \brief Checks an XML processing-instruction target and data.
 */
static bool_t IsProcessingInstruction(const char_t* target, const char_t* data);

/*************************************************************************************************/
/**
 * \brief Allocates a copy of a null-terminated string.
 */
static char_t* CopyString(const char_t* text, uint32_t maximum);

/*************************************************************************************************/
/**
 * \brief Allocates a detached node with copied name and value.
 */
static Utl_Xml_Node_t* AllocateNode(Utl_Xml_NodeType_t type, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Frees one node and its owned fields after its children have been freed.
 */
static void FreeNode(Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Finds an element attribute by case-sensitive name.
 */
static Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Checks whether candidate is node or an ancestor of node.
 */
static bool_t IsAncestor(const Utl_Xml_Node_t* candidate, const Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Finds the root of a bounded connected tree.
 */
static Utl_Xml_Node_t* FindRoot(Utl_Xml_Node_t* node);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks at most maximum + 1 bytes, including the terminator position.
 */
static bool_t GetBoundedLength(const char_t* text, uint32_t maximum, uint32_t* length)
{
    uint32_t index;

    for (index = 0; index <= maximum; ++index)
    {
        if ('\0' == text[index])
        {
            (*length) = index;
            return TRUE;
        }
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rejects truncated sequences, overlong forms, surrogates, and code points above U+10FFFF.
 */
static bool_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t* offset, uint32_t* codePoint)
{
    byte_t        first;
    uint32_t             decoded;
    uint32_t             minimum;
    uint32_t             num_continuations;
    uint32_t             index;
    uint32_t             position;

    if ((*offset) >= numBytes)
    {
        return FALSE;
    }

    position = (*offset);
    first    = bytes[position];

    if (first <= 0x7F)
    {
        decoded           = first;
        minimum           = 0;
        num_continuations = 0;
    }
    else if ((0xC2 <= first) && (first <= 0xDF))
    {
        decoded           = first & 0x1F;
        minimum           = 0x80;
        num_continuations = 1;
    }
    else if ((0xE0 <= first) && (first <= 0xEF))
    {
        decoded           = first & 0x0F;
        minimum           = 0x800;
        num_continuations = 2;
    }
    else if ((0xF0 <= first) && (first <= 0xF4))
    {
        decoded           = first & 0x07;
        minimum           = 0x10000;
        num_continuations = 3;
    }
    else
    {
        return FALSE;
    }

    for (index = 0; index < num_continuations; ++index)
    {
        ++position;

        if ((position >= numBytes) || (0x80 != (bytes[position] & 0xC0)))
        {
            return FALSE;
        }

        decoded = (decoded << 6) | (bytes[position] & 0x3F);
    }

    if ((decoded < minimum) || (0x10FFFF < decoded) || ((0xD800 <= decoded) && (decoded <= 0xDFFF)))
    {
        return FALSE;
    }

    (*codePoint) = decoded;
    (*offset)    = position + 1;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Empty strings are permitted as text and attribute values.
 */
static bool_t IsXmlString(const char_t* text)
{
    const byte_t* bytes;
    uint32_t      byte_count;
    uint32_t      byte_offset;
    uint32_t      code_point;
    uint32_t      code_point_index;

    if (FALSE == GetBoundedLength(text, UTL_XML_NODE_MAX_VALUE_BYTES, &byte_count))
    {
        return FALSE;
    }

    bytes       = (const byte_t*)text;
    byte_offset = 0;

    for (code_point_index = 0; (code_point_index < byte_count) && (byte_offset < byte_count); ++code_point_index)
    {
        if (FALSE == DecodeUtf8(bytes, byte_count, &byte_offset, &code_point))
        {
            return FALSE;
        }

        if (FALSE == Utl_Xml_IsChar(code_point))
        {
            return FALSE;
        }
    }

    return (byte_offset == byte_count);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the existing XML 1.0 NameStartChar and NameChar classifiers.
 */
static bool_t IsName(const char_t* text)
{
    const byte_t* bytes;
    uint32_t      byte_count;
    uint32_t      byte_offset;
    uint32_t      code_point;
    uint32_t      code_point_index;

    if (FALSE == GetBoundedLength(text, UTL_XML_NODE_MAX_NAME_BYTES, &byte_count))
    {
        return FALSE;
    }

    if (0 == byte_count)
    {
        return FALSE;
    }

    bytes       = (const byte_t*)text;
    byte_offset = 0;

    for (code_point_index = 0; (code_point_index < byte_count) && (byte_offset < byte_count); ++code_point_index)
    {
        if (FALSE == DecodeUtf8(bytes, byte_count, &byte_offset, &code_point))
        {
            return FALSE;
        }

        if (0 == code_point_index)
        {
            if (FALSE == Utl_Xml_IsNameStartChar(code_point))
            {
                return FALSE;
            }
        }
        else if (FALSE == Utl_Xml_IsNameChar(code_point))
        {
            return FALSE;
        }
    }

    return (byte_offset == byte_count);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML 1.0 production [15] disallows "--" within the content and a final '-'.
 */
static bool_t IsComment(const char_t* text)
{
    uint32_t length;
    uint32_t index;

    if (FALSE == GetBoundedLength(text, UTL_XML_NODE_MAX_VALUE_BYTES, &length))
    {
        return FALSE;
    }

    if (FALSE == IsXmlString(text))
    {
        return FALSE;
    }

    for (index = 0; index + 1 < length; ++index)
    {
        if (('-' == text[index]) && ('-' == text[index + 1]))
        {
            return FALSE;
        }
    }

    return (0 == length) || ('-' != text[length - 1]);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML 1.0 productions [16] and [17] reserve all case spellings of "xml" as PI targets.
 */
static bool_t IsProcessingInstruction(const char_t* target, const char_t* data)
{
    bool_t   is_xml_target;
    uint32_t target_length;
    uint32_t data_length;
    uint32_t index;

    if (FALSE == IsName(target))
    {
        return FALSE;
    }

    if (FALSE == GetBoundedLength(target, UTL_XML_NODE_MAX_NAME_BYTES, &target_length))
    {
        return FALSE;
    }

    is_xml_target = (3 == target_length) &&
                    (('x' == target[0]) || ('X' == target[0])) &&
                    (('m' == target[1]) || ('M' == target[1])) &&
                    (('l' == target[2]) || ('L' == target[2]));

    if (is_xml_target)
    {
        return FALSE;
    }

    if (FALSE == GetBoundedLength(data, UTL_XML_NODE_MAX_VALUE_BYTES, &data_length))
    {
        return FALSE;
    }

    if (FALSE == IsXmlString(data))
    {
        return FALSE;
    }

    for (index = 0; index + 1 < data_length; ++index)
    {
        if (('?' == data[index]) && ('>' == data[index + 1]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A NULL return indicates allocation failure.
 */
static char_t* CopyString(const char_t* text, uint32_t maximum)
{
    char_t*  copy;
    uint32_t length;

    if (FALSE == GetBoundedLength(text, maximum, &length))
    {
        return NULL;
    }

    copy = (char_t*)malloc((size_t)length + 1u);

    if (NULL != copy)
    {
        (void)memcpy(copy, text, (size_t)length + 1u);
    }

    return copy;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller validates name and value before allocating.
 */
static Utl_Xml_Node_t* AllocateNode(Utl_Xml_NodeType_t type, const char_t* name, const char_t* value)
{
    Utl_Xml_Node_t* node;

    node = (Utl_Xml_Node_t*)calloc(1, sizeof((*node)));

    if (NULL == node)
    {
        return NULL;
    }

    node->type         = type;
    node->subtreeCount = 1;

    if (NULL != name)
    {
        node->name = CopyString(name, UTL_XML_NODE_MAX_NAME_BYTES);

        if (NULL == node->name)
        {
            free(node);
            return NULL;
        }
    }

    if (NULL != value)
    {
        node->value = CopyString(value, UTL_XML_NODE_MAX_VALUE_BYTES);

        if (NULL == node->value)
        {
            free(node->name);
            free(node);
            return NULL;
        }
    }

    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Child nodes are freed separately by Utl_Xml_Node_Destroy.
 */
static void FreeNode(Utl_Xml_Node_t* node)
{
    Utl_Xml_Attribute_t* attribute;
    Utl_Xml_Attribute_t* next_attribute;
    uint32_t             attribute_index;

    attribute = node->firstAttribute;
    LOGASSERT(node->attributeCount <= UTL_XML_NODE_MAX_ATTRIBUTES);

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != attribute);
        next_attribute = attribute->next;
        free(attribute->name);
        free(attribute->value);
        free(attribute);
        attribute = next_attribute;
    }

    LOGASSERT(NULL == attribute);
    free(node->name);
    free(node->value);
    free(node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attribute names are compared without case folding or normalization.
 */
static Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* attribute;
    uint32_t             attribute_index;

    attribute = node->firstAttribute;
    LOGASSERT(node->attributeCount <= UTL_XML_NODE_MAX_ATTRIBUTES);

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != attribute);

        if (0 == strncmp(attribute->name, name, UTL_XML_NODE_MAX_NAME_BYTES + 1u))
        {
            return attribute;
        }

        attribute = attribute->next;
    }

    LOGASSERT(NULL == attribute);
    return NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A node cannot be inserted under itself or under one of its descendants.
 */
static bool_t IsAncestor(const Utl_Xml_Node_t* candidate, const Utl_Xml_Node_t* node)
{
    const Utl_Xml_Node_t* current;
    uint32_t              node_index;

    current = node;

    for (node_index = 0; (node_index < UTL_XML_NODE_MAX_NODES) && (NULL != current); ++node_index)
    {
        if (candidate == current)
        {
            return TRUE;
        }

        current = current->parent;
    }

    LOGASSERT(NULL == current);
    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The public tree limit bounds every parent traversal.
 */
static Utl_Xml_Node_t* FindRoot(Utl_Xml_Node_t* node)
{
    Utl_Xml_Node_t* current;
    uint32_t        node_index;

    current = node;

    for (node_index = 0; node_index < UTL_XML_NODE_MAX_NODES; ++node_index)
    {
        if (NULL == current->parent)
        {
            return current;
        }

        current = current->parent;
    }

    LOGASSERT(FALSE);
    return NULL;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateElement(const char_t* name)
{
    LOGASSERT(NULL != name);
    return (TRUE == IsName(name)) ? AllocateNode(UTL_XML_NODE_ELEMENT, name, NULL) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateText(const char_t* text)
{
    LOGASSERT(NULL != text);
    return (TRUE == IsXmlString(text)) ? AllocateNode(UTL_XML_NODE_TEXT, NULL, text) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateComment(const char_t* text)
{
    LOGASSERT(NULL != text);
    return (TRUE == IsComment(text)) ? AllocateNode(UTL_XML_NODE_COMMENT, NULL, text) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateProcessingInstruction(const char_t* target, const char_t* data)
{
    LOGASSERT(NULL != target);
    LOGASSERT(NULL != data);
    return (TRUE == IsProcessingInstruction(target, data)) ? AllocateNode(UTL_XML_NODE_PROCESSING_INSTRUCTION, target, data) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks to the first remaining child, then frees leaves upward without recursive calls.
 */
void Utl_Xml_Node_Destroy(Utl_Xml_Node_t* node)
{
    Utl_Xml_Node_t* current;
    Utl_Xml_Node_t* parent;
    uint32_t        node_count;
    uint32_t        visit_index;

    LOGASSERT(NULL != node);
    node_count = node->subtreeCount;
    LOGASSERT((0 < node_count) && (node_count <= UTL_XML_NODE_MAX_NODES));
    Utl_Xml_Node_Detach(node);
    current = node;

    for (visit_index = 0; (visit_index < (2u * node_count)) && (NULL != current); ++visit_index)
    {
        if (NULL != current->firstChild)
        {
            current = current->firstChild;
        }
        else
        {
            parent = current->parent;

            if (NULL != parent)
            {
                parent->firstChild = current->nextSibling;

                if (NULL != current->nextSibling)
                {
                    current->nextSibling->previousSibling = NULL;
                }
                else
                {
                    parent->lastChild = NULL;
                }
            }

            FreeNode(current);
            current = parent;
        }
    }

    LOGASSERT(NULL == current);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_SetName(Utl_Xml_Node_t* node, const char_t* name)
{
    char_t* copy;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT((UTL_XML_NODE_ELEMENT == node->type) || (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type));

    if (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type)
    {
        if (FALSE == IsProcessingInstruction(name, node->value))
        {
            return FALSE;
        }
    }
    else if (FALSE == IsName(name))
    {
        return FALSE;
    }

    copy = CopyString(name, UTL_XML_NODE_MAX_NAME_BYTES);

    if (NULL == copy)
    {
        return FALSE;
    }

    free(node->name);
    node->name = copy;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_SetValue(Utl_Xml_Node_t* node, const char_t* value)
{
    bool_t valid;
    char_t*  copy;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT != node->type);

    valid = FALSE;

    if (UTL_XML_NODE_TEXT == node->type)
    {
        valid = IsXmlString(value);
    }
    else if (UTL_XML_NODE_COMMENT == node->type)
    {
        valid = IsComment(value);
    }
    else if (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type)
    {
        valid = IsProcessingInstruction(node->name, value);
    }

    if (FALSE == valid)
    {
        return FALSE;
    }

    copy = CopyString(value, UTL_XML_NODE_MAX_VALUE_BYTES);

    if (NULL == copy)
    {
        return FALSE;
    }

    free(node->value);
    node->value = copy;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child)
{
    return Utl_Xml_Node_InsertBefore(parent, child, NULL);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Validates the position and cycle before detaching the child.
 */
bool_t Utl_Xml_Node_InsertBefore(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child, Utl_Xml_Node_t* beforeSibling)
{
    Utl_Xml_Node_t* target_root;
    Utl_Xml_Node_t* source_root;
    Utl_Xml_Node_t* ancestor;
    uint32_t        ancestor_index;

    LOGASSERT(NULL != parent);
    LOGASSERT(NULL != child);
    LOGASSERT(UTL_XML_NODE_ELEMENT == parent->type);

    if ((NULL != beforeSibling) && (beforeSibling->parent != parent))
    {
        return FALSE;
    }

    if (TRUE == IsAncestor(child, parent))
    {
        return FALSE;
    }

    if (beforeSibling == child)
    {
        return TRUE;
    }

    target_root = FindRoot(parent);
    source_root = FindRoot(child);
    LOGASSERT((0 < target_root->subtreeCount) && (target_root->subtreeCount <= UTL_XML_NODE_MAX_NODES));
    LOGASSERT((0 < child->subtreeCount) && (child->subtreeCount <= UTL_XML_NODE_MAX_NODES));

    if ((target_root != source_root) && (child->subtreeCount > UTL_XML_NODE_MAX_NODES - target_root->subtreeCount))
    {
        return FALSE;
    }

    Utl_Xml_Node_Detach(child);
    child->parent = parent;

    ancestor = parent;

    for (ancestor_index = 0; (ancestor_index < UTL_XML_NODE_MAX_NODES) && (NULL != ancestor); ++ancestor_index)
    {
        LOGASSERT(ancestor->subtreeCount <= UTL_XML_NODE_MAX_NODES - child->subtreeCount);
        ancestor->subtreeCount += child->subtreeCount;
        ancestor = ancestor->parent;
    }

    LOGASSERT(NULL == ancestor);

    if (NULL == beforeSibling)
    {
        child->previousSibling = parent->lastChild;

        if (NULL != parent->lastChild)
        {
            parent->lastChild->nextSibling = child;
        }
        else
        {
            parent->firstChild = child;
        }

        parent->lastChild = child;
    }
    else
    {
        child->nextSibling     = beforeSibling;
        child->previousSibling = beforeSibling->previousSibling;
        beforeSibling->previousSibling = child;

        if (NULL != child->previousSibling)
        {
            child->previousSibling->nextSibling = child;
        }
        else
        {
            parent->firstChild = child;
        }
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Node_Detach(Utl_Xml_Node_t* node)
{
    Utl_Xml_Node_t* parent;
    Utl_Xml_Node_t* ancestor;
    uint32_t        ancestor_index;

    LOGASSERT(NULL != node);
    parent = node->parent;

    if (NULL == parent)
    {
        return;
    }

    ancestor = parent;

    for (ancestor_index = 0; (ancestor_index < UTL_XML_NODE_MAX_NODES) && (NULL != ancestor); ++ancestor_index)
    {
        LOGASSERT(node->subtreeCount <= ancestor->subtreeCount);
        ancestor->subtreeCount -= node->subtreeCount;
        ancestor = ancestor->parent;
    }

    LOGASSERT(NULL == ancestor);

    if (NULL != node->previousSibling)
    {
        node->previousSibling->nextSibling = node->nextSibling;
    }
    else
    {
        parent->firstChild = node->nextSibling;
    }

    if (NULL != node->nextSibling)
    {
        node->nextSibling->previousSibling = node->previousSibling;
    }
    else
    {
        parent->lastChild = node->previousSibling;
    }

    node->parent          = NULL;
    node->previousSibling = NULL;
    node->nextSibling     = NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Adds only after all validation and allocation have succeeded.
 */
bool_t Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* attribute;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    if ((FALSE == IsName(name)) || (FALSE == IsXmlString(value)) ||
        (UTL_XML_NODE_MAX_ATTRIBUTES <= node->attributeCount) || (NULL != FindAttribute(node, name)))
    {
        return FALSE;
    }

    attribute = (Utl_Xml_Attribute_t*)calloc(1, sizeof((*attribute)));

    if (NULL == attribute)
    {
        return FALSE;
    }

    attribute->name  = CopyString(name, UTL_XML_NODE_MAX_NAME_BYTES);
    attribute->value = CopyString(value, UTL_XML_NODE_MAX_VALUE_BYTES);

    if ((NULL == attribute->name) || (NULL == attribute->value))
    {
        free(attribute->name);
        free(attribute->value);
        free(attribute);
        return FALSE;
    }

    if (NULL != node->lastAttribute)
    {
        node->lastAttribute->next = attribute;
    }
    else
    {
        node->firstAttribute = attribute;
    }

    node->lastAttribute = attribute;
    ++node->attributeCount;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Replacing a value does not change attribute order.
 */
bool_t Utl_Xml_Node_SetAttribute(Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* attribute;
    char_t*                copy;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    if ((FALSE == IsName(name)) || (FALSE == IsXmlString(value)))
    {
        return FALSE;
    }

    attribute = FindAttribute(node, name);

    if (NULL == attribute)
    {
        return Utl_Xml_Node_AddAttribute(node, name, value);
    }

    copy = CopyString(value, UTL_XML_NODE_MAX_VALUE_BYTES);

    if (NULL == copy)
    {
        return FALSE;
    }

    free(attribute->value);
    attribute->value = copy;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_RemoveAttribute(Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* current;
    Utl_Xml_Attribute_t* previous;
    uint32_t             attribute_index;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    current  = node->firstAttribute;
    previous = NULL;
    LOGASSERT(node->attributeCount <= UTL_XML_NODE_MAX_ATTRIBUTES);

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != current);

        if (0 == strncmp(current->name, name, UTL_XML_NODE_MAX_NAME_BYTES + 1u))
        {
            if (NULL != previous)
            {
                previous->next = current->next;
            }
            else
            {
                node->firstAttribute = current->next;
            }

            if (node->lastAttribute == current)
            {
                node->lastAttribute = previous;
            }

            free(current->name);
            free(current->value);
            free(current);
            --node->attributeCount;
            return TRUE;
        }

        previous = current;
        current  = current->next;
    }

    LOGASSERT(NULL == current);
    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
const char_t* Utl_Xml_Node_GetAttributeValue(const Utl_Xml_Node_t* node, const char_t* name)
{
    Utl_Xml_Attribute_t* attribute;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    attribute = FindAttribute(node, name);
    return (NULL != attribute) ? attribute->value : NULL;
}
