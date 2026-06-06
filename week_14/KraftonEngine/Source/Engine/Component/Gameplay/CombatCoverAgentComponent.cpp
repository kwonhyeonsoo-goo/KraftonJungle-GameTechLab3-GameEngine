#include "CombatCoverAgentComponent.h"

#include "Component/Gameplay/CombatFlowManagerComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/Character.h"

#include <algorithm>
#include <cmath>

namespace
{
    float Distance2D(const FVector& A, const FVector& B)
    {
        const float DX = A.X - B.X;
        const float DY = A.Y - B.Y;
        return sqrtf(DX * DX + DY * DY);
    }

    float NormalizeYawDelta(float DeltaYaw)
    {
        while (DeltaYaw > 180.0f)
        {
            DeltaYaw -= 360.0f;
        }
        while (DeltaYaw < -180.0f)
        {
            DeltaYaw += 360.0f;
        }
        return DeltaYaw;
    }
}

UCombatCoverAgentComponent::UCombatCoverAgentComponent()
{
    PrimaryComponentTick.SetTickGroup(TG_PrePhysics);
    SetComponentTickEnabled(true);
}

void UCombatCoverAgentComponent::BeginPlay()
{
    UActorComponent::BeginPlay();
    State = ECombatCoverAgentState::Idle;
    CurrentNodeId.clear();
    CurrentSlotId = -1;
    TargetNodeId.clear();
    TargetSlotId = -1;
    CurrentMovePath.clear();
    CurrentMovePathIndex = 0;
    FinalReservedSlot.Reset();
    ApplyCombatRoleDefaults();
    MaxHealth = (std::max)(1.0f, MaxHealth);
    Health = (std::min)((std::max)(0.0f, Health), MaxHealth);
    FireRange = (std::max)(0.0f, FireRange);
    MovingFireRange = (std::max)(0.0f, MovingFireRange);
    FacingYawRate = (std::max)(0.0f, FacingYawRate);
    FacingYawOffset = NormalizeYawDelta(FacingYawOffset);
    DeathDebugScaleMultiplier = (std::min)((std::max)(0.01f, DeathDebugScaleMultiplier), 1.0f);
    if (AActor* Owner = GetOwner())
    {
        InitialActorScale = Owner->GetActorScale();
        bHasInitialActorScale = true;
    }
    else
    {
        InitialActorScale = FVector(1.0f, 1.0f, 1.0f);
        bHasInitialActorScale = false;
    }
    bDeathDebugScaleApplied = false;
    AdvanceTimer = 0.0f;
    RetryTimer = 0.0f;
    TargetScanTimer = 0.0f;
    IncomingFireCount = 0;
    IncomingAttackDamage = 0.0f;
    SuppressionTimer = 0.0f;
    StateBeforeEngage = ECombatCoverAgentState::Idle;
    StateBeforeSuppressed = ECombatCoverAgentState::Idle;
    CurrentTarget.Reset();
    ResolveManager();
}

void UCombatCoverAgentComponent::EndPlay()
{
    if (UCombatFlowManagerComponent* Manager = ResolveManager())
    {
        Manager->ReleaseAgent(this);
    }
    CachedManager.Reset();
    UActorComponent::EndPlay();
}

void UCombatCoverAgentComponent::PostEditProperty(const char* PropertyName)
{
    UActorComponent::PostEditProperty(PropertyName);

    (void)PropertyName;
    ApplyCombatRoleDefaults();
    MaxHealth = (std::max)(1.0f, MaxHealth);
    Health = (std::min)((std::max)(0.0f, Health), MaxHealth);
    FireRange = (std::max)(0.0f, FireRange);
    MovingFireRange = (std::max)(0.0f, MovingFireRange);
    FacingYawRate = (std::max)(0.0f, FacingYawRate);
    FacingYawOffset = NormalizeYawDelta(FacingYawOffset);
    AttackDamage = (std::max)(0.0f, AttackDamage);
    AttackIntervalMin = (std::max)(0.0f, AttackIntervalMin);
    AttackIntervalMax = (std::max)(AttackIntervalMin, AttackIntervalMax);
    DeathDebugScaleMultiplier = (std::min)((std::max)(0.01f, DeathDebugScaleMultiplier), 1.0f);
}

