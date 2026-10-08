#ifndef ARINC661_SCENE_H
#define ARINC661_SCENE_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Part 1-7, section 3.2.3: a layer holds widgets, a container widget holds child
 * widgets, and section 8: an extension belongs to the widget it extends.
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd), whose
 * dfType, layerType, and widgetType each hold a model element of properties:
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "arinc661/arinc661.h"
#include "faceoe_utl/faceoe_utl_tree.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/** Nodes a scene of the given layer, widget, and property counts needs: the root and its model node, a node and a model node per layer and per widget, and a node per property. */
#define ARINC661_SCENE_NODE_COUNT(layerCount, widgetCount, propertyCount) (2UL + (2UL * (layerCount)) + (2UL * (widgetCount)) + (propertyCount))

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief The layer and widget hierarchy of one application, as a tree.
 *
 * \par Details:
 *  The root stands for the application. Each layer, widget, and extension is a node whose item points to
 *  its Arinc661_Layer_t or Arinc661_Widget_t, and whose first child is its model node. A model
 *  node has a NULL item, and its children are property nodes whose items point to the
 *  Arinc661_Property_t it owns, in array order:
 *
 * \verbatim
 * root                        item NULL
 *   model                     properties with owner ARINC661_PROPERTY_DEFINITION, such as ApplicationId
 *   layer                     item Arinc661_Layer_t
 *     model                   LayerId, ContextNumber, Width, Height
 *     widget                  item Arinc661_Widget_t, placed directly in the layer
 *       model                 WidgetIdent and the Creation Structure values
 *       widget                a widget nested in the one above
 *       widget                an extension of the one above; isExtension is set
 * \endverbatim
 */
typedef FACEOE_Utl_Tree_t Arinc661_Scene_t;

/*=================================================================================================
** 3.3 Constants
**===============================================================================================*/

/*=================================================================================================
** 3.4 Variables
**===============================================================================================*/

/*=================================================================================================
** 3.5 Functions
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Builds a scene from layers, widgets, and properties, in caller-owned nodes.
 *
 * \par Requirements:
 *  - 001 This function shall place layers, widgets, and properties in array order.
 *  - 002 This function shall leave out a widget whose layer, or whose parent widget, is not an earlier array entry in the same layer, and every widget below it.
 *  - 003 This function shall leave out a widget whose omit flag is non-zero, and every widget below it.
 *  - 004 This function shall leave out a property whose owner is not in the arrays.
 *  - 005 This function shall return FALSE, with only the root in the scene, if nodeCapacity is less than ARINC661_SCENE_NODE_COUNT of the counts.
 *  - 006 This function shall fail an assertion if scene or nodes is NULL, nodeCapacity is zero, or an array is NULL with a non-zero count.
 *
 * \par Details:
 *  The scene points into the arrays and the nodes; the caller keeps them unchanged while the
 *  scene is used, and builds the scene again after changing them.
 *
 * \param[out] scene           Scene to build
 * \param[out] nodes           Caller-owned node storage for the scene's tree
 * \param[in]  nodeCapacity    Number of nodes
 * \param[in]  layers          Layers
 * \param[in]  layerCount      Number of layers
 * \param[in]  widgets         Widgets and extensions
 * \param[in]  widgetCount     Number of widgets and extensions
 * \param[in]  properties      Properties of the application, the layers, and the widgets
 * \param[in]  propertyCount   Number of properties
 * \param[in]  omitFlags       Per-widget non-zero to leave the widget and everything below it out, or NULL
 *
 * \retval     TRUE            if the scene was built.
 * \retval     FALSE           if the nodes are too few.
 */
bool_t Arinc661_Scene_Build(Arinc661_Scene_t* scene, FACEOE_Utl_Node_t* nodes, uint32_t nodeCapacity, const Arinc661_Layer_t* layers, uint32_t layerCount, const Arinc661_Widget_t* widgets, uint32_t widgetCount, const Arinc661_Property_t* properties, uint32_t propertyCount, const byte_t* omitFlags);

#endif /* ARINC661_SCENE_H */
