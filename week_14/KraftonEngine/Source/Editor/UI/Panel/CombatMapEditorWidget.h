#pragma once

#include "Editor/UI/EditorWidget.h"
#include "Math/Vector.h"

struct ImVec2;
namespace ax { namespace NodeEditor { struct EditorContext; } }

class AActor;
class UCombatCoverAgentComponent;
class UCombatCoverNodeComponent;
class UCombatFlowManagerComponent;
class UWorld;

class FCombatMapEditorWidget : public FEditorWidget
{
public:
    FCombatMapEditorWidget() = default;
    ~FCombatMapEditorWidget() override;

    void Initialize(UEditorEngine* InEditorEngine) override;
    void Render(float DeltaTime) override;

private:
    void Refresh();
	void RefreshIfWorldOrPIEStateChanged();
	void ClearCachedRuntimePointers();
	void PruneInvalidCachedReferences();
    UWorld* GetEditorWorld() const;
    AActor* GetSelectedActor() const;
    UCombatFlowManagerComponent* FindOrUseManager() const;
    void RenderToolbar();
    void RenderTwoColumnLayout();
    void RenderLeftColumn();
    void RenderRightColumn();
    void RenderNodeList();
    void RenderSelectedNodePanel();
    void RenderSlotPanel(UCombatCoverNodeComponent* Node);
    void RenderLinkPanel(UCombatCoverNodeComponent* Node);
    void RenderGraphEditor();
    void RenderAgentPanel();
    void RenderValidationPanel();
    void RenderPIEControls();
    template<typename TComponent>
    TComponent* AddComponentToSelectedActor();
    void DrawAllDebugOnce();
    void SelectNode(UCombatCoverNodeComponent* Node);

    void InitializeGraphEditor();
    void DestroyGraphEditor();
    void ResetGraphLayoutFromScene();
    void EnsureGraphNodePositionFromScene(UCombatCoverNodeComponent* Node, int32 NodeIndex);
    void ApplyGraphPositionToScene(UCombatCoverNodeComponent* Node, int32 NodeIndex);
    FVector GraphToWorld(const ImVec2& Position) const;
    ImVec2 WorldToGraph(const FVector& Position) const;

    UCombatCoverNodeComponent* CreateCoverNodeActorFromEditor();
    UCombatCoverNodeComponent* DuplicateSelectedCoverNodeActor();
    void GenerateNodeIdsAndRenameActors();
    void RenameActorToNodeId(UCombatCoverNodeComponent* Node);

private:
    TArray<UCombatCoverNodeComponent*> CachedNodes;
    TArray<UCombatCoverAgentComponent*> CachedAgents;
    UCombatFlowManagerComponent* CachedManager = nullptr;
	UWorld* CachedWorld = nullptr;
    UCombatCoverNodeComponent* SelectedNode = nullptr;
    int32 SelectedSlotIndex = -1;
    int32 LinkTargetIndex = -1;
    float AutoLinkMaxDistance = 1500.0f;
    int32 AutoLinkMaxLinksPerNode = 2;
    bool bAutoLinkDirectedByX = true;
    bool bShowOnlySelectedNodeDebug = false;
    TArray<FString> LastValidationMessages;

    ax::NodeEditor::EditorContext* GraphEditorContext = nullptr;
    TSet<uint32> InitializedGraphItemIds;
    bool bGraphApplyToScene = true;
    bool bGraphMirrorX = false;
    bool bGraphMirrorY = false;
	bool bWasPlayingInEditor = false;
	bool bPendingGraphNavigateToContent = false;
    float GraphSceneUnitsPerGraphUnit = 1.0f / 15.0f;
};
