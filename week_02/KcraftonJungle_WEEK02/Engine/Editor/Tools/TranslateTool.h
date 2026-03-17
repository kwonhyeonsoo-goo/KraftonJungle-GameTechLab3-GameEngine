#pragma once
#include "ITool.h"
#include <cmath>
#include "../../../AppContext.h"
#include "../../World/USceneComponent.h"
#include "../../Editor/Commands/SetTransformCommand.h"
#include "GizmoMath.h"

class TranslateTool : public ITool {
public:
    bool TryBeginManipulation(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseMove(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseUp(const MouseEvent& e, AppContext& ctx) override;
    FString GetName() const override { return "Translate"; }

protected:
    void FillAxisData(const FVector& origin, const FVector& axisDir,
                      float scale, int axisIndex, GizmoAxisData& out) const override;

private:
    float     DragStartAxisT = 0.0f;
    Transform OriginalTransform;
};