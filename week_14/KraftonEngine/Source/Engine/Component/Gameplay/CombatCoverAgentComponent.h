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
    Suppressed,
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

UENUM()
enum class ECombatAgentRole : uint8
{
    AutoFromTeam,
    Ally,
    EnemyShortRange,
    EnemyLongRangeSlow
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
    void PostEditProperty(const char* PropertyName) override;

    UFUNCTION(Callable, Category="CombatAgent")
    void RequestInitialSlot();

    UFUNCTION(Callable, Category="CombatAgent")
    void RequestAdvance();

    void MoveToReservedSlot(const FCombatMovePath& MovePath, bool bInitialMove);

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

    UFUNCTION(Pure, Category="CombatAgent")
    ECombatAgentRole GetCombatRole() const { return CombatRole; }

    UFUNCTION(Pure, Category="CombatAgent")
    ECombatAgentRole GetResolvedCombatRole() const;

    UFUNCTION(Pure, Category="CombatAgent")
    bool UsesRoleCombatDefaults() const { return bUseRoleCombatDefaults; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetHealth() const { return Health; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetFireRange() const { return FireRange; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetMovingFireRange() const { return MovingFireRange; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool UsesMovingFireRange() const { return bUseMovingFireRange; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool IsMovingForCombatRange() const;

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetEffectiveFireRange() const;

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetAttackDamage() const { return AttackDamage; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetAttackIntervalMin() const { return AttackIntervalMin; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetAttackIntervalMax() const { return AttackIntervalMax; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    int32 GetIncomingFireCount() const { return IncomingFireCount; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetIncomingAttackDamage() const { return IncomingAttackDamage; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    UCombatCoverAgentComponent* GetCurrentTarget() const { return CurrentTarget.Get(); }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool CanFireWhileMoving() const { return bCanFireWhileMoving; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool IsSuppressed() const { return State == ECombatCoverAgentState::Suppressed; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetSuppressionTimeRemaining() const { return SuppressionTimer; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    float GetDeathDebugScaleMultiplier() const { return DeathDebugScaleMultiplier; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool IsAlive() const { return State != ECombatCoverAgentState::Dead && Health > 0.0f; }

    UFUNCTION(Pure, Category="CombatAgent|Combat")
    bool IsEngaging() const { return State == ECombatCoverAgentState::Engaging || CurrentTarget.Get() != nullptr; }

    const char* GetStateName() const;
    const char* GetAdvanceLinkModeName() const;
    const char* GetCombatRoleName() const;
    const char* GetResolvedCombatRoleName() const;

    void SetEngagementTarget(UCombatCoverAgentComponent* Target);
    void ClearEngagementTarget();
    void ApplyDamage(float Damage);
    void ApplySuppression(float Duration);
    void SetIncomingFireStats(int32 Count, float AttackDamage);

private:
    UCombatFlowManagerComponent* ResolveManager();
    void ApplyCombatRoleDefaults();
    void FinishSuppression();
    void TickMoveToTarget(float DeltaTime);
    void FaceDirection2D(const FVector& Direction, float DeltaTime);
    void FaceLocation2D(const FVector& WorldLocation, float DeltaTime);
    void TickFaceCombatTarget(float DeltaTime);
    void SetBlocked();

private:
    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Team Tag")
    FString TeamTag = "Enemy";

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Advance Link Mode", Enum=ECombatAdvanceLinkMode)
    ECombatAdvanceLinkMode AdvanceLinkMode = ECombatAdvanceLinkMode::OutgoingLinks;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Combat Role", Enum=ECombatAgentRole)
    ECombatAgentRole CombatRole = ECombatAgentRole::AutoFromTeam;

    UPROPERTY(Edit, Save, Category="CombatAgent", DisplayName="Use Role Combat Defaults")
    bool bUseRoleCombatDefaults = true;

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

    UPROPERTY(Edit, Save, Category="CombatAgent|Facing", DisplayName="Orient To Combat Direction")
    bool bOrientToCombatDirection = true;

    UPROPERTY(Edit, Save, Category="CombatAgent|Facing", DisplayName="Facing Yaw Rate", Min=0.0f, Max=3600.0f, Speed=5.0f)
    float FacingYawRate = 720.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Facing", DisplayName="Facing Yaw Offset", Min=-180.0f, Max=180.0f, Speed=1.0f)
    float FacingYawOffset = 0.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Max Health", Min=1.0f, Max=100000.0f, Speed=1.0f)
    float MaxHealth = 100.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Health", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float Health = 100.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Fire Range", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float FireRange = 50.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Moving Fire Range", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float MovingFireRange = 30.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Use Moving Fire Range")
    bool bUseMovingFireRange = true;

    UPROPERTY(Edit, Save, Category="CombatAgent|Debug", DisplayName="Shrink Actor On Death")
    bool bShrinkActorOnDeath = true;

    UPROPERTY(Edit, Save, Category="CombatAgent|Debug", DisplayName="Death Debug Scale Multiplier", Min=0.01f, Max=1.0f, Speed=0.01f)
    float DeathDebugScaleMultiplier = 0.1f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Attack Damage", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float AttackDamage = 5.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Attack Interval Min", Min=0.0f, Max=120.0f, Speed=0.1f)
    float AttackIntervalMin = 1.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Attack Interval Max", Min=0.0f, Max=120.0f, Speed=0.1f)
    float AttackIntervalMax = 2.0f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Target Scan Interval", Min=0.01f, Max=10.0f, Speed=0.01f)
    float TargetScanInterval = 0.2f;

    UPROPERTY(Edit, Save, Category="CombatAgent|Combat", DisplayName="Can Fire While Moving")
    bool bCanFireWhileMoving = false;

    ECombatCoverAgentState State = ECombatCoverAgentState::Idle;
    FString CurrentNodeId;
    int32 CurrentSlotId = -1;
    FString TargetNodeId;
    int32 TargetSlotId = -1;
    TArray<FVector> CurrentMovePath;
    int32 CurrentMovePathIndex = 0;
    FCombatCoverSlotHandle FinalReservedSlot;
    float AdvanceTimer = 0.0f;
    float RetryTimer = 0.0f;
    float TargetScanTimer = 0.0f;
    int32 IncomingFireCount = 0;
    float IncomingAttackDamage = 0.0f;
    float SuppressionTimer = 0.0f;
    ECombatCoverAgentState StateBeforeEngage = ECombatCoverAgentState::Idle;
    ECombatCoverAgentState StateBeforeSuppressed = ECombatCoverAgentState::Idle;
    FVector InitialActorScale = FVector(1.0f, 1.0f, 1.0f);
    bool bHasInitialActorScale = false;
    bool bDeathDebugScaleApplied = false;
    TWeakObjectPtr<UCombatCoverAgentComponent> CurrentTarget;
    TWeakObjectPtr<UCombatFlowManagerComponent> CachedManager;

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;
};
