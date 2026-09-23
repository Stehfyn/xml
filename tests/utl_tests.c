#include "utl/utl_defines.h"
#include "utl/utl_xml.h"
#include "utl/utl_xml_node.h"

/* Keep assertions enabled in Release builds. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

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
 *  Checks ordered mixed content, reparenting, detachment, and cycle rejection.
 */
static void TestXmlNodeTree(void)
{
    Utl_Xml_Node_t* root;
    Utl_Xml_Node_t* first_text;
    Utl_Xml_Node_t* element;
    Utl_Xml_Node_t* comment;
    Utl_Xml_Node_t* last_text;

    root       = Utl_Xml_Node_CreateElement("root");
    first_text = Utl_Xml_Node_CreateText("before");
    element    = Utl_Xml_Node_CreateElement("child");
    comment    = Utl_Xml_Node_CreateComment("between");
    last_text  = Utl_Xml_Node_CreateText("after");

    assert(NULL != root);
    assert(NULL != first_text);
    assert(NULL != element);
    assert(NULL != comment);
    assert(NULL != last_text);
    assert(TRUE == Utl_Xml_Node_AppendChild(root, first_text));
    assert(TRUE == Utl_Xml_Node_AppendChild(root, element));
    assert(TRUE == Utl_Xml_Node_AppendChild(root, last_text));
    assert(TRUE == Utl_Xml_Node_InsertBefore(root, comment, last_text));
    assert(first_text == root->firstChild);
    assert(element == first_text->nextSibling);
    assert(comment == element->nextSibling);
    assert(last_text == comment->nextSibling);
    assert(last_text == root->lastChild);
    assert(comment == last_text->previousSibling);
    assert(NULL == last_text->nextSibling);
    assert(FALSE == Utl_Xml_Node_AppendChild(element, root));
    assert(FALSE == Utl_Xml_Node_InsertBefore(root, root, element));
    assert(NULL == root->parent);
    assert(root == comment->parent);

    assert(TRUE == Utl_Xml_Node_AppendChild(element, comment));
    assert(comment == element->firstChild);
    assert(last_text == element->nextSibling);
    Utl_Xml_Node_Detach(last_text);
    assert(NULL == last_text->parent);
    assert(element == root->lastChild);

    Utl_Xml_Node_Destroy(last_text);
    Utl_Xml_Node_Destroy(root);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks duplicate names, insertion order, replacement, and removal of element attributes.
 */
static void TestXmlNodeAttributes(void)
{
    Utl_Xml_Node_t*            node;
    const Utl_Xml_Attribute_t* attribute;

    node = Utl_Xml_Node_CreateElement("item");
    assert(NULL != node);
    assert(TRUE == Utl_Xml_Node_AddAttribute(node, "id", "1"));
    assert(TRUE == Utl_Xml_Node_AddAttribute(node, "lang", "en"));
    assert(FALSE == Utl_Xml_Node_AddAttribute(node, "id", "2"));
    assert(0 == strcmp(Utl_Xml_Node_GetAttributeValue(node, "id"), "1"));
    assert(TRUE == Utl_Xml_Node_SetAttribute(node, "id", "2"));
    assert(0 == strcmp(Utl_Xml_Node_GetAttributeValue(node, "id"), "2"));
    attribute = node->firstAttribute;
    assert(0 == strcmp(attribute->name, "id"));
    attribute = attribute->next;
    assert(0 == strcmp(attribute->name, "lang"));
    assert(0 == strcmp(attribute->value, "en"));
    assert(NULL == attribute->next);
    assert(TRUE == Utl_Xml_Node_RemoveAttribute(node, "id"));
    assert(FALSE == Utl_Xml_Node_RemoveAttribute(node, "id"));
    assert(NULL == Utl_Xml_Node_GetAttributeValue(node, "id"));
    Utl_Xml_Node_Destroy(node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks XML character and delimiter validation and non-destructive rejected mutations.
 */
static void TestXmlNodeValidation(void)
{
    Utl_Xml_Node_t* node;

    assert(NULL == Utl_Xml_Node_CreateElement(""));
    assert(NULL == Utl_Xml_Node_CreateElement("0bad"));
    assert(NULL == Utl_Xml_Node_CreateElement("\xC0\xAF"));
    assert(NULL == Utl_Xml_Node_CreateElement("\xED\xA0\x80"));
    assert(NULL == Utl_Xml_Node_CreateText("\x01"));
    assert(NULL == Utl_Xml_Node_CreateComment("bad--comment"));
    assert(NULL == Utl_Xml_Node_CreateComment("bad-"));
    assert(NULL == Utl_Xml_Node_CreateProcessingInstruction("XmL", "data"));
    assert(NULL == Utl_Xml_Node_CreateProcessingInstruction("target", "bad?>data"));

    node = Utl_Xml_Node_CreateElement("\xC3\xA9");
    assert(NULL != node);
    assert(FALSE == Utl_Xml_Node_SetName(node, "0bad"));
    assert(0 == strcmp(node->name, "\xC3\xA9"));
    assert(FALSE == Utl_Xml_Node_AddAttribute(node, "0bad", "value"));
    assert(FALSE == Utl_Xml_Node_AddAttribute(node, "ok", "\x01"));
    Utl_Xml_Node_Destroy(node);

    node = Utl_Xml_Node_CreateComment("valid");
    assert(NULL != node);
    assert(FALSE == Utl_Xml_Node_SetValue(node, "bad--value"));
    assert(0 == strcmp(node->value, "valid"));
    Utl_Xml_Node_Destroy(node);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the XML character-classification and name-validation checks.
 */
int main(void)
{
    TestCharacterClasses();
    TestNames();
    TestXmlNodeTree();
    TestXmlNodeAttributes();
    TestXmlNodeValidation();
    return 0;
}
