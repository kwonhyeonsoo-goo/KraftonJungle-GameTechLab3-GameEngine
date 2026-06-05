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
    MaxHealth = (std::max)(1.0f, MaxHealth);
    Health = (std::min)((std::max)(0.0f, Health), MaxHealth);
    AdvanceTimer = 0.0f;
    RetryTimer = 0.0f;
    TargetScanTimer = 0.0f;
    IncomingFireCount = 0;
    IncomingDamagePerSecond = 0.0f;
    StateBeforeEngage = ECombatCoverAgentState::Idle;
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

void UCombatCoverAgentComponent::RequestInitialSlot()
{
    if (State == ECombatCoverAgentState::Dead || (State == ECombatCoverAgentState::Engaging && !bCanFireWhileMoving))
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
    if (State == ECombatCoverAgentState::Dead || (State == ECombatCoverAgentState::Engaging && !bCanFireWhileMoving))
    {
        return;
    }

    UCombatFlowManagerComponent* Manager = ResolveManager();
    if (!Manager || !Manager->TryAdvance(this))
    {
        SetBlocked();
    }
}

void UCombatCoverAgentComponent::MoveToReservedSlot(const FCombatCoverSlotHandle& SlotHandle, bool bInitialMove)
{
    if (!SlotHandle.IsValid())
    {
        SetBlocked();
        return;
    }

    TargetNodeId = SlotHandle.NodeId;
    TargetSlotId = SlotHandle.SlotId;
    AdvanceTimer = 0.0f;
    RetryTimer = 0.0f;
    State = bInitialMove ? ECombatCoverAgentState::MovingToInitialSlot : ECombatCoverAgentState::MovingToLinkedNode;
}

void UCombatCoverAgentComponent::MarkDead()
{
    CurrentTarget.Reset();
    IncomingFireCount = 0;
    IncomingDamagePerSecond = 0.0f;
    Health = 0.0f;

    if (UCombatFlowManagerComponent* Manager = ResolveManager())
    {
        Manager->ReleaseAgent(this);
    }

    State = ECombatCoverAgentState::Dead;
    StateBeforeEngage = ECombatCoverAgentState::Dead;
    CurrentNodeId.clear();
    CurrentSlotId = -1;
    TargetNodeId.clear();
    TargetSlotId = -1;
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

void UCombatCoverAgentComponent::SetEngagementTarget(UCombatCoverAgentComponent* Target)
{
    if (State == ECombatCoverAgentState::Dead)
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

void UCombatCoverAgentComponent::SetIncomingFireStats(int32 Count, float DamagePerSecond)
{
    IncomingFireCount = (std::max)(0, Count);
    IncomingDamagePerSecond = (std::max)(0.0f, DamagePerSecond);
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

    const FVector TargetLocation = TargetNode->GetSlotWorldPosition(SlotIndex);
    FVector CurrentLocation = Owner->GetActorLocation();
    FVector Delta = TargetLocation - CurrentLocation;
    Delta.Z = 0.0f;

    const float Distance = Delta.Length();
    if (Distance <= AcceptanceRadius)
    {
        FCombatCoverSlotHandle ArrivedSlot;
        ArrivedSlot.NodeId = TargetNodeId;
        ArrivedSlot.SlotId = TargetSlotId;
        Manager->ConfirmArrived(this, ArrivedSlot);

        CurrentNodeId = TargetNodeId;
        CurrentSlotId = TargetSlotId;
        TargetNodeId.clear();
        TargetSlotId = -1;
        AdvanceTimer = 0.0f;
        RetryTimer = 0.0f;
        State = ECombatCoverAgentState::InCover;
        return;
    }

    if (Delta.IsNearlyZero())
    {
        return;
    }

    const FVector Direction = Delta.Normalized();
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
