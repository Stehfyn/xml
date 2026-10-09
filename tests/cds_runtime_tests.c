/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_definition.h"
#include "arinc661/arinc661_runtime.h"
#include "cds/cds_runtime.h"
#include "faceoe_utl/faceoe_utl_defines.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Runs the CDS run-time tests.
 *
 * \retval None
 */
void RunCDSRunTimeTests(void);

#define TEST_LAYER_ID      (7U)  ///< LayerId of the test layer
#define TEST_WIDGET_IDENT  (2U)  ///< WidgetIdent of the test label
#define TEST_PROPERTIES    (7)   ///< Properties of the test definition
#define TEST_SPARE_NODES   (1)   ///< Nodes left over for properties added at run time: Font
#define TEST_VALUES        (6)   ///< Properties that can be set at run time: the six the first block sets
#define TEST_IDENTS        (8)   ///< WidgetIdents the widget table covers per layer
#define TEST_NODES         ((2 * (1 + 1 + 1)) + TEST_PROPERTIES + TEST_SPARE_NODES) ///< Root, one layer, one widget, the properties, and spares

/*************************************************************************************************/
/**
 * \brief Clears a property and sets its owner, name, and value.
 */
static void SetProperty(Arinc661_Property_t* property, Arinc661_PropertyOwner_t owner, const char_t* name, const char_t* value)
{
    memset(property, 0, sizeof((*property)));
    property->owner      = owner;
    property->ownerIndex = 0;
    strcpy(property->name, name);
    strcpy(property->value, value);
}

/*************************************************************************************************/
/**
 * \brief Returns the value of an owner's occurrence-th property of a name, or NULL.
 */
static const char_t* ValueOf(const FACEOE_Utl_Node_t* owner, const char_t* name, uint32_t occurrence)
{
    const FACEOE_Utl_Node_t*   entry;
    const Arinc661_Property_t* property;
    const char_t*              value;
    uint32_t                   seen;

    value = NULL;
    seen  = 0;
    for (entry = owner->firstChild->firstChild; NULL != entry; entry = entry->nextSibling)
    {
        property = (const Arinc661_Property_t*)((const Arinc661_Scene_Node_t*)entry->item)->data;
        if (0 == strncmp(name, property->name, sizeof(property->name)))
        {
            if (seen == occurrence)
            {
                value = property->value;
                break;
            }
            seen += 1;
        }
    }

    return value;
}

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the run-time parameter table, Arinc661_Definition_ReadBinaryValue, the scene's widget
 *  table and run-time values, and CDS_RunTime_ApplyBlock on blocks that set parameters, add a
 *  run-time-only one, change the layer's activity and visibility, and name an unknown widget.
 */
