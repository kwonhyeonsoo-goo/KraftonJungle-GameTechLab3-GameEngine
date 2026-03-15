#pragma once
#include "ITool.h"
#include <cmath>
#include "../../../AppContext.h"
#include "../../World/USceneComponent.h"
#include "../../Editor/Commands/SetTransformCommand.h"
#include "../../Rendering/RenderQueue.h"
#include "../../Rendering/RenderTypes.h"
#include "GizmoMath.h"

class TranslateTool : public ITool {
public:
    bool TryBeginManipulation(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseMove(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseUp(const MouseEvent& e, AppContext& ctx) override;
    void BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue) override;
    FString GetName() const override { return "Translate"; }

private:
    enum class EAxis { None, X, Y, Z };

    static int AxisToIndex(EAxis axis);
    static EAxis IndexToAxis(int index);

private:
    EAxis     ActiveAxis = EAxis::None;
    bool      bDragging = false;
    float     DragStartAxisT = 0.0f;
    Transform OriginalTransform;
};