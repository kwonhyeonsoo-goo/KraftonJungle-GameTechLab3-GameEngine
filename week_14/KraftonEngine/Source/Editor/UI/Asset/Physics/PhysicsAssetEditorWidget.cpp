#include "PhysicsAssetEditorWidget.h"

#include "Asset/AssetRegistry.h"
#include "Animation/Skeleton/Skeleton.h"
#include "Component/Debug/PhysicsAssetPreviewComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Debug/DrawDebugHelpers.h"
#include "Editor/EditorEngine.h"
#include "GameFramework/World.h"
#include "Math/MathUtils.h"
#include "Mesh/MeshManager.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Object/Object.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetManager.h"
#include "Physics/PhysicsAssetPreviewUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

#include <imgui.h>
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace
{
    constexpr float LeftPanelWidth = 310.0f;
    constexpr float DetailsPanelMinWidth = 360.0f;
    constexpr int32 DebugCircleSegments = 24;
    constexpr int32 DebugHalfCircleSegments = 12;
    constexpr float PhysicsEditorTreeIndentSpacing = 10.0f;
    constexpr float PhysicsPreviewShapeScale = 0.1f;

    FTransform ComposePreviewDebugTransforms(const FTransform& ParentWorld, const FTransform& Local)
    {
        FTransform Result = Local;
        Result.Location = ParentWorld.Location + ParentWorld.Rotation.RotateVector(Local.Location);
        Result.Rotation = (ParentWorld.Rotation * Local.Rotation).GetNormalized();
        Result.Scale = FVector::OneVector;
        return Result;
    }

    void DrawDebugCircle(
        UWorld* World,
        const FVector& Center,
        const FVector& AxisA,
        const FVector& AxisB,
        float Radius,
        const FColor& Color)
    {
        if (!World || Radius <= 0.0f)
        {
            return;
        }

        const float Step = 2.0f * FMath::Pi / static_cast<float>(DebugCircleSegments);
        FVector Prev = Center + AxisA * Radius;
        for (int32 i = 1; i <= DebugCircleSegments; ++i)
        {
            const float Angle = Step * static_cast<float>(i);
            const FVector Next = Center + (AxisA * cosf(Angle) + AxisB * sinf(Angle)) * Radius;
            DrawDebugLine(World, Prev, Next, Color, 0.0f);
            Prev = Next;
        }
    }

    void DrawDebugHalfCircle(
        UWorld* World,
        const FVector& Center,
        const FVector& AxisA,
        const FVector& AxisB,
        float Radius,
        int32 Segments,
        float StartAngle,
        const FColor& Color)
    {
        if (!World || Radius <= 0.0f || Segments < 3)
        {
            return;
        }

        const float Step = FMath::Pi / static_cast<float>(Segments);
        FVector Prev = Center + (AxisA * cosf(StartAngle) + AxisB * sinf(StartAngle)) * Radius;
        for (int32 i = 1; i <= Segments; ++i)
        {
            const float Angle = StartAngle + Step * static_cast<float>(i);
            const FVector Next = Center + (AxisA * cosf(Angle) + AxisB * sinf(Angle)) * Radius;
            DrawDebugLine(World, Prev, Next, Color, 0.0f);
            Prev = Next;
        }
    }

    void DrawDebugOrientedBox(
        UWorld* World,
        const FVector& Center,
        const FVector& HalfExtent,
        const FQuat& Rotation,
        const FColor& Color)
    {
        if (!World)
        {
            return;
        }

        const FVector ClampedHalfExtent(
            (std::max)(HalfExtent.X, 0.001f),
            (std::max)(HalfExtent.Y, 0.001f),
            (std::max)(HalfExtent.Z, 0.001f));

        FVector Corners[8];
        for (int32 i = 0; i < 8; ++i)
        {
            const FVector LocalCorner(
                (i & 1) ? ClampedHalfExtent.X : -ClampedHalfExtent.X,
                (i & 2) ? ClampedHalfExtent.Y : -ClampedHalfExtent.Y,
                (i & 4) ? ClampedHalfExtent.Z : -ClampedHalfExtent.Z);
            Corners[i] = Center + Rotation.RotateVector(LocalCorner);
        }

        DrawDebugBox(
            World,
            Corners[0],
            Corners[1],
            Corners[3],
            Corners[2],
            Corners[4],
            Corners[5],
            Corners[7],
            Corners[6],
            Color,
            0.0f);
    }

    void DrawDebugCapsuleZAxis(
        UWorld* World,
        const FVector& Center,
        float Radius,
        float HalfHeight,
        const FQuat& Rotation,
        const FColor& Color)
    {
        if (!World)
        {
            return;
        }

        Radius = (std::max)(Radius, 0.001f);
        HalfHeight = (std::max)(HalfHeight, Radius);
        const float CylinderHalf = (std::max)(0.0f, HalfHeight - Radius);

        const FVector AxisX = Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
        const FVector AxisY = Rotation.RotateVector(FVector(0.0f, 1.0f, 0.0f));
        const FVector AxisZ = Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

        const FVector TopCenter = Center + AxisZ * CylinderHalf;
        const FVector BottomCenter = Center - AxisZ * CylinderHalf;

        DrawDebugCircle(World, TopCenter, AxisX, AxisY, Radius, Color);
        DrawDebugCircle(World, BottomCenter, AxisX, AxisY, Radius, Color);

        DrawDebugLine(World, TopCenter + AxisX * Radius, BottomCenter + AxisX * Radius, Color, 0.0f);
        DrawDebugLine(World, TopCenter - AxisX * Radius, BottomCenter - AxisX * Radius, Color, 0.0f);
        DrawDebugLine(World, TopCenter + AxisY * Radius, BottomCenter + AxisY * Radius, Color, 0.0f);
        DrawDebugLine(World, TopCenter - AxisY * Radius, BottomCenter - AxisY * Radius, Color, 0.0f);

        DrawDebugHalfCircle(World, TopCenter, AxisX, AxisZ, Radius, DebugHalfCircleSegments, 0.0f, Color);
        DrawDebugHalfCircle(World, TopCenter, AxisY, AxisZ, Radius, DebugHalfCircleSegments, 0.0f, Color);
        DrawDebugHalfCircle(World, BottomCenter, AxisX, AxisZ, Radius, DebugHalfCircleSegments, FMath::Pi, Color);
        DrawDebugHalfCircle(World, BottomCenter, AxisY, AxisZ, Radius, DebugHalfCircleSegments, FMath::Pi, Color);
    }

    const char* ShapeTypeText(EPhysicsAssetShapeType Type)
    {
        switch (Type)
        {
        case EPhysicsAssetShapeType::Box: return "Box";
        case EPhysicsAssetShapeType::Sphere: return "Sphere";
        case EPhysicsAssetShapeType::Capsule: return "Capsule";
        default: return "Unknown";
        }
    }

    const char* MotionText(EConstraintMotion Motion)
    {
        switch (Motion)
        {
        case EConstraintMotion::Free: return "Free";
        case EConstraintMotion::Limited: return "Limited";
        case EConstraintMotion::Locked: return "Locked";
        default: return "Unknown";
        }
    }

    const char* StateText(UPhysicsAsset::EEditorSetupState State)
    {
        switch (State)
        {
        case UPhysicsAsset::EEditorSetupState::RuntimeReady: return "Ready";
        case UPhysicsAsset::EEditorSetupState::Placeholder: return "Placeholder";
        case UPhysicsAsset::EEditorSetupState::Invalid: return "Invalid";
        default: return "Unknown";
        }
    }

    bool InputString(const char* Label, FString& Value)
    {
        char Buffer[512] = {};
        std::snprintf(Buffer, sizeof(Buffer), "%s", Value.c_str());
        if (ImGui::InputText(Label, Buffer, sizeof(Buffer)))
        {
            Value = Buffer;
            return true;
        }
        return false;
    }

    bool InputName(const char* Label, FName& Name)
    {
        FString Text = (Name == FName::None) ? FString() : Name.ToString();
        if (InputString(Label, Text))
        {
            Name = Text.empty() ? FName::None : FName(Text);
            return true;
        }
        return false;
    }

    bool DragVec3(const char* Label, FVector& Value, float Speed = 0.1f)
    {
        float V[3] = { Value.X, Value.Y, Value.Z };
        if (ImGui::DragFloat3(Label, V, Speed))
        {
            Value = FVector(V[0], V[1], V[2]);
            return true;
        }
        return false;
    }

    bool DragMinFloat(const char* Label, float& Value, float Speed = 0.1f, float MinValue = 0.0f)
    {
        float Tmp = Value;
        if (ImGui::DragFloat(Label, &Tmp, Speed, MinValue, 0.0f))
        {
            Value = (std::max)(Tmp, MinValue);
            return true;
        }
        return false;
    }

    bool InputMinInt(const char* Label, int32& Value, int32 MinValue = 1)
    {
        int Tmp = static_cast<int>(Value);
        if (ImGui::InputInt(Label, &Tmp))
        {
            Value = static_cast<int32>((std::max)(Tmp, static_cast<int>(MinValue)));
            return true;
        }
        return false;
    }

    bool EditTransform(const char* Label, FTransform& Transform)
    {
        bool bChanged = false;
        if (ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            bChanged |= DragVec3("Location", Transform.Location, 0.1f);

            FRotator Rot = Transform.GetRotator();
            float R[3] = { Rot.Pitch, Rot.Yaw, Rot.Roll };
            if (ImGui::DragFloat3("Rotation", R, 0.1f))
            {
                Transform.SetRotation(FRotator(R[0], R[1], R[2]));
                bChanged = true;
            }

            bChanged |= DragVec3("Scale", Transform.Scale, 0.01f);
            ImGui::TreePop();
        }
        return bChanged;
    }

    bool ComboShapeType(const char* Label, EPhysicsAssetShapeType& Type)
    {
        bool bChanged = false;
        if (ImGui::BeginCombo(Label, ShapeTypeText(Type)))
        {
            const EPhysicsAssetShapeType Options[] = { EPhysicsAssetShapeType::Box, EPhysicsAssetShapeType::Sphere, EPhysicsAssetShapeType::Capsule };
            for (EPhysicsAssetShapeType Candidate : Options)
            {
                const bool bSelected = Type == Candidate;
                if (ImGui::Selectable(ShapeTypeText(Candidate), bSelected))
                {
                    Type = Candidate;
                    bChanged = true;
                }
                if (bSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return bChanged;
    }

    bool ComboMotion(const char* Label, EConstraintMotion& Motion)
    {
        bool bChanged = false;
        if (ImGui::BeginCombo(Label, MotionText(Motion)))
        {
            const EConstraintMotion Options[] = { EConstraintMotion::Free, EConstraintMotion::Limited, EConstraintMotion::Locked };
            for (EConstraintMotion Candidate : Options)
            {
                const bool bSelected = Motion == Candidate;
                if (ImGui::Selectable(MotionText(Candidate), bSelected))
                {
                    Motion = Candidate;
                    bChanged = true;
                }
                if (bSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return bChanged;
    }

    FString BodyLabel(const FPhysicsAssetBodySetup& Body, int32 Index)
    {
        const FString Bone = Body.BoneName == FName::None ? FString("<None>") : Body.BoneName.ToString();
        const int32 ShapeCount = static_cast<int32>(Body.Shapes.size());
        char Buffer[256] = {};
        std::snprintf(
            Buffer,
            sizeof(Buffer),
            "Body %d  %s  (%d %s)",
            Index,
            Bone.c_str(),
            ShapeCount,
            ShapeCount == 1 ? "shape" : "shapes");
        return Buffer;
    }

    FString ConstraintLabel(const FPhysicsAssetConstraintSetup& Constraint, int32 Index, UPhysicsAsset::EEditorSetupState State)
    {
        const FString Parent = Constraint.ParentBoneName == FName::None ? FString("<None>") : Constraint.ParentBoneName.ToString();
        const FString Child = Constraint.ChildBoneName == FName::None ? FString("<None>") : Constraint.ChildBoneName.ToString();
        char Buffer[384] = {};
        std::snprintf(Buffer, sizeof(Buffer), "Constraint %d  %s -> %s  [%s]", Index, Parent.c_str(), Child.c_str(), StateText(State));
        return Buffer;
    }

    bool HasBoneName(const FName& BoneName)
    {
        return BoneName.IsValid() && BoneName != FName::None;
    }

    bool IsBoneInReferenceSkeleton(const FReferenceSkeleton& RefSkeleton, const FName& BoneName)
    {
        return HasBoneName(BoneName) && RefSkeleton.FindBoneIndex(BoneName.ToString()) >= 0;
    }

    bool HasChildBone(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex)
    {
        for (int32 Index = BoneIndex + 1; Index < RefSkeleton.GetNumBones(); ++Index)
        {
            if (RefSkeleton.Bones[Index].ParentIndex == BoneIndex)
            {
                return true;
            }
        }
        return false;
    }

    int32 FindNearestParentBodyBoneIndex(
        UPhysicsAsset* PhysicsAsset,
        const FReferenceSkeleton& RefSkeleton,
        int32 BoneIndex)
    {
        if (!PhysicsAsset || BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones())
        {
            return -1;
        }

        int32 ParentIndex = RefSkeleton.Bones[BoneIndex].ParentIndex;
        while (ParentIndex >= 0 && ParentIndex < RefSkeleton.GetNumBones())
        {
            if (PhysicsAsset->HasBodySetupForBone(FName(RefSkeleton.Bones[ParentIndex].Name)))
            {
                return ParentIndex;
            }
            ParentIndex = RefSkeleton.Bones[ParentIndex].ParentIndex;
        }
        return -1;
    }

    int32 FindConstraintToNearestParentBodyIndex(
        UPhysicsAsset* PhysicsAsset,
        const FReferenceSkeleton& RefSkeleton,
        int32 BoneIndex)
    {
        if (!PhysicsAsset || BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones())
        {
            return -1;
        }

        const int32 ParentBodyIndex = FindNearestParentBodyBoneIndex(PhysicsAsset, RefSkeleton, BoneIndex);
        if (ParentBodyIndex < 0)
        {
            return -1;
        }

        return PhysicsAsset->FindConstraintSetupIndex(
            FName(RefSkeleton.Bones[ParentBodyIndex].Name),
            FName(RefSkeleton.Bones[BoneIndex].Name));
    }

    FString MakeBoneTreeLabel(
        UPhysicsAsset* PhysicsAsset,
        const FReferenceSkeleton& RefSkeleton,
        int32 BoneIndex)
    {
        (void)PhysicsAsset;
        return RefSkeleton.Bones[BoneIndex].Name;
    }

    constexpr uint32 PhysicsBodyNodeBase = 100000;
    constexpr uint32 PhysicsConstraintNodeBase = 200000;
    constexpr uint32 PhysicsBodyInputPinBase = 300000;
    constexpr uint32 PhysicsBodyOutputPinBase = 400000;
    constexpr uint32 PhysicsConstraintInputPinBase = 500000;
    constexpr uint32 PhysicsConstraintOutputPinBase = 600000;
    constexpr uint32 PhysicsParentLinkBase = 700000;
    constexpr uint32 PhysicsChildLinkBase = 800000;

    inline ed::NodeId ToPhysicsNodeId(uint32 Id) { return static_cast<ed::NodeId>(Id); }
    inline ed::PinId ToPhysicsPinId(uint32 Id) { return static_cast<ed::PinId>(Id); }
    inline ed::LinkId ToPhysicsLinkId(uint32 Id) { return static_cast<ed::LinkId>(Id); }
    inline uint32 PhysicsNodeIdToU32(ed::NodeId Id) { return static_cast<uint32>(Id.Get()); }
    inline uint32 PhysicsPinIdToU32(ed::PinId Id) { return static_cast<uint32>(Id.Get()); }
    inline uint32 PhysicsLinkIdToU32(ed::LinkId Id) { return static_cast<uint32>(Id.Get()); }

    uint32 MakeBodyNodeId(int32 BodyIndex) { return PhysicsBodyNodeBase + static_cast<uint32>(BodyIndex); }
    uint32 MakeConstraintNodeId(int32 ConstraintIndex) { return PhysicsConstraintNodeBase + static_cast<uint32>(ConstraintIndex); }
    uint32 MakeBodyInputPinId(int32 BodyIndex) { return PhysicsBodyInputPinBase + static_cast<uint32>(BodyIndex); }
    uint32 MakeBodyOutputPinId(int32 BodyIndex) { return PhysicsBodyOutputPinBase + static_cast<uint32>(BodyIndex); }
    uint32 MakeConstraintInputPinId(int32 ConstraintIndex) { return PhysicsConstraintInputPinBase + static_cast<uint32>(ConstraintIndex); }
    uint32 MakeConstraintOutputPinId(int32 ConstraintIndex) { return PhysicsConstraintOutputPinBase + static_cast<uint32>(ConstraintIndex); }
    uint32 MakeParentLinkId(int32 ConstraintIndex) { return PhysicsParentLinkBase + static_cast<uint32>(ConstraintIndex); }
    uint32 MakeChildLinkId(int32 ConstraintIndex) { return PhysicsChildLinkBase + static_cast<uint32>(ConstraintIndex); }

    bool DecodeBodyInputPin(uint32 PinId, int32& OutBodyIndex)
    {
        if (PinId < PhysicsBodyInputPinBase || PinId >= PhysicsBodyOutputPinBase)
        {
            return false;
        }
        OutBodyIndex = static_cast<int32>(PinId - PhysicsBodyInputPinBase);
        return true;
    }

    bool DecodeBodyOutputPin(uint32 PinId, int32& OutBodyIndex)
    {
        if (PinId < PhysicsBodyOutputPinBase || PinId >= PhysicsConstraintInputPinBase)
        {
            return false;
        }
        OutBodyIndex = static_cast<int32>(PinId - PhysicsBodyOutputPinBase);
        return true;
    }

    bool DecodeBodyNode(uint32 NodeId, int32& OutBodyIndex)
    {
        if (NodeId < PhysicsBodyNodeBase || NodeId >= PhysicsConstraintNodeBase)
        {
            return false;
        }
        OutBodyIndex = static_cast<int32>(NodeId - PhysicsBodyNodeBase);
        return true;
    }

    bool DecodeConstraintNode(uint32 NodeId, int32& OutConstraintIndex)
    {
        if (NodeId < PhysicsConstraintNodeBase || NodeId >= PhysicsBodyInputPinBase)
        {
            return false;
        }
        OutConstraintIndex = static_cast<int32>(NodeId - PhysicsConstraintNodeBase);
        return true;
    }

    bool DecodeConstraintLink(uint32 LinkId, int32& OutConstraintIndex)
    {
        if (LinkId >= PhysicsParentLinkBase && LinkId < PhysicsChildLinkBase)
        {
            OutConstraintIndex = static_cast<int32>(LinkId - PhysicsParentLinkBase);
            return true;
        }
        if (LinkId >= PhysicsChildLinkBase)
        {
            OutConstraintIndex = static_cast<int32>(LinkId - PhysicsChildLinkBase);
            return true;
        }
        return false;
    }

    void HashCombine(uint64& Seed, uint64 Value)
    {
        Seed ^= Value + 0x9e3779b97f4a7c15ull + (Seed << 6) + (Seed >> 2);
    }

    void HashName(uint64& Seed, const FName& Name)
    {
        const FString Text = Name.ToString();
        for (char Ch : Text)
        {
            HashCombine(Seed, static_cast<uint64>(static_cast<unsigned char>(Ch)));
        }
    }

    bool IsValidBodyIndex(const UPhysicsAsset* PhysicsAsset, int32 BodyIndex)
    {
        return PhysicsAsset && BodyIndex >= 0 && BodyIndex < static_cast<int32>(PhysicsAsset->GetBodySetups().size());
    }

    bool IsValidConstraintIndex(const UPhysicsAsset* PhysicsAsset, int32 ConstraintIndex)
    {
        return PhysicsAsset && ConstraintIndex >= 0 && ConstraintIndex < static_cast<int32>(PhysicsAsset->GetConstraintSetups().size());
    }
}

FPhysicsAssetEditorWidget::~FPhysicsAssetEditorWidget()
{
    DestroyConstraintGraphEditor();
}

void FPhysicsAssetEditorWidget::Close()
{
    DestroyConstraintGraphEditor();
    PreviewSkeletalMesh = nullptr;
    SelectedBodyIndex = -1;
    SelectedShapeIndex = -1;
    SelectedConstraintIndex = -1;
    SelectedTreeBoneIndex = -1;
    bPendingClose = false;
    bConstraintGraphLayoutDirty = true;
    ConstraintGraphTopologyHash = 0;
    FAssetEditorWidget::Close();
}

bool FPhysicsAssetEditorWidget::CanEdit(UObject* Object) const
{
    return Object && Object->IsA<UPhysicsAsset>();
}

bool FPhysicsAssetEditorWidget::IsEditingObject(UObject* Object) const
{
    const UPhysicsAsset* A = Cast<UPhysicsAsset>(Object);
    const UPhysicsAsset* B = GetEditedPhysicsAsset();
    if (!A || !B || !IsOpen()) return false;

    const FString APath = A->GetAssetPathFileName();
    const FString BPath = B->GetAssetPathFileName();
    return !APath.empty() && APath != "None" && APath == BPath;
}

void FPhysicsAssetEditorWidget::Open(UObject* Object)
{
    if (!CanEdit(Object)) return;

    EditedObject = Object;
    bOpen = true;
    bPendingClose = false;
    bConstraintGraphLayoutDirty = true;
    ConstraintGraphTopologyHash = 0;
    SelectedBodyIndex = -1;
    SelectedShapeIndex = -1;
    SelectedConstraintIndex = -1;
    SelectedTreeBoneIndex = -1;
    PreviewSkeletalMesh = nullptr;
    ValidationIssues.clear();
    ValidationMeshPath.clear();
    bValidationRan = false;
    ClearDirty();
    RequestFocus();
}

void FPhysicsAssetEditorWidget::Render(float DeltaTime)
{
    if (!IsOpen() || !EditedObject) return;

    bool bWindowOpen = true;
    FString Title = GetDocumentTitle();
    if (IsDirty()) Title += " *";
    Title += "###PhysicsAssetEditor";

    ImGui::SetNextWindowSize(ImVec2(960.0f, 620.0f), ImGuiCond_Once);
    if (!ImGui::Begin(Title.c_str(), &bWindowOpen))
    {
        ImGui::End();
        if (!bWindowOpen) Close();
        return;
    }

    RenderDocument(DeltaTime);
    ImGui::End();
    if (!bWindowOpen) bPendingClose = true;
}

void FPhysicsAssetEditorWidget::OpenEmbedded(UPhysicsAsset* PhysicsAsset)
{
    Open(PhysicsAsset);
}

void FPhysicsAssetEditorWidget::RenderEmbedded(UPhysicsAsset* PhysicsAsset, float DeltaTime)
{
    RenderEmbedded(PhysicsAsset, nullptr, DeltaTime);
}

void FPhysicsAssetEditorWidget::RenderEmbedded(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime)
{
    PreviewSkeletalMesh = PreviewMesh;

    if (!PhysicsAsset)
    {
        Close();
        ImGui::TextDisabled("No PhysicsAsset selected.");
        return;
    }

    if (!IsEditingObject(PhysicsAsset))
    {
        OpenEmbedded(PhysicsAsset);
        PreviewSkeletalMesh = PreviewMesh;
    }

    RenderDocument(DeltaTime);
}

void FPhysicsAssetEditorWidget::RenderEmbeddedToolbar(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime)
{
    (void)DeltaTime;
    if (!PrepareEmbeddedRender(PhysicsAsset, PreviewMesh))
    {
        return;
    }

    ClampSelection(PhysicsAsset);
    RenderToolbar(PhysicsAsset);
}

void FPhysicsAssetEditorWidget::RenderEmbeddedTreeAndGraph(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime)
{
    (void)DeltaTime;
    if (!PrepareEmbeddedRender(PhysicsAsset, PreviewMesh))
    {
        return;
    }

    ClampSelection(PhysicsAsset);
    RenderTreeAndGraphPanel(PhysicsAsset);
}

void FPhysicsAssetEditorWidget::RenderEmbeddedDetails(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime)
{
    (void)DeltaTime;
    if (!PrepareEmbeddedRender(PhysicsAsset, PreviewMesh))
    {
        return;
    }

    ClampSelection(PhysicsAsset);
    RenderDetailsAndValidationPanel(PhysicsAsset);
}

bool FPhysicsAssetEditorWidget::SaveEditedPhysicsAsset()
{
    UPhysicsAsset* PhysicsAsset = GetEditedPhysicsAsset();
    if (!PhysicsAsset)
    {
        return false;
    }

    if (FPhysicsAssetManager::Get().SavePhysicsAssetPreservingMetadata(PhysicsAsset))
    {
        ClearDirty();
        bValidationRan = false;
        return true;
    }
    return false;
}

void FPhysicsAssetEditorWidget::NotifyViewportGizmoModified()
{
    MarkPhysicsAssetDirty();
}

void FPhysicsAssetEditorWidget::SelectPhysicsShapeFromViewport(UPhysicsAsset* PhysicsAsset, int32 BodyIndex, int32 ShapeIndex)
{
    if (!PhysicsAsset || !IsValidBodyIndex(PhysicsAsset, BodyIndex))
    {
        return;
    }

    if (!IsEditingObject(PhysicsAsset))
    {
        OpenEmbedded(PhysicsAsset);
    }

    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    const FPhysicsAssetBodySetup& Body = Bodies[BodyIndex];

    SelectedBodyIndex = BodyIndex;
    SelectedShapeIndex =
        ShapeIndex >= 0 && ShapeIndex < static_cast<int32>(Body.Shapes.size())
            ? ShapeIndex
            : -1;
    SelectedConstraintIndex = -1;
    SelectedTreeBoneIndex = FindPreviewBoneIndexByName(Body.BoneName);
}

void FPhysicsAssetEditorWidget::RenderDocument(float DeltaTime)
{
    (void)DeltaTime;
    if (bPendingClose)
    {
        Close();
        bPendingClose = false;
        return;
    }

    UPhysicsAsset* PhysicsAsset = GetEditedPhysicsAsset();
    if (!PhysicsAsset)
    {
        ImGui::TextDisabled("No PhysicsAsset selected.");
        return;
    }

    ClampSelection(PhysicsAsset);
    RenderToolbar(PhysicsAsset);
    ImGui::Separator();

    const float TotalWidth = ImGui::GetContentRegionAvail().x;
    const float LeftWidth = (std::min)(LeftPanelWidth, (std::max)(240.0f, TotalWidth * 0.36f));
    const float RightWidth = (std::max)(DetailsPanelMinWidth, TotalWidth - LeftWidth - ImGui::GetStyle().ItemSpacing.x);

    ImGui::BeginChild("##PhysicsAssetLeft", ImVec2(LeftWidth, 0.0f), true);
    RenderTreeAndGraphPanel(PhysicsAsset);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##PhysicsAssetRight", ImVec2(RightWidth, 0.0f), true);
    RenderDetailsAndValidationPanel(PhysicsAsset);
    ImGui::EndChild();
}

FString FPhysicsAssetEditorWidget::GetDocumentTitle() const
{
    const UPhysicsAsset* Asset = GetEditedPhysicsAsset();
    const FString Path = Asset ? Asset->GetAssetPathFileName() : FString();
    return Path.empty() || Path == "None" ? FString("Physics Asset") : FString("Physics Asset - " + Path);
}

FString FPhysicsAssetEditorWidget::GetDocumentPayloadId() const
{
    const UPhysicsAsset* Asset = GetEditedPhysicsAsset();
    const FString Path = Asset ? Asset->GetAssetPathFileName() : FString();
    return Path.empty() || Path == "None" ? FAssetEditorWidget::GetDocumentPayloadId() : Path;
}

UPhysicsAsset* FPhysicsAssetEditorWidget::GetEditedPhysicsAsset() const
{
    return Cast<UPhysicsAsset>(EditedObject);
}

bool FPhysicsAssetEditorWidget::PrepareEmbeddedRender(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh)
{
    PreviewSkeletalMesh = PreviewMesh;

    if (!PhysicsAsset)
    {
        Close();
        ImGui::TextDisabled("No PhysicsAsset selected.");
        return false;
    }

    if (!IsEditingObject(PhysicsAsset))
    {
        OpenEmbedded(PhysicsAsset);
        PreviewSkeletalMesh = PreviewMesh;
    }

    return true;
}

void FPhysicsAssetEditorWidget::RenderTreeAndGraphPanel(UPhysicsAsset* PhysicsAsset)
{
    const float AvailableHeight = ImGui::GetContentRegionAvail().y;
    const float GraphHeight = (std::max)(200.0f, AvailableHeight * 0.42f);
    const float TreeHeight = (std::max)(140.0f, AvailableHeight - GraphHeight - ImGui::GetStyle().ItemSpacing.y - 4.0f);

    ImGui::BeginChild(
        "##PhysicsAssetTreeArea",
        ImVec2(0.0f, TreeHeight),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (PreviewSkeletalMesh && PreviewSkeletalMesh->GetSkeleton())
    {
        RenderSkeletonPhysicsTree(PhysicsAsset, PreviewSkeletalMesh);
    }
    else
    {
        ImGui::TextDisabled("No preview Skeleton. Falling back to raw setup lists.");
        ImGui::Separator();
        RenderBodyList(PhysicsAsset);
        ImGui::Separator();
        RenderConstraintList(PhysicsAsset);
    }
    ImGui::EndChild();

    ImGui::Separator();
    RenderConstraintGraphPanel(PhysicsAsset);
}

void FPhysicsAssetEditorWidget::RenderDetailsAndValidationPanel(UPhysicsAsset* PhysicsAsset)
{
    RenderDetailsPanel(PhysicsAsset);
    ImGui::Separator();
    RenderValidationPanel();
}

void FPhysicsAssetEditorWidget::RenderToolbar(UPhysicsAsset* PhysicsAsset)
{
    const bool bCanSave = PhysicsAsset && !PhysicsAsset->GetAssetPathFileName().empty() && PhysicsAsset->GetAssetPathFileName() != "None";
    if (!bCanSave) ImGui::BeginDisabled();
    if (ImGui::Button(IsDirty() ? "Save*" : "Save", ImVec2(72.0f, 0.0f)))
    {
        SaveEditedPhysicsAsset();
    }
    if (!bCanSave) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Validate", ImVec2(86.0f, 0.0f)))
    {
        RunValidation(PhysicsAsset);
    }
}

void FPhysicsAssetEditorWidget::RenderAssetSummary(UPhysicsAsset* PhysicsAsset)
{
    const FSkeletonBinding& Binding = PhysicsAsset->GetSkeletonBinding();
    ImGui::TextUnformatted("Physics Asset");
    ImGui::TextDisabled("Skeleton: %s", Binding.SkeletonPath.c_str());
    ImGui::TextDisabled("Bodies: %d  Constraints: %d", static_cast<int32>(PhysicsAsset->GetBodySetups().size()), static_cast<int32>(PhysicsAsset->GetConstraintSetups().size()));
}

void FPhysicsAssetEditorWidget::RenderSkeletonPhysicsTree(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh)
{
    USkeleton* Skeleton = PreviewMesh ? PreviewMesh->GetSkeleton() : nullptr;
    if (!Skeleton)
    {
        RenderBodyList(PhysicsAsset);
        ImGui::Separator();
        RenderConstraintList(PhysicsAsset);
        return;
    }

    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
    ImGui::TextUnformatted("Skeleton Physics Tree");
    ImGui::TextDisabled(
        "Bones: %d  Bodies: %d  Constraints: %d",
        RefSkeleton.GetNumBones(),
        static_cast<int32>(PhysicsAsset->GetBodySetups().size()),
        static_cast<int32>(PhysicsAsset->GetConstraintSetups().size()));

    const float ActionPanelHeight = ImGui::GetFrameHeightWithSpacing();
    constexpr float BodyListHeight = 150.0f;
    const float TreeHeight = (std::max)(
        110.0f,
        ImGui::GetContentRegionAvail().y
            - ActionPanelHeight
            - BodyListHeight
            - ImGui::GetTextLineHeightWithSpacing()
            - ImGui::GetStyle().ItemSpacing.y * 3.0f);
    ImGui::BeginChild("##PhysicsAssetSkeletonTree", ImVec2(0.0f, TreeHeight), true);
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, PhysicsEditorTreeIndentSpacing);
    if (RefSkeleton.GetNumBones() <= 0)
    {
        ImGui::TextDisabled("Skeleton has no bones.");
    }
    else
    {
        for (int32 Index = 0; Index < RefSkeleton.GetNumBones(); ++Index)
        {
            if (RefSkeleton.Bones[Index].ParentIndex == -1)
            {
                RenderPhysicsBoneTree(PhysicsAsset, RefSkeleton, Index);
            }
        }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();

    RenderSelectedBoneActionPanel(PhysicsAsset, RefSkeleton);

    ImGui::Separator();
    RenderBodyList(PhysicsAsset);

    RenderUnboundPhysicsSetups(PhysicsAsset, RefSkeleton);
}

void FPhysicsAssetEditorWidget::RenderPhysicsBoneTree(
    UPhysicsAsset* PhysicsAsset,
    const FReferenceSkeleton& RefSkeleton,
    int32 BoneIndex)
{
    if (!PhysicsAsset || BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones())
    {
        return;
    }

    const FReferenceBone& Bone = RefSkeleton.Bones[BoneIndex];
    const FName BoneName(Bone.Name);
    const int32 BodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(BoneName);
    const int32 ConstraintIndex = FindConstraintToNearestParentBodyIndex(PhysicsAsset, RefSkeleton, BoneIndex);
    const bool bHasSkeletonChildren = HasChildBone(RefSkeleton, BoneIndex);
    const bool bHasBody = BodyIndex >= 0;
    const bool bSelected =
        SelectedTreeBoneIndex == BoneIndex ||
        (BodyIndex >= 0 && SelectedBodyIndex == BodyIndex && SelectedConstraintIndex < 0) ||
        (ConstraintIndex >= 0 && SelectedConstraintIndex == ConstraintIndex);

    ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!bHasSkeletonChildren)
    {
        Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (bSelected)
    {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    const FString Label = MakeBoneTreeLabel(PhysicsAsset, RefSkeleton, BoneIndex);
    ImGui::PushID(BoneIndex);
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    const bool bOpen = ImGui::TreeNodeEx("##PhysicsBone", Flags, "%s", Label.c_str());

    if (bHasBody)
    {
        const ImVec2 ItemMin = ImGui::GetItemRectMin();
        const ImVec2 ItemMax = ImGui::GetItemRectMax();
        constexpr float Radius = 3.5f;
        const ImVec2 Center(
            ItemMax.x - Radius - 6.0f,
            (ItemMin.y + ItemMax.y) * 0.5f);

        ImGui::GetWindowDrawList()->AddCircleFilled(
            Center,
            Radius,
            IM_COL32(80, 210, 190, 255));

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Body");
        }
    }

    if (ImGui::IsItemClicked())
    {
        SelectBoneInPhysicsTree(PhysicsAsset, RefSkeleton, BoneIndex);
    }

    RenderPhysicsBoneContextMenu(PhysicsAsset, RefSkeleton, BoneIndex);

    if (bOpen && !bHasSkeletonChildren)
    {
        ImGui::PopID();
        return;
    }

    if (bOpen)
    {
        for (int32 ChildIndex = BoneIndex + 1; ChildIndex < RefSkeleton.GetNumBones(); ++ChildIndex)
        {
            if (RefSkeleton.Bones[ChildIndex].ParentIndex == BoneIndex)
            {
                RenderPhysicsBoneTree(PhysicsAsset, RefSkeleton, ChildIndex);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void FPhysicsAssetEditorWidget::RenderPhysicsBoneContextMenu(
    UPhysicsAsset* PhysicsAsset,
    const FReferenceSkeleton& RefSkeleton,
    int32 BoneIndex)
{
    if (!PhysicsAsset || BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones())
    {
        return;
    }

    if (!ImGui::BeginPopupContextItem("##PhysicsBoneContext"))
    {
        return;
    }

    const FReferenceBone& Bone = RefSkeleton.Bones[BoneIndex];
    const FName BoneName(Bone.Name);
    const int32 BodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(BoneName);
    const int32 ParentBodyBoneIndex = FindNearestParentBodyBoneIndex(PhysicsAsset, RefSkeleton, BoneIndex);
    const FName ParentBodyBoneName = ParentBodyBoneIndex >= 0 ? FName(RefSkeleton.Bones[ParentBodyBoneIndex].Name) : FName::None;
    const int32 ConstraintIndex = ParentBodyBoneIndex >= 0
        ? PhysicsAsset->FindConstraintSetupIndex(ParentBodyBoneName, BoneName)
        : -1;

    ImGui::TextDisabled("Bone: %s", Bone.Name.c_str());
    ImGui::Separator();

    if (BodyIndex >= 0)
    {
        if (ImGui::MenuItem("Select Body"))
        {
            SelectedTreeBoneIndex = BoneIndex;
            SelectedBodyIndex = BodyIndex;
            SelectedShapeIndex = PhysicsAsset->GetBodySetups()[BodyIndex].Shapes.empty() ? -1 : 0;
            SelectedConstraintIndex = -1;
        }

        if (ImGui::MenuItem("Remove Body"))
        {
            PhysicsAsset->RemoveBodySetupByIndex(BodyIndex);
            SelectedTreeBoneIndex = BoneIndex;
            SelectedBodyIndex = -1;
            SelectedShapeIndex = -1;
            SelectedConstraintIndex = -1;
            MarkPhysicsAssetDirty();
        }
    }
    else
    {
        if (ImGui::MenuItem("Add Body to Bone"))
        {
            AddDefaultBodyForBone(PhysicsAsset, BoneName);
            SelectedTreeBoneIndex = BoneIndex;
        }
    }

    ImGui::Separator();
    const bool bCanCreateConstraint = BodyIndex >= 0 && ParentBodyBoneIndex >= 0 && ConstraintIndex < 0;
    if (!bCanCreateConstraint) ImGui::BeginDisabled();
    if (ImGui::MenuItem("Create Constraint to Parent Body"))
    {
        AddDefaultConstraintForBones(PhysicsAsset, ParentBodyBoneName, BoneName);
        SelectedTreeBoneIndex = BoneIndex;
    }
    if (!bCanCreateConstraint) ImGui::EndDisabled();

    if (ConstraintIndex >= 0)
    {
        if (ImGui::MenuItem("Remove Parent Constraint"))
        {
            PhysicsAsset->RemoveConstraintSetupByIndex(ConstraintIndex);
            SelectedConstraintIndex = -1;
            SelectedBodyIndex = BodyIndex;
            SelectedShapeIndex = BodyIndex >= 0 && !PhysicsAsset->GetBodySetups()[BodyIndex].Shapes.empty() ? 0 : -1;
            SelectedTreeBoneIndex = BoneIndex;
            MarkPhysicsAssetDirty();
        }
    }
    else if (ParentBodyBoneIndex >= 0)
    {
        ImGui::TextDisabled("Parent Body: %s", ParentBodyBoneName.ToString().c_str());
    }
    else
    {
        ImGui::TextDisabled("No parent body found.");
    }

    ImGui::EndPopup();
}

void FPhysicsAssetEditorWidget::RenderSelectedBoneActionPanel(
    UPhysicsAsset* PhysicsAsset,
    const FReferenceSkeleton& RefSkeleton)
{
    if (!PhysicsAsset)
    {
        return;
    }

    const bool bHasSelectedBone = SelectedTreeBoneIndex >= 0 && SelectedTreeBoneIndex < RefSkeleton.GetNumBones();
    FName BoneName = FName::None;
    FString BoneLabel = "<None>";
    int32 BodyIndex = -1;
    int32 ParentBodyBoneIndex = -1;
    FName ParentBodyBoneName = FName::None;
    int32 ConstraintIndex = -1;

    if (bHasSelectedBone)
    {
        const FReferenceBone& Bone = RefSkeleton.Bones[SelectedTreeBoneIndex];
        BoneName = FName(Bone.Name);
        BoneLabel = Bone.Name;
        BodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(BoneName);
        ParentBodyBoneIndex = FindNearestParentBodyBoneIndex(PhysicsAsset, RefSkeleton, SelectedTreeBoneIndex);
        ParentBodyBoneName = ParentBodyBoneIndex >= 0 ? FName(RefSkeleton.Bones[ParentBodyBoneIndex].Name) : FName::None;
        ConstraintIndex = ParentBodyBoneIndex >= 0
            ? PhysicsAsset->FindConstraintSetupIndex(ParentBodyBoneName, BoneName)
            : -1;
    }

    const bool bCanAddBody = bHasSelectedBone && BodyIndex < 0;
    if (!bCanAddBody) ImGui::BeginDisabled();
    const float ButtonWidth = (std::max)(80.0f, (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f);
    if (ImGui::Button("+ Body", ImVec2(ButtonWidth, 0.0f)))
    {
        AddDefaultBodyForBone(PhysicsAsset, BoneName);
    }
    if (!bCanAddBody) ImGui::EndDisabled();

    ImGui::SameLine();
    const bool bCanCreateConstraint = bHasSelectedBone && BodyIndex >= 0 && ParentBodyBoneIndex >= 0 && ConstraintIndex < 0;
    if (!bCanCreateConstraint) ImGui::BeginDisabled();
    if (ImGui::Button("+ Constraint To Parent", ImVec2(ButtonWidth, 0.0f)))
    {
        AddDefaultConstraintForBones(PhysicsAsset, ParentBodyBoneName, BoneName);
    }
    if (!bCanCreateConstraint) ImGui::EndDisabled();
}

void FPhysicsAssetEditorWidget::RenderUnboundPhysicsSetups(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton)
{
    if (!PhysicsAsset)
    {
        return;
    }

    bool bHasUnbound = false;
    for (const FPhysicsAssetBodySetup& Body : PhysicsAsset->GetBodySetups())
    {
        if (!IsBoneInReferenceSkeleton(RefSkeleton, Body.BoneName))
        {
            bHasUnbound = true;
            break;
        }
    }
    if (!bHasUnbound)
    {
        for (const FPhysicsAssetConstraintSetup& Constraint : PhysicsAsset->GetConstraintSetups())
        {
            if (!IsBoneInReferenceSkeleton(RefSkeleton, Constraint.ParentBoneName) ||
                !IsBoneInReferenceSkeleton(RefSkeleton, Constraint.ChildBoneName))
            {
                bHasUnbound = true;
                break;
            }
        }
    }

    if (!bHasUnbound)
    {
        return;
    }

    ImGui::Separator();
    if (!ImGui::TreeNodeEx("Unbound / Invalid Setups", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    for (int32 Index = 0; Index < static_cast<int32>(Bodies.size()); ++Index)
    {
        if (!IsBoneInReferenceSkeleton(RefSkeleton, Bodies[Index].BoneName))
        {
            const FString Label = BodyLabel(Bodies[Index], Index);
            if (ImGui::Selectable(Label.c_str(), SelectedBodyIndex == Index && SelectedConstraintIndex < 0))
            {
                SelectedBodyIndex = Index;
                SelectedShapeIndex = Bodies[Index].Shapes.empty() ? -1 : 0;
                SelectedConstraintIndex = -1;
                SelectedTreeBoneIndex = -1;
            }
        }
    }

    const TArray<FPhysicsAssetConstraintSetup>& Constraints = PhysicsAsset->GetConstraintSetups();
    for (int32 Index = 0; Index < static_cast<int32>(Constraints.size()); ++Index)
    {
        if (!IsBoneInReferenceSkeleton(RefSkeleton, Constraints[Index].ParentBoneName) ||
            !IsBoneInReferenceSkeleton(RefSkeleton, Constraints[Index].ChildBoneName))
        {
            const FString Label = ConstraintLabel(Constraints[Index], Index, PhysicsAsset->GetConstraintSetupEditorState(Index));
            if (ImGui::Selectable(Label.c_str(), SelectedConstraintIndex == Index))
            {
                SelectedConstraintIndex = Index;
                SelectedBodyIndex = -1;
                SelectedShapeIndex = -1;
                SelectedTreeBoneIndex = -1;
            }
        }
    }

    ImGui::TreePop();
}

void FPhysicsAssetEditorWidget::RenderBodyList(UPhysicsAsset* PhysicsAsset)
{
    ImGui::TextUnformatted("Bodies");
    ImGui::Separator();
    ImGui::BeginChild("PhysicsAssetBodyTree", ImVec2(0.0f, 150.0f), true);
    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    for (int32 i = 0; i < static_cast<int32>(Bodies.size()); ++i)
    {
        const FString Label = BodyLabel(Bodies[i], i);
        if (ImGui::Selectable(Label.c_str(), SelectedBodyIndex == i && SelectedConstraintIndex < 0))
        {
            SelectedBodyIndex = i;
            SelectedShapeIndex = Bodies[i].Shapes.empty() ? -1 : 0;
            SelectedConstraintIndex = -1;
            SelectedTreeBoneIndex = -1;
        }
    }
    if (Bodies.empty()) ImGui::TextDisabled("No bodies. Click Add Body to start authoring.");
    ImGui::EndChild();

    //if (ImGui::Button("Add Body##BodyList")) AddDefaultBody(PhysicsAsset);
    //ImGui::SameLine();
    //const bool bCanRemove = SelectedBodyIndex >= 0 && SelectedBodyIndex < static_cast<int32>(Bodies.size());
    //if (!bCanRemove) ImGui::BeginDisabled();
    //if (ImGui::Button("Remove Body"))
    //{
    //    PhysicsAsset->RemoveBodySetupByIndex(SelectedBodyIndex);
    //    SelectedBodyIndex = -1;
    //    SelectedShapeIndex = -1;
    //    MarkPhysicsAssetDirty();
    //}
    //if (!bCanRemove) ImGui::EndDisabled();
}

void FPhysicsAssetEditorWidget::RenderConstraintList(UPhysicsAsset* PhysicsAsset)
{
    ImGui::TextUnformatted("Constraints");
    ImGui::BeginChild("##PhysicsAssetConstraintList", ImVec2(0.0f, 185.0f), true);
    const TArray<FPhysicsAssetConstraintSetup>& Constraints = PhysicsAsset->GetConstraintSetups();
    for (int32 i = 0; i < static_cast<int32>(Constraints.size()); ++i)
    {
        const FString Label = ConstraintLabel(Constraints[i], i, PhysicsAsset->GetConstraintSetupEditorState(i));
        if (ImGui::Selectable(Label.c_str(), SelectedConstraintIndex == i))
        {
            SelectedConstraintIndex = i;
            SelectedBodyIndex = -1;
            SelectedShapeIndex = -1;
            SelectedTreeBoneIndex = -1;
        }
    }
    if (Constraints.empty()) ImGui::TextDisabled("No constraints. Click Add Constraint to start authoring.");
    ImGui::EndChild();

    if (ImGui::Button("Add Constraint##ConstraintList")) AddDefaultConstraint(PhysicsAsset);
    ImGui::SameLine();
    const bool bCanRemove = SelectedConstraintIndex >= 0 && SelectedConstraintIndex < static_cast<int32>(Constraints.size());
    if (!bCanRemove) ImGui::BeginDisabled();
    if (ImGui::Button("Remove Constraint"))
    {
        PhysicsAsset->RemoveConstraintSetupByIndex(SelectedConstraintIndex);
        SelectedConstraintIndex = -1;
        MarkPhysicsAssetDirty();
    }
    if (!bCanRemove) ImGui::EndDisabled();
}


void FPhysicsAssetEditorWidget::InitializeConstraintGraphEditor()
{
    if (ConstraintGraphContext)
    {
        return;
    }

    ed::Config Config;
    ConstraintGraphContext = ed::CreateEditor(&Config);
    bConstraintGraphLayoutDirty = true;
    ConstraintGraphTopologyHash = 0;
}

void FPhysicsAssetEditorWidget::DestroyConstraintGraphEditor()
{
    if (ConstraintGraphContext)
    {
        ed::DestroyEditor(ConstraintGraphContext);
        ConstraintGraphContext = nullptr;
    }
}

void FPhysicsAssetEditorWidget::RenderConstraintGraphPanel(UPhysicsAsset* PhysicsAsset)
{
    ImGui::TextUnformatted("Constraint Graph");

    if (!PhysicsAsset)
    {
        ImGui::TextDisabled("No PhysicsAsset selected.");
        return;
    }

    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    const TArray<FPhysicsAssetConstraintSetup>& Constraints = PhysicsAsset->GetConstraintSetups();
    if (Bodies.empty())
    {
        ImGui::BeginChild("##PhysicsConstraintGraphEmpty", ImVec2(0.0f, 0.0f), true);
        ImGui::TextDisabled("No bodies. Add bodies from the Skeleton Physics Tree first.");
        ImGui::EndChild();
        return;
    }

    InitializeConstraintGraphEditor();
    if (!ConstraintGraphContext)
    {
        ImGui::TextDisabled("Constraint graph editor is unavailable.");
        return;
    }

    uint64 TopologyHash = 0;
    HashCombine(TopologyHash, static_cast<uint64>(Bodies.size()));
    HashCombine(TopologyHash, static_cast<uint64>(Constraints.size()));
    for (const FPhysicsAssetBodySetup& Body : Bodies)
    {
        HashName(TopologyHash, Body.BoneName);
        HashCombine(TopologyHash, static_cast<uint64>(Body.Shapes.size()));
    }
    for (const FPhysicsAssetConstraintSetup& Constraint : Constraints)
    {
        HashName(TopologyHash, Constraint.ParentBoneName);
        HashName(TopologyHash, Constraint.ChildBoneName);
    }

    if (TopologyHash != ConstraintGraphTopologyHash)
    {
        ConstraintGraphTopologyHash = TopologyHash;
        bConstraintGraphLayoutDirty = true;
    }

    ImGui::BeginChild("##PhysicsConstraintGraphHost", ImVec2(0.0f, 0.0f), true);
    ed::SetCurrentEditor(ConstraintGraphContext);
    ed::Begin("PhysicsConstraintGraph");

    if (bConstraintGraphLayoutDirty)
    {
        for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(Bodies.size()); ++BodyIndex)
        {
            const float X = (BodyIndex % 2 == 0) ? 20.0f : 520.0f;
            const float Y = 30.0f + static_cast<float>(BodyIndex / 2) * 115.0f;
            ed::SetNodePosition(ToPhysicsNodeId(MakeBodyNodeId(BodyIndex)), ImVec2(X, Y));
        }

        for (int32 ConstraintIndex = 0; ConstraintIndex < static_cast<int32>(Constraints.size()); ++ConstraintIndex)
        {
            ed::SetNodePosition(
                ToPhysicsNodeId(MakeConstraintNodeId(ConstraintIndex)),
                ImVec2(275.0f, 45.0f + static_cast<float>(ConstraintIndex) * 115.0f));
        }
        bConstraintGraphLayoutDirty = false;
    }

    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(Bodies.size()); ++BodyIndex)
    {
        const FPhysicsAssetBodySetup& Body = Bodies[BodyIndex];
        const bool bSelected = SelectedBodyIndex == BodyIndex && SelectedConstraintIndex < 0;
        const FString BoneLabel = Body.BoneName == FName::None ? FString("<None>") : Body.BoneName.ToString();
        const int32 ShapeCount = static_cast<int32>(Body.Shapes.size());

        ed::BeginNode(ToPhysicsNodeId(MakeBodyNodeId(BodyIndex)));
        ed::BeginPin(ToPhysicsPinId(MakeBodyInputPinId(BodyIndex)), ed::PinKind::Input);
        ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.80f, 1.0f), "o");
        ed::EndPin();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextColored(
            bSelected ? ImVec4(1.0f, 0.86f, 0.18f, 1.0f) : ImVec4(0.64f, 0.88f, 0.62f, 1.0f),
            "Body");
        if (ImGui::IsItemClicked())
        {
            SelectedBodyIndex = BodyIndex;
            SelectedShapeIndex = Body.Shapes.empty() ? -1 : 0;
            SelectedConstraintIndex = -1;
            SelectedTreeBoneIndex = -1;
        }
        ImGui::TextUnformatted(BoneLabel.c_str());
        if (ImGui::IsItemClicked())
        {
            SelectedBodyIndex = BodyIndex;
            SelectedShapeIndex = Body.Shapes.empty() ? -1 : 0;
            SelectedConstraintIndex = -1;
            SelectedTreeBoneIndex = -1;
        }
        ImGui::TextDisabled("%d %s", ShapeCount, ShapeCount == 1 ? "shape" : "shapes");
        ImGui::EndGroup();
        ImGui::SameLine();
        ed::BeginPin(ToPhysicsPinId(MakeBodyOutputPinId(BodyIndex)), ed::PinKind::Output);
        ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.80f, 1.0f), "o");
        ed::EndPin();
        ed::EndNode();
    }

    for (int32 ConstraintIndex = 0; ConstraintIndex < static_cast<int32>(Constraints.size()); ++ConstraintIndex)
    {
        const FPhysicsAssetConstraintSetup& Constraint = Constraints[ConstraintIndex];
        const bool bSelected = SelectedConstraintIndex == ConstraintIndex;
        const FString ParentLabel = Constraint.ParentBoneName == FName::None ? FString("<None>") : Constraint.ParentBoneName.ToString();
        const FString ChildLabel = Constraint.ChildBoneName == FName::None ? FString("<None>") : Constraint.ChildBoneName.ToString();

        ed::BeginNode(ToPhysicsNodeId(MakeConstraintNodeId(ConstraintIndex)));
        ed::BeginPin(ToPhysicsPinId(MakeConstraintInputPinId(ConstraintIndex)), ed::PinKind::Input);
        ImGui::TextColored(ImVec4(0.90f, 0.86f, 0.55f, 1.0f), "o");
        ed::EndPin();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextColored(
            bSelected ? ImVec4(1.0f, 0.70f, 0.42f, 1.0f) : ImVec4(0.90f, 0.86f, 0.55f, 1.0f),
            "Constraint");
        if (ImGui::IsItemClicked())
        {
            SelectedConstraintIndex = ConstraintIndex;
            SelectedBodyIndex = -1;
            SelectedShapeIndex = -1;
            SelectedTreeBoneIndex = -1;
        }
        ImGui::TextDisabled("%s -> %s", ParentLabel.c_str(), ChildLabel.c_str());
        if (ImGui::IsItemClicked())
        {
            SelectedConstraintIndex = ConstraintIndex;
            SelectedBodyIndex = -1;
            SelectedShapeIndex = -1;
            SelectedTreeBoneIndex = -1;
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ed::BeginPin(ToPhysicsPinId(MakeConstraintOutputPinId(ConstraintIndex)), ed::PinKind::Output);
        ImGui::TextColored(ImVec4(0.90f, 0.86f, 0.55f, 1.0f), "o");
        ed::EndPin();
        ed::EndNode();
    }

    for (int32 ConstraintIndex = 0; ConstraintIndex < static_cast<int32>(Constraints.size()); ++ConstraintIndex)
    {
        const FPhysicsAssetConstraintSetup& Constraint = Constraints[ConstraintIndex];
        const int32 ParentBodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(Constraint.ParentBoneName);
        const int32 ChildBodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(Constraint.ChildBoneName);
        if (ParentBodyIndex >= 0)
        {
            ed::Link(
                ToPhysicsLinkId(MakeParentLinkId(ConstraintIndex)),
                ToPhysicsPinId(MakeBodyOutputPinId(ParentBodyIndex)),
                ToPhysicsPinId(MakeConstraintInputPinId(ConstraintIndex)),
                ImColor(130, 210, 190));
        }
        if (ChildBodyIndex >= 0)
        {
            ed::Link(
                ToPhysicsLinkId(MakeChildLinkId(ConstraintIndex)),
                ToPhysicsPinId(MakeConstraintOutputPinId(ConstraintIndex)),
                ToPhysicsPinId(MakeBodyInputPinId(ChildBodyIndex)),
                ImColor(225, 215, 145));
        }
    }

    if (ed::BeginCreate())
    {
        ed::PinId StartPinId = 0;
        ed::PinId EndPinId = 0;
        if (ed::QueryNewLink(&StartPinId, &EndPinId) && StartPinId && EndPinId)
        {
            int32 ParentBodyIndex = -1;
            int32 ChildBodyIndex = -1;
            const uint32 StartPin = PhysicsPinIdToU32(StartPinId);
            const uint32 EndPin = PhysicsPinIdToU32(EndPinId);
            const bool bForward = DecodeBodyOutputPin(StartPin, ParentBodyIndex) && DecodeBodyInputPin(EndPin, ChildBodyIndex);
            const bool bReverse = DecodeBodyOutputPin(EndPin, ParentBodyIndex) && DecodeBodyInputPin(StartPin, ChildBodyIndex);

            const bool bCanCreate =
                (bForward || bReverse) &&
                ParentBodyIndex != ChildBodyIndex &&
                IsValidBodyIndex(PhysicsAsset, ParentBodyIndex) &&
                IsValidBodyIndex(PhysicsAsset, ChildBodyIndex) &&
                HasBoneName(Bodies[ParentBodyIndex].BoneName) &&
                HasBoneName(Bodies[ChildBodyIndex].BoneName) &&
                !PhysicsAsset->HasConstraintBetweenBones(Bodies[ParentBodyIndex].BoneName, Bodies[ChildBodyIndex].BoneName);

            if (bCanCreate)
            {
                if (ed::AcceptNewItem(ImVec4(0.55f, 0.90f, 0.80f, 1.0f), 2.0f))
                {
                    AddDefaultConstraintForBones(PhysicsAsset, Bodies[ParentBodyIndex].BoneName, Bodies[ChildBodyIndex].BoneName);
                    bConstraintGraphLayoutDirty = true;
                }
            }
            else
            {
                ed::RejectNewItem(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), 2.0f);
            }
        }
    }
    ed::EndCreate();

    TArray<int32> ConstraintsToDelete;
    TArray<int32> BodiesToDelete;
    if (ed::BeginDelete())
    {
        ed::LinkId DeletedLinkId = 0;
        while (ed::QueryDeletedLink(&DeletedLinkId))
        {
            int32 ConstraintIndex = -1;
            if (DecodeConstraintLink(PhysicsLinkIdToU32(DeletedLinkId), ConstraintIndex) &&
                IsValidConstraintIndex(PhysicsAsset, ConstraintIndex))
            {
                if (ed::AcceptDeletedItem())
                {
                    ConstraintsToDelete.push_back(ConstraintIndex);
                }
            }
        }

        ed::NodeId DeletedNodeId = 0;
        while (ed::QueryDeletedNode(&DeletedNodeId))
        {
            int32 BodyIndex = -1;
            int32 ConstraintIndex = -1;
            const uint32 DeletedNode = PhysicsNodeIdToU32(DeletedNodeId);
            if (DecodeBodyNode(DeletedNode, BodyIndex) && IsValidBodyIndex(PhysicsAsset, BodyIndex))
            {
                if (ed::AcceptDeletedItem())
                {
                    BodiesToDelete.push_back(BodyIndex);
                }
            }
            else if (DecodeConstraintNode(DeletedNode, ConstraintIndex) && IsValidConstraintIndex(PhysicsAsset, ConstraintIndex))
            {
                if (ed::AcceptDeletedItem())
                {
                    ConstraintsToDelete.push_back(ConstraintIndex);
                }
            }
        }
    }
    ed::EndDelete();

    if (!ConstraintsToDelete.empty())
    {
        std::sort(ConstraintsToDelete.begin(), ConstraintsToDelete.end(), std::greater<int32>());
        ConstraintsToDelete.erase(std::unique(ConstraintsToDelete.begin(), ConstraintsToDelete.end()), ConstraintsToDelete.end());
        for (int32 ConstraintIndex : ConstraintsToDelete)
        {
            if (PhysicsAsset->RemoveConstraintSetupByIndex(ConstraintIndex))
            {
                SelectedConstraintIndex = -1;
                MarkPhysicsAssetDirty();
            }
        }
        bConstraintGraphLayoutDirty = true;
    }

    if (!BodiesToDelete.empty())
    {
        std::sort(BodiesToDelete.begin(), BodiesToDelete.end(), std::greater<int32>());
        BodiesToDelete.erase(std::unique(BodiesToDelete.begin(), BodiesToDelete.end()), BodiesToDelete.end());
        for (int32 BodyIndex : BodiesToDelete)
        {
            if (PhysicsAsset->RemoveBodySetupByIndex(BodyIndex))
            {
                SelectedBodyIndex = -1;
                SelectedShapeIndex = -1;
                SelectedConstraintIndex = -1;
                MarkPhysicsAssetDirty();
            }
        }
        bConstraintGraphLayoutDirty = true;
    }

    ed::End();
    ImGui::EndChild();
}

void FPhysicsAssetEditorWidget::RenderDetailsPanel(UPhysicsAsset* PhysicsAsset)
{
    if (SelectedBodyIndex >= 0 && SelectedBodyIndex < static_cast<int32>(PhysicsAsset->GetMutableBodySetups().size()))
    {
        ImGui::TextUnformatted("Body Details");
        RenderBodyDetails(PhysicsAsset, PhysicsAsset->GetMutableBodySetups()[SelectedBodyIndex]);
        return;
    }

    if (SelectedConstraintIndex >= 0 && SelectedConstraintIndex < static_cast<int32>(PhysicsAsset->GetMutableConstraintSetups().size()))
    {
        ImGui::TextUnformatted("Constraint Details");
        RenderConstraintDetails(PhysicsAsset, PhysicsAsset->GetMutableConstraintSetups()[SelectedConstraintIndex]);
        return;
    }

    if (SelectedTreeBoneIndex >= 0 && PreviewSkeletalMesh)
    {
        RenderSelectedBoneDetails(PhysicsAsset, PreviewSkeletalMesh);
        return;
    }

    ImGui::TextDisabled("Select a bone, body, or constraint to edit details.");
}

void FPhysicsAssetEditorWidget::RenderSelectedBoneDetails(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh)
{
    USkeleton* Skeleton = PreviewMesh ? PreviewMesh->GetSkeleton() : nullptr;
    if (!PhysicsAsset || !Skeleton)
    {
        ImGui::TextDisabled("Select a bone, body, or constraint to edit details.");
        return;
    }

    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
    if (SelectedTreeBoneIndex < 0 || SelectedTreeBoneIndex >= RefSkeleton.GetNumBones())
    {
        ImGui::TextDisabled("Selected bone is invalid.");
        return;
    }

    const FReferenceBone& Bone = RefSkeleton.Bones[SelectedTreeBoneIndex];
    const FName BoneName(Bone.Name);
    const int32 BodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(BoneName);
    const int32 ParentBodyBoneIndex = FindNearestParentBodyBoneIndex(PhysicsAsset, RefSkeleton, SelectedTreeBoneIndex);
    const FName ParentBodyBoneName = ParentBodyBoneIndex >= 0 ? FName(RefSkeleton.Bones[ParentBodyBoneIndex].Name) : FName::None;
    const int32 ConstraintIndex = ParentBodyBoneIndex >= 0
        ? PhysicsAsset->FindConstraintSetupIndex(ParentBodyBoneName, BoneName)
        : -1;

    ImGui::TextUnformatted("Bone Details");
    ImGui::Text("Name: %s", Bone.Name.c_str());
    ImGui::Text("Index: %d", SelectedTreeBoneIndex);
    ImGui::Text("Parent Index: %d", Bone.ParentIndex);
    ImGui::Separator();

    if (BodyIndex >= 0)
    {
        ImGui::TextDisabled("This bone already has a Physics Body.");
        if (ImGui::Button("Select Body", ImVec2(140.0f, 0.0f)))
        {
            SelectedBodyIndex = BodyIndex;
            SelectedShapeIndex = PhysicsAsset->GetBodySetups()[BodyIndex].Shapes.empty() ? -1 : 0;
            SelectedConstraintIndex = -1;
        }
    }
    else
    {
        if (ImGui::Button("Add Body to Bone", ImVec2(180.0f, 0.0f)))
        {
            AddDefaultBodyForBone(PhysicsAsset, BoneName);
        }
    }

    ImGui::Spacing();
    if (ParentBodyBoneIndex >= 0)
    {
        ImGui::TextDisabled("Nearest Parent Body: %s", ParentBodyBoneName.ToString().c_str());
        if (ConstraintIndex < 0)
        {
            const bool bCanCreateConstraint = BodyIndex >= 0;
            if (!bCanCreateConstraint) ImGui::BeginDisabled();
            if (ImGui::Button("Create Constraint to Parent Body", ImVec2(240.0f, 0.0f)))
            {
                AddDefaultConstraintForBones(PhysicsAsset, ParentBodyBoneName, BoneName);
            }
            if (!bCanCreateConstraint) ImGui::EndDisabled();
            if (!bCanCreateConstraint)
            {
                ImGui::TextDisabled("Add a Body to this bone first.");
            }
        }
    }
    else
    {
        ImGui::TextDisabled("No parent body found. This bone can act as a root body.");
    }
}

void FPhysicsAssetEditorWidget::RenderBodyDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetBodySetup& Body)
{
    bool bChanged = false;
    ImGui::Text("Bone Name: %s", Body.BoneName == FName::None ? "<None>" : Body.BoneName.ToString().c_str());
    //ImGui::TextDisabled("Bodies should be bound from the Skeleton Physics Tree, not typed manually.");
    //if (PreviewSkeletalMesh && PreviewSkeletalMesh->GetSkeleton() &&
    //    SelectedTreeBoneIndex >= 0 &&
    //    SelectedTreeBoneIndex < PreviewSkeletalMesh->GetSkeleton()->GetReferenceSkeleton().GetNumBones())
    //{
    //    const FReferenceSkeleton& RefSkeleton = PreviewSkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
    //    const FName SelectedBoneName(RefSkeleton.Bones[SelectedTreeBoneIndex].Name);
    //    const bool bCanBindToSelectedBone =
    //        SelectedBoneName != Body.BoneName &&
    //        !PhysicsAsset->HasBodySetupForBone(SelectedBoneName);
    //    if (!bCanBindToSelectedBone) ImGui::BeginDisabled();
    //    if (ImGui::Button("Bind Body to Selected Bone", ImVec2(-1.0f, 0.0f)))
    //    {
    //        Body.BoneName = SelectedBoneName;
    //        bChanged = true;
    //    }
    //    if (!bCanBindToSelectedBone) ImGui::EndDisabled();
    //}
    //if (SelectedShapeIndex >= 0 && SelectedShapeIndex < static_cast<int32>(Body.Shapes.size()))
    //{
    //    ImGui::TextDisabled("Viewport gizmo targets the selected shape. Clear/remove the selected shape to edit the Body Local Frame with the gizmo.");
    //}
    //else
    //{
    //    ImGui::TextDisabled("Viewport gizmo targets the Body Local Frame.");
    //}

    bChanged |= EditTransform("Body Local Frame", Body.BodyLocalFrame);
    bChanged |= DragMinFloat("Mass", Body.Mass, 0.05f, 0.001f);
    bChanged |= DragVec3("Center Of Mass Offset", Body.CenterOfMassLocalOffset, 0.1f);
    bChanged |= DragMinFloat("Linear Damping", Body.LinearDamping, 0.01f, 0.0f);
    bChanged |= DragMinFloat("Angular Damping", Body.AngularDamping, 0.01f, 0.0f);
    bChanged |= DragMinFloat("Max Angular Velocity", Body.MaxAngularVelocity, 0.1f, 0.0f);
    bChanged |= InputMinInt("Position Solver Iterations", Body.PositionSolverIterationCount, 1);
    bChanged |= InputMinInt("Velocity Solver Iterations", Body.VelocitySolverIterationCount, 1);
    bChanged |= ImGui::Checkbox("Enable CCD", &Body.bEnableCCD);
    bChanged |= ImGui::Checkbox("Enable Gravity", &Body.bEnableGravity);

    if (ImGui::TreeNodeEx("Locks", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bChanged |= ImGui::Checkbox("Lock Linear X", &Body.bLockLinearX); ImGui::SameLine();
        bChanged |= ImGui::Checkbox("Lock Linear Y", &Body.bLockLinearY); ImGui::SameLine();
        bChanged |= ImGui::Checkbox("Lock Linear Z", &Body.bLockLinearZ);
        bChanged |= ImGui::Checkbox("Lock Angular X", &Body.bLockAngularX); ImGui::SameLine();
        bChanged |= ImGui::Checkbox("Lock Angular Y", &Body.bLockAngularY); ImGui::SameLine();
        bChanged |= ImGui::Checkbox("Lock Angular Z", &Body.bLockAngularZ);
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Shapes");
    for (int32 i = 0; i < static_cast<int32>(Body.Shapes.size()); ++i)
    {
        char Label[96] = {};
        std::snprintf(Label, sizeof(Label), "%d: %s", i, ShapeTypeText(Body.Shapes[i].Type));
        if (ImGui::Selectable(Label, SelectedShapeIndex == i)) SelectedShapeIndex = i;
    }

    if (ImGui::Button("Add Shape"))
    {
        AddDefaultShape(Body);
        bChanged = true;
    }
    ImGui::SameLine();
    const bool bCanRemoveShape = SelectedShapeIndex >= 0 && SelectedShapeIndex < static_cast<int32>(Body.Shapes.size());
    if (!bCanRemoveShape) ImGui::BeginDisabled();
    if (ImGui::Button("Remove Shape"))
    {
        Body.Shapes.erase(Body.Shapes.begin() + SelectedShapeIndex);
        SelectedShapeIndex = Body.Shapes.empty() ? -1 : (std::min)(SelectedShapeIndex, static_cast<int32>(Body.Shapes.size()) - 1);
        bChanged = true;
    }
    if (!bCanRemoveShape) ImGui::EndDisabled();

    if (SelectedShapeIndex >= 0 && SelectedShapeIndex < static_cast<int32>(Body.Shapes.size()))
    {
        ImGui::Separator();
        RenderShapeDetails(Body.Shapes[SelectedShapeIndex]);
    }

    if (bChanged)
    {
        PhysicsAsset->UpdateBodySetup(SelectedBodyIndex, Body);
        MarkPhysicsAssetDirty();
    }
}

void FPhysicsAssetEditorWidget::RenderShapeDetails(FPhysicsAssetShapeSetup& Shape)
{
    bool bChanged = false;
    bChanged |= ComboShapeType("Shape Type", Shape.Type);
    bChanged |= EditTransform("Shape Local Transform", Shape.LocalTransform);
    switch (Shape.Type)
    {
    case EPhysicsAssetShapeType::Box:
        bChanged |= DragVec3("Box Half Extent", Shape.BoxHalfExtent, 0.1f);
        Shape.BoxHalfExtent.X = (std::max)(Shape.BoxHalfExtent.X, 0.001f);
        Shape.BoxHalfExtent.Y = (std::max)(Shape.BoxHalfExtent.Y, 0.001f);
        Shape.BoxHalfExtent.Z = (std::max)(Shape.BoxHalfExtent.Z, 0.001f);
        break;
    case EPhysicsAssetShapeType::Sphere:
        bChanged |= DragMinFloat("Sphere Radius", Shape.SphereRadius, 0.1f, 0.001f);
        break;
    case EPhysicsAssetShapeType::Capsule:
        bChanged |= DragMinFloat("Capsule Radius", Shape.CapsuleRadius, 0.1f, 0.001f);
        bChanged |= DragMinFloat("Capsule Half Height", Shape.CapsuleHalfHeight, 0.1f, 0.001f);
        break;
    default:
        break;
    }
    if (bChanged) MarkPhysicsAssetDirty();
}

void FPhysicsAssetEditorWidget::RenderConstraintDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetConstraintSetup& Constraint)
{
    bool bChanged = false;
    ImGui::Text("Parent Bone: %s", Constraint.ParentBoneName == FName::None ? "<None>" : Constraint.ParentBoneName.ToString().c_str());
    ImGui::Text("Child Bone: %s", Constraint.ChildBoneName == FName::None ? "<None>" : Constraint.ChildBoneName.ToString().c_str());
    ImGui::TextDisabled("Constraint endpoints are created from the bone tree or graph pins.");
    ImGui::Separator();

    ImGui::TextUnformatted("Viewport Gizmo Target");
    int GizmoFrame = (SelectedConstraintGizmoFrame == EPhysicsAssetConstraintFrameTarget::Parent) ? 0 : 1;
    if (ImGui::RadioButton("Parent Frame", &GizmoFrame, 0))
    {
        SelectedConstraintGizmoFrame = EPhysicsAssetConstraintFrameTarget::Parent;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Child Frame", &GizmoFrame, 1))
    {
        SelectedConstraintGizmoFrame = EPhysicsAssetConstraintFrameTarget::Child;
    }

    ImGui::Separator();
    bChanged |= EditTransform("Parent Local Frame", Constraint.ParentLocalFrame);
    bChanged |= EditTransform("Child Local Frame", Constraint.ChildLocalFrame);

    if (ImGui::TreeNodeEx("Linear Motion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bChanged |= ComboMotion("Linear X", Constraint.Limits.LinearX);
        bChanged |= ComboMotion("Linear Y", Constraint.Limits.LinearY);
        bChanged |= ComboMotion("Linear Z", Constraint.Limits.LinearZ);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Angular Motion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bChanged |= ComboMotion("Twist", Constraint.Limits.Twist);
        bChanged |= ComboMotion("Swing 1", Constraint.Limits.Swing1);
        bChanged |= ComboMotion("Swing 2", Constraint.Limits.Swing2);
        bChanged |= ImGui::DragFloat("Twist Min", &Constraint.Limits.TwistLimitMinDegrees, 0.1f);
        bChanged |= ImGui::DragFloat("Twist Max", &Constraint.Limits.TwistLimitMaxDegrees, 0.1f);
        bChanged |= DragMinFloat("Swing 1 Limit", Constraint.Limits.Swing1LimitDegrees, 0.1f, 0.0f);
        bChanged |= DragMinFloat("Swing 2 Limit", Constraint.Limits.Swing2LimitDegrees, 0.1f, 0.0f);
        ImGui::TreePop();
    }

    bChanged |= ImGui::Checkbox("Enable Projection", &Constraint.Limits.bEnableProjection);
    bChanged |= ImGui::Checkbox("Disable Collision Between Bodies", &Constraint.bDisableCollisionBetweenBodies);

    if (Constraint.Limits.TwistLimitMinDegrees > Constraint.Limits.TwistLimitMaxDegrees)
    {
        std::swap(Constraint.Limits.TwistLimitMinDegrees, Constraint.Limits.TwistLimitMaxDegrees);
        bChanged = true;
    }

    if (bChanged)
    {
        PhysicsAsset->UpdateConstraintSetup(SelectedConstraintIndex, Constraint);
        MarkPhysicsAssetDirty();
    }
}

void FPhysicsAssetEditorWidget::RenderValidationPanel()
{
    ImGui::TextUnformatted("Validation");
    if (!bValidationRan)
    {
        ImGui::TextDisabled("Click Validate to check skeleton, bodies, and constraints.");
        return;
    }

    if (!ValidationMeshPath.empty()) ImGui::TextDisabled("Mesh: %s", ValidationMeshPath.c_str());
    if (ValidationIssues.empty())
    {
        ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.45f, 1.0f), "No validation issues.");
        return;
    }

    ImGui::BeginChild("##PhysicsAssetValidation", ImVec2(0.0f, 150.0f), true);
    for (const FPhysicsAssetValidationIssue& Issue : ValidationIssues)
    {
        const bool bWarning = Issue.Severity == FPhysicsAssetValidationIssue::ESeverity::Warning;
        ImGui::TextColored(bWarning ? ImVec4(1.0f, 0.78f, 0.32f, 1.0f) : ImVec4(1.0f, 0.42f, 0.42f, 1.0f), "%s: %s", bWarning ? "Warning" : "Error", Issue.Message.c_str());
        if (Issue.BodyIndex >= 0 || Issue.ConstraintIndex >= 0)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Select"))
            {
                if (Issue.BodyIndex >= 0)
                {
                    SelectedBodyIndex = Issue.BodyIndex;
                    SelectedConstraintIndex = -1;
                }
                else
                {
                    SelectedConstraintIndex = Issue.ConstraintIndex;
                    SelectedBodyIndex = -1;
                    SelectedShapeIndex = -1;
                }
            }
        }
    }
    ImGui::EndChild();
}

void FPhysicsAssetEditorWidget::SelectBoneInPhysicsTree(
    UPhysicsAsset* PhysicsAsset,
    const FReferenceSkeleton& RefSkeleton,
    int32 BoneIndex)
{
    if (!PhysicsAsset || BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones())
    {
        return;
    }

    SelectedTreeBoneIndex = BoneIndex;
    SelectedBodyIndex = -1;
    SelectedShapeIndex = -1;
    SelectedConstraintIndex = -1;
}

void FPhysicsAssetEditorWidget::SelectBodySetup(UPhysicsAsset* PhysicsAsset, int32 BodyIndex, int32 TreeBoneIndex)
{
    if (!IsValidBodyIndex(PhysicsAsset, BodyIndex))
    {
        return;
    }

    SelectedBodyIndex = BodyIndex;
    SelectedShapeIndex = PhysicsAsset->GetBodySetups()[BodyIndex].Shapes.empty() ? -1 : 0;
    SelectedConstraintIndex = -1;
    SelectedTreeBoneIndex = TreeBoneIndex >= 0 ? TreeBoneIndex : FindPreviewBoneIndexByName(PhysicsAsset->GetBodySetups()[BodyIndex].BoneName);
}

void FPhysicsAssetEditorWidget::SelectConstraintSetup(UPhysicsAsset* PhysicsAsset, int32 ConstraintIndex, int32 TreeBoneIndex)
{
    if (!IsValidConstraintIndex(PhysicsAsset, ConstraintIndex))
    {
        return;
    }

    SelectedConstraintIndex = ConstraintIndex;
    SelectedBodyIndex = -1;
    SelectedShapeIndex = -1;
    SelectedConstraintGizmoFrame = EPhysicsAssetConstraintFrameTarget::Child;
    if (TreeBoneIndex >= 0)
    {
        SelectedTreeBoneIndex = TreeBoneIndex;
    }
    else
    {
        SelectedTreeBoneIndex = FindPreviewBoneIndexByName(PhysicsAsset->GetConstraintSetups()[ConstraintIndex].ChildBoneName);
    }
}

int32 FPhysicsAssetEditorWidget::FindPreviewBoneIndexByName(const FName& BoneName) const
{
    if (!HasBoneName(BoneName) || !PreviewSkeletalMesh || !PreviewSkeletalMesh->GetSkeleton())
    {
        return -1;
    }

    return PreviewSkeletalMesh->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(BoneName.ToString());
}

void FPhysicsAssetEditorWidget::AddConstraintToSelectedParentBody(UPhysicsAsset* PhysicsAsset)
{
    if (!PhysicsAsset || !PreviewSkeletalMesh || !PreviewSkeletalMesh->GetSkeleton())
    {
        return;
    }

    const FReferenceSkeleton& RefSkeleton = PreviewSkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
    int32 BoneIndex = SelectedTreeBoneIndex;
    if ((BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones()) && IsValidBodyIndex(PhysicsAsset, SelectedBodyIndex))
    {
        BoneIndex = FindPreviewBoneIndexByName(PhysicsAsset->GetBodySetups()[SelectedBodyIndex].BoneName);
    }

    if (BoneIndex < 0 || BoneIndex >= RefSkeleton.GetNumBones())
    {
        return;
    }

    const FName ChildBoneName(RefSkeleton.Bones[BoneIndex].Name);
    const int32 ChildBodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(ChildBoneName);
    const int32 ParentBodyBoneIndex = FindNearestParentBodyBoneIndex(PhysicsAsset, RefSkeleton, BoneIndex);
    if (ChildBodyIndex < 0 || ParentBodyBoneIndex < 0)
    {
        return;
    }

    const FName ParentBoneName(RefSkeleton.Bones[ParentBodyBoneIndex].Name);
    AddDefaultConstraintForBones(PhysicsAsset, ParentBoneName, ChildBoneName);
    SelectedTreeBoneIndex = BoneIndex;
}

void FPhysicsAssetEditorWidget::AddDefaultBody(UPhysicsAsset* PhysicsAsset)
{
    AddDefaultBodyForBone(PhysicsAsset, FName::None);
}

void FPhysicsAssetEditorWidget::AddDefaultBodyForBone(UPhysicsAsset* PhysicsAsset, const FName& BoneName)
{
    if (!PhysicsAsset) return;

    if (HasBoneName(BoneName))
    {
        const int32 ExistingBodyIndex = PhysicsAsset->FindBodySetupIndexByBoneName(BoneName);
        if (ExistingBodyIndex >= 0)
        {
            SelectBodySetup(PhysicsAsset, ExistingBodyIndex, FindPreviewBoneIndexByName(BoneName));
            return;
        }
    }

    FPhysicsAssetBodySetup Body;
    Body.BoneName = BoneName;

    FPhysicsAssetShapeSetup Shape;
    Shape.Type = EPhysicsAssetShapeType::Capsule;
    Shape.BoxHalfExtent = FVector(0.8f, 0.8f, 0.8f);
    Shape.SphereRadius = 0.8f;
    Shape.CapsuleRadius = 0.4f;
    Shape.CapsuleHalfHeight = 1.2f;
    Body.Shapes.push_back(Shape);

    const int32 NewIndex = PhysicsAsset->AddBodySetup(Body);
    if (NewIndex >= 0)
    {
        SelectBodySetup(PhysicsAsset, NewIndex, FindPreviewBoneIndexByName(BoneName));
        MarkPhysicsAssetDirty();
    }
}

void FPhysicsAssetEditorWidget::AddDefaultShape(FPhysicsAssetBodySetup& Body)
{
    FPhysicsAssetShapeSetup Shape;
    Shape.Type = EPhysicsAssetShapeType::Box;
    Shape.BoxHalfExtent = FVector(0.8f, 0.8f, 0.8f);
    Shape.SphereRadius = 0.8f;
    Shape.CapsuleRadius = 0.4f;
    Shape.CapsuleHalfHeight = 1.2f;
    Body.Shapes.push_back(Shape);
    SelectedShapeIndex = static_cast<int32>(Body.Shapes.size()) - 1;
}

void FPhysicsAssetEditorWidget::AddDefaultConstraint(UPhysicsAsset* PhysicsAsset)
{
    if (!PhysicsAsset) return;

    const int32 ConstraintCountBefore = static_cast<int32>(PhysicsAsset->GetConstraintSetups().size());
    const int32 PreviousConstraintIndex = SelectedConstraintIndex;
    AddConstraintToSelectedParentBody(PhysicsAsset);
    if (static_cast<int32>(PhysicsAsset->GetConstraintSetups().size()) != ConstraintCountBefore ||
        SelectedConstraintIndex != PreviousConstraintIndex)
    {
        return;
    }

    // Fallback for assets edited without a preview skeleton: connect the first two named bodies.
    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    if (Bodies.size() >= 2 && HasBoneName(Bodies[0].BoneName) && HasBoneName(Bodies[1].BoneName))
    {
        AddDefaultConstraintForBones(PhysicsAsset, Bodies[0].BoneName, Bodies[1].BoneName);
    }
}

void FPhysicsAssetEditorWidget::AddDefaultConstraintForBones(
    UPhysicsAsset* PhysicsAsset,
    const FName& ParentBoneName,
    const FName& ChildBoneName)
{
    if (!PhysicsAsset) return;
    if (!HasBoneName(ParentBoneName) || !HasBoneName(ChildBoneName) || ParentBoneName == ChildBoneName)
    {
        return;
    }
    if (!PhysicsAsset->HasBodySetupForBone(ParentBoneName) || !PhysicsAsset->HasBodySetupForBone(ChildBoneName))
    {
        return;
    }

    const int32 ExistingConstraintIndex = PhysicsAsset->FindConstraintSetupIndex(ParentBoneName, ChildBoneName);
    if (ExistingConstraintIndex >= 0)
    {
        SelectConstraintSetup(PhysicsAsset, ExistingConstraintIndex, FindPreviewBoneIndexByName(ChildBoneName));
        return;
    }

    FPhysicsAssetConstraintSetup Constraint;
    Constraint.ParentBoneName = ParentBoneName;
    Constraint.ChildBoneName = ChildBoneName;

    const int32 NewIndex = PhysicsAsset->AddConstraintSetup(Constraint);
    if (NewIndex >= 0)
    {
        SelectConstraintSetup(PhysicsAsset, NewIndex, FindPreviewBoneIndexByName(ChildBoneName));
        MarkPhysicsAssetDirty();
    }
}

void FPhysicsAssetEditorWidget::RunValidation(UPhysicsAsset* PhysicsAsset)
{
    ValidationIssues.clear();
    ValidationMeshPath.clear();
    bValidationRan = true;

    if (!PhysicsAsset)
    {
        FPhysicsAssetValidationIssue Issue;
        Issue.Message = "PhysicsAsset is missing.";
        ValidationIssues.push_back(Issue);
        return;
    }

    USkeletalMesh* ValidationMesh = nullptr;
    ID3D11Device* Device = EditorEngine ? EditorEngine->GetRenderer().GetFD3DDevice().GetDevice() : nullptr;
    const TArray<FAssetListItem> Meshes = FAssetRegistry::ListMeshesForSkeleton(PhysicsAsset->GetSkeletonBinding(), true);
    for (const FAssetListItem& Item : Meshes)
    {
        if (USkeletalMesh* Mesh = FMeshManager::LoadSkeletalMesh(Item.FullPath, Device))
        {
            ValidationMesh = Mesh;
            ValidationMeshPath = Item.FullPath;
            break;
        }
    }

    if (!ValidationMesh)
    {
        FPhysicsAssetValidationIssue Issue;
        Issue.Message = "No compatible skeletal mesh was found for this PhysicsAsset skeleton binding.";
        Issue.Severity = FPhysicsAssetValidationIssue::ESeverity::Warning;
        ValidationIssues.push_back(Issue);
        return;
    }

    FPhysicsAssetValidation::ValidateAll(ValidationMesh, PhysicsAsset, ValidationIssues);
}

void FPhysicsAssetEditorWidget::RenderPreviewDebug(
    UPhysicsAsset* PhysicsAsset,
    USkeletalMesh* PreviewMesh,
    UWorld* PreviewWorld,
    USkeletalMeshComponent* PreviewComponent)
{
    PreviewSkeletalMesh = PreviewMesh;

    if (!PhysicsAsset || !PreviewWorld || !PreviewComponent)
    {
        return;
    }

    ClampSelection(PhysicsAsset);

    if (bShowPreviewBodies)
    {
        RenderBodyDebug(PhysicsAsset, PreviewComponent, PreviewWorld);
    }

    if (bShowPreviewConstraints)
    {
        RenderConstraintDebug(PhysicsAsset, PreviewComponent, PreviewWorld);
    }
}

void FPhysicsAssetEditorWidget::RenderPhysicsPreview(
    UPhysicsAsset* PhysicsAsset,
    USkeletalMesh* PreviewMesh,
    UWorld* PreviewWorld,
    USkeletalMeshComponent* PreviewComponent,
    UPhysicsAssetPreviewComponent* SolidPreviewComponent,
    ID3D11Device* Device)
{
    PreviewSkeletalMesh = PreviewMesh;

    if (!PhysicsAsset || !PreviewWorld || !PreviewComponent)
    {
        if (SolidPreviewComponent)
        {
            SolidPreviewComponent->ClearPreview(Device);
        }
        return;
    }

    ClampSelection(PhysicsAsset);

    if (SolidPreviewComponent)
    {
        SolidPreviewComponent->UpdatePreview(
            PhysicsAsset,
            PreviewComponent,
            SelectedBodyIndex,
            SelectedShapeIndex,
            SelectedConstraintIndex,
            bShowPreviewBodies,
            Device);
    }

    if (bShowPreviewBodies)
    {
        RenderBodyDebug(PhysicsAsset, PreviewComponent, PreviewWorld);
    }

    if (bShowPreviewConstraints)
    {
        RenderConstraintDebug(PhysicsAsset, PreviewComponent, PreviewWorld);
    }
}

void FPhysicsAssetEditorWidget::RenderViewportDebugOptions()
{
    ImGui::Separator();
    ImGui::TextUnformatted("Physics Preview");
    ImGui::Checkbox("Physics Bodies", &bShowPreviewBodies);
    ImGui::Checkbox("Physics Constraints", &bShowPreviewConstraints);
}

void FPhysicsAssetEditorWidget::RenderBodyDebug(
    UPhysicsAsset* PhysicsAsset,
    USkeletalMeshComponent* PreviewComponent,
    UWorld* PreviewWorld)
{
    if (!PhysicsAsset || !PreviewComponent || !PreviewWorld)
    {
        return;
    }

    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(Bodies.size()); ++BodyIndex)
    {
        DrawBodySetupDebug(PhysicsAsset, PreviewComponent, PreviewWorld, BodyIndex, Bodies[BodyIndex]);
    }
}

void FPhysicsAssetEditorWidget::RenderConstraintDebug(
    UPhysicsAsset* PhysicsAsset,
    USkeletalMeshComponent* PreviewComponent,
    UWorld* PreviewWorld)
{
    if (!PhysicsAsset || !PreviewComponent || !PreviewWorld)
    {
        return;
    }

    const TArray<FPhysicsAssetConstraintSetup>& Constraints = PhysicsAsset->GetConstraintSetups();
    for (int32 ConstraintIndex = 0; ConstraintIndex < static_cast<int32>(Constraints.size()); ++ConstraintIndex)
    {
        DrawConstraintSetupDebug(
            PhysicsAsset,
            PreviewComponent,
            PreviewWorld,
            ConstraintIndex,
            Constraints[ConstraintIndex]);
    }
}

void FPhysicsAssetEditorWidget::DrawBodySetupDebug(
    UPhysicsAsset* PhysicsAsset,
    USkeletalMeshComponent* PreviewComponent,
    UWorld* PreviewWorld,
    int32 BodyIndex,
    const FPhysicsAssetBodySetup& BodySetup)
{
    if (!PhysicsAsset || !PreviewComponent || !PreviewWorld)
    {
        return;
    }

    FTransform BodyWorld;
    if (!FPhysicsAssetPreviewUtils::ComputePreviewBodyWorldTransform(
            PreviewComponent,
            PhysicsAsset,
            BodyIndex,
            BodyWorld))
    {
        return;
    }

    const bool bSelectedBody = SelectedBodyIndex == BodyIndex && SelectedConstraintIndex < 0;
    for (int32 ShapeIndex = 0; ShapeIndex < static_cast<int32>(BodySetup.Shapes.size()); ++ShapeIndex)
    {
        const FPhysicsAssetShapeSetup& Shape = BodySetup.Shapes[ShapeIndex];
        const FTransform ShapeWorld = ComposePreviewDebugTransforms(BodyWorld, Shape.LocalTransform);
        const bool bSelectedShape = bSelectedBody && SelectedShapeIndex == ShapeIndex;
        const FColor Color = bSelectedShape
            ? FColor(255, 245, 120, 190)
            : (bSelectedBody ? FColor(255, 180, 60, 170) : FColor(90, 210, 255, 115));

        switch (Shape.Type)
        {
        case EPhysicsAssetShapeType::Box:
        {
            FVector HalfExtent = Shape.BoxHalfExtent * PhysicsPreviewShapeScale;
            HalfExtent.X = (std::max)(HalfExtent.X, 0.001f);
            HalfExtent.Y = (std::max)(HalfExtent.Y, 0.001f);
            HalfExtent.Z = (std::max)(HalfExtent.Z, 0.001f);
            DrawDebugOrientedBox(
                PreviewWorld,
                ShapeWorld.Location,
                HalfExtent,
                ShapeWorld.Rotation.GetNormalized(),
                Color);
            break;
        }
        case EPhysicsAssetShapeType::Sphere:
        {
            const float Radius = (std::max)(Shape.SphereRadius * PhysicsPreviewShapeScale, 0.001f);
            DrawDebugSphere(PreviewWorld, ShapeWorld.Location, Radius, 24, Color, 0.0f);
            break;
        }
        case EPhysicsAssetShapeType::Capsule:
        {
            const float Radius = (std::max)(Shape.CapsuleRadius * PhysicsPreviewShapeScale, 0.001f);
            const float HalfHeight = (std::max)(Shape.CapsuleHalfHeight * PhysicsPreviewShapeScale, Radius);
            DrawDebugCapsuleZAxis(
                PreviewWorld,
                ShapeWorld.Location,
                Radius,
                HalfHeight,
                ShapeWorld.Rotation.GetNormalized(),
                Color);
            break;
        }
        default:
            break;
        }
    }
}

void FPhysicsAssetEditorWidget::DrawConstraintSetupDebug(
    UPhysicsAsset* PhysicsAsset,
    USkeletalMeshComponent* PreviewComponent,
    UWorld* PreviewWorld,
    int32 ConstraintIndex,
    const FPhysicsAssetConstraintSetup& ConstraintSetup)
{
    (void)ConstraintSetup;
    if (!PhysicsAsset || !PreviewComponent || !PreviewWorld)
    {
        return;
    }

    FTransform ParentFrameWorld;
    FTransform ChildFrameWorld;
    if (!FPhysicsAssetPreviewUtils::ComputePreviewConstraintWorldFrames(
            PreviewComponent,
            PhysicsAsset,
            ConstraintIndex,
            ParentFrameWorld,
            ChildFrameWorld))
    {
        return;
    }

    const bool bSelected = SelectedConstraintIndex == ConstraintIndex;
    const FColor Color = bSelected ? FColor(255, 120, 80, 180) : FColor(200, 150, 255, 115);
    DrawDebugLine(PreviewWorld, ParentFrameWorld.Location, ChildFrameWorld.Location, Color, 0.0f);
    DrawDebugPoint(PreviewWorld, ParentFrameWorld.Location, bSelected ? 0.08f : 0.05f, Color, 0.0f);
    DrawDebugPoint(PreviewWorld, ChildFrameWorld.Location, bSelected ? 0.08f : 0.05f, Color, 0.0f);
}

void FPhysicsAssetEditorWidget::MarkPhysicsAssetDirty()
{
    MarkDirty();
    bConstraintGraphLayoutDirty = true;
    ConstraintGraphTopologyHash = 0;
    bValidationRan = false;
    ValidationIssues.clear();
    ValidationMeshPath.clear();
}

void FPhysicsAssetEditorWidget::ClampSelection(UPhysicsAsset* PhysicsAsset)
{
    if (!PhysicsAsset)
    {
        SelectedBodyIndex = -1;
        SelectedShapeIndex = -1;
        SelectedConstraintIndex = -1;
        SelectedTreeBoneIndex = -1;
        return;
    }

    if (PreviewSkeletalMesh && PreviewSkeletalMesh->GetSkeleton())
    {
        const int32 BoneCount = PreviewSkeletalMesh->GetSkeleton()->GetReferenceSkeleton().GetNumBones();
        if (SelectedTreeBoneIndex >= BoneCount) SelectedTreeBoneIndex = -1;
    }
    else
    {
        SelectedTreeBoneIndex = -1;
    }

    const int32 BodyCount = static_cast<int32>(PhysicsAsset->GetBodySetups().size());
    const int32 ConstraintCount = static_cast<int32>(PhysicsAsset->GetConstraintSetups().size());
    if (SelectedBodyIndex >= BodyCount) SelectedBodyIndex = BodyCount - 1;
    if (SelectedConstraintIndex >= ConstraintCount) SelectedConstraintIndex = ConstraintCount - 1;

    if (SelectedBodyIndex < 0)
    {
        SelectedShapeIndex = -1;
    }
    else
    {
        const int32 ShapeCount = static_cast<int32>(PhysicsAsset->GetBodySetups()[SelectedBodyIndex].Shapes.size());
        if (SelectedShapeIndex >= ShapeCount) SelectedShapeIndex = ShapeCount - 1;
        if (SelectedShapeIndex < 0 && ShapeCount > 0) SelectedShapeIndex = 0;
    }
}
