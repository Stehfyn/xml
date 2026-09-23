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
 * \brief Finds child in parent's ancestor chain, including parent.
 *
 * \param[in] parent  Node whose parent chain is searched
 * \param[in] child   Node to find
 *
 * \return Matching ancestor, or NULL if child is absent.
 */
static const Utl_Xml_Node_t* FindAncestor(const Utl_Xml_Node_t* parent, const Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Finds an attribute with the requested name.
 *
 * \param[in] node           Element whose attributes are searched
 * \param[in] attribute      Attribute to be appended; must not already be linked
 * \param[in] name           Name bytes to find
 * \param[in] nameByteCount  Number of name bytes
 *
 * \return Matching attribute, or NULL if the name is absent.
 */
static const Utl_Xml_Attribute_t* FindAttribute(const Utl_Xml_Node_t* node, const Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

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

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  None
 */
void Utl_Xml_Node_Initialize(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount)
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
 *  Links an unlinked child at the end of the element's child list.
 */
void Utl_Xml_Node_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child)
{
    const Utl_Xml_Node_t* ancestor;

    LOGASSERT(NULL != parent);
    LOGASSERT(NULL != child);
    LOGASSERT(UTL_XML_NODE_ELEMENT == parent->type);
    LOGASSERT(NULL == child->parent);
    LOGASSERT(NULL == child->nextSibling);

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
void Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount)
{
    bool_t                     is_name;
    bool_t                     is_xml_string;
    const Utl_Xml_Attribute_t* found_attribute;

    LOGASSERT(NULL != node);
    LOGASSERT(NULL != attribute);
    LOGASSERT(NULL != name);
    LOGASSERT(NULL != value);
    LOGASSERT(UTL_XML_NODE_ELEMENT == node->type);
    LOGASSERT(node->attributeCount < UTL_XML_NODE_MAX_ATTRIBUTES);

    is_name = Utl_Xml_IsName(name, nameByteCount);
    LOGASSERT(is_name);

    is_xml_string = Utl_Xml_IsString(value, valueByteCount);
    LOGASSERT(is_xml_string);

    found_attribute = FindAttribute(node, attribute, name, nameByteCount);
    LOGASSERT(NULL == found_attribute);

    attribute->name           = name;
    attribute->nameByteCount  = nameByteCount;
    attribute->value          = value;
    attribute->valueByteCount = valueByteCount;
    attribute->next           = NULL;

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
