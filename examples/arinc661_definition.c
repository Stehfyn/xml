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

#include <stdio.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define XML_LITERAL(text) (const byte_t*)(text), (uint32_t)(sizeof(text) - 1)

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
 *  Builds an ARINC 661 Supplement 6 Definition File and writes an XML artifact.
 */
int main(void)
{
    Utl_Arinc661_Definition_t definition_file;
    Utl_Arinc661_Layer_t layers[1];
    Utl_Arinc661_Widget_t widgets[3];
    Utl_Arinc661_Property_t properties[COUNT_OF(c_arinc661Properties)];
    byte_t output[4096];
    FILE* xml_file;
    uint64_t byte_count;
    uint64_t bytes_written;
    uint32_t index;
    uint32_t owner_index;

    memset(&definition_file, 0, sizeof(definition_file));
    memset(layers, 0, sizeof(layers));
    memset(widgets, 0, sizeof(widgets));
    memset(properties, 0, sizeof(properties));
    strcpy(definition_file.name, "arinc661_definition");
    definition_file.libraryVersion = 0;
    definition_file.supplementVersion = 6;
    definition_file.layers = layers;
    definition_file.widgets = widgets;
    definition_file.properties = properties;
    definition_file.layerCapacity = COUNT_OF(layers);
    definition_file.widgetCapacity = COUNT_OF(widgets);
    definition_file.propertyCapacity = COUNT_OF(properties);
    definition_file.layerCount = COUNT_OF(layers);
    definition_file.widgetCount = COUNT_OF(widgets);
    definition_file.propertyCount = COUNT_OF(properties);

    strcpy(layers[0].name, "PrimaryLayer");
    strcpy(widgets[0].name, "MainPanel");
    strcpy(widgets[0].type, "A661_PANEL");
    widgets[0].layerIndex = 0;
    widgets[0].parentWidgetIndex = UINT32_MAX;
    strcpy(widgets[1].name, "StatusLabel");
    strcpy(widgets[1].type, "A661_LABEL");
    widgets[1].layerIndex = 0;
    widgets[1].parentWidgetIndex = 0;
    strcpy(widgets[2].name, "SecondaryLabel");
    strcpy(widgets[2].type, "A661_LABEL");
    widgets[2].layerIndex = 0;
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

    byte_count = Utl_Arinc661_WriteDefinition(&definition_file, output, sizeof(output), c_workspace, sizeof(c_workspace));
    LOGASSERT(0 < byte_count);
    xml_file = fopen("examples/arinc661_definition.xml", "wb");
    LOGASSERT(NULL != xml_file);
    bytes_written = fwrite(output, 1, byte_count, xml_file);
    LOGASSERT(byte_count == bytes_written);
    fclose(xml_file);
    return 0;
}
