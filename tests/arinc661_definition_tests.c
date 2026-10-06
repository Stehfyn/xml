#include "arinc661/arinc661_definition.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <string.h>

static uint64_t c_workspace[ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)];

/*************************************************************************************************/
/**
 * \brief Checks in-memory Definition File conversion and nested widget ownership.
 */
static void TestDefinition(void)
{
    byte_t input[] = "<?xml version=\"1.0\"?><a661_df name=\"example\" library_version=\"0\" supp_version=\"6\"><model><prop name=\"ApplicationId\" value=\"1\"/></model><a661_layer name=\"Primary\"><model><prop name=\"LayerId\" value=\"1\"/></model><a661_widget name=\"Panel\" type=\"A661_PANEL\"><model><prop name=\"WidgetIdent\" value=\"1\"/></model><a661_widget name=\"Label\" type=\"A661_LABEL\"><model><prop name=\"WidgetIdent\" value=\"2\"/></model></a661_widget></a661_widget></a661_layer></a661_df>";
    byte_t output[2048];
    Arinc661_Layer_t layers[2];
    Arinc661_Widget_t widgets[3];
    Arinc661_Property_t properties[8];
    Arinc661_Definition_t definition;
    uint64_t output_byte_count;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.layers = layers;
    definition.widgets = widgets;
    definition.properties = properties;
    definition.layerCapacity = 2;
    definition.widgetCapacity = 3;
    definition.propertyCapacity = 8;
    is_valid = Arinc661_ReadDefinition(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    assert(4 == definition.propertyCount);
    assert(UINT32_MAX == widgets[0].parentWidgetIndex);
    assert(0 == widgets[1].parentWidgetIndex);

    output_byte_count = Arinc661_WriteDefinition(&definition, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    assert(0 == memcmp(output, "<a661_df ", sizeof("<a661_df ") - 1));
    is_valid = Arinc661_ReadDefinition(&definition, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    assert(4 == definition.propertyCount);
}

/*************************************************************************************************/
/**
 * \brief Checks Definition File writing and reading without an XML declaration.
 */
static void TestDefinitionWithoutDeclaration(void)
{
    Arinc661_Definition_t definition;
    byte_t output[1024];
    uint64_t byte_count;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.supplementVersion = 6;
    byte_count = Arinc661_WriteDefinition(&definition, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    assert(0 == memcmp(output, "<a661_df ", sizeof("<a661_df ") - 1));
    is_valid = Arinc661_ReadDefinition(&definition, output, byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(6 == definition.supplementVersion);
}

/*************************************************************************************************/
/**
 * \brief Checks binary Definition File bytes against ARINC 661 Part 1-7, Appendix C-11, and the round trip.
 */
static void TestDefinitionBinary(void)
{
    byte_t input[] = "<a661_df name=\"example\" library_version=\"1\" supp_version=\"4\"><model><prop name=\"ApplicationId\" value=\"26504\"/></model><a661_layer name=\"Primary\"><model><prop name=\"LayerId\" value=\"1\"/><prop name=\"ContextNumber\" value=\"0\"/><prop name=\"Width\" value=\"10000\"/></model><a661_widget name=\"Panel\" type=\"A661_PANEL\"><model><prop name=\"WidgetIdent\" value=\"4641\"/><prop name=\"Enable\" value=\"A661_TRUE\"/><prop name=\"Visible\" value=\"A661_TRUE\"/><prop name=\"PosX\" value=\"0\"/><prop name=\"PosY\" value=\"0\"/><prop name=\"SizeX\" value=\"10000\"/><prop name=\"SizeY\" value=\"10000\"/><prop name=\"StyleSet\" value=\"0\"/></model><a661_widget name=\"Label\" type=\"A661_LABEL\"><model><prop name=\"WidgetIdent\" value=\"2\"/><prop name=\"Visible\" value=\"A661_TRUE\"/><prop name=\"PosX\" value=\"-1\"/><prop name=\"RotationAngle\" value=\"90.0\"/><prop name=\"Alignment\" value=\"A661_CENTER\"/><prop name=\"LabelString\" value=\"YOU\"/></model></a661_widget></a661_widget></a661_layer></a661_df>";
    /* Header, layer block, and panel command are the bytes of Appendix C-11 */
    static const byte_t expected[] =
    {
        0xA6, 0x61, 0x01, 0x04, 0x67, 0x88, 0x00, 0x00,
        0xA0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58,
        0xCA, 0x01, 0x00, 0x20, 0xA1, 0xF0, 0x12, 0x21, 0x00, 0x00, 0x01, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x10, 0x00, 0x00, 0x27, 0x10,
        0x00, 0x00, 0x00, 0x00,
        0xCA, 0x01, 0x00, 0x2C, 0xA1, 0x60, 0x00, 0x02, 0x12, 0x21, 0x00, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15,
        'Y', 'O', 'U', 0x00,
        0xC0, 0x00, 0x00, 0x00,
        0xE0, 0x00, 0x00, 0x00
    };
    byte_t output[256];
    Arinc661_Layer_t layers[1];
    Arinc661_Widget_t widgets[2];
    Arinc661_Property_t properties[32];
    Arinc661_Definition_t definition;
    uint64_t byte_count;
    uint32_t index;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.layers = layers;
    definition.widgets = widgets;
    definition.properties = properties;
    definition.layerCapacity = 1;
    definition.widgetCapacity = 2;
    definition.propertyCapacity = 32;
    is_valid = Arinc661_ReadDefinition(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);

    byte_count = Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));
    /* An output that does not fit is rejected */
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(expected) - 1));

    is_valid = Arinc661_ReadDefinitionBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    assert(1 == definition.libraryVersion);
    assert(4 == definition.supplementVersion);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    /* ApplicationId, LayerId, ContextNumber, and WidgetIdent plus eight panel and fourteen label parameters */
    assert(27 == definition.propertyCount);
    assert(0 == strcmp("26504", properties[0].value));
    assert(0 == strcmp("A661_PANEL", widgets[0].type));
    assert(UINT32_MAX == widgets[0].parentWidgetIndex);
    assert(0 == strcmp("A661_LABEL", widgets[1].type));
    assert(0 == widgets[1].parentWidgetIndex);
    assert(0 == strcmp("WidgetIdent", properties[3].name));
    assert(0 == strcmp("4641", properties[3].value));
    assert(0 == strcmp("PosX", properties[15].name));
    assert(0 == strcmp("-1", properties[15].value));
    assert(0 == strcmp("RotationAngle", properties[19].name));
    assert(0 == strcmp("90", properties[19].value));
    assert(0 == strcmp("LabelString", properties[26].name));
    assert(0 == strcmp("YOU", properties[26].value));

    /* Values read from binary write the same bytes again */
    byte_count = Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));

    /* Every truncation is rejected without reading past the input */
    for (index = 0; index < sizeof(expected); ++index)
    {
        memcpy(output, expected, index);
        assert(FALSE == Arinc661_ReadDefinitionBinary(&definition, output, index));
        assert(0 == definition.layerCount);
    }

    /* A symbolic value without an ARINC 661 constant, an unknown property, and an unknown widget type are rejected */
    is_valid = Arinc661_ReadDefinitionBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    strcpy(properties[10].value, "red");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output)));
    strcpy(properties[10].value, "0");
    strcpy(properties[10].name, "Unknown");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output)));
    strcpy(properties[10].name, "StyleSet");
    strcpy(widgets[1].type, "A661_SELECTOR");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output)));
}

