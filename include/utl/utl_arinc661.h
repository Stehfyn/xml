#ifndef UTL_ARINC661_H
#define UTL_ARINC661_H

/*=================================================================================================
** 1. References
**===============================================================================================*/

/**
 * ARINC 661 Supplement 6, Cockpit Display System Interfaces to User Systems:
 * https://saemobilus.sae.org/standards/arinc661-6-cockpit-display-system-interfaces-user-systems
 * J661-hosted Supplement 6 strict XML Definition File schema (a661Strict.xsd):
 * https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 * FACE Technical Standard, Edition 3.1, Appendix H.3, DisplayManagement.xsd:
 * https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 */

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

#include "os/os_types.h"

/*=================================================================================================
** 3. DECLARATIONS
**
** 3.1 Macros
**===============================================================================================*/

/* Caller-owned storage limits; the XML schemas do not prescribe these byte capacities. */
#define UTL_ARINC661_MAX_NAME_BYTES        (128)
#define UTL_ARINC661_MAX_DESCRIPTION_BYTES (256)
#define UTL_ARINC661_MAX_PROPERTY_BYTES    (256)
#define UTL_ARINC661_MAX_PATH_BYTES        (512)
#define UTL_ARINC661_WORKSPACE_BYTES       (16777216)

/* FACE Edition 3.1, Appendix H.3, DisplayManagement.xsd, Units simpleType. */
#define UTL_ARINC661_XML_UNIT_INCH   ("inch")
#define UTL_ARINC661_XML_UNIT_MM     ("mm")
#define UTL_ARINC661_XML_UNIT_SCREEN ("screen")
#define UTL_ARINC661_XML_UNIT_PIXEL  ("pixel")

/*=================================================================================================
** 3.2 Types
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Unit values in FACE Edition 3.1, Appendix H.3, Units simpleType.
 */
typedef enum Utl_Arinc661_Unit_e
{
    UTL_ARINC661_UNIT_INCH,
    UTL_ARINC661_UNIT_MM,
    UTL_ARINC661_UNIT_SCREEN,
    UTL_ARINC661_UNIT_PIXEL
} Utl_Arinc661_Unit_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Size values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Size complexType; sections H.3.4 and H.3.5.
 */
typedef struct Utl_Arinc661_Size_s
{
    int64_t             width;  ///< Width in units.
    int64_t             height; ///< Height in units.
    Utl_Arinc661_Unit_t units;  ///< Unit of both dimensions.
} Utl_Arinc661_Size_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Rectangle values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Rectangle complexType, used by Window.pixelArea.
 */
typedef struct Utl_Arinc661_Rectangle_s
{
    int64_t x;      ///< Horizontal origin.
    int64_t y;      ///< Vertical origin.
    int64_t width;  ///< Width.
    int64_t height; ///< Height.
} Utl_Arinc661_Rectangle_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Scale values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Scale complexType, used by Window.scalingFactor.
 */
typedef struct Utl_Arinc661_Scale_s
{
    float32_t           xScale;   ///< Horizontal scaling factor.
    float32_t           yScale;   ///< Vertical scaling factor.
    Utl_Arinc661_Unit_t baseUnit; ///< Units before scaling.
    Utl_Arinc661_Unit_t perUnit;  ///< Units after scaling.
} Utl_Arinc661_Scale_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Screen values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Screen complexType and sections H.3.3 through H.3.5.
 */
typedef struct Utl_Arinc661_Screen_s
{
    int64_t             id;                 ///< Screen identifier.
    Utl_Arinc661_Size_t pixelSize;          ///< Pixel dimensions.
    Utl_Arinc661_Size_t physicalDimensions; ///< Physical dimensions.
} Utl_Arinc661_Screen_t;

/*************************************************************************************************/
/**
 * \brief FACE display management layout values and their owning screen.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Screen.layout element and section H.3.6.
 */
typedef struct Utl_Arinc661_Layout_s
{
    int64_t  id;          ///< Layout identifier.
    uint32_t screenIndex; ///< Index of the owning screen.
} Utl_Arinc661_Layout_t;

/*************************************************************************************************/
/**
 * \brief FACE display management Window values and their owning layout.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, Window complexType and section H.3.2.
 */
