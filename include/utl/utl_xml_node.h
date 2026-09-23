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

#include <stdint.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define UTL_XML_NODE_MAX_NAME_BYTES  (1024u)    ///< Maximum UTF-8 bytes in an element, PI, or attribute name.
#define UTL_XML_NODE_MAX_VALUE_BYTES (1048576u) ///< Maximum UTF-8 bytes in a text, comment, PI, or attribute value.
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
 * \brief Attribute in an element's insertion-ordered attribute list.
 */
typedef struct Utl_Xml_Attribute_s
{
    char_t*                       name;  ///< Owned UTF-8 XML attribute name.
    char_t*                       value; ///< Owned UTF-8 expanded attribute value.
    struct Utl_Xml_Attribute_s* next;  ///< Next attribute in insertion order, or NULL.
} Utl_Xml_Attribute_t;

/*************************************************************************************************/
/**
 * \brief XML node with parent, sibling, child, and attribute links.
 */
typedef struct Utl_Xml_Node_s
{
    Utl_Xml_NodeType_t     type;            ///< Element, text, comment, or processing instruction.
    char_t*                  name;            ///< Owned UTF-8 element name or PI target; NULL otherwise.
    char_t*                  value;           ///< Owned UTF-8 text, comment, or PI data; NULL for elements.
    struct Utl_Xml_Node_s* parent;          ///< Parent element, or NULL when detached.
    struct Utl_Xml_Node_s* previousSibling; ///< Previous child of parent, or NULL.
    struct Utl_Xml_Node_s* nextSibling;     ///< Next child of parent, or NULL.
    struct Utl_Xml_Node_s* firstChild;      ///< First ordered child of an element, or NULL.
    struct Utl_Xml_Node_s* lastChild;       ///< Last ordered child of an element, or NULL.
    Utl_Xml_Attribute_t*   firstAttribute;  ///< First attribute of an element, or NULL.
    Utl_Xml_Attribute_t*   lastAttribute;   ///< Last attribute of an element, or NULL.
    uint32_t               attributeCount;  ///< Number of attributes on this element.
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
 * \brief Creates a detached element.
 *
 * \par Requirements:
 *  - 001 This function shall accept only a complete UTF-8 XML 1.0 Name.
 *  - 002 This function shall reject names longer than UTL_XML_NODE_MAX_NAME_BYTES.
 *  - 003 This function shall copy name and return NULL for an invalid name or allocation failure.
 *  - 004 This function shall fail an assertion if name is NULL.
 *
 * \param[in] name   Non-NULL null-terminated UTF-8 element name
 *
 * \retval Non-NULL   if the element was created.
 * \retval NULL       otherwise
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateElement(const char_t* name);

/*************************************************************************************************/
/**
 * \brief Creates a detached text node.
 *
 * \par Requirements:
 *  - 001 This function shall accept only UTF-8 sequences of XML 1.0 Char values.
 *  - 002 This function shall reject text longer than UTL_XML_NODE_MAX_VALUE_BYTES.
 *  - 003 This function shall copy text and return NULL for invalid text or allocation failure.
 *  - 004 This function shall fail an assertion if text is NULL.
 *
 * \param[in] text   Non-NULL null-terminated UTF-8 expanded character data
 *
 * \retval Non-NULL   if the text node was created.
 * \retval NULL       otherwise
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateText(const char_t* text);

/*************************************************************************************************/
/**
 * \brief Creates a detached comment node.
 *
 * \par Requirements:
 *  - 001 This function shall accept only UTF-8 sequences of XML 1.0 Char values.
 *  - 002 This function shall reject "--" and a final '-' in comment text.
 *  - 003 This function shall reject text longer than UTL_XML_NODE_MAX_VALUE_BYTES.
 *  - 004 This function shall copy text and return NULL for invalid text or allocation failure.
 *  - 005 This function shall fail an assertion if text is NULL.
 *
 * \param[in] text   Non-NULL null-terminated UTF-8 comment content
 *
 * \retval Non-NULL   if the comment was created.
 * \retval NULL       otherwise
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateComment(const char_t* text);

/*************************************************************************************************/
/**
 * \brief Creates a detached processing instruction.
 *
 * \par Requirements:
 *  - 001 This function shall accept only a UTF-8 XML 1.0 Name target other than any case of "xml".
 *  - 002 This function shall reject invalid XML characters and "?>" in data.
 *  - 003 This function shall enforce the name and value byte limits.
 *  - 004 This function shall copy target and data, returning NULL for invalid input or allocation failure.
 *  - 005 This function shall fail an assertion if target or data is NULL.
 *
 * \param[in] target   Non-NULL null-terminated UTF-8 target
 * \param[in] data     Non-NULL null-terminated UTF-8 instruction data
 *
 * \retval Non-NULL   if the processing instruction was created.
 * \retval NULL       otherwise
 */
Utl_Xml_Node_t* Utl_Xml_Node_CreateProcessingInstruction(const char_t* target, const char_t* data);

/*************************************************************************************************/
/**
 * \brief Destroys a node and all descendants.
 *
 * \par Requirements:
 *  - 001 This function shall detach an attached node before freeing its subtree.
 *  - 002 This function shall free all attributes and owned strings in the subtree.
 *  - 003 This function shall fail an assertion if node is NULL.
 *
 * \param[in,out] node   Node to destroy
 *
 * \retval None
 */
void Utl_Xml_Node_Destroy(Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Changes an element name or processing-instruction target.
 *
 * \par Requirements:
 *  - 001 This function shall accept only a complete UTF-8 XML 1.0 Name.
 *  - 002 This function shall reject every case of "xml" as a processing-instruction target.
 *  - 003 This function shall reject names longer than UTL_XML_NODE_MAX_NAME_BYTES.
 *  - 004 This function shall preserve the old name on invalid input or allocation failure.
 *  - 005 This function shall fail an assertion for NULL arguments or another node kind.
 *
 * \param[in,out] node   Element or processing instruction to change
 * \param[in]     name   Non-NULL null-terminated UTF-8 name
 *
 * \retval TRUE    if the name was set.
 * \retval FALSE   otherwise
 */
bool_t Utl_Xml_Node_SetName(Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Changes text, comment, or processing-instruction data.
 *
 * \par Requirements:
 *  - 001 This function shall accept only UTF-8 sequences of XML 1.0 Char values.
 *  - 002 This function shall enforce the comment and processing-instruction delimiter restrictions.
 *  - 003 This function shall reject values longer than UTL_XML_NODE_MAX_VALUE_BYTES.
 *  - 004 This function shall preserve the old value on invalid input or allocation failure.
 *  - 005 This function shall fail an assertion for NULL arguments or an element node.
 *
 * \param[in,out] node    Node to change
 * \param[in]     value   Non-NULL null-terminated UTF-8 value
 *
 * \retval TRUE    if the value was set.
 * \retval FALSE   otherwise
 */
bool_t Utl_Xml_Node_SetValue(Utl_Xml_Node_t* node, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Appends a node to an element's ordered children.
 *
 * \par Requirements:
 *  - 001 This function shall move an attached child from its old parent.
 *  - 002 This function shall reject cycles and connected trees larger than UTL_XML_NODE_MAX_NODES without modifying the tree.
 *  - 003 This function shall fail an assertion for NULL arguments or a non-element parent.
 *
 * \param[in,out] parent   Element receiving the child
 * \param[in,out] child    Node to move
 *
 * \retval TRUE    if the child was appended.
 * \retval FALSE   if the move would create a cycle
 */
bool_t Utl_Xml_Node_AppendChild(Utl_Xml_Node_t* parent, Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Inserts a node before an element child, or appends when beforeSibling is NULL.
 *
 * \par Requirements:
 *  - 001 This function shall move an attached child from its old parent.
 *  - 002 This function shall reject cycles, oversized trees, and invalid insertion positions without modifying the tree.
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
 * \brief Detaches a node from its parent without freeing it.
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
 * \brief Adds a distinct attribute to an element.
 *
 * \par Requirements:
 *  - 001 This function shall accept only a UTF-8 XML 1.0 Name and XML 1.0 Char value.
 *  - 002 This function shall reject a duplicate case-sensitive name.
 *  - 003 This function shall enforce the name, value, and UTL_XML_NODE_MAX_ATTRIBUTES limits.
 *  - 004 This function shall preserve attributes on failure.
 *  - 005 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in,out] node    Element receiving the attribute
 * \param[in]     name    Non-NULL null-terminated UTF-8 attribute name
 * \param[in]     value   Non-NULL null-terminated UTF-8 expanded value
 *
 * \retval TRUE    if the attribute was added.
 * \retval FALSE   if invalid, duplicate, or allocation failed
 */
bool_t Utl_Xml_Node_AddAttribute(Utl_Xml_Node_t* node, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Changes an attribute value or adds the attribute if absent.
 *
 * \par Requirements:
 *  - 001 This function shall accept only a UTF-8 XML 1.0 Name and XML 1.0 Char value.
 *  - 002 This function shall preserve insertion order when replacing a value.
 *  - 003 This function shall enforce the name, value, and UTL_XML_NODE_MAX_ATTRIBUTES limits.
 *  - 004 This function shall preserve attributes on failure.
 *  - 005 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in,out] node    Element receiving the attribute
 * \param[in]     name    Non-NULL null-terminated UTF-8 attribute name
 * \param[in]     value   Non-NULL null-terminated UTF-8 expanded value
 *
 * \retval TRUE    if the attribute was set.
 * \retval FALSE   if invalid or allocation failed
 */
bool_t Utl_Xml_Node_SetAttribute(Utl_Xml_Node_t* node, const char_t* name, const char_t* value);

/*************************************************************************************************/
/**
 * \brief Removes an attribute by case-sensitive name.
 *
 * \par Requirements:
 *  - 001 This function shall return FALSE when the attribute is absent.
 *  - 002 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in,out] node   Element to change
 * \param[in]     name   Non-NULL null-terminated attribute name
 *
 * \retval TRUE    if the attribute was removed.
 * \retval FALSE   if it was absent
 */
bool_t Utl_Xml_Node_RemoveAttribute(Utl_Xml_Node_t* node, const char_t* name);

/*************************************************************************************************/
/**
 * \brief Gets an attribute value by case-sensitive name.
 *
 * \par Requirements:
 *  - 001 This function shall return NULL when the attribute is absent.
 *  - 002 This function shall fail an assertion for NULL arguments or a non-element node.
 *
 * \param[in] node   Element to inspect
 * \param[in] name   Non-NULL null-terminated attribute name
 *
 * \return Node-owned UTF-8 value or NULL
 */
const char_t* Utl_Xml_Node_GetAttributeValue(const Utl_Xml_Node_t* node, const char_t* name);

#endif /* UTL_XML_NODE_H */
