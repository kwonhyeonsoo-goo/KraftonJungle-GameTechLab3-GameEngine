#pragma once

#include "Editor/UI/Asset/AssetEditorWidget.h"
#include "Physics/PhysicsAssetValidation.h"
#include "Object/FName.h"

namespace ax { namespace NodeEditor { struct EditorContext; } }

class UPhysicsAsset;
class UPhysicsAssetPreviewComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UWorld;
struct ID3D11Device;
struct FReferenceSkeleton;
struct FPhysicsAssetBodySetup;
struct FPhysicsAssetConstraintSetup;
struct FPhysicsAssetShapeSetup;

class FPhysicsAssetEditorWidget : public FAssetEditorWidget
{
public:
    FPhysicsAssetEditorWidget() = default;
    ~FPhysicsAssetEditorWidget() override;

    bool CanEdit(UObject* Object) const override;
    bool IsEditingObject(UObject* Object) const override;
    bool AllowsMultipleInstances() const override { return true; }

    void Open(UObject* Object) override;
    void Close() override;
    void Render(float DeltaTime) override;
    void RenderDocument(float DeltaTime) override;

    void OpenEmbedded(UPhysicsAsset* PhysicsAsset);
    void RenderEmbedded(UPhysicsAsset* PhysicsAsset, float DeltaTime);
    void RenderEmbedded(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime);
    void RenderEmbeddedToolbar(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime);
    void RenderEmbeddedTreeAndGraph(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime);
    void RenderEmbeddedDetails(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh, float DeltaTime);
    void RenderPreviewDebug(
        UPhysicsAsset* PhysicsAsset,
        USkeletalMesh* PreviewMesh,
        UWorld* PreviewWorld,
        USkeletalMeshComponent* PreviewComponent);
    void RenderPhysicsPreview(
        UPhysicsAsset* PhysicsAsset,
        USkeletalMesh* PreviewMesh,
        UWorld* PreviewWorld,
        USkeletalMeshComponent* PreviewComponent,
        UPhysicsAssetPreviewComponent* SolidPreviewComponent,
        ID3D11Device* Device);
    void RenderViewportDebugOptions();
    bool SaveEditedPhysicsAsset();
    bool HasUnsavedChanges() const { return IsDirty(); }

    FString GetDocumentTitle() const override;
    FString GetDocumentPayloadId() const override;
    EEditorDocumentTabKind GetDocumentTabKind() const override { return EEditorDocumentTabKind::PhysicsAssetEditor; }

private:
    UPhysicsAsset* GetEditedPhysicsAsset() const;
    bool PrepareEmbeddedRender(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh);
    void RenderTreeAndGraphPanel(UPhysicsAsset* PhysicsAsset);
    void RenderDetailsAndValidationPanel(UPhysicsAsset* PhysicsAsset);

    void RenderToolbar(UPhysicsAsset* PhysicsAsset);
    void RenderAssetSummary(UPhysicsAsset* PhysicsAsset);
    void RenderSkeletonPhysicsTree(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh);
    void RenderPhysicsBoneTree(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);
    void RenderPhysicsBoneContextMenu(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);
    void RenderSelectedBoneActionPanel(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton);
    void RenderUnboundPhysicsSetups(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton);
    void RenderBodyList(UPhysicsAsset* PhysicsAsset);
    void RenderConstraintList(UPhysicsAsset* PhysicsAsset);
    void RenderDetailsPanel(UPhysicsAsset* PhysicsAsset);
    void RenderSelectedBoneDetails(UPhysicsAsset* PhysicsAsset, USkeletalMesh* PreviewMesh);
    void RenderBodyDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetBodySetup& BodySetup);
    void RenderShapeDetails(FPhysicsAssetShapeSetup& ShapeSetup);
    void RenderConstraintDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetConstraintSetup& ConstraintSetup);
    void RenderValidationPanel();
    void RenderConstraintGraphPanel(UPhysicsAsset* PhysicsAsset);
    void RenderBodyDebug(UPhysicsAsset* PhysicsAsset, USkeletalMeshComponent* PreviewComponent, UWorld* PreviewWorld);
    void RenderConstraintDebug(UPhysicsAsset* PhysicsAsset, USkeletalMeshComponent* PreviewComponent, UWorld* PreviewWorld);
    void DrawBodySetupDebug(
        UPhysicsAsset* PhysicsAsset,
        USkeletalMeshComponent* PreviewComponent,
        UWorld* PreviewWorld,
        int32 BodyIndex,
        const FPhysicsAssetBodySetup& BodySetup);
    void DrawConstraintSetupDebug(
        UPhysicsAsset* PhysicsAsset,
        USkeletalMeshComponent* PreviewComponent,
        UWorld* PreviewWorld,
        int32 ConstraintIndex,
        const FPhysicsAssetConstraintSetup& ConstraintSetup);

    void SelectBoneInPhysicsTree(UPhysicsAsset* PhysicsAsset, const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);
    void SelectBodySetup(UPhysicsAsset* PhysicsAsset, int32 BodyIndex, int32 TreeBoneIndex);
    void SelectConstraintSetup(UPhysicsAsset* PhysicsAsset, int32 ConstraintIndex, int32 TreeBoneIndex);
    int32 FindPreviewBoneIndexByName(const FName& BoneName) const;

    void AddDefaultBody(UPhysicsAsset* PhysicsAsset);
    void AddDefaultBodyForBone(UPhysicsAsset* PhysicsAsset, const FName& BoneName);
    void AddDefaultShape(FPhysicsAssetBodySetup& BodySetup);
    void AddDefaultConstraint(UPhysicsAsset* PhysicsAsset);
    void AddDefaultConstraintForBones(UPhysicsAsset* PhysicsAsset, const FName& ParentBoneName, const FName& ChildBoneName);
    void AddConstraintToSelectedParentBody(UPhysicsAsset* PhysicsAsset);
    void RunValidation(UPhysicsAsset* PhysicsAsset);
    void MarkPhysicsAssetDirty();
    void InitializeConstraintGraphEditor();
    void DestroyConstraintGraphEditor();
    void ClampSelection(UPhysicsAsset* PhysicsAsset);

private:
    int32 SelectedBodyIndex = -1;
    int32 SelectedShapeIndex = -1;
    int32 SelectedConstraintIndex = -1;
    int32 SelectedTreeBoneIndex = -1;
    USkeletalMesh* PreviewSkeletalMesh = nullptr;
    ax::NodeEditor::EditorContext* ConstraintGraphContext = nullptr;
    bool bPendingClose = false;
    bool bConstraintGraphLayoutDirty = true;
    bool bShowPreviewBodies = true;
    bool bShowPreviewConstraints = true;
    uint64 ConstraintGraphTopologyHash = 0;

    TArray<FPhysicsAssetValidationIssue> ValidationIssues;
    FString ValidationMeshPath;
    bool bValidationRan = false;
};