typedef struct Utl_Arinc661_Window_s
{
    int64_t                  id;                                              ///< Window identifier.
    char_t                   name[UTL_ARINC661_MAX_NAME_BYTES];               ///< Optional window name.
    char_t                   description[UTL_ARINC661_MAX_DESCRIPTION_BYTES]; ///< Window description.
    Utl_Arinc661_Rectangle_t pixelArea;                                       ///< Window pixel rectangle.
    Utl_Arinc661_Scale_t     scalingFactor;                                   ///< Window scaling factor.
    uint32_t                 layoutIndex;                                     ///< Index of the owning layout.
} Utl_Arinc661_Window_t;

/*************************************************************************************************/
/**
 * \brief FACE display management UserApplication values.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, UserApplication complexType and section H.3.1.
 */
typedef struct Utl_Arinc661_UserApplication_s
{
    int64_t applicationId;                              ///< ARINC application identifier.
    int64_t window;                                     ///< Destination window identifier.
    char_t  dFPath[UTL_ARINC661_MAX_PATH_BYTES];        ///< Definition File path.
    char_t  styleFilePath[UTL_ARINC661_MAX_PATH_BYTES]; ///< Optional style file path.
    bool_t  mapSourceLayer;                             ///< Map source layer setting.
    bool_t  visible;                                    ///< Initial visibility.
} Utl_Arinc661_UserApplication_t;

/*************************************************************************************************/
/**
 * \brief FACE display management ExternalSource attributes.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, ExternalSource complexType and section H.3.7.
 */
typedef struct Utl_Arinc661_ExternalSource_s
{
    uint32_t id;                                ///< External source identifier.
    char_t   name[UTL_ARINC661_MAX_NAME_BYTES]; ///< Optional source name.
    char_t   type[UTL_ARINC661_MAX_NAME_BYTES]; ///< Optional source type.
} Utl_Arinc661_ExternalSource_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned FACE display management configuration data.
 *
 * \par Details:
 *  FACE Edition 3.1, Appendix H.3, configuration root element.
 */
typedef struct Utl_Arinc661_Configuration_s
{
    Utl_Arinc661_Screen_t*          screens;                 ///< Screen storage.
    Utl_Arinc661_Layout_t*          layouts;                 ///< Layout storage.
    Utl_Arinc661_Window_t*          windows;                 ///< Window storage.
    Utl_Arinc661_UserApplication_t* userApplications;        ///< User application storage.
    Utl_Arinc661_ExternalSource_t*  externalSources;         ///< External source storage.
    uint32_t                        screenCapacity;          ///< Screen slots.
    uint32_t                        layoutCapacity;          ///< Layout slots.
    uint32_t                        windowCapacity;          ///< Window slots.
    uint32_t                        userApplicationCapacity; ///< User application slots.
    uint32_t                        externalSourceCapacity;  ///< External source slots.
    uint32_t                        screenCount;             ///< Screens in use.
    uint32_t                        layoutCount;             ///< Layouts in use.
    uint32_t                        windowCount;             ///< Windows in use.
    uint32_t                        userApplicationCount;    ///< User applications in use.
    uint32_t                        externalSourceCount;     ///< External sources in use.
    bool_t                          hasMetaData;             ///< TRUE when an empty metaData element is present.
} Utl_Arinc661_Configuration_t;

/*************************************************************************************************/
/**
 * \brief Owner of an ARINC 661 Definition File property.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: dfType, layerType, and widgetType each contain a model.
 */
typedef enum Utl_Arinc661_PropertyOwner_e
{
    UTL_ARINC661_PROPERTY_DEFINITION,
    UTL_ARINC661_PROPERTY_LAYER,
    UTL_ARINC661_PROPERTY_WIDGET
} Utl_Arinc661_PropertyOwner_t;

/*************************************************************************************************/
/**
 * \brief Property in a Definition File model.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: modelType/propType, with name and value attributes.
 */
typedef struct Utl_Arinc661_Property_s
{
    Utl_Arinc661_PropertyOwner_t owner;                                  ///< Owning model kind.
    uint32_t                     ownerIndex;                             ///< Layer or widget index, or zero.
    char_t                       name[UTL_ARINC661_MAX_NAME_BYTES];      ///< Property name.
    char_t                       value[UTL_ARINC661_MAX_PROPERTY_BYTES]; ///< Property value.
} Utl_Arinc661_Property_t;

