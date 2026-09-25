#ifndef ARINC661_CONFIGURATION_H
#define ARINC661_CONFIGURATION_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

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
#define CONFIGURATION_MAX_PREVIEW_NODES          (UTL_XML_DOCUMENT_MAX_NODES + CONFIGURATION_MAX_ADDED_NODES)
#define CONFIGURATION_STATUS_BYTES              (256)

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Control type selected for an XML attribute.
 */
typedef enum ConfigurationAttributeKind_e
{
    CONFIGURATION_ATTRIBUTE_TEXT,
    CONFIGURATION_ATTRIBUTE_INTEGER,
    CONFIGURATION_ATTRIBUTE_FLOAT,
    CONFIGURATION_ATTRIBUTE_UNIT
} ConfigurationAttributeKind_t;

/*************************************************************************************************/
/**
 * \brief Editable text and its numeric control value.
 */
typedef struct ConfigurationAttributeValue_s
{
    char_t                       text[CONFIGURATION_MAX_ATTRIBUTE_VALUE_BYTES + 1]; ///< XML value; first member for attribute borrowing.
    ConfigurationAttributeKind_t kind;                                               ///< Inspector control type.
    uint32_t                     integerValue;                                       ///< Cached integer control value.
    float32_t                    floatValue;                                         ///< Cached floating-point control value.
    bool_t                       isValid;                                            ///< TRUE when the numeric text is valid.
} ConfigurationAttributeValue_t;

/*************************************************************************************************/
/**
 * \brief Screen geometry prepared for the visualizer.
 */
typedef struct ConfigurationPreviewScreen_s
{
    const char_t* id;                ///< Borrowed screen ID.
    float32_t     width;             ///< Screen width in pixels.
    float32_t     height;            ///< Screen height in pixels.
    uint32_t      firstWindowIndex;  ///< First window in previewWindows.
    uint32_t      windowCount;       ///< Number of windows on this screen.
} ConfigurationPreviewScreen_t;

/*************************************************************************************************/
/**
 * \brief Window geometry prepared for the visualizer.
 */
typedef struct ConfigurationPreviewWindow_s
{
    const char_t* name;   ///< Borrowed window name.
    float32_t     x;      ///< Left edge in screen pixels.
    float32_t     y;      ///< Top edge in screen pixels.
    float32_t     width;  ///< Window width in pixels.
    float32_t     height; ///< Window height in pixels.
} ConfigurationPreviewWindow_t;

/*************************************************************************************************/
/**
 * \brief Parsed configuration and fixed editor storage.
 */
typedef struct ConfigurationEditor_s
{
    byte_t                        inputBuffer[CONFIGURATION_INPUT_BUFFER_BYTES];           ///< Input XML and terminator.
    byte_t                        outputBuffer[CONFIGURATION_MAX_DOCUMENT_BYTES];          ///< Written XML.
    Utl_Xml_Node_t                nodes[UTL_XML_DOCUMENT_MAX_NODES];                        ///< Parsed nodes.
    Utl_Xml_Attribute_t           attributes[CONFIGURATION_MAX_PARSED_ATTRIBUTES];         ///< Parsed attributes.
    Utl_Xml_Node_t                addedNodes[CONFIGURATION_MAX_ADDED_NODES];                ///< Added nodes.
    Utl_Xml_Attribute_t           addedAttributes[CONFIGURATION_MAX_ADDED_ATTRIBUTES];     ///< Added attributes.
    ConfigurationAttributeValue_t attributeValues[CONFIGURATION_MAX_EDITABLE_ATTRIBUTES];  ///< Editable XML values.
    ConfigurationPreviewScreen_t  previewScreens[CONFIGURATION_MAX_PREVIEW_NODES];           ///< Prepared screen geometry.
    ConfigurationPreviewWindow_t  previewWindows[CONFIGURATION_MAX_PREVIEW_NODES];           ///< Prepared window geometry.
    uint32_t                      addedNodeCount;                                           ///< Added nodes in use.
    uint32_t                      addedAttributeCount;                                      ///< Added attributes in use.
    uint32_t                      attributeValueCount;                                      ///< Editable values in use.
    uint32_t                      previewScreenCount;                                       ///< Screens with valid geometry.
    uint32_t                      previewWindowCount;                                       ///< Windows with valid geometry.
    float32_t                     previewTotalWidth;                                        ///< Sum of screen widths.
    float32_t                     previewMaximumHeight;                                     ///< Largest screen height.
    Utl_Xml_Document_t            document;                                                 ///< Edited XML tree.
    Utl_Xml_Node_t*               selectedNode;                                             ///< Selected hierarchy node.
    char_t                        status[CONFIGURATION_STATUS_BYTES];                       ///< Current message.
    bool_t                        isLoaded;                                                 ///< TRUE after reading XML.
    bool_t                        isLayoutInitialized;                                      ///< TRUE after docking.
    bool_t                        isPreviewDirty;                                           ///< TRUE after a geometry edit.
} ConfigurationEditor_t;

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
 * \brief Loads the ARINC 661 configuration into fixed editor storage.
 *
 * \par Details:
 *  Reads the XML document and prepares the first dock layout.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
void InitializeConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Refreshes visualizer geometry after an edit.
 *
 * \par Details:
 *  Reads XML values only when the configuration has changed.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
void UpdateConfiguration(ConfigurationEditor_t* editor);

/*************************************************************************************************/
/**
 * \brief Draws the ARINC 661 configuration editor and visualizer.
 *
 * \par Details:
 *  Draws both docked windows from the current editor state.
 *
 * \param[in,out] editor   Editor state
 *
 * \retval None
 */
void DrawConfiguration(ConfigurationEditor_t* editor);

#endif /* ARINC661_CONFIGURATION_H */
