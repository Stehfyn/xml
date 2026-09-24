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
#include "utl/utl_xml.h"

#include "os/os_assert.h"

#include <stdio.h>

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
    Utl_Xml_Document_t  document;
    Utl_Xml_Node_t      declaration;
    Utl_Xml_Node_t      definition;
    Utl_Xml_Node_t      layer;
    Utl_Xml_Node_t      panel;
    Utl_Xml_Node_t      label;
    Utl_Xml_Node_t      second_label;
    Utl_Xml_Node_t      models[ARINC661_MODEL_COUNT];
    Utl_Xml_Node_t      property_nodes[COUNT_OF(c_arinc661Properties)];
    Utl_Xml_Attribute_t definition_attributes[3];
    Utl_Xml_Attribute_t layer_attribute;
    Utl_Xml_Attribute_t panel_attributes[2];
    Utl_Xml_Attribute_t label_attributes[2];
    Utl_Xml_Attribute_t second_label_attributes[2];
    Utl_Xml_Attribute_t property_names[COUNT_OF(c_arinc661Properties)];
    Utl_Xml_Attribute_t property_values[COUNT_OF(c_arinc661Properties)];
    byte_t              output[4096];
    FILE*               xml_file;
    uint64_t            byte_count;
    uint64_t            bytes_written;
    uint32_t            index;
    int                 close_result;

    Utl_Xml_InitializeDocument(&document);
    Utl_Xml_InitializeNode(&declaration, UTL_XML_NODE_XML_DECLARATION, NULL, 0, NULL, 0);
    Utl_Xml_InitializeNode(&definition, UTL_XML_NODE_ELEMENT, XML_LITERAL("a661_df"), NULL, 0);
    Utl_Xml_InitializeNode(&layer, UTL_XML_NODE_ELEMENT, XML_LITERAL("a661_layer"), NULL, 0);
    Utl_Xml_InitializeNode(&panel, UTL_XML_NODE_ELEMENT, XML_LITERAL("a661_widget"), NULL, 0);
    Utl_Xml_InitializeNode(&label, UTL_XML_NODE_ELEMENT, XML_LITERAL("a661_widget"), NULL, 0);
    Utl_Xml_InitializeNode(&second_label, UTL_XML_NODE_ELEMENT, XML_LITERAL("a661_widget"), NULL, 0);

    for (index = 0; index < ARINC661_MODEL_COUNT; ++index)
    {
        Utl_Xml_InitializeNode(&models[index], UTL_XML_NODE_ELEMENT, XML_LITERAL("model"), NULL, 0);
    }

    Utl_Xml_InitializeAttribute(&definition_attributes[0], XML_LITERAL("name"), XML_LITERAL("arinc661_definition"));
    Utl_Xml_AddAttribute(&definition, &definition_attributes[0]);
    Utl_Xml_InitializeAttribute(&definition_attributes[1], XML_LITERAL("library_version"), XML_LITERAL("0"));
    Utl_Xml_AddAttribute(&definition, &definition_attributes[1]);
    Utl_Xml_InitializeAttribute(&definition_attributes[2], XML_LITERAL("supp_version"), XML_LITERAL("6"));
    Utl_Xml_AddAttribute(&definition, &definition_attributes[2]);
    Utl_Xml_InitializeAttribute(&layer_attribute, XML_LITERAL("name"), XML_LITERAL("PrimaryLayer"));
    Utl_Xml_AddAttribute(&layer, &layer_attribute);
    Utl_Xml_InitializeAttribute(&panel_attributes[0], XML_LITERAL("name"), XML_LITERAL("MainPanel"));
    Utl_Xml_AddAttribute(&panel, &panel_attributes[0]);
    Utl_Xml_InitializeAttribute(&panel_attributes[1], XML_LITERAL("type"), XML_LITERAL("A661_PANEL"));
    Utl_Xml_AddAttribute(&panel, &panel_attributes[1]);
    Utl_Xml_InitializeAttribute(&label_attributes[0], XML_LITERAL("name"), XML_LITERAL("StatusLabel"));
    Utl_Xml_AddAttribute(&label, &label_attributes[0]);
    Utl_Xml_InitializeAttribute(&label_attributes[1], XML_LITERAL("type"), XML_LITERAL("A661_LABEL"));
    Utl_Xml_AddAttribute(&label, &label_attributes[1]);
    Utl_Xml_InitializeAttribute(&second_label_attributes[0], XML_LITERAL("name"), XML_LITERAL("SecondaryLabel"));
    Utl_Xml_AddAttribute(&second_label, &second_label_attributes[0]);
    Utl_Xml_InitializeAttribute(&second_label_attributes[1], XML_LITERAL("type"), XML_LITERAL("A661_LABEL"));
    Utl_Xml_AddAttribute(&second_label, &second_label_attributes[1]);

    for (index = 0; index < (uint32_t)COUNT_OF(c_arinc661Properties); ++index)
    {
        Utl_Xml_InitializeNode(&property_nodes[index], UTL_XML_NODE_ELEMENT, XML_LITERAL("prop"), NULL, 0);
        Utl_Xml_InitializeAttribute(&property_names[index], XML_LITERAL("name"), c_arinc661Properties[index].name, c_arinc661Properties[index].nameByteCount);
        Utl_Xml_AddAttribute(&property_nodes[index], &property_names[index]);
        Utl_Xml_InitializeAttribute(&property_values[index], XML_LITERAL("value"), c_arinc661Properties[index].value, c_arinc661Properties[index].valueByteCount);
        Utl_Xml_AddAttribute(&property_nodes[index], &property_values[index]);
        Utl_Xml_AppendChild(&models[c_arinc661Properties[index].modelIndex], &property_nodes[index]);
    }

    Utl_Xml_AppendChild(&definition, &models[ARINC661_DOCUMENT_MODEL]);
    Utl_Xml_AppendChild(&definition, &layer);
    Utl_Xml_AppendChild(&layer, &models[ARINC661_LAYER_MODEL]);
    Utl_Xml_AppendChild(&layer, &panel);
    Utl_Xml_AppendChild(&panel, &models[ARINC661_PANEL_MODEL]);
    Utl_Xml_AppendChild(&panel, &label);
    Utl_Xml_AppendChild(&label, &models[ARINC661_LABEL_MODEL]);
    Utl_Xml_AppendChild(&panel, &second_label);
    Utl_Xml_AppendChild(&second_label, &models[ARINC661_SECOND_LABEL_MODEL]);
    Utl_Xml_AppendNode(&document, &declaration);
    Utl_Xml_AppendNode(&document, &definition);

    byte_count = Utl_Xml_WriteDocument(&document, output, (uint32_t)sizeof(output));
    xml_file = fopen("examples/arinc661_definition.xml", "wb");
    LOGASSERT(NULL != xml_file);
    bytes_written = fwrite(output, 1, (uint32_t)byte_count, xml_file);
    LOGASSERT(byte_count == bytes_written);
    close_result = fclose(xml_file);
    LOGASSERT(0 == close_result);
    return 0;
}
