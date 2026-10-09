/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "faceoe_utl/faceoe_utl_tree.h"
#include "os/os_assert.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

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
 * \brief Returns the node after a node in a depth-first walk from the root.
 *
 * \param[in] tree   Tree being walked
 * \param[in] node   Node just visited
 *
 * \retval    const FACEOE_Utl_Node_t*   Next node, or NULL after the last one.
 */
static const FACEOE_Utl_Node_t* NextNode(const FACEOE_Utl_Tree_t* tree, const FACEOE_Utl_Node_t* node);

/*=================================================================================================
** 4. PRIVATE FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The first child comes next; without one, the first next sibling found climbing toward the
 *  root. The climb is bounded by nodeCount, the deepest a linked node can sit.
 */
static const FACEOE_Utl_Node_t* NextNode(const FACEOE_Utl_Tree_t* tree, const FACEOE_Utl_Node_t* node)
{
    const FACEOE_Utl_Node_t* current;
    const FACEOE_Utl_Node_t* next;
    uint32_t                 depth;
    bool_t                   has_next;
    bool_t                   is_root;
    bool_t                   has_sibling;

    current = node;
    next    = current->firstChild;
    for (depth = 0; depth < tree->nodeCount; ++depth)
    {
        has_next = (NULL != next);
        is_root  = (tree->root == current);
        if (has_next || is_root)
        {
            break;
        }

        has_sibling = (NULL != current->nextSibling);
        if (has_sibling)
        {
            next = current->nextSibling;
        }
        else
        {
            current = current->parent;
        }
    }

    return next;
}

/*=================================================================================================
** 5. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  The root is taken with FACEOE_Utl_Tree_AddNode, leaving nodeCapacity - 1 nodes for later calls.
 */
void FACEOE_Utl_Tree_Initialize(FACEOE_Utl_Tree_t* tree, FACEOE_Utl_Node_t* nodes, uint32_t nodeCapacity, void* rootItem)
{
    LOGASSERT(NULL != tree);
    LOGASSERT(NULL != nodes);
    LOGASSERT(0 < nodeCapacity);

    (void)memset(tree, 0, sizeof((*tree)));
    tree->nodes        = nodes;
    tree->nodeCapacity = nodeCapacity;
    tree->nodeCount    = 0;
    tree->root         = FACEOE_Utl_Tree_AddNode(tree, rootItem);
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Nodes are never given back; initializing the tree again reuses the storage.
 */
FACEOE_Utl_Node_t* FACEOE_Utl_Tree_AddNode(FACEOE_Utl_Tree_t* tree, void* item)
{
    FACEOE_Utl_Node_t* node;
    bool_t             has_room;

    LOGASSERT(NULL != tree);

    has_room = (tree->nodeCount < tree->nodeCapacity);

    if (has_room)
    {
        node = &tree->nodes[tree->nodeCount];

        FACEOE_Utl_Node_Initialize(node, item);
        
        tree->nodeCount += 1;
    }
    else
    {
        node = NULL;
    }

    return node;
}

/*************************************************************************************************/
/**
 * \par Details:
 *  Walks the links without recursion, so a deep tree needs no extra stack. A linked tree holds
 *  at most nodeCount nodes, so a walk still going after that many visits has met a cycle.
 */
void FACEOE_Utl_Tree_Visit(const FACEOE_Utl_Tree_t* tree, FACEOE_Utl_Tree_Visitor_t visitor, void* context)
{
    const FACEOE_Utl_Node_t* node;
    uint32_t                 visited;

    LOGASSERT(NULL != tree);
    LOGASSERT(NULL != visitor);

    node = tree->root;

    for (visited = 0; visited < tree->nodeCount; ++visited)
    {
        if (NULL == node)
        {
            break;
        }

        visitor(node, context);

        node = NextNode(tree, node);
    }

    LOGASSERT(NULL == node);
}
