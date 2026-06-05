#include "CombatFlowManagerComponent.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Core/Logging/Log.h"
#include "Debug/DrawDebugHelpers.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace
{
    FString MakeNodeDebugName(const UCombatCoverNodeComponent* Node)
    {
        if (!Node)
        {
            return "(null)";
        }

        const FString& NodeId = Node->GetNodeId();
        if (!NodeId.empty())
        {
            return NodeId;
        }

        const AActor* Owner = Node->GetOwner();
        return Owner ? Owner->GetName() : FString("(unnamed node)");
    }

    float Dist2D(const FVector& A, const FVector& B)
    {
        const float DX = A.X - B.X;
        const float DY = A.Y - B.Y;
        return sqrtf(DX * DX + DY * DY);
    }

    FString FormatSlotHandle(const FCombatCoverSlotHandle& Handle)
    {
        char Buffer[128] = {};
        std::snprintf(Buffer, sizeof(Buffer), "%s:%d", Handle.NodeId.c_str(), Handle.SlotId);
        return FString(Buffer);
    }

    bool IsSameAgent(const TWeakObjectPtr<UCombatCoverAgentComponent>& Ptr, const UCombatCoverAgentComponent* Agent)
    {
        return Agent && Ptr.Get() == Agent;
    }
}

UCombatFlowManagerComponent::UCombatFlowManagerComponent()
{
    PrimaryComponentTick.SetTickGroup(TG_PrePhysics);
    SetComponentTickEnabled(true);
}

void UCombatFlowManagerComponent::BeginPlay()
{
    UActorComponent::BeginPlay();
    RefreshRegistry();
    ResetRuntimeState();
}

void UCombatFlowManagerComponent::EndPlay()
{
    ResetRuntimeState();
    CachedNodes.clear();
    CachedAgents.clear();
    NodeById.clear();
    UActorComponent::EndPlay();
}

void UCombatFlowManagerComponent::RefreshRegistry()
{
    CachedNodes.clear();
    CachedAgents.clear();
    NodeById.clear();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (AActor* Actor : World->GetActors())
    {
        if (!IsValid(Actor))
        {
            continue;
        }

        if (UCombatCoverNodeComponent* Node = Actor->GetComponentByClass<UCombatCoverNodeComponent>())
        {
            CachedNodes.push_back(Node);
            if (!Node->GetNodeId().empty())
            {
                NodeById[Node->GetNodeId()] = Node;
            }
        }

        if (UCombatCoverAgentComponent* Agent = Actor->GetComponentByClass<UCombatCoverAgentComponent>())
        {
            CachedAgents.push_back(Agent);
        }
    }

    RemoveStaleRuntimeState();
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        EnsureRuntimeSlotsForNode(Node);
    }
}

void UCombatFlowManagerComponent::ResetRuntimeState()
{
    RuntimeStateByNodeId.clear();
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        EnsureRuntimeSlotsForNode(Node);
    }
}

bool UCombatFlowManagerComponent::AssignInitialSlot(UCombatCoverAgentComponent* Agent)
{
    if (!IsValid(Agent) || !Agent->GetOwner())
    {
        return false;
    }

    RefreshRegistry();
    const FCombatCoverSlotHandle Candidate = FindNearestFreeSlot(Agent->GetOwner()->GetActorLocation(), Agent->GetTeamTag(), Agent);
    if (!Candidate.IsValid())
    {
        return false;
    }

    if (!ReserveSlot(Agent, Candidate))
    {
        return false;
    }

    Agent->MoveToReservedSlot(Candidate, true);
    return true;
}

bool UCombatFlowManagerComponent::TryAdvance(UCombatCoverAgentComponent* Agent)
{
    if (!IsValid(Agent))
    {
        return false;
    }

    RefreshRegistry();

    UCombatCoverNodeComponent* CurrentNode = FindNode(Agent->GetCurrentNodeId());
    if (!CurrentNode)
    {
        return AssignInitialSlot(Agent);
    }

    for (const FCombatCoverLink& Link : CurrentNode->GetLinks())
    {
        UCombatCoverNodeComponent* NextNode = FindNode(Link.TargetNodeId);
        if (!NextNode)
        {
            continue;
        }

        if (IsNodeOccupiedOrReserved(NextNode, Agent))
        {
            continue;
        }

        const FCombatCoverSlotHandle Candidate = FindFreeSlotInNode(NextNode, Agent->GetTeamTag(), Agent);
        if (!Candidate.IsValid())
        {
            continue;
        }

        if (!ReserveSlot(Agent, Candidate))
        {
            continue;
        }

        ReleaseAgentExcept(Agent, Candidate);
        Agent->MoveToReservedSlot(Candidate, false);
        return true;
    }

    return false;
}

