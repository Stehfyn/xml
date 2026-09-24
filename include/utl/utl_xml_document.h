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

#define UTL_XML_DOCUMENT_MAX_NODES (65536) ///< Maximum nodes visited in one document traversal.

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
 *  XML declaration nodes may use value for optional encoding and standalone
 *  fields. DOCTYPE nodes use name for the root element type and may use value
 *  for an ExternalID and/or internal subset. The caller supplies valid suffix
 *  grammar without a leading space.
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
 * \brief Writes the document into caller-provided UTF-8 byte storage.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [1] document:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *
 *  XML declaration markup is written with version="1.0" and optional fields.
 *  DOCTYPE markup is written from its name and optional suffix.
 *  The caller keeps nodes, attributes, and borrowed byte spans valid while
 *  serializing, with output storage disjoint from them.
 *
 * \par Requirements:
 *  - 001 This function shall fail an assertion if document or output is NULL.
 *  - 002 This function shall fail an assertion for invalid element, DOCTYPE, attribute, or processing-instruction names and invalid text, attribute, comment, or processing-instruction content.
 *  - 003 This function shall construct XML declaration and DOCTYPE markup from their node data.
 *  - 004 This function shall write nodes, children, and attributes in append order, escaping text and attribute values.
 *  - 005 This function shall return the number of bytes written, without writing a zero terminator.
 *  - 006 This function shall fail an assertion before writing beyond capacity.
 *
 * \param[in]  document   Document to write
 * \param[out] output     Caller-provided byte storage
 * \param[in]  capacity   Number of bytes available in output
 *
 * \return Number of bytes written.
 */
uint64_t Utl_Xml_Document_Write(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity);

#endif /* UTL_XML_DOCUMENT_H */
