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

#define TEST_LAYERS     (2)
#define TEST_WIDGETS    (7)
#define TEST_PROPERTIES (6)
#define TEST_NO_PARENT  (UINT32_MAX)

/*************************************************************************************************/
/**
 * \brief Sets a widget's place in the hierarchy.
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
 * \brief Sets a property's owner.
 */
static void SetProperty(Arinc661_Property_t* property, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    memset(property, 0, sizeof((*property)));
    property->owner      = owner;
    property->ownerIndex = ownerIndex;
}

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Builds two layers holding a widget with a nested widget, an extension, and an omitted
 *  subtree, plus a widget whose parent comes later, and checks every link the renderer walks.
 */
void RunArinc661SceneTests(void)
{
    static FACEOE_Utl_Node_t nodes[ARINC661_SCENE_NODE_COUNT(TEST_LAYERS, TEST_WIDGETS, TEST_PROPERTIES)];
    Arinc661_Layer_t         layers[TEST_LAYERS];
    Arinc661_Widget_t        widgets[TEST_WIDGETS];
    Arinc661_Property_t      properties[TEST_PROPERTIES];
    byte_t                   omit_flags[TEST_WIDGETS];
    Arinc661_Scene_t         scene;
    const FACEOE_Utl_Node_t* layer;
    const FACEOE_Utl_Node_t* panel;
    const FACEOE_Utl_Node_t* child;

    memset(layers, 0, sizeof(layers));
    memset(omit_flags, 0, sizeof(omit_flags));
    SetWidget(&widgets[0], 0, TEST_NO_PARENT, FALSE); // panel in layer 0
    SetWidget(&widgets[1], 0, 0, FALSE);              // label in the panel
    SetWidget(&widgets[2], 0, 0, TRUE);               // extension of the panel
    SetWidget(&widgets[3], 1, TEST_NO_PARENT, FALSE); // widget in layer 1
    SetWidget(&widgets[4], 0, 6, FALSE);              // parent comes later: left out
    SetWidget(&widgets[5], 0, 0, FALSE);              // omitted
    SetWidget(&widgets[6], 0, 5, FALSE);              // below the omitted widget: left out
    omit_flags[5] = 1;
    SetProperty(&properties[0], ARINC661_PROPERTY_DEFINITION, 0);
    SetProperty(&properties[1], ARINC661_PROPERTY_LAYER, 0);
    SetProperty(&properties[2], ARINC661_PROPERTY_WIDGET, 0);
    SetProperty(&properties[3], ARINC661_PROPERTY_WIDGET, 0);
    SetProperty(&properties[4], ARINC661_PROPERTY_WIDGET, 1);
    SetProperty(&properties[5], ARINC661_PROPERTY_WIDGET, TEST_WIDGETS); // owner not in the arrays

    // Too few nodes leaves only the root
    assert(FALSE == Arinc661_Scene_Build(&scene, nodes, COUNT_OF(nodes) - 1, layers, TEST_LAYERS, widgets, TEST_WIDGETS, properties, TEST_PROPERTIES, omit_flags));
    assert(NULL == scene.root->firstChild);

    assert(Arinc661_Scene_Build(&scene, nodes, COUNT_OF(nodes), layers, TEST_LAYERS, widgets, TEST_WIDGETS, properties, TEST_PROPERTIES, omit_flags));
    assert(COUNT_OF(nodes) == scene.nodeCount);

    // The root's model node holds the application's property, then come the layers
    assert(NULL == scene.root->item);
    assert(NULL == scene.root->firstChild->item);
    assert(&properties[0] == scene.root->firstChild->firstChild->item);
    assert(NULL == scene.root->firstChild->firstChild->nextSibling);
    layer = scene.root->firstChild->nextSibling;
    assert(&layers[0] == layer->item);
    assert(&layers[1] == layer->nextSibling->item);
    assert(NULL == layer->nextSibling->nextSibling);

    // Layer 0: its model node, then only the panel
    assert(&properties[1] == layer->firstChild->firstChild->item);
    panel = layer->firstChild->nextSibling;
    assert(&widgets[0] == panel->item);
    assert(NULL == panel->nextSibling);

    // The panel: its two properties in order, then the label and the extension; the omitted widget is gone
    assert(&properties[2] == panel->firstChild->firstChild->item);
    assert(&properties[3] == panel->firstChild->firstChild->nextSibling->item);
    child = panel->firstChild->nextSibling;
    assert(&widgets[1] == child->item);
    assert(&properties[4] == child->firstChild->firstChild->item);
    assert(&widgets[2] == child->nextSibling->item);
    assert(NULL == child->nextSibling->nextSibling);

    // Layer 1 holds its own widget
    assert(&widgets[3] == layer->nextSibling->firstChild->nextSibling->item);

    // Without omit flags the omitted widget and its child come back
    assert(Arinc661_Scene_Build(&scene, nodes, COUNT_OF(nodes), layers, TEST_LAYERS, widgets, TEST_WIDGETS, properties, TEST_PROPERTIES, NULL));
    child = scene.root->firstChild->nextSibling->firstChild->nextSibling->firstChild->nextSibling->nextSibling->nextSibling;
    assert(&widgets[5] == child->item);
    assert(&widgets[6] == child->firstChild->nextSibling->item);
}