void UCombatFlowManagerComponent::ConfirmArrived(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& SlotHandle)
{
    if (!IsValid(Agent) || !SlotHandle.IsValid())
    {
        return;
    }

    UCombatCoverNodeComponent* Node = FindNode(SlotHandle.NodeId);
    if (!Node)
    {
        return;
    }

    EnsureRuntimeSlotsForNode(Node);
    FCombatNodeRuntimeState& NodeState = RuntimeStateByNodeId[SlotHandle.NodeId];
    FCombatSlotRuntimeState& SlotState = NodeState.Slots[SlotHandle.SlotId];

    if (SlotState.ReservedBy.Get() == Agent || !SlotState.ReservedBy.Get())
    {
        SlotState.ReservedBy.Reset();
        SlotState.OccupiedBy.Reset(Agent);
    }
}

void UCombatFlowManagerComponent::ReleaseAgent(UCombatCoverAgentComponent* Agent)
{
    if (!Agent)
    {
        return;
    }

    for (auto& NodePair : RuntimeStateByNodeId)
    {
        for (auto& SlotPair : NodePair.second.Slots)
        {
            FCombatSlotRuntimeState& SlotState = SlotPair.second;
            if (SlotState.ReservedBy.Get() == Agent)
            {
                SlotState.ReservedBy.Reset();
            }
            if (SlotState.OccupiedBy.Get() == Agent)
            {
                SlotState.OccupiedBy.Reset();
            }
        }
    }
}

int32 UCombatFlowManagerComponent::AutoGenerateMissingNodeIds()
{
    RefreshRegistry();

    int32 GeneratedCount = 0;
    int32 Index = 1;
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (!Node)
        {
            continue;
        }

        if (Node->GetNodeId().empty())
        {
            Node->EnsureNodeId(Index);
            ++GeneratedCount;
        }
        ++Index;
    }

    RefreshRegistry();
    return GeneratedCount;
}

int32 UCombatFlowManagerComponent::AutoLinkNearby(float MaxDistance, int32 MaxLinksPerNode, bool bDirectedByX)
{
    RefreshRegistry();

    if (MaxDistance <= 0.0f || MaxLinksPerNode <= 0)
    {
        return 0;
    }

    int32 CreatedCount = 0;
    for (UCombatCoverNodeComponent* Source : CachedNodes)
    {
        if (!Source || !Source->GetOwner() || Source->GetNodeId().empty())
        {
            continue;
        }

        TArray<TPair<float, UCombatCoverNodeComponent*>> Candidates;
        const FVector SourceLocation = Source->GetOwner()->GetActorLocation();
        for (UCombatCoverNodeComponent* Target : CachedNodes)
        {
            if (!Target || Target == Source || !Target->GetOwner() || Target->GetNodeId().empty())
            {
                continue;
            }

            const FVector TargetLocation = Target->GetOwner()->GetActorLocation();
            if (bDirectedByX && TargetLocation.X <= SourceLocation.X)
            {
                continue;
            }

            const float Distance = Dist2D(SourceLocation, TargetLocation);
            if (Distance <= MaxDistance)
            {
                Candidates.push_back({ Distance, Target });
            }
        }

        std::sort(Candidates.begin(), Candidates.end(), [](const auto& A, const auto& B)
        {
            return A.first < B.first;
        });

        int32 LinksMadeForNode = 0;
        for (const auto& Candidate : Candidates)
        {
            if (LinksMadeForNode >= MaxLinksPerNode)
            {
                break;
            }

            if (Source->AddLinkToNodeId(Candidate.second->GetNodeId(), false))
            {
                ++LinksMadeForNode;
                ++CreatedCount;
            }
        }
    }

    RefreshRegistry();
    return CreatedCount;
}