/*************************************************************************************************/
/**
 * \brief Checks that the binary Definition File of ARINC 661 Part 1-7, Appendix C-11, reads and writes back unchanged.
 */
static void TestDefinitionBinaryLabelComplex(void)
{
    /* Appendix C-11, with the label's ParentIdent set to the panel's WidgetIdent; the appendix names a parent that is not in the file */
    static const byte_t expected[] =
    {
        0xA6, 0x61, 0x01, 0x04, 0x67, 0x88, 0x00, 0x00,
        0xA0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C,
        0xCA, 0x01, 0x00, 0x20, 0xA1, 0xF0, 0x12, 0x21, 0x00, 0x00, 0x01, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x10, 0x00, 0x00, 0x27, 0x10,
        0x00, 0x00, 0x00, 0x00,
        0xCA, 0x01, 0x00, 0x50, 0xA1, 0x70, 0x00, 0x02, 0x12, 0x21, 0x00, 0x01,
        0x00, 0x00, 0x03, 0x71, 0x00, 0x00, 0x1E, 0x58, 0x00, 0x00, 0x0E, 0xD7, 0x00, 0x00, 0x03, 0x2F,
        0x00, 0x00, 0x00, 0x50, 0x15, 0x00, 0x00, 0x00,
        0x1B, 0x42, 0x00, 0x1B, 0x41, 0x00, 0x1B, 0x43, 0x00, 0x00, 0x00, 0x00,
        'Y', 'O', 'U', ' ', 0x1B, 0x41, 0x01, 0x1B, 0x4A, 0x1B, 0x48, 'A', 'R', 'E', 0x1B, 0x4B,
        0x1B, 0x49, ' ', 'A', ' ', 0x1B, 0x42, 0x02, 'B', 'A', 'N', 'A', 'N', 'A', 0x00, 0x00,
        0xC0, 0x00, 0x00, 0x00,
        0xE0, 0x00, 0x00, 0x00
    };
    byte_t output[256];
    Arinc661_Layer_t layers[1];
    Arinc661_Widget_t widgets[2];
    Arinc661_Property_t properties[32];
    Arinc661_Definition_t definition;
    uint64_t byte_count;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.layers = layers;
    definition.widgets = widgets;
    definition.properties = properties;
    definition.layerCapacity = 1;
    definition.widgetCapacity = 2;
    definition.propertyCapacity = 32;
    is_valid = Arinc661_ReadDefinitionBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    assert(2 == definition.widgetCount);
    assert(24 == definition.propertyCount);
    assert(0 == strcmp("A661_LABEL_COMPLEX", widgets[1].type));
    assert(0 == widgets[1].parentWidgetIndex);
    assert(0 == strcmp("MaxStringLength", properties[20].name));
    assert(0 == strcmp("80", properties[20].value));
    assert(0 == strcmp("DefaultStyleText", properties[22].name));
    assert(0 == strcmp("1B42001B41001B4300000000", properties[22].value));
    assert(0 == strcmp("LabelString", properties[23].name));
    assert(30 == strlen(properties[23].value));

    byte_count = Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));

    /* Bytes need two hexadecimal digits each */
    strcpy(properties[22].value, "1B42001B41001B43000000");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output)));
    strcpy(properties[22].value, "1B42001B41001B430000000G");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output)));
}

