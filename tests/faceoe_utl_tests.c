#include "faceoe_utl/faceoe_utl_defines.h"
#include "faceoe_utl/faceoe_utl_scan.h"
#include "faceoe_utl/faceoe_utl_xml.h"

#include "os/os_diagnostics.h"

/* Keep assertions enabled in Release builds. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdio.h>

/*************************************************************************************************/
/**
 * \brief Runs the ARINC 661 Definition File tests.
 *
 * \retval None
 */
void RunArinc661Tests(void);

/*************************************************************************************************/
/**
 * \brief Runs the FACE display management tests.
 *
 * \retval None
 */
void RunFACEOEDisplayManagementTests(void);

/*************************************************************************************************/
/**
 * \brief Runs the FACEOE graphics draw list tests.
 *
 * \retval None
 */
void RunFACEOEGfxTests(void);

/*************************************************************************************************/
/**
 * \brief Runs the ARINC 661 scene tests.
 *
 * \retval None
 */
void RunArinc661SceneTests(void);

/*************************************************************************************************/
/**
 * \brief Runs the editor updater tests.
 *
 * \retval None
 */
void RunArinc661UpdateTests(void);

/*************************************************************************************************/
/**
 * \par Details:
 *  Exercises XML name validation through the public document writer.
 */
static uint64_t WriteTestElement(const byte_t* name, uint32_t nameByteCount)
{
    FACEOE_Utl_Xml_Document_t document;
    FACEOE_Utl_Xml_Node_t     root;
    byte_t            output[FACEOE_UTL_XML_MAX_NAME_BYTES + sizeof("</>\n")];

    FACEOE_Utl_Xml_InitializeDocument(&document);
    FACEOE_Utl_Xml_InitializeNode(&root, FACEOE_UTL_XML_NODE_ELEMENT, name, nameByteCount);
    FACEOE_Utl_Xml_AppendNode(&document, &root);

    return FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks counted UTF-8 strings and names.
 */
static void TestXmlByteSpans(void)
{
    static const byte_t empty[] = "";
    static const byte_t valid_text[] = "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    static const byte_t invalid_utf8[] = "\xC3";
    static const byte_t invalid_xml_char[] = "\x01";
    static const byte_t valid_name[] = "\xC3\xA9" "b";
    static const byte_t invalid_name[] = "1a";
    static const byte_t embedded_zero[] = {'a', 0, 'b'};
    static const byte_t unterminated[] = {'a', 'b'};
    static const byte_t plain_attribute_value[] = "A b'c>d";
    static const byte_t nonliteral_or_normalized[] = "<&\"\t\n\r";
    uint32_t            byte_index;

    assert(TRUE == FACEOE_Utl_Xml_IsAttributeValue(empty, 0));
    assert(TRUE == FACEOE_Utl_Xml_IsAttributeValue(valid_text, (uint32_t)(sizeof(valid_text) - 1)));
    assert(TRUE == FACEOE_Utl_Xml_IsAttributeValue(unterminated, (uint32_t)sizeof(unterminated)));
    assert(TRUE == FACEOE_Utl_Xml_IsAttributeValue(plain_attribute_value, (uint32_t)(sizeof(plain_attribute_value) - 1)));
    assert(FALSE == FACEOE_Utl_Xml_IsAttributeValue(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == FACEOE_Utl_Xml_IsAttributeValue(invalid_xml_char, (uint32_t)(sizeof(invalid_xml_char) - 1)));
    assert(FALSE == FACEOE_Utl_Xml_IsAttributeValue(embedded_zero, (uint32_t)sizeof(embedded_zero)));
    assert(FALSE == FACEOE_Utl_Xml_IsAttributeValue(valid_text, (uint32_t)sizeof(valid_text)));

    for (byte_index = 0; byte_index < (uint32_t)(sizeof(nonliteral_or_normalized) - 1); ++byte_index)
    {
        assert(FALSE == FACEOE_Utl_Xml_IsAttributeValue(&nonliteral_or_normalized[byte_index], 1));
    }

    assert(0 < WriteTestElement(valid_name, (uint32_t)(sizeof(valid_name) - 1)));
    assert(0 < WriteTestElement(unterminated, (uint32_t)sizeof(unterminated)));
    assert(0 == WriteTestElement(invalid_name, (uint32_t)(sizeof(invalid_name) - 1)));
    assert(0 == WriteTestElement(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(0 == WriteTestElement(embedded_zero, (uint32_t)sizeof(embedded_zero)));
    assert(0 == WriteTestElement(valid_name, (uint32_t)sizeof(valid_name)));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks caller-owned node initialization and forward links used during serialization.
 */
static void TestXmlNodeInitialization(void)
{
    FACEOE_Utl_Xml_Node_t      root;
    FACEOE_Utl_Xml_Node_t      child;
    FACEOE_Utl_Xml_Node_t      sibling;
    FACEOE_Utl_Xml_Attribute_t attribute;
    FACEOE_Utl_Xml_Attribute_t utf8_attribute;
    FACEOE_Utl_Xml_Attribute_t longer_attribute;
    const byte_t        name[] = "\xC3\xA9";
    const byte_t        child_name[] = "child";
    const byte_t        sibling_name[] = "sibling";
    const byte_t        attribute_name[] = "id";
    const byte_t        attribute_value[] = "7";
    const byte_t        utf8_name[] = "\xC3\xA9";
    const byte_t        utf8_value[] = "A\xE2\x82\xAC\xF0\x9F\x98\x80" "Z";
    const byte_t        longer_name[] = "id2";

    FACEOE_Utl_Xml_InitializeNode(&root, FACEOE_UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1));
    FACEOE_Utl_Xml_InitializeNode(&child, FACEOE_UTL_XML_NODE_ELEMENT, child_name, (uint32_t)(sizeof(child_name) - 1));
    FACEOE_Utl_Xml_InitializeNode(&sibling, FACEOE_UTL_XML_NODE_ELEMENT, sibling_name, (uint32_t)(sizeof(sibling_name) - 1));

    assert(FACEOE_UTL_XML_NODE_ELEMENT == root.type);
    assert(name == root.name);
    assert((sizeof(name) - 1) == root.nameByteCount);
    assert(NULL == root.firstChild);
    assert(NULL == root.nextSibling);
    assert(NULL == root.firstAttribute);
    assert((sizeof(child_name) - 1) == child.nameByteCount);
    assert(NULL == child.parent);

    attribute.next = &utf8_attribute;
    FACEOE_Utl_Xml_InitializeAttribute(&attribute, attribute_name, (uint32_t)(sizeof(attribute_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    assert(attribute_name == attribute.name);
    assert((sizeof(attribute_name) - 1) == attribute.nameByteCount);
    assert(attribute_value == attribute.value);
    assert((sizeof(attribute_value) - 1) == attribute.valueByteCount);
    assert(NULL == attribute.next);
    FACEOE_Utl_Xml_AppendAttribute(&root, &attribute);
    FACEOE_Utl_Xml_InitializeAttribute(&utf8_attribute, utf8_name, (uint32_t)(sizeof(utf8_name) - 1), utf8_value, (uint32_t)(sizeof(utf8_value) - 1));
    FACEOE_Utl_Xml_AppendAttribute(&root, &utf8_attribute);
    FACEOE_Utl_Xml_InitializeAttribute(&longer_attribute, longer_name, (uint32_t)(sizeof(longer_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    FACEOE_Utl_Xml_AppendAttribute(&root, &longer_attribute);
    FACEOE_Utl_Xml_AppendChild(&root, &child);
    FACEOE_Utl_Xml_AppendChild(&root, &sibling);

    assert(&attribute == root.firstAttribute);
    assert(&utf8_attribute == attribute.next);
    assert(&longer_attribute == utf8_attribute.next);
    assert(&longer_attribute == root.lastAttribute);
    assert(3 == root.attributeCount);
    assert((sizeof(utf8_name) - 1) == utf8_attribute.nameByteCount);
    assert((sizeof(utf8_value) - 1) == utf8_attribute.valueByteCount);
    assert(&child == root.firstChild);
    assert(&sibling == child.nextSibling);
    assert(&sibling == root.lastChild);
    assert(&root == sibling.parent);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Node initializers store names that the writer will validate later.
 */
static void TestDeferredNodeContentValidation(void)
{
    FACEOE_Utl_Xml_Node_t element;
    const byte_t   name[] = "bad name";

    FACEOE_Utl_Xml_InitializeNode(&element, FACEOE_UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1));

    assert(name == element.name);
    assert((sizeof(name) - 1) == element.nameByteCount);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds a document in source order, checks its serialized bytes, and writes an XML artifact.
 */
static void TestXmlDocumentWrite(void)
{
    FACEOE_Utl_Xml_Document_t  document;
    FACEOE_Utl_Xml_Node_t      declaration_node;
    FACEOE_Utl_Xml_Node_t      root;
    FACEOE_Utl_Xml_Node_t      child;
    FACEOE_Utl_Xml_Node_t      grandchild;
    FACEOE_Utl_Xml_Node_t      sibling;
    FACEOE_Utl_Xml_Attribute_t attribute;
    FACEOE_Utl_Xml_Attribute_t empty_attribute;
    const byte_t        root_name[] = "root";
    const byte_t        child_name[] = "child";
    const byte_t        grandchild_name[] = "grandchild";
    const byte_t        sibling_name[] = "sibling";
    const byte_t        attribute_name[] = "a";
    const byte_t        attribute_value[] = "A > \xC3\xA9";
    const byte_t        empty_name[] = "b";
    const byte_t        empty_value[] = "";
    const byte_t        expected[] =
        "<?xml version=\"1.0\"?>\n<root a=\"A > \xC3\xA9\" b=\"\">\n    <child>\n        <grandchild/>\n    </child>\n    <sibling/>\n</root>\n";
    byte_t              output[256];
    FILE*               xml_file;
    uint64_t            byte_count;
    uint64_t            bytes_written;
    int                 close_result;

    FACEOE_Utl_Xml_InitializeDocument(&document);
    assert(NULL == document.firstNode);
    assert(NULL == document.lastNode);

    FACEOE_Utl_Xml_InitializeNode(&declaration_node, FACEOE_UTL_XML_NODE_XML_DECLARATION, NULL, 0);
    FACEOE_Utl_Xml_InitializeNode(&root, FACEOE_UTL_XML_NODE_ELEMENT, root_name, (uint32_t)(sizeof(root_name) - 1));
    FACEOE_Utl_Xml_InitializeNode(&child, FACEOE_UTL_XML_NODE_ELEMENT, child_name, (uint32_t)(sizeof(child_name) - 1));
    FACEOE_Utl_Xml_InitializeNode(&grandchild, FACEOE_UTL_XML_NODE_ELEMENT, grandchild_name, (uint32_t)(sizeof(grandchild_name) - 1));
    FACEOE_Utl_Xml_InitializeNode(&sibling, FACEOE_UTL_XML_NODE_ELEMENT, sibling_name, (uint32_t)(sizeof(sibling_name) - 1));

    FACEOE_Utl_Xml_InitializeAttribute(&attribute, attribute_name, (uint32_t)(sizeof(attribute_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    FACEOE_Utl_Xml_AppendAttribute(&root, &attribute);
    FACEOE_Utl_Xml_InitializeAttribute(&empty_attribute, empty_name, (uint32_t)(sizeof(empty_name) - 1), empty_value, 0);
    FACEOE_Utl_Xml_AppendAttribute(&root, &empty_attribute);
    FACEOE_Utl_Xml_AppendChild(&root, &child);
    FACEOE_Utl_Xml_AppendChild(&child, &grandchild);
    FACEOE_Utl_Xml_AppendChild(&root, &sibling);

    FACEOE_Utl_Xml_AppendNode(&document, &declaration_node);
    FACEOE_Utl_Xml_AppendNode(&document, &root);

    assert(&declaration_node == document.firstNode);
    assert(&root == declaration_node.nextSibling);
    assert(NULL == root.nextSibling);
    assert(&root == document.lastNode);

    memset(output, 0xA5, sizeof(output));
    byte_count = FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
    assert((sizeof(expected) - 1) == byte_count);
    assert(0 == memcmp(output, expected, byte_count));
    assert(0xA5 == output[byte_count]);

    xml_file = fopen("faceoe_utl_xml_document.xml", "wb");
    assert(NULL != xml_file);
    bytes_written = fwrite(output, 1, (size_t)byte_count, xml_file);
    assert(byte_count == bytes_written);
    close_result = fclose(xml_file);
    assert(0 == close_result);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes a schema-instance declaration and a qualified schema-location attribute.
 */
static void TestXmlSchemaLocationWrite(void)
{
    FACEOE_Utl_Xml_Document_t  document;
    FACEOE_Utl_Xml_Node_t      configuration;
    FACEOE_Utl_Xml_Node_t      setting;
    FACEOE_Utl_Xml_Attribute_t namespace_attribute;
    FACEOE_Utl_Xml_Attribute_t schema_attribute;
    const byte_t        configuration_name[] = "configuration";
    const byte_t        setting_name[] = "setting";
    const byte_t        namespace_name[] = "xmlns:xsi";
    const byte_t        namespace_uri[] = "http://www.w3.org/2001/XMLSchema-instance";
    const byte_t        schema_name[] = "xsi:noNamespaceSchemaLocation";
    const byte_t        schema_location[] = "H3.xsd";
    const byte_t        expected[] =
        "<configuration xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
        "xsi:noNamespaceSchemaLocation=\"H3.xsd\">\n"
        "    <setting/>\n"
        "</configuration>\n";
    byte_t              output[sizeof(expected)];
    uint64_t            byte_count;

    FACEOE_Utl_Xml_InitializeDocument(&document);
    FACEOE_Utl_Xml_InitializeNode(&configuration, FACEOE_UTL_XML_NODE_ELEMENT, configuration_name, (uint32_t)(sizeof(configuration_name) - 1));
    FACEOE_Utl_Xml_InitializeNode(&setting, FACEOE_UTL_XML_NODE_ELEMENT, setting_name, (uint32_t)(sizeof(setting_name) - 1));
    FACEOE_Utl_Xml_InitializeAttribute(&namespace_attribute, namespace_name, (uint32_t)(sizeof(namespace_name) - 1), namespace_uri, (uint32_t)(sizeof(namespace_uri) - 1));
    FACEOE_Utl_Xml_InitializeAttribute(&schema_attribute, schema_name, (uint32_t)(sizeof(schema_name) - 1), schema_location, (uint32_t)(sizeof(schema_location) - 1));

    FACEOE_Utl_Xml_AppendAttribute(&configuration, &namespace_attribute);
    FACEOE_Utl_Xml_AppendAttribute(&configuration, &schema_attribute);
    FACEOE_Utl_Xml_AppendChild(&configuration, &setting);
    FACEOE_Utl_Xml_AppendNode(&document, &configuration);

    byte_count = FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
    assert((sizeof(expected) - 1) == byte_count);
    assert(0 == memcmp(output, expected, byte_count));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Reads an ARINC 661 Definition File excerpt and checks that writing it reproduces the input.
 */
static void TestXmlDocumentRoundTrip(void)
{
    static const char_t text[] =
        "<?xml version=\"1.0\"?>\n"
        "<a661_df name=\"arinc661_definition\" library_version=\"0\" supp_version=\"6\">\n"
        "    <model>\n"
        "        <prop name=\"ApplicationId\" value=\"1\"/>\n"
        "    </model>\n"
        "    <a661_layer name=\"PrimaryLayer\">\n"
        "        <a661_widget name=\"StatusLabel\" type=\"A661_LABEL\">\n"
        "            <model>\n"
        "                <prop name=\"LabelString\" value=\"SYSTEM READY\"/>\n"
        "            </model>\n"
        "        </a661_widget>\n"
        "    </a661_layer>\n"
        "</a661_df>\n";
    FACEOE_Utl_Xml_Document_t  document;
    FACEOE_Utl_Xml_Node_t      nodes[8];
    FACEOE_Utl_Xml_Attribute_t attributes[10];
    byte_t              input[sizeof(text)];
    byte_t              output[sizeof(text)];
    uint64_t            byte_count;

    memcpy(input, text, sizeof(text));
    FACEOE_Utl_Xml_InitializeDocument(&document);
    byte_count = FACEOE_Utl_Xml_ReadDocument(&document, input, (uint64_t)(sizeof(text) - 1), nodes, (uint32_t)COUNT_OF(nodes), attributes, (uint32_t)COUNT_OF(attributes));
    assert((sizeof(text) - 1) == byte_count);

    byte_count = FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
    assert((sizeof(text) - 1) == byte_count);
    assert(0 == memcmp(output, text, byte_count));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Returns zero for invalid XML and insufficient storage.
 */
static void TestXmlDocumentFailures(void)
{
    FACEOE_Utl_Xml_Document_t  document;
    FACEOE_Utl_Xml_Node_t      root;
    FACEOE_Utl_Xml_Node_t      nodes[2];
    FACEOE_Utl_Xml_Attribute_t attributes[1];
    byte_t              name[] = "root";
    byte_t              output[sizeof("<root/>\n")];
    byte_t              bad_end_tag[] = "<root><child/></wrong>";
    byte_t              too_many_nodes[] = "<root><child/></root>";
    byte_t              trailing_text[] = "<root/>oops";
    uint64_t            byte_count;

    FACEOE_Utl_Xml_InitializeDocument(&document);
    FACEOE_Utl_Xml_InitializeNode(&root, FACEOE_UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1));
    FACEOE_Utl_Xml_AppendNode(&document, &root);

    memset(output, 0xA5, sizeof(output));
    byte_count = FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)(sizeof(output) - 2));
    assert(0 == byte_count);
    assert(0xA5 == output[sizeof(output) - 2]);

    byte_count = FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)(sizeof(output) - 1));
    assert((sizeof(output) - 1) == byte_count);
    assert(0 == memcmp(output, "<root/>\n", (size_t)byte_count));

    name[0] = '1';
    byte_count = FACEOE_Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
    assert(0 == byte_count);

    FACEOE_Utl_Xml_InitializeDocument(&document);
    byte_count = FACEOE_Utl_Xml_ReadDocument(&document, bad_end_tag, (uint64_t)(sizeof(bad_end_tag) - 1), nodes, (uint32_t)COUNT_OF(nodes), attributes, (uint32_t)COUNT_OF(attributes));
    assert(0 == byte_count);

    FACEOE_Utl_Xml_InitializeDocument(&document);
    byte_count = FACEOE_Utl_Xml_ReadDocument(&document, too_many_nodes, (uint64_t)(sizeof(too_many_nodes) - 1), nodes, 1, attributes, (uint32_t)COUNT_OF(attributes));
    assert(0 == byte_count);

    FACEOE_Utl_Xml_InitializeDocument(&document);
    byte_count = FACEOE_Utl_Xml_ReadDocument(&document, trailing_text, (uint64_t)(sizeof(trailing_text) - 1), nodes, (uint32_t)COUNT_OF(nodes), attributes, (uint32_t)COUNT_OF(attributes));
    assert(0 == byte_count);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks declaration placement, element closure, shared storage limits, and input preservation.
 */
static void TestXmlReaderBoundaries(void)
{
    static const struct
    {
        const char_t* text;
        uint32_t      nodeCapacity;
        uint32_t      attributeCapacity;
        bool_t        isValid;
    } cases[] = {
        {"<root/>", 1, 0, TRUE},
        {" \t<root><child/><child/></root>\r\n", 3, 0, TRUE},
        {"<?xml version=\"1.0\"?><root/>", 2, 0, TRUE},
        {"<?xml\tversion=\"1.0\"?>\n<root></root>", 2, 0, TRUE},
        {"<root></root >", 1, 0, TRUE},
        {"<root a=\"1\"><child b=\"2\"/></root>", 2, 2, TRUE},
        {"", 4, 2, FALSE},
        {" \t\n", 4, 2, FALSE},
        {"<?xml version=\"1.0\"?>", 4, 2, FALSE},
        {" <?xml version=\"1.0\"?><root/>", 4, 2, FALSE},
        {"<?xml version=\"1.1\"?><root/>", 4, 2, FALSE},
        {"<?xmlversion=\"1.0\"?><root/>", 4, 2, FALSE},
        {"<?xml version=\"1.0\"?><?xml version=\"1.0\"?><root/>", 4, 2, FALSE},
        {"<root><?xml version=\"1.0\"?></root>", 4, 2, FALSE},
        {"<root/><?xml version=\"1.0\"?>", 4, 2, FALSE},
        {"<root/><second/>", 4, 2, FALSE},
        {"</root>", 4, 2, FALSE},
        {"<root><child/></wrong>", 4, 2, FALSE},
        {"<root><child></root></child>", 4, 2, FALSE},
        {"<root><child/>", 4, 2, FALSE},
        {"<root/></root>", 4, 2, FALSE},
        {"<root/>text<second/>", 4, 2, FALSE},
        {"<root a=\"1\" a=\"2\"/><second/>", 4, 2, FALSE},
        {"<root a=\"&\"/><second/>", 4, 2, FALSE},
        {"<root/>", 0, 2, FALSE},
        {"<?xml version=\"1.0\"?><root/>", 1, 2, FALSE},
        {"<root><child/><child/></root>", 2, 2, FALSE},
        {"<root a=\"1\"><child b=\"2\"/></root>", 2, 1, FALSE}
    };
    FACEOE_Utl_Xml_Document_t  document;
    FACEOE_Utl_Xml_Node_t      nodes[4];
    FACEOE_Utl_Xml_Attribute_t attributes[2];
    byte_t             input[128];
    uint32_t           case_index;
    uint64_t           input_byte_count;
    uint64_t           bytes_read;

    for (case_index = 0; case_index < COUNT_OF(cases); ++case_index)
    {
        input_byte_count = (uint64_t)strlen(cases[case_index].text);
        memcpy(input, cases[case_index].text, (size_t)input_byte_count);
        input[input_byte_count] = 0xA5;
        FACEOE_Utl_Xml_InitializeDocument(&document);

        bytes_read = FACEOE_Utl_Xml_ReadDocument(&document, input, input_byte_count, nodes, cases[case_index].nodeCapacity, attributes, cases[case_index].attributeCapacity);

        assert((cases[case_index].isValid ? input_byte_count : 0) == bytes_read);
        assert(0 == memcmp(input, cases[case_index].text, (size_t)input_byte_count));
        assert(0 == input[input_byte_count]);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  The markup limit includes the declaration and both opening and closing element tags.
 */
static void TestXmlReaderMarkupLimit(void)
{
    static FACEOE_Utl_Xml_Node_t nodes[FACEOE_UTL_XML_DOCUMENT_MAX_NODES];
    static byte_t input[FACEOE_UTL_XML_DOCUMENT_MAX_NODES * 4 + sizeof("<?xml version=\"1.0\"?><r></r>")];
    static const char_t declaration[] = "<?xml version=\"1.0\"?>";
    FACEOE_Utl_Xml_Document_t  document;
    FACEOE_Utl_Xml_Attribute_t attributes[1];
    uint32_t           child_index;
    uint64_t           element_offset;
    uint64_t           position;
    uint64_t           bytes_read;

    element_offset = sizeof(declaration) - 1;
    memcpy(input, declaration, (size_t)element_offset);
    memcpy(input + element_offset, "<r>", 3);
    position = element_offset + 3;
    for (child_index = 0; child_index < FACEOE_UTL_XML_DOCUMENT_MAX_NODES - 2; ++child_index)
    {
        memcpy(input + position, "<c/>", 4);
        position += 4;
    }
    memcpy(input + position, "</r>", 4);
    position += 4;

    FACEOE_Utl_Xml_InitializeDocument(&document);
    bytes_read = FACEOE_Utl_Xml_ReadDocument(&document, input + element_offset, position - element_offset, nodes, (uint32_t)COUNT_OF(nodes), attributes, 0);
    assert(position - element_offset == bytes_read);

    FACEOE_Utl_Xml_InitializeDocument(&document);
    bytes_read = FACEOE_Utl_Xml_ReadDocument(&document, input, position, nodes, (uint32_t)COUNT_OF(nodes), attributes, 0);
    assert(0 == bytes_read);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks numeric prefixes, signed bounds, and XML Schema decimal syntax.
 */
static void TestNumberScanning(void)
{
    const char_t minimum[]         = "-9223372036854775808";
    const char_t maximum[]         = "+9223372036854775807";
    const char_t overflow[]        = "9223372036854775808";
    const char_t decimal[]         = "-.125";
    const char_t exponent[]        = "1e3";
    const char_t non_ascii_digit[] = {(char_t)0xB2};
    char_t       long_decimal[204];
    int64_t      integer;
    float64_t    fraction;
    int32_t      consumed;

    integer  = 0;
    consumed = FACEOE_Utl_Scan_Int64(minimum, &integer);
    assert((sizeof(minimum) - 1) == consumed);
    assert(INT64_MIN == integer);

    consumed = FACEOE_Utl_Scan_Int64(maximum, &integer);
    assert((sizeof(maximum) - 1) == consumed);
    assert(INT64_MAX == integer);

    consumed = FACEOE_Utl_Scan_Int64(overflow, &integer);
    assert(-1 == consumed);
    assert(INT64_MAX == integer);

    consumed = FACEOE_Utl_Scan_Int64(non_ascii_digit, &integer);
    assert(-1 == consumed);
    assert(INT64_MAX == integer);

    consumed = FACEOE_Utl_Scan_Int64("42tail", &integer);
    assert(2 == consumed);
    assert(42 == integer);

    consumed = FACEOE_Utl_Scan_Int64("+", &integer);
    assert(-1 == consumed);
    assert(42 == integer);

    fraction = 0.0;
    consumed = FACEOE_Utl_Scan_Float(decimal, &fraction);
    assert((sizeof(decimal) - 1) == consumed);
    assert(-0.125 == fraction);

    consumed = FACEOE_Utl_Scan_Float(exponent, &fraction);
    assert(1 == consumed);
    assert(1.0 == fraction);

    consumed = FACEOE_Utl_Scan_Float("2.5tail", &fraction);
    assert(3 == consumed);
    assert(2.5 == fraction);

    long_decimal[0] = '0';
    long_decimal[1] = '.';
    memset(long_decimal + 2, '0', 200);
    long_decimal[202] = '1';
    long_decimal[203] = '\0';
    consumed = FACEOE_Utl_Scan_Float(long_decimal, &fraction);
    assert(203 == consumed);
    assert(0.0 < fraction);

    consumed = FACEOE_Utl_Scan_Float(non_ascii_digit, &fraction);
    assert(-1 == consumed);
    assert(0.0 < fraction);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the XML, number scanning, and ARINC 661 utility checks.
 */
int main(void)
{
    // Route assertion and runtime failures to stderr instead of dialog boxes
    Os_Diagnostics_Initialize();
    TestNumberScanning();
    TestXmlByteSpans();
    TestXmlNodeInitialization();
    TestDeferredNodeContentValidation();
    TestXmlDocumentWrite();
    TestXmlSchemaLocationWrite();
    TestXmlDocumentRoundTrip();
    TestXmlDocumentFailures();
    TestXmlReaderBoundaries();
    TestXmlReaderMarkupLimit();
    RunArinc661Tests();
    RunFACEOEDisplayManagementTests();
    RunFACEOEGfxTests();
    RunArinc661SceneTests();
    RunArinc661UpdateTests();
    return 0;
}
