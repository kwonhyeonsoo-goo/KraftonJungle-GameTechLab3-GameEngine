#pragma once
#include <cstdint>
#include "../../Engine/Foundation/Core/CoreTypes.h"
#include "../../Engine/Editor/EditorSession.h"
#include "../../Engine/Editor/SelectionSet.h"
struct AppContext;
class RenderQueue;
class USceneComponent;

class OverlayBuilder {
public:
    // Gizmo, 선택 Highlight, Axis, Grid → 큐에 추가
    // ActiveTool::BuildGizmoOverlay()에 위임해 Gizmo 데이터를 수집한다
    static void Build(const EditorSession& session,
        AppContext& ctx,
        RenderQueue& queue);

    // ★ LMS 요구: "축별 색상은 UE와 똑같이 한다"
    //   0xRRGGBBAA 형식 (Alpha=FF)
    static constexpr uint32 AxisColorX         = 0xFF0000FF;   // Red   — X축
    static constexpr uint32 AxisColorY         = 0x00FF00FF;   // Green — Y축
    static constexpr uint32 AxisColorZ         = 0x0000FFFF;   // Blue  — Z축
    static constexpr uint32 AxisColorHighlight = 0xFFFF00FF;   // Yellow — 호버/드래그

private:
    static void PushWorldAxis(RenderQueue& queue);
    static void PushLocalAxis(const USceneComponent* comp, RenderQueue& queue);

    static void PushHighlight(const TArray<USceneComponent*> objs, RenderQueue& queue);
    static void PushGrid(RenderQueue& queue, FVector cameraPos);

    static void PushGizmoAxes(const GizmoState& state, RenderQueue& queue);
};