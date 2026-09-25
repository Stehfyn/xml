#include "utl/utl_arinc661.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <string.h>

static uint64_t c_workspace[UTL_ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)];

/*************************************************************************************************/
/**
 * \brief Checks in-memory configuration conversion and typed values.
 */
static void TestConfiguration(void)
{
    byte_t input[] = "<?xml version=\"1.0\"?><configuration><metaData/><screen id=\"0\"><pixelSize width=\"640\" height=\"480\" units=\"pixel\"/><physicalDimensions width=\"5\" height=\"4\" units=\"inch\"/><layout id=\"0\"><window id=\"0\" name=\"window0\"><description/><pixelArea x=\"0\" y=\"0\" width=\"640\" height=\"480\"/><scalingFactor xScale=\"20.0\" yScale=\"20.0\" baseUnit=\"screen\" perUnit=\"pixel\"/></window></layout></screen><ua applicationId=\"3\" dFPath=\"display.df\" window=\"0\" visible=\"true\"/><externalSource id=\"7\" name=\"Camera\" type=\"video\"/></configuration>";
    byte_t output[4096];
    Utl_Arinc661_Screen_t screens[2];
    Utl_Arinc661_Layout_t layouts[2];
    Utl_Arinc661_Window_t windows[2];
    Utl_Arinc661_UserApplication_t applications[2];
    Utl_Arinc661_ExternalSource_t sources[2];
    Utl_Arinc661_Configuration_t configuration;
    uint64_t output_byte_count;
    bool_t is_valid;

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
    is_valid = Utl_Arinc661_ReadConfiguration(&configuration, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == configuration.screenCount);
    assert(1 == configuration.layoutCount);
    assert(1 == configuration.windowCount);
    assert(1 == configuration.userApplicationCount);
    assert(1 == configuration.externalSourceCount);
    assert(configuration.hasMetaData);
    assert(640 == screens[0].pixelSize.width);
    assert(480 == screens[0].pixelSize.height);
    assert(UTL_ARINC661_UNIT_INCH == screens[0].physicalDimensions.units);
    assert(0 == layouts[0].screenIndex);
    assert(0 == windows[0].layoutIndex);
    assert(0 == strcmp("window0", windows[0].name));
    assert(3 == applications[0].applicationId);
    assert(applications[0].visible);
    assert(0 == strcmp("display.df", applications[0].dFPath));
    assert(7 == sources[0].id);

    windows[0].pixelArea.x = 12;
    windows[0].scalingFactor.xScale = 0.00000001f;
    windows[0].scalingFactor.yScale = 100000000000000000000.0f;
    output_byte_count = Utl_Arinc661_WriteConfiguration(&configuration, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    is_valid = Utl_Arinc661_ReadConfiguration(&configuration, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(12 == windows[0].pixelArea.x);
    assert(0.00000001f == windows[0].scalingFactor.xScale);
    assert(100000000000000000000.0f == windows[0].scalingFactor.yScale);
    assert(1 == configuration.userApplicationCount);
    assert(1 == configuration.externalSourceCount);
    assert(0 == strcmp("video", sources[0].type));

    windows[0].scalingFactor.xScale = 1.0e-40f;
    windows[0].scalingFactor.yScale = FLT_MAX;
    output_byte_count = Utl_Arinc661_WriteConfiguration(&configuration, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    is_valid = Utl_Arinc661_ReadConfiguration(&configuration, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1.0e-40f == windows[0].scalingFactor.xScale);
    assert(FLT_MAX == windows[0].scalingFactor.yScale);
}

/*************************************************************************************************/
/**
 * \brief Checks signed identifier bounds without relying on errno.
 */
static void TestIntegerBounds(void)
{
    byte_t minimum_input[] = "<configuration><screen id=\"-9223372036854775808\"><pixelSize width=\"9223372036854775807\" height=\"1\"/><physicalDimensions width=\"1\" height=\"1\"/></screen></configuration>";
    byte_t overflow_input[] = "<configuration><screen id=\"9223372036854775808\"><pixelSize width=\"1\" height=\"1\"/><physicalDimensions width=\"1\" height=\"1\"/></screen></configuration>";
    Utl_Arinc661_Screen_t screen[1];
    Utl_Arinc661_Configuration_t configuration;
    bool_t is_valid;

    memset(&configuration, 0, sizeof(configuration));
    configuration.screens = screen;
    configuration.screenCapacity = 1;
    is_valid = Utl_Arinc661_ReadConfiguration(&configuration, minimum_input, sizeof(minimum_input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(INT64_MIN == screen[0].id);
    assert(INT64_MAX == screen[0].pixelSize.width);

    is_valid = Utl_Arinc661_ReadConfiguration(&configuration, overflow_input, sizeof(overflow_input) - 1, c_workspace, sizeof(c_workspace));
    assert(FALSE == is_valid);
    assert(0 == configuration.screenCount);
}

/*************************************************************************************************/
/**
 * \brief Checks in-memory Definition File conversion and nested widget ownership.
 */
static void TestDefinition(void)
{
    byte_t input[] = "<?xml version=\"1.0\"?><a661_df name=\"example\" library_version=\"0\" supp_version=\"6\"><model><prop name=\"ApplicationId\" value=\"1\"/></model><a661_layer name=\"Primary\"><model><prop name=\"LayerId\" value=\"1\"/></model><a661_widget name=\"Panel\" type=\"A661_PANEL\"><model><prop name=\"WidgetIdent\" value=\"1\"/></model><a661_widget name=\"Label\" type=\"A661_LABEL\"><model><prop name=\"WidgetIdent\" value=\"2\"/></model></a661_widget></a661_widget></a661_layer></a661_df>";
    byte_t output[2048];
    Utl_Arinc661_Layer_t layers[2];
    Utl_Arinc661_Widget_t widgets[3];
    Utl_Arinc661_Property_t properties[8];
    Utl_Arinc661_Definition_t definition;
    uint64_t output_byte_count;
    bool_t is_valid;

    memset(&definition, 0, sizeof(definition));
    definition.layers = layers;
    definition.widgets = widgets;
    definition.properties = properties;
    definition.layerCapacity = 2;
    definition.widgetCapacity = 3;
    definition.propertyCapacity = 8;
    is_valid = Utl_Arinc661_ReadDefinition(&definition, input, sizeof(input) - 1, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    assert(4 == definition.propertyCount);
    assert(UINT32_MAX == widgets[0].parentWidgetIndex);
    assert(0 == widgets[1].parentWidgetIndex);

    output_byte_count = Utl_Arinc661_WriteDefinition(&definition, output, sizeof(output), c_workspace, sizeof(c_workspace));
    assert(0 < output_byte_count);
    is_valid = Utl_Arinc661_ReadDefinition(&definition, output, output_byte_count, c_workspace, sizeof(c_workspace));
    assert(is_valid);
    assert(1 == definition.layerCount);
    assert(2 == definition.widgetCount);
    assert(4 == definition.propertyCount);
}

int main(void)
{
    TestConfiguration();
    TestIntegerBounds();
    TestDefinition();

    return 0;
}
