#pragma once
#include "ITool.h"
#include <cmath>
#include <limits>
#include "../../../AppContext.h"
#include "../../World/USceneComponent.h"
#include "../../Editor/Commands/SetTransformCommand.h"
#include "GizmoMath.h"

class ScaleTool : public ITool {
public:
    bool TryBeginManipulation(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseMove(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseUp(const MouseEvent& e, AppContext& ctx) override;
    void FillGizmoState(AppContext& ctx, GizmoState& out) const override;
    FString GetName() const override { return "Scale"; }

private:
    bool TryPickAxisHandle(const MouseEvent& e, AppContext& ctx, EAxis& outAxis) const;
    void ApplyScaleDelta(const TArray<USceneComponent*>& primaries, float delta) const;
    void ResetManipulationState();

private:
    bool bActiveUniformScale = false;
    FVector DragAxisOrigin{};
    FVector DragAxisDirection{};
    float     DragStartAxisT = 0.0f;
    TArray<Transform> OriginalTransforms;
};