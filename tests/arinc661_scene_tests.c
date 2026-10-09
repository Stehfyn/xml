/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_scene.h"
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
 * \brief Runs the ARINC 661 scene tests.
 *
 * \retval None
 */
void RunArinc661SceneTests(void);

#define TEST_LAYERS     (4)
#define TEST_WIDGETS    (8)
#define TEST_PROPERTIES (11)
#define TEST_NO_PARENT  (UINT32_MAX)
#define TEST_FIRST_ID   (3U) ///< LayerId of layers[1], first in LayerId order
#define TEST_SECOND_ID  (9U) ///< LayerId of layers[0], and claimed again by layers[2]

/*************************************************************************************************/
/**
 * \brief Clears a widget and sets its layer, parent, and extension flag.
 */
static void SetWidget(Arinc661_Widget_t* widget, uint32_t layerIndex, uint32_t parentWidgetIndex, bool_t isExtension)
{
    memset(widget, 0, sizeof((*widget)));
    widget->layerIndex        = layerIndex;
    widget->parentWidgetIndex = parentWidgetIndex;
    widget->isExtension       = isExtension;
}

/*************************************************************************************************/
/**
 * \brief Clears a property and sets its owner, name, and value.
 */
static void SetProperty(Arinc661_Property_t* property, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex, const char_t* name, const char_t* value)
{
    memset(property, 0, sizeof((*property)));
    property->owner      = owner;
    property->ownerIndex = ownerIndex;
    strcpy(property->name, name);
    strcpy(property->value, value);
}

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks Arinc661_Scene_ParseLayerId on valid and invalid text. Checks Arinc661_Scene_Build:
 *  too few nodes leave only the root; layers sit in LayerId order at the slots of their
 *  LayerIds, a duplicate or invalid LayerId leaves a layer out with its widgets and
 *  properties; the tree holds model nodes with their properties, a panel with a nested label
 *  and an extension, and a widget in the other layer; it leaves out a widget whose parent
 *  comes later, an omitted subtree, and a property whose owner is not in the arrays; without
 *  omit flags the omitted subtree returns. Checks that Arinc661_Scene_DisableLayer and
 *  Arinc661_Scene_EnableLayer change only isActive.
 */
