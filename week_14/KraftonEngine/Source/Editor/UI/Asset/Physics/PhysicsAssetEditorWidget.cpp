#include "PhysicsAssetEditorWidget.h"

#include "Asset/AssetRegistry.h"
#include "Editor/EditorEngine.h"
#include "Mesh/MeshManager.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Object/Object.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetManager.h"

#include <algorithm>
#include <cstdio>

#include <imgui.h>

namespace
{
    constexpr float LeftPanelWidth = 310.0f;
    constexpr float DetailsPanelMinWidth = 360.0f;

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

    FString BodyLabel(const FPhysicsAssetBodySetup& Body, int32 Index, UPhysicsAsset::EEditorSetupState State)
    {
        const FString Bone = Body.BoneName == FName::None ? FString("<None>") : Body.BoneName.ToString();
        char Buffer[256] = {};
        std::snprintf(Buffer, sizeof(Buffer), "Body %d  %s  [%s]", Index, Bone.c_str(), StateText(State));
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
    SelectedBodyIndex = -1;
    SelectedShapeIndex = -1;
    SelectedConstraintIndex = -1;
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
    RenderAssetSummary(PhysicsAsset);
    ImGui::Separator();
    RenderBodyList(PhysicsAsset);
    ImGui::Separator();
    RenderConstraintList(PhysicsAsset);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##PhysicsAssetRight", ImVec2(RightWidth, 0.0f), true);
    RenderDetailsPanel(PhysicsAsset);
    ImGui::Separator();
    RenderValidationPanel();
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

void FPhysicsAssetEditorWidget::RenderToolbar(UPhysicsAsset* PhysicsAsset)
{
    const bool bCanSave = PhysicsAsset && !PhysicsAsset->GetAssetPathFileName().empty() && PhysicsAsset->GetAssetPathFileName() != "None";
    if (!bCanSave) ImGui::BeginDisabled();
    if (ImGui::Button(IsDirty() ? "Save*" : "Save", ImVec2(72.0f, 0.0f)))
    {
        if (FPhysicsAssetManager::Get().SavePhysicsAssetPreservingMetadata(PhysicsAsset))
        {
            ClearDirty();
            bValidationRan = false;
        }
    }
    if (!bCanSave) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Validate", ImVec2(86.0f, 0.0f))) RunValidation(PhysicsAsset);
    ImGui::SameLine();
    if (ImGui::Button("Add Body", ImVec2(92.0f, 0.0f))) AddDefaultBody(PhysicsAsset);
    ImGui::SameLine();
    if (ImGui::Button("Add Constraint", ImVec2(120.0f, 0.0f))) AddDefaultConstraint(PhysicsAsset);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", bCanSave ? PhysicsAsset->GetAssetPathFileName().c_str() : "Unsaved PhysicsAsset");
}

void FPhysicsAssetEditorWidget::RenderAssetSummary(UPhysicsAsset* PhysicsAsset)
{
    const FSkeletonBinding& Binding = PhysicsAsset->GetSkeletonBinding();
    ImGui::TextUnformatted("Physics Asset");
    ImGui::TextDisabled("Skeleton: %s", Binding.SkeletonPath.c_str());
    ImGui::TextDisabled("Bodies: %d  Constraints: %d", static_cast<int32>(PhysicsAsset->GetBodySetups().size()), static_cast<int32>(PhysicsAsset->GetConstraintSetups().size()));
}

void FPhysicsAssetEditorWidget::RenderBodyList(UPhysicsAsset* PhysicsAsset)
{
    ImGui::TextUnformatted("Bodies");
    ImGui::BeginChild("##PhysicsAssetBodyList", ImVec2(0.0f, 185.0f), true);
    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    for (int32 i = 0; i < static_cast<int32>(Bodies.size()); ++i)
    {
        const FString Label = BodyLabel(Bodies[i], i, PhysicsAsset->GetBodySetupEditorState(i));
        if (ImGui::Selectable(Label.c_str(), SelectedBodyIndex == i && SelectedConstraintIndex < 0))
        {
            SelectedBodyIndex = i;
            SelectedShapeIndex = Bodies[i].Shapes.empty() ? -1 : 0;
            SelectedConstraintIndex = -1;
        }
    }
    if (Bodies.empty()) ImGui::TextDisabled("No bodies. Click Add Body to start authoring.");
    ImGui::EndChild();

    if (ImGui::Button("Add Body##BodyList")) AddDefaultBody(PhysicsAsset);
    ImGui::SameLine();
    const bool bCanRemove = SelectedBodyIndex >= 0 && SelectedBodyIndex < static_cast<int32>(Bodies.size());
    if (!bCanRemove) ImGui::BeginDisabled();
    if (ImGui::Button("Remove Body"))
    {
        PhysicsAsset->RemoveBodySetupByIndex(SelectedBodyIndex);
        SelectedBodyIndex = -1;
        SelectedShapeIndex = -1;
        MarkPhysicsAssetDirty();
    }
    if (!bCanRemove) ImGui::EndDisabled();
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

    ImGui::TextDisabled("Select a body or constraint to edit details.");
}

void FPhysicsAssetEditorWidget::RenderBodyDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetBodySetup& Body)
{
    bool bChanged = false;
    bChanged |= InputName("Bone Name", Body.BoneName);
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
    bChanged |= InputName("Parent Bone", Constraint.ParentBoneName);
    bChanged |= InputName("Child Bone", Constraint.ChildBoneName);
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

void FPhysicsAssetEditorWidget::AddDefaultBody(UPhysicsAsset* PhysicsAsset)
{
    if (!PhysicsAsset) return;
    FPhysicsAssetBodySetup Body;
    FPhysicsAssetShapeSetup Shape;
    Shape.Type = EPhysicsAssetShapeType::Capsule;
    Shape.CapsuleRadius = 12.0f;
    Shape.CapsuleHalfHeight = 32.0f;
    Body.Shapes.push_back(Shape);

    const int32 NewIndex = PhysicsAsset->AddBodySetup(Body);
    if (NewIndex >= 0)
    {
        SelectedBodyIndex = NewIndex;
        SelectedShapeIndex = 0;
        SelectedConstraintIndex = -1;
        MarkPhysicsAssetDirty();
    }
}

void FPhysicsAssetEditorWidget::AddDefaultShape(FPhysicsAssetBodySetup& Body)
{
    FPhysicsAssetShapeSetup Shape;
    Shape.Type = EPhysicsAssetShapeType::Box;
    Shape.BoxHalfExtent = FVector(25.0f, 25.0f, 25.0f);
    Body.Shapes.push_back(Shape);
    SelectedShapeIndex = static_cast<int32>(Body.Shapes.size()) - 1;
}

void FPhysicsAssetEditorWidget::AddDefaultConstraint(UPhysicsAsset* PhysicsAsset)
{
    if (!PhysicsAsset) return;
    FPhysicsAssetConstraintSetup Constraint;
    const TArray<FPhysicsAssetBodySetup>& Bodies = PhysicsAsset->GetBodySetups();
    if (Bodies.size() >= 2)
    {
        if (HasBoneName(Bodies[0].BoneName)) Constraint.ParentBoneName = Bodies[0].BoneName;
        if (HasBoneName(Bodies[1].BoneName)) Constraint.ChildBoneName = Bodies[1].BoneName;
    }

    const int32 NewIndex = PhysicsAsset->AddConstraintSetup(Constraint);
    if (NewIndex >= 0)
    {
        SelectedConstraintIndex = NewIndex;
        SelectedBodyIndex = -1;
        SelectedShapeIndex = -1;
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

void FPhysicsAssetEditorWidget::MarkPhysicsAssetDirty()
{
    MarkDirty();
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
        return;
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
