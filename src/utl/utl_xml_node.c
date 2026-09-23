#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

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
 * \brief Decodes one strictly encoded UTF-8 code point within a byte span.
 *
 * \param[in]     bytes       Non-NULL UTF-8 byte span
 * \param[in]     numBytes    Number of bytes in the span
 * \param[in]     offset      Current byte offset
 * \param[out]    decodedByteCount   Number of bytes decoded, or zero on failure
 *
 * \return Decoded code point, or zero for invalid UTF-8. Check decodedByteCount for success.
 */
static uint32_t DecodeUtf8(const byte_t* bytes, uint32_t numBytes, uint32_t offset, uint32_t* decodedByteCount);

/*************************************************************************************************/
/**
 * \brief Checks a null-terminated UTF-8 string against XML 1.0 Char.
 *
 * \param[in] text   Non-NULL null-terminated UTF-8 string
 *
 * \retval TRUE    if the string fits the byte limit and contains only XML Char code points; empty is valid.
 * \retval FALSE   otherwise
 */
static bool_t IsXmlString(const char_t* text);

/*************************************************************************************************/
/**
 * \brief Checks a null-terminated UTF-8 string against XML 1.0 Name.
 *
 * \param[in] text   Non-NULL null-terminated UTF-8 string
 *
 * \retval TRUE    if the non-empty string fits the byte limit and is a valid XML Name.
 * \retval FALSE   otherwise
 */
static bool_t IsName(const char_t* text);

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
 *  Empty strings are permitted as text and attribute values.
 */
static bool_t IsXmlString(const char_t* text)
{
    const byte_t* bytes;
    uint32_t      byte_count;
    uint32_t      byte_offset;
    uint32_t      code_point;
    uint32_t      decoded_byte_count;
    bool_t        is_xml_string;

    byte_count = (uint32_t)strnlen(text, UTL_XML_NODE_MAX_VALUE_BYTES + 1);
    is_xml_string = (byte_count <= UTL_XML_NODE_MAX_VALUE_BYTES);

    if (is_xml_string)
    {
        bytes = (const byte_t*)text;

        for (byte_offset = 0; byte_offset < byte_count; byte_offset += decoded_byte_count)
        {
            code_point = DecodeUtf8(bytes, byte_count, byte_offset, &decoded_byte_count);
            is_xml_string = (0 < decoded_byte_count);

            if (is_xml_string)
            {
                is_xml_string = Utl_Xml_IsChar(code_point);
            }

            if (FALSE == is_xml_string)
            {
                break;
            }
        }
    }

    return is_xml_string;
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
    uint32_t      decoded_byte_count;
    bool_t        is_name;

    byte_count = (uint32_t)strnlen(text, UTL_XML_NODE_MAX_NAME_BYTES + 1);
    is_name = (0 < byte_count) && (byte_count <= UTL_XML_NODE_MAX_NAME_BYTES);

    if (is_name)
    {
        bytes       = (const byte_t*)text;
        byte_offset = 0;

        for (code_point_index = 0; byte_offset < byte_count; ++code_point_index, byte_offset += decoded_byte_count)
        {
            code_point = DecodeUtf8(bytes, byte_count, byte_offset, &decoded_byte_count);
            is_name = (0 < decoded_byte_count);

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
        }
    }

    return is_name;
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
 *  Stores the borrowed strings and clears all tree links.
 */
void Utl_Xml_Node_Initialize(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const char_t* name, const char_t* value)
{
    LOGASSERT(NULL != node);

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
        is_unique = (0 != strncmp(current->name, name, UTL_XML_NODE_MAX_NAME_BYTES + 1));
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
