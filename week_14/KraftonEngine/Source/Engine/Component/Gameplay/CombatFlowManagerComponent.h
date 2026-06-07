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

struct FCombatAttackRuntimeState
{
    TWeakObjectPtr<UCombatCoverAgentComponent> Target;
    float TimeUntilNextAttack = 0.0f;
};

struct FCombatSuppressionRuntimeState
{
    int32 IncomingHitCount = 0;
    float TimeRemaining = 0.0f;
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

    UFUNCTION(Callable, Category="CombatFlow")
    bool TryRepositionNearby(UCombatCoverAgentComponent* Agent);

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
    const FCombatCoverSlot* FindCurrentSlot(const UCombatCoverAgentComponent* Agent) const;
    bool CanAgentAttackFromCurrentSlot(const UCombatCoverAgentComponent* Agent) const;
    bool CanAgentBeTargetedInCurrentSlot(const UCombatCoverAgentComponent* Agent) const;
    float GetTargetPriorityMultiplierForAgent(const UCombatCoverAgentComponent* Agent) const;

    void DrawAllDebugVisuals(bool bIncludeUnselected = true) const;
    void DrawCombatDebugVisuals(float Duration = 0.0f) const;

    UFUNCTION(Pure, Category="CombatFlow|Debug")
    bool GetDrawAllNodeDebugVisuals() const { return bDrawAllNodeDebugVisuals; }

    UFUNCTION(Callable, Category="CombatFlow|Debug")
    void SetDrawAllNodeDebugVisuals(bool bEnabled) { bDrawAllNodeDebugVisuals = bEnabled; }

    UFUNCTION(Callable, Category="CombatFlow|Combat")
    void UpdateCombatSimulation(float DeltaTime);

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

    UFUNCTION(Pure, Category="CombatFlow|Combat")
    bool GetEnableSuppression() const { return bEnableSuppression; }

    UFUNCTION(Callable, Category="CombatFlow|Combat")
    void SetEnableSuppression(bool bEnabled) { bEnableSuppression = bEnabled; }

    UFUNCTION(Pure, Category="CombatFlow|Combat")
    int32 GetSuppressionIncomingFireThreshold() const { return SuppressionIncomingFireThreshold; }

    UFUNCTION(Pure, Category="CombatFlow|Combat")
    float GetSuppressionDuration() const { return SuppressionDuration; }

    UFUNCTION(Pure, Category="CombatFlow|Combat")
    float GetSuppressionAccumulationWindow() const { return SuppressionAccumulationWindow; }

    static UCombatFlowManagerComponent* FindInWorld(UWorld* World);

private:
    FCombatCoverSlotHandle FindNearestFreeSlot(const FVector& WorldLocation, const FString& TeamTag, const UCombatCoverAgentComponent* RequestingAgent) const;
    FCombatCoverSlotHandle FindFreeSlotInNode(UCombatCoverNodeComponent* Node, const FString& TeamTag, const UCombatCoverAgentComponent* RequestingAgent) const;
    bool ReserveSlot(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& SlotHandle);
    void ReleaseAgentExcept(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& KeepSlotHandle);
    int32 CountNodeClaims(const UCombatCoverNodeComponent* Node, const UCombatCoverAgentComponent* IgnoreAgent) const;
    bool SlotTagsMatchTeam(const FCombatCoverSlot& Slot, const FString& TeamTag) const;
    void GatherAdvanceCandidateNodes(UCombatCoverAgentComponent* Agent, UCombatCoverNodeComponent* CurrentNode, TArray<UCombatCoverNodeComponent*>& OutNodes) const;
    bool BuildMovePathToSlot(const FCombatCoverSlotHandle& SlotHandle, FCombatMovePath& OutPath) const;
    bool BuildMovePathBetweenNodes(UCombatCoverNodeComponent* FromNode, UCombatCoverNodeComponent* ToNode, const FCombatCoverSlotHandle& StartSlot, const FCombatCoverSlotHandle& FinalSlot, FCombatMovePath& OutPath) const;
    bool AppendSlotApproachPoint(const FCombatCoverSlotHandle& SlotHandle, bool bForExit, TArray<FVector>& OutPoints) const;
    const FCombatCoverLink* FindTraversalLink(UCombatCoverNodeComponent* FromNode, UCombatCoverNodeComponent* ToNode, bool& bOutReverse) const;
    UCombatCoverAgentComponent* FindBestTargetFor(UCombatCoverAgentComponent* Agent) const;
    bool CanEngage(const UCombatCoverAgentComponent* Shooter, const UCombatCoverAgentComponent* Target) const;
    void DrawFireDebugLine(UCombatCoverAgentComponent* Shooter, UCombatCoverAgentComponent* Target, float Duration) const;
    void DrawFireRanges(float Duration) const;
    float PickAttackInterval(const UCombatCoverAgentComponent* Agent) const;
    float PickCoverHoldDuration(const UCombatCoverAgentComponent* Agent) const;
    void RemoveStaleAttackState();
    void EnsureRuntimeSlotsForNode(UCombatCoverNodeComponent* Node);
    void RemoveStaleRuntimeState();
    void RemoveInvalidOrDeadRuntimeClaims();
    void AddValidationMessage(FCombatCoverGraphValidationResult& Result, bool bError, const FString& Message) const;

private:
    UPROPERTY(Edit, Save, Category="CombatFlow", DisplayName="Require Slot Tag Match")
    bool bRequireSlotTagMatch = true;

    UPROPERTY(Edit, Save, Category="CombatFlow", DisplayName="Use Node Occupancy Limit")
    bool bUseNodeOccupancyLimit = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Combat", DisplayName="Enable Suppression")
    bool bEnableSuppression = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Combat", DisplayName="Suppression Incoming Fire Threshold", Min=1, Max=16, Speed=1)
    int32 SuppressionIncomingFireThreshold = 2;

    UPROPERTY(Edit, Save, Category="CombatFlow|Combat", DisplayName="Suppression Duration", Min=0.0f, Max=30.0f, Speed=0.1f)
    float SuppressionDuration = 1.5f;

    UPROPERTY(Edit, Save, Category="CombatFlow|Combat", DisplayName="Suppression Accumulation Window", Min=0.0f, Max=10.0f, Speed=0.1f)
    float SuppressionAccumulationWindow = 1.0f;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw Fire Debug Lines")
    bool bDrawFireDebugLines = true;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw Fire Ranges")
    bool bDrawFireRanges = false;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw All Node Debug Visuals")
    bool bDrawAllNodeDebugVisuals = false;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Draw Debug During Tick")
    bool bDrawDebugDuringTick = false;

    UPROPERTY(Edit, Save, Category="CombatFlow|Debug", DisplayName="Debug Draw Interval", Min=0.01f, Max=10.0f, Speed=0.01f)
    float DebugDrawInterval = 0.1f;

    TArray<UCombatCoverNodeComponent*> CachedNodes;
    TArray<UCombatCoverAgentComponent*> CachedAgents;
    TMap<FString, UCombatCoverNodeComponent*> NodeById;
    TMap<FString, FCombatNodeRuntimeState> RuntimeStateByNodeId;
    TMap<UCombatCoverAgentComponent*, FCombatAttackRuntimeState> AttackStateByAgent;
    TMap<UCombatCoverAgentComponent*, FCombatSuppressionRuntimeState> SuppressionStateByAgent;
    float DebugDrawTimer = 0.0f;

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;
};
