#pragma once
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FMatrix.h"
#include "./SelectionSet.h"
#include "./ToolContext.h"

struct CameraState {
    FVector Position;
    float   Yaw = 0.f;
    float   Pitch = 0.f;
    float   MoveSpeed = 5.f;
    float   RotSpeed = 0.3f;

    FMatrix GetViewMatrix() const;
    FVector GetForwardVector() const;
    FVector GetRightVector() const;
};

class EditorSession {
public:
    EditorSession();

    SelectionSet Selection;
    ToolContext Tools;
    CameraState Camera;

    // 카메라 입력 처리 (WASD + 우클릭 마우스)
    // InputState를 직접 읽는다 — 폴링/이벤트 이중 경로 금지
    // ImGui가 입력을 선점한 프레임에서는 호출하지 않는다
    void ProcessCameraInput(const InputState& input, float deltaTime);

    // Projection 파라미터
    float FovY = 60.f;
    float AspectRatio;
    float NearZ = 0.1f;
    float FarZ = 1000.f;

    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjMatrix()   const;
};