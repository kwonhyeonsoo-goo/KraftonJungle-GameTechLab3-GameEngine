#include "Editor/UI/Panel/CombatMapEditorWidget.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Component/Gameplay/CombatCoverNodeComponent.h"
#include "Component/Gameplay/CombatFlowManagerComponent.h"
#include "Core/Logging/Log.h"
#include "Editor/EditorEngine.h"
#include "Editor/PIE/PIETypes.h"
#include "Editor/Selection/SelectionManager.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "Component/Primitive/StaticMeshComponent.h"
#include "Component/SceneComponent.h"
#include "GameFramework/World.h"

#include "ImGui/imgui.h"
#include "imgui_node_editor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

namespace ed = ax::NodeEditor;

namespace
{
    bool InputTextString(const char* Label, FString& Value)
    {
        char Buffer[256] = {};
        strncpy_s(Buffer, sizeof(Buffer), Value.c_str(), _TRUNCATE);
        if (ImGui::InputText(Label, Buffer, sizeof(Buffer)))
        {
            Value = Buffer;
            return true;
        }
        return false;
    }

    FString ActorNameForUI(const AActor* Actor)
    {
		return IsValid(Actor) ? Actor->GetName() : FString("(none)");
    }

	bool IsValidCombatNode(const UCombatCoverNodeComponent* Node)
	{
		return IsValid(Node) && IsValid(Node->GetOwner());
	}

	bool IsValidCombatAgent(const UCombatCoverAgentComponent* Agent)
	{
		return IsValid(Agent) && IsValid(Agent->GetOwner());
	}

    uint32 HashString32(const FString& Text)
    {
        uint32 Hash = 2166136261u;
        for (char Ch : Text)
        {
            Hash ^= static_cast<uint8>(Ch);
            Hash *= 16777619u;
        }
        return Hash ? Hash : 1u;
    }

    uint32 HashCombine32(uint32 A, uint32 B)
    {
        uint32 Hash = A ? A : 2166136261u;
        Hash ^= B + 0x9e3779b9u + (Hash << 6) + (Hash >> 2);
        return Hash ? Hash : 1u;
    }

    uint32 GraphKeyForNode(const UCombatCoverNodeComponent* Node)
    {
		if (!IsValid(Node))
        {
            return 1u;
        }

        uint32 Key = Node->GetUUID();
		if (Key == 0 && IsValid(Node->GetOwner()))
        {
            Key = Node->GetOwner()->GetUUID();
        }
        if (Key == 0)
        {
            Key = HashString32(Node->GetNodeId());
        }
        return Key ? Key : 1u;
    }

    uint32 MakeCombatNodeGraphNodeId(const UCombatCoverNodeComponent* Node)
    {
        return 0x71000000u | (GraphKeyForNode(Node) & 0x00ffffffu);
    }

    uint32 MakeCombatNodeInputPinId(const UCombatCoverNodeComponent* Node)
    {
        return 0x72000000u | (GraphKeyForNode(Node) & 0x00ffffffu);
    }

    uint32 MakeCombatNodeOutputPinId(const UCombatCoverNodeComponent* Node)
    {
        return 0x73000000u | (GraphKeyForNode(Node) & 0x00ffffffu);
    }

    uint32 MakeCombatLinkGraphId(const UCombatCoverNodeComponent* Source, const FCombatCoverLink& Link, int32 LinkIndex)
    {
        return 0x75000000u | (HashCombine32(GraphKeyForNode(Source), HashCombine32(HashString32(Link.TargetNodeId), static_cast<uint32>(LinkIndex + 1))) & 0x00ffffffu);
    }

    inline ed::NodeId ToGraphNodeId(uint32 Id) { return static_cast<ed::NodeId>(Id); }
    inline ed::PinId ToGraphPinId(uint32 Id) { return static_cast<ed::PinId>(Id); }
    inline ed::LinkId ToGraphLinkId(uint32 Id) { return static_cast<ed::LinkId>(Id); }
    inline uint32 GraphPinIdToU32(ed::PinId Id) { return static_cast<uint32>(Id.Get()); }
    inline uint32 GraphLinkIdToU32(ed::LinkId Id) { return static_cast<uint32>(Id.Get()); }

    bool IsActorNameInUse(UWorld* World, const FString& CandidateName, const AActor* IgnoreActor)
    {
        if (!World || CandidateName.empty())
        {
            return false;
        }

        const FName CandidateFName(CandidateName);
        for (AActor* Actor : World->GetActors())
        {
            if (Actor && Actor != IgnoreActor && Actor->GetFName() == CandidateFName)
            {
                return true;
            }
        }
        return false;
    }

    FString MakeUniqueActorName(UWorld* World, const FString& BaseName, const AActor* IgnoreActor)
    {
        FString Base = BaseName.empty() ? FString("CoverNode") : BaseName;
        FString Candidate = Base;
        int32 Suffix = 2;
        while (IsActorNameInUse(World, Candidate, IgnoreActor))
        {
            Candidate = Base + "_" + std::to_string(Suffix++);
        }
        return Candidate;
    }

    FString NodeListLabel(const UCombatCoverNodeComponent* Node)
    {
		if (!IsValidCombatNode(Node))
        {
            return "(null)";
        }

        FString Label = Node->GetNodeId().empty() ? FString("<empty NodeId>") : Node->GetNodeId();
		if (IsValid(Node->GetOwner()))
        {
            Label += "  [" + Node->GetOwner()->GetName() + "]";
        }
        return Label;
    }
}

