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
 * \brief Checks whether candidate is node or one of its ancestors.
 *
 * \param[in] candidate   Node to find
 * \param[in] node        Node whose ancestors are checked
 *
 * \retval TRUE    if candidate is node or an ancestor.
 * \retval FALSE   otherwise
 */
static bool_t IsAncestor(const Utl_Xml_Node_t* candidate, const Utl_Xml_Node_t* node);

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
    uint32_t num_continuations;
    uint32_t index;
    uint32_t position;
    bool_t   is_valid;

    is_valid = ((*offset) < numBytes);

    if (is_valid)
    {
        position = (*offset);
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
            (*codePoint) = decoded;
            (*offset)    = position + 1;
        }
    }

    return is_valid;
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
    bool_t        is_xml_string;
    bool_t        is_fully_consumed;

    is_fully_consumed = FALSE;
    is_xml_string     = GetBoundedLength(text, UTL_XML_NODE_MAX_VALUE_BYTES, &byte_count);

    if (is_xml_string)
    {
        bytes             = (const byte_t*)text;
        byte_offset       = 0;
        is_fully_consumed = (0 == byte_count);

        for (code_point_index = 0; code_point_index < byte_count; ++code_point_index)
        {
            if (is_fully_consumed)
            {
                break;
            }

            is_xml_string = DecodeUtf8(bytes, byte_count, &byte_offset, &code_point);

            if (is_xml_string)
            {
                is_xml_string = Utl_Xml_IsChar(code_point);
            }

            if (FALSE == is_xml_string)
            {
                break;
            }

            is_fully_consumed = (byte_offset == byte_count);
        }

    }

    return is_xml_string && is_fully_consumed;
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
    bool_t        is_name;
    bool_t        is_fully_consumed;

    is_fully_consumed = FALSE;
    is_name           = GetBoundedLength(text, UTL_XML_NODE_MAX_NAME_BYTES, &byte_count);

    if (is_name)
    {
        is_name           = (0 < byte_count);
        bytes             = (const byte_t*)text;
        byte_offset       = 0;
        is_fully_consumed = (0 == byte_count);

        for (code_point_index = 0; code_point_index < byte_count; ++code_point_index)
        {
            if (is_fully_consumed)
            {
                break;
            }

            is_name = DecodeUtf8(bytes, byte_count, &byte_offset, &code_point);

            if (is_name)
            {
                if (0 == code_point_index)
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

            is_fully_consumed = (byte_offset == byte_count);
        }
    }

    return is_name && is_fully_consumed;
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
    uint32_t pair_count;
    bool_t   is_comment;

    is_comment = GetBoundedLength(text, UTL_XML_NODE_MAX_VALUE_BYTES, &length);

    if (is_comment)
    {
        is_comment = IsXmlString(text);
    }

    if (is_comment)
    {
        pair_count = 0;

        if (0 < length)
        {
            pair_count = length - 1;
        }

        for (index = 0; index < pair_count; ++index)
        {
            if (('-' == text[index]) && ('-' == text[index + 1]))
            {
                is_comment = FALSE;
                break;
            }
        }

        if (is_comment)
        {
            is_comment = (0 == length) || ('-' != text[length - 1]);
        }
    }

    return is_comment;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML 1.0 productions [16] and [17] reserve all case spellings of "xml" as PI targets.
 */
static bool_t IsProcessingInstruction(const char_t* target, const char_t* data)
{
    bool_t   is_processing_instruction;
    bool_t   is_xml_target;
    uint32_t target_length;
    uint32_t data_length;
    uint32_t index;
    uint32_t pair_count;

    is_processing_instruction = IsName(target);

    if (is_processing_instruction)
    {
        is_processing_instruction = GetBoundedLength(target, UTL_XML_NODE_MAX_NAME_BYTES, &target_length);
    }

    if (is_processing_instruction)
    {
        is_xml_target = (3 == target_length) &&
                        (('x' == target[0]) || ('X' == target[0])) &&
                        (('m' == target[1]) || ('M' == target[1])) &&
                        (('l' == target[2]) || ('L' == target[2]));

        is_processing_instruction = (FALSE == is_xml_target);
    }

    if (is_processing_instruction)
    {
        is_processing_instruction = GetBoundedLength(data, UTL_XML_NODE_MAX_VALUE_BYTES, &data_length);
    }

    if (is_processing_instruction)
    {
        is_processing_instruction = IsXmlString(data);
    }

    if (is_processing_instruction)
    {
        pair_count = 0;

        if (0 < data_length)
        {
            pair_count = data_length - 1;
        }

        for (index = 0; index < pair_count; ++index)
        {
            if (('?' == data[index]) && ('>' == data[index + 1]))
            {
                is_processing_instruction = FALSE;
                break;
            }
        }
    }

    return is_processing_instruction;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
static void InitializeNode(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const char_t* name, const char_t* value)
{
    node->type           = type;
    node->name           = name;
    node->value          = value;
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
 *  Bounds the ancestor traversal by UTL_XML_NODE_MAX_DEPTH.
 */
static bool_t IsAncestor(const Utl_Xml_Node_t* candidate, const Utl_Xml_Node_t* node)
{
    const Utl_Xml_Node_t* current;
    uint32_t              depth;
    bool_t                is_ancestor;

    current     = node;
    is_ancestor = FALSE;

    for (depth = 0; depth < UTL_XML_NODE_MAX_DEPTH; ++depth)
    {
        if (NULL == current)
        {
            break;
        }

        is_ancestor = (candidate == current);

        if (is_ancestor)
        {
            break;
        }

        current = current->parent;
    }

    LOGASSERT(is_ancestor || (NULL == current));
    return is_ancestor;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Node_InitializeElement(Utl_Xml_Node_t* node, const char_t* name)
{
    bool_t is_name;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);

    is_name = IsName(name);
    LOGASSERT(is_name);

    InitializeNode(node, UTL_XML_NODE_ELEMENT, name, NULL);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Node_InitializeText(Utl_Xml_Node_t* node, const char_t* text)
{
    bool_t is_xml_string;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != text);

    is_xml_string = IsXmlString(text);
    LOGASSERT(is_xml_string);

    InitializeNode(node, UTL_XML_NODE_TEXT, NULL, text);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Node_InitializeComment(Utl_Xml_Node_t* node, const char_t* text)
{
    bool_t is_comment;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != text);

    is_comment = IsComment(text);
    LOGASSERT(is_comment);

    InitializeNode(node, UTL_XML_NODE_COMMENT, NULL, text);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Node_InitializeProcessingInstruction(Utl_Xml_Node_t* node, const char_t* target, const char_t* data)
{
    bool_t is_processing_instruction;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != target);
    LOGASSERT(NULL != data);

    is_processing_instruction = IsProcessingInstruction(target, data);
    LOGASSERT(is_processing_instruction);

    InitializeNode(node, UTL_XML_NODE_PROCESSING_INSTRUCTION, target, data);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Links an unlinked child at the end of the element's child list.
 */
void Utl_Xml_Node_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child)
{
    bool_t creates_cycle;

    LOGASSERT(NULL != parent);
    LOGASSERT(NULL != child);
    LOGASSERT(UTL_XML_NODE_ELEMENT == parent->type);
    LOGASSERT(NULL == child->parent);
    LOGASSERT(NULL == child->nextSibling);

    creates_cycle = IsAncestor(child, parent);
    LOGASSERT(FALSE == creates_cycle);

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
void Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute, const char_t* name, const char_t* value)
{
    Utl_Xml_Attribute_t* current;
    uint32_t             attribute_index;
    bool_t               is_name;
    bool_t               is_xml_string;
    bool_t               is_unique;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != attribute);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);
    LOGASSERT(node->attributeCount < UTL_XML_NODE_MAX_ATTRIBUTES);

    is_name       = IsName(name);
    is_xml_string = IsXmlString(value);
    LOGASSERT(is_name);
    LOGASSERT(is_xml_string);

    current = node->firstAttribute;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != current);
        LOGASSERT(attribute != current);
        is_unique = (0 != strncmp(current->name, name, UTL_XML_NODE_MAX_NAME_BYTES + 1u));
        LOGASSERT(is_unique);
        current = current->next;
    }

    LOGASSERT(NULL == current);
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
