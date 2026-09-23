# XML reading and writing

## Goal and scope

Build a conventional C90 XML reader and writer on the XML 1.0 (Fifth Edition)
character and name helpers in `utl_xml`. The resulting XML tree must represent
nested elements, attributes, and ordered content. ARINC 661 Definition Files are
an eventual consumer of this generic layer; widget types and ARINC rules do not
belong in the XML model.

The character helpers classify decoded Unicode code points. The string and
name validators decode counted UTF-8 byte spans. `utl_xml_document` builds a
document sequentially and serializes it; no parser is implemented yet.

## Tree model

The node and document modules provide the tree model and a serializer.

- `utl_xml_node` represents elements, text, comments, processing instructions,
  and the ordered children of an element. Element attributes form a separate
  collection. The model must retain text before, between, and after child
  elements.
- `utl_xml_document` borrows caller-owned nodes and represents the XML declaration,
  optional document type declaration, one root element, and document-level
  comments and processing instructions.

The caller owns node storage and UTF-8 byte spans. Tree operations preserve parent-child
relationships and reject cycles, duplicate attributes, and a second root.
Malformed XML read from input must produce a recoverable parse error; assertions
remain for misuse of the C API.

The node initializer stores the chosen node type and borrows its byte spans without
validating XML syntax. The document serializer validates node content, including
element and processing-instruction names, UTF-8 and XML character rules,
comment and processing-instruction delimiters, and the reserved `xml` target.
It escapes text and attribute values. The XML declaration and DOCTYPE are
borrowed complete markup spans whose internal grammar remains the caller's
responsibility.

## Naming

The public functions already use the `Utl_Xml_` prefix. The existing
`XmlCodePointRange_t` is private to `utl_xml.c` and does not establish a public
type convention. Public node and document types therefore need the same prefix;
`XmlNode_t` and `XmlDocument_t` are not suitable public names. Proposed names are
`Utl_Xml_Node_t` and `Utl_Xml_Document_t`, with module functions named
`Utl_Xml_Node_*` and `Utl_Xml_Document_*`. These names retain the established
`Utl_Xml_` prefix and make the entity boundary explicit.

## Remaining work

1. Implement an XML reader that builds this tree and reports malformed input.
2. Parse and validate XML declarations and DOCTYPE content instead of relying on
   caller-supplied complete markup.
3. Add specification tests for accepted and rejected documents and semantic
   read-write-read round trips.

Open decisions before claiming complete XML processor conformance include input
encodings, DTD and entity handling, namespace support, and whether source-level
details such as CDATA boundaries must survive a round trip. XML 1.0 (Fifth
Edition): https://www.w3.org/TR/2008/REC-xml-20081126/
