/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * See arinc661_scene.h.
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661_scene.h"
#include "os/os_assert.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define SCENE_NODES_PER_OWNER (2UL) ///< A layer or widget node and its model node

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*=================================================================================================
** 3.3 External global variables
**===============================================================================================*/

/*=================================================================================================
** 3.4 Static global variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Static function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Adds a layer or widget node and its model node, linked as its first child.
 *
 * \param[in,out] scene   Scene with room for both nodes
 * \param[in]     item    Layer or widget
 *
 * \retval        FACEOE_Utl_Node_t*   The layer or widget node.
 */
static FACEOE_Utl_Node_t* AddOwner(Arinc661_Scene_t* scene, const void* item);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The model node comes first so that a node's properties are always its first child's
 *  children, as in a661Strict.xsd.
 */
static FACEOE_Utl_Node_t* AddOwner(Arinc661_Scene_t* scene, const void* item)
{
    FACEOE_Utl_Node_t* owner;
    FACEOE_Utl_Node_t* model;

    owner = FACEOE_Utl_Tree_AddNode(scene, item);
    model = FACEOE_Utl_Tree_AddNode(scene, NULL);
    FACEOE_Utl_Node_AppendChild(owner, model);

    return owner;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Nodes are taken in array order, so an owner's node is found by its array index without a
 *  search:
 *
 * \verbatim
 * nodes[0], nodes[1]                              the root and its model node
 * nodes[2 + (2 * i)], and the next                layers[i] and its model node
 * nodes[2 + (2 * L) + (2 * j)], and the next      widgets[j] and its model node
 * nodes[2 + (2 * L) + (2 * W) + k]                properties[k]
 * \endverbatim
 *
 *  A parent widget is an earlier entry, so it is already placed or left out when its child is
 *  reached; a widget is placed only under a parent in the scene, so leaving one widget out
 *  leaves out everything below it.
 */
bool_t Arinc661_Scene_Build(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, uint32_t nodeCapacity, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags)
{
    const Arinc661_Widget_t*   widget;
    const Arinc661_Property_t* property;
    FACEOE_Utl_Node_t*         layer_nodes;
    FACEOE_Utl_Node_t*         widget_nodes;
    FACEOE_Utl_Node_t*         property_node;
    FACEOE_Utl_Node_t*         parent;
    FACEOE_Utl_Node_t*         owner;
    uint32_t                   index;
    bool_t                     has_room;
    bool_t                     is_in_layer;
    bool_t                     is_layer_child;
    bool_t                     is_under_widget;
    bool_t                     is_parent_in_scene;
    bool_t                     is_omitted;

    LOGASSERT(NULL != scene);
    LOGASSERT((NULL != layers) || (0 == layerCount));
    LOGASSERT((NULL != widgets) || (0 == widgetCount));
    LOGASSERT((NULL != properties) || (0 == propertyCount));

    FACEOE_Utl_Tree_Initialize(scene, nodes, nodeCapacity, NULL);

    has_room = (ARINC661_SCENE_NODE_COUNT((uint64_t)layerCount, (uint64_t)widgetCount, (uint64_t)propertyCount) <= nodeCapacity);
    if (has_room)
    {
        FACEOE_Utl_Node_AppendChild(scene->root, FACEOE_Utl_Tree_AddNode(scene, NULL));

        layer_nodes = &scene->nodes[scene->nodeCount];
        for (index = 0; index < layerCount; ++index)
        {
            FACEOE_Utl_Node_AppendChild(scene->root, AddOwner(scene, &layers[index]));
        }

        widget_nodes = &scene->nodes[scene->nodeCount];
        for (index = 0; index < widgetCount; ++index)
        {
            widget = &widgets[index];
            owner  = AddOwner(scene, widget);

            is_in_layer        = (widget->layerIndex < layerCount);
            is_layer_child     = is_in_layer && (UINT32_MAX == widget->parentWidgetIndex);
            is_under_widget    = is_in_layer && (widget->parentWidgetIndex < index) && (widget->layerIndex == widgets[widget->parentWidgetIndex].layerIndex);
            is_parent_in_scene = is_under_widget && (NULL != widget_nodes[SCENE_NODES_PER_OWNER * widget->parentWidgetIndex].parent);
            is_omitted         = (NULL != omitFlags) && (0 != omitFlags[index]);

            parent = NULL;
            if (is_layer_child)
            {
                parent = &layer_nodes[SCENE_NODES_PER_OWNER * widget->layerIndex];
            }
            else if (is_parent_in_scene)
            {
                // Only a parent in the scene takes children, so an omitted parent omits them too
                parent = &widget_nodes[SCENE_NODES_PER_OWNER * widget->parentWidgetIndex];
            }
            else
            {
                // The widget stays out of the scene
            }

            if ((NULL != parent) && (FALSE == is_omitted))
            {
                FACEOE_Utl_Node_AppendChild(parent, owner);
            }
        }

        for (index = 0; index < propertyCount; ++index)
        {
            property      = &properties[index];
            property_node = FACEOE_Utl_Tree_AddNode(scene, property);

            owner = NULL;
            if (ARINC661_PROPERTY_DEFINITION == property->owner)
            {
                owner = scene->root;
            }
            else if ((ARINC661_PROPERTY_LAYER == property->owner) && (property->ownerIndex < layerCount))
            {
                owner = &layer_nodes[SCENE_NODES_PER_OWNER * property->ownerIndex];
            }
            else if ((ARINC661_PROPERTY_WIDGET == property->owner) && (property->ownerIndex < widgetCount))
            {
                owner = &widget_nodes[SCENE_NODES_PER_OWNER * property->ownerIndex];
            }
            else
            {
                // The property's owner is not in the arrays
            }

            if (NULL != owner)
            {
                FACEOE_Utl_Node_AppendChild(owner->firstChild, property_node);
            }
        }
    }

    return has_room;
}
