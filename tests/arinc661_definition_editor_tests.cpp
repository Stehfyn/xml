/*=================================================================================================
** 1. References
**===============================================================================================*/

/*=================================================================================================
** 2. INCLUDE FILES
**===============================================================================================*/

/* Include the editor source so its static model functions can be exercised without a window. */
#include "../examples/arinc661_definition_editor.cpp"

extern "C"
{
#include "os/os_diagnostics.h"
#include "os/os_malloc.h"
}

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

/*=================================================================================================
** 3. DECLARATIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \brief Counts the properties owned by one model.
 */
static uint32_t CountOwned(const Arinc661_DefinitionEditor_Document_t* document, Arinc661_PropertyOwner_t owner, uint32_t ownerIndex)
{
    uint32_t property_index;
    uint32_t count;

    count = 0;
    for (property_index = 0; property_index < document->definition.propertyCount; ++property_index)
    {
        if ((owner == document->properties[property_index].owner) && (ownerIndex == document->properties[property_index].ownerIndex))
        {
            count += 1;
        }
    }

    return count;
}

/*=================================================================================================
** 4. PUBLIC FUNCTIONS
**===============================================================================================*/

/*************************************************************************************************/
/**
 * \par Details:
 *  Checks the editor model without a window: array order and ownership after insertions and
 *  removals, parameter seeding for every widget and extension type, duplication, moves,
 *  history, play mode, the animation list, binary export, and prefabs. The edited definition
 *  is written as binary along the way.
 */
