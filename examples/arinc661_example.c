/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6 Definition File schema:
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "utl/utl_defines.h"
#include "utl/utl_arinc661.h"

#include "os/os_assert.h"
#include "os/os_file.h"

#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define XML_LITERAL(text) (const byte_t*)(text), (uint32_t)(sizeof(text) - 1)

#define CONFIGURATION_FILE_PATH ("examples/arinc661_configuration.xml")
#define DEFINITION_FILE_PATH    ("examples/arinc661_definition.xml")
#define DEFINITION_FILE_NAME    ("arinc661_definition.xml")
#define XML_BUFFER_BYTES        (4096)

#define EXAMPLE_PIXEL_WIDTH     (640)
#define EXAMPLE_PIXEL_HEIGHT    (480)
#define EXAMPLE_PHYSICAL_WIDTH  (5)
#define EXAMPLE_PHYSICAL_HEIGHT (4)
#define EXAMPLE_SCALE           (20.0f)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Index of a model node in the example Definition File.
 */
typedef enum Arinc661ModelIndex_e
{
    ARINC661_DOCUMENT_MODEL,
    ARINC661_LAYER_MODEL,
    ARINC661_PANEL_MODEL,
    ARINC661_LABEL_MODEL,
    ARINC661_SECOND_LABEL_MODEL,
    ARINC661_MODEL_COUNT
} Arinc661ModelIndex_t;

/*************************************************************************************************/
/**
 * \brief Property name and value assigned to one example model node.
 */
typedef struct Arinc661Property_s
{
    Arinc661ModelIndex_t modelIndex;
    const byte_t*        name;
    uint32_t             nameByteCount;
    const byte_t*        value;
    uint32_t             valueByteCount;
} Arinc661Property_t;

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Properties in document order for the example Definition File.
 */
static const Arinc661Property_t c_arinc661Properties[] =
{
    {ARINC661_DOCUMENT_MODEL, XML_LITERAL("ApplicationId"),   XML_LITERAL("1")},
    {ARINC661_LAYER_MODEL,    XML_LITERAL("LayerId"),         XML_LITERAL("1")},
    {ARINC661_LAYER_MODEL,    XML_LITERAL("ContextNumber"),   XML_LITERAL("0")},
    {ARINC661_LAYER_MODEL,    XML_LITERAL("Height"),          XML_LITERAL("10000")},
    {ARINC661_LAYER_MODEL,    XML_LITERAL("Width"),           XML_LITERAL("10000")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("WidgetIdent"),     XML_LITERAL("1")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("Enable"),          XML_LITERAL("A661_TRUE")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("Visible"),         XML_LITERAL("A661_TRUE")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("PosX"),            XML_LITERAL("0")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("PosY"),            XML_LITERAL("0")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("SizeX"),           XML_LITERAL("10000")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("SizeY"),           XML_LITERAL("10000")},
    {ARINC661_PANEL_MODEL,    XML_LITERAL("StyleSet"),        XML_LITERAL("0")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("WidgetIdent"),     XML_LITERAL("2")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("Anonymous"),       XML_LITERAL("A661_FALSE")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("Visible"),         XML_LITERAL("A661_TRUE")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("PosX"),            XML_LITERAL("3000")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("PosY"),            XML_LITERAL("4000")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("SizeX"),           XML_LITERAL("4000")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("SizeY"),           XML_LITERAL("1000")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("RotationAngle"),   XML_LITERAL("0.0")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("StyleSet"),        XML_LITERAL("0")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("MaxStringLength"), XML_LITERAL("32")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("MotionAllowed"),   XML_LITERAL("A661_TRUE")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("Font"),            XML_LITERAL("t2")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("ColorIndex"),      XML_LITERAL("red")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("Alignment"),       XML_LITERAL("A661_CENTER")},
    {ARINC661_LABEL_MODEL,    XML_LITERAL("LabelString"),     XML_LITERAL("SYSTEM READY")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("WidgetIdent"),     XML_LITERAL("3")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("Anonymous"),       XML_LITERAL("A661_FALSE")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("Visible"),         XML_LITERAL("A661_TRUE")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("PosX"),            XML_LITERAL("3000")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("PosY"),            XML_LITERAL("5500")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("SizeX"),           XML_LITERAL("4000")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("SizeY"),           XML_LITERAL("1000")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("RotationAngle"),   XML_LITERAL("0.0")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("StyleSet"),        XML_LITERAL("0")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("MaxStringLength"), XML_LITERAL("32")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("MotionAllowed"),   XML_LITERAL("A661_TRUE")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("Font"),            XML_LITERAL("t2")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("ColorIndex"),      XML_LITERAL("red")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("Alignment"),       XML_LITERAL("A661_CENTER")},
    {ARINC661_SECOND_LABEL_MODEL,    XML_LITERAL("LabelString"),     XML_LITERAL("SECONDARY STATUS")}
};

