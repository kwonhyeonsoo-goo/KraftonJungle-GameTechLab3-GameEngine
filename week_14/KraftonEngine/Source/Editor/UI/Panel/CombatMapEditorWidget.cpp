#include "Editor/UI/Panel/CombatMapEditorWidget.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Component/Gameplay/CombatCoverNodeComponent.h"
#include "Component/Gameplay/CombatFlowManagerComponent.h"
#include "Core/Logging/Log.h"
#include "Editor/EditorEngine.h"
#include "Editor/PIE/PIETypes.h"
#include "Editor/Selection/SelectionManager.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <cstring>
#include <string>

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

    const char* SafeActorName(const AActor* Actor)
    {
        return Actor ? Actor->GetName().c_str() : "(none)";
    }

    FString NodeListLabel(const UCombatCoverNodeComponent* Node)
    {
        if (!Node)
        {
            return "(null)";
        }

        FString Label = Node->GetNodeId().empty() ? FString("<empty NodeId>") : Node->GetNodeId();
        if (Node->GetOwner())
        {
            Label += "  [" + Node->GetOwner()->GetName() + "]";
        }
        return Label;
    }
}

void FCombatMapEditorWidget::Initialize(UEditorEngine* InEditorEngine)
{
    FEditorWidget::Initialize(InEditorEngine);
    Refresh();
}