/*************************************************************************************************/
/**
 * \brief Checks array parameters, whose elements are properties sharing the array's name.
 */
static void TestDefinitionBinaryArrays(void)
{
    byte_t input[] = "<a661_df name=\"example\" library_version=\"0\" supp_version=\"7\"><model/><a661_layer name=\"Primary\"><model><prop name=\"LayerId\" value=\"1\"/></model>"
                     "<a661_widget name=\"Format\" type=\"A661_BUFFER_FORMAT\"><model><prop name=\"WidgetIdent\" value=\"4\"/><prop name=\"NumberOfFields\" value=\"2\"/><prop name=\"BufferStructure\" value=\"2\"/><prop name=\"BufferStructure\" value=\"A661_ENABLE\"/><prop name=\"BufferStructure\" value=\"3\"/><prop name=\"BufferStructure\" value=\"200\"/></model></a661_widget>"
                     "<a661_widget name=\"Tree\" type=\"A661_TREE\"><model><prop name=\"WidgetIdent\" value=\"5\"/><prop name=\"NumberOfEntries\" value=\"3\"/>"
                     "<prop name=\"NodeStyleArray\" value=\"1\"/><prop name=\"NodeStyleArray\" value=\"2\"/><prop name=\"NodeStyleArray\" value=\"3\"/>"
                     "<prop name=\"ParentArray\" value=\"0\"/><prop name=\"ParentArray\" value=\"1\"/><prop name=\"ParentArray\" value=\"1\"/>"
                     "<prop name=\"EnableArray\" value=\"A661_TRUE\"/><prop name=\"EnableArray\" value=\"A661_FALSE\"/><prop name=\"EnableArray\" value=\"A661_TRUE\"/>"
                     "<prop name=\"OpenArray\" value=\"1\"/><prop name=\"OpenArray\" value=\"0\"/><prop name=\"OpenArray\" value=\"0\"/>"
                     "<prop name=\"LabelStringArray\" value=\"A\"/><prop name=\"LabelStringArray\" value=\"BC\"/><prop name=\"LabelStringArray\" value=\"\"/></model></a661_widget></a661_layer></a661_df>";
    /* Table 3.3.4-2: NumberOfFields, then WidgetIdent and ParameterIdent pairs; A661_ENABLE is 0xB180 in Table 4.6-8 */
    static const byte_t expected_format[] =
    {
        0xCA, 0x01, 0x00, 0x14, 0xA0, 0x40, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02,
        0x00, 0x02, 0xB1, 0x80, 0x00, 0x03, 0x00, 0xC8
    };
    /* Table 3.10.3-2: the four arrays, alignment (none needed here), three strings, and command padding */
    static const byte_t expected_arrays[] =
    {
        0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
        0x01, 0x00, 0x01, 0x01, 0x00, 0x00,
        'A', 0x00, 'B', 'C', 0x00, 0x00, 0x00, 0x00
    };
    byte_t output[256];
    byte_t copy[256];
    Arinc661_Layer_t layers[1];
    Arinc661_Widget_t widgets[2];
    Arinc661_Property_t properties[64];
    Arinc661_Definition_t definition;
    uint64_t byte_count;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.layers = layers;
    definition.widgets = widgets;
    definition.properties = properties;
    definition.layerCapacity = 1;
    definition.widgetCapacity = 2;
    definition.propertyCapacity = 64;
    is_valid = Arinc661_ReadDefinition(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);

    /* Header, layer header, 20-byte format command, 72-byte tree command, layer end, and footer */
    byte_count = Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output));
    assert(116 == byte_count);
    assert(0 == memcmp(&output[16], expected_format, sizeof(expected_format)));
    assert((0x00 == output[38]) && (0x48 == output[39]));
    assert(0 == memcmp(&output[82], expected_arrays, sizeof(expected_arrays)));

    is_valid = Arinc661_ReadDefinitionBinary(&definition, output, byte_count);
    assert(is_valid);
    assert(2 == definition.widgetCount);
    /* Three header properties, six for the format, and seventeen single plus fifteen array values for the tree */
    assert(41 == definition.propertyCount);
    assert(0 == strcmp("NumberOfFields", properties[4].name));
    assert(0 == strcmp("BufferStructure", properties[6].name));
    assert(0 == strcmp("45440", properties[6].value));
    assert(0 == strcmp("LabelStringArray", properties[39].name));
    assert(0 == strcmp("BC", properties[39].value));
    assert(0 == strcmp("LabelStringArray", properties[40].name));
    assert(0 == strcmp("", properties[40].value));

    byte_count = Arinc661_WriteDefinitionBinary(&definition, copy, sizeof(copy));
    assert(116 == byte_count);
    assert(0 == memcmp(copy, output, 116));

    /* The count parameter must match the number of array properties */
    strcpy(properties[4].value, "3");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, copy, sizeof(copy)));
    strcpy(properties[4].value, "1");
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, copy, sizeof(copy)));

    /* A count that promises more elements than the command holds is rejected */
    output[27] = 0x09;
    assert(FALSE == Arinc661_ReadDefinitionBinary(&definition, output, 116));
}