void UCombatCoverAgentComponent::RequestInitialSlot()
{
    if (State == ECombatCoverAgentState::Dead || State == ECombatCoverAgentState::Suppressed || (State == ECombatCoverAgentState::Engaging && !bCanFireWhileMoving))
    {
        return;
    }

    UCombatFlowManagerComponent* Manager = ResolveManager();
    if (!Manager || !Manager->AssignInitialSlot(this))
    {
        SetBlocked();
    }
}

void UCombatCoverAgentComponent::RequestAdvance()
{
    if (State == ECombatCoverAgentState::Dead || State == ECombatCoverAgentState::Suppressed || (State == ECombatCoverAgentState::Engaging && !bCanFireWhileMoving))
    {
        return;
    }

    UCombatFlowManagerComponent* Manager = ResolveManager();
    if (!Manager || !Manager->TryAdvance(this))
    {
        SetBlocked();
    }
}

void UCombatCoverAgentComponent::MoveToReservedSlot(const FCombatMovePath& MovePath, bool bInitialMove)
{
    if (!MovePath.IsValid())
    {
        SetBlocked();
        return;
    }

    FinalReservedSlot = MovePath.FinalSlot;
    TargetNodeId = MovePath.FinalSlot.NodeId;
    TargetSlotId = MovePath.FinalSlot.SlotId;
    CurrentMovePath = MovePath.Points;
    CurrentMovePathIndex = 0;
    AdvanceTimer = 0.0f;
    RetryTimer = 0.0f;
    State = bInitialMove ? ECombatCoverAgentState::MovingToInitialSlot : ECombatCoverAgentState::MovingToLinkedNode;
}

void UCombatCoverAgentComponent::MarkDead()
{
    if (State == ECombatCoverAgentState::Dead && bDeathDebugScaleApplied)
    {
        if (UCombatFlowManagerComponent* Manager = ResolveManager())
        {
            Manager->ReleaseAgent(this);
        }
        return;
    }

    CurrentTarget.Reset();
    IncomingFireCount = 0;
    IncomingAttackDamage = 0.0f;
    SuppressionTimer = 0.0f;
    Health = 0.0f;

    if (UCombatFlowManagerComponent* Manager = ResolveManager())
    {
        Manager->ReleaseAgent(this);
    }

    if (bShrinkActorOnDeath && !bDeathDebugScaleApplied)
    {
        if (AActor* Owner = GetOwner())
        {
            const FVector BaseScale = bHasInitialActorScale ? InitialActorScale : Owner->GetActorScale();
            Owner->SetActorScale(BaseScale * DeathDebugScaleMultiplier);
            bDeathDebugScaleApplied = true;
        }
    }

    State = ECombatCoverAgentState::Dead;
    StateBeforeEngage = ECombatCoverAgentState::Dead;
    StateBeforeSuppressed = ECombatCoverAgentState::Dead;
    CurrentNodeId.clear();
    CurrentSlotId = -1;
    TargetNodeId.clear();
    TargetSlotId = -1;
    CurrentMovePath.clear();
    CurrentMovePathIndex = 0;
    FinalReservedSlot.Reset();
}


ECombatAgentRole UCombatCoverAgentComponent::GetResolvedCombatRole() const
{
    if (CombatRole != ECombatAgentRole::AutoFromTeam)
    {
        return CombatRole;
    }

    if (TeamTag.find("Ally") != FString::npos)
    {
        return ECombatAgentRole::Ally;
    }

    return ECombatAgentRole::EnemyShortRange;
}

