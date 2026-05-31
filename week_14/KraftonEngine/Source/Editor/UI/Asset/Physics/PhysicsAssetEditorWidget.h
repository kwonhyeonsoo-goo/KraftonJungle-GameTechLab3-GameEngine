#pragma once

#include "Editor/UI/Asset/AssetEditorWidget.h"
#include "Physics/PhysicsAssetValidation.h"
#include "Object/FName.h"

class UPhysicsAsset;
class USkeletalMesh;
struct FReferenceSkeleton;
struct FPhysicsAssetBodySetup;
struct FPhysicsAssetConstraintSetup;
struct FPhysicsAssetShapeSetup;

class FPhysicsAssetEditorWidget : public FAssetEditorWidget
{
public:
    FPhysicsAssetEditorWidget() = default;

    bool CanEdit(UObject* Object) const override;
    bool IsEditingObject(UObject* Object) const override;
    bool AllowsMultipleInstances() const override { return true; }

    void Open(UObject* Object) override;
    void Render(float DeltaTime) override;
    void RenderDocument(float DeltaTime) override;

    void OpenEmbedded(UPhysicsAsset* PhysicsAsset);
    void RenderEmbedded(UPhysicsAsset* PhysicsAsset, float DeltaTime);
    void RenderEmbedded(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime);
    bool SaveEditedPhysicsAsset();
    bool HasUnsavedChanges() const { return IsDirty(); }

    FString GetDocumentTitle() const override;
    FString GetDocumentPayloadId() const override;
    EEditorDocumentTabKind GetDocumentTabKind() const override { return EEditorDocumentTabKind::PhysicsAssetEditor; }

private:
    UPhysicsAsset* GetEditedPhysicsAsset() const;

    void RenderToolbar(UPhysicsAsset* PhysicsAsset);
    void RenderAssetSummary(UPhysicsAsset* PhysicsAsset);
    void RenderSkeletonPhysicsTree(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh);
    void RenderPhysicsBoneTree(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);
    void RenderPhysicsBoneContextMenu(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);
    void RenderUnboundPhysicsSetups(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton);
    void RenderBodyList(UPhysicsAsset* PhysicsAsset);
    void RenderConstraintList(UPhysicsAsset* PhysicsAsset);
    void RenderDetailsPanel(UPhysicsAsset* PhysicsAsset);
    void RenderSelectedBoneDetails(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh);
    void RenderBodyDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetBodySetup& BodySetup);
    void RenderShapeDetails(FPhysicsAssetShapeSetup& ShapeSetup);
    void RenderConstraintDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetConstraintSetup& ConstraintSetup);
    void RenderValidationPanel();

    void AddDefaultBody(UPhysicsAsset* PhysicsAsset);
    void AddDefaultBodyForBone(UPhysicsAsset* PhysicsAsset, const FName& BoneName);
    void AddDefaultShape(FPhysicsAssetBodySetup& BodySetup);
    void AddDefaultConstraint(UPhysicsAsset* PhysicsAsset);
    void AddDefaultConstraintForBones(UPhysicsAsset* PhysicsAsset, const FName& ParentBoneName, const FName& ChildBoneName);
    void RunValidation(UPhysicsAsset* PhysicsAsset);
    void MarkPhysicsAssetDirty();
    void ClampSelection(UPhysicsAsset* PhysicsAsset);

private:
    int32 SelectedBodyIndex = -1;
    int32 SelectedShapeIndex = -1;
    int32 SelectedConstraintIndex = -1;
    int32 SelectedTreeBoneIndex = -1;
    USkeletalMesh* PreviewSkeletalMesh = nullptr;
    bool bPendingClose = false;

    TArray<FPhysicsAssetValidationIssue> ValidationIssues;
    FString ValidationMeshPath;
    bool bValidationRan = false;
};
