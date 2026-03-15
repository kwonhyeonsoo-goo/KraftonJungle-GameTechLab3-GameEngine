#pragma once

#include "../Foundation/Math/FMatrix.h"
#include "../../Engine/Foundation/Core/CoreTypes.h"
//RenderType
enum class ERenderType {

    Primitive,    // 일반 오브젝트
    // ★ LMS 요구: "Vertex Shader를 사용해 선택된 오브젝트를 강조"
    //   Highlight는 별도 VS 상수(color tint 또는 scale bias)로 처리한다.
    //   D3D11Renderer::Flush()에서 ERenderType::Highlight 명령을 받으면
    //   Highlight 전용 셰이더 상태(또는 cbuffer 플래그)로 전환해 렌더한다.
    Highlight,    // 선택된 오브젝트 — Vertex Shader 강조 경로
    Gizmo,        // 이동/회전/스케일 핸들
    WorldAxis,    // 월드 좌표계 축
    LocalAxis,    // 선택 오브젝트 로컬 축
    Grid,         // 바닥 그리드
    GizmoLine,
};

enum class EPrimitiveShape;   // World에서 정의됨

/*
enum class EPrimitiveShape
{
    Cube,
    Sphere,
    Plane,
};
*/
//From Object
// ★ 포인터 금지 — 추출된 값만 보관
struct RenderCommand {
    ERenderType     Type;
    EPrimitiveShape Shape;          // Primitive/Highlight만 유효
    FMatrix        WorldTransform;
    uint32_t          Color;          // 0xRRGGBBAA
    uint32_t         ObjectId;       // Picking 역참조, 디버그용
    FVector LineStart;
    FVector LineEnd;
};