void UCombatCoverAgentComponent::ApplyCombatRoleDefaults()
{
    if (!bUseRoleCombatDefaults)
    {
        return;
    }

    switch (GetResolvedCombatRole())
    {
    case ECombatAgentRole::Ally:
        TeamTag = "Ally";
        AdvanceLinkMode = ECombatAdvanceLinkMode::OutgoingLinks;
        FireRange = 50.0f;
        MovingFireRange = 30.0f;
        AttackDamage = 5.0f;
        AttackIntervalMin = 1.0f;
        AttackIntervalMax = 2.0f;
        break;

    case ECombatAgentRole::EnemyLongRangeSlow:
        TeamTag = "Enemy";
        AdvanceLinkMode = ECombatAdvanceLinkMode::IncomingLinks;
        FireRange = 80.0f;
        MovingFireRange = 30.0f;
        AttackDamage = 7.0f;
        AttackIntervalMin = 2.4f;
        AttackIntervalMax = 3.6f;
        break;

    case ECombatAgentRole::EnemyShortRange:
    case ECombatAgentRole::AutoFromTeam:
    default:
        TeamTag = "Enemy";
        AdvanceLinkMode = ECombatAdvanceLinkMode::IncomingLinks;
        FireRange = 35.0f;
        MovingFireRange = 25.0f;
        AttackDamage = 5.0f;
        AttackIntervalMin = 0.8f;
        AttackIntervalMax = 1.4f;
        break;
    }

    bUseMovingFireRange = true;
    bCanFireWhileMoving = false;
}

bool UCombatCoverAgentComponent::IsMovingForCombatRange() const
{
    return State == ECombatCoverAgentState::MovingToInitialSlot ||
        State == ECombatCoverAgentState::MovingToLinkedNode;
}

float UCombatCoverAgentComponent::GetEffectiveFireRange() const
{
    if (bUseMovingFireRange && IsMovingForCombatRange())
    {
        return (std::max)(0.0f, MovingFireRange);
    }

    return (std::max)(0.0f, FireRange);
}

const char* UCombatCoverAgentComponent::GetStateName() const
{
    switch (State)
    {
    case ECombatCoverAgentState::Idle: return "Idle";
    case ECombatCoverAgentState::MovingToInitialSlot: return "MovingToInitialSlot";
    case ECombatCoverAgentState::InCover: return "InCover";
    case ECombatCoverAgentState::MovingToLinkedNode: return "MovingToLinkedNode";
    case ECombatCoverAgentState::Engaging: return "Engaging";
    case ECombatCoverAgentState::Suppressed: return "Suppressed";
    case ECombatCoverAgentState::Blocked: return "Blocked";
    case ECombatCoverAgentState::Dead: return "Dead";
    default: return "Unknown";
    }
}

const char* UCombatCoverAgentComponent::GetAdvanceLinkModeName() const
{
    switch (AdvanceLinkMode)
    {
    case ECombatAdvanceLinkMode::OutgoingLinks: return "OutgoingLinks";
    case ECombatAdvanceLinkMode::IncomingLinks: return "IncomingLinks";
    case ECombatAdvanceLinkMode::Both: return "Both";
    default: return "Unknown";
    }
}


const char* UCombatCoverAgentComponent::GetCombatRoleName() const
{
    switch (CombatRole)
    {
    case ECombatAgentRole::AutoFromTeam: return "AutoFromTeam";
    case ECombatAgentRole::Ally: return "Ally";
    case ECombatAgentRole::EnemyShortRange: return "EnemyShortRange";
    case ECombatAgentRole::EnemyLongRangeSlow: return "EnemyLongRangeSlow";
    default: return "Unknown";
    }
}

const char* UCombatCoverAgentComponent::GetResolvedCombatRoleName() const
{
    switch (GetResolvedCombatRole())
    {
    case ECombatAgentRole::Ally: return "Ally";
    case ECombatAgentRole::EnemyShortRange: return "EnemyShortRange";
    case ECombatAgentRole::EnemyLongRangeSlow: return "EnemyLongRangeSlow";
    case ECombatAgentRole::AutoFromTeam: return "AutoFromTeam";
    default: return "Unknown";
    }
}

