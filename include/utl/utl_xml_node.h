#ifndef UTL_XML_NODE_H
#define UTL_XML_NODE_H

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

#include "os/os_types.h"
#include "utl/utl_xml.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define UTL_XML_NODE_MAX_NAME_BYTES  UTL_XML_MAX_NAME_BYTES   ///< Maximum UTF-8 bytes in a node or attribute name.
#define UTL_XML_NODE_MAX_VALUE_BYTES UTL_XML_MAX_STRING_BYTES ///< Maximum UTF-8 bytes in a node or attribute value.
#define UTL_XML_NODE_MAX_ATTRIBUTES  (256u)     ///< Maximum attributes on one element.
#define UTL_XML_NODE_MAX_DEPTH       (65536u)   ///< Maximum ancestor links checked when appending a child.

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief XML element, text, comment, or processing-instruction kind.
 */
typedef enum Utl_Xml_NodeType_e
{
    UTL_XML_NODE_ELEMENT,
    UTL_XML_NODE_TEXT,
    UTL_XML_NODE_COMMENT,
    UTL_XML_NODE_PROCESSING_INSTRUCTION,
    UTL_XML_NODE_XML_DECLARATION,
    UTL_XML_NODE_DOCTYPE
} Utl_Xml_NodeType_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned XML attribute for an element's forward list.
 */
typedef struct Utl_Xml_Attribute_s
{
    const byte_t*               name;           ///< Borrowed UTF-8 XML name bytes.
    uint32_t                    nameByteCount;  ///< Number of name bytes.
    const byte_t*               value;          ///< Borrowed UTF-8 XML value bytes.
    uint32_t                    valueByteCount; ///< Number of value bytes.
    struct Utl_Xml_Attribute_s* next;           ///< Next attribute in insertion order, or NULL.
} Utl_Xml_Attribute_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned XML node with borrowed strings and forward links for serialization.
 */
typedef struct Utl_Xml_Node_s
{
    Utl_Xml_NodeType_t     type;           ///< Element, text, comment, or processing instruction.
    const byte_t*          name;           ///< Borrowed element name or PI target; NULL otherwise.
    uint32_t               nameByteCount;  ///< Number of name bytes; zero when name is NULL.
    const byte_t*          value;          ///< Borrowed content or complete declaration markup; NULL for elements.
    uint32_t               valueByteCount; ///< Number of value bytes; zero when value is NULL.
    struct Utl_Xml_Node_s* parent;         ///< Parent set when appended, or NULL.
    struct Utl_Xml_Node_s* firstChild;     ///< First child of an element, or NULL.
    struct Utl_Xml_Node_s* lastChild;      ///< Last child of an element, or NULL.
    struct Utl_Xml_Node_s* nextSibling;    ///< Next node at the same level, or NULL.
    Utl_Xml_Attribute_t*   firstAttribute; ///< First attribute of an element, or NULL.
    Utl_Xml_Attribute_t*   lastAttribute;  ///< Last attribute of an element, or NULL.
    uint32_t               attributeCount; ///< Number of linked attributes.
} Utl_Xml_Node_t;

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as an unlinked XML node.
 *
 * \par Requirements:
 *  - 001 This function shall store type, name, value, and their byte counts without checking XML syntax or length.
 *  - 002 This function shall fail an assertion if node is NULL.
 *  - 003 This function shall borrow non-NULL byte spans; the caller shall keep them valid while referenced.
 *  - 004 This function shall clear the node's tree and attribute links.
 *  - 005 This function shall require a zero byte count when the corresponding pointer is NULL.
 *
 * \par Details:
 *  Elements use name and set value to NULL. Text and comments set name to NULL
 *  and use value for content. Processing instructions use name for the target
 *  and value for data. XML declaration and DOCTYPE nodes use value for complete
 *  markup and a NULL name. XML validation and escaping are deferred until serialization.
 *
 * \param[out] node            Uninitialized caller-owned node storage
 * \param[in]  type            XML node kind
 * \param[in]  name            Borrowed UTF-8 name byte span or NULL
 * \param[in]  nameByteCount   Number of name bytes
 * \param[in]  value           Borrowed UTF-8 value byte span or NULL
 * \param[in]  valueByteCount  Number of value bytes
 *
 * \retval None
 */
void Utl_Xml_Node_Initialize(Utl_Xml_Node_t* node, Utl_Xml_NodeType_t type, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount);

/*************************************************************************************************/
/**
 * \brief Appends a caller-owned child while constructing a serializable element.
 *
 * \par Requirements:
 *  - 001 This function shall preserve the order in which children are appended.
 *  - 002 This function shall fail an assertion for NULL arguments, a non-element parent, an attached child, a document-only declaration, or a cycle.
 *  - 003 This function shall check at most UTL_XML_NODE_MAX_DEPTH ancestor links.
 *
 * \param[in,out] parent   Element receiving the child
 * \param[in,out] child    Unlinked caller-owned child
 *
 * \retval None
 */
void Utl_Xml_Node_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Appends a caller-owned attribute while constructing a serializable element.
 *
 * \par Requirements:
 *  - 001 This function shall require a valid XML name and XML character value.
 *  - 002 This function shall fail an assertion for NULL arguments, a non-element node, a duplicate name, or an attribute already linked to the element.
 *  - 003 This function shall enforce UTL_XML_NODE_MAX_ATTRIBUTES and borrow both byte spans.
 *
 * \param[in,out] node            Element receiving the attribute
 * \param[out]    attribute       Unlinked caller-owned attribute storage
 * \param[in]     name            Non-NULL UTF-8 attribute name byte span
 * \param[in]     nameByteCount   Number of name bytes
 * \param[in]     value           Non-NULL UTF-8 expanded value byte span
 * \param[in]     valueByteCount  Number of value bytes
 *
 * \retval None
 */
void Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount);

#endif /* UTL_XML_NODE_H */
