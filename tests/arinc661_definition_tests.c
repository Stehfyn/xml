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
    byte_t                output[2048];
    Arinc661_Layer_t      layers[2];
    Arinc661_Widget_t     widgets[3];
    Arinc661_Property_t   properties[8];
    Arinc661_Definition_t definition;
    uint64_t              output_byte_count;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 2, widgets, 3, properties, 8, NULL, 0);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    assert(4 == definition.propertyCount);
    assert(UINT32_MAX == widgets[0].parentWidgetIndex);
    assert(0 == widgets[1].parentWidgetIndex);

    output_byte_count = Arinc661_Definition_Write(&definition, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    assert(0 == memcmp(output, "<a661_df ", sizeof("<a661_df ") - 1));
    is_valid = Arinc661_Definition_Read(&definition, output, output_byte_count, c_workspace, sizeof(c_workspace));
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
    byte_t                output[1024];
    uint64_t              byte_count;
    bool_t                is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.supplementVersion = 6;
    byte_count = Arinc661_Definition_Write(&definition, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    assert(0 == memcmp(output, "<a661_df ", sizeof("<a661_df ") - 1));
    is_valid = Arinc661_Definition_Read(&definition, output, byte_count, c_workspace, sizeof(c_workspace));
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
    byte_t                output[256];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[2];
    Arinc661_Property_t   properties[32];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    uint32_t              index;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 2, properties, 32, NULL, 0);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);

    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));
    /* An output that does not fit is rejected */
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(expected) - 1));

    is_valid = Arinc661_Definition_ReadBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    assert(1 == definition.libraryVersion);
    assert(4 == definition.supplementVersion);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    /* ApplicationId, LayerId, ContextNumber, and WidgetIdent plus eight panel and fourteen label parameters */
    assert(27 == definition.propertyCount);
    assert(0 == strncmp("26504", properties[0].value, sizeof("26504")));
    assert(0 == strncmp("A661_PANEL", widgets[0].type, sizeof("A661_PANEL")));
    assert(UINT32_MAX == widgets[0].parentWidgetIndex);
    assert(0 == strncmp("A661_LABEL", widgets[1].type, sizeof("A661_LABEL")));
    assert(0 == widgets[1].parentWidgetIndex);
    assert(0 == strncmp("WidgetIdent", properties[3].name, sizeof("WidgetIdent")));
    assert(0 == strncmp("4641", properties[3].value, sizeof("4641")));
    assert(0 == strncmp("PosX", properties[15].name, sizeof("PosX")));
    assert(0 == strncmp("-1", properties[15].value, sizeof("-1")));
    assert(0 == strncmp("RotationAngle", properties[19].name, sizeof("RotationAngle")));
    assert(0 == strncmp("90", properties[19].value, sizeof("90")));
    assert(0 == strncmp("LabelString", properties[26].name, sizeof("LabelString")));
    assert(0 == strncmp("YOU", properties[26].value, sizeof("YOU")));

    /* Values read from binary write the same bytes again */
    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));

    /* Every truncation is rejected without reading past the input */
    for (index = 0; index < sizeof(expected); ++index)
    {
        memcpy(output, expected, index);
        assert(FALSE == Arinc661_Definition_ReadBinary(&definition, output, index));
        assert(0 == definition.layerCount);
    }

    /* A symbolic value without an ARINC 661 constant, an unknown property, and an unknown widget type are rejected */
    is_valid = Arinc661_Definition_ReadBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    strcpy(properties[10].value, "red");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
    strcpy(properties[10].value, "0");
    strcpy(properties[10].name, "Unknown");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
    strcpy(properties[10].name, "StyleSet");
    strcpy(widgets[1].type, "A661_SELECTOR");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
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
    byte_t                output[256];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[2];
    Arinc661_Property_t   properties[32];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 2, properties, 32, NULL, 0);
    is_valid = Arinc661_Definition_ReadBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    assert(2 == definition.widgetCount);
    assert(24 == definition.propertyCount);
    assert(0 == strncmp("A661_LABEL_COMPLEX", widgets[1].type, sizeof("A661_LABEL_COMPLEX")));
    assert(0 == widgets[1].parentWidgetIndex);
    assert(0 == strncmp("MaxStringLength", properties[20].name, sizeof("MaxStringLength")));
    assert(0 == strncmp("80", properties[20].value, sizeof("80")));
    assert(0 == strncmp("DefaultStyleText", properties[22].name, sizeof("DefaultStyleText")));
    assert(0 == strncmp("1B42001B41001B4300000000", properties[22].value, sizeof("1B42001B41001B4300000000")));
    assert(0 == strncmp("LabelString", properties[23].name, sizeof("LabelString")));
    assert(30 == strnlen(properties[23].value, sizeof(properties[23].value)));

    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));

    /* Bytes need two hexadecimal digits each */
    strcpy(properties[22].value, "1B42001B41001B43000000");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
    strcpy(properties[22].value, "1B42001B41001B430000000G");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
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
    byte_t                output[256];
    byte_t                copy[256];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[2];
    Arinc661_Property_t   properties[64];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 2, properties, 64, NULL, 0);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);

    /* Header, layer header, 20-byte format command, 72-byte tree command, layer end, and footer */
    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert(116 == byte_count);
    assert(0 == memcmp(&output[16], expected_format, sizeof(expected_format)));
    assert((0x00 == output[38]) && (0x48 == output[39]));
    assert(0 == memcmp(&output[82], expected_arrays, sizeof(expected_arrays)));

    is_valid = Arinc661_Definition_ReadBinary(&definition, output, byte_count);
    assert(is_valid);
    assert(2 == definition.widgetCount);
    /* Three header properties, six for the format, and seventeen single plus fifteen array values for the tree */
    assert(41 == definition.propertyCount);
    assert(0 == strncmp("NumberOfFields", properties[4].name, sizeof("NumberOfFields")));
    assert(0 == strncmp("BufferStructure", properties[6].name, sizeof("BufferStructure")));
    assert(0 == strncmp("45440", properties[6].value, sizeof("45440")));
    assert(0 == strncmp("LabelStringArray", properties[39].name, sizeof("LabelStringArray")));
    assert(0 == strncmp("BC", properties[39].value, sizeof("BC")));
    assert(0 == strncmp("LabelStringArray", properties[40].name, sizeof("LabelStringArray")));
    assert(0 == strncmp("", properties[40].value, sizeof("")));

    byte_count = Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy));
    assert(116 == byte_count);
    assert(0 == memcmp(copy, output, 116));

    /* The count parameter must match the number of array properties */
    strcpy(properties[4].value, "3");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));
    strcpy(properties[4].value, "1");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));

    /* A count that promises more elements than the command holds is rejected */
    output[27] = 0x09;
    assert(FALSE == Arinc661_Definition_ReadBinary(&definition, output, 116));
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
    byte_t                output[2048];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[2];
    Arinc661_Property_t   properties[16];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 2, properties, 16, NULL, 0);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(11 == definition.propertyCount);
    assert(0 == strncmp("Vertices", properties[2].name, sizeof("Vertices")));
    assert(0 == strncmp("-500", properties[2].value, sizeof("-500")));
    assert(0 == strncmp("Vertices", properties[5].name, sizeof("Vertices")));
    assert(0 == strncmp("500", properties[5].value, sizeof("500")));
    assert(0 == strncmp("BufferStructure", properties[8].name, sizeof("BufferStructure")));
    assert(0 == strncmp("2", properties[8].value, sizeof("2")));
    assert(0 == strncmp("BufferStructure", properties[9].name, sizeof("BufferStructure")));
    assert(0 == strncmp("A661_ENABLE", properties[9].value, sizeof("A661_ENABLE")));
    assert(0 == strncmp("Single", properties[10].name, sizeof("Single")));

    /* Repeated names are written as one arrayprop of entries; a single value stays a prop */
    byte_count = Arinc661_Definition_Write(&definition, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    output[byte_count] = '\0';
    assert(NULL != strstr((const char_t*)output, "<arrayprop name=\"Vertices\">"));
    assert(NULL != strstr((const char_t*)output, "<entry value=\"-500\"/>"));
    assert(NULL != strstr((const char_t*)output, "<entry value=\"A661_ENABLE\"/>"));
    assert(NULL != strstr((const char_t*)output, "<prop name=\"Single\" value=\"1\"/>"));
    assert(NULL == strstr((const char_t*)output, "xyentry"));
    is_valid = Arinc661_Definition_Read(&definition, output, byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(11 == definition.propertyCount);
    assert(0 == strncmp("BufferStructure", properties[9].name, sizeof("BufferStructure")));
    assert(0 == strncmp("A661_ENABLE", properties[9].value, sizeof("A661_ENABLE")));

    /* Single names no BufferFormat parameter; without it the flat form converts to 40-byte polyline and 16-byte format commands */
    assert(0 == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
    definition.propertyCount = 10;
    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert((8 + 8 + 40 + 16 + 4 + 4) == byte_count);

    /* Unsupported entry forms are rejected */
    memcpy(strstr((char_t*)input, "<entry value=\"1\"/>"), "<datum value=\"1\"/>", sizeof("<entry value=\"1\"/>") - 1);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
}

/*************************************************************************************************/
/**
 * \brief Checks widget extensions (Part 1-7, section 8) in XML, binary, and round trip.
 */
static void TestDefinitionBinaryExtensions(void)
{
    byte_t input[] = "<a661_df name=\"example\" library_version=\"0\" supp_version=\"7\"><model/><a661_layer name=\"Primary\"><model/>"
                     "<a661_widget name=\"Button\" type=\"A661_PUSH_BUTTON\"><model><prop name=\"WidgetIdent\" value=\"1\"/><prop name=\"LabelString\" value=\"OK\"/></model>"
                     "<a661_extension type=\"A661_LEGEND_ALIGN_EXTENSION\"><model><prop name=\"LegendAlignment\" value=\"A661_CENTER\"/></model></a661_extension>"
                     "<a661_extension type=\"A661_DIRECTIONAL_TABBING_EXTENSION\"><model><prop name=\"NumDirections\" value=\"2\"/><arrayprop name=\"TabDirectionWidgetIdArray\"><entry value=\"2\"/><entry value=\"5\"/></arrayprop></model></a661_extension>"
                     "<a661_extension type=\"A661_TICS_ARRAY_EXTENSION\"><model><prop name=\"MaxNumberOfTicsCoarse\" value=\"4\"/><prop name=\"NumberOfTicsCoarse\" value=\"1\"/><prop name=\"MaxNumberOfTicsFine\" value=\"4\"/><prop name=\"NumberOfTicsFine\" value=\"2\"/>"
                     "<arrayprop name=\"TicsCoarseArray\"><xyentry x=\"1\" y=\"2\"/></arrayprop><arrayprop name=\"TicsFineArray\"><xyentry x=\"0.5\" y=\"1\"/><xyentry x=\"1.5\" y=\"2\"/></arrayprop></model></a661_extension>"
                     "<a661_extension type=\"A661_EXCLUDED_REGIONS_EXTENSION\"><model><prop name=\"MaxNumberOfRegions\" value=\"2\"/><arrayprop name=\"EffectivityArray\"><entry value=\"1\"/><entry value=\"0\"/></arrayprop>"
                     "<arrayprop name=\"ExcludedRegionsList\"><entry value=\"0001000000000000000000000000006400000064\"/><entry value=\"01000000\"/></arrayprop></model></a661_extension>"
                     "<a661_extension type=\"A661_STATIC_PARAM_BUFFER_EXTENSION\"><model><prop name=\"NumberOfBuffers\" value=\"2\"/><arrayprop name=\"BufferArray\"><entry value=\"0A0B0C\"/><entry value=\"\"/></arrayprop></model></a661_extension>"
                     "</a661_widget></a661_layer></a661_df>";
    /* Tables 8.5.3-2 and 8.5.1-2: ExtensionType, parameters, 32-bit padding */
    static const byte_t expected_legend[]  = {0xCA, 0x05, 0x00, 0x08, 0x80, 0x04, 0x15, 0x00};
    static const byte_t expected_tabbing[] = {0xCA, 0x05, 0x00, 0x0C, 0x80, 0x03, 0x02, 0x00, 0x00, 0x02, 0x00, 0x05};
    /* Table 8.6.3-2: pad, four counts, one coarse pair, two fine pairs as IEEE 754 */
    static const byte_t expected_tics[] =
    {
        0xCA, 0x05, 0x00, 0x28, 0x80, 0x0B, 0x00, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02,
        0x3F, 0x80, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x3F, 0x00, 0x00, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x3F, 0xC0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00
    };
    /* Table 8.6.7-2: count, pad, effectivity bytes, alignment, a rectangle and an unused region */
    static const byte_t expected_regions[] =
    {
        0xCA, 0x05, 0x00, 0x24, 0x80, 0x0E, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64,
        0x01, 0x00, 0x00, 0x00
    };
    /* Tables 8.6.5-2 and 8.6.5-3: count, then each buffer as size, pad, bytes, alignment */
    static const byte_t expected_buffers[] =
    {
        0xCA, 0x05, 0x00, 0x14, 0x80, 0x0D, 0x00, 0x02,
        0x00, 0x03, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    byte_t                output[4096];
    byte_t                copy[512];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[8];
    Arinc661_Property_t   properties[64];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    uint64_t              offset;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 8, properties, 64, NULL, 0);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(6 == definition.widgetCount);
    assert(FALSE == widgets[0].isExtension);
    assert(TRUE == widgets[1].isExtension);
    assert(0 == widgets[1].parentWidgetIndex);
    assert(0 == strncmp("A661_LEGEND_ALIGN_EXTENSION", widgets[1].type, sizeof("A661_LEGEND_ALIGN_EXTENSION")));

    /* Header 8, layer header 8, push button command 40 (10 + 2 + 16 + 6 + 2 + "OK" + pad), then the extensions */
    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    offset = 8 + 8 + 40;
    assert((offset + 8 + 12 + 40 + 36 + 20 + 4 + 4) == byte_count);
    assert(0 == memcmp(&output[offset], expected_legend, sizeof(expected_legend)));
    offset += sizeof(expected_legend);
    assert(0 == memcmp(&output[offset], expected_tabbing, sizeof(expected_tabbing)));
    offset += sizeof(expected_tabbing);
    assert(0 == memcmp(&output[offset], expected_tics, sizeof(expected_tics)));
    offset += sizeof(expected_tics);
    assert(0 == memcmp(&output[offset], expected_regions, sizeof(expected_regions)));
    offset += sizeof(expected_regions);
    assert(0 == memcmp(&output[offset], expected_buffers, sizeof(expected_buffers)));

    is_valid = Arinc661_Definition_ReadBinary(&definition, output, byte_count);
    assert(is_valid);
    assert(6 == definition.widgetCount);
    assert(TRUE == widgets[5].isExtension);
    assert(0 == widgets[5].parentWidgetIndex);
    assert(0 == strncmp("A661_STATIC_PARAM_BUFFER_EXTENSION", widgets[5].type, sizeof("A661_STATIC_PARAM_BUFFER_EXTENSION")));
    assert(0 == strncmp("ExcludedRegionsList", properties[definition.propertyCount - 5].name, sizeof("ExcludedRegionsList")));
    assert(0 == strncmp("0001000000000000000000000000006400000064", properties[definition.propertyCount - 5].value, sizeof("0001000000000000000000000000006400000064")));
    assert(0 == strncmp("BufferArray", properties[definition.propertyCount - 1].name, sizeof("BufferArray")));
    assert(0 == strncmp("", properties[definition.propertyCount - 1].value, sizeof("")));
    assert(0 == strncmp("TicsFineArray", properties[definition.propertyCount - 9].name, sizeof("TicsFineArray")));
    assert(0 == strncmp("2", properties[definition.propertyCount - 9].value, sizeof("2")));
    assert(0 == Arinc661_Definition_WriteBinary(&definition, copy, 0));
    assert(byte_count == Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));
    assert(0 == memcmp(copy, output, byte_count));

    /* The XML form writes the extensions back as a661_extension elements */
    byte_count = Arinc661_Definition_Write(&definition, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    output[byte_count] = '\0';
    assert(NULL != strstr((const char_t*)output, "<a661_extension type=\"A661_LEGEND_ALIGN_EXTENSION\">"));

    /* An extension before any widget, a region of unknown type, and an extension on an extension are rejected */
    widgets[1].parentWidgetIndex = UINT32_MAX;
    assert(0 == Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));
    widgets[1].parentWidgetIndex = 0;
    strcpy(properties[definition.propertyCount - 4].value, "0004000000000000");
    assert(0 == Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));
    strcpy(properties[definition.propertyCount - 4].value, "01000000");
    widgets[2].parentWidgetIndex = 1;
    assert(0 == Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));
    widgets[2].parentWidgetIndex = 0;
    assert(byte_count != Arinc661_Definition_WriteBinary(&definition, copy, sizeof(copy)));
    memcpy(strstr((char_t*)input, "<a661_widget name=\"Button\""), "<a661_extensio", sizeof("<a661_extensio") - 1);
    assert(FALSE == Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace)));
}

