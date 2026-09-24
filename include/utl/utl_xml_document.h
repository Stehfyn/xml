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
 *  - 002 This function shall clear the document's top-level node links.
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
 * \par Requirements:
 *  - 001 An XML declaration shall be first, if present.
 *  - 002 A DOCTYPE shall appear at most once and before the root element.
 *  - 003 At most one element may be appended; only comments and processing instructions may follow it.
 *  - 004 The node shall not already belong to a document or element.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), productions [1] document and [22] prolog:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-prolog
 *
 *  Initialize XML declaration and DOCTYPE nodes with their complete UTF-8 markup
 *  in value and a NULL name. The markup is borrowed and written verbatim after
 *  XML character and delimiter checks; its internal grammar is the caller's responsibility.
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
 * \par Requirements:
 *  - 001 This function shall require one root element and assert on invalid node content.
 *  - 002 This function shall assert if output or byteCount is NULL, or if capacity is exceeded.
 *  - 003 Output shall contain exactly byteCount bytes, without a zero terminator.
 *  - 004 The caller shall keep borrowed spans unchanged during serialization and use output storage that does not overlap them.
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
