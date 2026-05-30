#pragma once

#include "Editor/UI/Asset/AssetEditorWidget.h"
#include "Physics/PhysicsAssetValidation.h"

class UPhysicsAsset;
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
    FString GetDocumentTitle() const override;
    FString GetDocumentPayloadId() const override;
    EEditorDocumentTabKind GetDocumentTabKind() const override { return EEditorDocumentTabKind::PhysicsAssetEditor; }

private:
    UPhysicsAsset* GetEditedPhysicsAsset() const;

    void RenderToolbar(UPhysicsAsset* PhysicsAsset);
    void RenderAssetSummary(UPhysicsAsset* PhysicsAsset);
    void RenderBodyList(UPhysicsAsset* PhysicsAsset);
    void RenderConstraintList(UPhysicsAsset* PhysicsAsset);
    void RenderDetailsPanel(UPhysicsAsset* PhysicsAsset);
    void RenderBodyDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetBodySetup& BodySetup);
    void RenderShapeDetails(FPhysicsAssetShapeSetup& ShapeSetup);
    void RenderConstraintDetails(UPhysicsAsset* PhysicsAsset, FPhysicsAssetConstraintSetup& ConstraintSetup);
    void RenderValidationPanel();

    void AddDefaultBody(UPhysicsAsset* PhysicsAsset);
    void AddDefaultShape(FPhysicsAssetBodySetup& BodySetup);
    void AddDefaultConstraint(UPhysicsAsset* PhysicsAsset);
    void RunValidation(UPhysicsAsset* PhysicsAsset);
    void MarkPhysicsAssetDirty();
    void ClampSelection(UPhysicsAsset* PhysicsAsset);

private:
    int32 SelectedBodyIndex = -1;
    int32 SelectedShapeIndex = -1;
    int32 SelectedConstraintIndex = -1;
    bool bPendingClose = false;

    TArray<FPhysicsAssetValidationIssue> ValidationIssues;
    FString ValidationMeshPath;
    bool bValidationRan = false;
};