int main(void)
{
    Arinc661_DefinitionEditor_t*          editor;
    Arinc661_DefinitionEditor_Document_t* document;
    Arinc661_Parameter_t                  extension_parameter;
    Arinc661_Parameter_t                  parameter;
    char_t                                value[ARINC661_MAX_PROPERTY_BYTES];
    uint32_t                              type_index;
    uint32_t                              parameter_count;
    uint32_t                              parameter_index;
    const char_t*                         animation_name;
    const char_t*                         animation_type;
    FILE*                                 export_file;
    char_t                                export_path[ARINC661_DEFINITION_EDITOR_MAX_FILE_PATH_BYTES];
    uint64_t                              byte_count;
    uint32_t                              extension_parameter_count;
    uint32_t                              extension_index;
    uint32_t                              single_count;
    uint32_t                              widget_count_before;
    uint32_t                              subtree_size;
    uint32_t                              history_index;
    float32_t                             button_state;
    float32_t                             animation_delay;
    float32_t                             animation_duration;

    Os_Diagnostics_Initialize();
    editor = (Arinc661_DefinitionEditor_t*)OS_CALLOC(sizeof(Arinc661_DefinitionEditor_t));
    Arinc661_DefinitionEditor_Initialize(editor);
    CreateDocument(editor);
    document = &editor->documents[0];
    assert(document->isLoaded);

    /* Two layers; the first gets a panel with a child panel and an extension (new widgets start as panels) */
    AddLayer(document);
    AddLayer(document);
    assert(2 == document->definition.layerCount);
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);                 /* widget 0: panel */
    AddWidget(document, 0, 0, FALSE);                                       /* widget 1: child of 0 */
    AddWidget(document, 0, 0, TRUE);                                        /* extension goes to index 1, child moves to 2 */
    assert(3 == document->definition.widgetCount);
    assert(TRUE == document->widgets[1].isExtension);
    assert(0 == document->widgets[1].parentWidgetIndex);
    assert(FALSE == document->widgets[2].isExtension);
    assert(0 == document->widgets[2].parentWidgetIndex);
    /* The child has WidgetIdent plus the eight Table 3.3.27-2 parameters; the extension takes a type Table 8.4 allows on a panel */
    assert(9 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 2));
    assert(Arinc661_Definition_ExtensionApplies(document->widgets[1].type, "A661_PANEL"));
    assert(Arinc661_Definition_FindType(document->widgets[1].type, TRUE, &extension_parameter_count));
    /* Every single-valued extension parameter is seeded; arrays start empty */
    single_count = 0;
    for (extension_index = 0; extension_index < extension_parameter_count; ++extension_index)
    {
        (void)Arinc661_Definition_GetParameter(document->widgets[1].type, TRUE, extension_index, &extension_parameter);
        if (0 == extension_parameter.elementsPerEntry)
        {
            single_count += 1;
        }
    }
    assert(single_count == CountOwned(document, ARINC661_PROPERTY_WIDGET, 1));
    AddWidget(document, 0, 2, FALSE);                                       /* widget 3: grandchild under 2 */
    AddWidget(document, 0, 0, FALSE);                                       /* widget 4: second child of 0, after the subtree of 2 */
    assert(5 == document->definition.widgetCount);
    assert(2 == document->widgets[3].parentWidgetIndex);
    assert(0 == document->widgets[4].parentWidgetIndex);
    AddWidget(document, 1, INVALID_HIERARCHY_INDEX, FALSE);                 /* widget 5: layer 1 */
    strcpy(document->widgets[5].type, "A661_LABEL");
    assert(ConformParameters(document, 5));
    /* A label has no Enable; WidgetIdent plus the fourteen Table 3.3.20-2 parameters remain */
    assert(15 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 5));
    assert(INVALID_HIERARCHY_INDEX == FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 5, PROPERTY_ENABLE));
    assert(INVALID_HIERARCHY_INDEX != FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 5, "Alignment"));
    assert(5 == document->widgets[5].parentWidgetIndex + 6);
    assert(SubtreeEnd(document, 0) == 5);
    assert(SubtreeEnd(document, 2) == 4);
    assert(TRUE == IsDescendant(document, 3, 0));
    assert(FALSE == IsDescendant(document, 4, 2));

    /* The definition writes as binary, with the extension command directly after the 32-byte panel command */
    byte_count = Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer));
    assert(0 < byte_count);
    assert(0xCA == document->outputBuffer[16 + 32]);
    assert(0x05 == document->outputBuffer[16 + 33]);

    /* Removing the child subtree drops widgets 2 and 3 and renumbers 4 and 5 */
    RemoveWidgetSubtree(document, 2);
    assert(4 == document->definition.widgetCount);
    assert(0 == document->widgets[2].parentWidgetIndex);
    assert(1 == document->widgets[3].layerIndex);
    assert(9 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 2));
    assert(15 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 3));
    assert(0 == strncmp("A661_LABEL", document->widgets[3].type, sizeof("A661_LABEL")));

    /* Removing layer 0 leaves only the layer 1 widget, now in layer 0 */
    RemoveLayer(document, 0);
    assert(1 == document->definition.layerCount);
    assert(1 == document->definition.widgetCount);
    assert(0 == document->widgets[0].layerIndex);
    assert(INVALID_HIERARCHY_INDEX == document->widgets[0].parentWidgetIndex);
    assert(4 == CountOwned(document, ARINC661_PROPERTY_LAYER, 0));
    assert(15 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 0));
    assert(0 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 1));
    byte_count = Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer));
    assert(0 < byte_count);

    /* Removing a property leaves the others; the edited definition round-trips through XML */
    RemovePropertyAt(document, FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 0, PROPERTY_VISIBLE));
    assert(14 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 0));
    assert(INVALID_HIERARCHY_INDEX == FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 0, PROPERTY_VISIBLE));
    byte_count = Arinc661_Definition_Write(&document->definition, document->outputBuffer, sizeof(document->outputBuffer), document->workspace, sizeof(document->workspace));
    assert(0 < byte_count);
    memcpy(document->inputBuffer, document->outputBuffer, (size_t)byte_count);
    assert(Arinc661_Definition_Read(&document->definition, document->inputBuffer, byte_count, document->workspace, sizeof(document->workspace)));
    assert(1 == document->definition.widgetCount);
    assert(14 == CountOwned(document, ARINC661_PROPERTY_WIDGET, 0));

    /* Every widget and extension type seeds a property set that writes as binary */
    for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(FALSE); ++type_index)
    {
        strcpy(document->widgets[0].type, Arinc661_Definition_GetTypeName(FALSE, type_index));
        (void)ConformParameters(document, 0);
        if (0 == Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer)))
        {
            fprintf(stderr, "seed write failed for %s\n", document->widgets[0].type);
            for (parameter_index = 0; parameter_index < document->definition.propertyCount; ++parameter_index)
            {
                if ((ARINC661_PROPERTY_WIDGET == document->properties[parameter_index].owner) && (0 == document->properties[parameter_index].ownerIndex))
                {
                    fprintf(stderr, "  %s = %s\n", document->properties[parameter_index].name, document->properties[parameter_index].value);
                }
            }
            assert(FALSE);
        }
        /* Growing every array to two entries still writes */
        assert(Arinc661_Definition_FindType(document->widgets[0].type, FALSE, &parameter_count));
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Definition_GetParameter(document->widgets[0].type, FALSE, parameter_index, &parameter);
            if (0 != parameter.elementsPerEntry)
            {
                strcpy(document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 0, parameter.countName)].value, "2");
            }
        }
        for (parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
        {
            (void)Arinc661_Definition_GetParameter(document->widgets[0].type, FALSE, parameter_index, &parameter);
            if (0 != parameter.elementsPerEntry)
            {
                (void)SyncArrayLength(document, 0, &parameter);
                assert(INVALID_HIERARCHY_INDEX != FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, 0, parameter.name, (2 * parameter.elementsPerEntry) - 1));
                assert(INVALID_HIERARCHY_INDEX == FindOwnedPropertyAt(document, ARINC661_PROPERTY_WIDGET, 0, parameter.name, 2 * parameter.elementsPerEntry));
                DefaultParameterValue(&parameter, value, sizeof(value));
            }
        }
        if (0 == Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer)))
        {
            fprintf(stderr, "array write failed for %s\n", document->widgets[0].type);
            assert(FALSE);
        }
    }
    strcpy(document->widgets[0].type, "A661_PANEL");
    (void)ConformParameters(document, 0);
    AddWidget(document, 0, 0, TRUE);
    for (type_index = 0; type_index < Arinc661_Definition_GetTypeCount(TRUE); ++type_index)
    {
        strcpy(document->widgets[1].type, Arinc661_Definition_GetTypeName(TRUE, type_index));
        (void)ConformParameters(document, 1);
        if (0 == Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer)))
        {
            fprintf(stderr, "extension write failed for %s\n", document->widgets[1].type);
            assert(FALSE);
        }
    }

    /* Duplicating a container copies its subtree and extensions with fresh idents after the source */
    document->definition.widgetCount   = 0;
    document->definition.propertyCount = 0;
    document->definition.layerCount    = 0;
    AddLayer(document);
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);                 /* 0: panel */
    AddWidget(document, 0, 0, FALSE);                                       /* 1: child */
    AddWidget(document, 0, 0, TRUE);                                        /* extension at 1, child moves to 2 */
    AddWidget(document, 0, 2, FALSE);                                       /* 3: grandchild */
    assert(4 == document->definition.widgetCount);
    assert(4 == DuplicateWidget(document, 0));
    assert(8 == document->definition.widgetCount);
    assert(INVALID_HIERARCHY_INDEX == document->widgets[4].parentWidgetIndex);
    assert(TRUE == document->widgets[5].isExtension);
    assert(4 == document->widgets[5].parentWidgetIndex);
    assert(4 == document->widgets[6].parentWidgetIndex);
    assert(6 == document->widgets[7].parentWidgetIndex);
    assert(0 == strncmp("A661_PANEL", document->widgets[4].type, sizeof("A661_PANEL")));
    assert(NULL != strstr(document->widgets[4].name, "copy"));
    assert(CountOwned(document, ARINC661_PROPERTY_WIDGET, 0) == CountOwned(document, ARINC661_PROPERTY_WIDGET, 4));
    assert(CountOwned(document, ARINC661_PROPERTY_WIDGET, 3) == CountOwned(document, ARINC661_PROPERTY_WIDGET, 7));
    assert(0 != strncmp(document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 0, PROPERTY_WIDGET_IDENT)].value, document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 4, PROPERTY_WIDGET_IDENT)].value, ARINC661_MAX_PROPERTY_BYTES));
    assert(0 < Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer)));

    /* Moving the grandchild (3) under the copied panel (4) re-parents and selects it */
    assert(MoveWidgetSubtree(document, 3, 0, 4));
    assert(8 == document->definition.widgetCount);
    assert(SELECTION_WIDGET == document->selectedKind);
    assert(document->widgets[document->selectedIndex].parentWidgetIndex == 3);  /* panel copy shifted down to 3 after the removal */
    assert(SubtreeEnd(document, 0) == 3);
    assert(0 < Arinc661_Definition_WriteBinary(&document->definition, document->outputBuffer, sizeof(document->outputBuffer)));
    /* Moving a container onto its own descendant is refused */
    assert(FALSE == MoveWidgetSubtree(document, 3, 0, document->selectedIndex));
    /* Moving a top-level panel to the layer is a no-op */
    assert(FALSE == MoveWidgetSubtree(document, 0, 0, INVALID_HIERARCHY_INDEX));

    /* History: each committed change is one undo step; undo and redo restore the arrays; a new change clears redo */
    ClearHistory(document);
    CommitHistory(document);
    assert(0 == document->undoCount);
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);
    CommitHistory(document);
    assert(1 == document->undoCount);
    assert(9 == document->definition.widgetCount);
    UndoDocument(document);
    assert(8 == document->definition.widgetCount);
    assert(0 == document->undoCount);
    assert(1 == document->redoCount);
    RedoDocument(document);
    assert(9 == document->definition.widgetCount);
    assert(1 == document->undoCount);
    assert(0 == document->redoCount);
    UndoDocument(document);
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);
    CommitHistory(document);
    assert(0 == document->redoCount);
    /* History states carry labels, the open state then the addition, and GoToHistory moves between them */
    assert(2 == Arinc661_DefinitionEditor_HistoryCount(editor, &history_index));
    assert(1 == history_index);
    assert(0 == strncmp(HISTORY_OPEN_LABEL, Arinc661_DefinitionEditor_HistoryLabel(editor, 0), sizeof(HISTORY_OPEN_LABEL)));
    assert(0 == strncmp("Add ", Arinc661_DefinitionEditor_HistoryLabel(editor, 1), 4));
    Arinc661_DefinitionEditor_GoToHistory(editor, 0);
    assert(8 == document->definition.widgetCount);
    Arinc661_DefinitionEditor_GoToHistory(editor, 1);
    assert(9 == document->definition.widgetCount);
    ClearHistory(document);
    assert(NULL == document->shadow);

    /* Play mode: interaction flips a toggle, advances a multi-state button to state 1, and ignores a panel; stopping restores the values and the dirty flag */
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);
    strcpy(document->widgets[9].type, "A661_TOGGLE_BUTTON");
    assert(ConformParameters(document, 9));
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);
    strcpy(document->widgets[10].type, "A661_MULTI_STATE_BUTTON");
    assert(ConformParameters(document, 10));
    strcpy(document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 9, "ToggleState")].value, "A661_UNSELECTED");
    document->isDirty = FALSE;
    Arinc661_DefinitionEditor_BeginPlay(editor);
    assert(editor->isPlaying);
    assert(Arinc661_DefinitionEditor_Interact(editor, 0, 9, 0.5f, 0.5f));
    assert(0 == strncmp("A661_SELECTED", document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 9, "ToggleState")].value, sizeof("A661_SELECTED")));
    assert(Arinc661_DefinitionEditor_Interact(editor, 0, 10, 0.5f, 0.5f));
    assert(GetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, 10, "ButtonState", &button_state));
    assert(1.0f == button_state);
    assert(FALSE == Arinc661_DefinitionEditor_Interact(editor, 0, 0, 0.5f, 0.5f));
    Arinc661_DefinitionEditor_EndPlay(editor);
    assert(FALSE == editor->isPlaying);
    assert(FALSE == document->isDirty);
    assert(0 == strncmp("A661_UNSELECTED", document->properties[FindOwnedProperty(document, ARINC661_PROPERTY_WIDGET, 9, "ToggleState")].value, sizeof("A661_UNSELECTED")));

    /* The animation list gives delay and duration in seconds */
    AddWidget(document, 0, INVALID_HIERARCHY_INDEX, FALSE);
    strcpy(document->widgets[11].type, "A661_ANIMATION_ROTATION");
    assert(ConformParameters(document, 11));
    SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, 11, "Delay", 500.0f);
    SetPropertyNumber(document, ARINC661_PROPERTY_WIDGET, 11, "Duration", 2000.0f);
    assert(Arinc661_DefinitionEditor_AnimationAt(editor, 0, &animation_name, &animation_type, &animation_delay, &animation_duration));
    assert(0 == strncmp("A661_ANIMATION_ROTATION", animation_type, sizeof("A661_ANIMATION_ROTATION")));
    assert(0.5f == animation_delay);
    assert(2.0f == animation_duration);
    assert(FALSE == Arinc661_DefinitionEditor_AnimationAt(editor, 1, &animation_name, &animation_type, &animation_delay, &animation_duration));

    /* Export writes the .bin file beside the XML path; a slot without a document is refused */
    strcpy(document->documentPath, "arinc661_editor_export_test.xml");
    assert(Arinc661_DefinitionEditor_ExportBinary(editor, 0, export_path, sizeof(export_path)));
    assert(0 == strncmp("arinc661_editor_export_test.bin", export_path, sizeof("arinc661_editor_export_test.bin")));
    export_file = fopen(export_path, "rb");
    assert(NULL != export_file);
    (void)fclose(export_file);
    (void)remove(export_path);
    assert(FALSE == Arinc661_DefinitionEditor_ExportBinary(editor, 1, export_path, sizeof(export_path)));

    /* A prefab round trip: the selected panel subtree is saved, then pasted back from the file; a name with a slash is refused */
    strcpy(editor->folderPath, ".");
    SelectOnly(document, 0);
    widget_count_before = document->definition.widgetCount;
    subtree_size        = SubtreeEnd(document, 0);
    assert(0 < Arinc661_DefinitionEditor_Copy(editor));
    assert(Arinc661_DefinitionEditor_SavePrefab(editor, "editor_test_prefab", export_path, sizeof(export_path)));
    assert(FALSE == Arinc661_DefinitionEditor_SavePrefab(editor, "bad/name", export_path, sizeof(export_path)));
    assert(Arinc661_DefinitionEditor_LoadPrefab(editor, "editor_test_prefab.prefab.xml"));
    assert(document->definition.widgetCount == (widget_count_before + subtree_size));
    (void)remove("./editor_test_prefab.prefab.xml");

    return 0;
}