FCombatMapEditorWidget::~FCombatMapEditorWidget()
{
    DestroyGraphEditor();
}

void FCombatMapEditorWidget::Initialize(UEditorEngine* InEditorEngine)
{
    FEditorWidget::Initialize(InEditorEngine);
    InitializeGraphEditor();
    Refresh();
}

void FCombatMapEditorWidget::Render(float /*DeltaTime*/)
{
	RefreshIfWorldOrPIEStateChanged();
	PruneInvalidCachedReferences();

    ImGui::SetNextWindowSize(ImVec2(900.0f, 720.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Combat Map Editor"))
    {
        ImGui::End();
        return;
    }

    RenderToolbar();
    ImGui::Separator();
    RenderMainLayout();

    ImGui::End();
}

void FCombatMapEditorWidget::Refresh()
{
    CachedNodes.clear();
    CachedAgents.clear();
    CachedManager = nullptr;
	CachedWorld = nullptr;

    UWorld* World = GetEditorWorld();
    if (!World)
    {
        SelectedNode = nullptr;
        SelectedSlotIndex = -1;
        return;
    }
	CachedWorld = World;
	bWasPlayingInEditor = EditorEngine && EditorEngine->IsPlayingInEditor();

    CachedManager = UCombatFlowManagerComponent::FindInWorld(World);
	if (IsValid(CachedManager))
    {
        CachedManager->RefreshRegistry();
		for (UCombatCoverNodeComponent* Node : CachedManager->GetNodes())
		{
			if (IsValidCombatNode(Node))
			{
				CachedNodes.push_back(Node);
			}
		}
		for (UCombatCoverAgentComponent* Agent : CachedManager->GetAgents())
		{
			if (IsValidCombatAgent(Agent))
			{
				CachedAgents.push_back(Agent);
			}
		}
    }
    else
    {
        for (AActor* Actor : World->GetActors())
        {
            if (!IsValid(Actor))
            {
                continue;
            }

			if (UCombatCoverNodeComponent* Node = Actor->GetComponentByClass<UCombatCoverNodeComponent>())
            {
				if (IsValidCombatNode(Node))
				{
					CachedNodes.push_back(Node);
				}
            }
            if (UCombatCoverAgentComponent* Agent = Actor->GetComponentByClass<UCombatCoverAgentComponent>())
            {
				if (IsValidCombatAgent(Agent))
				{
					CachedAgents.push_back(Agent);
				}
            }
        }
    }

	PruneInvalidCachedReferences();
}

void FCombatMapEditorWidget::RefreshIfWorldOrPIEStateChanged()
{
	const bool bPlaying = EditorEngine && EditorEngine->IsPlayingInEditor();
	UWorld* CurrentWorld = GetEditorWorld();
	if (CurrentWorld == CachedWorld && bPlaying == bWasPlayingInEditor)
	{
		return;
	}

	ClearCachedRuntimePointers();
	Refresh();
	ResetGraphLayoutFromScene();
}

void FCombatMapEditorWidget::ClearCachedRuntimePointers()
{
	CachedNodes.clear();
	CachedAgents.clear();
	CachedManager = nullptr;
	CachedWorld = nullptr;
	SelectedNode = nullptr;
	SelectedSlotIndex = -1;
	LinkTargetIndex = -1;
}

void FCombatMapEditorWidget::PruneInvalidCachedReferences()
{
	CachedNodes.erase(
		std::remove_if(
			CachedNodes.begin(),
			CachedNodes.end(),
			[](const UCombatCoverNodeComponent* Node)
			{
				return !IsValidCombatNode(Node);
			}),
		CachedNodes.end());

	CachedAgents.erase(
		std::remove_if(
			CachedAgents.begin(),
			CachedAgents.end(),
			[](const UCombatCoverAgentComponent* Agent)
			{
				return !IsValidCombatAgent(Agent);
			}),
		CachedAgents.end());

	if (!IsValid(CachedManager))
	{
		CachedManager = nullptr;
	}

	if (SelectedNode && std::find(CachedNodes.begin(), CachedNodes.end(), SelectedNode) == CachedNodes.end())
    {
        SelectedNode = nullptr;
        SelectedSlotIndex = -1;
    }
}

UWorld* FCombatMapEditorWidget::GetEditorWorld() const
{
    return EditorEngine ? EditorEngine->GetWorld() : nullptr;
}

AActor* FCombatMapEditorWidget::GetSelectedActor() const
{
    return EditorEngine ? EditorEngine->GetSelectionManager().GetPrimarySelection() : nullptr;
}

UCombatFlowManagerComponent* FCombatMapEditorWidget::FindOrUseManager() const
{
	if (IsValid(CachedManager))
    {
        return CachedManager;
    }
    return UCombatFlowManagerComponent::FindInWorld(GetEditorWorld());
}

void FCombatMapEditorWidget::RenderToolbar()
{
    if (ImGui::Button("Refresh"))
    {
        Refresh();
        ResetGraphLayoutFromScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Generate Missing NodeIds"))
    {
        GenerateNodeIdsAndRenameActors();
    }
    ImGui::SameLine();
    if (ImGui::Button("Validate Now"))
    {
        LastValidationMessages.clear();
        if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
        {
            const FCombatCoverGraphValidationResult Result = Manager->ValidateGraph(true);
            LastValidationMessages = Result.Messages;
        }
        if (!LastValidationMessages.empty())
        {
            bPendingOpenValidationPopup = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Link Nearby"))
    {
        bPendingOpenAutoLinkPopup = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Draw Debug Once"))
    {
        DrawAllDebugOnce();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Selected Debug Only", &bShowOnlySelectedNodeDebug);

    AActor* SelectedActor = GetSelectedActor();
    const FString SelectedActorName = ActorNameForUI(SelectedActor);
    ImGui::Text("Selected Actor: %s", SelectedActorName.c_str());

    RenderAutoLinkPopup();
    RenderValidationPopup();
}

void FCombatMapEditorWidget::RenderMainLayout()
{
    const float AvailableHeight = ImGui::GetContentRegionAvail().y;
    const float GraphHeight = (std::max)(260.0f, AvailableHeight * 0.48f);
    const float MiddleHeight = (std::max)(220.0f, AvailableHeight - GraphHeight - ImGui::GetStyle().ItemSpacing.y);

    ImGui::BeginChild("CombatMapEditorMiddleRegion", ImVec2(0.0f, MiddleHeight), false);
    RenderMiddleLayout();
    ImGui::EndChild();

    ImGui::Separator();
    RenderGraphEditor();
}

void FCombatMapEditorWidget::RenderMiddleLayout()
{
    const ImGuiTableFlags Flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("CombatMapEditorMiddleLayout", 2, Flags))
    {
        ImGui::TableSetupColumn("NodesAndAgents", ImGuiTableColumnFlags_WidthFixed, 340.0f);
        ImGui::TableSetupColumn("SelectedDetails", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("CombatMapEditorLeftColumn", ImVec2(0.0f, 0.0f), false);
        RenderLeftColumn();
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("CombatMapEditorRightColumn", ImVec2(0.0f, 0.0f), false);
        RenderRightColumn();
        ImGui::EndChild();

        ImGui::EndTable();
    }
}

void FCombatMapEditorWidget::RenderLeftColumn()
{
    RenderNodeList();
    ImGui::Separator();
    RenderAgentPanel();
}

void FCombatMapEditorWidget::RenderRightColumn()
{
    RenderSelectedNodePanel();
    ImGui::Separator();
    RenderLinkPanel(SelectedNode);
}

void FCombatMapEditorWidget::RenderNodeList()
{
    ImGui::SeparatorText("Cover Nodes");
    ImGui::Text("Nodes: %d", static_cast<int32>(CachedNodes.size()));
    ImGui::BeginChild("CombatNodeList", ImVec2(0.0f, 210.0f), true);
    for (int32 Index = 0; Index < static_cast<int32>(CachedNodes.size()); ++Index)
    {
        UCombatCoverNodeComponent* Node = CachedNodes[Index];
		if (!IsValidCombatNode(Node))
		{
			continue;
		}
        const FString Label = NodeListLabel(Node) + "##CombatNode" + std::to_string(Index);
        if (ImGui::Selectable(Label.c_str(), Node == SelectedNode))
        {
            SelectNode(Node);
        }
    }
    ImGui::EndChild();
}

void FCombatMapEditorWidget::RenderSelectedNodePanel()
{
    ImGui::SeparatorText("Selected Node");

	UCombatCoverNodeComponent* Node = IsValidCombatNode(SelectedNode) ? SelectedNode : nullptr;
	if (!Node)
	{
		SelectedNode = nullptr;
		SelectedSlotIndex = -1;
	}
    if (!Node)
    {
        AActor* SelectedActor = GetSelectedActor();
		Node = IsValid(SelectedActor) ? SelectedActor->GetComponentByClass<UCombatCoverNodeComponent>() : nullptr;
		if (IsValidCombatNode(Node))
        {
            SelectedNode = Node;
        }
		else
		{
			Node = nullptr;
		}
    }

    if (!Node)
    {
        ImGui::TextDisabled("No cover node selected.");
        return;
    }

    const FString OwnerName = ActorNameForUI(Node->GetOwner());
    ImGui::Text("Owner: %s", OwnerName.c_str());

    FString NodeId = Node->GetNodeId();
    if (InputTextString("NodeId", NodeId))
    {
        Node->SetNodeId(NodeId);
        Refresh();
    }

    FString DisplayName = Node->GetDisplayName();
    if (InputTextString("Display Name", DisplayName))
    {
        Node->SetDisplayName(DisplayName);
    }

    ImGui::Text("Slots: %d  Links: %d  MaxOccupants: %d",
        Node->GetSlotCount(), Node->GetLinkCount(), Node->GetMaxOccupants());

    if (ImGui::Button("Add Slot At Actor Origin"))
    {
        Node->AddSlotAtLocalPosition(FVector::ZeroVector);
        Refresh();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Slot In Front"))
    {
        Node->AddSlotInFront(150.0f);
        Refresh();
    }

    RenderSlotPanel(Node);
}

void FCombatMapEditorWidget::RenderSlotPanel(UCombatCoverNodeComponent* Node)
{
	if (!IsValidCombatNode(Node))
    {
        return;
    }

    TArray<FCombatCoverSlot>& Slots = Node->GetMutableSlots();
    ImGui::SeparatorText("Slots");
    if (Slots.empty())
    {
        ImGui::TextDisabled("No slots.");
        return;
    }

    ImGui::BeginChild("CombatSlotList", ImVec2(0.0f, 220.0f), true);
    for (int32 Index = 0; Index < static_cast<int32>(Slots.size()); ++Index)
    {
        FCombatCoverSlot& Slot = Slots[Index];
        ImGui::PushID(Index);
        const bool bSelected = SelectedSlotIndex == Index;
        if (ImGui::Selectable((FString("Slot ") + std::to_string(Slot.SlotId)).c_str(), bSelected))
        {
            SelectedSlotIndex = Index;
        }
        if (bSelected)
        {
            ImGui::DragInt("SlotId", &Slot.SlotId, 1.0f, 0, 100000);
            ImGui::DragFloat3("Local Position", Slot.LocalPosition.Data, 1.0f);
            ImGui::DragFloat3("Local Forward", Slot.LocalForward.Data, 0.05f);
            ImGui::DragFloat("Radius", &Slot.Radius, 1.0f, 1.0f, 10000.0f);
            InputTextString("Tags", Slot.Tags);
            ImGui::DragFloat("Weight", &Slot.Weight, 0.1f, 0.0f, 1000.0f);
            if (ImGui::Button("Delete This Slot"))
            {
                Node->RemoveSlotByIndex(Index);
                SelectedSlotIndex = -1;
                ImGui::PopID();
                break;
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

void FCombatMapEditorWidget::RenderLinkPanel(UCombatCoverNodeComponent* Node)
{
    ImGui::SeparatorText("Links");
	if (!IsValidCombatNode(Node))
    {
        ImGui::TextDisabled("Select a cover node first.");
        return;
    }

    ImGui::Text("Source: %s", NodeListLabel(Node).c_str());

    TArray<FCombatCoverLink>& Links = Node->GetMutableLinks();
    if (Links.empty())
    {
        ImGui::TextDisabled("No outgoing links.");
    }
    else
    {
        ImGui::BeginChild("CombatLinkList", ImVec2(0.0f, 130.0f), true);
        for (int32 Index = 0; Index < static_cast<int32>(Links.size()); ++Index)
        {
            FCombatCoverLink& Link = Links[Index];
            ImGui::PushID(Index);
            ImGui::Text("-> %s", Link.TargetNodeId.c_str());
            ImGui::SameLine();
            ImGui::Checkbox("Bidirectional", &Link.bBidirectional);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::DragFloat("Weight", &Link.Weight, 0.1f, 0.0f, 1000.0f);
            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                Links.erase(Links.begin() + Index);
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    ImGui::SeparatorText("Create Link");
    if (LinkTargetIndex < 0 || LinkTargetIndex >= static_cast<int32>(CachedNodes.size()))
    {
        LinkTargetIndex = CachedNodes.empty() ? -1 : 0;
    }

    const FString PreviewLabel = LinkTargetIndex >= 0 ? NodeListLabel(CachedNodes[LinkTargetIndex]) : FString("(none)");
    if (ImGui::BeginCombo("Target Node", PreviewLabel.c_str()))
    {
        for (int32 Index = 0; Index < static_cast<int32>(CachedNodes.size()); ++Index)
        {
            UCombatCoverNodeComponent* Candidate = CachedNodes[Index];
			if (!IsValidCombatNode(Candidate) || Candidate == Node)
            {
                continue;
            }
            const bool bSelected = Index == LinkTargetIndex;
            if (ImGui::Selectable(NodeListLabel(Candidate).c_str(), bSelected))
            {
                LinkTargetIndex = Index;
            }
        }
        ImGui::EndCombo();
    }

    static bool bNewLinkBidirectional = false;
    ImGui::Checkbox("New Link Bidirectional", &bNewLinkBidirectional);
    if (ImGui::Button("Create Link"))
    {
        if (LinkTargetIndex >= 0 && LinkTargetIndex < static_cast<int32>(CachedNodes.size()))
        {
            UCombatCoverNodeComponent* Target = CachedNodes[LinkTargetIndex];
			if (IsValidCombatNode(Target) && !Target->GetNodeId().empty())
            {
                Node->AddLinkToNodeId(Target->GetNodeId(), bNewLinkBidirectional);
                if (bNewLinkBidirectional && !Node->GetNodeId().empty())
                {
                    Target->AddLinkToNodeId(Node->GetNodeId(), bNewLinkBidirectional);
                }
                Refresh();
            }
        }
    }
}

void FCombatMapEditorWidget::RenderGraphEditor()
{
    ImGui::SeparatorText("Graph Editor");

    InitializeGraphEditor();

    if (ImGui::Button("Pull Layout From Scene"))
    {
        ResetGraphLayoutFromScene();
    }
	ImGui::SameLine();
	if (ImGui::Button("Fit Graph"))
	{
		bPendingGraphNavigateToContent = true;
	}
    ImGui::SameLine();
    ImGui::Checkbox("Apply Graph To Scene", &bGraphApplyToScene);
    ImGui::TextDisabled("Top-down graph uses fixed 1/15 scale and always flips scene X/Y for Z-up left-handed view. Scene Z is fixed to 0.");

    if (!GraphEditorContext)
    {
        ImGui::TextDisabled("Node editor context is not available.");
        return;
    }

    ImGui::BeginChild("CombatGraphEditorChild", ImVec2(0.0f, 0.0f), true);
    ed::SetCurrentEditor(GraphEditorContext);
    ed::Begin("CombatCoverGraphCanvas");

    TMap<uint32, UCombatCoverNodeComponent*> InputPinToNode;
    TMap<uint32, UCombatCoverNodeComponent*> OutputPinToNode;
    TMap<uint32, std::pair<UCombatCoverNodeComponent*, FString>> LinkIdToEdge;
    bool bSkipApplyGraphToScene = false;

    for (int32 NodeIndex = 0; NodeIndex < static_cast<int32>(CachedNodes.size()); ++NodeIndex)
    {
        UCombatCoverNodeComponent* Node = CachedNodes[NodeIndex];
		if (!IsValidCombatNode(Node))
        {
            continue;
        }

        EnsureGraphNodePositionFromScene(Node, NodeIndex);

        const uint32 NodeGraphId = MakeCombatNodeGraphNodeId(Node);
        const uint32 InputPinId = MakeCombatNodeInputPinId(Node);
        const uint32 OutputPinId = MakeCombatNodeOutputPinId(Node);
        InputPinToNode[InputPinId] = Node;
        OutputPinToNode[OutputPinId] = Node;

        const bool bSelected = Node == SelectedNode;
        const FString OwnerName = ActorNameForUI(Node->GetOwner());
        const FString NodeIdText = Node->GetNodeId().empty() ? FString("<empty NodeId>") : Node->GetNodeId();

        ed::BeginNode(ToGraphNodeId(NodeGraphId));
        ed::BeginPin(ToGraphPinId(OutputPinId), ed::PinKind::Output);
        ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.80f, 1.0f), "out");
        ed::EndPin();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextColored(
            bSelected ? ImVec4(1.0f, 0.86f, 0.18f, 1.0f) : ImVec4(0.45f, 0.78f, 1.0f, 1.0f),
            "%s",
            NodeIdText.c_str());
        if (ImGui::IsItemClicked())
        {
            SelectNode(Node);
			if (EditorEngine && IsValid(Node->GetOwner()))
            {
                EditorEngine->GetSelectionManager().Select(Node->GetOwner());
            }
        }
        ImGui::TextDisabled("%s", OwnerName.c_str());
        ImGui::TextDisabled("S %d | L %d", Node->GetSlotCount(), Node->GetLinkCount());
        ImGui::EndGroup();
        ImGui::SameLine();
        ed::BeginPin(ToGraphPinId(InputPinId), ed::PinKind::Input);
        ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.80f, 1.0f), "in");
        ed::EndPin();
        ed::EndNode();

    }

    for (int32 SourceIndex = 0; SourceIndex < static_cast<int32>(CachedNodes.size()); ++SourceIndex)
    {
        UCombatCoverNodeComponent* Source = CachedNodes[SourceIndex];
		if (!IsValidCombatNode(Source))
        {
            continue;
        }

        const TArray<FCombatCoverLink>& Links = Source->GetLinks();
        for (int32 LinkIndex = 0; LinkIndex < static_cast<int32>(Links.size()); ++LinkIndex)
        {
            const FCombatCoverLink& Link = Links[LinkIndex];
            UCombatCoverNodeComponent* Target = nullptr;
            for (UCombatCoverNodeComponent* Candidate : CachedNodes)
            {
				if (IsValidCombatNode(Candidate) && Candidate->GetNodeId() == Link.TargetNodeId)
                {
                    Target = Candidate;
                    break;
                }
            }
            if (!Target)
            {
                continue;
            }

            const uint32 LinkGraphId = MakeCombatLinkGraphId(Source, Link, LinkIndex);
            LinkIdToEdge[LinkGraphId] = { Source, Link.TargetNodeId };
            ed::Link(
                ToGraphLinkId(LinkGraphId),
                ToGraphPinId(MakeCombatNodeOutputPinId(Source)),
                ToGraphPinId(MakeCombatNodeInputPinId(Target)),
                ImColor(125, 210, 190),
                Link.bBidirectional ? 4.0f : 2.0f);
        }
    }

	if (bPendingGraphNavigateToContent && !CachedNodes.empty())
	{
		ed::NavigateToContent(0.25f);
		bPendingGraphNavigateToContent = false;
	}

    if (ed::BeginCreate())
    {
        ed::PinId StartPinId = 0;
        ed::PinId EndPinId = 0;
        if (ed::QueryNewLink(&StartPinId, &EndPinId) && StartPinId && EndPinId)
        {
            UCombatCoverNodeComponent* Source = nullptr;
            UCombatCoverNodeComponent* Target = nullptr;

            const uint32 Start = GraphPinIdToU32(StartPinId);
            const uint32 End = GraphPinIdToU32(EndPinId);
            const auto StartOut = OutputPinToNode.find(Start);
            const auto EndIn = InputPinToNode.find(End);
            if (StartOut != OutputPinToNode.end() && EndIn != InputPinToNode.end())
            {
                Source = StartOut->second;
                Target = EndIn->second;
            }
            else
            {
                const auto EndOut = OutputPinToNode.find(End);
                const auto StartIn = InputPinToNode.find(Start);
                if (EndOut != OutputPinToNode.end() && StartIn != InputPinToNode.end())
                {
                    Source = EndOut->second;
                    Target = StartIn->second;
                }
            }

            const bool bCanCreate =
			IsValidCombatNode(Source) && IsValidCombatNode(Target) && Source != Target &&
                !Source->GetNodeId().empty() && !Target->GetNodeId().empty();

            if (bCanCreate)
            {
                if (ed::AcceptNewItem(ImVec4(0.55f, 0.90f, 0.80f, 1.0f), 2.0f))
                {
                    Source->AddLinkToNodeId(Target->GetNodeId(), false);
                    Refresh();
                }
            }
            else
            {
                ed::RejectNewItem(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), 2.0f);
            }
        }
    }
    ed::EndCreate();

    if (ed::BeginDelete())
    {
        ed::LinkId DeletedLinkId = 0;
        while (ed::QueryDeletedLink(&DeletedLinkId))
        {
            const uint32 LinkId = GraphLinkIdToU32(DeletedLinkId);
            const auto It = LinkIdToEdge.find(LinkId);
            if (It != LinkIdToEdge.end() && It->second.first)
            {
                if (ed::AcceptDeletedItem())
                {
                    It->second.first->RemoveLinkToNodeId(It->second.second);
                    Refresh();
                }
            }
        }
    }
    ed::EndDelete();

    static ImVec2 ContextGraphPosition(0.0f, 0.0f);
    static uint32 ContextGraphNodeId = 0;
    ed::NodeId ContextNodeId = 0;
    ed::Suspend();
    if (ed::ShowNodeContextMenu(&ContextNodeId))
    {
        ContextGraphNodeId = static_cast<uint32>(ContextNodeId.Get());
        ContextGraphPosition = ed::ScreenToCanvas(ImGui::GetMousePos());
        ImGui::OpenPopup("CombatCoverGraphNodeMenu");
    }
    else if (ed::ShowBackgroundContextMenu())
    {
        ContextGraphNodeId = 0;
        ContextGraphPosition = ed::ScreenToCanvas(ImGui::GetMousePos());
        ImGui::OpenPopup("CombatCoverGraphBackgroundMenu");
    }

    if (ImGui::BeginPopup("CombatCoverGraphNodeMenu"))
    {
        UCombatCoverNodeComponent* ContextNode = FindNodeByGraphNodeId(ContextGraphNodeId);
        const bool bCanDuplicate = IsValidCombatNode(ContextNode);
        if (ImGui::MenuItem("Duplicate Cover Node", nullptr, false, bCanDuplicate))
        {
            if (UCombatCoverNodeComponent* NewNode = DuplicateCoverNodeActor(ContextNode, &ContextGraphPosition))
            {
                SelectNode(NewNode);
                bSkipApplyGraphToScene = true;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("CombatCoverGraphBackgroundMenu"))
    {
        if (ImGui::MenuItem("Create Cover Node Actor"))
        {
            if (UCombatCoverNodeComponent* NewNode = CreateCoverNodeActorFromEditor(&ContextGraphPosition))
            {
                SelectNode(NewNode);
                bSkipApplyGraphToScene = true;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ed::Resume();


    ed::End();

    if (bGraphApplyToScene && !bSkipApplyGraphToScene)
    {
        for (int32 NodeIndex = 0; NodeIndex < static_cast<int32>(CachedNodes.size()); ++NodeIndex)
        {
            UCombatCoverNodeComponent* Node = CachedNodes[NodeIndex];
		if (!IsValidCombatNode(Node))
            {
                continue;
            }

            ApplyGraphPositionToScene(Node, NodeIndex);
        }
    }

    ed::SetCurrentEditor(nullptr);
    ImGui::EndChild();
}

void FCombatMapEditorWidget::RenderAgentPanel()
{
    ImGui::SeparatorText("Simulation");
    RenderPIEControls();

    ImGui::Text("Agents: %d", static_cast<int32>(CachedAgents.size()));
    if (!FindOrUseManager())
    {
        ImGui::TextDisabled("No UCombatFlowManagerComponent in this world.");
    }

    ImGui::BeginChild("CombatAgentList", ImVec2(0.0f, 0.0f), true);
    if (CachedAgents.empty())
    {
        ImGui::TextDisabled("No combat agents.");
    }

    for (UCombatCoverAgentComponent* Agent : CachedAgents)
    {
        if (!IsValidCombatAgent(Agent))
        {
            continue;
        }

        ImGui::TextWrapped("%s | Team %s | %s",
            ActorNameForUI(Agent->GetOwner()).c_str(),
            Agent->GetTeamTag().c_str(),
            Agent->GetStateName());
        ImGui::TextDisabled("Current: %s:%d  Target: %s:%d",
            Agent->GetCurrentNodeId().c_str(),
            Agent->GetCurrentSlotId(),
            Agent->GetTargetNodeId().c_str(),
            Agent->GetTargetSlotId());
        ImGui::Separator();
    }
    ImGui::EndChild();
}

void FCombatMapEditorWidget::RenderAutoLinkPopup()
{
    if (bPendingOpenAutoLinkPopup)
    {
        ImGui::OpenPopup("Auto Link Nearby Settings");
        bPendingOpenAutoLinkPopup = false;
    }

    if (ImGui::BeginPopupModal("Auto Link Nearby Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetNextItemWidth(180.0f);
        ImGui::DragFloat("Max Distance", &AutoLinkMaxDistance, 10.0f, 0.0f, 100000.0f);
        ImGui::SetNextItemWidth(180.0f);
        ImGui::DragInt("Max Lines", &AutoLinkMaxLinksPerNode, 1.0f, 1, 16);
        ImGui::Checkbox("Directed By +X", &bAutoLinkDirectedByX);

        if (ImGui::Button("Run Auto Link"))
        {
            if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
            {
                const int32 Count = Manager->AutoLinkNearby(AutoLinkMaxDistance, AutoLinkMaxLinksPerNode, bAutoLinkDirectedByX);
                UE_LOG("CombatMapEditor: auto linked %d edges", Count);
            }
            Refresh();
            ResetGraphLayoutFromScene();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void FCombatMapEditorWidget::RenderValidationPopup()
{
    if (bPendingOpenValidationPopup)
    {
        ImGui::OpenPopup("Combat Graph Validation");
        bPendingOpenValidationPopup = false;
    }

    if (ImGui::BeginPopupModal("Combat Graph Validation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Validation issues: %d", static_cast<int32>(LastValidationMessages.size()));
        ImGui::Separator();
        for (const FString& Message : LastValidationMessages)
        {
            ImGui::TextWrapped("%s", Message.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void FCombatMapEditorWidget::RenderPIEControls()
{
    const bool bPlaying = EditorEngine && EditorEngine->IsPlayingInEditor();

    if (bPlaying)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Start PIE Simulation"))
    {
        if (EditorEngine)
        {
            FRequestPlaySessionParams Params;
            Params.PlayMode = EPIEPlayMode::PlayInViewport;
            EditorEngine->RequestPlaySession(Params);
        }
    }
    if (bPlaying)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (!bPlaying)
    {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Stop PIE"))
    {
        if (EditorEngine)
        {
            EditorEngine->RequestEndPlayMap();
        }
    }
    if (!bPlaying)
    {
        ImGui::EndDisabled();
    }

    ImGui::TextDisabled("PIE State: %s", bPlaying ? "Running" : "Stopped");
}

void FCombatMapEditorWidget::InitializeGraphEditor()
{
    if (GraphEditorContext)
    {
        return;
    }

    ed::Config Config;
    GraphEditorContext = ed::CreateEditor(&Config);
    InitializedGraphItemIds.clear();
}

void FCombatMapEditorWidget::DestroyGraphEditor()
{
    if (GraphEditorContext)
    {
        ed::DestroyEditor(GraphEditorContext);
        GraphEditorContext = nullptr;
    }
    InitializedGraphItemIds.clear();
}

void FCombatMapEditorWidget::ResetGraphLayoutFromScene()
{
    InitializedGraphItemIds.clear();
	bPendingGraphNavigateToContent = true;
}

ImVec2 FCombatMapEditorWidget::WorldToGraph(const FVector& Position) const
{
    const float SafeUnitsPerGraphUnit = (std::max)(0.001f, GraphSceneUnitsPerGraphUnit);
    return ImVec2(-Position.X / SafeUnitsPerGraphUnit, -Position.Y / SafeUnitsPerGraphUnit);
}

FVector FCombatMapEditorWidget::GraphToWorld(const ImVec2& Position) const
{
    const float SafeUnitsPerGraphUnit = (std::max)(0.001f, GraphSceneUnitsPerGraphUnit);
    return FVector(-Position.x * SafeUnitsPerGraphUnit, -Position.y * SafeUnitsPerGraphUnit, 0.0f);
}

void FCombatMapEditorWidget::EnsureGraphNodePositionFromScene(UCombatCoverNodeComponent* Node, int32 /*NodeIndex*/)
{
	if (!GraphEditorContext || !IsValidCombatNode(Node))
    {
        return;
    }

    const uint32 NodeGraphId = MakeCombatNodeGraphNodeId(Node);
    if (InitializedGraphItemIds.find(NodeGraphId) != InitializedGraphItemIds.end())
    {
        return;
    }

	const FVector WorldLocation = Node->GetOwner()->GetActorLocation();
    ed::SetNodePosition(ToGraphNodeId(NodeGraphId), WorldToGraph(WorldLocation));
    InitializedGraphItemIds.insert(NodeGraphId);
}

void FCombatMapEditorWidget::ApplyGraphPositionToScene(UCombatCoverNodeComponent* Node, int32 /*NodeIndex*/)
{
	if (!GraphEditorContext || !IsValidCombatNode(Node))
    {
        return;
    }

    const uint32 NodeGraphId = MakeCombatNodeGraphNodeId(Node);
    const ImVec2 GraphPosition = ed::GetNodePosition(ToGraphNodeId(NodeGraphId));
    Node->GetOwner()->SetActorLocation(GraphToWorld(GraphPosition));
}

UCombatCoverNodeComponent* FCombatMapEditorWidget::CreateCoverNodeActorFromEditor(const ImVec2* GraphPosition)
{
    UWorld* World = GetEditorWorld();
    if (!World)
    {
        return nullptr;
    }

    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>();
    if (!Actor)
    {
        return nullptr;
    }

    Actor->InitDefaultComponents("Content/Data/BasicShape/Cube.OBJ");
    if (UStaticMeshComponent* StaticMeshComponent = Actor->GetStaticMeshComponent())
    {
        if (!StaticMeshComponent->SetMaterialByPath(0, "Content/Material/Auto/BasicShapeMaterial.uasset"))
        {
            UE_LOG("CombatMapEditor: failed to assign BasicShapeMaterial to new cover node actor");
        }
    }

    FVector SpawnLocation(static_cast<float>(CachedNodes.size()) * 500.0f, 0.0f, 0.0f);
    if (GraphPosition)
    {
        SpawnLocation = GraphToWorld(*GraphPosition);
    }
    else if (IsValidCombatNode(SelectedNode))
    {
        SpawnLocation = SelectedNode->GetOwner()->GetActorLocation() + FVector(500.0f, 0.0f, 0.0f);
    }
    SpawnLocation.Z = 0.0f;
    Actor->SetActorLocation(SpawnLocation);

    UCombatCoverNodeComponent* Node = Actor->AddComponent<UCombatCoverNodeComponent>();
    if (!Node)
    {
        return nullptr;
    }

    Node->AddSlotAtLocalPosition(FVector::ZeroVector);
    Refresh();
    GenerateNodeIdsAndRenameActors();
    Refresh();

    if (EditorEngine)
    {
        EditorEngine->GetSelectionManager().Select(Actor);
    }
    ResetGraphLayoutFromScene();
    return Actor->GetComponentByClass<UCombatCoverNodeComponent>();
}

UCombatCoverNodeComponent* FCombatMapEditorWidget::DuplicateCoverNodeActor(UCombatCoverNodeComponent* SourceNode, const ImVec2* GraphPosition)
{
    if (!IsValidCombatNode(SourceNode))
    {
        return nullptr;
    }

    AActor* SourceActor = SourceNode->GetOwner();
    if (!IsValid(SourceActor))
    {
        return nullptr;
    }

    AActor* DuplicateActor = Cast<AActor>(SourceActor->Duplicate(nullptr));
    if (!DuplicateActor)
    {
        return nullptr;
    }

    FVector NewLocation = SourceActor->GetActorLocation() + FVector(500.0f, 0.0f, 0.0f);
    if (GraphPosition)
    {
        NewLocation = GraphToWorld(*GraphPosition);
    }
    NewLocation.Z = 0.0f;
    DuplicateActor->SetActorLocation(NewLocation);

    UCombatCoverNodeComponent* DuplicateNode = DuplicateActor->GetComponentByClass<UCombatCoverNodeComponent>();
    if (DuplicateNode)
    {
        DuplicateNode->SetNodeId(FString());
        DuplicateNode->SetDisplayName(FString());
        DuplicateNode->GetMutableLinks().clear();
    }

    Refresh();
    GenerateNodeIdsAndRenameActors();
    Refresh();

    if (EditorEngine)
    {
        EditorEngine->GetSelectionManager().Select(DuplicateActor);
    }
    ResetGraphLayoutFromScene();
    return DuplicateActor->GetComponentByClass<UCombatCoverNodeComponent>();
}

UCombatCoverNodeComponent* FCombatMapEditorWidget::FindNodeByGraphNodeId(uint32 GraphNodeId) const
{
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (IsValidCombatNode(Node) && MakeCombatNodeGraphNodeId(Node) == GraphNodeId)
        {
            return Node;
        }
    }
    return nullptr;
}

void FCombatMapEditorWidget::GenerateNodeIdsAndRenameActors()
{
    if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
    {
        const int32 Count = Manager->AutoGenerateMissingNodeIds();
        UE_LOG("CombatMapEditor: generated %d missing NodeIds", Count);
    }
    else
    {
        TSet<FString> UsedIds;
		for (UCombatCoverNodeComponent* Node : CachedNodes)
        {
			if (IsValidCombatNode(Node) && !Node->GetNodeId().empty())
            {
                UsedIds.insert(Node->GetNodeId());
            }
        }

        int32 NextIndex = 1;
		for (UCombatCoverNodeComponent* Node : CachedNodes)
        {
			if (!IsValidCombatNode(Node) || !Node->GetNodeId().empty())
            {
                continue;
            }

            FString Candidate;
            do
            {
                char Buffer[64] = {};
                std::snprintf(Buffer, sizeof(Buffer), "CoverNode_%03d", NextIndex++);
                Candidate = Buffer;
            }
            while (UsedIds.find(Candidate) != UsedIds.end());

            Node->SetNodeId(Candidate);
            UsedIds.insert(Candidate);
        }
    }

    Refresh();
    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        RenameActorToNodeId(Node);
    }
    Refresh();
    ResetGraphLayoutFromScene();
}

void FCombatMapEditorWidget::RenameActorToNodeId(UCombatCoverNodeComponent* Node)
{
	if (!IsValidCombatNode(Node) || Node->GetNodeId().empty())
    {
        return;
    }

    UWorld* World = GetEditorWorld();
    AActor* Owner = Node->GetOwner();
    const FString UniqueName = MakeUniqueActorName(World, Node->GetNodeId(), Owner);
    Owner->SetFName(FName(UniqueName));
}

template<typename TComponent>
TComponent* FCombatMapEditorWidget::AddComponentToSelectedActor()
{
    AActor* SelectedActor = GetSelectedActor();
	if (!IsValid(SelectedActor))
    {
        return nullptr;
    }

    if (TComponent* Existing = SelectedActor->GetComponentByClass<TComponent>())
    {
        UE_LOG("CombatMapEditor: selected actor already has %s", TComponent::StaticClass()->GetName());
        return Existing;
    }

    TComponent* Component = SelectedActor->AddComponent<TComponent>();
    if (!Component)
    {
        UE_LOG("CombatMapEditor: failed to add %s", TComponent::StaticClass()->GetName());
        return nullptr;
    }

    Refresh();
    return Component;
}

void FCombatMapEditorWidget::DrawAllDebugOnce()
{
    Refresh();
    if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
    {
        Manager->DrawAllDebugVisuals(!bShowOnlySelectedNodeDebug);
        return;
    }

    UWorld* World = GetEditorWorld();
    if (!World)
    {
        return;
    }

    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
		if (IsValidCombatNode(Node) && (!bShowOnlySelectedNodeDebug || Node == SelectedNode))
        {
            Node->DrawDebugVisuals(World->GetScene(), Node == SelectedNode);
        }
    }
}

void FCombatMapEditorWidget::SelectNode(UCombatCoverNodeComponent* Node)
{
	if (!IsValidCombatNode(Node))
	{
		SelectedNode = nullptr;
		SelectedSlotIndex = -1;
		return;
	}

    SelectedNode = Node;
    SelectedSlotIndex = -1;

	if (EditorEngine && IsValid(Node->GetOwner()))
    {
        EditorEngine->GetSelectionManager().Select(Node->GetOwner());
    }
}
