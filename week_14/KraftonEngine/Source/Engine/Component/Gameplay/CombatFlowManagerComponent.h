#pragma once

#include "Component/ActorComponent.h"
#include "Component/Gameplay/CombatCoverNodeComponent.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/Component/Gameplay/CombatFlowManagerComponent.generated.h"

class UCombatCoverAgentComponent;
class UCombatCoverNodeComponent;
class UWorld;

struct FCombatSlotRuntimeState
{
    TWeakObjectPtr<UCombatCoverAgentComponent> ReservedBy;
    TWeakObjectPtr<UCombatCoverAgentComponent> OccupiedBy;
};

struct FCombatNodeRuntimeState
{
    TMap<int32, FCombatSlotRuntimeState> Slots;
};

struct FCombatCoverGraphValidationResult
{
    int32 NodeCount = 0;
    int32 SlotCount = 0;
    int32 LinkCount = 0;
    int32 ErrorCount = 0;
    int32 WarningCount = 0;
    TArray<FString> Messages;
};

UCLASS()
class UCombatFlowManagerComponent : public UActorComponent
{
public:
    GENERATED_BODY()

    UCombatFlowManagerComponent();
    ~UCombatFlowManagerComponent() override = default;

    void BeginPlay() override;
    void EndPlay() override;

    UFUNCTION(Callable, Category="CombatFlow")
    void RefreshRegistry();

    UFUNCTION(Callable, Category="CombatFlow")
    void ResetRuntimeState();

    UFUNCTION(Callable, Category="CombatFlow")
    bool AssignInitialSlot(UCombatCoverAgentComponent* Agent);

    UFUNCTION(Callable, Category="CombatFlow")
    bool TryAdvance(UCombatCoverAgentComponent* Agent);

    void ConfirmArrived(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& SlotHandle);

    UFUNCTION(Callable, Category="CombatFlow")
    void ReleaseAgent(UCombatCoverAgentComponent* Agent);

    UFUNCTION(Callable, Category="CombatFlow")
    int32 AutoGenerateMissingNodeIds();

    UFUNCTION(Callable, Category="CombatFlow")
    int32 AutoLinkNearby(float MaxDistance, int32 MaxLinksPerNode, bool bDirectedByX = true);

    FCombatCoverGraphValidationResult ValidateGraph(bool bLogToConsole = true);

    UCombatCoverNodeComponent* FindNode(const FString& NodeId) const;
    const TArray<UCombatCoverNodeComponent*>& GetNodes() const { return CachedNodes; }
    const TArray<UCombatCoverAgentComponent*>& GetAgents() const { return CachedAgents; }

    bool IsSlotFree(const FCombatCoverSlotHandle& SlotHandle, const UCombatCoverAgentComponent* RequestingAgent = nullptr) const;
    bool IsNodeOccupiedOrReserved(const UCombatCoverNodeComponent* Node, const UCombatCoverAgentComponent* RequestingAgent = nullptr) const;

    void DrawAllDebugVisuals(bool bIncludeUnselected = true) const;

    UFUNCTION(Callable, Category="CombatFlow|Combat")
    void UpdateCombatSimulation(float DeltaTime);

    UFUNCTION(Pure, Category="CombatFlow|Combat")
    bool GetEnableCombatSimulation() const { return bEnableCombatSimulation; }

    UFUNCTION(Callable, Category="CombatFlow|Combat")
    void SetEnableCombatSimulation(bool bEnabled) { bEnableCombatSimulation = bEnabled; }

    UFUNCTION(Pure, Category="CombatFlow|Combat")
    bool GetRequireMutualFireRange() const { return bRequireMutualFireRange; }

    UFUNCTION(Callable, Category="CombatFlow|Combat")
    void SetRequireMutualFireRange(bool bRequired) { bRequireMutualFireRange = bRequired; }

    UFUNCTION(Pure, Category="CombatFlow")
    bool GetRequireSlotTagMatch() const { return bRequireSlotTagMatch; }

    UFUNCTION(Callable, Category="CombatFlow")
    void SetRequireSlotTagMatch(bool bRequired) { bRequireSlotTagMatch = bRequired; }

    UFUNCTION(Pure, Category="CombatFlow|Debug")
    bool GetDrawFireDebugLines() const { return bDrawFireDebugLines; }

    UFUNCTION(Callable, Category="CombatFlow|Debug")
    void SetDrawFireDebugLines(bool bEnabled) { bDrawFireDebugLines = bEnabled; }

    UFUNCTION(Pure, Category="CombatFlow|Debug")
    bool GetDrawFireRanges() const { return bDrawFireRanges; }

    UFUNCTION(Callable, Category="CombatFlow|Debug")
    void SetDrawFireRanges(bool bEnabled) { bDrawFireRanges = bEnabled; }

    static UCombatFlowManagerComponent* FindInWorld(UWorld* World);

private:
    FCombatCoverSlotHandle FindNearestFreeSlot(const FVector& WorldLocation, const FString& TeamTag, const UCombatCoverAgentComponent* RequestingAgent) const;
    FCombatCoverSlotHandle FindFreeSlotInNode(UCombatCoverNodeComponent* Node, const FString& TeamTag, const UCombatCoverAgentComponent* RequestingAgent) const;
    bool ReserveSlot(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& SlotHandle);
    void ReleaseAgentExcept(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& KeepSlotHandle);
    int32 CountNodeClaims(const UCombatCoverNodeComponent* Node, const UCombatCoverAgentComponent* IgnoreAgent) const;
    bool SlotTagsMatchTeam(const FCombatCoverSlot& Slot, const FString& TeamTag) const;
    void GatherAdvanceCandidateNodes(UCombatCoverAgentComponent* Agent, UCombatCoverNodeComponent* CurrentNode, TArray<UCombatCoverNodeComponent*>& OutNodes) const;
    UCombatCoverAgentComponent* FindBestTargetFor(UCombatCoverAgentComponent* Agent) const;
    bool CanEngage(const UCombatCoverAgentComponent* Shooter, const UCombatCoverAgentComponent* Target) const;
    void DrawFireDebugLines() const;
    void DrawFireRanges() const;
    void EnsureRuntimeSlotsForNode(UCombatCoverNodeComponent* Node);
    void RemoveStaleRuntimeState();
    void AddValidationMessage(FCombatCoverGraphValidationResult& Result, bool bError, const FString& Message) const;

private:
    UPROPERTY(Edit, Save, Category="CombatFlow", DisplayName="Require Slot Tag Match")
    bool bRequireSlotTagMatch = true;

    UPROPERTY(Edit, Save, Category="CombatFlow", DisplayName="Use Node Occupancy Limit")
    bool bUseNodeOccupancyLimit = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Combat", DisplayName="Enable Combat Simulation")
    bool bEnableCombatSimulation = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Combat", DisplayName="Require Mutual Fire Range")
    bool bRequireMutualFireRange = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw Fire Debug Lines")
    bool bDrawFireDebugLines = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw Fire Ranges")
    bool bDrawFireRanges = false;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw Debug During Tick")
    bool bDrawDebugDuringTick = false;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Debug Draw Interval", Min=0.01f, Max=10.0f, Speed=0.01f)
    float DebugDrawInterval = 0.1f;

    TArray<UCombatCoverNodeComponent*> CachedNodes;
    TArray<UCombatCoverAgentComponent*> CachedAgents;
    TMap<FString, UCombatCoverNodeComponent*> NodeById;
    TMap<FString, FCombatNodeRuntimeState> RuntimeStateByNodeId;
    float DebugDrawTimer = 0.0f;

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;
};
