#ifndef FACEOE_UTL_XML_H
#define FACEOE_UTL_XML_H

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

/* Arbitrarily chosen storage and traversal limits; these are not XML specification limits. */
#define FACEOE_UTL_XML_MAX_NAME_BYTES          (1024UL)    ///< Maximum UTF-8 bytes in an element or attribute name, excluding a zero terminator
#define FACEOE_UTL_XML_MAX_VALUE_BYTES         (1048576UL) ///< Maximum UTF-8 bytes in an attribute value, excluding quotes and a zero terminator
#define FACEOE_UTL_XML_NODE_MAX_ATTRIBUTES     (256UL)     ///< Maximum attributes on one element.
#define FACEOE_UTL_XML_NODE_MAX_DEPTH          (65536UL)   ///< Maximum ancestor links checked when appending a child.
#define FACEOE_UTL_XML_DOCUMENT_MAX_NODES      (4096UL)    ///< Maximum element nodes visited by the writer or markup spans read by the reader.

/* Nominally one document element, or two top-level nodes when an XML declaration provides version information. */
#define FACEOE_UTL_XML_DOCUMENT_MAX_ROOT_NODES (2UL)       ///< Maximum top-level nodes: one document element plus an optional XML declaration

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief XML 1.0 node type.
 */
typedef enum FACEOE_Utl_Xml_NodeType_e
{
    FACEOE_UTL_XML_NODE_XML_DECLARATION,
    FACEOE_UTL_XML_NODE_ELEMENT
} FACEOE_Utl_Xml_NodeType_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned XML attribute for an element's forward list.
 */
typedef struct FACEOE_Utl_Xml_Attribute_s
{
    const byte_t*                      name;           ///< Pointer to UTF-8 XML name bytes
    uint32_t                           nameByteCount;  ///< Number of name bytes.
    const byte_t*                      value;          ///< Pointer to UTF-8 XML value bytes
    uint32_t                           valueByteCount; ///< Number of value bytes.
    struct FACEOE_Utl_Xml_Attribute_s* next;           ///< Next attribute in insertion order, or NULL.
} FACEOE_Utl_Xml_Attribute_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned XML node with pointers to strings and forward links for serialization.
 */
typedef struct FACEOE_Utl_Xml_Node_s
{
    FACEOE_Utl_Xml_NodeType_t     type;           ///< Kind of XML node.
    const byte_t*                 name;           ///< Pointer to the element name; NULL for the XML declaration
    uint32_t                      nameByteCount;  ///< Number of name bytes; zero when name is NULL.
    struct FACEOE_Utl_Xml_Node_s* parent;         ///< Parent set when appended, or NULL.
    struct FACEOE_Utl_Xml_Node_s* firstChild;     ///< First child of an element, or NULL.
    struct FACEOE_Utl_Xml_Node_s* lastChild;      ///< Last child of an element, or NULL.
    struct FACEOE_Utl_Xml_Node_s* nextSibling;    ///< Next node at the same level, or NULL.
    FACEOE_Utl_Xml_Attribute_t*   firstAttribute; ///< First attribute of an element, or NULL.
    FACEOE_Utl_Xml_Attribute_t*   lastAttribute;  ///< Last attribute of an element, or NULL.
    uint32_t                      attributeCount; ///< Number of linked attributes.
} FACEOE_Utl_Xml_Node_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned XML 1.0 document built by appending top-level nodes in order.
 */
typedef struct FACEOE_Utl_Xml_Document_s
{
    FACEOE_Utl_Xml_Node_t* firstNode; ///< First top-level node, or NULL.
    FACEOE_Utl_Xml_Node_t* lastNode;  ///< Last top-level node, or NULL.
} FACEOE_Utl_Xml_Document_t;

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
 * \brief Checks a counted UTF-8 byte span for a directly writable XML attribute value.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), production [10] AttValue:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-AttValue
 *
 *  The writer uses double quotes and does not escape values. Apostrophes and > are allowed;
 *  <, &, and double quotes cannot appear literally. Tabs, line feeds, and carriage
 *  returns are permitted literally but require character references to preserve
 *  their value. This validator accepts only the directly writable, lossless subset.
 *  XML 1.0 section 3.3.3 describes normalization of literal attribute whitespace:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#AVNormalize
 *
 * \par Requirements:
 *  - 001 This function shall accept an empty span and reject invalid UTF-8 or non-Char code points.
 *  - 002 This function shall return FALSE for nonliteral characters and for tab, LF, and CR.
 *  - 003 This function shall fail an assertion if utf8Text is NULL or byteCount exceeds FACEOE_UTL_XML_MAX_VALUE_BYTES.
 *
 * \param[in] utf8Text    Non-NULL UTF-8 byte span
 * \param[in] byteCount   Number of bytes in the span
 *
 * \retval    TRUE        if the span can be written directly as an attribute value.
 * \retval    FALSE       otherwise
 */
