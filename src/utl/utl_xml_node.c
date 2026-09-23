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
 * \brief Initializes caller-owned node storage after validating its strings.
 */
static void InitializeNode(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const char_t* name, const char_t* value);

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
    bool_t   found;

    found = FALSE;

    for (index = 0; index <= maximum; ++index)
    {
        if ('\0' == text[index])
        {
            (*length) = index;
            found = TRUE;
            break;
        }
    }

    return found;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Rejects truncated sequences, overlong forms, surrogates, and code points above U+10FFFF.
 */
static bool_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t* offset, uint32_t* codePoint)
{
    byte_t   first;
    uint32_t decoded;
    uint32_t minimum;
    uint32_t numContinuations;
    uint32_t index;
    uint32_t position;
    bool_t   isValid;

    isValid = ((*offset) < numBytes);

    if (TRUE == isValid)
    {
        position = (*offset);
        first    = bytes[position];

        if (first <= 0x7F)
        {
            decoded          = first;
            minimum          = 0;
            numContinuations = 0;
        }
        else if ((0xC2 <= first) && (first <= 0xDF))
        {
            decoded          = first & 0x1F;
            minimum          = 0x80;
            numContinuations = 1;
        }
        else if ((0xE0 <= first) && (first <= 0xEF))
        {
            decoded          = first & 0x0F;
            minimum          = 0x800;
            numContinuations = 2;
        }
        else if ((0xF0 <= first) && (first <= 0xF4))
        {
            decoded          = first & 0x07;
            minimum          = 0x10000;
            numContinuations = 3;
        }
        else
        {
            isValid = FALSE;
        }

        if (TRUE == isValid)
        {
            for (index = 0; (index < numContinuations) && (TRUE == isValid); ++index)
            {
                ++position;
                isValid = (position < numBytes) && (0x80 == (bytes[position] & 0xC0));

                if (TRUE == isValid)
                {
                    decoded = (decoded << 6) | (bytes[position] & 0x3F);
                }
            }
        }

        if (TRUE == isValid)
        {
            isValid = (minimum <= decoded) && (decoded <= 0x10FFFF) &&
                      ((decoded < 0xD800) || (0xDFFF < decoded));
        }

        if (TRUE == isValid)
        {
            (*codePoint) = decoded;
            (*offset)    = position + 1;
        }
    }

    return isValid;
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
    bool_t   isProcessingInstruction;
    bool_t   isXmlTarget;
    uint32_t targetLength;
    uint32_t dataLength;
    uint32_t index;

    isProcessingInstruction = IsName(target) &&
                              GetBoundedLength(target, UTL_XML_NODE_MAX_NAME_BYTES, &targetLength);

    if (TRUE == isProcessingInstruction)
    {
        isXmlTarget = (3 == targetLength) &&
                      (('x' == target[0]) || ('X' == target[0])) &&
                      (('m' == target[1]) || ('M' == target[1])) &&
                      (('l' == target[2]) || ('L' == target[2]));

        isProcessingInstruction = (FALSE == isXmlTarget) &&
                                  GetBoundedLength(data, UTL_XML_NODE_MAX_VALUE_BYTES, &dataLength) &&
                                  IsXmlString(data);
    }

    if (TRUE == isProcessingInstruction)
    {
        for (index = 0; index + 1 < dataLength; ++index)
        {
            if (('?' == data[index]) && ('>' == data[index + 1]))
            {
                isProcessingInstruction = FALSE;
                break;
            }
        }
    }

    return isProcessingInstruction;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void InitializeNode(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const char_t* name, const char_t* value)
{
    node->type            = type;
    node->name            = name;
    node->value           = value;
    node->parent          = NULL;
    node->previousSibling = NULL;
    node->nextSibling     = NULL;
    node->firstChild      = NULL;
    node->lastChild       = NULL;
    node->firstAttribute  = NULL;
    node->lastAttribute   = NULL;
    node->attributeCount  = 0;
    node->subtreeCount    = 1;
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
bool_t Utl_Xml_Node_InitializeElement(Utl_Xml_Node_t* node, const char_t* name)
{
    bool_t is_name;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);

    is_name = IsName(name);

    if (TRUE == is_name)
    {
        InitializeNode(node, UTL_XML_NODE_ELEMENT, name, NULL);
    }

    return is_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_InitializeText(Utl_Xml_Node_t* node, const char_t* text)
{
    bool_t is_xml_string;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != text);

    is_xml_string = IsXmlString(text);

    if (TRUE == is_xml_string)
    {
        InitializeNode(node, UTL_XML_NODE_TEXT, NULL, text);
    }

    return is_xml_string;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_InitializeComment(Utl_Xml_Node_t* node, const char_t* text)
{
    bool_t is_comment;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != text);

    is_comment = IsComment(text);

    if (TRUE == is_comment)
    {
        InitializeNode(node, UTL_XML_NODE_COMMENT, NULL, text);
    }

    return is_comment;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_InitializeProcessingInstruction(Utl_Xml_Node_t* node, const char_t* target, const char_t* data)
{
    bool_t is_processing_instruction;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != target);
    LOGASSERT(NULL != data);

    is_processing_instruction = IsProcessingInstruction(target, data);

    if (TRUE == is_processing_instruction)
    {
        InitializeNode(node, UTL_XML_NODE_PROCESSING_INSTRUCTION, target, data);
    }

    return is_processing_instruction;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_SetName(Utl_Xml_Node_t* node, const char_t* name)
{
    bool_t is_valid_name;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT((UTL_XML_NODE_ELEMENT == node->type) || (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type));

    if (UTL_XML_NODE_PROCESSING_INSTRUCTION == node->type)
    {
        is_valid_name = IsProcessingInstruction(name, node->value);
    }
    else
    {
        is_valid_name = IsName(name);
    }

    if (TRUE == is_valid_name)
    {
        node->name = name;
    }

    return is_valid_name;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_SetValue(Utl_Xml_Node_t* node, const char_t* value)
{
    bool_t valid;

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

    if (TRUE == valid)
    {
        node->value = value;
    }

    return valid;
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
    Utl_Xml_Node_t* targetRoot;
    Utl_Xml_Node_t* sourceRoot;
    Utl_Xml_Node_t* ancestor;
    uint32_t        ancestorIndex;
    bool_t          isValidPosition;
    bool_t          isCycleFree;
    bool_t          hasCapacity;

    LOGASSERT(NULL != parent);
    LOGASSERT(NULL != child);
    LOGASSERT(UTL_XML_NODE_ELEMENT == parent->type);

    isValidPosition = (NULL == beforeSibling) || (beforeSibling->parent == parent);
    isCycleFree     = (FALSE == IsAncestor(child, parent));

    if ((TRUE == isValidPosition) && (TRUE == isCycleFree))
    {
        if (beforeSibling == child)
        {
            return TRUE;
        }

        targetRoot = FindRoot(parent);
        sourceRoot = FindRoot(child);
        LOGASSERT((0 < targetRoot->subtreeCount) && (targetRoot->subtreeCount <= UTL_XML_NODE_MAX_NODES));
        LOGASSERT((0 < child->subtreeCount) && (child->subtreeCount <= UTL_XML_NODE_MAX_NODES));

        hasCapacity = (targetRoot == sourceRoot) ||
                      (child->subtreeCount <= UTL_XML_NODE_MAX_NODES - targetRoot->subtreeCount);

        if (TRUE == hasCapacity)
        {
            Utl_Xml_Node_Detach(child);
            child->parent = parent;
            ancestor = parent;

            for (ancestorIndex = 0; (ancestorIndex < UTL_XML_NODE_MAX_NODES) && (NULL != ancestor); ++ancestorIndex)
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
                child->nextSibling         = beforeSibling;
                child->previousSibling     = beforeSibling->previousSibling;
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
    }

    return FALSE;
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
    uint32_t        ancestorIndex;

    LOGASSERT(NULL != node);
    parent = node->parent;

    if (NULL != parent)
    {
        ancestor = parent;

        for (ancestorIndex = 0; (ancestorIndex < UTL_XML_NODE_MAX_NODES) && (NULL != ancestor); ++ancestorIndex)
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
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Links the caller's attribute only after validating its strings and list position.
 */
bool_t Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* current;
    uint32_t             attribute_index;
    bool_t               is_valid;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != attribute);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    is_valid = IsName(name) && IsXmlString(value) &&
               (node->attributeCount < UTL_XML_NODE_MAX_ATTRIBUTES);

    if (TRUE == is_valid)
    {
        is_valid = (NULL == FindAttribute(node, name));
    }

    if (TRUE == is_valid)
    {
        current = node->firstAttribute;

        for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
        {
            LOGASSERT(NULL != current);

            if (attribute == current)
            {
                is_valid = FALSE;
                break;
            }

            current = current->next;
        }

        if (TRUE == is_valid)
        {
            LOGASSERT(NULL == current);
        }
    }

    if (TRUE == is_valid)
    {
        attribute->name  = name;
        attribute->value = value;
        attribute->next  = NULL;

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
    }

    return is_valid;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Replacing a value does not change attribute order.
 */
bool_t Utl_Xml_Node_SetAttribute(Utl_Xml_Node_t* node, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* attribute;
    bool_t               is_valid;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    is_valid = IsName(name) && IsXmlString(value);
    attribute = NULL;

    if (TRUE == is_valid)
    {
        attribute = FindAttribute(node, name);
    }

    if (NULL != attribute)
    {
        attribute->value = value;
    }

    return (NULL != attribute);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Attribute_t* Utl_Xml_Node_RemoveAttribute(Utl_Xml_Node_t* node, const char_t* name)
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

            current->next = NULL;
            --node->attributeCount;
            return current;
        }

        previous = current;
        current  = current->next;
    }

    LOGASSERT(NULL == current);
    return NULL;
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
