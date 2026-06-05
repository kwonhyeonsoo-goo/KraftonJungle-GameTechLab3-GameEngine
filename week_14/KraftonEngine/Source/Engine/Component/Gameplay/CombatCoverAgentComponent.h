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
    Engaging,
    Blocked,
    Dead
};

UENUM()
enum class ECombatAdvanceLinkMode : uint8
{
    OutgoingLinks,
    IncomingLinks,
    Both
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

    UFUNCTION(Pure, Category="CombatAgent")
    ECombatAdvanceLinkMode GetAdvanceLinkMode() const { return AdvanceLinkMode; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetHealth() const { return Health; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetFireRange() const { return FireRange; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetDamagePerSecond() const { return DamagePerSecond; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    int32 GetIncomingFireCount() const { return IncomingFireCount; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetIncomingDamagePerSecond() const { return IncomingDamagePerSecond; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    UCombatCoverAgentComponent* GetCurrentTarget() const { return CurrentTarget.Get(); }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool CanFireWhileMoving() const { return bCanFireWhileMoving; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool IsAlive() const { return State != ECombatCoverAgentState::Dead && Health > 0.0f; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool IsEngaging() const { return State == ECombatCoverAgentState::Engaging || CurrentTarget.Get() != nullptr; }

    const char* GetStateName() const;
    const char* GetAdvanceLinkModeName() const;

    void SetEngagementTarget(UCombatCoverAgentComponent* Target);
    void ClearEngagementTarget();
    void ApplyDamage(float Damage);
    void SetIncomingFireStats(int32 Count, float DamagePerSecond);

private:
    UCombatFlowManagerComponent* ResolveManager();
    void TickMoveToTarget(float DeltaTime);
    void SetBlocked();

private:
    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Team Tag")
    FString TeamTag = "Enemy";

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Advance Link Mode", Enum=ECombatAdvanceLinkMode)
    ECombatAdvanceLinkMode AdvanceLinkMode = ECombatAdvanceLinkMode::OutgoingLinks;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Move Speed", Min=0.0f, Max=10000.0f, Speed=1.0f)
    float MoveSpeed = 10.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Acceptance Radius", Min=1.0f, Max=10000.0f, Speed=1.0f)
    float AcceptanceRadius = 3.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Advance Interval", Min=0.0f, Max=120.0f, Speed=0.1f)
    float AdvanceInterval = 3.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Retry Interval", Min=0.1f, Max=120.0f, Speed=0.1f)
    float RetryInterval = 1.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Auto Start")
    bool bAutoStart = true;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Use Character Movement")
    bool bUseCharacterMovement = true;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Max Health", Min=1.0f, Max=100000.0f, Speed=1.0f)
    float MaxHealth = 100.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Health", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float Health = 100.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Fire Range", Min=0.0f, Max=100000.0f, Speed=10.0f)
    float FireRange = 1200.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Damage Per Second", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float DamagePerSecond = 10.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Target Scan Interval", Min=0.01f, Max=10.0f, Speed=0.01f)
    float TargetScanInterval = 0.2f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Can Fire While Moving")
    bool bCanFireWhileMoving = false;

    ECombatCoverAgentState State = ECombatCoverAgentState::Idle;
    FString CurrentNodeId;
    int32 CurrentSlotId = -1;
    FString TargetNodeId;
    int32 TargetSlotId = -1;
    float AdvanceTimer = 0.0f;
    float RetryTimer = 0.0f;
    float TargetScanTimer = 0.0f;
    int32 IncomingFireCount = 0;
    float IncomingDamagePerSecond = 0.0f;
    ECombatCoverAgentState StateBeforeEngage = ECombatCoverAgentState::Idle;
    TWeakObjectPtr<UCombatCoverAgentComponent> CurrentTarget;
    TWeakObjectPtr<UCombatFlowManagerComponent> CachedManager;

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;
};