bool_t FACEOE_Utl_Xml_IsAttributeValue(const byte_t* utf8Text, uint32_t byteCount);

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as an unlinked XML attribute.
 *
 * \par Requirements:
 *  - 001 This function shall store name, value, and their byte counts without checking XML syntax or length.
 *  - 002 This function shall store pointers to the byte spans without copying them; the caller shall keep the spans valid while referenced.
 *  - 003 This function shall clear the attribute's next link.
 *  - 004 This function shall fail an assertion if attribute, name, or value is NULL.
 *
 * \param[out] attribute        Uninitialized caller-owned attribute storage
 * \param[in]  name             Pointer to a UTF-8 name byte span
 * \param[in]  nameByteCount    Number of name bytes
 * \param[in]  value            Pointer to a UTF-8 value byte span
 * \param[in]  valueByteCount   Number of value bytes
 *
 * \retval     None
 */
void FACEOE_Utl_Xml_InitializeAttribute(FACEOE_Utl_Xml_Attribute_t* attribute, const byte_t* name, uint32_t nameByteCount, const byte_t* value, uint32_t valueByteCount);

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as an unlinked XML node.
 *
 * \par Requirements:
 *  - 001 This function shall store type, name, and its byte count without checking XML syntax or length.
 *  - 002 This function shall store the name pointer without copying its bytes; the caller shall keep a non-NULL name valid while referenced.
 *  - 003 This function shall clear the node's tree and attribute links.
 *  - 004 This function shall require a zero byte count when name is NULL.
 *  - 005 This function shall fail an assertion if node is NULL.
 *
 * \par Details:
 *  Elements use name. The XML declaration uses a NULL name.
 *
 * \param[out] node            Uninitialized caller-owned node storage
 * \param[in]  type            XML node kind
 * \param[in]  name            Pointer to a UTF-8 name byte span or NULL
 * \param[in]  nameByteCount   Number of name bytes
 *
 * \retval     None
 */
void FACEOE_Utl_Xml_InitializeNode(FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_NodeType_t type, const byte_t* name, uint32_t nameByteCount);

/*************************************************************************************************/
/**
 * \brief Initializes caller-provided storage as an empty XML document.
 *
 * \par Requirements:
 *  - 001 This function shall set firstNode and lastNode to NULL.
 *  - 002 This function shall fail an assertion if document is NULL.
 *
 * \param[out] document   Uninitialized document storage
 *
 * \retval     None
 */
void FACEOE_Utl_Xml_InitializeDocument(FACEOE_Utl_Xml_Document_t* document);

/*************************************************************************************************/
/**
 * \brief Appends an initialized caller-owned attribute while constructing a serializable element.
 *
 * \par Requirements:
 *  - 001 This function shall require a valid XML name and an XML character value without <, &, ", or whitespace other than spaces.
 *  - 002 This function shall enforce FACEOE_UTL_XML_NODE_MAX_ATTRIBUTES and preserve the pointers to the name and value byte spans.
 *  - 003 This function shall fail an assertion for NULL arguments, a non-element node, a duplicate name, or an attribute already linked to the element.
 *
 * \param[in,out] node        Element receiving the attribute
 * \param[in,out] attribute   Initialized, unlinked caller-owned attribute
 *
 * \retval        None
 */
void FACEOE_Utl_Xml_AppendAttribute(FACEOE_Utl_Xml_Node_t* node, FACEOE_Utl_Xml_Attribute_t* attribute);

/*************************************************************************************************/
/**
 * \brief Appends a caller-owned child while constructing a serializable element.
 *
 * \par Requirements:
 *  - 001 This function shall preserve the order in which children are appended.
 *  - 002 This function shall check at most FACEOE_UTL_XML_NODE_MAX_DEPTH ancestor links.
 *  - 003 This function shall fail an assertion for NULL arguments, a non-element parent or child, an attached child, or a cycle.
 *
 * \param[in,out] parent   Element receiving the child
 * \param[in,out] child    Unlinked caller-owned child
 *
 * \retval        None
 */
void FACEOE_Utl_Xml_AppendChild(FACEOE_Utl_Xml_Node_t* parent, FACEOE_Utl_Xml_Node_t* child);

/*************************************************************************************************/
/**
 * \brief Appends a caller-owned top-level node in source order.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), productions [1] document and [23] XMLDecl:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-XMLDecl
 *
 * \par Requirements:
 *  - 001 This function shall link node after lastNode, set firstNode if empty, and update lastNode.
 *  - 002 This function shall fail an assertion if document or node is NULL.
 *  - 003 This function shall fail an assertion if node has a parent or next sibling, or is document->lastNode.
 *  - 004 This function shall fail an assertion for a type other than XML declaration or element.
 *  - 005 This function shall fail an assertion unless an XML declaration is the first node.
 *  - 006 This function shall fail an assertion for a second element.
 *
 * \param[in,out] document   Document receiving the node
 * \param[in,out] node       Unlinked caller-owned node
 *
 * \retval        None
 */
