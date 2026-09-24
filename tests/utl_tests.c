#include "utl/utl_defines.h"
#include "utl/utl_xml.h"

/* Keep assertions enabled in Release builds. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
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
 *  Checks Nmtoken validation with non-ASCII code points, explicit span lengths, and invalid characters.
 */
static void TestNmtokens(void)
{
    static const struct
    {
        uint32_t codePoints[8];
        uint32_t count;
        bool_t   isNmtoken;
    } cases[] = {
        {{'a'}, 1, 1},
        {{'a', '-', 'b', '.', '0'}, 5, 1},
        {{'_', 'i', 'd'}, 3, 1},
        {{'0'}, 1, 1},
        {{'-', 'a'}, 2, 1},
        {{'.', 'a'}, 2, 1},
        {{0x0300, 'a'}, 2, 1},
        {{'a', 0x0300}, 2, 1},
        {{0x00B7}, 1, 1},
        {{0x4E00, 0x540D}, 2, 1},
        {{0x10000, 0xEFFFF}, 2, 1},
        {{'p', ':', 'n'}, 3, 1},
        {{':', ':'}, 2, 1},      /* Name syntax does not impose namespace rules. */
        {{'x', 'm', 'l'}, 3, 1}, /* Reserved names still match the production. */
        {{'a', ' ', 'b'}, 3, 0},
        {{' ', 'a'}, 2, 0},
        {{'a', '\t'}, 2, 0},
        {{'a', '/'}, 2, 0},
        {{'a', '&'}, 2, 0},
        {{'a', 0x037E}, 2, 0},
        {{'a', 0xF0000}, 2, 0},
        {{0xD800, 0xDC00}, 2, 0}, /* Surrogate values are invalid XML name characters. */
        {{'a', 0x110000}, 2, 0},
        {{'a', UINT32_MAX}, 2, 0},
        {{0}, 1, 0},
        {{'a', 0, 'b'}, 3, 0}, /* Embedded NUL must not truncate validation. */
        {{'a', 0}, 2, 0},
        {{'a', 0}, 1, 1}, /* Only the requested span is checked. */
        {{'a', '/'}, 1, 1},
        {{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'}, 8, 1} /* No terminator. */
    };

    uint32_t test_case_index;

    for (test_case_index = 0; test_case_index < COUNT_OF(cases); ++test_case_index)
    {
        assert(Utl_Xml_IsNmtoken(cases[test_case_index].codePoints, cases[test_case_index].count) == cases[test_case_index].isNmtoken);
    }
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks counted UTF-8 strings, names, and markup content.
 */
static void TestXmlByteSpans(void)
{
    static const byte_t empty[] = "";
    static const byte_t valid_text[] = "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    static const byte_t invalid_utf8[] = "\xC3";
    static const byte_t invalid_xml_char[] = "\x01";
    static const byte_t valid_name[] = "\xC3\xA9" "b";
    static const byte_t invalid_name[] = "1a";
    static const byte_t reserved_target[] = "xml";
    static const byte_t mixed_case_target[] = "XmL";
    static const byte_t valid_target[] = "xml-stylesheet";
    static const byte_t valid_comment[] = "a-b";
    static const byte_t double_hyphen[] = "a--b";
    static const byte_t trailing_hyphen[] = "a-";
    static const byte_t valid_pi_data[] = "a?b>";
    static const byte_t pi_close_delimiter[] = "a?>b";
    static const byte_t embedded_zero[] = {'a', 0, 'b'};
    static const byte_t unterminated[] = {'a', 'b'};

    assert(TRUE == Utl_Xml_IsString(empty, 0));
    assert(TRUE == Utl_Xml_IsString(valid_text, (uint32_t)(sizeof(valid_text) - 1)));
    assert(TRUE == Utl_Xml_IsString(unterminated, (uint32_t)sizeof(unterminated)));
    assert(FALSE == Utl_Xml_IsString(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsString(invalid_xml_char, (uint32_t)(sizeof(invalid_xml_char) - 1)));
    assert(FALSE == Utl_Xml_IsString(embedded_zero, (uint32_t)sizeof(embedded_zero)));
    assert(FALSE == Utl_Xml_IsString(valid_text, (uint32_t)sizeof(valid_text)));

    assert(TRUE == Utl_Xml_IsName(valid_name, (uint32_t)(sizeof(valid_name) - 1)));
    assert(TRUE == Utl_Xml_IsName(unterminated, (uint32_t)sizeof(unterminated)));
    assert(FALSE == Utl_Xml_IsName(invalid_name, (uint32_t)(sizeof(invalid_name) - 1)));
    assert(FALSE == Utl_Xml_IsName(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsName(embedded_zero, (uint32_t)sizeof(embedded_zero)));
    assert(FALSE == Utl_Xml_IsName(valid_name, (uint32_t)sizeof(valid_name)));

    assert(TRUE == Utl_Xml_IsPITarget(valid_name, (uint32_t)(sizeof(valid_name) - 1)));
    assert(TRUE == Utl_Xml_IsPITarget(valid_target, (uint32_t)(sizeof(valid_target) - 1)));
    assert(FALSE == Utl_Xml_IsPITarget(reserved_target, (uint32_t)(sizeof(reserved_target) - 1)));
    assert(FALSE == Utl_Xml_IsPITarget(mixed_case_target, (uint32_t)(sizeof(mixed_case_target) - 1)));
    assert(FALSE == Utl_Xml_IsPITarget(invalid_name, (uint32_t)(sizeof(invalid_name) - 1)));
    assert(FALSE == Utl_Xml_IsPITarget(embedded_zero, (uint32_t)sizeof(embedded_zero)));

    assert(TRUE == Utl_Xml_IsComment(empty, 0));
    assert(TRUE == Utl_Xml_IsComment(valid_comment, (uint32_t)(sizeof(valid_comment) - 1)));
    assert(FALSE == Utl_Xml_IsComment(double_hyphen, (uint32_t)(sizeof(double_hyphen) - 1)));
    assert(FALSE == Utl_Xml_IsComment(trailing_hyphen, (uint32_t)(sizeof(trailing_hyphen) - 1)));
    assert(FALSE == Utl_Xml_IsComment(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsComment(embedded_zero, (uint32_t)sizeof(embedded_zero)));

    assert(TRUE == Utl_Xml_IsPI(empty, 0));
    assert(TRUE == Utl_Xml_IsPI(valid_pi_data, (uint32_t)(sizeof(valid_pi_data) - 1)));
    assert(FALSE == Utl_Xml_IsPI(pi_close_delimiter, (uint32_t)(sizeof(pi_close_delimiter) - 1)));
    assert(FALSE == Utl_Xml_IsPI(invalid_utf8, (uint32_t)(sizeof(invalid_utf8) - 1)));
    assert(FALSE == Utl_Xml_IsPI(embedded_zero, (uint32_t)sizeof(embedded_zero)));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks caller-owned node initialization and forward links used during serialization.
 */
static void TestXmlNodeInitialization(void)
{
    Utl_Xml_Node_t      root;
    Utl_Xml_Node_t      text_node;
    Utl_Xml_Node_t      comment;
    Utl_Xml_Node_t      instruction;
    Utl_Xml_Attribute_t attribute;
    Utl_Xml_Attribute_t utf8_attribute;
    Utl_Xml_Attribute_t longer_attribute;
    const byte_t        name[] = "\xC3\xA9";
    const byte_t        text_value[] = "before";
    const byte_t        comment_value[] = "valid";
    const byte_t        target[] = "target";
    const byte_t        data[] = "data";
    const byte_t        attribute_name[] = "id";
    const byte_t        attribute_value[] = "7";
    const byte_t        utf8_name[] = "\xC3\xA9";
    const byte_t        utf8_value[] = "A\xE2\x82\xAC\xF0\x9F\x98\x80" "Z";
    const byte_t        longer_name[] = "id2";

    Utl_Xml_InitializeNode(&root, UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1), NULL, 0);
    Utl_Xml_InitializeNode(&text_node, UTL_XML_NODE_TEXT, NULL, 0, text_value, (uint32_t)(sizeof(text_value) - 1));
    Utl_Xml_InitializeNode(&comment, UTL_XML_NODE_COMMENT, NULL, 0, comment_value, (uint32_t)(sizeof(comment_value) - 1));
    Utl_Xml_InitializeNode(&instruction, UTL_XML_NODE_PROCESSING_INSTRUCTION, target, (uint32_t)(sizeof(target) - 1), data, (uint32_t)(sizeof(data) - 1));

    assert(UTL_XML_NODE_ELEMENT == root.type);
    assert(name == root.name);
    assert((sizeof(name) - 1) == root.nameByteCount);
    assert(NULL == root.value);
    assert(0 == root.valueByteCount);
    assert(NULL == root.firstChild);
    assert(NULL == root.nextSibling);
    assert(NULL == root.firstAttribute);
    assert(UTL_XML_NODE_TEXT == text_node.type);
    assert((sizeof(text_value) - 1) == text_node.valueByteCount);
    assert(UTL_XML_NODE_COMMENT == comment.type);
    assert(UTL_XML_NODE_PROCESSING_INSTRUCTION == instruction.type);
    assert((sizeof(target) - 1) == instruction.nameByteCount);
    assert((sizeof(data) - 1) == instruction.valueByteCount);
    assert(NULL == instruction.firstChild);

    Utl_Xml_AddAttribute(&root, &attribute, attribute_name, (uint32_t)(sizeof(attribute_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    Utl_Xml_AddAttribute(&root, &utf8_attribute, utf8_name, (uint32_t)(sizeof(utf8_name) - 1), utf8_value, (uint32_t)(sizeof(utf8_value) - 1));
    Utl_Xml_AddAttribute(&root, &longer_attribute, longer_name, (uint32_t)(sizeof(longer_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    Utl_Xml_AppendChild(&root, &text_node);
    Utl_Xml_AppendChild(&root, &comment);
    Utl_Xml_AppendChild(&root, &instruction);

    assert(&attribute == root.firstAttribute);
    assert(&utf8_attribute == attribute.next);
    assert(&longer_attribute == utf8_attribute.next);
    assert(&longer_attribute == root.lastAttribute);
    assert(3 == root.attributeCount);
    assert((sizeof(attribute_name) - 1) == attribute.nameByteCount);
    assert((sizeof(attribute_value) - 1) == attribute.valueByteCount);
    assert((sizeof(utf8_name) - 1) == utf8_attribute.nameByteCount);
    assert((sizeof(utf8_value) - 1) == utf8_attribute.valueByteCount);
    assert(&text_node == root.firstChild);
    assert(&comment == text_node.nextSibling);
    assert(&instruction == comment.nextSibling);
    assert(&instruction == root.lastChild);
    assert(&root == comment.parent);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Node initializers store content that the writer will validate later.
 */
static void TestDeferredNodeContentValidation(void)
{
    Utl_Xml_Node_t element;
    Utl_Xml_Node_t text_node;
    Utl_Xml_Node_t comment;
    Utl_Xml_Node_t instruction;
    const byte_t   name[] = "bad name";
    const byte_t   text[] = "\x01";
    const byte_t   comment_text[] = "--";
    const byte_t   target[] = "xml";
    const byte_t   data[] = "?>";

    Utl_Xml_InitializeNode(&element, UTL_XML_NODE_ELEMENT, name, (uint32_t)(sizeof(name) - 1), NULL, 0);
    Utl_Xml_InitializeNode(&text_node, UTL_XML_NODE_TEXT, NULL, 0, text, (uint32_t)(sizeof(text) - 1));
    Utl_Xml_InitializeNode(&comment, UTL_XML_NODE_COMMENT, NULL, 0, comment_text, (uint32_t)(sizeof(comment_text) - 1));
    Utl_Xml_InitializeNode(&instruction, UTL_XML_NODE_PROCESSING_INSTRUCTION, target, (uint32_t)(sizeof(target) - 1), data, (uint32_t)(sizeof(data) - 1));

    assert(name == element.name);
    assert((sizeof(name) - 1) == element.nameByteCount);
    assert(text == text_node.value);
    assert((sizeof(text) - 1) == text_node.valueByteCount);
    assert(comment_text == comment.value);
    assert(target == instruction.name);
    assert(data == instruction.value);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds a document in source order, checks its serialized bytes, and writes an XML artifact.
 */
static void TestXmlDocumentWrite(void)
{
    Utl_Xml_Document_t document;
    Utl_Xml_Node_t     declaration_node;
    Utl_Xml_Node_t     prolog_comment;
    Utl_Xml_Node_t     doctype_node;
    Utl_Xml_Node_t     prolog_instruction;
    Utl_Xml_Node_t     root;
    Utl_Xml_Node_t     text_node;
    Utl_Xml_Node_t     child;
    Utl_Xml_Node_t     grandchild;
    Utl_Xml_Node_t     tail_node;
    Utl_Xml_Node_t     inner_comment;
    Utl_Xml_Node_t     epilog_comment;
    Utl_Xml_Attribute_t attribute;
    const byte_t       before[] = "before";
    const byte_t       target[] = "target";
    const byte_t       data[] = "data";
    const byte_t       root_name[] = "root";
    const byte_t       child_name[] = "child";
    const byte_t       grandchild_name[] = "grandchild";
    const byte_t       attribute_name[] = "a";
    const byte_t       attribute_value[] = "A&\"<\t\n\r";
    const byte_t       text_value[] = "<&>\r";
    const byte_t       tail[] = "tail";
    const byte_t       inside[] = "inside";
    const byte_t       after[] = "after";
    const byte_t       expected[] =
        "<?xml version=\"1.0\"?><!--before--><!DOCTYPE root><?target data?>"
        "<root a=\"A&amp;&quot;&lt;&#x9;&#xA;&#xD;\">&lt;&amp;&gt;&#xD;"
        "<child><grandchild/></child>tail<!--inside--></root><!--after-->";
    byte_t             output[256];
    FILE*              xml_file;
    uint64_t           byte_count;
    uint64_t           bytes_written;
    int                close_result;

    Utl_Xml_InitializeDocument(&document);
    assert(NULL == document.firstNode);
    assert(NULL == document.lastNode);

    Utl_Xml_InitializeNode(&declaration_node, UTL_XML_NODE_XML_DECLARATION, NULL, 0, NULL, 0);
    Utl_Xml_InitializeNode(&prolog_comment, UTL_XML_NODE_COMMENT, NULL, 0, before, (uint32_t)(sizeof(before) - 1));
    Utl_Xml_InitializeNode(&doctype_node, UTL_XML_NODE_DOCTYPE, root_name, (uint32_t)(sizeof(root_name) - 1), NULL, 0);
    Utl_Xml_InitializeNode(&prolog_instruction, UTL_XML_NODE_PROCESSING_INSTRUCTION, target, (uint32_t)(sizeof(target) - 1), data, (uint32_t)(sizeof(data) - 1));
    Utl_Xml_InitializeNode(&root, UTL_XML_NODE_ELEMENT, root_name, (uint32_t)(sizeof(root_name) - 1), NULL, 0);
    Utl_Xml_InitializeNode(&text_node, UTL_XML_NODE_TEXT, NULL, 0, text_value, (uint32_t)(sizeof(text_value) - 1));
    Utl_Xml_InitializeNode(&child, UTL_XML_NODE_ELEMENT, child_name, (uint32_t)(sizeof(child_name) - 1), NULL, 0);
    Utl_Xml_InitializeNode(&grandchild, UTL_XML_NODE_ELEMENT, grandchild_name, (uint32_t)(sizeof(grandchild_name) - 1), NULL, 0);
    Utl_Xml_InitializeNode(&tail_node, UTL_XML_NODE_TEXT, NULL, 0, tail, (uint32_t)(sizeof(tail) - 1));
    Utl_Xml_InitializeNode(&inner_comment, UTL_XML_NODE_COMMENT, NULL, 0, inside, (uint32_t)(sizeof(inside) - 1));
    Utl_Xml_InitializeNode(&epilog_comment, UTL_XML_NODE_COMMENT, NULL, 0, after, (uint32_t)(sizeof(after) - 1));

    Utl_Xml_AddAttribute(&root, &attribute, attribute_name, (uint32_t)(sizeof(attribute_name) - 1), attribute_value, (uint32_t)(sizeof(attribute_value) - 1));
    Utl_Xml_AppendChild(&root, &text_node);
    Utl_Xml_AppendChild(&root, &child);
    Utl_Xml_AppendChild(&child, &grandchild);
    Utl_Xml_AppendChild(&root, &tail_node);
    Utl_Xml_AppendChild(&root, &inner_comment);

    Utl_Xml_AppendNode(&document, &declaration_node);
    Utl_Xml_AppendNode(&document, &prolog_comment);
    Utl_Xml_AppendNode(&document, &doctype_node);
    Utl_Xml_AppendNode(&document, &prolog_instruction);
    Utl_Xml_AppendNode(&document, &root);
    Utl_Xml_AppendNode(&document, &epilog_comment);

    assert(&declaration_node == document.firstNode);
    assert(&prolog_comment == declaration_node.nextSibling);
    assert(&doctype_node == prolog_comment.nextSibling);
    assert(&prolog_instruction == doctype_node.nextSibling);
    assert(&root == prolog_instruction.nextSibling);
    assert(&epilog_comment == root.nextSibling);
    assert(NULL == epilog_comment.nextSibling);
    assert(&epilog_comment == document.lastNode);

    memset(output, 0xA5, sizeof(output));
    byte_count = Utl_Xml_WriteDocument(&document, output, (uint32_t)sizeof(output));
    assert((sizeof(expected) - 1) == byte_count);
    assert(0 == memcmp(output, expected, byte_count));
    assert(0xA5 == output[byte_count]);

    xml_file = fopen("utl_xml_document.xml", "wb");
    assert(NULL != xml_file);
    bytes_written = fwrite(output, 1, (uint32_t)byte_count, xml_file);
    assert(byte_count == bytes_written);
    close_result = fclose(xml_file);
    assert(0 == close_result);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the XML character and node initialization checks.
 */
int main(void)
{
    TestCharacterClasses();
    TestNmtokens();
    TestXmlByteSpans();
    TestXmlNodeInitialization();
    TestDeferredNodeContentValidation();
    TestXmlDocumentWrite();
    return 0;
}
