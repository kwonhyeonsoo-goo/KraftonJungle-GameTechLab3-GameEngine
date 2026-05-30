#include "Editor/UI/Asset/Material/MaterialEditorWidget.h"

#include "Materials/Material.h"
#include "Materials/MaterialManager.h"
#include "Object/Object.h"
#include "Platform/Paths.h"
#include "Serialization/MemoryArchive.h"

#include "Component/Light/DirectionalLightComponent.h"
#include "Component/Primitive/StaticMeshComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "GameFramework/Light/DirectionalLightActor.h"
#include "Runtime/Engine.h"
#include "Slate/SlateApplication.h"
#include "Viewport/Viewport.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace ed = ax::NodeEditor;

namespace
{
    inline ed::NodeId ToNodeId(uint32 Id)
    {
        return static_cast<ed::NodeId>(Id);
    }

    inline ed::PinId ToPinId(uint32 Id)
    {
        return static_cast<ed::PinId>(Id);
    }

    inline ed::LinkId ToLinkId(uint32 Id)
    {
        return static_cast<ed::LinkId>(Id);
    }

    inline uint32 NodeIdToU32(ed::NodeId Id)
    {
        return static_cast<uint32>(Id.Get());
    }

    inline uint32 PinIdToU32(ed::PinId Id)
    {
        return static_cast<uint32>(Id.Get());
    }

    inline uint32 LinkIdToU32(ed::LinkId Id)
    {
        return static_cast<uint32>(Id.Get());
    }

    struct FScopedNodeEditorCurrent
    {
        ed::EditorContext* Previous = nullptr;
        ed::EditorContext* Desired  = nullptr;

        explicit FScopedNodeEditorCurrent(ed::EditorContext* InDesired) : Desired(InDesired)
        {
            Previous = ed::GetCurrentEditor();
            if (Desired && Previous != Desired) ed::SetCurrentEditor(Desired);
        }

        ~FScopedNodeEditorCurrent()
        {
            if (Desired && Previous != Desired) ed::SetCurrentEditor(Previous);
        }
    };

    ImVec4 PinTypeColor(EMaterialGraphPinType Type)
    {
        switch (Type)
        {
        case EMaterialGraphPinType::Float:
            return ImVec4(0.90f, 0.90f, 0.30f, 1.0f);
        case EMaterialGraphPinType::Float2:
        case EMaterialGraphPinType::UV:
            return ImVec4(0.35f, 0.85f, 0.95f, 1.0f);
        case EMaterialGraphPinType::Float3:
            return ImVec4(0.35f, 0.95f, 0.45f, 1.0f);
        case EMaterialGraphPinType::Float4:
        case EMaterialGraphPinType::Color:
            return ImVec4(0.95f, 0.55f, 0.30f, 1.0f);
        case EMaterialGraphPinType::Texture2D:
            return ImVec4(0.80f, 0.55f, 1.0f, 1.0f);
        case EMaterialGraphPinType::Bool:
            return ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
        default:
            return ImVec4(0.78f, 0.78f, 0.78f, 1.0f);
        }
    }

    const char* NodeLabel(EMaterialGraphNodeType Type)
    {
        return ToString(Type);
    }

    // UE Material editor 의 카테고리별 헤더 컬러 컨벤션. 노드 분류를 한눈에 구분한다.
    ImVec4 NodeHeaderColor(EMaterialGraphNodeType Type)
    {
        switch (Type)
        {
        case EMaterialGraphNodeType::Output:
            return ImVec4(0.92f, 0.46f, 0.40f, 1.0f);

        case EMaterialGraphNodeType::ScalarParameter:
        case EMaterialGraphNodeType::VectorParameter:
        case EMaterialGraphNodeType::ColorParameter:
            return ImVec4(0.52f, 0.92f, 0.62f, 1.0f);

        case EMaterialGraphNodeType::ConstantFloat:
        case EMaterialGraphNodeType::ConstantFloat2:
        case EMaterialGraphNodeType::ConstantFloat3:
        case EMaterialGraphNodeType::ConstantFloat4:
            return ImVec4(0.45f, 0.83f, 0.95f, 1.0f);

        case EMaterialGraphNodeType::TextureObject:
        case EMaterialGraphNodeType::TextureSample:
            return ImVec4(0.78f, 0.56f, 1.00f, 1.0f);

        case EMaterialGraphNodeType::Add:
        case EMaterialGraphNodeType::Subtract:
        case EMaterialGraphNodeType::Multiply:
        case EMaterialGraphNodeType::Divide:
        case EMaterialGraphNodeType::OneMinus:
        case EMaterialGraphNodeType::Saturate:
        case EMaterialGraphNodeType::Clamp:
        case EMaterialGraphNodeType::Power:
        case EMaterialGraphNodeType::Lerp:
        case EMaterialGraphNodeType::Append:
        case EMaterialGraphNodeType::ComponentMask:
        case EMaterialGraphNodeType::ConstantBiasScale:
        case EMaterialGraphNodeType::Distance:
        case EMaterialGraphNodeType::Normalize:
        case EMaterialGraphNodeType::Dot:
        case EMaterialGraphNodeType::Cross:
            return ImVec4(0.76f, 0.66f, 1.00f, 1.0f);

        case EMaterialGraphNodeType::TexCoord:
        case EMaterialGraphNodeType::Panner:
        case EMaterialGraphNodeType::Time:
            return ImVec4(0.95f, 0.85f, 0.40f, 1.0f);

        case EMaterialGraphNodeType::VertexColor:
        case EMaterialGraphNodeType::ParticleColor:
        case EMaterialGraphNodeType::ParticleSubUV:
        case EMaterialGraphNodeType::DynamicParameter:
            return ImVec4(0.96f, 0.64f, 0.34f, 1.0f);

        case EMaterialGraphNodeType::Reroute:
        case EMaterialGraphNodeType::Comment:
        default:
            return ImVec4(0.82f, 0.82f, 0.82f, 1.0f);
        }
    }

    const char* NodeCategoryLabel(EMaterialGraphNodeType Type)
    {
        switch (Type)
        {
        case EMaterialGraphNodeType::Output:
            return "Output";
        case EMaterialGraphNodeType::ScalarParameter:
        case EMaterialGraphNodeType::VectorParameter:
        case EMaterialGraphNodeType::ColorParameter:
            return "Parameter";
        case EMaterialGraphNodeType::ConstantFloat:
        case EMaterialGraphNodeType::ConstantFloat2:
        case EMaterialGraphNodeType::ConstantFloat3:
        case EMaterialGraphNodeType::ConstantFloat4:
            return "Constant";
        case EMaterialGraphNodeType::TextureObject:
        case EMaterialGraphNodeType::TextureSample:
            return "Texture";
        case EMaterialGraphNodeType::TexCoord:
        case EMaterialGraphNodeType::Panner:
        case EMaterialGraphNodeType::Time:
            return "Coordinates";
        case EMaterialGraphNodeType::VertexColor:
        case EMaterialGraphNodeType::ParticleColor:
        case EMaterialGraphNodeType::ParticleSubUV:
        case EMaterialGraphNodeType::DynamicParameter:
            return "Particle";
        case EMaterialGraphNodeType::Reroute:
        case EMaterialGraphNodeType::Comment:
            return "Utility";
        default:
            return "Math";
        }
    }

    // 한 프레임 동안 연결되어 있는 pin 집합 — 핀 아이콘을 채워진 원/빈 원으로 구분하기 위함.
    bool IsPinLinked(const FMaterialGraph& Graph, uint32 PinId)
    {
        for (const FMaterialGraphLink& Link : Graph.Links)
        {
            if (Link.FromPinId == PinId || Link.ToPinId == PinId) return true;
        }
        return false;
    }

    // UE 스타일 핀 아이콘: 연결되면 채워진 원, 비연결이면 테두리만. "●" 글리프 대신 draw-list 사용.
    void DrawPinIcon(const ImVec4& Color, bool bConnected)
    {
        const float  Diameter = ImGui::GetTextLineHeight() * 0.85f;
        const ImVec2 Size(Diameter, Diameter);
        const ImVec2 P        = ImGui::GetCursorScreenPos();
        ImDrawList*  DrawList = ImGui::GetWindowDrawList();
        const ImVec2 Center(P.x + Size.x * 0.5f, P.y + Size.y * 0.5f);
        const float  Radius = Size.x * 0.40f;
        const ImU32  Col    = ImGui::ColorConvertFloat4ToU32(Color);
        if (bConnected)
        {
            DrawList->AddCircleFilled(Center, Radius, Col, 12);
        }
        else
        {
            DrawList->AddCircle(Center, Radius, Col, 12, 1.6f);
            DrawList->AddCircleFilled(Center, Radius * 0.45f, ImGui::ColorConvertFloat4ToU32(ImVec4(Color.x, Color.y, Color.z, 0.30f)), 8);
        }
        ImGui::Dummy(Size);
    }