FCombatCoverGraphValidationResult UCombatFlowManagerComponent::ValidateGraph(bool bLogToConsole)
{
    RefreshRegistry();

    FCombatCoverGraphValidationResult Result;
    Result.NodeCount = static_cast<int32>(CachedNodes.size());

    TMap<FString, int32> NodeIdCounts;
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (!Node)
        {
            continue;
        }

        Result.SlotCount += static_cast<int32>(Node->GetSlots().size());
        Result.LinkCount += static_cast<int32>(Node->GetLinks().size());

        if (Node->GetNodeId().empty())
        {
            AddValidationMessage(Result, true, MakeNodeDebugName(Node) + ": NodeId is empty.");
        }
        else
        {
            NodeIdCounts[Node->GetNodeId()]++;
        }

        if (Node->GetSlots().empty())
        {
            AddValidationMessage(Result, true, MakeNodeDebugName(Node) + ": has no slots.");
        }

        TSet<int32> SlotIds;
        for (const FCombatCoverSlot& Slot : Node->GetSlots())
        {
            if (!SlotIds.insert(Slot.SlotId).second)
            {
                AddValidationMessage(Result, true, MakeNodeDebugName(Node) + ": duplicated SlotId.");
            }
        }
    }

    for (const auto& Pair : NodeIdCounts)
    {
        if (Pair.second > 1)
        {
            AddValidationMessage(Result, true, Pair.first + ": duplicated NodeId.");
        }
    }

    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (!Node)
        {
            continue;
        }

        for (const FCombatCoverLink& Link : Node->GetLinks())
        {
            if (Link.TargetNodeId.empty())
            {
                AddValidationMessage(Result, true, MakeNodeDebugName(Node) + ": link target is empty.");
                continue;
            }

            if (Link.TargetNodeId == Node->GetNodeId())
            {
                AddValidationMessage(Result, true, MakeNodeDebugName(Node) + ": self link is not allowed.");
                continue;
            }

            UCombatCoverNodeComponent* Target = FindNode(Link.TargetNodeId);
            if (!Target)
            {
                AddValidationMessage(Result, true, MakeNodeDebugName(Node) + ": broken link to " + Link.TargetNodeId + ".");
                continue;
            }

            if (Link.bBidirectional)
            {
                bool bReverseFound = false;
                for (const FCombatCoverLink& Reverse : Target->GetLinks())
                {
                    if (Reverse.TargetNodeId == Node->GetNodeId())
                    {
                        bReverseFound = true;
                        break;
                    }
                }
                if (!bReverseFound)
                {
                    AddValidationMessage(Result, false, MakeNodeDebugName(Node) + ": bidirectional flag has no reverse link on " + MakeNodeDebugName(Target) + ".");
                }
            }
        }
    }

    if (bLogToConsole)
    {
        UE_LOG("Combat graph validate: nodes=%d slots=%d links=%d errors=%d warnings=%d",
            Result.NodeCount,
            Result.SlotCount,
            Result.LinkCount,
            Result.ErrorCount,
            Result.WarningCount);
        for (const FString& Message : Result.Messages)
        {
            UE_LOG("  %s", Message.c_str());
        }
    }

    return Result;
}

UCombatCoverNodeComponent* UCombatFlowManagerComponent::FindNode(const FString& NodeId) const
{
    const auto It = NodeById.find(NodeId);
    return It != NodeById.end() ? It->second : nullptr;
}

bool UCombatFlowManagerComponent::IsSlotFree(const FCombatCoverSlotHandle& SlotHandle, const UCombatCoverAgentComponent* RequestingAgent) const
{
    if (!SlotHandle.IsValid())
    {
        return false;
    }

    const auto NodeIt = RuntimeStateByNodeId.find(SlotHandle.NodeId);
    if (NodeIt == RuntimeStateByNodeId.end())
    {
        return true;
    }

    const auto SlotIt = NodeIt->second.Slots.find(SlotHandle.SlotId);
    if (SlotIt == NodeIt->second.Slots.end())
    {
        return true;
    }

    const FCombatSlotRuntimeState& SlotState = SlotIt->second;
    const UCombatCoverAgentComponent* ReservedBy = SlotState.ReservedBy.Get();
    const UCombatCoverAgentComponent* OccupiedBy = SlotState.OccupiedBy.Get();
    return (!ReservedBy || ReservedBy == RequestingAgent) && (!OccupiedBy || OccupiedBy == RequestingAgent);
}