void UCombatCoverAgentComponent::SetEngagementTarget(UCombatCoverAgentComponent* Target)
{
    if (State == ECombatCoverAgentState::Dead || State == ECombatCoverAgentState::Suppressed)
    {
        return;
    }

    if (!IsValid(Target) || Target == this || !Target->IsAlive())
    {
        ClearEngagementTarget();
        return;
    }

    CurrentTarget.Reset(Target);

    if (!bCanFireWhileMoving && State != ECombatCoverAgentState::Engaging)
    {
        StateBeforeEngage = State;
        State = ECombatCoverAgentState::Engaging;
    }
}

void UCombatCoverAgentComponent::ClearEngagementTarget()
{
    const bool bHadTarget = CurrentTarget.Get() != nullptr;
    CurrentTarget.Reset();

    if (State != ECombatCoverAgentState::Engaging)
    {
        return;
    }

    switch (StateBeforeEngage)
    {
    case ECombatCoverAgentState::MovingToInitialSlot:
    case ECombatCoverAgentState::MovingToLinkedNode:
        if (!TargetNodeId.empty() && TargetSlotId >= 0)
        {
            State = StateBeforeEngage;
        }
        else
        {
            State = CurrentNodeId.empty() ? ECombatCoverAgentState::Idle : ECombatCoverAgentState::InCover;
        }
        break;

    case ECombatCoverAgentState::Suppressed:
        State = ECombatCoverAgentState::Suppressed;
        break;

    case ECombatCoverAgentState::Blocked:
        State = ECombatCoverAgentState::Blocked;
        break;

    case ECombatCoverAgentState::Dead:
        State = ECombatCoverAgentState::Dead;
        break;

    case ECombatCoverAgentState::Idle:
        State = CurrentNodeId.empty() ? ECombatCoverAgentState::Idle : ECombatCoverAgentState::InCover;
        break;

    case ECombatCoverAgentState::InCover:
    case ECombatCoverAgentState::Engaging:
    default:
        State = ECombatCoverAgentState::InCover;
        break;
    }

    if (bHadTarget)
    {
        AdvanceTimer = 0.0f;
        RetryTimer = 0.0f;
    }
    StateBeforeEngage = ECombatCoverAgentState::Idle;
}

void UCombatCoverAgentComponent::ApplyDamage(float Damage)
{
    if (State == ECombatCoverAgentState::Dead || Damage <= 0.0f)
    {
        return;
    }

    Health = (std::max)(0.0f, Health - Damage);
    if (Health <= 0.0f)
    {
        MarkDead();
    }
}


void UCombatCoverAgentComponent::ApplySuppression(float Duration)
{
    if (State == ECombatCoverAgentState::Dead || Duration <= 0.0f)
    {
        return;
    }

    CurrentTarget.Reset();
    AdvanceTimer = 0.0f;
    RetryTimer = 0.0f;

    if (State != ECombatCoverAgentState::Suppressed)
    {
        StateBeforeSuppressed = State;
        State = ECombatCoverAgentState::Suppressed;
    }

    SuppressionTimer = (std::max)(SuppressionTimer, Duration);
}

void UCombatCoverAgentComponent::FinishSuppression()
{
    SuppressionTimer = 0.0f;

    switch (StateBeforeSuppressed)
    {
    case ECombatCoverAgentState::MovingToInitialSlot:
    case ECombatCoverAgentState::MovingToLinkedNode:
        if (!TargetNodeId.empty() && TargetSlotId >= 0)
        {
            State = StateBeforeSuppressed;
        }
        else
        {
            State = CurrentNodeId.empty() ? ECombatCoverAgentState::Idle : ECombatCoverAgentState::InCover;
        }
        break;

    case ECombatCoverAgentState::Blocked:
        State = ECombatCoverAgentState::Blocked;
        break;

    case ECombatCoverAgentState::Dead:
        State = ECombatCoverAgentState::Dead;
        break;

    case ECombatCoverAgentState::Idle:
        State = CurrentNodeId.empty() ? ECombatCoverAgentState::Idle : ECombatCoverAgentState::InCover;
        break;

    case ECombatCoverAgentState::Engaging:
    case ECombatCoverAgentState::Suppressed:
    case ECombatCoverAgentState::InCover:
    default:
        State = CurrentNodeId.empty() ? ECombatCoverAgentState::Idle : ECombatCoverAgentState::InCover;
        break;
    }

    StateBeforeSuppressed = ECombatCoverAgentState::Idle;
}