/*************************************************************************************************/
/**
 * \brief Checks the charset encoding block and byte-for-byte retention of symbol, picture, and animation law blocks.
 */
static void TestDefinitionBinaryBlocks(void)
{
    static const byte_t expected[] =
    {
        0xA6, 0x61, 0x00, 0x07, 0x00, 0x01, 0x00, 0x00,
        /* Table 4.5.3.2-6 charset encoding block: A661_UTF8 */
        0xC8, 0x00, 0x00, 0x02, 0xC9, 0x00, 0x00, 0x00,
        /* Table 4.5.3.2-5 symbol block, 16 bytes, content not interpreted */
        0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xCA, 0x04, 0x00, 0x04, 0xF8, 0x00, 0x00, 0x00,
        /* Table 7-1 picture block: indexed pixels, one 1x1 picture, two-entry luminance-alpha color table, no reserved word */
        0xBB, 0xC2, 0x01, 0x02, 0xC1, 0x00, 0x00, 0x00, 0x10, 0xFF, 0x20, 0xFF,
        0xBC, 0x00, 0x00, 0x05, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0xBD, 0x00, 0x00, 0x00,
        0xBE, 0x00, 0x00, 0x00,
        /* Table 7-1 picture block: RGBA, one 2x1 picture, no color table, reserved word before the end */
        0xBB, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xBC, 0x00, 0x00, 0x06, 0x00, 0x02, 0x00, 0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xBD, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xBE, 0x00, 0x00, 0x00,
        /* Table 9.2-1 animation law block, 12 bytes */
        0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xCB, 0x00, 0x00, 0x00,
        0xA0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xC0, 0x00, 0x00, 0x00,
        0xE0, 0x00, 0x00, 0x00
    };
    byte_t                output[1024];
    byte_t                blocks[128];
    Arinc661_Layer_t      layers[1];
    Arinc661_Property_t   properties[8];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    uint32_t              index;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, NULL, 0, properties, 8, blocks, sizeof(blocks));
    is_valid = Arinc661_Definition_ReadBinary(&definition, expected, sizeof(expected));
    assert(is_valid);
    assert(ARINC661_CHARSET_ENCODING_UTF8 == definition.charsetEncoding);
    assert(1 == definition.layerCount);
    assert((16 + 32 + 36 + 12) == definition.blockByteCount);
    assert(0 == memcmp(blocks, &expected[16], definition.blockByteCount));

    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert(sizeof(expected) == byte_count);
    assert(0 == memcmp(output, expected, sizeof(expected)));

    /* The XML form carries the charset encoding and drops the binary-only blocks */
    byte_count = Arinc661_Definition_Write(&definition, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    output[byte_count] = '\0';
    assert(NULL != strstr((const char_t*)output, "<charset-encoding name=\"A661_UTF8\"/>"));
    is_valid = Arinc661_Definition_Read(&definition, output, byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(ARINC661_CHARSET_ENCODING_UTF8 == definition.charsetEncoding);
    assert(0 == definition.blockByteCount);

    /* Every truncation is rejected without reading past the input */
    for (index = 0; index < sizeof(expected); ++index)
    {
        memcpy(output, expected, index);
        assert(FALSE == Arinc661_Definition_ReadBinary(&definition, output, index));
        assert(0 == definition.blockByteCount);
    }

    /* Too little block storage, an unknown pixel format, and a second charset block are rejected */
    definition.blockCapacity = 95;
    assert(FALSE == Arinc661_Definition_ReadBinary(&definition, expected, sizeof(expected)));
    definition.blockCapacity = sizeof(blocks);
    memcpy(output, expected, sizeof(expected));
    output[33] = 0xC9;
    assert(FALSE == Arinc661_Definition_ReadBinary(&definition, output, sizeof(expected)));
    memcpy(output, expected, sizeof(expected));
    memcpy(&output[16], &expected[8], 8);
    assert(FALSE == Arinc661_Definition_ReadBinary(&definition, output, sizeof(expected)));
}

/*************************************************************************************************/
/**
 * \brief Checks the Creation Structure schema exposed for editors.
 */
/*************************************************************************************************/
/**
 * \brief Checks that floats and fr() angles keep A661 precision through the binary form with the shortest text.
 */
static void TestDefinitionBinaryPrecision(void)
{
    /* Table 3.5.4-2 Slider holds floats; Table 3.3.20-2 Label holds a fr(180) angle */
    byte_t input[] = "<a661_df name=\"example\" library_version=\"0\" supp_version=\"7\"><model/><a661_layer name=\"Primary\"><model/>"
                     "<a661_widget name=\"Slide\" type=\"A661_SLIDER\"><model><prop name=\"WidgetIdent\" value=\"1\"/><prop name=\"MinValue\" value=\"0.1\"/><prop name=\"MaxValue\" value=\"123456.7\"/><prop name=\"Value\" value=\"100\"/></model></a661_widget>"
                     "<a661_widget name=\"Text\" type=\"A661_LABEL\"><model><prop name=\"WidgetIdent\" value=\"2\"/><prop name=\"RotationAngle\" value=\"0.0000001\"/></model></a661_widget></a661_layer></a661_df>";
    byte_t                output[512];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[2];
    Arinc661_Property_t   properties[64];
    Arinc661_Definition_t definition;
    uint64_t              byte_count;
    uint32_t              index;
    uint32_t              found;
    bool_t                is_valid;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 2, properties, 64, NULL, 0);
    is_valid = Arinc661_Definition_Read(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    byte_count = Arinc661_Definition_WriteBinary(&definition, output, sizeof(output));
    assert(0 < byte_count);
    is_valid = Arinc661_Definition_ReadBinary(&definition, output, byte_count);
    assert(is_valid);

    /* Floats come back as the shortest decimal that is the same IEEE 754 single; 123456.7 is exactly 123456.703125 */
    found = 0;
    for (index = 0; index < definition.propertyCount; ++index)
    {
        if ((ARINC661_PROPERTY_WIDGET == properties[index].owner) && (0 == properties[index].ownerIndex))
        {
            if (0 == strncmp("MinValue", properties[index].name, sizeof("MinValue")))
            {
                assert(0 == strncmp("0.1", properties[index].value, sizeof("0.1")));
                found += 1;
            }
            else if (0 == strncmp("MaxValue", properties[index].name, sizeof("MaxValue")))
            {
                assert(0 == strncmp("123456.7", properties[index].value, sizeof("123456.7")));
                found += 1;
            }
            else if (0 == strncmp("Value", properties[index].name, sizeof("Value")))
            {
                assert(0 == strncmp("100", properties[index].value, sizeof("100")));
                found += 1;
            }
        }
        /* One fr(180) LSB is 8.38e-8 degrees; the shortest text that encodes to one LSB is 0.0000001 */
        else if ((ARINC661_PROPERTY_WIDGET == properties[index].owner) && (1 == properties[index].ownerIndex) && (0 == strncmp("RotationAngle", properties[index].name, sizeof("RotationAngle"))))
        {
            assert(0 == strncmp("0.0000001", properties[index].value, sizeof("0.0000001")));
            found += 1;
        }
    }
    assert(4 == found);
}

/*************************************************************************************************/
/**
 * \brief Checks the Creation Structure schema exposed for editors.
 */
static void TestDefinitionSchema(void)
{
    Arinc661_Parameter_t parameter;
    uint32_t             parameter_count;
    uint32_t             type_index;
    uint32_t             found;

    assert(100 == Arinc661_Definition_GetTypeCount(FALSE));
    assert(20 == Arinc661_Definition_GetTypeCount(TRUE));
    found = 0;
    for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(FALSE); ++type_index)
    {
        assert(Arinc661_Definition_FindType(Arinc661_Definition_GetTypeName(FALSE, type_index), FALSE, &parameter_count));
        if (0 == strncmp("A661_LABEL", Arinc661_Definition_GetTypeName(FALSE, type_index), sizeof("A661_LABEL")))
        {
            found += 1;
        }
    }
    assert(1 == found);
    assert(FALSE == Arinc661_Definition_FindType("A661_SELECTOR", FALSE, &parameter_count));
    assert(FALSE == Arinc661_Definition_FindType("A661_LABEL", TRUE, &parameter_count));

    /* Table 3.3.20-2: fourteen named parameters, Alignment listing nine constants */
    assert(Arinc661_Definition_FindType("A661_LABEL", FALSE, &parameter_count));
    assert(14 == parameter_count);
    assert(Arinc661_Definition_GetParameter("A661_LABEL", FALSE, 12, &parameter));
    assert(0 == strncmp("Alignment", parameter.name, sizeof("Alignment")));
    assert(ARINC661_PARAMETER_UCHAR == parameter.kind);
    assert(9 == parameter.valueCount);
    assert(0 == strncmp("A661_BOTTOM_CENTER", parameter.values[0], sizeof("A661_BOTTOM_CENTER")));
    assert(Arinc661_Definition_GetParameter("A661_LABEL", FALSE, 13, &parameter));
    assert(ARINC661_PARAMETER_STRING == parameter.kind);
    assert(0 == parameter.elementsPerEntry);
    assert(FALSE == Arinc661_Definition_GetParameter("A661_LABEL", FALSE, 14, &parameter));

    /* Table 3.3.27-2: Panel keeps its pad out of the count; Enable lists three constants */
    assert(Arinc661_Definition_FindType("A661_PANEL", FALSE, &parameter_count));
    assert(8 == parameter_count);
    assert(Arinc661_Definition_GetParameter("A661_PANEL", FALSE, 0, &parameter));
    assert(0 == strncmp("Enable", parameter.name, sizeof("Enable")));
    assert(3 == parameter.valueCount);

    /* Table 8.6.3-2: each tics array names its own count; Table 8.7.1-2 has no named parameter */
    assert(Arinc661_Definition_FindType("A661_TICS_ARRAY_EXTENSION", TRUE, &parameter_count));
    assert(6 == parameter_count);
    assert(Arinc661_Definition_GetParameter("A661_TICS_ARRAY_EXTENSION", TRUE, 5, &parameter));
    assert(0 == strncmp("TicsFineArray", parameter.name, sizeof("TicsFineArray")));
    assert(2 == parameter.elementsPerEntry);
    assert(0 == strncmp("NumberOfTicsFine", parameter.countName, sizeof("NumberOfTicsFine")));
    assert(ARINC661_PARAMETER_FLOAT == parameter.kind);
    assert(Arinc661_Definition_FindType("A661_REORDER_EXTENSION", TRUE, &parameter_count));
    assert(0 == parameter_count);

    assert(0 < Arinc661_Definition_GetConstantCount());
    found = 0;
    for (type_index = 0; type_index < Arinc661_Definition_GetConstantCount(); ++type_index)
    {
        if (0 == strncmp("A661_TRUE", Arinc661_Definition_GetConstantName(type_index), sizeof("A661_TRUE")))
        {
            assert(1 == Arinc661_Definition_GetConstantValue(type_index));
            found += 1;
        }
    }
    assert(1 == found);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the ARINC 661 checks as part of the unit test executable.
 */
/*************************************************************************************************/
/**
 * \par Details:
 *  Table 8.4 rows read from the specification: ActiveArea takes the style set extension, a
 *  BasicContainer the reorder extension, a BufferFormat only the static parameter buffer, and
 *  GpArcCircle takes no word wrap.
 */
static void TestExtensionApplicability(void)
{
    assert(Arinc661_Definition_ExtensionApplies("A661_STYLE_SET_EXTENSION", "A661_ACTIVE_AREA"));
    assert(Arinc661_Definition_ExtensionApplies("A661_REORDER_EXTENSION", "A661_BASIC_CONTAINER"));
    assert(Arinc661_Definition_ExtensionApplies("A661_STATIC_PARAM_BUFFER_EXTENSION", "A661_BUFFER_FORMAT"));
    assert(FALSE == Arinc661_Definition_ExtensionApplies("A661_PICTURE_EXTENSION", "A661_BUFFER_FORMAT"));
    assert(FALSE == Arinc661_Definition_ExtensionApplies("A661_WORD_WRAP_EXTENSION", "A661_GP_ARC_CIRCLE"));
    assert(NULL != Arinc661_Definition_GetExtensionTarget("A661_WORD_WRAP_EXTENSION", 0));
    assert(NULL == Arinc661_Definition_GetExtensionTarget("A661_NO_SUCH_EXTENSION", 0));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Table 4.6-13 reserves 0xB800 to 0xBFFF for OEM encodings, which round-trip
 *  through the binary form; a reserved value outside both ranges is rejected.
 */
static void TestDefinitionCharsetEncodings(void)
{
    /* Header, a charset block holding the first OEM encoding, and the footer */
    static const byte_t oem_file[] =
    {
        0xA6, 0x61, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00,
        0xC8, 0x00, 0xB8, 0x00, 0xC9, 0x00, 0x00, 0x00,
        0xE0, 0x00, 0x00, 0x00
    };
    byte_t                output[sizeof(oem_file)];
    Arinc661_Layer_t      layers[1];
    Arinc661_Widget_t     widgets[1];
    Arinc661_Property_t   properties[4];
    Arinc661_Definition_t definition;

    Arinc661_Definition_Initialize(&definition, layers, 1, widgets, 1, properties, 4, NULL, 0);
    assert(Arinc661_Definition_ReadBinary(&definition, oem_file, sizeof(oem_file)));
    assert(ARINC661_CHARSET_ENCODING_OEM_FIRST == definition.charsetEncoding);
    assert(sizeof(oem_file) == Arinc661_Definition_WriteBinary(&definition, output, sizeof(output)));
    assert(0 == memcmp(oem_file, output, sizeof(oem_file)));

    /* 0x0004 lies between the standard and the OEM values */
    output[11] = 0x04;
    output[10] = 0x00;
    assert(FALSE == Arinc661_Definition_ReadBinary(&definition, output, sizeof(output)));
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs every ARINC 661 Definition File test in order.
 */
void RunArinc661Tests(void)
{
    TestExtensionApplicability();
    TestDefinitionCharsetEncodings();
    TestDefinition();
    TestDefinitionWithoutDeclaration();
    TestDefinitionBinary();
    TestDefinitionBinaryLabelComplex();
    TestDefinitionBinaryArrays();
    TestDefinitionArrayProperties();
    TestDefinitionBinaryExtensions();
    TestDefinitionBinaryBlocks();
    TestDefinitionBinaryPrecision();
    TestDefinitionSchema();
}