    // 비대화형 컬러 스와치 — 노드 안에서는 ColorEdit 의 picker 팝업이 캔버스 변환 때문에
    // 엉뚱한 위치에 뜨므로, 노드 본문은 읽기 전용 스와치만 그리고 편집은 인스펙터에서 한다.
    void DrawColorSwatch(const FVector4& Color, float Size = 0.0f)
    {
        const float  S        = Size > 0.0f ? Size : ImGui::GetFrameHeight();
        const ImVec2 P        = ImGui::GetCursorScreenPos();
        ImDrawList*  DrawList = ImGui::GetWindowDrawList();
        const ImU32  Fill     = ImGui::ColorConvertFloat4ToU32(ImVec4(Color.X, Color.Y, Color.Z, 1.0f));
        DrawList->AddRectFilled(P, ImVec2(P.x + S * 2.4f, P.y + S), Fill, 3.0f);
        DrawList->AddRect(P, ImVec2(P.x + S * 2.4f, P.y + S), IM_COL32(0, 0, 0, 160), 3.0f);
        ImGui::Dummy(ImVec2(S * 2.4f, S));
    }

    bool StringContainsInsensitive(const char* Haystack, const char* Needle)
    {
        if (!Needle || !Needle[0]) return true;
        if (!Haystack) return false;
        FString H = Haystack;
        FString N = Needle;
        std::transform(
            H.begin(),
            H.end(),
            H.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            }
        );
        std::transform(
            N.begin(),
            N.end(),
            N.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            }
        );
        return H.find(N) != FString::npos;
    }

    ImVec2 ComputeNodeFragmentMin(const TArray<FMaterialGraphNode>& Nodes)
    {
        ImVec2 Min(FLT_MAX, FLT_MAX);
        for (const FMaterialGraphNode& Node : Nodes)
        {
            Min.x = min(Min.x, Node.PosX);
            Min.y = min(Min.y, Node.PosY);
        }
        if (Min.x == FLT_MAX) return ImVec2(0, 0);
        return Min;
    }

    bool InputFString(const char* Label, FString& Value, size_t BufferSize = 256)
    {
        TArray<char> Buffer;
        Buffer.resize(BufferSize);
        std::snprintf(Buffer.data(), Buffer.size(), "%s", Value.c_str());
        if (ImGui::InputText(Label, Buffer.data(), Buffer.size()))
        {
            Value = Buffer.data();
            return true;
        }
        return false;
    }
}

FMaterialEditorWidget::~FMaterialEditorWidget()
{
    DestroyContext();
}

bool FMaterialEditorWidget::CanEdit(UObject* Object) const
{
    return Cast<UMaterial>(Object) != nullptr;
}

void FMaterialEditorWidget::Open(UObject* Object)
{
    FAssetEditorWidget::Open(Object);
    EnsureContext();

    if (UMaterial* Material = GetMaterial())
    {
        if (!Material->GetGraphDocument().bEnabled)
        {
            Material->EnableGraphMaterial();
        }
        WorkingGraph = Material->GetGraphDocument().Graph;
        if (!WorkingGraph.HasOutputNode())
        {
            WorkingGraph.InitializeDefault(Material->GetGraphDocument().Target);
        }
        bPositionsPushed          = false;
        bPendingInitialContentFit = true;
        CaptureInitialUndoSnapshot();
    }
}

void FMaterialEditorWidget::Close()
{
    PreviewMaterial = nullptr;
    DestroyContext();
    FAssetEditorWidget::Close();
}

void FMaterialEditorWidget::Render(float DeltaTime)
{
    RenderDocument(DeltaTime);
}

void FMaterialEditorWidget::RenderDocument(float DeltaTime)
{
    (void)DeltaTime;
    UMaterial* Material = GetMaterial();
    if (!Material)
    {
        Close();
        return;
    }

    EnsureContext();
    QueueNodeEditorShortcuts();

    RenderToolbar(Material);

    const float LeftWidth   = 280.0f;
    const float RightWidth  = 360.0f;
    const float Spacing     = ImGui::GetStyle().ItemSpacing.x;
    const float Height      = ImGui::GetContentRegionAvail().y;
    const float TotalWidth  = ImGui::GetContentRegionAvail().x;
    const float CanvasWidth = max(320.0f, TotalWidth - LeftWidth - RightWidth - Spacing * 2.0f);

    // ── Left column: Settings + Parameters + Palette ──
    ImGui::BeginChild("MaterialLeftPanel", ImVec2(LeftWidth, Height), ImGuiChildFlags_Borders);
    RenderLeftPanel(Material);
    ImGui::EndChild();

    // ── Center: node graph canvas ──
    ImGui::SameLine();
    ImGui::BeginChild("MaterialGraphCanvas", ImVec2(CanvasWidth, Height), ImGuiChildFlags_Borders);
    RenderGraphCanvas(Material);
    ImGui::EndChild();

    // ── Right column: node inspector (top) + compiler results (bottom) ──
    ImGui::SameLine();
    ImGui::BeginChild("MaterialRightPanel", ImVec2(RightWidth, Height), ImGuiChildFlags_Borders);
    const float InnerHeight    = ImGui::GetContentRegionAvail().y;
    const float CompilerHeight = min(240.0f, InnerHeight * 0.42f);
    ImGui::BeginChild("MaterialDetails", ImVec2(0, InnerHeight - CompilerHeight - ImGui::GetStyle().ItemSpacing.y), ImGuiChildFlags_None);
    RenderDetailsPanel(Material);
    ImGui::EndChild();
    ImGui::BeginChild("MaterialCompiler", ImVec2(0, 0), ImGuiChildFlags_Borders);
    RenderCompilerPanel(Material);
    ImGui::EndChild();
    ImGui::EndChild();
}

bool FMaterialEditorWidget::IsEditingObject(UObject* Object) const
{
    return IsOpen() && GetEditedObject() == Object;
}

FString FMaterialEditorWidget::GetDocumentTitle() const
{
    if (UMaterial* Material = GetMaterial())
    {
        return Material->GetAssetPathFileName().empty() ? FString("Material") : Material->GetAssetPathFileName();
    }
    return "Material";
}

FString FMaterialEditorWidget::GetDocumentPayloadId() const
{
    if (UMaterial* Material = GetMaterial())
    {
        return Material->GetAssetPathFileName();
    }
    return "Material";
}

void FMaterialEditorWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
    FAssetEditorWidget::AddReferencedObjects(Collector);
    Collector.AddReferencedObject(PreviewMaterial, "FMaterialEditorWidget::PreviewMaterial");
}

void FMaterialEditorWidget::EnsureContext()
{
    if (!NodeEditorContext)
    {
        ed::Config Config;
        Config.SettingsFile = "Settings/MaterialGraphEditor.json";
        NodeEditorContext   = ed::CreateEditor(&Config);
    }
}

void FMaterialEditorWidget::DestroyContext()
{
    if (NodeEditorContext)
    {
        ed::DestroyEditor(NodeEditorContext);
        NodeEditorContext = nullptr;
    }
}

UMaterial* FMaterialEditorWidget::GetMaterial() const
{
    return Cast<UMaterial>(EditedObject);
}

void FMaterialEditorWidget::RenderToolbar(UMaterial* Material)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 5.0f));

    // Two primary actions: Compile updates the live preview; Save persists source + runtime.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.46f, 0.36f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.62f, 0.48f, 0.20f, 1.0f));
    if (ImGui::Button("Compile")) CompilePreview(Material);
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Compile the graph and refresh the preview. Does not save.");

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.50f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.64f, 0.34f, 1.0f));
    if (ImGui::Button(IsDirty() ? "Save *" : "Save")) SaveAll(Material);
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save graph source, compile, and bind the runtime shader to this material.");

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    ImGui::BeginDisabled(UndoStack.size() <= 1);
    if (ImGui::Button("Undo")) UndoGraphEdit();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(RedoStack.empty());
    if (ImGui::Button("Redo")) RedoGraphEdit();
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Add Node"))
    {
        PendingNewNodePosition = ImVec2(160.0f, 120.0f);
        AddNodeSearchBuf[0]    = 0;
        ImGui::OpenPopup("MaterialAddNodePopup");
    }

    ImGui::SameLine();
    bool bAutoPreview = Material->GetGraphDocument().bAutoPreview;
    if (ImGui::Checkbox("Live", &bAutoPreview))
    {
        Material->GetGraphDocument().bAutoPreview = bAutoPreview;
        MarkDirty();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Auto re-compile the preview on every edit.");

    // Right-aligned compile status badge.
    if (!LastCompileStatus.empty() || !LastCompileError.empty())
    {
        const char*  Status = !LastCompileError.empty() ? "Compile Error" : (bLastCompileOk ? "Compiled" : "Ready");
        const ImVec4 Col    = !LastCompileError.empty() ? ImVec4(0.95f, 0.40f, 0.32f, 1.0f)
                : bLastCompileOk ? ImVec4(0.45f, 0.90f, 0.55f, 1.0f)
                : ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
        const float TextW = ImGui::CalcTextSize(Status).x;
        ImGui::SameLine(max(0.0f, ImGui::GetWindowWidth() - TextW - 24.0f));
        ImGui::TextColored(Col, "%s", Status);
    }

    ImGui::PopStyleVar();
    ImGui::Separator();

    RenderAddNodePopup(Material);
}

namespace
{
    const char* ValueTypeLabel(EMaterialValueType Type)
    {
        switch (Type)
        {
        case EMaterialValueType::Float:
            return "Float";
        case EMaterialValueType::Float2:
            return "Float2";
        case EMaterialValueType::Float3:
            return "Float3";
        case EMaterialValueType::Float4:
            return "Float4";
        case EMaterialValueType::Color:
            return "Color";
        case EMaterialValueType::Bool:
            return "Bool";
        case EMaterialValueType::Texture2D:
            return "Texture2D";
        }
        return "Float";
    }

