#include "faceoe/faceoe_displaymanagement.h"
#include "faceoe_utl/faceoe_utl_defines.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <string.h>

static uint64_t c_workspace[FACEOE_DISPLAYMANAGEMENT_WORKSPACE_BYTES / sizeof(uint64_t)];

/*************************************************************************************************/
/**
 * \brief Checks reading, writing, and rereading every configuration element, exact scale round trips,
 *        an empty window name left out, and a scale with an exponent rejected.
 */
static void TestConfiguration(void)
{
    byte_t input[] = "<?xml version=\"1.0\"?><configuration><metaData/><screen id=\"0\"><pixelSize width=\"640\" height=\"480\" units=\"pixel\"/><physicalDimensions width=\"5\" height=\"4\" units=\"inch\"/><layout id=\"0\"><window id=\"0\" name=\"window0\"><description/><pixelArea x=\"0\" y=\"0\" width=\"640\" height=\"480\"/><scalingFactor xScale=\"20.0\" yScale=\"20.0\" baseUnit=\"screen\" perUnit=\"pixel\"/></window></layout></screen><ua applicationId=\"3\" dFPath=\"display.df\" window=\"0\" visible=\"true\"/><externalSource id=\"7\" name=\"Camera\" type=\"video\"/></configuration>";
    byte_t                                     output[4096];
    FACEOE_DisplayManagement_Screen_t          screens[2];
    FACEOE_DisplayManagement_Layout_t          layouts[2];
    FACEOE_DisplayManagement_Window_t          windows[2];
    FACEOE_DisplayManagement_UserApplication_t applications[2];
    FACEOE_DisplayManagement_ExternalSource_t  sources[2];
    FACEOE_DisplayManagement_Configuration_t   configuration;
    uint64_t                                   output_byte_count;
    char_t*                                    scale_text;
    bool_t                                     is_valid;

    memset(&configuration, 0, sizeof(configuration));
    configuration.screens = screens;
    configuration.layouts = layouts;
    configuration.windows = windows;
    configuration.userApplications = applications;
    configuration.externalSources = sources;
    configuration.screenCapacity = 2;
    configuration.layoutCapacity = 2;
    configuration.windowCapacity = 2;
    configuration.userApplicationCapacity = 2;
    configuration.externalSourceCapacity = 2;
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == configuration.screenCount);
    assert(1 == configuration.layoutCount);
    assert(1 == configuration.windowCount);
    assert(1 == configuration.userApplicationCount);
    assert(1 == configuration.externalSourceCount);
    assert(configuration.hasMetaData);
    assert(configuration.hasXmlDeclaration);
    assert(640 == screens[0].pixelSize.width);
    assert(480 == screens[0].pixelSize.height);
    assert(FACEOE_DISPLAYMANAGEMENT_UNIT_INCH == screens[0].physicalDimensions.units);
    assert(0 == layouts[0].screenIndex);
    assert(0 == windows[0].layoutIndex);
    assert(0 == strncmp("window0", windows[0].name, sizeof("window0")));
    assert(3 == applications[0].applicationId);
    assert(applications[0].visible);
    assert(0 == strncmp("display.df", applications[0].dFPath, sizeof("display.df")));
    assert(7 == sources[0].id);

    windows[0].pixelArea.x = 12;
    windows[0].scalingFactor.xScale = 0.00000001f;
    windows[0].scalingFactor.yScale = 100000000000000000000.0f;
    output_byte_count = FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    assert(0 == memcmp(output, "<?xml version=\"1.0\"?>\n", sizeof("<?xml version=\"1.0\"?>\n") - 1));
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(12 == windows[0].pixelArea.x);
    assert(0.00000001f == windows[0].scalingFactor.xScale);
    assert(100000000000000000000.0f == windows[0].scalingFactor.yScale);
    assert(1 == configuration.userApplicationCount);
    assert(1 == configuration.externalSourceCount);
    assert(0 == strncmp("video", sources[0].type, sizeof("video")));

    windows[0].scalingFactor.xScale = 1.0e-40f;
    windows[0].scalingFactor.yScale = FLT_MAX;
    output_byte_count = FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1.0e-40f == windows[0].scalingFactor.xScale);
    assert(FLT_MAX == windows[0].scalingFactor.yScale);

    /* An empty window name is left out, like an empty external source name, and reads back empty */
    windows[0].name[0] = '\0';
    output_byte_count = FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    output[output_byte_count] = '\0';
    assert(NULL == strstr((const char_t*)output, "name=\"\""));
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert('\0' == windows[0].name[0]);

    /* A scale with an exponent is rejected */
    scale_text = strstr((char_t*)input, "xScale=\"20.0\"");
    assert(NULL != scale_text);
    memcpy(scale_text + (sizeof("xScale=\"") - 1), "1e03", 4);
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
    assert(0 == configuration.screenCount);
}

/*************************************************************************************************/
/**
 * \brief Checks signed 64-bit integer limits, overflow, and trailing characters.
 */
