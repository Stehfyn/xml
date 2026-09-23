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
 * \brief Decodes one strictly encoded UTF-8 code point and advances cursor.
 */
static bool_t DecodeUtf8(const byte_t** cursor, uint32_t* codePoint);

/*************************************************************************************************/
/**
 * \brief Checks a null-terminated UTF-8 string against XML 1.0 Char.
 */
static bool_t IsXmlString(const char* text);

/*************************************************************************************************/
/**
 * \brief Checks a null-terminated UTF-8 string against XML 1.0 Name.
 */
static bool_t IsName(const char* text);

/*************************************************************************************************/
/**
 * \brief Checks whether comment text can occur between XML comment delimiters.
 */
static bool_t IsComment(const char* text);

/*************************************************************************************************/
/**
 * \brief Checks an XML processing-instruction target and data.
 */
static bool_t IsProcessingInstruction(const char* target, const char* data);

/*************************************************************************************************/
/**
 * \brief Allocates a copy of a null-terminated string.
 */
static char* CopyString(const char* text);

/*************************************************************************************************/
/**
 * \brief Allocates a detached node with copied name and value.
 */
static Utl_Xml_Node_t* AllocateNode(Utl_Xml_NodeType_t type, const char* name, const char* value);

/*************************************************************************************************/
/**
 * \brief Frees one node and its owned fields after its children have been freed.
 */
static void FreeNode(Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Finds an element attribute by case-sensitive name.
 */
static Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const char* name);

/*************************************************************************************************/
/**
 * \brief Checks whether candidate is node or an ancestor of node.
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
static bool_t DecodeUtf8(const byte_t** cursor, uint32_t* codePoint)
{
    const byte_t* current;
    byte_t        first;
    uint32_t             decoded;
    uint32_t             minimum;
    uint32_t             num_continuations;
    uint32_t             index;

    current = *cursor;
    first   = *current;

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
        ++current;

        if ((0 == *current) || (0x80 != (*current & 0xC0)))
        {
            return FALSE;
        }

        decoded = (decoded << 6) | (*current & 0x3F);
    }

    if ((decoded < minimum) || (0x10FFFF < decoded) || ((0xD800 <= decoded) && (decoded <= 0xDFFF)))
    {
        return FALSE;
    }

    *codePoint = decoded;
    *cursor    = current + 1;
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Empty strings are permitted as text and attribute values.
 */
