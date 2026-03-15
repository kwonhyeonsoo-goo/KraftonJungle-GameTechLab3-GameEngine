#pragma once
#include "ITool.h"
#include "../../World/Transform.h"

class TranslateTool : public ITool {
public:
    void OnMouseMove(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseDown(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseUp(const MouseEvent& e, AppContext& ctx) override;
    void BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue) override;
    FString GetName() const override { return "Translate"; }

private:
    enum class EAxis { None, X, Y, Z };
    EAxis     ActiveAxis = EAxis::None;
    bool      bDragging = false;
    FVector   DragStartPos;
    Transform OriginalTransform;

    // GizmoMath 헬퍼를 사용해 축 hit 판정
    // 1. GizmoMath::RaySegmentDistance()로 각 축 핸들과의 거리 계산
    // 2. threshold 이내면 해당 축 ActiveAxis로 기록
    // 3. 못 잡으면 PickingService::Pick()으로 오브젝트 선택 시도
};

// RotateTool — GizmoMath::RayRingHit()으로 링 판정
// ScaleTool  — GizmoMath::RaySegmentDistance() 공유 (Translate와 동일)
// 둘 다 동일한 private 상태 구조 (ActiveAxis, bDragging, DragStartPos, OriginalTransform)