#ifndef ARINC661_CONFIGURATION_H
#define ARINC661_CONFIGURATION_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/* ARINC 661 configuration editor and visualizer. */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

extern "C"
{
#include "utl/utl_xml.h"
}

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

#define CONFIGURATION_MAX_DOCUMENT_BYTES       (262144)
#define CONFIGURATION_INPUT_BUFFER_BYTES        (CONFIGURATION_MAX_DOCUMENT_BYTES + 1)
#define CONFIGURATION_MAX_PARSED_ATTRIBUTES     (65536)
#define CONFIGURATION_MAX_ADDED_NODES           (4096)
#define CONFIGURATION_MAX_ADDED_ATTRIBUTES      (4096)
#define CONFIGURATION_MAX_EDITABLE_ATTRIBUTES   (4096)
#define CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES (4096)
#define CONFIGURATION_STATUS_BYTES              (256)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Fixed storage for one editable XML attribute value.
 */
typedef struct AttributeValue_s
{
    Utl_Xml_Attribute_t* attribute;                                          ///< Linked XML attribute, or NULL after removal.
    char_t               value[CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 1]; ///< Null-terminated editable value.
} AttributeValue_t;

/*************************************************************************************************/
/**
 * \brief Parsed configuration and fixed editor storage.
 */
typedef struct ConfigurationEditor_s
{
    byte_t              inputBuffer[CONFIGURATION_INPUT_BUFFER_BYTES];              ///< Mutable XML bytes borrowed by parsed nodes.
    byte_t              outputBuffer[CONFIGURATION_MAX_DOCUMENT_BYTES];             ///< Storage for the generated XML document.
    Utl_Xml_Node_t      nodes[UTL_XML_DOCUMENT_MAX_NODES];                           ///< Parsed node storage.
    Utl_Xml_Attribute_t attributes[CONFIGURATION_MAX_PARSED_ATTRIBUTES];            ///< Parsed attribute storage.
    Utl_Xml_Node_t      addedNodes[CONFIGURATION_MAX_ADDED_NODES];                   ///< Stable storage for newly added nodes.
    Utl_Xml_Attribute_t addedAttributes[CONFIGURATION_MAX_ADDED_ATTRIBUTES];        ///< Stable storage for newly added attributes.
    AttributeValue_t    attributeValues[CONFIGURATION_MAX_EDITABLE_ATTRIBUTES];     ///< Editable values for linked attributes.
    uint32_t            addedNodeCount;                                             ///< Number of added node slots used.
    uint32_t            addedAttributeCount;                                        ///< Number of added attribute slots used.
    uint32_t            attributeValueCount;                                        ///< Number of editable value slots used.
    Utl_Xml_Document_t  document;                                                   ///< Parsed and edited XML tree.
    Utl_Xml_Node_t*     selectedNode;                                               ///< Selected screen, layout, or window.
    char_t              status[CONFIGURATION_STATUS_BYTES];                         ///< Current load or build message.
    bool_t              isLoaded;                                                   ///< TRUE once the document has been read.
    bool_t              isLayoutInitialized;                                        ///< TRUE once the dock layout has been built.
} ConfigurationEditor_t;

/*=================================================================================================
** 3.3 Public function prototypes
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Loads the ARINC 661 configuration into fixed editor storage.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
void InitializeConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the ARINC 661 configuration editor and visualizer.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
void DrawConfiguration(ConfigurationEditor_t* editor);

#endif /* ARINC661_CONFIGURATION_H */