void UCombatCoverAgentComponent::SetIncomingFireStats(int32 Count, float AttackDamage)
{
    IncomingFireCount = (std::max)(0, Count);
    IncomingAttackDamage = (std::max)(0.0f, AttackDamage);
}


UCombatFlowManagerComponent* UCombatCoverAgentComponent::ResolveManager()
{
    if (UCombatFlowManagerComponent* Manager = CachedManager.Get())
    {
        return Manager;
    }

    UCombatFlowManagerComponent* FoundManager = UCombatFlowManagerComponent::FindInWorld(GetWorld());
    if (FoundManager)
    {
        CachedManager.Reset(FoundManager);
    }
    return FoundManager;
}

void UCombatCoverAgentComponent::TickMoveToTarget(float DeltaTime)
{
    AActor* Owner = GetOwner();
    UCombatFlowManagerComponent* Manager = ResolveManager();
    if (!Owner || !Manager)
    {
        SetBlocked();
        return;
    }

    UCombatCoverNodeComponent* TargetNode = Manager->FindNode(TargetNodeId);
    if (!TargetNode)
    {
        SetBlocked();
        return;
    }

    const int32 SlotIndex = TargetNode->FindSlotIndexById(TargetSlotId);
    if (SlotIndex < 0)
    {
        SetBlocked();
        return;
    }

    if (CurrentMovePath.empty())
    {
        CurrentMovePath.push_back(TargetNode->GetSlotWorldPosition(SlotIndex));
        CurrentMovePathIndex = 0;
    }

    if (CurrentMovePathIndex < 0)
    {
        CurrentMovePathIndex = 0;
    }

    auto FinishMove = [this, Manager]()
    {
        FCombatCoverSlotHandle ArrivedSlot = FinalReservedSlot;
        if (!ArrivedSlot.IsValid())
        {
            ArrivedSlot.NodeId = TargetNodeId;
            ArrivedSlot.SlotId = TargetSlotId;
        }

        Manager->ConfirmArrived(this, ArrivedSlot);

        CurrentNodeId = ArrivedSlot.NodeId;
        CurrentSlotId = ArrivedSlot.SlotId;
        TargetNodeId.clear();
        TargetSlotId = -1;
        CurrentMovePath.clear();
        CurrentMovePathIndex = 0;
        FinalReservedSlot.Reset();
        AdvanceTimer = 0.0f;
        RetryTimer = 0.0f;
        State = ECombatCoverAgentState::InCover;
    };

    if (CurrentMovePathIndex >= static_cast<int32>(CurrentMovePath.size()))
    {
        FinishMove();
        return;
    }

    const FVector TargetLocation = CurrentMovePath[CurrentMovePathIndex];
    FVector CurrentLocation = Owner->GetActorLocation();
    FVector Delta = TargetLocation - CurrentLocation;
    Delta.Z = 0.0f;

    const float Distance = Delta.Length();
    if (Distance <= AcceptanceRadius)
    {
        ++CurrentMovePathIndex;
        if (CurrentMovePathIndex >= static_cast<int32>(CurrentMovePath.size()))
        {
            FinishMove();
        }
        return;
    }

    if (Delta.IsNearlyZero())
    {
        return;
    }

    const FVector Direction = Delta.Normalized();
    if (!CurrentTarget.Get())
    {
        FaceDirection2D(Direction, DeltaTime);
    }

    if (bUseCharacterMovement)
    {
        if (ACharacter* Character = Cast<ACharacter>(Owner))
        {
            Character->AddMovementInput(Direction, 1.0f);
            return;
        }
    }

    const float Step = (std::max)(0.0f, MoveSpeed) * DeltaTime;
    if (Step <= 0.0f)
    {
        return;
    }

    if (Step >= Distance)
    {
        Owner->SetActorLocation(FVector(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z));
    }
    else
    {
        Owner->SetActorLocation(CurrentLocation + Direction * Step);
    }
}