/*************************************************************************************************/
/**
 * \brief Checks that XML arrayprop entry forms flatten to repeated properties and write back as entries.
 */
static void TestDefinitionArrayProperties(void)
{
    byte_t input[] = "<a661_df name=\"example\" library_version=\"0\" supp_version=\"7\"><model/><a661_layer name=\"Primary\"><model/>"
                     "<a661_widget name=\"Line\" type=\"A661_GP_POLYLINE\"><model><prop name=\"WidgetIdent\" value=\"7\"/><prop name=\"NumberOfVertices\" value=\"2\"/>"
                     "<arrayprop name=\"Vertices\"><xyentry x=\"-500\" y=\"-500\"/><xyentry x=\"500\" y=\"500\"/></arrayprop></model></a661_widget>"
                     "<a661_widget name=\"Format\" type=\"A661_BUFFER_FORMAT\"><model><prop name=\"WidgetIdent\" value=\"4\"/><prop name=\"NumberOfFields\" value=\"1\"/>"
                     "<arrayprop name=\"BufferStructure\"><structentry><field name=\"WidgetIdent\" value=\"2\"/><field name=\"ParameterIdent\" value=\"A661_ENABLE\"/></structentry></arrayprop>"
                     "<arrayprop name=\"Single\"><entry value=\"1\"/></arrayprop></model></a661_widget></a661_layer></a661_df>";
    byte_t output[2048];
    Arinc661_Layer_t layers[1];
    Arinc661_Widget_t widgets[2];
    Arinc661_Property_t properties[16];
    Arinc661_Definition_t definition;
    uint64_t byte_count;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.layers = layers;
    definition.widgets = widgets;
    definition.properties = properties;
    definition.layerCapacity = 1;
    definition.widgetCapacity = 2;
    definition.propertyCapacity = 16;
    is_valid = Arinc661_ReadDefinition(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(11 == definition.propertyCount);
    assert(0 == strcmp("Vertices", properties[2].name));
    assert(0 == strcmp("-500", properties[2].value));
    assert(0 == strcmp("Vertices", properties[5].name));
    assert(0 == strcmp("500", properties[5].value));
    assert(0 == strcmp("BufferStructure", properties[8].name));
    assert(0 == strcmp("2", properties[8].value));
    assert(0 == strcmp("BufferStructure", properties[9].name));
    assert(0 == strcmp("A661_ENABLE", properties[9].value));
    assert(0 == strcmp("Single", properties[10].name));

    /* Repeated names are written as one arrayprop of entries; a single value stays a prop */
    byte_count = Arinc661_WriteDefinition(&definition, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    output[byte_count] = '\0';
    assert(NULL != strstr((const char_t*)output, "<arrayprop name=\"Vertices\">"));
    assert(NULL != strstr((const char_t*)output, "<entry value=\"-500\"/>"));
    assert(NULL != strstr((const char_t*)output, "<entry value=\"A661_ENABLE\"/>"));
    assert(NULL != strstr((const char_t*)output, "<prop name=\"Single\" value=\"1\"/>"));
    assert(NULL == strstr((const char_t*)output, "xyentry"));
    is_valid = Arinc661_ReadDefinition(&definition, output, byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(11 == definition.propertyCount);
    assert(0 == strcmp("BufferStructure", properties[9].name));
    assert(0 == strcmp("A661_ENABLE", properties[9].value));

    /* Single names no BufferFormat parameter; without it the flat form converts to 40-byte polyline and 16-byte format commands */
    assert(0 == Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output)));
    definition.propertyCount = 10;
    byte_count = Arinc661_WriteDefinitionBinary(&definition, output, sizeof(output));
    assert((8 + 8 + 40 + 16 + 4 + 4) == byte_count);

    /* Unsupported entry forms are rejected */
    memcpy(strstr((char_t*)input, "<entry value=\"1\"/>"), "<datum value=\"1\"/>", sizeof("<entry value=\"1\"/>") - 1);
    is_valid = Arinc661_ReadDefinition(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the ARINC 661 checks as part of the unit test executable.
 */
void RunArinc661Tests(void)
{
    TestDefinition();
    TestDefinitionWithoutDeclaration();
    TestDefinitionBinary();
    TestDefinitionBinaryLabelComplex();
    TestDefinitionBinaryArrays();
    TestDefinitionArrayProperties();
}