/*************************************************************************************************/
/**
 * \brief Layer in an ARINC 661 Definition File.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: dfType/a661_layer (layerType).
 */
typedef struct Utl_Arinc661_Layer_s
{
    char_t name[UTL_ARINC661_MAX_NAME_BYTES]; ///< Optional layer name.
} Utl_Arinc661_Layer_t;

/*************************************************************************************************/
/**
 * \brief Widget in an ARINC 661 Definition File.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: layerType/a661_widget and nested widgetType/a661_widget.
 */
typedef struct Utl_Arinc661_Widget_s
{
    char_t   name[UTL_ARINC661_MAX_NAME_BYTES]; ///< Optional widget name.
    char_t   type[UTL_ARINC661_MAX_NAME_BYTES]; ///< Required widget type.
    uint32_t layerIndex;                        ///< Owning layer.
    uint32_t parentWidgetIndex;                 ///< Parent widget, or UINT32_MAX for a layer child.
} Utl_Arinc661_Widget_t;

/*************************************************************************************************/
/**
 * \brief Caller-owned ARINC 661 Definition File data.
 *
 * \par Details:
 *  Supplement 6 a661Strict.xsd: a661_df root (dfType), with model and a661_layer children.
 */
typedef struct Utl_Arinc661_Definition_s
{
    char_t                   name[UTL_ARINC661_MAX_NAME_BYTES]; ///< Optional Definition File name.
    uint32_t                 libraryVersion;                    ///< Library version.
    uint32_t                 supplementVersion;                 ///< Supplement version.
    Utl_Arinc661_Layer_t*    layers;                            ///< Layer storage.
    Utl_Arinc661_Widget_t*   widgets;                           ///< Widget storage.
    Utl_Arinc661_Property_t* properties;                        ///< Property storage.
    uint32_t                 layerCapacity;                     ///< Layer slots.
    uint32_t                 widgetCapacity;                    ///< Widget slots.
    uint32_t                 propertyCapacity;                  ///< Property slots.
    uint32_t                 layerCount;                        ///< Layers in use.
    uint32_t                 widgetCount;                       ///< Widgets in use.
    uint32_t                 propertyCount;                     ///< Properties in use.
} Utl_Arinc661_Definition_t;

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
 * \brief Reads a display management configuration from memory into caller-owned data.
 *
 * \par Details:
 *  FACE Technical Standard, Edition 3.1, Appendix H.3, DisplayManagement.xsd
 *  defines configuration and its screen, ua, and externalSource children;
 *  sections H.3.1 through H.3.7 describe those values:
 *  https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 *  This function reads those values without retaining XML nodes in the result.
 *  Text content in metaData and description, and child elements of externalSource,
 *  are outside the supported XML form of Utl_Xml_ReadDocument.
 *  The input buffer must have one writable byte after byteCount bytes.
 *  The reader changes the input buffer while constructing the temporary XML tree.
 *
 * \par Requirements:
 *  - 001 This function shall read screen, layout, window, user application, and external source values into caller-owned arrays.
 *  - 002 This function shall clear all result counts when a supported value is invalid or a result array is too small.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall fail an assertion if configuration, buffer, or workspace is NULL.
 *  - 005 This function shall return FALSE when Utl_Xml_ReadDocument rejects the input.
 *
 * \param[in,out] configuration   Configuration with storage pointers and capacities initialized
 * \param[in,out] buffer          Input XML bytes, followed by one writable byte
 * \param[in]     byteCount       Number of input bytes, excluding the extra byte
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \retval        TRUE        if the supported configuration values were read.
 * \retval        FALSE       if the XML or a supported value is invalid, or supplied storage is insufficient.
 */