    const char* BlendModeLabel(EBlendMode Mode)
    {
        switch (Mode)
        {
        case EBlendMode::Opaque:
            return "Opaque";
        case EBlendMode::Masked:
            return "Masked";
        case EBlendMode::Translucent:
            return "Translucent";
        case EBlendMode::Additive:
            return "Additive";
        case EBlendMode::Modulate:
            return "Modulate";
        default:
            return "Opaque";
        }
    }

    const char* ShadingModelLabel(EMaterialShadingModel Model)
    {
        switch (Model)
        {
        case EMaterialShadingModel::Unlit:
            return "Unlit";
        case EMaterialShadingModel::DefaultLit:
            return "Default Lit";
        case EMaterialShadingModel::Phong:
            return "Phong";
        case EMaterialShadingModel::Lambert:
            return "Lambert";
        default:
            return "Default Lit";
        }
    }

    // 생성된 HLSL 파일을 디스크에서 읽어 뷰어에 표시. 패널이 펼쳐졌을 때만 호출(경로 변경 시 1회).
    FString ReadGeneratedHlsl(const FString& ProjectRelativePath)
    {
        const std::filesystem::path AbsolutePath = std::filesystem::path(FPaths::RootDir()) / FPaths::ToWide(ProjectRelativePath);
        std::ifstream               File(AbsolutePath);
        if (!File.is_open())
        {
            return FString("// Generated shader not found on disk:\n// ") + ProjectRelativePath;
        }
        std::stringstream Buffer;
        Buffer << File.rdbuf();
        return Buffer.str();
    }
}

void FMaterialEditorWidget::RenderLeftPanel(UMaterial* Material)
{
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.24f, 0.26f, 0.32f, 1.0f));
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderSettingsPanel(Material);
    }
    if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderParametersPanel(Material);
    }
    if (ImGui::CollapsingHeader("Palette", ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderPalettePanel(Material);
    }
    ImGui::PopStyleColor();
}

