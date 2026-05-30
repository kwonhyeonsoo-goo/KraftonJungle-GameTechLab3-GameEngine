#pragma once

#include "imgui.h"
#include "Editor/UI/Asset/AssetEditorWidget.h"
#include "Editor/Viewport/Asset/StaticMeshEditorViewportClient.h"
#include "Materials/Graph/MaterialGraphTypes.h"
#include "Object/FName.h"

namespace ax::NodeEditor
{
    struct EditorContext;
}

class UMaterial;
class UTexture2D;
class AActor;
class UStaticMeshComponent;
class IEditorPreviewViewportClient;

// Material graph editor for 20. It intentionally mirrors the Lua Blueprint editor workflow:
// searchable add-node menu, context-sensitive pin spawn, undo/redo snapshots, copy/paste,
// duplicate/delete, comment grouping, separated Save / Preview Compile / Apply Compile.
class FMaterialEditorWidget : public FAssetEditorWidget
{
public:
    FMaterialEditorWidget() = default;
    ~FMaterialEditorWidget() override;

    bool CanEdit(UObject* Object) const override;
    void Open(UObject* Object) override;
    void Close() override;
    void Render(float DeltaTime) override;
    void RenderDocument(float DeltaTime) override;

    bool AllowsMultipleInstances() const override
    {
        return true;
    }

    bool    IsEditingObject(UObject* Object) const override;
    FString GetDocumentTitle() const override;
    FString GetDocumentPayloadId() const override;

    EEditorDocumentTabKind GetDocumentTabKind() const override
    {
        return EEditorDocumentTabKind::MaterialEditor;
    }

    void AddReferencedObjects(FReferenceCollector& Collector) override;
    void Tick(float DeltaTime) override;
    void CollectPreviewViewports(TArray<IEditorPreviewViewportClient*>& OutClients) const override;

private:
    void       EnsureContext();
    void       DestroyContext();
    UMaterial* GetMaterial() const;

    void SetupPreviewScene();
    void TeardownPreviewScene();
    void ApplyPreviewMaterialToMesh();
    void RenderPreviewPanel(UMaterial* Material);

    void RenderToolbar(UMaterial* Material);
    void RenderLeftPanel(UMaterial* Material);
    void RenderSettingsPanel(UMaterial* Material);
    void RenderParametersPanel(UMaterial* Material);
    void RenderPalettePanel(UMaterial* Material);
    void RenderGraphCanvas(UMaterial* Material);
    void RenderNodeBody(FMaterialGraphNode& Node);
    void RenderNodeValueEditor(FMaterialGraphNode& Node, bool bCompact);
    void RenderDetailsPanel(UMaterial* Material);
    void RenderNodeInspector(UMaterial* Material, FMaterialGraphNode& Node);
    void RenderCompilerPanel(UMaterial* Material);
    void RenderAddNodePopup(UMaterial* Material);
    void RenderAddNodeMenuBody(UMaterial* Material);
    void RenderPinSpawnMenuBody(UMaterial* Material);

    void SaveGraph(UMaterial* Material);
    void CompilePreview(UMaterial* Material);
    void ApplyCompile(UMaterial* Material, bool bPersistCompiledState);
    void SaveAll(UMaterial* Material);

    void          CaptureInitialUndoSnapshot();
    void          CommitGraphEdit();
    void          UndoGraphEdit();
    void          RedoGraphEdit();
    TArray<uint8> MakeGraphSnapshot() const;
    bool          RestoreGraphSnapshot(const TArray<uint8>& Snapshot);

    bool GatherSelectedNodes(TArray<FMaterialGraphNode>& OutNodes, TArray<FMaterialGraphLink>& OutLinks) const;
    bool CloneNodeFragment(const TArray<FMaterialGraphNode>& SourceNodes, const TArray<FMaterialGraphLink>& SourceLinks, const ImVec2& TargetAnchor, TArray<uint32>* OutNewNodeIds = nullptr);
    void SelectOnlyNodes(const TArray<uint32>& NodeIds);
    void CopySelectedNodes();
    void PasteCopiedNodes(const ImVec2* OverrideAnchor = nullptr);
    void DuplicateSelectedNodes();
    void DeleteSelectedNodes();
    void GroupSelectedNodesAsComment();

    bool               AddNodeMenuItem(UMaterial* Material, EMaterialGraphNodeType Type, const char* Label = nullptr);
    bool               AddContextNodeMenuItem(UMaterial* Material, EMaterialGraphNodeType Type);
    bool               NodeTypeCanConnectToPendingPin(EMaterialGraphNodeType Type) const;
    FMaterialGraphPin* FindFirstCompatiblePinOnNode(FMaterialGraphNode& Node, const FMaterialGraphPin& DragPin) const;

    void QueueNodeEditorShortcuts();
    void ProcessQueuedNodeEditorCommands();

private:
    ax::NodeEditor::EditorContext* NodeEditorContext         = nullptr;
    bool                           bPositionsPushed          = false;
    bool                           bPendingInitialContentFit = false;

    FMaterialGraph WorkingGraph;
    UMaterial*     PreviewMaterial = nullptr;

    uint32 SelectedNodeId = 0;

    FString LastCompileError;
    FString LastCompileStatus;
    bool    bLastCompileOk = false;
    FString HlslViewPath;
    FString HlslViewText;
    char    AddNodeSearchBuf[96]  = {};
    char    PinSpawnSearchBuf[96] = {};
    char    PaletteSearchBuf[96]  = {};

    bool   bShowAddNodeMenu       = false;
    bool   bShowPinSpawnMenu      = false;
    uint32 PendingPinSpawnPinId   = 0;
    ImVec2 PendingNewNodePosition = ImVec2(0, 0);
    ImVec2 PendingPinSpawnPos     = ImVec2(0, 0);

    TArray<TArray<uint8>>      UndoStack;
    TArray<TArray<uint8>>      RedoStack;
    TArray<FMaterialGraphNode> ClipboardNodes;
    TArray<FMaterialGraphLink> ClipboardLinks;
    bool                       bRestoringSnapshot = false;

    bool bQueuedCopySelected      = false;
    bool bQueuedPasteNodes        = false;
    bool bQueuedDuplicateSelected = false;
    bool bQueuedDeleteSelected    = false;
    bool bQueuedGroupSelected     = false;

    // 3D material preview — a lit sphere with the compiled preview material applied.
    FStaticMeshEditorViewportClient PreviewViewportClient;
    FName                           PreviewWorldHandle     = FName::None;
    AActor*                         PreviewMeshActor       = nullptr;
    UStaticMeshComponent*           PreviewMeshComp        = nullptr;
    UMaterial*                      AppliedPreviewMaterial = nullptr;
    int32                           InstanceId             = 0;
    bool                            bPreviewSceneReady     = false;
};