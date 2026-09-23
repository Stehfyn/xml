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
 * \brief Caller-owned XML document with ordered, borrowed top-level nodes.
 *
 * \par Details:
 *  xmlDeclaration and doctype are complete UTF-8 markup spans, including their delimiters.
 *  They and the nodes remain owned by the caller. nodeBeforeDoctype records where the
 *  optional DOCTYPE occurs in the top-level sequence; NULL places it before firstNode.
 *  Declaration and DOCTYPE markup are serialized verbatim. The caller supplies
 *  complete, well-formed markup for those two spans.
 */
typedef struct Utl_Xml_Document_s
{
    const byte_t*    xmlDeclaration;          ///< Complete XML declaration, or NULL.
    uint32_t         xmlDeclarationByteCount; ///< Number of XML declaration bytes.
    const byte_t*    doctype;                 ///< Complete DOCTYPE declaration, or NULL.
    uint32_t         doctypeByteCount;        ///< Number of DOCTYPE bytes.
    Utl_Xml_Node_t*  nodeBeforeDoctype;       ///< Last node before DOCTYPE, or NULL.
    Utl_Xml_Node_t*  firstNode;               ///< First top-level node, or NULL.
    Utl_Xml_Node_t*  lastNode;                ///< Last top-level node, or NULL.
    Utl_Xml_Node_t*  root;                    ///< Sole document element, or NULL while building.
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
 * \param[out] document  Uninitialized caller-owned document storage
 *
 * \retval None
 */
void Utl_Xml_Document_Initialize(Utl_Xml_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Stores an optional complete XML declaration as a borrowed byte span.
 *
 * \par Requirements:
 *  - 001 This function shall require the declaration to precede every top-level node and DOCTYPE.
 *  - 002 This function shall fail an assertion for NULL arguments, an empty span, or a second declaration.
 *  - 003 This function shall borrow the markup without parsing it.
 *
 * \param[in,out] document   Document receiving the declaration
 * \param[in]     utf8Text   Complete XML declaration, including delimiters
 * \param[in]     byteCount  Number of declaration bytes
 *
 * \retval None
 */
void Utl_Xml_Document_SetXmlDeclaration(Utl_Xml_Document_t* document, const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Stores an optional complete DOCTYPE declaration at the current prolog position.
 *
 * \par Requirements:
 *  - 001 This function shall require DOCTYPE to precede the root element.
 *  - 002 This function shall fail an assertion for NULL arguments, an empty span, or a second DOCTYPE.
 *  - 003 This function shall borrow the markup without parsing it.
 *
 * \param[in,out] document   Document receiving the DOCTYPE
 * \param[in]     utf8Text   Complete DOCTYPE declaration, including delimiters
 * \param[in]     byteCount  Number of DOCTYPE bytes
 *
 * \retval None
 */
void Utl_Xml_Document_SetDoctype(Utl_Xml_Document_t* document, const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Appends a top-level comment, processing instruction, or root element.
 *
 * \par Requirements:
 *  - 001 This function shall preserve the order of top-level nodes.
 *  - 002 This function shall allow one root element and comments or processing instructions around it.
 *  - 003 This function shall fail an assertion for NULL arguments, other node kinds, or a node already attached to a tree or this document.
 *  - 004 The caller shall not attach the same node to another document while this document uses it.
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
 * \par Requirements:
 *  - 001 This function shall require one root element and validate node content before writing.
 *  - 002 A NULL output with zero capacity shall measure the required byte count.
 *  - 003 A short output buffer shall remain untouched and byteCount shall report the required size.
 *  - 004 Output shall contain exactly byteCount bytes, without a zero terminator.
 *  - 005 The caller shall keep borrowed spans unchanged during serialization and use output storage that does not overlap them.
 *  - 006 This function shall reject serialized output longer than UINT32_MAX bytes.
 *
 * \par Details:
 *  The XML declaration and DOCTYPE spans are written verbatim after UTF-8 and
 *  XML-character validation; their internal grammars remain the caller's responsibility.
 *
 * \param[in]  document  Document to serialize
 * \param[out] output    Byte storage, or NULL to measure only
 * \param[in]  capacity  Number of bytes available in output
 * \param[out] byteCount Required or written byte count; zero for invalid documents
 *
 * \retval TRUE   if validation and measurement or serialization succeeded.
 * \retval FALSE  if the document is invalid or output is too small.
 */
bool_t Utl_Xml_Document_Serialize(const Utl_Xml_Document_t* document, byte_t* output, uint32_t capacity, uint32_t* byteCount);

#endif /* UTL_XML_DOCUMENT_H */