void FMaterialEditorWidget::RenderSettingsPanel(UMaterial* Material)
{
    FMaterialGraphDocument& Doc      = Material->GetGraphDocument();
    FMaterialSettings&      Settings = Material->GetMaterialSettings();

    // Labels are drawn above each control so they never clip in the narrow sidebar.
    auto LabeledCombo = [](const char* Label, const char* Preview) -> bool
    {
        ImGui::TextUnformatted(Label);
        ImGui::SetNextItemWidth(-FLT_MIN);
        FString Id = FString("##") + Label;
        return ImGui::BeginCombo(Id.c_str(), Preview);
    };

    if (LabeledCombo("Target", ToString(Doc.Target)))
    {
        const EMaterialGraphTarget Targets[] = {
            EMaterialGraphTarget::Surface, EMaterialGraphTarget::Decal, EMaterialGraphTarget::PostProcess,
            EMaterialGraphTarget::ParticleSprite, EMaterialGraphTarget::ParticleMesh,
        };
        for (EMaterialGraphTarget Candidate : Targets)
        {
            const bool bSelected = (Candidate == Doc.Target);
            if (ImGui::Selectable(ToString(Candidate), bSelected))
            {
                Doc.Target = Candidate;
                WorkingGraph.RebuildOutputPinsForDomain(Candidate);
                CommitGraphEdit();
            }
            if (bSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    EMaterialDomain Domain      = Material->GetDomain();
    const char*     DomainLabel = Domain == EMaterialDomain::Surface ? "Surface" : Domain == EMaterialDomain::PostProcess ? "PostProcess" : Domain == EMaterialDomain::UI ? "UI" : "Decal";
    if (LabeledCombo("Domain", DomainLabel))
    {
        struct
        {
            EMaterialDomain Domain;
            const char*     Label;
        } Choices[] = {
            { EMaterialDomain::Surface, "Surface" }, { EMaterialDomain::PostProcess, "PostProcess" },
            { EMaterialDomain::UI, "UI" }, { EMaterialDomain::Decal, "Decal" },
        };
        for (const auto& Choice : Choices)
        {
            if (ImGui::Selectable(Choice.Label, Domain == Choice.Domain))
            {
                Material->SetDomainBlend(Choice.Domain, Material->GetBlendMode());
                MarkDirty();
            }
        }
        ImGui::EndCombo();
    }

    EBlendMode Blend = Material->GetBlendMode();
    if (LabeledCombo("Blend Mode", BlendModeLabel(Blend)))
    {
        const EBlendMode Modes[] = { EBlendMode::Opaque, EBlendMode::Masked, EBlendMode::Translucent, EBlendMode::Additive, EBlendMode::Modulate };
        for (EBlendMode Mode : Modes)
        {
            if (ImGui::Selectable(BlendModeLabel(Mode), Blend == Mode))
            {
                Material->SetDomainBlend(Material->GetDomain(), Mode);
                MarkDirty();
            }
        }
        ImGui::EndCombo();
    }

    if (LabeledCombo("Shading Model", ShadingModelLabel(Settings.ShadingModel)))
    {
        const EMaterialShadingModel Models[] = { EMaterialShadingModel::Unlit, EMaterialShadingModel::DefaultLit, EMaterialShadingModel::Phong, EMaterialShadingModel::Lambert };
        for (EMaterialShadingModel Model : Models)
        {
            if (ImGui::Selectable(ShadingModelLabel(Model), Settings.ShadingModel == Model))
            {
                Settings.ShadingModel = Model;
                MarkDirty();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    bool bTwoSided = Material->IsTwoSided();
    if (ImGui::Checkbox("Two Sided", &bTwoSided))
    {
        Material->SetTwoSided(bTwoSided);
        MarkDirty();
    }
    if (ImGui::Checkbox("Receive Lighting", &Settings.bReceiveLighting)) MarkDirty();
    if (ImGui::Checkbox("Cast Shadow", &Settings.bCastShadow)) MarkDirty();

    if (Blend == EBlendMode::Masked)
    {
        ImGui::TextUnformatted("Mask Clip");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderFloat("##MaskClip", &Settings.OpacityMaskClipValue, 0.0f, 1.0f, "%.3f")) MarkDirty();
    }
}

void FMaterialEditorWidget::RenderParametersPanel(UMaterial* Material)
{
    FMaterialGraphDocument&               Doc  = Material->GetGraphDocument();
    TArray<FMaterialParameterDefinition>& Defs = Material->GetParameterDefinitions();

    // Adds both a definition and a matching parameter node, wired to a stable name.
    auto AddParameter = [&](const char* BaseName, EMaterialValueType Type, EMaterialGraphNodeType NodeType, const FVector4& Default)
    {
        FString Name    = BaseName;
        int32   Suffix  = 1;
        bool    bUnique = false;
        while (!bUnique)
        {
            bUnique = true;
            for (const FMaterialParameterDefinition& D : Defs) if (D.Name == Name)
            {
                Name    = FString(BaseName) + std::to_string(Suffix++);
                bUnique = false;
                break;
            }
        }
        FMaterialParameterDefinition Def;
        Def.Name         = Name;
        Def.Type         = Type;
        Def.DefaultValue = Default;
        Defs.push_back(Def);
        if (FMaterialGraphNode* Node = WorkingGraph.AddNodeOfType(NodeType, PendingNewNodePosition.x, PendingNewNodePosition.y, Doc.Target))
        {
            Node->ParameterName = Name;
            Node->Value         = Default;
        }
        CommitGraphEdit();
    };

    if (Defs.empty())
    {
        ImGui::TextDisabled("No exposed parameters.");
    }

    int32 RemoveIndex = -1;
    for (int32 i = 0; i < static_cast<int32>(Defs.size()); ++i)
    {
        FMaterialParameterDefinition& Def = Defs[i];
        ImGui::PushID(i);
        const bool bOpen = ImGui::TreeNodeEx(
            "##param",
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding,
            "%s  (%s)",
            Def.Name.empty() ? "(unnamed)" : Def.Name.c_str(),
            ValueTypeLabel(Def.Type)
        );
        if (bOpen)
        {
            char NameBuf[128];
            std::snprintf(NameBuf, sizeof(NameBuf), "%s", Def.Name.c_str());
            ImGui::TextUnformatted("Name");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##Name", NameBuf, sizeof(NameBuf)))
            {
                Def.Name = NameBuf;
                MarkDirty();
            }

            ImGui::TextUnformatted("Default");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (Def.Type == EMaterialValueType::Float)
            {
                if (ImGui::DragFloat("##Default", &Def.DefaultValue.X, 0.01f)) MarkDirty();
            }
            else if (Def.Type == EMaterialValueType::Color)
            {
                if (ImGui::ColorEdit4("##Default", &Def.DefaultValue.X, ImGuiColorEditFlags_NoInputs)) MarkDirty();
            }
            else if (Def.Type == EMaterialValueType::Texture2D)
            {
                if (InputFString("##Default", Def.DefaultTexturePath, 512)) MarkDirty();
            }
            else
            {
                if (ImGui::DragFloat4("##Default", &Def.DefaultValue.X, 0.01f)) MarkDirty();
            }

            char GroupBuf[96];
            std::snprintf(GroupBuf, sizeof(GroupBuf), "%s", Def.Group.c_str());
            ImGui::TextUnformatted("Group");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##Group", GroupBuf, sizeof(GroupBuf)))
            {
                Def.Group = GroupBuf;
                MarkDirty();
            }
            if (ImGui::Checkbox("Expose to Instance", &Def.bExposeToInstance)) MarkDirty();

            if (ImGui::SmallButton("Remove")) RemoveIndex = i;
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (RemoveIndex >= 0)
    {
        Defs.erase(Defs.begin() + RemoveIndex);
        MarkDirty();
    }

    ImGui::Spacing();
    if (ImGui::SmallButton("+ Scalar")) AddParameter("Scalar", EMaterialValueType::Float, EMaterialGraphNodeType::ScalarParameter, FVector4(0, 0, 0, 0));
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Vector")) AddParameter("Vector", EMaterialValueType::Float4, EMaterialGraphNodeType::VectorParameter, FVector4(0, 0, 0, 1));
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Color")) AddParameter("Color", EMaterialValueType::Color, EMaterialGraphNodeType::ColorParameter, FVector4(1, 1, 1, 1));
}

void FMaterialEditorWidget::RenderPalettePanel(UMaterial* Material)
{
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##palettesearch", "Search nodes...", PaletteSearchBuf, sizeof(PaletteSearchBuf));

    struct FPaletteItem
    {
        const char*            Category;
        EMaterialGraphNodeType Type;
    };
    static const FPaletteItem Items[] = {
        { "Constants", EMaterialGraphNodeType::ConstantFloat },
        { "Constants", EMaterialGraphNodeType::ConstantFloat2 },
        { "Constants", EMaterialGraphNodeType::ConstantFloat3 },
        { "Constants", EMaterialGraphNodeType::ConstantFloat4 },
        { "Parameters", EMaterialGraphNodeType::ScalarParameter },
        { "Parameters", EMaterialGraphNodeType::VectorParameter },
        { "Parameters", EMaterialGraphNodeType::ColorParameter },
        { "Texture", EMaterialGraphNodeType::TextureObject },
        { "Texture", EMaterialGraphNodeType::TextureSample },
        { "Math", EMaterialGraphNodeType::Add },
        { "Math", EMaterialGraphNodeType::Subtract },
        { "Math", EMaterialGraphNodeType::Multiply },
        { "Math", EMaterialGraphNodeType::Divide },
        { "Math", EMaterialGraphNodeType::Lerp },
        { "Math", EMaterialGraphNodeType::Clamp },
        { "Math", EMaterialGraphNodeType::Power },
        { "Math", EMaterialGraphNodeType::OneMinus },
        { "Math", EMaterialGraphNodeType::Saturate },
        { "Math", EMaterialGraphNodeType::ConstantBiasScale },
        { "Math", EMaterialGraphNodeType::Distance },
        { "Math", EMaterialGraphNodeType::Normalize },
        { "Math", EMaterialGraphNodeType::Dot },
        { "Math", EMaterialGraphNodeType::Cross },
        { "Math", EMaterialGraphNodeType::Append },
        { "Math", EMaterialGraphNodeType::ComponentMask },
        { "Coordinates", EMaterialGraphNodeType::TexCoord },
        { "Coordinates", EMaterialGraphNodeType::Panner },
        { "Coordinates", EMaterialGraphNodeType::Time },
        { "Particle", EMaterialGraphNodeType::VertexColor },
        { "Particle", EMaterialGraphNodeType::ParticleColor },
        { "Particle", EMaterialGraphNodeType::ParticleSubUV },
        { "Particle", EMaterialGraphNodeType::DynamicParameter },
        { "Utility", EMaterialGraphNodeType::Reroute },
        { "Utility", EMaterialGraphNodeType::Comment },
    };

    const char* CurrentCategory = nullptr;
    const bool  bSearching      = PaletteSearchBuf[0] != 0;
    for (const FPaletteItem& Item : Items)
    {
        const char* Label = NodeLabel(Item.Type);
        if (bSearching && !StringContainsInsensitive(Label, PaletteSearchBuf)) continue;

        if (!CurrentCategory || std::strcmp(CurrentCategory, Item.Category) != 0)
        {
            CurrentCategory = Item.Category;
            ImGui::Spacing();
            ImGui::TextDisabled("%s", Item.Category);
            ImGui::Separator();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, NodeHeaderColor(Item.Type));
        ImGui::Bullet();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Selectable(Label))
        {
            const ImVec2 Spawn = ImVec2(PendingNewNodePosition.x + 30.0f, PendingNewNodePosition.y + 30.0f);
            if (WorkingGraph.AddNodeOfType(Item.Type, Spawn.x, Spawn.y, Material->GetGraphDocument().Target))
            {
                PendingNewNodePosition = Spawn;
                CommitGraphEdit();
            }
        }
    }
}

void FMaterialEditorWidget::RenderGraphCanvas(UMaterial* Material)
{
    if (!NodeEditorContext) return;

    FScopedNodeEditorCurrent Scope(NodeEditorContext);
    ed::Begin("MaterialGraph");

    ProcessQueuedNodeEditorCommands();

    for (FMaterialGraphNode& Node : WorkingGraph.Nodes)
    {
        if (!bPositionsPushed)
        {
            ed::SetNodePosition(ToNodeId(Node.NodeId), ImVec2(Node.PosX, Node.PosY));
        }

        // Comment 는 ed::Group 으로 — 내부에 겹친 다른 노드들을 같이 드래그/리사이즈한다 (UE BP Comment).
        if (Node.Type == EMaterialGraphNodeType::Comment)
        {
            ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(118, 96, 30, 70));
            ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(224, 198, 96, 200));
            ed::BeginNode(ToNodeId(Node.NodeId));
            ImGui::PushID(static_cast<int>(Node.NodeId));
            ImGui::TextColored(ImVec4(1.0f, 0.93f, 0.5f, 1.0f), "%s", Node.ParameterName.empty() ? "Comment" : Node.ParameterName.c_str());
            const ImVec2 GroupSize(max(120.0f, Node.Value.X), max(60.0f, Node.Value.Y));
            ed::Group(GroupSize);
            ImGui::PopID();
            ed::EndNode();

            const ImVec2 ActualSize = ed::GetNodeSize(ToNodeId(Node.NodeId));
            if (ActualSize.x > 1.0f && ActualSize.y > 1.0f &&
                (std::fabs(Node.Value.X - ActualSize.x) > 0.5f || std::fabs(Node.Value.Y - ActualSize.y) > 0.5f))
            {
                Node.Value.X = ActualSize.x;
                Node.Value.Y = ActualSize.y;
                MarkDirty();
            }
            ed::PopStyleColor(2);
            continue;
        }

        ed::BeginNode(ToNodeId(Node.NodeId));
        RenderNodeBody(Node);
        ed::EndNode();
    }

    bPositionsPushed = true;

    for (const FMaterialGraphLink& Link : WorkingGraph.Links)
    {
        ImVec4 LinkColor(0.75f, 0.75f, 0.75f, 1.0f);
        if (const FMaterialGraphPin* From = WorkingGraph.FindPin(Link.FromPinId))
        {
            LinkColor = PinTypeColor(From->Type);
        }
        ed::Link(ToLinkId(Link.LinkId), ToPinId(Link.FromPinId), ToPinId(Link.ToPinId), LinkColor);
    }

    if (bPendingInitialContentFit && !WorkingGraph.Nodes.empty())
    {
        ed::NavigateToContent(0.0f);
        bPendingInitialContentFit = false;
    }

    if (ed::BeginCreate())
    {
        ed::PinId StartId, EndId;
        if (ed::QueryNewLink(&StartId, &EndId))
        {
            if (StartId && EndId)
            {
                uint32 FromPinId = 0;
                uint32 ToPinId   = 0;
                if (WorkingGraph.CanLinkPins(PinIdToU32(StartId), PinIdToU32(EndId), &FromPinId, &ToPinId))
                {
                    if (ed::AcceptNewItem())
                    {
                        WorkingGraph.AddLink(FromPinId, ToPinId);
                        CommitGraphEdit();
                    }
                }
                else
                {
                    ed::RejectNewItem(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), 2.0f);
                }
            }
        }

        ed::PinId NewNodePinId = 0;
        if (ed::QueryNewNode(&NewNodePinId))
        {
            if (NewNodePinId && ed::AcceptNewItem(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), 1.5f))
            {
                PendingPinSpawnPinId   = PinIdToU32(NewNodePinId);
                PendingPinSpawnPos     = ed::ScreenToCanvas(ImGui::GetMousePos());
                PendingNewNodePosition = PendingPinSpawnPos;
                PinSpawnSearchBuf[0]   = 0;
                bShowPinSpawnMenu      = true; // popup is opened inside the Suspend block below
            }
        }
    }
    ed::EndCreate();

    if (ed::BeginDelete())
    {
        ed::LinkId DeletedLink;
        while (ed::QueryDeletedLink(&DeletedLink))
        {
            if (ed::AcceptDeletedItem())
            {
                if (WorkingGraph.RemoveLink(LinkIdToU32(DeletedLink))) CommitGraphEdit();
            }
        }
        ed::NodeId DeletedNode;
        bool       bDeletedNode = false;
        while (ed::QueryDeletedNode(&DeletedNode))
        {
            if (ed::AcceptDeletedItem())
            {
                bDeletedNode |= WorkingGraph.RemoveNode(NodeIdToU32(DeletedNode));
            }
        }
        if (bDeletedNode) CommitGraphEdit();
    }
    ed::EndDelete();

    // All context menus / spawn popups are opened AND begun inside one Suspend/Resume block so
    // ImGui draws them in screen space (not the node-editor's transformed canvas space).
    ed::NodeId ContextNodeId = 0;
    ed::LinkId ContextLinkId = 0;
    ed::Suspend();
    if (ed::ShowNodeContextMenu(&ContextNodeId))
    {
        ImGui::OpenPopup("MaterialNodeMenu");
    }
    else if (ed::ShowLinkContextMenu(&ContextLinkId))
    {
        ImGui::OpenPopup("MaterialLinkMenu");
    }
    else if (ed::ShowBackgroundContextMenu())
    {
        PendingNewNodePosition = ed::ScreenToCanvas(ImGui::GetMousePos());
        AddNodeSearchBuf[0]    = 0;
        ImGui::OpenPopup("MaterialCanvasAddNode");
    }

    if (ImGui::BeginPopup("MaterialNodeMenu"))
    {
        if (ImGui::MenuItem("Duplicate")) bQueuedDuplicateSelected = true;
        if (ImGui::MenuItem("Comment Selection")) bQueuedGroupSelected = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
        {
            if (ContextNodeId && WorkingGraph.RemoveNode(NodeIdToU32(ContextNodeId))) CommitGraphEdit();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("MaterialLinkMenu"))
    {
        if (ImGui::MenuItem("Break Link"))
        {
            if (ContextLinkId && WorkingGraph.RemoveLink(LinkIdToU32(ContextLinkId))) CommitGraphEdit();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("MaterialCanvasAddNode"))
    {
        RenderAddNodeMenuBody(Material);
        ImGui::EndPopup();
    }

    if (bShowPinSpawnMenu)
    {
        ImGui::OpenPopup("MaterialPinSpawnPopup");
        bShowPinSpawnMenu = false;
    }
    if (ImGui::BeginPopup("MaterialPinSpawnPopup"))
    {
        RenderPinSpawnMenuBody(Material);
        ImGui::EndPopup();
    }
    ed::Resume();

    for (FMaterialGraphNode& Node : WorkingGraph.Nodes)
    {
        const ImVec2 Pos = ed::GetNodePosition(ToNodeId(Node.NodeId));
        if (Node.PosX != Pos.x || Node.PosY != Pos.y)
        {
            Node.PosX = Pos.x;
            Node.PosY = Pos.y;
            MarkDirty();
        }
    }

    {
        ed::NodeId SelectedNodes[1];
        const int  Count = ed::GetSelectedNodes(SelectedNodes, 1);
        SelectedNodeId   = (Count > 0) ? NodeIdToU32(SelectedNodes[0]) : 0;
    }

    ed::End();
}

void FMaterialEditorWidget::RenderNodeBody(FMaterialGraphNode& Node)
{
    ImGui::PushID(static_cast<int>(Node.NodeId));

    // ── Header: category-colored title ──
    const ImVec4  Header = NodeHeaderColor(Node.Type);
    const FString Title  = Node.DisplayName != FName::None ? Node.DisplayName.ToString() : FString(NodeLabel(Node.Type));
    ImGui::TextColored(Header, "%s", Title.c_str());
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(Header.x, Header.y, Header.z, 0.45f), "  %s", NodeCategoryLabel(Node.Type));
    ImGui::Dummy(ImVec2(0.0f, 3.0f));

    int32 NumInput  = 0;
    int32 NumOutput = 0;
    for (const FMaterialGraphPin& Pin : Node.Pins)
    {
        if (Pin.Kind == EMaterialGraphPinKind::Input) ++NumInput;
        else ++NumOutput;
    }

    const bool bHasValueEditor =
            Node.Type == EMaterialGraphNodeType::ConstantFloat || Node.Type == EMaterialGraphNodeType::ConstantFloat2 ||
            Node.Type == EMaterialGraphNodeType::ConstantFloat3 || Node.Type == EMaterialGraphNodeType::ConstantFloat4 ||
            Node.Type == EMaterialGraphNodeType::ColorParameter || Node.Type == EMaterialGraphNodeType::VectorParameter ||
            Node.Type == EMaterialGraphNodeType::ScalarParameter || Node.Type == EMaterialGraphNodeType::ComponentMask ||
            Node.Type == EMaterialGraphNodeType::TextureObject || Node.Type == EMaterialGraphNodeType::TexCoord ||
            Node.Type == EMaterialGraphNodeType::ParticleSubUV || Node.Type == EMaterialGraphNodeType::ConstantBiasScale;

    // ── Left column: input pins + inline value editor ──
    ImGui::BeginGroup();
    for (FMaterialGraphPin& Pin : Node.Pins)
    {
        if (Pin.Kind != EMaterialGraphPinKind::Input) continue;
        ed::BeginPin(ToPinId(Pin.PinId), ed::PinKind::Input);
        ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
        DrawPinIcon(PinTypeColor(Pin.Type), IsPinLinked(WorkingGraph, Pin.PinId));
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::TextColored(PinTypeColor(Pin.Type), "%s", Pin.DisplayName.ToString().c_str());
        ed::EndPin();
    }
    if (bHasValueEditor)
    {
        RenderNodeValueEditor(Node, true);
    }
    ImGui::EndGroup();

    // ── Right column: output pins ──
    if (NumOutput > 0)
    {
        if (NumInput > 0 || bHasValueEditor) ImGui::SameLine(0.0f, 20.0f);
        ImGui::BeginGroup();
        for (FMaterialGraphPin& Pin : Node.Pins)
        {
            if (Pin.Kind != EMaterialGraphPinKind::Output) continue;
            ed::BeginPin(ToPinId(Pin.PinId), ed::PinKind::Output);
            ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
            ImGui::TextColored(PinTypeColor(Pin.Type), "%s", Pin.DisplayName.ToString().c_str());
            ImGui::SameLine(0.0f, 6.0f);
            DrawPinIcon(PinTypeColor(Pin.Type), IsPinLinked(WorkingGraph, Pin.PinId));
            ed::EndPin();
        }
        ImGui::EndGroup();
    }

    ImGui::PopID();
}

void FMaterialEditorWidget::RenderNodeValueEditor(FMaterialGraphNode& Node, bool bCompact)
{
    const float Width = bCompact ? 130.0f : -1.0f;
    ImGui::PushID(static_cast<int>(Node.NodeId) * 7 + 3);

    switch (Node.Type)
    {
    case EMaterialGraphNodeType::ScalarParameter:
    case EMaterialGraphNodeType::VectorParameter:
    case EMaterialGraphNodeType::ColorParameter:
        ImGui::TextDisabled("[%s]", Node.ParameterName.empty() ? "param" : Node.ParameterName.c_str());
        if (Node.Type == EMaterialGraphNodeType::ColorParameter)
        {
            if (bCompact) DrawColorSwatch(Node.Value);
            else if (ImGui::ColorEdit4("##v", &Node.Value.X, ImGuiColorEditFlags_NoInputs)) MarkDirty();
        }
        else if (Node.Type == EMaterialGraphNodeType::ScalarParameter)
        {
            ImGui::SetNextItemWidth(Width);
            if (ImGui::DragFloat("##v", &Node.Value.X, 0.01f)) MarkDirty();
        }
        else if (!bCompact) // VectorParameter, full editor in inspector only
        {
            if (ImGui::DragFloat4("##v", &Node.Value.X, 0.01f)) MarkDirty();
        }
        break;
    case EMaterialGraphNodeType::ConstantFloat:
        ImGui::SetNextItemWidth(Width);
        if (ImGui::DragFloat("##v", &Node.Value.X, 0.01f)) MarkDirty();
        break;
    case EMaterialGraphNodeType::ConstantFloat2:
        ImGui::SetNextItemWidth(Width);
        if (ImGui::DragFloat2("##v", &Node.Value.X, 0.01f)) MarkDirty();
        break;
    case EMaterialGraphNodeType::ConstantFloat3:
        if (bCompact)
        {
            DrawColorSwatch(Node.Value);
        }
        else
        {
            if (ImGui::ColorEdit3("##v", &Node.Value.X, ImGuiColorEditFlags_NoInputs)) MarkDirty();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat3("##vd", &Node.Value.X, 0.01f)) MarkDirty();
        }
        break;
    case EMaterialGraphNodeType::ConstantFloat4:
        ImGui::SetNextItemWidth(Width);
        if (ImGui::DragFloat4("##v", &Node.Value.X, 0.01f)) MarkDirty();
        break;
    case EMaterialGraphNodeType::ConstantBiasScale:
        ImGui::SetNextItemWidth(Width);
        if (ImGui::DragFloat2("Bias/Scale", &Node.Value.X, 0.01f)) MarkDirty();
        break;
    case EMaterialGraphNodeType::TexCoord:
        ImGui::SetNextItemWidth(Width);
        if (ImGui::DragFloat("UV Ch", &Node.Value.X, 1.0f, 0.0f, 3.0f, "%.0f")) MarkDirty();
        break;
    case EMaterialGraphNodeType::ParticleSubUV:
        ImGui::SetNextItemWidth(Width);
        if (ImGui::DragFloat2("Cols/Rows", &Node.Value.X, 1.0f, 1.0f, 64.0f, "%.0f")) MarkDirty();
        break;
    case EMaterialGraphNodeType::ComponentMask:
    {
        char MaskBuf[8] = {};
        std::snprintf(MaskBuf, sizeof(MaskBuf), "%s", Node.Mask.c_str());
        ImGui::SetNextItemWidth(Width);
        if (ImGui::InputText("Mask", MaskBuf, sizeof(MaskBuf)))
        {
            Node.Mask = MaskBuf;
            MarkDirty();
        }
        break;
    }
    case EMaterialGraphNodeType::TextureObject:
    {
        std::filesystem::path P(Node.TexturePath);
        const FString         Base = Node.TexturePath.empty() ? FString("(no texture)") : P.filename().string();
        ImGui::TextDisabled("%s", Base.c_str());
        break;
    }
    default:
        break;
    }

    ImGui::PopID();
}

void FMaterialEditorWidget::RenderDetailsPanel(UMaterial* Material)
{
    if (SelectedNodeId != 0)
    {
        if (FMaterialGraphNode* Node = WorkingGraph.FindNode(SelectedNodeId))
        {
            RenderNodeInspector(Material, *Node);
            return;
        }
        SelectedNodeId = 0;
    }

    ImGui::TextDisabled("Select a node to edit its details.");
    ImGui::Separator();
    ImGui::Text("Nodes: %zu", WorkingGraph.Nodes.size());
    ImGui::Text("Links: %zu", WorkingGraph.Links.size());
    ImGui::Spacing();
    ImGui::TextDisabled("Saved:   %s", Material->GetGraphDocument().LastSavedGraphHash.c_str());
    ImGui::TextDisabled("Working: %s", ComputeMaterialGraphStructuralHash(WorkingGraph).c_str());
    if (PreviewMaterial)
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.45f, 0.95f, 0.55f, 1.0f), "Preview material active");
    }

    ImGui::Spacing();
    if (ImGui::Button("Frame Graph"))
    {
        FScopedNodeEditorCurrent Scope(NodeEditorContext);
        ed::NavigateToContent(0.25f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Comment Selected")) GroupSelectedNodesAsComment();
}

void FMaterialEditorWidget::RenderNodeInspector(UMaterial* Material, FMaterialGraphNode& Node)
{
    const ImVec4 Header = NodeHeaderColor(Node.Type);
    ImGui::TextColored(Header, "%s", Node.DisplayName != FName::None ? Node.DisplayName.ToString().c_str() : NodeLabel(Node.Type));
    ImGui::SameLine();
    ImGui::TextDisabled("#%u  %s", Node.NodeId, NodeCategoryLabel(Node.Type));
    ImGui::Separator();

    // Parameter nodes: name binds the generated cbuffer field; keep it editable here.
    if (Node.Type == EMaterialGraphNodeType::ScalarParameter ||
        Node.Type == EMaterialGraphNodeType::VectorParameter ||
        Node.Type == EMaterialGraphNodeType::ColorParameter)
    {
        char NameBuf[128];
        std::snprintf(NameBuf, sizeof(NameBuf), "%s", Node.ParameterName.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("Parameter Name", NameBuf, sizeof(NameBuf)))
        {
            Node.ParameterName = NameBuf;
            MarkDirty();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Default Value");
        RenderNodeValueEditor(Node, false);
    }
    else if (Node.Type == EMaterialGraphNodeType::TextureObject ||
        Node.Type == EMaterialGraphNodeType::TextureSample)
    {
        if (Node.Type == EMaterialGraphNodeType::TextureObject)
        {
            ImGui::SetNextItemWidth(-1);
            if (InputFString("Texture Path", Node.TexturePath, 512)) MarkDirty();
        }
        int32 Slot = static_cast<int32>(Node.TextureSlot);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("Slot", MaterialTextureSlot::ToString(Slot).c_str()))
        {
            for (int32 s = 0; s < static_cast<int32>(EMaterialTextureSlot::Max); ++s)
            {
                if (ImGui::Selectable(MaterialTextureSlot::ToString(s).c_str(), Slot == s))
                {
                    Node.TextureSlot = static_cast<EMaterialTextureSlot>(s);
                    MarkDirty();
                }
            }
            ImGui::EndCombo();
        }
    }
    else if (Node.Type == EMaterialGraphNodeType::Comment)
    {
        ImGui::SetNextItemWidth(-1);
        if (InputFString("Comment", Node.ParameterName)) MarkDirty();
        if (ImGui::DragFloat2("Size", &Node.Value.X, 1.0f, 60.0f, 4000.0f, "%.0f")) MarkDirty();
    }
    else
    {
        RenderNodeValueEditor(Node, false);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Pins");
    for (const FMaterialGraphPin& Pin : Node.Pins)
    {
        const bool bInput = Pin.Kind == EMaterialGraphPinKind::Input;
        ImGui::PushStyleColor(ImGuiCol_Text, PinTypeColor(Pin.Type));
        ImGui::Bullet();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s %s", bInput ? "In " : "Out", Pin.DisplayName.ToString().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s%s)", ToString(Pin.Type), IsPinLinked(WorkingGraph, Pin.PinId) ? ", linked" : "");
    }

    ImGui::Spacing();
    if (ImGui::Button("Delete Node"))
    {
        if (WorkingGraph.RemoveNode(Node.NodeId))
        {
            SelectedNodeId = 0;
            CommitGraphEdit();
        }
    }
}

void FMaterialEditorWidget::RenderCompilerPanel(UMaterial* Material)
{
    ImGui::TextUnformatted("Compiler Results");
    ImGui::Separator();

    if (!LastCompileError.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.42f, 0.32f, 1.0f));
        ImGui::TextWrapped("Error: %s", LastCompileError.c_str());
        ImGui::PopStyleColor();
    }
    else if (!LastCompileStatus.empty())
    {
        ImGui::TextWrapped("%s", LastCompileStatus.c_str());
    }
    else
    {
        ImGui::TextDisabled("Not compiled yet.");
    }

    const FMaterialCompileRecord& Record = Material->GetLastCompileRecord();
    if (!Record.GeneratedShaderPath.empty())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("Shader: %s", Record.GeneratedShaderPath.c_str());
        ImGui::TextDisabled("Hash:   %s", Record.SourceHash.c_str());
        ImGui::TextColored(
            Record.bSucceeded ? ImVec4(0.45f, 0.9f, 0.55f, 1.0f) : ImVec4(0.95f, 0.45f, 0.35f, 1.0f),
            Record.bSucceeded ? "Runtime: up to date" : "Runtime: failed"
        );
    }

    if (ImGui::CollapsingHeader("Generated HLSL"))
    {
        const FString& Path = Material->GetGraphDocument().LastCompiledShaderPath;
        if (Path.empty())
        {
            ImGui::TextDisabled("Apply Compile to generate a shader.");
        }
        else
        {
            if (Path != HlslViewPath)
            {
                HlslViewPath = Path;
                HlslViewText = ReadGeneratedHlsl(Path);
            }
            if (ImGui::SmallButton("Reload")) HlslViewText = ReadGeneratedHlsl(Path);
            ImGui::InputTextMultiline(
                "##hlsl",
                HlslViewText.data(),
                HlslViewText.size() + 1,
                ImVec2(-1, 200),
                ImGuiInputTextFlags_ReadOnly
            );
        }
    }
}