void UCombatCoverAgentComponent::FaceDirection2D(const FVector& Direction, float DeltaTime)
{
    if (!bOrientToCombatDirection)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FVector FlatDirection = Direction;
    FlatDirection.Z = 0.0f;

    const float LengthSq = FlatDirection.X * FlatDirection.X + FlatDirection.Y * FlatDirection.Y;
    if (LengthSq <= 1e-6f)
    {
        return;
    }

    const float TargetYaw = std::atan2(FlatDirection.Y, FlatDirection.X) * (180.0f / 3.14159265358979323846f) + FacingYawOffset;

    FRotator Rotation = Owner->GetActorRotation();
    const float DeltaYaw = NormalizeYawDelta(TargetYaw - Rotation.Yaw);
    const float YawRate = (std::max)(0.0f, FacingYawRate);

    if (YawRate <= 0.0f || DeltaTime <= 0.0f)
    {
        Rotation.Yaw = TargetYaw;
    }
    else
    {
        const float Step = YawRate * DeltaTime;
        if (std::fabs(DeltaYaw) <= Step)
        {
            Rotation.Yaw = TargetYaw;
        }
        else
        {
            Rotation.Yaw += (DeltaYaw > 0.0f ? Step : -Step);
        }
    }

    Rotation.Yaw = NormalizeYawDelta(Rotation.Yaw);
    Owner->SetActorRotation(Rotation);
}

void UCombatCoverAgentComponent::FaceLocation2D(const FVector& WorldLocation, float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FaceDirection2D(WorldLocation - Owner->GetActorLocation(), DeltaTime);
}

void UCombatCoverAgentComponent::TickFaceCombatTarget(float DeltaTime)
{
    UCombatCoverAgentComponent* Target = CurrentTarget.Get();
    if (!Target || !Target->IsAlive() || !Target->GetOwner())
    {
        return;
    }

    FaceLocation2D(Target->GetOwner()->GetActorLocation(), DeltaTime);
}

void UCombatCoverAgentComponent::SetBlocked()
{
    State = ECombatCoverAgentState::Blocked;
    RetryTimer = 0.0f;
}

void UCombatCoverAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (State != ECombatCoverAgentState::Dead && Health <= 0.0f)
    {
        MarkDead();
        return;
    }

    if (State != ECombatCoverAgentState::Dead)
    {
        UCombatCoverAgentComponent* Target = CurrentTarget.Get();
        if (Target && !Target->IsAlive())
        {
            ClearEngagementTarget();
        }
        else
        {
            TickFaceCombatTarget(DeltaTime);
        }
    }

    switch (State)
    {
    case ECombatCoverAgentState::Idle:
        if (bAutoStart)
        {
            RequestInitialSlot();
        }
        break;

    case ECombatCoverAgentState::MovingToInitialSlot:
    case ECombatCoverAgentState::MovingToLinkedNode:
        TickMoveToTarget(DeltaTime);
        break;

    case ECombatCoverAgentState::InCover:
        AdvanceTimer += DeltaTime;
        if (AdvanceTimer >= AdvanceInterval)
        {
            AdvanceTimer = 0.0f;
            RequestAdvance();
        }
        break;

    case ECombatCoverAgentState::Engaging:
        break;

    case ECombatCoverAgentState::Suppressed:
        SuppressionTimer -= DeltaTime;
        if (SuppressionTimer <= 0.0f)
        {
            FinishSuppression();
        }
        break;

    case ECombatCoverAgentState::Blocked:
        RetryTimer += DeltaTime;
        if (RetryTimer >= RetryInterval)
        {
            RetryTimer = 0.0f;
            if (CurrentNodeId.empty())
            {
                RequestInitialSlot();
            }
            else
            {
                RequestAdvance();
            }
        }
        break;

    case ECombatCoverAgentState::Dead:
    default:
        break;
    }
}