/*************************************************************************************************/
/**
 * \brief Temporary XML conversion storage for the example.
 */
static uint64_t c_workspace[UTL_ARINC661_WORKSPACE_BYTES / sizeof(uint64_t)];

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Writes configuration and Definition File XML, then reads both files back.
 */
int main(void)
{
    Utl_Arinc661_Configuration_t   configuration;
    Utl_Arinc661_Screen_t          screens[1];
    Utl_Arinc661_Layout_t          layouts[1];
    Utl_Arinc661_Window_t          windows[1];
    Utl_Arinc661_UserApplication_t applications[1];
    Utl_Arinc661_Definition_t      definition_file;
    Utl_Arinc661_Layer_t           layers[1];
    Utl_Arinc661_Widget_t          widgets[3];
    Utl_Arinc661_Property_t        properties[COUNT_OF(c_arinc661Properties)];
    byte_t                         xml_buffer[XML_BUFFER_BYTES + 1];
    uint64_t                       byte_count;
    int32_t                        bytes_written;
    int32_t                        bytes_read;
    uint32_t                       index;
    uint32_t                       owner_index;
    bool_t                         is_valid;

    memset(&configuration, 0, sizeof(configuration));
    memset(screens, 0, sizeof(screens));
    memset(layouts, 0, sizeof(layouts));
    memset(windows, 0, sizeof(windows));
    memset(applications, 0, sizeof(applications));

    configuration.screens                 = screens;
    configuration.layouts                 = layouts;
    configuration.windows                 = windows;
    configuration.userApplications        = applications;
    configuration.externalSources         = NULL;
    configuration.screenCapacity          = COUNT_OF(screens);
    configuration.layoutCapacity          = COUNT_OF(layouts);
    configuration.windowCapacity          = COUNT_OF(windows);
    configuration.userApplicationCapacity = COUNT_OF(applications);
    configuration.externalSourceCapacity  = 0;
    configuration.screenCount             = COUNT_OF(screens);
    configuration.layoutCount             = COUNT_OF(layouts);
    configuration.windowCount             = COUNT_OF(windows);
    configuration.userApplicationCount    = COUNT_OF(applications);
    configuration.externalSourceCount     = 0;
    configuration.hasMetaData             = FALSE;
    configuration.hasXmlDeclaration       = TRUE;

    screens[0].id                        = 0;
    screens[0].pixelSize.width           = EXAMPLE_PIXEL_WIDTH;
    screens[0].pixelSize.height          = EXAMPLE_PIXEL_HEIGHT;
    screens[0].pixelSize.units           = UTL_ARINC661_UNIT_PIXEL;
    screens[0].physicalDimensions.width  = EXAMPLE_PHYSICAL_WIDTH;
    screens[0].physicalDimensions.height = EXAMPLE_PHYSICAL_HEIGHT;
    screens[0].physicalDimensions.units  = UTL_ARINC661_UNIT_INCH;

    layouts[0].id          = 0;
    layouts[0].screenIndex = 0;

    windows[0].id                       = 0;
    strcpy(windows[0].name, "window0");
    windows[0].description[0]           = '\0';
    windows[0].pixelArea.x              = 0;
    windows[0].pixelArea.y              = 0;
    windows[0].pixelArea.width          = EXAMPLE_PIXEL_WIDTH;
    windows[0].pixelArea.height         = EXAMPLE_PIXEL_HEIGHT;
    windows[0].scalingFactor.xScale     = EXAMPLE_SCALE;
    windows[0].scalingFactor.yScale     = EXAMPLE_SCALE;
    windows[0].scalingFactor.baseUnit   = UTL_ARINC661_UNIT_SCREEN;
    windows[0].scalingFactor.perUnit    = UTL_ARINC661_UNIT_PIXEL;
    windows[0].layoutIndex              = 0;

    applications[0].applicationId    = 1;
    applications[0].window           = windows[0].id;
    strcpy(applications[0].dFPath, DEFINITION_FILE_NAME);
    applications[0].styleFilePath[0] = '\0';
    applications[0].mapSourceLayer   = FALSE;
    applications[0].visible          = TRUE;

    memset(&definition_file, 0, sizeof(definition_file));
    memset(layers, 0, sizeof(layers));
    memset(widgets, 0, sizeof(widgets));
    memset(properties, 0, sizeof(properties));
    strcpy(definition_file.name, "arinc661_definition");
    definition_file.libraryVersion    = 0;
    definition_file.supplementVersion = 6;
    definition_file.layers            = layers;
    definition_file.widgets           = widgets;
    definition_file.properties        = properties;
    definition_file.layerCapacity     = COUNT_OF(layers);
    definition_file.widgetCapacity    = COUNT_OF(widgets);
    definition_file.propertyCapacity  = COUNT_OF(properties);
    definition_file.layerCount        = COUNT_OF(layers);
    definition_file.widgetCount       = COUNT_OF(widgets);
    definition_file.propertyCount     = COUNT_OF(properties);

    strcpy(layers[0].name, "PrimaryLayer");
    strcpy(widgets[0].name, "MainPanel");
    strcpy(widgets[0].type, "A661_PANEL");
    widgets[0].layerIndex        = 0;
    widgets[0].parentWidgetIndex = UINT32_MAX;
    strcpy(widgets[1].name, "StatusLabel");
    strcpy(widgets[1].type, "A661_LABEL");
    widgets[1].layerIndex        = 0;
    widgets[1].parentWidgetIndex = 0;
    strcpy(widgets[2].name, "SecondaryLabel");
    strcpy(widgets[2].type, "A661_LABEL");
    widgets[2].layerIndex        = 0;
    widgets[2].parentWidgetIndex = 0;

    for (index = 0; index < COUNT_OF(c_arinc661Properties); ++index)
    {
        owner_index = 0;
        if (ARINC661_DOCUMENT_MODEL == c_arinc661Properties[index].modelIndex)
        {
            properties[index].owner = UTL_ARINC661_PROPERTY_DEFINITION;
        }
        else if (ARINC661_LAYER_MODEL == c_arinc661Properties[index].modelIndex)
        {
            properties[index].owner = UTL_ARINC661_PROPERTY_LAYER;
        }
        else
        {
            properties[index].owner = UTL_ARINC661_PROPERTY_WIDGET;
            owner_index = (uint32_t)c_arinc661Properties[index].modelIndex - (uint32_t)ARINC661_PANEL_MODEL;
        }
        properties[index].ownerIndex = owner_index;
        LOGASSERT(c_arinc661Properties[index].nameByteCount < sizeof(properties[index].name));
        LOGASSERT(c_arinc661Properties[index].valueByteCount < sizeof(properties[index].value));
        memcpy(properties[index].name, c_arinc661Properties[index].name, c_arinc661Properties[index].nameByteCount);
        memcpy(properties[index].value, c_arinc661Properties[index].value, c_arinc661Properties[index].valueByteCount);
    }

    byte_count = Utl_Arinc661_WriteConfiguration(&configuration, xml_buffer, XML_BUFFER_BYTES, c_workspace, sizeof(c_workspace));
    LOGASSERT(0 < byte_count);
    LOGASSERT(byte_count <= XML_BUFFER_BYTES);
    bytes_written = OS_File_WriteFile(CONFIGURATION_FILE_PATH, xml_buffer, (uint32_t)byte_count);
    LOGASSERT(byte_count == (uint64_t)bytes_written);

    byte_count = Utl_Arinc661_WriteDefinition(&definition_file, xml_buffer, XML_BUFFER_BYTES, c_workspace, sizeof(c_workspace));
    LOGASSERT(0 < byte_count);
    LOGASSERT(byte_count <= XML_BUFFER_BYTES);
    bytes_written = OS_File_WriteFile(DEFINITION_FILE_PATH, xml_buffer, (uint32_t)byte_count);
    LOGASSERT(byte_count == (uint64_t)bytes_written);

    bytes_read = OS_File_ReadFile(CONFIGURATION_FILE_PATH, xml_buffer, sizeof(xml_buffer));
    LOGASSERT(0 < bytes_read);
    LOGASSERT(bytes_read <= XML_BUFFER_BYTES);
    is_valid = Utl_Arinc661_ReadConfiguration(&configuration, xml_buffer, (uint64_t)bytes_read, c_workspace, sizeof(c_workspace));
    LOGASSERT(is_valid);
    LOGASSERT(COUNT_OF(screens) == configuration.screenCount);
    LOGASSERT(COUNT_OF(layouts) == configuration.layoutCount);
    LOGASSERT(COUNT_OF(windows) == configuration.windowCount);
    LOGASSERT(COUNT_OF(applications) == configuration.userApplicationCount);
    LOGASSERT(0 == strcmp(applications[0].dFPath, DEFINITION_FILE_NAME));

    bytes_read = OS_File_ReadFile(DEFINITION_FILE_PATH, xml_buffer, sizeof(xml_buffer));
    LOGASSERT(0 < bytes_read);
    LOGASSERT(bytes_read <= XML_BUFFER_BYTES);
    is_valid = Utl_Arinc661_ReadDefinition(&definition_file, xml_buffer, (uint64_t)bytes_read, c_workspace, sizeof(c_workspace));
    LOGASSERT(is_valid);
    LOGASSERT(COUNT_OF(layers) == definition_file.layerCount);
    LOGASSERT(COUNT_OF(widgets) == definition_file.widgetCount);
    LOGASSERT(COUNT_OF(properties) == definition_file.propertyCount);

    return 0;
}