void FMaterialEditorWidget::RenderAddNodePopup(UMaterial* Material)
{
    if (ImGui::BeginPopup("MaterialAddNodePopup"))
    {
        RenderAddNodeMenuBody(Material);
        ImGui::EndPopup();
    }
}

void FMaterialEditorWidget::RenderAddNodeMenuBody(UMaterial* Material)
{
    ImGui::SetNextItemWidth(260.0f);
    ImGui::InputTextWithHint("##Search", "Search nodes...", AddNodeSearchBuf, sizeof(AddNodeSearchBuf));
    ImGui::Separator();

    struct FMenuItem
    {
        const char*            Category;
        EMaterialGraphNodeType Type;
    };
    static const FMenuItem Items[] = {
        { "Texture", EMaterialGraphNodeType::TextureObject },
        { "Texture", EMaterialGraphNodeType::TextureSample },
        { "Parameters", EMaterialGraphNodeType::ScalarParameter },
        { "Parameters", EMaterialGraphNodeType::VectorParameter },
        { "Parameters", EMaterialGraphNodeType::ColorParameter },
        { "Constants", EMaterialGraphNodeType::ConstantFloat },
        { "Constants", EMaterialGraphNodeType::ConstantFloat2 },
        { "Constants", EMaterialGraphNodeType::ConstantFloat3 },
        { "Constants", EMaterialGraphNodeType::ConstantFloat4 },
        { "Math", EMaterialGraphNodeType::Add },
        { "Math", EMaterialGraphNodeType::Subtract },
        { "Math", EMaterialGraphNodeType::Multiply },
        { "Math", EMaterialGraphNodeType::Divide },
        { "Math", EMaterialGraphNodeType::Lerp },
        { "Math", EMaterialGraphNodeType::Clamp },
        { "Math", EMaterialGraphNodeType::Power },
        { "Math", EMaterialGraphNodeType::OneMinus },
        { "Math", EMaterialGraphNodeType::Saturate },
        { "Math", EMaterialGraphNodeType::ConstantBiasScale },
        { "Math", EMaterialGraphNodeType::Distance },
        { "Math", EMaterialGraphNodeType::Normalize },
        { "Math", EMaterialGraphNodeType::Dot },
        { "Math", EMaterialGraphNodeType::Cross },
        { "Math", EMaterialGraphNodeType::ComponentMask },
        { "Math", EMaterialGraphNodeType::Append },
        { "Coordinates", EMaterialGraphNodeType::TexCoord },
        { "Coordinates", EMaterialGraphNodeType::Panner },
        { "Coordinates", EMaterialGraphNodeType::Time },
        { "Particle", EMaterialGraphNodeType::VertexColor },
        { "Particle", EMaterialGraphNodeType::ParticleColor },
        { "Particle", EMaterialGraphNodeType::ParticleSubUV },
        { "Particle", EMaterialGraphNodeType::DynamicParameter },
        { "Utility", EMaterialGraphNodeType::Reroute },
        { "Utility", EMaterialGraphNodeType::Comment },
    };

    // When searching, show a flat filtered list. Otherwise group into category submenus.
    if (AddNodeSearchBuf[0] != 0)
    {
        for (const FMenuItem& Item : Items)
        {
            AddNodeMenuItem(Material, Item.Type);
        }
        return;
    }

    const char* CurrentCategory = nullptr;
    bool        bMenuOpen       = false;
    for (const FMenuItem& Item : Items)
    {
        if (!CurrentCategory || std::strcmp(CurrentCategory, Item.Category) != 0)
        {
            if (CurrentCategory && bMenuOpen) ImGui::EndMenu();
            CurrentCategory = Item.Category;
            bMenuOpen       = ImGui::BeginMenu(Item.Category);
        }
        if (bMenuOpen)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, NodeHeaderColor(Item.Type));
            AddNodeMenuItem(Material, Item.Type);
            ImGui::PopStyleColor();
        }
    }
    if (CurrentCategory && bMenuOpen) ImGui::EndMenu();
}

