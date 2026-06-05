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
    AdvanceTimer = 0.0f;
    RetryTimer = 0.0f;
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
    if (State == ECombatCoverAgentState::Dead)
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
    if (State == ECombatCoverAgentState::Dead)
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
    if (UCombatFlowManagerComponent* Manager = ResolveManager())
    {
        Manager->ReleaseAgent(this);
    }

    State = ECombatCoverAgentState::Dead;
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
    case ECombatCoverAgentState::Blocked: return "Blocked";
    case ECombatCoverAgentState::Dead: return "Dead";
    default: return "Unknown";
    }
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
