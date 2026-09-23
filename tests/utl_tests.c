#include "utl/utl_defines.h"
#include "utl/utl_xml.h"
#include "utl/utl_xml_node.h"

/* Keep assertions enabled in Release builds. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

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
 *  Checks Name and Nmtoken validation with first-character restrictions, non-ASCII code points,
 *  explicit span lengths and invalid characters.
 */
static void TestNames(void)
{
    static const struct
    {
        uint32_t codePoints[8];
        uint32_t count;
        bool_t   isName;
        bool_t   isNmtoken;
    } cases[] = {
        {{'a'}, 1, 1, 1},
        {{'a', '-', 'b', '.', '0'}, 5, 1, 1},
        {{'_', 'i', 'd'}, 3, 1, 1},
        {{'0'}, 1, 0, 1},
        {{'-', 'a'}, 2, 0, 1},
        {{'.', 'a'}, 2, 0, 1},
        {{0x0300, 'a'}, 2, 0, 1},
        {{'a', 0x0300}, 2, 1, 1},
        {{0x00B7}, 1, 0, 1},
        {{0x4E00, 0x540D}, 2, 1, 1},
        {{0x10000, 0xEFFFF}, 2, 1, 1},
        {{'p', ':', 'n'}, 3, 1, 1},
        {{':', ':'}, 2, 1, 1},      /* Name syntax does not impose namespace rules. */
        {{'x', 'm', 'l'}, 3, 1, 1}, /* Reserved names still match the production. */
        {{'a', ' ', 'b'}, 3, 0, 0},
        {{' ', 'a'}, 2, 0, 0},
        {{'a', '\t'}, 2, 0, 0},
        {{'a', '/'}, 2, 0, 0},
        {{'a', '&'}, 2, 0, 0},
        {{'a', 0x037E}, 2, 0, 0},
        {{'a', 0xF0000}, 2, 0, 0},
        {{0xD800, 0xDC00}, 2, 0, 0}, /* Surrogate values are invalid XML name characters. */
        {{'a', 0x110000}, 2, 0, 0},
        {{'a', UINT32_MAX}, 2, 0, 0},
        {{0}, 1, 0, 0},
        {{'a', 0, 'b'}, 3, 0, 0}, /* Embedded NUL must not truncate validation. */
        {{'a', 0}, 2, 0, 0},
        {{'a', 0}, 1, 1, 1}, /* Only the requested span is checked. */
        {{'a', '/'}, 1, 1, 1},
        {{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'}, 8, 1, 1} /* No terminator. */
    };

    uint32_t test_case_index;

    for (test_case_index = 0; test_case_index < COUNT_OF(cases); ++test_case_index)
    {
        assert(Utl_Xml_IsName(cases[test_case_index].codePoints, cases[test_case_index].count) == cases[test_case_index].isName);
        assert(Utl_Xml_IsNmtoken(cases[test_case_index].codePoints, cases[test_case_index].count) == cases[test_case_index].isNmtoken);
    }
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
    const char_t*       name;

    name = "\xC3\xA9";
    Utl_Xml_Node_Initialize(&root, UTL_XML_NODE_ELEMENT, name, NULL);
    Utl_Xml_Node_Initialize(&text_node, UTL_XML_NODE_TEXT, NULL, "before");
    Utl_Xml_Node_Initialize(&comment, UTL_XML_NODE_COMMENT, NULL, "valid");
    Utl_Xml_Node_Initialize(&instruction, UTL_XML_NODE_PROCESSING_INSTRUCTION, "target", "data");

    assert(UTL_XML_NODE_ELEMENT == root.type);
    assert(name == root.name);
    assert(NULL == root.value);
    assert(NULL == root.firstChild);
    assert(NULL == root.nextSibling);
    assert(NULL == root.firstAttribute);
    assert(UTL_XML_NODE_TEXT == text_node.type);
    assert(UTL_XML_NODE_COMMENT == comment.type);
    assert(UTL_XML_NODE_PROCESSING_INSTRUCTION == instruction.type);
    assert(NULL == instruction.firstChild);

    Utl_Xml_Node_AddAttribute(&root, &attribute, "id", "7");
    Utl_Xml_Node_AddAttribute(&root, &utf8_attribute, "\xC3\xA9" "b", "A\xE2\x82\xAC\xF0\x9F\x98\x80" "Z");
    Utl_Xml_Node_AppendChild(&root, &text_node);
    Utl_Xml_Node_AppendChild(&root, &comment);
    Utl_Xml_Node_AppendChild(&root, &instruction);

    assert(&attribute == root.firstAttribute);
    assert(&utf8_attribute == attribute.next);
    assert(&utf8_attribute == root.lastAttribute);
    assert(2 == root.attributeCount);
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
    const char_t*  name;
    const char_t*  text;
    const char_t*  comment_text;
    const char_t*  target;
    const char_t*  data;

    name         = "bad name";
    text         = "\x01";
    comment_text = "--";
    target       = "xml";
    data         = "?>";

    Utl_Xml_Node_Initialize(&element, UTL_XML_NODE_ELEMENT, name, NULL);
    Utl_Xml_Node_Initialize(&text_node, UTL_XML_NODE_TEXT, NULL, text);
    Utl_Xml_Node_Initialize(&comment, UTL_XML_NODE_COMMENT, NULL, comment_text);
    Utl_Xml_Node_Initialize(&instruction, UTL_XML_NODE_PROCESSING_INSTRUCTION, target, data);

    assert(name == element.name);
    assert(text == text_node.value);
    assert(comment_text == comment.value);
    assert(target == instruction.name);
    assert(data == instruction.value);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the XML character and node initialization checks.
 */
int main(void)
{
    TestCharacterClasses();
    TestNames();
    TestXmlNodeInitialization();
    TestDeferredNodeContentValidation();
    return 0;
}
