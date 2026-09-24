#ifndef UTL_XML_DOCUMENT_H
#define UTL_XML_DOCUMENT_H

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

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Caller-owned XML document built by appending top-level nodes in order.
 */
typedef struct Utl_Xml_Document_s
{
    Utl_Xml_Node_t* firstNode; ///< First top-level node, or NULL.
    Utl_Xml_Node_t* lastNode;  ///< Last top-level node, or NULL.
} Utl_Xml_Document_t;

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
 * \brief Initializes caller-provided storage as an empty XML document.
 *
 * \par Requirements:
 *  - 001 This function shall fail an assertion if document is NULL.
 *  - 002 This function shall set firstNode and lastNode to NULL.
 *
 * \param[out] document  Uninitialized document storage
 *
 * \retval None
 */
void Utl_Xml_Document_Initialize(Utl_Xml_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Appends a caller-owned top-level node in source order.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), productions [1] document and [22] prolog:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-prolog
 *
 *  XML declaration and DOCTYPE nodes use value for complete UTF-8 markup and
 *  a NULL name. The caller supplies valid internal markup grammar.
 *
 * \par Requirements:
 *  - 001 This function shall fail an assertion if document or node is NULL.
 *  - 002 This function shall fail an assertion if node has a parent or next sibling, or is document->lastNode.
 *  - 003 This function shall fail an assertion for a type other than XML declaration, DOCTYPE, element, comment, or processing instruction.
 *  - 004 This function shall fail an assertion unless an XML declaration is the first node.
 *  - 005 This function shall fail an assertion for a second DOCTYPE or a DOCTYPE after an element.
 *  - 006 This function shall fail an assertion for a second element.
 *  - 007 This function shall link node after lastNode, set firstNode if empty, and update lastNode.
 *
 * \param[in,out] document  Document receiving the node
 * \param[in,out] node      Unlinked caller-owned node
 *
 * \retval None
 */
void Utl_Xml_Document_AppendNode(Utl_Xml_Document_t* document, Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Serializes the document into caller-provided UTF-8 byte storage.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [1] document:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *
 *  XML declaration and DOCTYPE markup is checked for XML characters and opening
 *  and closing delimiters; the caller supplies valid internal markup grammar.
 *  The caller keeps nodes, attributes, and borrowed byte spans valid while
 *  serializing, with output storage disjoint from them.
 *
 * \par Requirements:
 *  - 001 This function shall fail an assertion if document, output, or byteCount is NULL.
 *  - 002 This function shall fail an assertion unless the document has exactly one element and its lastNode matches the top-level list.
 *  - 003 This function shall require any XML declaration first, any DOCTYPE once and before the element, and only comments or processing instructions otherwise.
 *  - 004 This function shall fail an assertion for invalid XML names, UTF-8, XML characters, comment content, or processing instruction content or target.
 *  - 005 This function shall fail an assertion if declaration or DOCTYPE markup lacks its opening or closing delimiter.
 *  - 006 This function shall write nodes, children, and attributes in append order, escaping text and attribute values and writing declaration and DOCTYPE markup verbatim.
 *  - 007 This function shall set byteCount to the number of bytes written, without writing a zero terminator.
 *  - 008 This function shall fail an assertion before writing beyond capacity.
 *
 * \param[in]  document   Document to serialize
 * \param[out] output     Caller-provided byte storage
 * \param[in]  capacity   Number of bytes available in output
 * \param[out] byteCount  Number of bytes written
 *
 * \retval None
 */
void Utl_Xml_Document_Serialize(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity, uint32_t* byteCount);

#endif /* UTL_XML_DOCUMENT_H */