void FMaterialEditorWidget::RenderPinSpawnMenuBody(UMaterial* Material)
{
    ImGui::SetNextItemWidth(260.0f);
    ImGui::InputTextWithHint("##PinSearch", "Search compatible nodes...", PinSpawnSearchBuf, sizeof(PinSpawnSearchBuf));
    ImGui::Separator();
    // Only type-compatible nodes are listed (AddContextNodeMenuItem filters by pin compatibility).
    const EMaterialGraphNodeType Candidates[] = {
        EMaterialGraphNodeType::Multiply, EMaterialGraphNodeType::Add, EMaterialGraphNodeType::Subtract,
        EMaterialGraphNodeType::Divide, EMaterialGraphNodeType::Lerp, EMaterialGraphNodeType::OneMinus,
        EMaterialGraphNodeType::Saturate, EMaterialGraphNodeType::Clamp, EMaterialGraphNodeType::Power,
        EMaterialGraphNodeType::TextureSample, EMaterialGraphNodeType::ComponentMask, EMaterialGraphNodeType::Append,
        EMaterialGraphNodeType::Normalize, EMaterialGraphNodeType::Dot, EMaterialGraphNodeType::Reroute,
        EMaterialGraphNodeType::Output,
    };
    bool bAny = false;
    for (EMaterialGraphNodeType Type : Candidates)
    {
        bAny |= AddContextNodeMenuItem(Material, Type);
    }
    (void)bAny;
}

