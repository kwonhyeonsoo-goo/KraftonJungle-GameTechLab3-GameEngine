#pragma once

#include "Editor/UI/EditorWidget.h"
#include "Math/Vector.h"

class AActor;
class UCombatCoverAgentComponent;
class UCombatCoverNodeComponent;
class UCombatFlowManagerComponent;
class UWorld;

class FCombatMapEditorWidget : public FEditorWidget
{
public:
    void Initialize(UEditorEngine* InEditorEngine) override;
    void Render(float DeltaTime) override;

private:
    void Refresh();
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
    void RenderGraphPreview();
    void RenderAgentPanel();
    void RenderValidationPanel();
    void RenderPIEControls();
    template<typename TComponent>
    TComponent* AddComponentToSelectedActor();
    void DrawAllDebugOnce();
    void SelectNode(UCombatCoverNodeComponent* Node);

private:
    TArray<UCombatCoverNodeComponent*> CachedNodes;
    TArray<UCombatCoverAgentComponent*> CachedAgents;
    UCombatFlowManagerComponent* CachedManager = nullptr;
    UCombatCoverNodeComponent* SelectedNode = nullptr;
    int32 SelectedSlotIndex = -1;
    int32 LinkTargetIndex = -1;
    float AutoLinkMaxDistance = 1500.0f;
    int32 AutoLinkMaxLinksPerNode = 2;
    bool bAutoLinkDirectedByX = true;
    bool bShowOnlySelectedNodeDebug = false;
    TArray<FString> LastValidationMessages;
};