void RunCDSRunTimeTests(void)
{
    static const byte_t c_fr180Ninety[]  = {0x40, 0x00, 0x00, 0x00};
    static const byte_t c_shortMinusOne[] = {0xFF, 0xFF};
    static const byte_t c_longValue[]     = {0xFF, 0xFF, 0xFF, 0x38};
    static const byte_t c_unterminated[]  = {'H', 'I'};

    // Layer 7, ContextNumber 5: hide widget 2, move it to (100, -200), label it "HI", and set Font, which it did not have
    static const byte_t c_setBlock[] =
    {
        0xB0, TEST_LAYER_ID, 0x00, 0x05, 0x00, 0x00, 0x00, 0x30,
        0xCA, 0x02, 0x00, 0x24, 0x00, TEST_WIDGET_IDENT, 0x00, 0x00,
        0xB5, 0x30, 0x00, 0x00,
        0xB3, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0xFF, 0xFF, 0xFF, 0x38,
        0xB4, 0x90, 0x00, 0x03, 'H', 'I', 0x00, 0x00,
        0xB5, 0x90, 0x03, 0x00,
        0xD0, 0x00, 0x00, 0x00
    };
    // ContextNumber 6, then A661_REQ_LAYER_INACTIVE
    static const byte_t c_inactiveBlock[] =
    {
        0xB0, TEST_LAYER_ID, 0x00, 0x06, 0x00, 0x00, 0x00, 0x14,
        0xCA, 0x03, 0x00, 0x08, 0xDA, 0x02, 0x00, 0x00,
        0xD0, 0x00, 0x00, 0x00
    };
    // A661_VISIBLE = 1, passed over while the layer is inactive
    static const byte_t c_showWidgetBlock[] =
    {
        0xB0, TEST_LAYER_ID, 0x00, 0x07, 0x00, 0x00, 0x00, 0x18,
        0xCA, 0x02, 0x00, 0x0C, 0x00, TEST_WIDGET_IDENT, 0x00, 0x00,
        0xB5, 0x30, 0x01, 0x00,
        0xD0, 0x00, 0x00, 0x00
    };
    // A661_REQ_LAYER_ACTIVE, then A661_REQ_LAYER_VISIBLE
    static const byte_t c_activeBlock[] =
    {
        0xB0, TEST_LAYER_ID, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1C,
        0xCA, 0x03, 0x00, 0x08, 0xDA, 0x01, 0x00, 0x00,
        0xCA, 0x03, 0x00, 0x08, 0xDA, 0x04, 0x00, 0x00,
        0xD0, 0x00, 0x00, 0x00
    };
    // A661_VISIBLE for widget 3, which the layer does not have
    static const byte_t c_unknownWidgetBlock[] =
    {
        0xB0, TEST_LAYER_ID, 0x00, 0x09, 0x00, 0x00, 0x00, 0x18,
        0xCA, 0x02, 0x00, 0x0C, 0x00, 0x03, 0x00, 0x00,
        0xB5, 0x30, 0x01, 0x00,
        0xD0, 0x00, 0x00, 0x00
    };
    static FACEOE_Utl_Node_t     nodes[TEST_NODES];
    static Arinc661_Scene_Node_t items[TEST_NODES];
    static FACEOE_Utl_Node_t*    widget_nodes[TEST_IDENTS];
    static Arinc661_Property_t   values[TEST_VALUES];
    Arinc661_RunTimeParameter_t  parameter;
    Arinc661_Layer_t             layer;
    Arinc661_Widget_t            widget;
    Arinc661_Property_t          properties[TEST_PROPERTIES];
    Arinc661_Scene_t             scene;
    const FACEOE_Utl_Node_t*     layer_node;
    const FACEOE_Utl_Node_t*     label;
    char_t                       text[ARINC661_MAX_PROPERTY_BYTES];

    // The label's run-time table: Visible is one byte, PosX and PosY a pair of longs
    assert(Arinc661_RunTime_GetParameter("A661_LABEL", 0xB530, &parameter));
    assert((ARINC661_RUNTIME_1BYTE == parameter.structure) && (ARINC661_PARAMETER_UCHAR == parameter.kind) && (0 == strcmp("Visible", parameter.name)) && (NULL == parameter.secondName));
    assert(Arinc661_RunTime_GetParameter("A661_LABEL", 0xB320, &parameter));
    assert((ARINC661_RUNTIME_8BYTES == parameter.structure) && (0 == strcmp("PosX", parameter.name)) && (0 == strcmp("PosY", parameter.secondName)));
    assert(FALSE == Arinc661_RunTime_GetParameter("A661_LABEL", 0xFFFF, &parameter));
    assert(FALSE == Arinc661_RunTime_GetParameter("A661_NO_SUCH_TYPE", 0xB530, &parameter));

    // Binary values read as the text a binary Definition File gives them
    assert(Arinc661_Definition_ReadBinaryValue(ARINC661_PARAMETER_FR180, c_fr180Ninety, sizeof(c_fr180Ninety), text, sizeof(text)) && (0 == strcmp("90", text)));
    assert(Arinc661_Definition_ReadBinaryValue(ARINC661_PARAMETER_SHORT, c_shortMinusOne, sizeof(c_shortMinusOne), text, sizeof(text)) && (0 == strcmp("-1", text)));
    assert(Arinc661_Definition_ReadBinaryValue(ARINC661_PARAMETER_LONG, c_longValue, sizeof(c_longValue), text, sizeof(text)) && (0 == strcmp("-200", text)));
    assert(FALSE == Arinc661_Definition_ReadBinaryValue(ARINC661_PARAMETER_STRING, c_unterminated, sizeof(c_unterminated), text, sizeof(text)));
    assert(FALSE == Arinc661_Definition_ReadBinaryValue(ARINC661_PARAMETER_LONG, c_shortMinusOne, sizeof(c_shortMinusOne), text, sizeof(text)));

    // One layer holding one label
    memset(&layer, 0, sizeof(layer));
    memset(&widget, 0, sizeof(widget));
    strcpy(widget.type, "A661_LABEL");
    widget.layerIndex        = 0;
    widget.parentWidgetIndex = UINT32_MAX;
    SetProperty(&properties[0], ARINC661_PROPERTY_LAYER, "LayerId", "7");
    SetProperty(&properties[1], ARINC661_PROPERTY_LAYER, "ContextNumber", "0");
    SetProperty(&properties[2], ARINC661_PROPERTY_WIDGET, "WidgetIdent", "2");
    SetProperty(&properties[3], ARINC661_PROPERTY_WIDGET, "Visible", "1");
    SetProperty(&properties[4], ARINC661_PROPERTY_WIDGET, "PosX", "0");
    SetProperty(&properties[5], ARINC661_PROPERTY_WIDGET, "PosY", "0");
    SetProperty(&properties[6], ARINC661_PROPERTY_WIDGET, "LabelString", "A");
    Arinc661_Scene_Initialize(&scene, nodes, items, COUNT_OF(nodes), widget_nodes, COUNT_OF(widget_nodes), TEST_IDENTS, values, COUNT_OF(values));
    Arinc661_Scene_Build(&scene, &layer, 1, &widget, 1, properties, TEST_PROPERTIES, NULL);

    // The widget table finds the label by WidgetIdent; other WidgetIdents and LayerIds find nothing
    layer_node = Arinc661_Scene_GetLayer(&scene, TEST_LAYER_ID);
    label      = Arinc661_Scene_GetWidget(&scene, TEST_LAYER_ID, TEST_WIDGET_IDENT);
    assert((NULL != label) && (&widget == ((const Arinc661_Scene_Node_t*)label->item)->data));
    assert(NULL == Arinc661_Scene_GetWidget(&scene, TEST_LAYER_ID, 3));
    assert(NULL == Arinc661_Scene_GetWidget(&scene, TEST_LAYER_ID, TEST_IDENTS));
    assert(NULL == Arinc661_Scene_GetWidget(&scene, 0, TEST_WIDGET_IDENT));

    // A block sets the ContextNumber and the parameters, as copies; the definition is unchanged
    assert(CDS_RunTime_ApplyBlock(&scene, c_setBlock, sizeof(c_setBlock)));
    assert(0 == strcmp("5", ValueOf(layer_node, "ContextNumber", 0)));
    assert(0 == strcmp("0", ValueOf(label, "Visible", 0)));
    assert(0 == strcmp("100", ValueOf(label, "PosX", 0)));
    assert(0 == strcmp("-200", ValueOf(label, "PosY", 0)));
    assert(0 == strcmp("HI", ValueOf(label, "LabelString", 0)));
    assert(0 == strcmp("3", ValueOf(label, "Font", 0)));
    assert((0 == strcmp("1", properties[3].value)) && (0 == strcmp("A", properties[6].value)));
    assert(TEST_VALUES == scene.valueCount);
    assert(COUNT_OF(nodes) == scene.tree.nodeCount);

    // Setting a value again reuses its copy
    Arinc661_Scene_SetValue(&scene, label, "Visible", 0, "1");
    assert(0 == strcmp("1", ValueOf(label, "Visible", 0)));
    assert(TEST_VALUES == scene.valueCount);

    // A661_REQ_LAYER_INACTIVE makes the layer inactive and invisible
    assert(CDS_RunTime_ApplyBlock(&scene, c_inactiveBlock, sizeof(c_inactiveBlock)));
    assert(0 == strcmp("6", ValueOf(layer_node, "ContextNumber", 0)));
    assert((FALSE == ((const Arinc661_Scene_Node_t*)layer_node->item)->active) && (FALSE == ((const Arinc661_Scene_Node_t*)layer_node->item)->visible));

    // While inactive, set-parameter commands and the ContextNumber are passed over
    Arinc661_Scene_SetValue(&scene, label, "Visible", 0, "0");
    assert(CDS_RunTime_ApplyBlock(&scene, c_showWidgetBlock, sizeof(c_showWidgetBlock)));
    assert(0 == strcmp("0", ValueOf(label, "Visible", 0)));
    assert(0 == strcmp("6", ValueOf(layer_node, "ContextNumber", 0)));

    // A661_REQ_LAYER_ACTIVE, then A661_REQ_LAYER_VISIBLE
    assert(CDS_RunTime_ApplyBlock(&scene, c_activeBlock, sizeof(c_activeBlock)));
    assert(((const Arinc661_Scene_Node_t*)layer_node->item)->active && ((const Arinc661_Scene_Node_t*)layer_node->item)->visible);

    // A widget the layer does not have, and a block cut short, are not applied
    assert(FALSE == CDS_RunTime_ApplyBlock(&scene, c_unknownWidgetBlock, sizeof(c_unknownWidgetBlock)));
    assert(FALSE == CDS_RunTime_ApplyBlock(&scene, c_setBlock, sizeof(c_setBlock) - 1));

    // Building again drops every run-time value
    Arinc661_Scene_Build(&scene, &layer, 1, &widget, 1, properties, TEST_PROPERTIES, NULL);
    label = Arinc661_Scene_GetWidget(&scene, TEST_LAYER_ID, TEST_WIDGET_IDENT);
    assert((0 == scene.valueCount) && (0 == strcmp("1", ValueOf(label, "Visible", 0))) && (NULL == ValueOf(label, "Font", 0)));
}
