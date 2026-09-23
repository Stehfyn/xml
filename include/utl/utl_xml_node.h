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

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define UTL_XML_NODE_MAX_NAME_BYTES  (1024u)    ///< Maximum UTF-8 bytes in a node or attribute name.
#define UTL_XML_NODE_MAX_VALUE_BYTES (1048576u) ///< Maximum UTF-8 bytes in a node or attribute value.
#define UTL_XML_NODE_MAX_ATTRIBUTES  (256u)     ///< Maximum attributes on one element.
#define UTL_XML_NODE_MAX_NODES       (65536u)   ///< Maximum nodes in one connected tree.

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
    UTL_XML_NODE_PROCESSING_INSTRUCTION
} Utl_Xml_NodeType_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned attribute linked to a caller-owned element.
 */
typedef struct Utl_Xml_Attribute_s
{
    const char_t*               name;  ///< Borrowed null-terminated UTF-8 XML name.
    const char_t*               value; ///< Borrowed null-terminated UTF-8 XML value.
    struct Utl_Xml_Attribute_s* next;  ///< Next attribute in insertion order, or NULL.
} Utl_Xml_Attribute_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned XML node with borrowed strings and caller-owned links.
 */
typedef struct Utl_Xml_Node_s
{
    Utl_Xml_NodeType_t     type;            ///< Element, text, comment, or processing instruction.
    const char_t*          name;            ///< Borrowed element name or PI target; NULL otherwise.
    const char_t*          value;           ///< Borrowed text, comment, or PI data; NULL for elements.
    struct Utl_Xml_Node_s* parent;          ///< Parent element, or NULL when detached.
    struct Utl_Xml_Node_s* previousSibling; ///< Previous child of parent, or NULL.
    struct Utl_Xml_Node_s* nextSibling;     ///< Next child of parent, or NULL.
    struct Utl_Xml_Node_s* firstChild;      ///< First ordered child of an element, or NULL.
    struct Utl_Xml_Node_s* lastChild;       ///< Last ordered child of an element, or NULL.
    Utl_Xml_Attribute_t*   firstAttribute;  ///< First attribute of an element, or NULL.
    Utl_Xml_Attribute_t*   lastAttribute;   ///< Last attribute of an element, or NULL.
    uint32_t               attributeCount;  ///< Number of linked attributes.
    uint32_t               subtreeCount;    ///< Number of nodes in this subtree, including this node.
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
 * \brief Initializes caller-provided storage as a detached element.
 *
 * \par Requirements:
 *  - 001 This function shall accept a complete UTF-8 XML 1.0 Name within UTL_XML_NODE_MAX_NAME_BYTES.
 *  - 002 This function shall leave node unchanged if name is invalid.
 *  - 003 This function shall borrow name; the caller shall keep it valid while referenced.
 *  - 004 This function shall fail an assertion for NULL arguments.
 *
 * \param[out] node   Uninitialized caller-owned node storage
 * \param[in]  name   Non-NULL null-terminated UTF-8 element name
 *
 * \retval TRUE    if initialized.
 * \retval FALSE   if name is invalid
 */
bool_t Utl_Xml_Node_InitializeElement(Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as a detached text node.
 *
 * \par Requirements:
 *  - 001 This function shall accept only UTF-8 XML 1.0 characters within UTL_XML_NODE_MAX_VALUE_BYTES.
 *  - 002 This function shall leave node unchanged if text is invalid.
 *  - 003 This function shall borrow text; the caller shall keep it valid while referenced.
 *  - 004 This function shall fail an assertion for NULL arguments.
 *
 * \param[out] node   Uninitialized caller-owned node storage
 * \param[in]  text   Non-NULL null-terminated UTF-8 text
 *
 * \retval TRUE    if initialized.
 * \retval FALSE   if text is invalid
 */
bool_t Utl_Xml_Node_InitializeText(Utl_Xml_Node_t* node, const char_t* text);

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as a detached comment node.
 *
 * \par Requirements:
 *  - 001 This function shall reject invalid XML characters, "--", a final '-', and oversized text.
 *  - 002 This function shall leave node unchanged if text is invalid.
 *  - 003 This function shall borrow text; the caller shall keep it valid while referenced.
 *  - 004 This function shall fail an assertion for NULL arguments.
 *
 * \param[out] node   Uninitialized caller-owned node storage
 * \param[in]  text   Non-NULL null-terminated UTF-8 comment content
 *
 * \retval TRUE    if initialized.
 * \retval FALSE   if text is invalid
 */
bool_t Utl_Xml_Node_InitializeComment(Utl_Xml_Node_t* node, const char_t* text);

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as a detached processing instruction.
 *
 * \par Requirements:
 *  - 001 This function shall reject an invalid or "xml" target in any case spelling.
 *  - 002 This function shall reject invalid XML characters, "?>" in data, and oversized strings.
 *  - 003 This function shall leave node unchanged if either input is invalid.
 *  - 004 This function shall borrow both strings; the caller shall keep them valid while referenced.
 *  - 005 This function shall fail an assertion for NULL arguments.
 *
 * \param[out] node     Uninitialized caller-owned node storage
 * \param[in]  target   Non-NULL null-terminated UTF-8 target
 * \param[in]  data     Non-NULL null-terminated UTF-8 instruction data
 *
 * \retval TRUE    if initialized.
 * \retval FALSE   if either string is invalid
 */
bool_t Utl_Xml_Node_InitializeProcessingInstruction(Utl_Xml_Node_t* node, const char_t* target, const char_t* data);

/*************************************************************************************************/
/**
 * \brief Changes an element name or processing-instruction target.
 *
 * \par Requirements:
 *  - 001 This function shall enforce XML Name, PI target, and name length restrictions.
 *  - 002 This function shall preserve the old name on invalid input.
 *  - 003 This function shall borrow name; the caller shall keep it valid while referenced.
 *  - 004 This function shall fail an assertion for NULL arguments or another node kind.
 *
 * \param[in,out] node   Element or processing instruction to change
 * \param[in]     name   Non-NULL null-terminated UTF-8 name
 *
 * \retval TRUE    if the name was set.
 * \retval FALSE   if name is invalid
 */
bool_t Utl_Xml_Node_SetName(Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Changes text, comment, or processing-instruction data.
 *
 * \par Requirements:
 *  - 001 This function shall enforce XML character, delimiter, and value length restrictions.
 *  - 002 This function shall preserve the old value on invalid input.
 *  - 003 This function shall borrow value; the caller shall keep it valid while referenced.
 *  - 004 This function shall fail an assertion for NULL arguments or an element node.
 *
 * \param[in,out] node    Node to change
 * \param[in]     value   Non-NULL null-terminated UTF-8 value
 *
 * \retval TRUE    if the value was set.
 * \retval FALSE   if value is invalid
 */
bool_t Utl_Xml_Node_SetValue(Utl_Xml_Node_t* node, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Appends a node to an element's ordered children.
 *
 * \par Requirements:
 *  - 001 This function shall move an attached child from its old parent.
 *  - 002 This function shall reject cycles and trees larger than UTL_XML_NODE_MAX_NODES without changing links.
 *  - 003 This function shall fail an assertion for NULL arguments or a non-element parent.
 *
 * \param[in,out] parent   Element receiving the child
 * \param[in,out] child    Node to move
 *
 * \retval TRUE    if the child was appended.
 * \retval FALSE   if the move is invalid
 */
bool_t Utl_Xml_Node_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Inserts a node before an element child, or appends when beforeSibling is NULL.
 *
 * \par Requirements:
 *  - 001 This function shall move an attached child from its old parent.
 *  - 002 This function shall reject cycles, oversized trees, and invalid positions without changing links.
 *  - 003 This function shall fail an assertion for NULL parent or child, or a non-element parent.
 *
 * \param[in,out] parent          Element receiving the child
 * \param[in,out] child           Node to move
 * \param[in]     beforeSibling   Existing child of parent, or NULL to append
 *
 * \retval TRUE    if the child was inserted.
 * \retval FALSE   if the move or position is invalid
 */
bool_t Utl_Xml_Node_InsertBefore(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child, Utl_Xml_Node_t* beforeSibling);

/*************************************************************************************************/
/**
 * \brief Detaches a node from its parent without changing its storage.
 *
 * \par Requirements:
 *  - 001 This function shall preserve the node's children and attributes.
 *  - 002 This function shall have no effect on a detached node.
 *  - 003 This function shall fail an assertion if node is NULL.
 *
 * \param[in,out] node   Node to detach
 *
 * \retval None
 */
void Utl_Xml_Node_Detach(Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Links a caller-provided attribute to an element.
 *
 * \par Requirements:
 *  - 001 This function shall validate name and value and reject duplicate names.
 *  - 002 This function shall enforce string and UTL_XML_NODE_MAX_ATTRIBUTES limits.
 *  - 003 This function shall leave node and attribute unchanged on failure.
 *  - 004 This function shall borrow name and value; the caller shall keep them valid while linked.
 *  - 005 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in,out] node        Element receiving the attribute
 * \param[out]    attribute   Unlinked caller-owned attribute storage
 * \param[in]     name        Non-NULL null-terminated UTF-8 attribute name
 * \param[in]     value       Non-NULL null-terminated UTF-8 expanded value
 *
 * \retval TRUE    if the attribute was linked.
 * \retval FALSE   if invalid or duplicate
 */
bool_t Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, Utl_Xml_Attribute_t* attribute, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Changes the value of an existing attribute.
 *
 * \par Requirements:
 *  - 001 This function shall validate name and value and preserve insertion order.
 *  - 002 This function shall leave the attribute unchanged if absent or invalid.
 *  - 003 This function shall borrow value; the caller shall keep it valid while linked.
 *  - 004 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in,out] node    Element containing the attribute
 * \param[in]     name    Non-NULL null-terminated UTF-8 attribute name
 * \param[in]     value   Non-NULL null-terminated UTF-8 expanded value
 *
 * \retval TRUE    if the value was set.
 * \retval FALSE   if absent or invalid
 */
bool_t Utl_Xml_Node_SetAttribute(Utl_Xml_Node_t* node, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Unlinks an attribute by case-sensitive name.
 *
 * \par Requirements:
 *  - 001 This function shall return the caller-owned attribute when found, or NULL if absent.
 *  - 002 This function shall clear the removed attribute's next link without freeing storage.
 *  - 003 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in,out] node   Element to change
 * \param[in]     name   Non-NULL null-terminated attribute name
 *
 * \return Unlinked caller-owned attribute or NULL
 */
Utl_Xml_Attribute_t* Utl_Xml_Node_RemoveAttribute(Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Finds an attribute value by case-sensitive name.
 *
 * \par Requirements:
 *  - 001 This function shall return NULL when the attribute is absent.
 *  - 002 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in] node   Element to inspect
 * \param[in] name   Non-NULL null-terminated attribute name
 *
 * \return Borrowed UTF-8 value or NULL
 */
const char_t* Utl_Xml_Node_GetAttributeValue(const Utl_Xml_Node_t* node, const char_t* name);

#endif /* UTL_XML_NODE_H */