static bool_t IsXmlString(const char* text)
{
    const byte_t* cursor;
    uint32_t             code_point;

    cursor = (const byte_t*)text;

    while (0 != *cursor)
    {
        if (FALSE == DecodeUtf8(&cursor, &code_point))
        {
            return FALSE;
        }

        if (FALSE == Utl_Xml_IsChar(code_point))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Uses the existing XML 1.0 NameStartChar and NameChar classifiers.
 */
static bool_t IsName(const char* text)
{
    const byte_t* cursor;
    uint32_t             code_point;

    cursor = (const byte_t*)text;

    if (0 == *cursor)
    {
        return FALSE;
    }

    if (FALSE == DecodeUtf8(&cursor, &code_point))
    {
        return FALSE;
    }

    if (FALSE == Utl_Xml_IsNameStartChar(code_point))
    {
        return FALSE;
    }

    while (0 != *cursor)
    {
        if (FALSE == DecodeUtf8(&cursor, &code_point))
        {
            return FALSE;
        }

        if (FALSE == Utl_Xml_IsNameChar(code_point))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML 1.0 production [15] disallows "--" within the content and a final '-'.
 */
static bool_t IsComment(const char* text)
{
    size_t length;

    if (FALSE == IsXmlString(text))
    {
        return FALSE;
    }

    if (NULL != strstr(text, "--"))
    {
        return FALSE;
    }

    length = strlen(text);
    return (0 == length) || ('-' != text[length - 1]);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  XML 1.0 productions [16] and [17] reserve all case spellings of "xml" as PI targets.
 */
static bool_t IsProcessingInstruction(const char* target, const char* data)
{
    bool_t is_xml_target;

    if (FALSE == IsName(target))
    {
        return FALSE;
    }

    is_xml_target = (3 == strlen(target)) &&
                    (('x' == target[0]) || ('X' == target[0])) &&
                    (('m' == target[1]) || ('M' == target[1])) &&
                    (('l' == target[2]) || ('L' == target[2]));

    if (is_xml_target)
    {
        return FALSE;
    }

    return IsXmlString(data) && (NULL == strstr(data, "?>"));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  A NULL return indicates allocation failure.
 */
static char* CopyString(const char* text)
{
    char*  copy;
    size_t length;

    length = strlen(text);

    if (SIZE_MAX == length)
    {
        return NULL;
    }

    copy = (char*)malloc(length + 1);

    if (NULL != copy)
    {
        (void)memcpy(copy, text, length + 1);
    }

    return copy;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The caller validates name and value before allocating.
 */
static Utl_Xml_Node_t* AllocateNode(Utl_Xml_NodeType_t type, const char* name, const char* value)
{
    Utl_Xml_Node_t* node;

    node = (Utl_Xml_Node_t*)calloc(1, sizeof(*node));

    if (NULL == node)
    {
        return NULL;
    }

    node->type = type;

    if (NULL != name)
    {
        node->name = CopyString(name);

        if (NULL == node->name)
        {
            free(node);
            return NULL;
        }
    }

    if (NULL != value)
    {
        node->value = CopyString(value);

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

    attribute = node->firstAttribute;

    while (NULL != attribute)
    {
        next_attribute = attribute->next;
        free(attribute->name);
        free(attribute->value);
        free(attribute);
        attribute = next_attribute;
    }

    free(node->name);
    free(node->value);
    free(node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Attribute names are compared without case folding or normalization.
 */
static Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const char* name)
{
    Utl_Xml_Attribute_t* attribute;

    attribute = node->firstAttribute;

    while (NULL != attribute)
    {
        if (0 == strcmp(attribute->name, name))
        {
            return attribute;
        }

        attribute = attribute->next;
    }

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

    current = node;

    while (NULL != current)
    {
        if (candidate == current)
        {
            return TRUE;
        }

        current = current->parent;
    }

    return FALSE;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateElement(const char* name)
{
    LOGASSERT(NULL != name);
    return (TRUE == IsName(name)) ? AllocateNode(UTL_XML_NODE_ELEMENT, name, NULL) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateText(const char* text)
{
    LOGASSERT(NULL != text);
    return (TRUE == IsXmlString(text)) ? AllocateNode(UTL_XML_NODE_TEXT, NULL, text) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateComment(const char* text)
{
    LOGASSERT(NULL != text);
    return (TRUE == IsComment(text)) ? AllocateNode(UTL_XML_NODE_COMMENT, NULL, text) : NULL;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateProcessingInstruction(const char* target, const char* data)
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

    LOGASSERT(NULL != node);
    Utl_Xml_Node_Detach(node);
    current = node;

    while (NULL != current)
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
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
bool_t Utl_Xml_Node_SetName(Utl_Xml_Node_t* node, const char* name)
{
    char* copy;

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

    copy = CopyString(name);

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
bool_t Utl_Xml_Node_SetValue(Utl_Xml_Node_t* node, const char* value)
{
    bool_t valid;
    char*  copy;

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

    copy = CopyString(value);

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

    Utl_Xml_Node_Detach(child);
    child->parent = parent;

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

    LOGASSERT(NULL != node);
    parent = node->parent;

    if (NULL == parent)
    {
        return;
    }

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
bool_t Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, const char* name, const char* value)
{
    Utl_Xml_Attribute_t* attribute;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    if ((FALSE == IsName(name)) || (FALSE == IsXmlString(value)) || (NULL != FindAttribute(node, name)))
    {
        return FALSE;
    }

    attribute = (Utl_Xml_Attribute_t*)calloc(1, sizeof(*attribute));

    if (NULL == attribute)
    {
        return FALSE;
    }

    attribute->name  = CopyString(name);
    attribute->value = CopyString(value);

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
    return TRUE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Replacing a value does not change attribute order.
 */
bool_t Utl_Xml_Node_SetAttribute(Utl_Xml_Node_t* node, const char* name, const char* value)
{
    Utl_Xml_Attribute_t* attribute;
    char*                copy;

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

    copy = CopyString(value);

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
bool_t Utl_Xml_Node_RemoveAttribute(Utl_Xml_Node_t* node, const char* name)
{
    Utl_Xml_Attribute_t* current;
    Utl_Xml_Attribute_t* previous;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    current  = node->firstAttribute;
    previous = NULL;

    while (NULL != current)
    {
        if (0 == strcmp(current->name, name))
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
            return TRUE;
        }

        previous = current;
        current  = current->next;
    }

    return FALSE;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
const char* Utl_Xml_Node_GetAttributeValue(const Utl_Xml_Node_t* node, const char* name)
{
    Utl_Xml_Attribute_t* attribute;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != name);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);

    attribute = FindAttribute(node, name);
    return (NULL != attribute) ? attribute->value : NULL;
}