bool_t Utl_Arinc661_ReadConfiguration(Utl_Arinc661_Configuration_t* configuration, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Writes caller-owned display management configuration data to memory.
 *
 * \par Details:
 *  Writes the FACE Edition 3.1, Appendix H.3, DisplayManagement.xsd
 *  configuration element from typed values:
 *  https://www.vdl.afrl.af.mil/programs/arsenl/files/FACE%20Technical%20Standard%20v3.1.pdf
 *  metaData and window description are written as empty elements. A nonempty
 *  window description cannot be represented by Utl_Xml_WriteDocument and
 *  causes this function to return zero.
 *
 * \par Requirements:
 *  - 001 This function shall write screens, layouts, windows, user applications, and external sources in schema order.
 *  - 002 This function shall return the number of bytes written without adding a zero terminator.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall fail an assertion if configuration, buffer, or workspace is NULL.
 *  - 005 This function shall return zero when Utl_Xml_WriteDocument rejects the output capacity.
 *
 * \param[in]     configuration   Configuration data to write
 * \param[out]    buffer          Output XML bytes
 * \param[in]     capacity        Output buffer capacity
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \return     Number of bytes written, or zero when a value or supplied storage is invalid.
 */
uint64_t Utl_Arinc661_WriteConfiguration(const Utl_Arinc661_Configuration_t* configuration, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Reads an ARINC 661 Definition File from memory into caller-owned data.
 *
 * \par Details:
 *  Reads the ARINC 661 Supplement 6 XML Definition File form. The strict
 *  a661Strict.xsd schema names a661_df (dfType), a661_layer (layerType),
 *  a661_widget (widgetType), model (modelType), and prop (propType):
 *  https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 *  This reader supports layers, nested widgets, and simple prop elements.
 *  The result contains no XML nodes.
 *  The input buffer must have one writable byte after byteCount bytes and is
 *  changed by Utl_Xml_ReadDocument while parsing.
 *
 * \par Requirements:
 *  - 001 This function shall read Definition File, layer, widget, and simple property values into caller-owned storage.
 *  - 002 This function shall clear all result counts when a supported value is invalid or a result array is too small.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall fail an assertion if definition, buffer, or workspace is NULL.
 *  - 005 This function shall return FALSE when Utl_Xml_ReadDocument rejects the input.
 *
 * \param[in,out] definition      Definition File with storage pointers and capacities initialized
 * \param[in,out] buffer          Input XML bytes, followed by one writable byte
 * \param[in]     byteCount       Number of input bytes, excluding the extra byte
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \retval        TRUE        if the supported Definition File values were read.
 * \retval        FALSE       if the XML or a supported value is invalid, or supplied storage is insufficient.
 */
bool_t Utl_Arinc661_ReadDefinition(Utl_Arinc661_Definition_t* definition, byte_t* buffer, uint64_t byteCount, void* workspace, uint64_t workspaceSize);

/*************************************************************************************************/
/**
 * \brief Writes caller-owned ARINC 661 Definition File data to memory.
 *
 * \par Details:
 *  Writes the ARINC 661 Supplement 6 XML Definition File form described by
 *  a661Strict.xsd, using a661_df, a661_layer, a661_widget, model, and prop:
 *  https://j661.sourceforge.net/articles/articles/resources/a661Strict.xsd
 *  This writer supports layers, nested widgets, and simple prop elements.
 *  The result is XML in the caller's output buffer.
 *
 * \par Requirements:
 *  - 001 This function shall write Definition File, layer, widget, and property values from caller-owned data.
 *  - 002 This function shall return the number of bytes written without adding a zero terminator.
 *  - 003 This function shall use only the supplied workspace for temporary XML storage.
 *  - 004 This function shall fail an assertion if definition, buffer, or workspace is NULL.
 *  - 005 This function shall return zero when Utl_Xml_WriteDocument rejects the output capacity.
 *
 * \param[in]     definition      Definition File data to write
 * \param[out]    buffer          Output XML bytes
 * \param[in]     capacity        Output buffer capacity
 * \param[in,out] workspace       Eight-byte-aligned temporary storage
 * \param[in]     workspaceSize   Number of temporary bytes
 *
 * \return     Number of bytes written, or zero when a value or supplied storage is invalid.
 */
uint64_t Utl_Arinc661_WriteDefinition(const Utl_Arinc661_Definition_t* definition, byte_t* buffer, uint64_t capacity, void* workspace, uint64_t workspaceSize);

#endif /* UTL_ARINC661_H */
