#include "AxisRenderer.h"
#include "Renderer/Renderer.h"

void FAxisRenderer::RenderWorldAxis(FRenderer* Renderer, float Length)
{
    if (Renderer == nullptr)
    {
        return;
    }
#if IS_OBJ_VIEWER //뷰어에서 월드축 렌더링x
#else
    Renderer->DrawLine({ 0.0f, 0.0f, 0.0f }, { Length, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
    Renderer->DrawLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, Length, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
    Renderer->DrawLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, Length }, { 0.0f, 0.0f, 1.0f, 1.0f });
#endif
}

//오컴의 면도날, 안 쓰는 코드 
//void FAxisRenderer::RenderLocalAxis(FRenderer* Renderer, const FVector& Position, const FMatrix& RotationMatrix, float Length)
//{
//    if (Renderer == nullptr)
//    {
//        return;
//    }
//
//    const FVector XAxis = RotationMatrix.GetUnitAxis(EAxis::X) * Length;
//    const FVector YAxis = RotationMatrix.GetUnitAxis(EAxis::Y) * Length;
//    const FVector ZAxis = RotationMatrix.GetUnitAxis(EAxis::Z) * Length;
//
//    Renderer->DrawLine(Position, Position + XAxis, { 1.0f, 0.0f, 0.0f, 1.0f });
//    Renderer->DrawLine(Position, Position + YAxis, { 0.0f, 1.0f, 0.0f, 1.0f });
//    Renderer->DrawLine(Position, Position + ZAxis, { 0.0f, 0.0f, 1.0f, 1.0f });
//}