bool UCombatFlowManagerComponent::IsNodeOccupiedOrReserved(const UCombatCoverNodeComponent* Node, const UCombatCoverAgentComponent* RequestingAgent) const
{
    if (!Node || Node->GetNodeId().empty())
    {
        return true;
    }

    if (!bUseNodeOccupancyLimit)
    {
        return false;
    }

    return CountNodeClaims(Node, RequestingAgent) >= (std::max)(1, Node->GetMaxOccupants());
}

void UCombatFlowManagerComponent::DrawAllDebugVisuals(bool bIncludeUnselected) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FScene& Scene = World->GetScene();
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (!Node)
        {
            continue;
        }
        Node->DrawDebugVisuals(Scene, !bIncludeUnselected);
    }
}

UCombatFlowManagerComponent* UCombatFlowManagerComponent::FindInWorld(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    for (AActor* Actor : World->GetActors())
    {
        if (!IsValid(Actor))
        {
            continue;
        }

        if (UCombatFlowManagerComponent* Manager = Actor->GetComponentByClass<UCombatFlowManagerComponent>())
        {
            return Manager;
        }
    }

    return nullptr;
}

FCombatCoverSlotHandle UCombatFlowManagerComponent::FindNearestFreeSlot(
    const FVector& WorldLocation,
    const FString& TeamTag,
    const UCombatCoverAgentComponent* RequestingAgent) const
{
    FCombatCoverSlotHandle BestHandle;
    float BestDistance = 0.0f;
    bool bHasBest = false;

    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (!Node || Node->GetNodeId().empty())
        {
            continue;
        }

        if (IsNodeOccupiedOrReserved(Node, RequestingAgent))
        {
            continue;
        }

        for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(Node->GetSlots().size()); ++SlotIndex)
        {
            const FCombatCoverSlot& Slot = Node->GetSlots()[SlotIndex];
            if (!SlotTagsMatchTeam(Slot, TeamTag))
            {
                continue;
            }

            FCombatCoverSlotHandle Handle;
            Handle.NodeId = Node->GetNodeId();
            Handle.SlotId = Slot.SlotId;
            if (!IsSlotFree(Handle, RequestingAgent))
            {
                continue;
            }

            const float Distance = Dist2D(WorldLocation, Node->GetSlotWorldPosition(SlotIndex));
            const float WeightedDistance = Slot.Weight > 0.0f ? Distance / Slot.Weight : Distance;
            if (!bHasBest || WeightedDistance < BestDistance)
            {
                BestDistance = WeightedDistance;
                BestHandle = Handle;
                bHasBest = true;
            }
        }
    }

    return BestHandle;
}

FCombatCoverSlotHandle UCombatFlowManagerComponent::FindFreeSlotInNode(
    UCombatCoverNodeComponent* Node,
    const FString& TeamTag,
    const UCombatCoverAgentComponent* RequestingAgent) const
{
    FCombatCoverSlotHandle BestHandle;
    float BestWeight = -1.0f;

    if (!Node || Node->GetNodeId().empty())
    {
        return BestHandle;
    }

    for (const FCombatCoverSlot& Slot : Node->GetSlots())
    {
        if (!SlotTagsMatchTeam(Slot, TeamTag))
        {
            continue;
        }

        FCombatCoverSlotHandle Handle;
        Handle.NodeId = Node->GetNodeId();
        Handle.SlotId = Slot.SlotId;
        if (!IsSlotFree(Handle, RequestingAgent))
        {
            continue;
        }

        if (BestWeight < 0.0f || Slot.Weight > BestWeight)
        {
            BestWeight = Slot.Weight;
            BestHandle = Handle;
        }
    }

    return BestHandle;
}

bool UCombatFlowManagerComponent::ReserveSlot(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& SlotHandle)
{
    if (!IsValid(Agent) || !SlotHandle.IsValid())
    {
        return false;
    }

    UCombatCoverNodeComponent* Node = FindNode(SlotHandle.NodeId);
    if (!Node || !Node->FindSlotById(SlotHandle.SlotId))
    {
        return false;
    }

    if (IsNodeOccupiedOrReserved(Node, Agent) || !IsSlotFree(SlotHandle, Agent))
    {
        return false;
    }

    EnsureRuntimeSlotsForNode(Node);
    FCombatSlotRuntimeState& SlotState = RuntimeStateByNodeId[SlotHandle.NodeId].Slots[SlotHandle.SlotId];
    SlotState.ReservedBy.Reset(Agent);
    return true;
}