void FCombatMapEditorWidget::Render(float /*DeltaTime*/)
{
    ImGui::SetNextWindowSize(ImVec2(900.0f, 720.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Combat Map Editor"))
    {
        ImGui::End();
        return;
    }

    RenderToolbar();
    ImGui::Separator();
    RenderTwoColumnLayout();

    ImGui::End();
}

void FCombatMapEditorWidget::Refresh()
{
    CachedNodes.clear();
    CachedAgents.clear();
    CachedManager = nullptr;

    UWorld* World = GetEditorWorld();
    if (!World)
    {
        SelectedNode = nullptr;
        SelectedSlotIndex = -1;
        return;
    }

    CachedManager = UCombatFlowManagerComponent::FindInWorld(World);
    if (CachedManager)
    {
        CachedManager->RefreshRegistry();
        CachedNodes = CachedManager->GetNodes();
        CachedAgents = CachedManager->GetAgents();
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
                CachedNodes.push_back(Node);
            }
            if (UCombatCoverAgentComponent* Agent = Actor->GetComponentByClass<UCombatCoverAgentComponent>())
            {
                CachedAgents.push_back(Agent);
            }
        }
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
    if (CachedManager)
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
    }
    ImGui::SameLine();
    if (ImGui::Button("Validate Graph"))
    {
        if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
        {
            const FCombatCoverGraphValidationResult Result = Manager->ValidateGraph(true);
            LastValidationMessages = Result.Messages;
        }
        Refresh();
    }
    ImGui::SameLine();
    if (ImGui::Button("Draw Debug Once"))
    {
        DrawAllDebugOnce();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Selected Debug Only", &bShowOnlySelectedNodeDebug);

    AActor* SelectedActor = GetSelectedActor();
    ImGui::Text("Selected Actor: %s", SafeActorName(SelectedActor));

    if (ImGui::Button("Add Cover Node Component"))
    {
        if (UCombatCoverNodeComponent* Node = AddComponentToSelectedActor<UCombatCoverNodeComponent>())
        {
            SelectNode(Node);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Cover Agent"))
    {
        AddComponentToSelectedActor<UCombatCoverAgentComponent>();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Flow Manager"))
    {
        AddComponentToSelectedActor<UCombatFlowManagerComponent>();
    }

    if (ImGui::Button("Auto Generate Missing NodeIds"))
    {
        if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
        {
            const int32 Count = Manager->AutoGenerateMissingNodeIds();
            UE_LOG("CombatMapEditor: generated %d missing NodeIds", Count);
        }
        else
        {
            int32 Index = 1;
            for (UCombatCoverNodeComponent* Node : CachedNodes)
            {
                if (Node)
                {
                    Node->EnsureNodeId(Index++);
                }
            }
        }
        Refresh();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Link Nearby"))
    {
        if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
        {
            const int32 Count = Manager->AutoLinkNearby(AutoLinkMaxDistance, AutoLinkMaxLinksPerNode, bAutoLinkDirectedByX);
            UE_LOG("CombatMapEditor: auto linked %d edges", Count);
        }
        Refresh();
    }

    ImGui::SetNextItemWidth(180.0f);
    ImGui::DragFloat("Auto Link Max Distance", &AutoLinkMaxDistance, 10.0f, 0.0f, 100000.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::DragInt("Max Links", &AutoLinkMaxLinksPerNode, 1.0f, 1, 16);
    ImGui::SameLine();
    ImGui::Checkbox("Directed By +X", &bAutoLinkDirectedByX);
}

void FCombatMapEditorWidget::RenderTwoColumnLayout()
{
    const ImGuiTableFlags Flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("CombatMapEditorTwoColumnLayout", 2, Flags))
    {
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthFixed, 340.0f);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch);
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
    ImGui::Separator();
    RenderValidationPanel();
}

void FCombatMapEditorWidget::RenderRightColumn()
{
    RenderSelectedNodePanel();
    ImGui::Separator();
    RenderLinkPanel(SelectedNode);
    ImGui::Separator();
    RenderGraphPreview();
}

void FCombatMapEditorWidget::RenderNodeList()
{
    ImGui::SeparatorText("Cover Nodes");
    ImGui::Text("Nodes: %d", static_cast<int32>(CachedNodes.size()));
    ImGui::BeginChild("CombatNodeList", ImVec2(0.0f, 210.0f), true);
    for (int32 Index = 0; Index < static_cast<int32>(CachedNodes.size()); ++Index)
    {
        UCombatCoverNodeComponent* Node = CachedNodes[Index];
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

    UCombatCoverNodeComponent* Node = SelectedNode;
    if (!Node)
    {
        AActor* SelectedActor = GetSelectedActor();
        Node = SelectedActor ? SelectedActor->GetComponentByClass<UCombatCoverNodeComponent>() : nullptr;
        if (Node)
        {
            SelectedNode = Node;
        }
    }

    if (!Node)
    {
        ImGui::TextDisabled("No cover node selected.");
        return;
    }

    ImGui::Text("Owner: %s", SafeActorName(Node->GetOwner()));

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
    if (!Node)
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
    if (!Node)
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
            if (!Candidate || Candidate == Node)
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
            if (Target && !Target->GetNodeId().empty())
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

void FCombatMapEditorWidget::RenderGraphPreview()
{
    ImGui::SeparatorText("Graph Preview");
    ImGui::BeginChild("CombatGraphPreview", ImVec2(0.0f, 180.0f), true);

    if (CachedNodes.empty())
    {
        ImGui::TextDisabled("No cover nodes.");
        ImGui::EndChild();
        return;
    }

    for (UCombatCoverNodeComponent* Node : CachedNodes)
    {
        if (!Node)
        {
            continue;
        }

        const bool bSelected = Node == SelectedNode;
        if (bSelected)
        {
            ImGui::Text("[selected] %s", NodeListLabel(Node).c_str());
        }
        else
        {
            ImGui::TextUnformatted(NodeListLabel(Node).c_str());
        }

        const TArray<FCombatCoverLink>& Links = Node->GetLinks();
        if (Links.empty())
        {
            ImGui::Indent();
            ImGui::TextDisabled("(no outgoing links)");
            ImGui::Unindent();
            continue;
        }

        ImGui::Indent();
        for (const FCombatCoverLink& Link : Links)
        {
            ImGui::BulletText("-> %s  weight %.2f%s",
                Link.TargetNodeId.c_str(),
                Link.Weight,
                Link.bBidirectional ? "  bidirectional" : "");
        }
        ImGui::Unindent();
    }

    ImGui::EndChild();
}

void FCombatMapEditorWidget::RenderAgentPanel()
{
    ImGui::SeparatorText("Simulation");
    RenderPIEControls();

    ImGui::Text("Agents: %d", static_cast<int32>(CachedAgents.size()));
    if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
    {
        if (ImGui::Button("Refresh Manager Registry"))
        {
            Manager->RefreshRegistry();
            Refresh();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Occupancy"))
        {
            Manager->ResetRuntimeState();
        }
    }
    else
    {
        ImGui::TextDisabled("No UCombatFlowManagerComponent in this world.");
    }

    ImGui::BeginChild("CombatAgentList", ImVec2(0.0f, 190.0f), true);
    if (CachedAgents.empty())
    {
        ImGui::TextDisabled("No combat agents.");
    }

    for (UCombatCoverAgentComponent* Agent : CachedAgents)
    {
        if (!Agent)
        {
            continue;
        }

        ImGui::TextWrapped("%s | Team %s | %s",
            SafeActorName(Agent->GetOwner()),
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

void FCombatMapEditorWidget::RenderValidationPanel()
{
    ImGui::SeparatorText("Validation");
    if (ImGui::Button("Validate Now"))
    {
        if (UCombatFlowManagerComponent* Manager = FindOrUseManager())
        {
            const FCombatCoverGraphValidationResult Result = Manager->ValidateGraph(true);
            LastValidationMessages = Result.Messages;
        }
    }

    ImGui::BeginChild("CombatValidationMessages", ImVec2(0.0f, 0.0f), true);
    if (LastValidationMessages.empty())
    {
        ImGui::TextDisabled("No validation result yet.");
        ImGui::EndChild();
        return;
    }

    for (const FString& Message : LastValidationMessages)
    {
        ImGui::TextWrapped("%s", Message.c_str());
    }
    ImGui::EndChild();
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

template<typename TComponent>
TComponent* FCombatMapEditorWidget::AddComponentToSelectedActor()
{
    AActor* SelectedActor = GetSelectedActor();
    if (!SelectedActor)
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
        if (Node && (!bShowOnlySelectedNodeDebug || Node == SelectedNode))
        {
            Node->DrawDebugVisuals(World->GetScene(), Node == SelectedNode);
        }
    }
}

void FCombatMapEditorWidget::SelectNode(UCombatCoverNodeComponent* Node)
{
    SelectedNode = Node;
    SelectedSlotIndex = -1;
}