void FMaterialEditorWidget::SaveGraph(UMaterial* Material)
{
    if (!Material) return;
    Material->EnableGraphMaterial();
    Material->GetGraphDocument().Graph = WorkingGraph;
    Material->MarkGraphSaved();
    if (FMaterialManager::Get().SaveMaterialSourceOnly(Material, Material->GetAssetPathFileName()))
    {
        ClearDirty();
        LastCompileStatus = "Graph source saved.";
        LastCompileError.clear();
    }
    else
    {
        LastCompileError = "Failed to save material graph source.";
    }
}

void FMaterialEditorWidget::CompilePreview(UMaterial* Material)
{
    LastCompileError.clear();
    if (FMaterialManager::Get().CompileMaterialGraphPreview(Material, WorkingGraph, PreviewMaterial, &LastCompileError))
    {
        LastCompileStatus = "Preview compile succeeded. Source asset was not modified.";
        bLastCompileOk    = true;
    }
    else
    {
        bLastCompileOk = false;
    }
}

void FMaterialEditorWidget::ApplyCompile(UMaterial* Material, bool bPersistCompiledState)
{
    LastCompileError.clear();
    if (FMaterialManager::Get().CompileMaterialGraphRuntime(Material, WorkingGraph, bPersistCompiledState, &LastCompileError))
    {
        LastCompileStatus = bPersistCompiledState ? "Runtime compile applied and saved." : "Runtime compile applied. Graph source was not saved.";
        bLastCompileOk    = true;
    }
    else
    {
        bLastCompileOk = false;
    }
}

void FMaterialEditorWidget::SaveAll(UMaterial* Material)
{
    SaveGraph(Material);
    ApplyCompile(Material, true);
}

void FMaterialEditorWidget::CaptureInitialUndoSnapshot()
{
    UndoStack.clear();
    RedoStack.clear();
    UndoStack.push_back(MakeGraphSnapshot());
}

void FMaterialEditorWidget::CommitGraphEdit()
{
    if (bRestoringSnapshot) return;
    UndoStack.push_back(MakeGraphSnapshot());
    if (UndoStack.size() > 128) UndoStack.erase(UndoStack.begin());
    RedoStack.clear();
    bPositionsPushed = false;
    MarkDirty();

    if (UMaterial* Material = GetMaterial())
    {
        Material->GetGraphDocument().bPreviewDirty = true;
        if (Material->GetGraphDocument().bAutoPreview)
        {
            CompilePreview(Material);
        }
    }
}

void FMaterialEditorWidget::UndoGraphEdit()
{
    if (UndoStack.size() <= 1) return;
    TArray<uint8> Current = UndoStack.back();
    UndoStack.pop_back();
    RedoStack.push_back(Current);
    RestoreGraphSnapshot(UndoStack.back());
}

void FMaterialEditorWidget::RedoGraphEdit()
{
    if (RedoStack.empty()) return;
    TArray<uint8> Snapshot = RedoStack.back();
    RedoStack.pop_back();
    UndoStack.push_back(Snapshot);
    RestoreGraphSnapshot(Snapshot);
}

TArray<uint8> FMaterialEditorWidget::MakeGraphSnapshot() const
{
    FMaterialGraph Copy = WorkingGraph;
    FMemoryArchive Saver(true);
    Saver << Copy;
    return Saver.GetBuffer();
}

bool FMaterialEditorWidget::RestoreGraphSnapshot(const TArray<uint8>& Snapshot)
{
    if (Snapshot.empty()) return false;
    bRestoringSnapshot = true;
    FMemoryArchive Loader(Snapshot, false);
    Loader << WorkingGraph;
    bRestoringSnapshot = false;
    bPositionsPushed   = false;
    MarkDirty();
    return true;
}

bool FMaterialEditorWidget::GatherSelectedNodes(TArray<FMaterialGraphNode>& OutNodes, TArray<FMaterialGraphLink>& OutLinks) const
{
    OutNodes.clear();
    OutLinks.clear();
    if (!NodeEditorContext) return false;
    FScopedNodeEditorCurrent Scope(NodeEditorContext);

    ed::NodeId Selected[512];
    const int  Count = ed::GetSelectedNodes(Selected, 512);
    if (Count <= 0) return false;

    std::unordered_set<uint32> SelectedIds;
    for (int i = 0; i < Count; ++i) SelectedIds.insert(NodeIdToU32(Selected[i]));

    for (const FMaterialGraphNode& Node : WorkingGraph.Nodes)
    {
        if (SelectedIds.count(Node.NodeId))
        {
            FMaterialGraphNode Copy = Node;
            const ImVec2       Pos  = ed::GetNodePosition(ToNodeId(Node.NodeId));
            Copy.PosX               = Pos.x;
            Copy.PosY               = Pos.y;
            OutNodes.push_back(Copy);
        }
    }

    for (const FMaterialGraphLink& Link : WorkingGraph.Links)
    {
        const FMaterialGraphPin* From = WorkingGraph.FindPin(Link.FromPinId);
        const FMaterialGraphPin* To   = WorkingGraph.FindPin(Link.ToPinId);
        if (From && To && SelectedIds.count(From->OwningNodeId) && SelectedIds.count(To->OwningNodeId))
        {
            OutLinks.push_back(Link);
        }
    }
    return !OutNodes.empty();
}