static void TestIntegerBounds(void)
{
    byte_t minimum_input[] = "<configuration><screen id=\"-9223372036854775808\"><pixelSize width=\"9223372036854775807\" height=\"1\"/><physicalDimensions width=\"1\" height=\"1\"/></screen></configuration>";
    byte_t overflow_input[] = "<configuration><screen id=\"9223372036854775808\"><pixelSize width=\"1\" height=\"1\"/><physicalDimensions width=\"1\" height=\"1\"/></screen></configuration>";
    byte_t trailing_input[] = "<configuration><screen id=\"12x\"><pixelSize width=\"1\" height=\"1\"/><physicalDimensions width=\"1\" height=\"1\"/></screen></configuration>";
    FACEOE_DisplayManagement_Screen_t        screen[1];
    FACEOE_DisplayManagement_Configuration_t configuration;
    bool_t                                   is_valid;

    memset(&configuration, 0, sizeof(configuration));
    configuration.screens = screen;
    configuration.screenCapacity = 1;
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, minimum_input, sizeof(minimum_input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(INT64_MIN == screen[0].id);
    assert(INT64_MAX == screen[0].pixelSize.width);

    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, overflow_input, sizeof(overflow_input) - 1, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
    assert(0 == configuration.screenCount);

    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, trailing_input, sizeof(trailing_input) - 1, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
    assert(0 == configuration.screenCount);
}

/*************************************************************************************************/
/**
 * \brief Checks schema location and XML declaration handling on read and write.
 */
static void TestConfigurationSchema(void)
{
    static const char_t historical[] =
        "<configuration xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
        "xsi:noNamespaceSchemaLocation=\"H3.xsd\">"
        "<screen id=\"2\"><pixelSize width=\"640\" height=\"480\"/>"
        "<physicalDimensions width=\"5\" height=\"4\" units=\"inch\"/></screen>"
        "</configuration>";
    static const char_t* invalid_inputs[] =
    {
        "<configuration xsi:noNamespaceSchemaLocation=\"H3.xsd\"/>",
        "<configuration xmlns:xsi=\"urn:wrong\" xsi:noNamespaceSchemaLocation=\"H3.xsd\"/>"
    };
    FACEOE_DisplayManagement_Configuration_t configuration;
    FACEOE_DisplayManagement_Screen_t        screen[1];
    byte_t                                   input[2048];
    byte_t                                   output[2048];
    uint64_t                                 byte_count;
    uint32_t                                 index;
    bool_t                                   is_valid;

    memset(&configuration, 0, sizeof(configuration));
    configuration.screens = screen;
    configuration.screenCapacity = 1;
    memcpy(input, historical, sizeof(historical));
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, input, sizeof(historical) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(FALSE == configuration.hasXmlDeclaration);
    assert(0 == strncmp(configuration.schemaLocation, "H3.xsd", sizeof("H3.xsd")));
    assert(1 == configuration.screenCount);
    assert(640 == screen[0].pixelSize.width);

    // Prove that the schema hint is copied rather than pointing into the input
    memset(input, 0, sizeof(input));
    byte_count = FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    assert(byte_count < sizeof(output));
    output[byte_count] = '\0';
    assert(0 == memcmp(output, "<configuration ", sizeof("<configuration ") - 1));
    assert(NULL != strstr((const char_t*)output, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""));
    assert(NULL != strstr((const char_t*)output, "xsi:noNamespaceSchemaLocation=\"H3.xsd\""));
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, output, byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(FALSE == configuration.hasXmlDeclaration);
    assert(1 == configuration.screenCount);

    // Custom locations and the declaration are independent output settings
    strcpy(configuration.schemaLocation, "schemas/display.xsd");
    configuration.hasXmlDeclaration = TRUE;
    byte_count = FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    output[byte_count] = '\0';
    assert(0 == memcmp(output, "<?xml version=\"1.0\"?>\n", sizeof("<?xml version=\"1.0\"?>\n") - 1));
    assert(NULL != strstr((const char_t*)output, "xsi:noNamespaceSchemaLocation=\"schemas/display.xsd\""));
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, output, byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(configuration.hasXmlDeclaration);
    assert(0 == strncmp(configuration.schemaLocation, "schemas/display.xsd", sizeof("schemas/display.xsd")));

    // Rereading clears the declaration flag and schema location; writing then uses H3.xsd
    strcpy((char_t*)input, "<configuration/>");
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, input, strnlen((const char_t*)input, sizeof(input)), c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(FALSE == configuration.hasXmlDeclaration);
    assert('\0' == configuration.schemaLocation[0]);
    byte_count = FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, sizeof(output) - 1, c_workspace, sizeof(c_workspace));
    assert(0 < byte_count);
    output[byte_count] = '\0';
    assert(NULL != strstr((const char_t*)output, "xsi:noNamespaceSchemaLocation=\"H3.xsd\""));
    assert(0 == FACEOE_DisplayManagement_WriteConfiguration(&configuration, output, byte_count - 1, c_workspace, sizeof(c_workspace)));

    for (index = 0; index < COUNT_OF(invalid_inputs); ++index)
    {
        strcpy((char_t*)input, invalid_inputs[index]);
        configuration.hasXmlDeclaration = TRUE;
        strcpy(configuration.schemaLocation, "stale.xsd");
        is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, input, strnlen((const char_t*)input, sizeof(input)), c_workspace, sizeof(c_workspace));
        assert(FALSE == is_valid);
        assert(0 == configuration.screenCount);
        assert(FALSE == configuration.hasXmlDeclaration);
        assert('\0' == configuration.schemaLocation[0]);
    }

    // An oversized schema location fails and leaves schemaLocation empty
    strcpy((char_t*)input, "<configuration xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"");
    byte_count = strnlen((const char_t*)input, sizeof(input));
    memset(input + byte_count, 'a', FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES);
    byte_count += FACEOE_DISPLAYMANAGEMENT_MAX_PATH_BYTES;
    strcpy((char_t*)input + byte_count, "\"/>");
    byte_count += sizeof("\"/>") - 1;
    is_valid = FACEOE_DisplayManagement_ReadConfiguration(&configuration, input, byte_count, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
    assert('\0' == configuration.schemaLocation[0]);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Runs the FACE display management checks as part of the unit test executable.
 */
void RunFACEOEDisplayManagementTests(void)
{
    TestConfiguration();
    TestIntegerBounds();
    TestConfigurationSchema();
}