void RunArinc661SceneTests(void)
{
    static FACEOE_Utl_Node_t     nodes[ARINC661_SCENE_NODE_COUNT(TEST_WIDGETS, TEST_PROPERTIES)];
    static Arinc661_Scene_Node_t items[COUNT_OF(nodes)];
    Arinc661_Layer_t             layers[TEST_LAYERS];
    Arinc661_Widget_t            widgets[TEST_WIDGETS];
    Arinc661_Property_t          properties[TEST_PROPERTIES];
    byte_t                       omit_flags[TEST_WIDGETS];
    Arinc661_Scene_t             scene;
    const FACEOE_Utl_Node_t*     layer;
    const FACEOE_Utl_Node_t*     panel;
    const FACEOE_Utl_Node_t*     child;
    uint32_t                     layer_id;

    // Whole decimal text from 0 to 255; layerId is untouched on failure
    assert(Arinc661_Scene_ParseLayerId("0", &layer_id) && (0 == layer_id));
    assert(Arinc661_Scene_ParseLayerId("255", &layer_id) && (255 == layer_id));
    assert(Arinc661_Scene_ParseLayerId("+7", &layer_id) && (7 == layer_id));
    assert(FALSE == Arinc661_Scene_ParseLayerId("256", &layer_id));
    assert(FALSE == Arinc661_Scene_ParseLayerId("-1", &layer_id));
    assert(FALSE == Arinc661_Scene_ParseLayerId("7x", &layer_id));
    assert(FALSE == Arinc661_Scene_ParseLayerId(" 7", &layer_id));
    assert(FALSE == Arinc661_Scene_ParseLayerId("", &layer_id));
    assert(7 == layer_id);

    memset(layers, 0, sizeof(layers));
    memset(omit_flags, 0, sizeof(omit_flags));
    SetWidget(&widgets[0], 0, TEST_NO_PARENT, FALSE); // panel in layers[0]
    SetWidget(&widgets[1], 0, 0, FALSE);              // label in the panel
    SetWidget(&widgets[2], 0, 0, TRUE);               // extension of the panel
    SetWidget(&widgets[3], 1, TEST_NO_PARENT, FALSE); // widget in layers[1]
    SetWidget(&widgets[4], 0, 6, FALSE);              // parent comes later: left out
    SetWidget(&widgets[5], 0, 0, FALSE);              // omitted
    SetWidget(&widgets[6], 0, 5, FALSE);              // below the omitted widget: left out
    SetWidget(&widgets[7], 2, TEST_NO_PARENT, FALSE); // in layers[2], which is left out
    omit_flags[5] = 1;
    SetProperty(&properties[0], ARINC661_PROPERTY_DEFINITION, 0, "ApplicationId", "1");
    SetProperty(&properties[1], ARINC661_PROPERTY_LAYER, 0, "LayerId", "9");
    SetProperty(&properties[2], ARINC661_PROPERTY_WIDGET, 0, "WidgetIdent", "1");
    SetProperty(&properties[3], ARINC661_PROPERTY_WIDGET, 0, "Visible", "A661_TRUE");
    SetProperty(&properties[4], ARINC661_PROPERTY_WIDGET, 1, "WidgetIdent", "2");
    SetProperty(&properties[5], ARINC661_PROPERTY_WIDGET, TEST_WIDGETS, "WidgetIdent", "3"); // owner not in the arrays
    SetProperty(&properties[6], ARINC661_PROPERTY_LAYER, 1, "LayerId", "3");
    SetProperty(&properties[7], ARINC661_PROPERTY_LAYER, 2, "LayerId", "9");   // an earlier layer has it: layers[2] is left out
    SetProperty(&properties[8], ARINC661_PROPERTY_LAYER, 3, "LayerId", "256"); // invalid: layers[3] is left out
    SetProperty(&properties[9], ARINC661_PROPERTY_LAYER, 0, "LayerId", "4");   // not the first: only a property of layers[0]
    SetProperty(&properties[10], ARINC661_PROPERTY_LAYER, 2, "Width", "1");    // owner left out

    // Too few nodes leave only the root
    assert(FALSE == Arinc661_Scene_Build(&scene, nodes, items, COUNT_OF(nodes) - 1, layers, TEST_LAYERS, widgets, TEST_WIDGETS, properties, TEST_PROPERTIES, omit_flags));
    assert(NULL == scene.root->firstChild);
    assert(ARINC661_SCENE_NODE_DEFINITION == ((const Arinc661_Scene_Node_t*)scene.root->item)->kind);
    assert(NULL == Arinc661_Scene_GetLayer(&scene, TEST_FIRST_ID));

    assert(Arinc661_Scene_Build(&scene, nodes, items, COUNT_OF(nodes), layers, TEST_LAYERS, widgets, TEST_WIDGETS, properties, TEST_PROPERTIES, omit_flags));
    assert(COUNT_OF(nodes) == scene.nodeCount);

    // The root's model node holds the definition's property, then come the layers in LayerId order
    assert(ARINC661_SCENE_NODE_DEFINITION == ((const Arinc661_Scene_Node_t*)scene.root->item)->kind);
    assert(ARINC661_SCENE_NODE_MODEL == ((const Arinc661_Scene_Node_t*)scene.root->firstChild->item)->kind);
    assert(ARINC661_SCENE_NODE_PROPERTY == ((const Arinc661_Scene_Node_t*)scene.root->firstChild->firstChild->item)->kind);
    assert(&properties[0] == ((const Arinc661_Scene_Node_t*)scene.root->firstChild->firstChild->item)->property);
    assert(NULL == scene.root->firstChild->firstChild->nextSibling);
    layer = scene.root->firstChild->nextSibling;
    assert(Arinc661_Scene_GetLayer(&scene, TEST_FIRST_ID) == layer);
    assert(Arinc661_Scene_GetLayer(&scene, TEST_SECOND_ID) == layer->nextSibling);
    assert(NULL == layer->nextSibling->nextSibling);
    assert((ARINC661_SCENE_NODE_LAYER == ((const Arinc661_Scene_Node_t*)layer->item)->kind) && (&layers[1] == ((const Arinc661_Scene_Node_t*)layer->item)->layer) && ((const Arinc661_Scene_Node_t*)layer->item)->isActive);
    assert(&layers[0] == ((const Arinc661_Scene_Node_t*)layer->nextSibling->item)->layer);
    assert(((const Arinc661_Scene_Node_t*)layer->nextSibling->item)->isActive);

    // A layer is at the slot of its LayerId; LayerIds no layer holds find nothing
    assert(&nodes[2 + (2 * TEST_SECOND_ID)] == Arinc661_Scene_GetLayer(&scene, TEST_SECOND_ID));
    assert(NULL == Arinc661_Scene_GetLayer(&scene, 0));
    assert(NULL == Arinc661_Scene_GetLayer(&scene, 4));
    assert(NULL == Arinc661_Scene_GetLayer(&scene, ARINC661_SCENE_LAYER_IDS));

    // The layer with TEST_SECOND_ID: both its LayerId properties, then only the panel
    layer = Arinc661_Scene_GetLayer(&scene, TEST_SECOND_ID);
    assert(&properties[1] == ((const Arinc661_Scene_Node_t*)layer->firstChild->firstChild->item)->property);
    assert(&properties[9] == ((const Arinc661_Scene_Node_t*)layer->firstChild->firstChild->nextSibling->item)->property);
    assert(NULL == layer->firstChild->firstChild->nextSibling->nextSibling);
    panel = layer->firstChild->nextSibling;
    assert((ARINC661_SCENE_NODE_WIDGET == ((const Arinc661_Scene_Node_t*)panel->item)->kind) && (&widgets[0] == ((const Arinc661_Scene_Node_t*)panel->item)->widget));
    assert(FALSE == ((const Arinc661_Scene_Node_t*)panel->item)->isActive);
    assert(NULL == panel->nextSibling);

    // The panel: its two properties in order, then the label and the extension; the omitted widget is gone
    assert(&properties[2] == ((const Arinc661_Scene_Node_t*)panel->firstChild->firstChild->item)->property);
    assert(&properties[3] == ((const Arinc661_Scene_Node_t*)panel->firstChild->firstChild->nextSibling->item)->property);
    child = panel->firstChild->nextSibling;
    assert(&widgets[1] == ((const Arinc661_Scene_Node_t*)child->item)->widget);
    assert(&properties[4] == ((const Arinc661_Scene_Node_t*)child->firstChild->firstChild->item)->property);
    assert(&widgets[2] == ((const Arinc661_Scene_Node_t*)child->nextSibling->item)->widget);
    assert(NULL == child->nextSibling->nextSibling);

    // The layer with TEST_FIRST_ID holds its own widget
    assert(&widgets[3] == ((const Arinc661_Scene_Node_t*)Arinc661_Scene_GetLayer(&scene, TEST_FIRST_ID)->firstChild->nextSibling->item)->widget);

    // Disabling and enabling change only isActive; the layer keeps its place and everything below it
    Arinc661_Scene_DisableLayer(&scene, TEST_SECOND_ID);
    assert(FALSE == ((const Arinc661_Scene_Node_t*)layer->item)->isActive);
    assert(((const Arinc661_Scene_Node_t*)Arinc661_Scene_GetLayer(&scene, TEST_FIRST_ID)->item)->isActive);
    assert((Arinc661_Scene_GetLayer(&scene, TEST_SECOND_ID) == layer) && (scene.root == layer->parent) && (panel == layer->firstChild->nextSibling));
    Arinc661_Scene_DisableLayer(&scene, TEST_SECOND_ID);
    assert(FALSE == ((const Arinc661_Scene_Node_t*)layer->item)->isActive);
    Arinc661_Scene_EnableLayer(&scene, TEST_SECOND_ID);
    assert(((const Arinc661_Scene_Node_t*)layer->item)->isActive);

    // Without omit flags the omitted widget and its child come back
    assert(Arinc661_Scene_Build(&scene, nodes, items, COUNT_OF(nodes), layers, TEST_LAYERS, widgets, TEST_WIDGETS, properties, TEST_PROPERTIES, NULL));
    child = Arinc661_Scene_GetLayer(&scene, TEST_SECOND_ID)->firstChild->nextSibling->firstChild->nextSibling->nextSibling->nextSibling;
    assert(&widgets[5] == ((const Arinc661_Scene_Node_t*)child->item)->widget);
    assert(&widgets[6] == ((const Arinc661_Scene_Node_t*)child->firstChild->nextSibling->item)->widget);
}