bool FMaterialEditorWidget::CloneNodeFragment(const TArray<FMaterialGraphNode>& SourceNodes, const TArray<FMaterialGraphLink>& SourceLinks, const ImVec2& TargetAnchor, TArray<uint32>* OutNewNodeIds)
{
    if (SourceNodes.empty()) return false;
    const ImVec2                       SourceAnchor = ComputeNodeFragmentMin(SourceNodes);
    const ImVec2                       Delta(TargetAnchor.x - SourceAnchor.x, TargetAnchor.y - SourceAnchor.y);
    std::unordered_map<uint32, uint32> PinIdMap;
    TArray<uint32>                     NewNodeIds;

    for (const FMaterialGraphNode& SrcNode : SourceNodes)
    {
        FMaterialGraphNode* NewNode = WorkingGraph.AddNodeOfType(SrcNode.Type, SrcNode.PosX + Delta.x, SrcNode.PosY + Delta.y, GetMaterial()->GetGraphDocument().Target);
        if (!NewNode) continue;
        NewNode->DisplayName   = SrcNode.DisplayName;
        NewNode->ParameterName = SrcNode.ParameterName;
        NewNode->TexturePath   = SrcNode.TexturePath;
        NewNode->TextureSlot   = SrcNode.TextureSlot;
        NewNode->Value         = SrcNode.Value;
        NewNode->Mask          = SrcNode.Mask;
        const size_t PinCount  = min(NewNode->Pins.size(), SrcNode.Pins.size());
        for (size_t i = 0; i < PinCount; ++i)
        {
            PinIdMap[SrcNode.Pins[i].PinId] = NewNode->Pins[i].PinId;
        }
        NewNodeIds.push_back(NewNode->NodeId);
        if (NodeEditorContext) ed::SetNodePosition(ToNodeId(NewNode->NodeId), ImVec2(NewNode->PosX, NewNode->PosY));
    }

    for (const FMaterialGraphLink& SrcLink : SourceLinks)
    {
        auto FromIt = PinIdMap.find(SrcLink.FromPinId);
        auto ToIt   = PinIdMap.find(SrcLink.ToPinId);
        if (FromIt != PinIdMap.end() && ToIt != PinIdMap.end())
        {
            uint32 FromPinId = 0;
            uint32 ToPinId   = 0;
            if (WorkingGraph.CanLinkPins(FromIt->second, ToIt->second, &FromPinId, &ToPinId))
            {
                WorkingGraph.AddLink(FromPinId, ToPinId);
            }
        }
    }

    if (OutNewNodeIds) *OutNewNodeIds = NewNodeIds;
    return !NewNodeIds.empty();
}

void FMaterialEditorWidget::SelectOnlyNodes(const TArray<uint32>& NodeIds)
{
    if (!NodeEditorContext) return;
    FScopedNodeEditorCurrent Scope(NodeEditorContext);
    ed::ClearSelection();
    bool bAppend = false;
    for (uint32 NodeId : NodeIds)
    {
        ed::SelectNode(ToNodeId(NodeId), bAppend);
        bAppend = true;
    }
}

void FMaterialEditorWidget::CopySelectedNodes()
{
    GatherSelectedNodes(ClipboardNodes, ClipboardLinks);
}

void FMaterialEditorWidget::PasteCopiedNodes(const ImVec2* OverrideAnchor)
{
    if (ClipboardNodes.empty()) return;
    ImVec2         Anchor = OverrideAnchor ? *OverrideAnchor : ImVec2(ComputeNodeFragmentMin(ClipboardNodes).x + 24.0f, ComputeNodeFragmentMin(ClipboardNodes).y + 24.0f);
    TArray<uint32> NewIds;
    if (CloneNodeFragment(ClipboardNodes, ClipboardLinks, Anchor, &NewIds))
    {
        SelectOnlyNodes(NewIds);
        CommitGraphEdit();
    }
}

void FMaterialEditorWidget::DuplicateSelectedNodes()
{
    CopySelectedNodes();
    PasteCopiedNodes();
}

void FMaterialEditorWidget::DeleteSelectedNodes()
{
    if (!NodeEditorContext) return;
    FScopedNodeEditorCurrent Scope(NodeEditorContext);
    ed::NodeId               Selected[512];
    const int                Count = ed::GetSelectedNodes(Selected, 512);
    if (Count <= 0) return;
    bool bChanged = false;
    for (int i = 0; i < Count; ++i)
    {
        bChanged |= WorkingGraph.RemoveNode(NodeIdToU32(Selected[i]));
    }
    if (bChanged) CommitGraphEdit();
}

void FMaterialEditorWidget::GroupSelectedNodesAsComment()
{
    TArray<FMaterialGraphNode> Nodes;
    TArray<FMaterialGraphLink> Links;
    if (!GatherSelectedNodes(Nodes, Links)) return;
    ImVec2              Min     = ComputeNodeFragmentMin(Nodes);
    FMaterialGraphNode* Comment = WorkingGraph.AddNodeOfType(EMaterialGraphNodeType::Comment, Min.x - 40.0f, Min.y - 60.0f, GetMaterial()->GetGraphDocument().Target);
    if (Comment)
    {
        Comment->ParameterName = "Comment";
        CommitGraphEdit();
    }
}

bool FMaterialEditorWidget::AddNodeMenuItem(UMaterial* Material, EMaterialGraphNodeType Type, const char* Label)
{
    const char* Display = Label ? Label : NodeLabel(Type);
    if (!StringContainsInsensitive(Display, AddNodeSearchBuf)) return false;
    if (ImGui::MenuItem(Display))
    {
        if (WorkingGraph.AddNodeOfType(Type, PendingNewNodePosition.x, PendingNewNodePosition.y, Material->GetGraphDocument().Target))
        {
            CommitGraphEdit();
            return true;
        }
    }
    return false;
}

bool FMaterialEditorWidget::AddContextNodeMenuItem(UMaterial* Material, EMaterialGraphNodeType Type)
{
    const char* Display = NodeLabel(Type);
    if (!StringContainsInsensitive(Display, PinSpawnSearchBuf)) return false;
    if (!NodeTypeCanConnectToPendingPin(Type)) return false;
    if (ImGui::MenuItem(Display))
    {
        FMaterialGraphNode*      Node    = WorkingGraph.AddNodeOfType(Type, PendingPinSpawnPos.x, PendingPinSpawnPos.y, Material->GetGraphDocument().Target);
        const FMaterialGraphPin* DragPin = WorkingGraph.FindPin(PendingPinSpawnPinId);
        if (Node && DragPin)
        {
            if (FMaterialGraphPin* Compatible = FindFirstCompatiblePinOnNode(*Node, *DragPin))
            {
                uint32 FromPinId = 0;
                uint32 ToPinId   = 0;
                if (WorkingGraph.CanLinkPins(DragPin->PinId, Compatible->PinId, &FromPinId, &ToPinId))
                {
                    WorkingGraph.AddLink(FromPinId, ToPinId);
                }
            }
            CommitGraphEdit();
            return true;
        }
    }
    return false;
}

bool FMaterialEditorWidget::NodeTypeCanConnectToPendingPin(EMaterialGraphNodeType Type) const
{
    const FMaterialGraphPin* DragPin = WorkingGraph.FindPin(PendingPinSpawnPinId);
    if (!DragPin) return false;
    FMaterialGraph      Temp = WorkingGraph;
    FMaterialGraphNode* Node = Temp.AddNodeOfType(Type, 0, 0, EMaterialGraphTarget::Surface);
    if (!Node) return false;
    for (const FMaterialGraphPin& Pin : Node->Pins)
    {
        uint32 From = 0, To = 0;
        if (Temp.CanLinkPins(DragPin->PinId, Pin.PinId, &From, &To)) return true;
    }
    return false;
}

FMaterialGraphPin* FMaterialEditorWidget::FindFirstCompatiblePinOnNode(FMaterialGraphNode& Node, const FMaterialGraphPin& DragPin) const
{
    for (FMaterialGraphPin& Pin : Node.Pins)
    {
        uint32 From = 0, To = 0;
        if (WorkingGraph.CanLinkPins(DragPin.PinId, Pin.PinId, &From, &To)) return &Pin;
    }
    return nullptr;
}

void FMaterialEditorWidget::QueueNodeEditorShortcuts()
{
    ImGuiIO& IO = ImGui::GetIO();
    if (!IO.KeyCtrl) return;
    if (ImGui::IsKeyPressed(ImGuiKey_C)) bQueuedCopySelected = true;
    if (ImGui::IsKeyPressed(ImGuiKey_V)) bQueuedPasteNodes = true;
    if (ImGui::IsKeyPressed(ImGuiKey_D)) bQueuedDuplicateSelected = true;
    if (ImGui::IsKeyPressed(ImGuiKey_G)) bQueuedGroupSelected = true;
    if (ImGui::IsKeyPressed(ImGuiKey_S)) if (UMaterial* Material = GetMaterial()) SaveGraph(Material);
}

void FMaterialEditorWidget::ProcessQueuedNodeEditorCommands()
{
    if (bQueuedCopySelected)
    {
        CopySelectedNodes();
        bQueuedCopySelected = false;
    }
    if (bQueuedPasteNodes)
    {
        PasteCopiedNodes();
        bQueuedPasteNodes = false;
    }
    if (bQueuedDuplicateSelected)
    {
        DuplicateSelectedNodes();
        bQueuedDuplicateSelected = false;
    }
    if (bQueuedDeleteSelected)
    {
        DeleteSelectedNodes();
        bQueuedDeleteSelected = false;
    }
    if (bQueuedGroupSelected)
    {
        GroupSelectedNodesAsComment();
        bQueuedGroupSelected = false;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) DeleteSelectedNodes();
}