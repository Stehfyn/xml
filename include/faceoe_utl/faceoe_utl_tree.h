#ifndef FACEOE_UTL_TREE_H
#define FACEOE_UTL_TREE_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "faceoe_utl/faceoe_utl_node.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Tree of nodes taken in order from caller-owned storage.
 *
 * \par Details:
 *  nodes[0] is the root. Nodes are linked with FACEOE_Utl_Node_AppendChild.
 */
typedef struct FACEOE_Utl_Tree_s
{
    FACEOE_Utl_Node_t* root;         ///< Root node, nodes[0]
    FACEOE_Utl_Node_t* nodes;        ///< Caller's node storage
    uint32_t           nodeCapacity; ///< Number of nodes in the storage
    uint32_t           nodeCount;    ///< Number of nodes taken, the root included
} FACEOE_Utl_Tree_t;

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
 * \brief Initializes a tree in caller-owned node storage, with a root holding an item.
 *
 * \par Requirements:
 *  - 001 This function shall initialize nodes[0] as the root holding rootItem, leaving nodeCount at 1.
 *  - 002 This function shall fail an assertion if tree or nodes is NULL, or nodeCapacity is zero.
 *
 * \param[out] tree           Tree to initialize
 * \param[out] nodes          Caller-owned node storage
 * \param[in]  nodeCapacity   Number of nodes in the storage
 * \param[in]  rootItem       Pointer to the root's item, or NULL
 *
 * \retval     None
 */
void FACEOE_Utl_Tree_Initialize(FACEOE_Utl_Tree_t* tree, FACEOE_Utl_Node_t* nodes, uint32_t nodeCapacity, const void* rootItem);

/*************************************************************************************************/
/**
 * \brief Takes the next node from the tree's storage, initialized and unlinked, holding an item.
 *
 * \par Requirements:
 *  - 001 This function shall take nodes in storage order; the first call after FACEOE_Utl_Tree_Initialize returns nodes[1].
 *  - 002 This function shall return NULL, taking nothing, when the storage is full.
 *  - 003 This function shall fail an assertion if tree is NULL.
 *
 * \param[in,out] tree   Tree to take the node from
 * \param[in]     item   Pointer to the node's item, or NULL
 *
 * \retval        FACEOE_Utl_Node_t*   The node, or NULL when the storage is full.
 */
FACEOE_Utl_Node_t* FACEOE_Utl_Tree_AddNode(FACEOE_Utl_Tree_t* tree, const void* item);

#endif /* FACEOE_UTL_TREE_H */
