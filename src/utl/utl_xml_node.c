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
void Utl_Xml_Node_Initialize(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const byte_t* name, const byte_t* value)
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
void Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute, const byte_t* name, const byte_t* value)
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

    is_name       = Utl_Xml_IsName(name);
    is_xml_string = Utl_Xml_IsString(value);
    LOGASSERT(is_name);
    LOGASSERT(is_xml_string);

    current = node->firstAttribute;

    for (attribute_index = 0; attribute_index < node->attributeCount; ++attribute_index)
    {
        LOGASSERT(NULL != current);
        LOGASSERT(attribute != current);
        is_unique = (0 != strncmp((const char*)current->name, (const char*)name, UTL_XML_NODE_MAX_NAME_BYTES + 1));
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
