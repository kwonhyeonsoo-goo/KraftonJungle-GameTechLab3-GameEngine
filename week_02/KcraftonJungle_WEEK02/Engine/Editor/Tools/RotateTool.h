#pragma once
#include "ITool.h"
#include "../../../AppContext.h"
#include "../../World/USceneComponent.h"
#include "../../Editor/Commands/SetTransformCommand.h"
#include "../../Foundation/Math/FQuat.h"
#include "GizmoMath.h"

class RotateTool : public ITool {
public:
    bool TryBeginManipulation(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseMove(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseUp(const MouseEvent& e, AppContext& ctx) override;
    FString GetName() const override { return "Rotate"; }

protected:
    void FillAxisData(const FVector& origin, const FVector& axisDir,
                      float scale, int axisIndex, GizmoAxisData& out) const override;

private:
    FVector DragStartDir = FVector::Zero;
    TArray<Transform> OriginalTransforms;
};
