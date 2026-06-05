#pragma once

#include "Component/ActorComponent.h"
#include "Component/Gameplay/CombatCoverNodeComponent.h"
#include "Object/Ptr/WeakObjectPtr.h"

#include "Source/Engine/Component/Gameplay/CombatCoverAgentComponent.generated.h"

class UCombatFlowManagerComponent;

UENUM()
enum class ECombatCoverAgentState : uint8
{
    Idle,
    MovingToInitialSlot,
    InCover,
    MovingToLinkedNode,
    Blocked,
    Dead
};

UCLASS()
class UCombatCoverAgentComponent : public UActorComponent
{
public:
    GENERATED_BODY()

    UCombatCoverAgentComponent();
    ~UCombatCoverAgentComponent() override = default;

    void BeginPlay() override;
    void EndPlay() override;

    UFUNCTION(Callable, Category="CombatAgent")
    void RequestInitialSlot();

    UFUNCTION(Callable, Category="CombatAgent")
    void RequestAdvance();

    void MoveToReservedSlot(const FCombatCoverSlotHandle& SlotHandle, bool bInitialMove);

    UFUNCTION(Callable, Category="CombatAgent")
    void MarkDead();

    UFUNCTION(Pure, Category="CombatAgent")
    const FString& GetTeamTag() const { return TeamTag; }

    UFUNCTION(Pure, Category="CombatAgent")
    const FString& GetCurrentNodeId() const { return CurrentNodeId; }

    UFUNCTION(Pure, Category="CombatAgent")
    int32 GetCurrentSlotId() const { return CurrentSlotId; }

    UFUNCTION(Pure, Category="CombatAgent")
    const FString& GetTargetNodeId() const { return TargetNodeId; }

    UFUNCTION(Pure, Category="CombatAgent")
    int32 GetTargetSlotId() const { return TargetSlotId; }

    UFUNCTION(Pure, Category="CombatAgent")
    ECombatCoverAgentState GetState() const { return State; }

    const char* GetStateName() const;

private:
    UCombatFlowManagerComponent* ResolveManager();
    void TickMoveToTarget(float DeltaTime);
    void SetBlocked();

private:
    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Team Tag")
    FString TeamTag = "Enemy";

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Move Speed", Min=0.0f, Max=10000.0f, Speed=1.0f)
    float MoveSpeed = 250.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Acceptance Radius", Min=1.0f, Max=10000.0f, Speed=1.0f)
    float AcceptanceRadius = 80.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Advance Interval", Min=0.0f, Max=120.0f, Speed=0.1f)
    float AdvanceInterval = 3.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Retry Interval", Min=0.1f, Max=120.0f, Speed=0.1f)
    float RetryInterval = 1.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Auto Start")
    bool bAutoStart = true;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Use Character Movement")
    bool bUseCharacterMovement = true;

    ECombatCoverAgentState State = ECombatCoverAgentState::Idle;
    FString CurrentNodeId;
    int32 CurrentSlotId = -1;
    FString TargetNodeId;
    int32 TargetSlotId = -1;
    float AdvanceTimer = 0.0f;
    float RetryTimer = 0.0f;
    TWeakObjectPtr<UCombatFlowManagerComponent> CachedManager;

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;
};