void UCombatFlowManagerComponent::ReleaseAgentExcept(UCombatCoverAgentComponent* Agent, const FCombatCoverSlotHandle& KeepSlotHandle)
{
    if (!Agent)
    {
        return;
    }

    for (auto& NodePair : RuntimeStateByNodeId)
    {
        for (auto& SlotPair : NodePair.second.Slots)
        {
            const bool bKeep = NodePair.first == KeepSlotHandle.NodeId && SlotPair.first == KeepSlotHandle.SlotId;
            if (bKeep)
            {
                continue;
            }

            FCombatSlotRuntimeState& SlotState = SlotPair.second;
            if (SlotState.ReservedBy.Get() == Agent)
            {
                SlotState.ReservedBy.Reset();
            }
            if (SlotState.OccupiedBy.Get() == Agent)
            {
                SlotState.OccupiedBy.Reset();
            }
        }
    }
}

int32 UCombatFlowManagerComponent::CountNodeClaims(const UCombatCoverNodeComponent* Node, const UCombatCoverAgentComponent* IgnoreAgent) const
{
    if (!Node || Node->GetNodeId().empty())
    {
        return 0;
    }

    const auto NodeIt = RuntimeStateByNodeId.find(Node->GetNodeId());
    if (NodeIt == RuntimeStateByNodeId.end())
    {
        return 0;
    }

    int32 Count = 0;
    for (const auto& SlotPair : NodeIt->second.Slots)
    {
        const FCombatSlotRuntimeState& SlotState = SlotPair.second;
        const UCombatCoverAgentComponent* ReservedBy = SlotState.ReservedBy.Get();
        const UCombatCoverAgentComponent* OccupiedBy = SlotState.OccupiedBy.Get();

        if (ReservedBy && ReservedBy != IgnoreAgent)
        {
            ++Count;
        }
        if (OccupiedBy && OccupiedBy != IgnoreAgent && OccupiedBy != ReservedBy)
        {
            ++Count;
        }
    }
    return Count;
}

bool UCombatFlowManagerComponent::SlotTagsMatchTeam(const FCombatCoverSlot& Slot, const FString& TeamTag) const
{
    if (!bRequireSlotTagMatch || TeamTag.empty())
    {
        return true;
    }

    return Slot.Tags.find(TeamTag) != FString::npos;
}

void UCombatFlowManagerComponent::EnsureRuntimeSlotsForNode(UCombatCoverNodeComponent* Node)
{
    if (!Node || Node->GetNodeId().empty())
    {
        return;
    }

    FCombatNodeRuntimeState& NodeState = RuntimeStateByNodeId[Node->GetNodeId()];
    TSet<int32> ValidSlotIds;
    for (const FCombatCoverSlot& Slot : Node->GetSlots())
    {
        ValidSlotIds.insert(Slot.SlotId);
        NodeState.Slots[Slot.SlotId];
    }

    for (auto It = NodeState.Slots.begin(); It != NodeState.Slots.end(); )
    {
        if (ValidSlotIds.find(It->first) == ValidSlotIds.end())
        {
            It = NodeState.Slots.erase(It);
            continue;
        }
        ++It;
    }
}

void UCombatFlowManagerComponent::RemoveStaleRuntimeState()
{
    TSet<FString> ValidNodeIds;
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (Node && !Node->GetNodeId().empty())
        {
            ValidNodeIds.insert(Node->GetNodeId());
        }
    }

    for (auto It = RuntimeStateByNodeId.begin(); It != RuntimeStateByNodeId.end(); )
    {
        if (ValidNodeIds.find(It->first) == ValidNodeIds.end())
        {
            It = RuntimeStateByNodeId.erase(It);
            continue;
        }
        ++It;
    }
}

void UCombatFlowManagerComponent::AddValidationMessage(FCombatCoverGraphValidationResult& Result, bool bError, const FString& Message) const
{
    if (bError)
    {
        ++Result.ErrorCount;
        Result.Messages.push_back(FString("ERROR: ") + Message);
    }
    else
    {
        ++Result.WarningCount;
        Result.Messages.push_back(FString("WARN: ") + Message);
    }
}

void UCombatFlowManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bDrawDebugDuringTick)
    {
        return;
    }

    DebugDrawTimer += DeltaTime;
    if (DebugDrawTimer < DebugDrawInterval)
    {
        return;
    }

    DebugDrawTimer = 0.0f;
    RefreshRegistry();
    DrawAllDebugVisuals(true);
}