void FACEOE_Utl_Xml_AppendNode(FACEOE_Utl_Xml_Document_t* document, FACEOE_Utl_Xml_Node_t* node);

/*************************************************************************************************/
/**
 * \brief Reads a UTF-8 element tree in the form FACEOE_Utl_Xml_WriteDocument writes into caller-owned nodes and attributes.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), productions [1] document, [23] XMLDecl, and [39] element:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-XMLDecl
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-element
 *
 *  The input buffer holds an optional XML declaration with version="1.0" and no other
 *  fields, then one element tree. Attribute values are double-quoted and contain
 *  no references and no whitespace other than spaces. Whitespace between tags is
 *  not stored, so writing the document reproduces the writer's formatting.
 *  The reader writes a zero terminator at bufferToReadDocumentFrom[sizeOfDocumentBuffer]
 *  and uses FACEOE_Utl_StrStr to find each <. Name and value pointers refer to bytes in the buffer.
 *  The caller keeps bufferToReadDocumentFrom, nodes, and attributes valid while the
 *  document is used.
 *
 * \par Requirements:
 *  - 001 This function shall append nodes and attributes in source order.
 *  - 002 This function shall return zero for input outside the supported form or for insufficient storage.
 *  - 003 This function shall fail an assertion if document, bufferToReadDocumentFrom, nodes, or attributes is NULL, or if document is not empty.
 *
 * \param[in,out] document                   Initialized, empty document
 * \param[in,out] bufferToReadDocumentFrom   UTF-8 XML bytes with one writable byte after sizeOfDocumentBuffer bytes
 * \param[in]     sizeOfDocumentBuffer       Number of XML bytes to read, excluding the extra writable byte
 * \param[out]    nodes                      Caller-owned node storage
 * \param[in]     nodeCapacity               Number of nodes in nodes
 * \param[out]    attributes                 Caller-owned attribute storage
 * \param[in]     attributeCapacity          Number of attributes in attributes
 *
 * \retval        uint64_t   Number of bytes read, or zero on failure. The document may contain partial results on failure.
 */
uint64_t FACEOE_Utl_Xml_ReadDocument(FACEOE_Utl_Xml_Document_t* document, byte_t* bufferToReadDocumentFrom, uint64_t sizeOfDocumentBuffer, FACEOE_Utl_Xml_Node_t* nodes, uint32_t nodeCapacity, FACEOE_Utl_Xml_Attribute_t* attributes, uint32_t attributeCapacity);

/*************************************************************************************************/
/**
 * \brief Writes the document into caller-provided UTF-8 byte storage.
 *
 * \par Details:
 *  XML 1.0 (Fifth Edition), productions [1] document and [23] XMLDecl:
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-document
 *  https://www.w3.org/TR/2008/REC-xml-20081126/#NT-XMLDecl
 *
 *  Production [1] requires one document element. This function also writes the
 *  current node sequence before that element has been appended; such output is
 *  not a well-formed XML document.
 *
 *  XML declaration markup is written with version="1.0".
 *  The writer uses LF line endings and four spaces per nesting level.
 *  The caller keeps nodes, attributes, and the byte spans they point to valid while
 *  serializing, with bufferToWriteDocumentIn disjoint from them.
 *
 * \par Requirements:
 *  - 001 This function shall construct XML declaration markup.
 *  - 002 This function shall write nodes, children, and attributes in append order.
 *  - 003 This function shall return the number of bytes written, without writing a zero terminator.
 *  - 004 This function shall place nested nodes on indented LF-separated lines.
 *  - 005 This function shall return zero for invalid element names or attribute content.
 *  - 006 This function shall return zero before writing beyond sizeOfDocumentBuffer.
 *  - 007 This function shall fail an assertion if document or bufferToWriteDocumentIn is NULL.
 *
 * \param[in]  document                  Document to write
 * \param[out] bufferToWriteDocumentIn   Caller-provided byte storage
 * \param[in]  sizeOfDocumentBuffer      Number of bytes available in bufferToWriteDocumentIn
 *
 * \retval    uint64_t   Number of bytes written, or zero on failure. The buffer may contain partial output on failure.
 */
uint64_t FACEOE_Utl_Xml_WriteDocument(const FACEOE_Utl_Xml_Document_t* document, byte_t* bufferToWriteDocumentIn, uint64_t sizeOfDocumentBuffer);

#endif /* FACEOE_UTL_XML_H */
