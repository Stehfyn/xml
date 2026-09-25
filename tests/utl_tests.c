#include "utl/utl_defines.h"
#include "utl/utl_scan.h"
#include "utl/utl_xml.h"

/* Keep assertions enabled in Release builds. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdio.h>

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks character classification against expected XML production membership at range boundaries,
 *  excluded gaps, whitespace distinctions, surrogate code points, and Unicode limits.
 */
static void TestCharacterClasses(void)
{
    /* Expected membership in XML 1.0 productions [2], [3], [4], and [4a]. */
    static const struct
    {
        uint32_t codePoint;
        bool_t   isChar;
        bool_t   isWhitespace;
        bool_t   isNameStartChar;
        bool_t   isNameChar;
    } cases[] = {{0x0000, 0, 0, 0, 0},
                 {0x0008, 0, 0, 0, 0},
                 {0x0009, 1, 1, 0, 0},
                 {0x000A, 1, 1, 0, 0},
                 {0x000B, 0, 0, 0, 0},
                 {0x000C, 0, 0, 0, 0},
                 {0x000D, 1, 1, 0, 0},
                 {0x000E, 0, 0, 0, 0},
                 {0x001F, 0, 0, 0, 0},
                 {0x0020, 1, 1, 0, 0},
                 {'&', 1, 0, 0, 0},
                 {'<', 1, 0, 0, 0},
                 {':', 1, 0, 1, 1},
                 {'_', 1, 0, 1, 1},
                 {'@', 1, 0, 0, 0},
                 {'A', 1, 0, 1, 1},
                 {'Z', 1, 0, 1, 1},
                 {'[', 1, 0, 0, 0},
                 {'`', 1, 0, 0, 0},
                 {'a', 1, 0, 1, 1},
                 {'z', 1, 0, 1, 1},
                 {'{', 1, 0, 0, 0},
                 {'-', 1, 0, 0, 1},
                 {'.', 1, 0, 0, 1},
                 {'/', 1, 0, 0, 0},
                 {'0', 1, 0, 0, 1},
                 {'9', 1, 0, 0, 1},
                 {0x007F, 1, 0, 0, 0},
                 {0x0085, 1, 0, 0, 0}, /* NEL is not XML 1.0 whitespace. */
                 {0x00A0, 1, 0, 0, 0}, /* Neither is nonbreaking space. */
                 {0x00B6, 1, 0, 0, 0},
                 {0x00B7, 1, 0, 0, 1},
                 {0x00B8, 1, 0, 0, 0},
                 {0x00BF, 1, 0, 0, 0},
                 {0x00C0, 1, 0, 1, 1},
                 {0x00D6, 1, 0, 1, 1},
                 {0x00D7, 1, 0, 0, 0},
                 {0x00D8, 1, 0, 1, 1},
                 {0x00F6, 1, 0, 1, 1},
                 {0x00F7, 1, 0, 0, 0},
                 {0x00F8, 1, 0, 1, 1},
                 {0x02FF, 1, 0, 1, 1},
                 {0x0300, 1, 0, 0, 1}, /* Combining marks cannot begin a Name. */
                 {0x036F, 1, 0, 0, 1},
                 {0x0370, 1, 0, 1, 1},
                 {0x037D, 1, 0, 1, 1},
                 {0x037E, 1, 0, 0, 0},
                 {0x037F, 1, 0, 1, 1},
                 {0x1FFF, 1, 0, 1, 1},
                 {0x2000, 1, 0, 0, 0},
                 {0x200B, 1, 0, 0, 0},
                 {0x200C, 1, 0, 1, 1},
                 {0x200D, 1, 0, 1, 1},
                 {0x200E, 1, 0, 0, 0},
                 {0x2028, 1, 0, 0, 0}, /* Line separator is not XML 1.0 whitespace. */
                 {0x203E, 1, 0, 0, 0},
                 {0x203F, 1, 0, 0, 1},
                 {0x2040, 1, 0, 0, 1},
                 {0x2041, 1, 0, 0, 0},
                 {0x206F, 1, 0, 0, 0},
                 {0x2070, 1, 0, 1, 1},
                 {0x218F, 1, 0, 1, 1},
                 {0x2190, 1, 0, 0, 0},
                 {0x2BFF, 1, 0, 0, 0},
                 {0x2C00, 1, 0, 1, 1},
                 {0x2FEF, 1, 0, 1, 1},
                 {0x2FF0, 1, 0, 0, 0},
                 {0x3000, 1, 0, 0, 0},
                 {0x3001, 1, 0, 1, 1},
                 {0xD7FF, 1, 0, 1, 1},
                 {0xD800, 0, 0, 0, 0}, /* Surrogates are not XML characters. */
                 {0xDFFF, 0, 0, 0, 0},
                 {0xE000, 1, 0, 0, 0},
                 {0xF8FF, 1, 0, 0, 0},
                 {0xF900, 1, 0, 1, 1},
                 {0xFDCF, 1, 0, 1, 1},
                 {0xFDD0, 1, 0, 0, 0}, /* Discouraged characters can still match Char. */
                 {0xFDEF, 1, 0, 0, 0},
                 {0xFDF0, 1, 0, 1, 1},
                 {0xFFFD, 1, 0, 1, 1},
                 {0xFFFE, 0, 0, 0, 0},
                 {0xFFFF, 0, 0, 0, 0},
                 {0x10000, 1, 0, 1, 1},
                 {0x1FFFF, 1, 0, 1, 1}, /* Non-BMP plane endings match Char and NameStartChar. */
                 {0xEFFFF, 1, 0, 1, 1},
                 {0xF0000, 1, 0, 0, 0},
                 {0x10FFFF, 1, 0, 0, 0},
                 {0x110000, 0, 0, 0, 0},
                 {UINT32_MAX, 0, 0, 0, 0}};

    uint32_t test_case_index;

    for (test_case_index = 0; test_case_index < COUNT_OF(cases); ++test_case_index)
    {
        uint32_t code_point = cases[test_case_index].codePoint;
        assert(Utl_Xml_IsChar(code_point) == cases[test_case_index].isChar);
        assert(Utl_Xml_IsWhitespace(code_point) == cases[test_case_index].isWhitespace);
        assert(Utl_Xml_IsNameStartChar(code_point) == cases[test_case_index].isNameStartChar);
        assert(Utl_Xml_IsNameChar(code_point) == cases[test_case_index].isNameChar);
    }
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
    static const byte_t reference_characters[] = "<&\"\t\n\r";
    uint32_t            byte_index;

    assert(TRUE == Utl_Xml_IsString(empty, 0));
    assert(TRUE == Utl_Xml_IsString(valid_text, (uint32_t)(sizeof(valid_text) - 1)));
    assert(TRUE == Utl_Xml_IsString(unterminated, (uint32_t)sizeof(unterminated)));
    assert(FALSE == Utl_Xml_IsString(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsString(invalid_xml_char, (uint32_t)(sizeof(invalid_xml_char) - 1)));
    assert(FALSE == Utl_Xml_IsString(embedded_zero, (uint32_t)sizeof(embedded_zero)));
    assert(FALSE == Utl_Xml_IsString(valid_text, (uint32_t)sizeof(valid_text)));

    assert(TRUE == Utl_Xml_IsAttributeValue(empty, 0));
    assert(TRUE == Utl_Xml_IsAttributeValue(valid_text, (uint32_t)(sizeof(valid_text) - 1)));
    assert(TRUE == Utl_Xml_IsAttributeValue(plain_attribute_value, (uint32_t)(sizeof(plain_attribute_value) - 1)));
    assert(FALSE == Utl_Xml_IsAttributeValue(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsAttributeValue(invalid_xml_char, (uint32_t)(sizeof(invalid_xml_char) - 1)));
    assert(FALSE == Utl_Xml_IsAttributeValue(embedded_zero, (uint32_t)sizeof(embedded_zero)));

    for (byte_index = 0; byte_index < (uint32_t)(sizeof(reference_characters) - 1); ++byte_index)
    {
        assert(FALSE == Utl_Xml_IsAttributeValue(&reference_characters[byte_index], 1));
    }

    assert(TRUE == Utl_Xml_IsName(valid_name, (uint32_t)(sizeof(valid_name) - 1)));
    assert(TRUE == Utl_Xml_IsName(unterminated, (uint32_t)sizeof(unterminated)));
    assert(FALSE == Utl_Xml_IsName(invalid_name, (uint32_t)(sizeof(invalid_name) - 1)));
    assert(FALSE == Utl_Xml_IsName(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsName(embedded_zero, (uint32_t)sizeof(embedded_zero)));
    assert(FALSE == Utl_Xml_IsName(valid_name, (uint32_t)sizeof(valid_name)));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks caller-owned node initialization and forward links used during serialization.
 */
static void TestXmlNodeInitialization(void)
{
    Utl_Xml_Node_t      root;
    Utl_Xml_Node_t      child;
    Utl_Xml_Node_t      sibling;
    Utl_Xml_Attribute_t attribute;
    Utl_Xml_Attribute_t utf8_attribute;
    Utl_Xml_Attribute_t longer_attribute;
    const byte_t        name[] = "\xC3\xA9";
    const byte_t        child_name[] = "child";
    const byte_t        sibling_name[] = "sibling";
    const byte_t        attribute_name[] = "id";
    const byte_t        attribute_value[] = "7";
    const byte_t        utf8_name[] = "\xC3\xA9";
    const byte_t        utf8_value[] = "A\xE2\x82\xAC\xF0\x9F\x98\x80" "Z";
    const byte_t        longer_name[] = "id2";

    Utl_Xml_InitializeNode(&root, UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1));
    Utl_Xml_InitializeNode(&child, UTL_XML_NODE_ELEMENT, child_name, (uint32_t)(sizeof(child_name) - 1));
    Utl_Xml_InitializeNode(&sibling, UTL_XML_NODE_ELEMENT, sibling_name, (uint32_t)(sizeof(sibling_name) - 1));

    assert(UTL_XML_NODE_ELEMENT == root.type);
    assert(name == root.name);
    assert((sizeof(name) - 1) == root.nameByteCount);
    assert(NULL == root.firstChild);
    assert(NULL == root.nextSibling);
    assert(NULL == root.firstAttribute);
    assert((sizeof(child_name) - 1) == child.nameByteCount);
    assert(NULL == child.parent);

    attribute.next = &utf8_attribute;
    Utl_Xml_InitializeAttribute(&attribute, attribute_name, (uint32_t)(sizeof(attribute_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    assert(attribute_name == attribute.name);
    assert((sizeof(attribute_name) - 1) == attribute.nameByteCount);
    assert(attribute_value == attribute.value);
    assert((sizeof(attribute_value) - 1) == attribute.valueByteCount);
    assert(NULL == attribute.next);
    Utl_Xml_AppendAttribute(&root, &attribute);
    Utl_Xml_InitializeAttribute(&utf8_attribute, utf8_name, (uint32_t)(sizeof(utf8_name) - 1), utf8_value, (uint32_t)(sizeof(utf8_value) - 1));
    Utl_Xml_AppendAttribute(&root, &utf8_attribute);
    Utl_Xml_InitializeAttribute(&longer_attribute, longer_name, (uint32_t)(sizeof(longer_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    Utl_Xml_AppendAttribute(&root, &longer_attribute);
    Utl_Xml_AppendChild(&root, &child);
    Utl_Xml_AppendChild(&root, &sibling);

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
    Utl_Xml_Node_t element;
    const byte_t   name[] = "bad name";

    Utl_Xml_InitializeNode(&element, UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1));

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
    Utl_Xml_Document_t  document;
    Utl_Xml_Node_t      declaration_node;
    Utl_Xml_Node_t      root;
    Utl_Xml_Node_t      child;
    Utl_Xml_Node_t      grandchild;
    Utl_Xml_Node_t      sibling;
    Utl_Xml_Attribute_t attribute;
    Utl_Xml_Attribute_t empty_attribute;
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

    Utl_Xml_InitializeDocument(&document);
    assert(NULL == document.firstNode);
    assert(NULL == document.lastNode);

    Utl_Xml_InitializeNode(&declaration_node, UTL_XML_NODE_XML_DECLARATION, NULL, 0);
    Utl_Xml_InitializeNode(&root, UTL_XML_NODE_ELEMENT, root_name, (uint32_t)(sizeof(root_name) - 1));
    Utl_Xml_InitializeNode(&child, UTL_XML_NODE_ELEMENT, child_name, (uint32_t)(sizeof(child_name) - 1));
    Utl_Xml_InitializeNode(&grandchild, UTL_XML_NODE_ELEMENT, grandchild_name, (uint32_t)(sizeof(grandchild_name) - 1));
    Utl_Xml_InitializeNode(&sibling, UTL_XML_NODE_ELEMENT, sibling_name, (uint32_t)(sizeof(sibling_name) - 1));

    Utl_Xml_InitializeAttribute(&attribute, attribute_name, (uint32_t)(sizeof(attribute_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    Utl_Xml_AppendAttribute(&root, &attribute);
    Utl_Xml_InitializeAttribute(&empty_attribute, empty_name, (uint32_t)(sizeof(empty_name) - 1), empty_value, 0);
    Utl_Xml_AppendAttribute(&root, &empty_attribute);
    Utl_Xml_AppendChild(&root, &child);
    Utl_Xml_AppendChild(&child, &grandchild);
    Utl_Xml_AppendChild(&root, &sibling);

    Utl_Xml_AppendNode(&document, &declaration_node);
    Utl_Xml_AppendNode(&document, &root);

    assert(&declaration_node == document.firstNode);
    assert(&root == declaration_node.nextSibling);
    assert(NULL == root.nextSibling);
    assert(&root == document.lastNode);

    memset(output, 0xA5, sizeof(output));
    byte_count = Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
    assert((sizeof(expected) - 1) == byte_count);
    assert(0 == memcmp(output, expected, byte_count));
    assert(0xA5 == output[byte_count]);

    xml_file = fopen("utl_xml_document.xml", "wb");
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
    Utl_Xml_Document_t  document;
    Utl_Xml_Node_t      configuration;
    Utl_Xml_Node_t      setting;
    Utl_Xml_Attribute_t namespace_attribute;
    Utl_Xml_Attribute_t schema_attribute;
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

    Utl_Xml_InitializeDocument(&document);
    Utl_Xml_InitializeNode(&configuration, UTL_XML_NODE_ELEMENT, configuration_name, (uint32_t)(sizeof(configuration_name) - 1));
    Utl_Xml_InitializeNode(&setting, UTL_XML_NODE_ELEMENT, setting_name, (uint32_t)(sizeof(setting_name) - 1));
    Utl_Xml_InitializeAttribute(&namespace_attribute, namespace_name, (uint32_t)(sizeof(namespace_name) - 1), namespace_uri, (uint32_t)(sizeof(namespace_uri) - 1));
    Utl_Xml_InitializeAttribute(&schema_attribute, schema_name, (uint32_t)(sizeof(schema_name) - 1), schema_location, (uint32_t)(sizeof(schema_location) - 1));

    Utl_Xml_AppendAttribute(&configuration, &namespace_attribute);
    Utl_Xml_AppendAttribute(&configuration, &schema_attribute);
    Utl_Xml_AppendChild(&configuration, &setting);
    Utl_Xml_AppendNode(&document, &configuration);

    byte_count = Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
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
    Utl_Xml_Document_t  document;
    Utl_Xml_Node_t      nodes[8];
    Utl_Xml_Attribute_t attributes[10];
    byte_t              input[sizeof(text)];
    byte_t              output[sizeof(text)];
    uint64_t            byte_count;

    memcpy(input, text, sizeof(text));
    Utl_Xml_InitializeDocument(&document);
    Utl_Xml_ReadDocument(&document, input, (uint64_t)(sizeof(text) - 1), nodes, (uint32_t)COUNT_OF(nodes), attributes, (uint32_t)COUNT_OF(attributes));

    byte_count = Utl_Xml_WriteDocument(&document, output, (uint64_t)sizeof(output));
    assert((sizeof(text) - 1) == byte_count);
    assert(0 == memcmp(output, text, byte_count));
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
    consumed = Utl_Scan_Int64(minimum, &integer);
    assert((sizeof(minimum) - 1) == consumed);
    assert(INT64_MIN == integer);

    consumed = Utl_Scan_Int64(maximum, &integer);
    assert((sizeof(maximum) - 1) == consumed);
    assert(INT64_MAX == integer);

    consumed = Utl_Scan_Int64(overflow, &integer);
    assert(-1 == consumed);
    assert(INT64_MAX == integer);

    consumed = Utl_Scan_Int64(non_ascii_digit, &integer);
    assert(-1 == consumed);
    assert(INT64_MAX == integer);

    consumed = Utl_Scan_Int64("42tail", &integer);
    assert(2 == consumed);
    assert(42 == integer);

    consumed = Utl_Scan_Int64("+", &integer);
    assert(-1 == consumed);
    assert(42 == integer);

    fraction = 0.0;
    consumed = Utl_Scan_Float(decimal, &fraction);
    assert((sizeof(decimal) - 1) == consumed);
    assert(-0.125 == fraction);

    consumed = Utl_Scan_Float(exponent, &fraction);
    assert(1 == consumed);
    assert(1.0 == fraction);

    consumed = Utl_Scan_Float("2.5tail", &fraction);
    assert(3 == consumed);
    assert(2.5 == fraction);

    long_decimal[0] = '0';
    long_decimal[1] = '.';
    memset(long_decimal + 2, '0', 200);
    long_decimal[202] = '1';
    long_decimal[203] = '\0';
    consumed = Utl_Scan_Float(long_decimal, &fraction);
    assert(203 == consumed);
    assert(0.0 < fraction);

    consumed = Utl_Scan_Float(non_ascii_digit, &fraction);
    assert(-1 == consumed);
    assert(0.0 < fraction);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the XML character, node initialization, writer, reader, and number scanning checks.
 */
int main(void)
{
    TestNumberScanning();
    TestCharacterClasses();
    TestXmlByteSpans();
    TestXmlNodeInitialization();
    TestDeferredNodeContentValidation();
    TestXmlDocumentWrite();
    TestXmlSchemaLocationWrite();
    TestXmlDocumentRoundTrip();
    return 0;
}
