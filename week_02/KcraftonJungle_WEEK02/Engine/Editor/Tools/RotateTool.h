#pragma once
#include "ITool.h"
#include <cmath>
#include "../../../AppContext.h"
#include "../../World/USceneComponent.h"
#include "../../Editor/Commands/SetTransformCommand.h"
#include "../../Rendering/RenderQueue.h"
#include "../../Rendering/RenderTypes.h"
#include "GizmoMath.h"

namespace
{
    struct Quatanian
    {
        float x, y, z, w;

        Quatanian(const FVector& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w)
        {
        }
        Quatanian(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w)
        {
        }
        Quatanian(const FVector4& v) : x(v.x), y(v.y), z(v.z), w(v.w)
        {
        }

        FVector4 Mul(const Quatanian& Q) const
        {
            return FVector4(
                w * Q.x + x * Q.w + y * Q.z - z * Q.y,
                w * Q.y - x * Q.z + y * Q.w + z * Q.x,
                w * Q.z + x * Q.y - y * Q.x + z * Q.w,
                w * Q.w - x * Q.x - y * Q.y - z * Q.z
            );
        }
    };

    static FVector4 NormalizeQuat(const FVector4& q)
    {
        const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len <= 1e-6f)
            return FVector4(0, 0, 0, 1);

        return FVector4(q.x / len, q.y / len, q.z / len, q.w / len);
    }

    static Quatanian MakeAxisAngleQuat(const FVector& axis, float angleRad)
    {
        const FVector n = axis.Normalized();
        const float half = angleRad * 0.5f;
        const float s = std::sin(half);
        const float c = std::cos(half);

        return Quatanian(n.x * s, n.y * s, n.z * s, c);
    }
}

class RotateTool : public ITool {
public:
    bool TryBeginManipulation(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseMove(const MouseEvent& e, AppContext& ctx) override;
    void OnMouseUp(const MouseEvent& e, AppContext& ctx) override;
    void BuildGizmoOverlay(AppContext& ctx, RenderQueue& queue) override;
    FString GetName() const override { return "Rotate"; }

private:
    enum class EAxis { None, X, Y, Z };

    static int AxisToIndex(EAxis axis);
    static EAxis IndexToAxis(int index);

private:
    EAxis     ActiveAxis = EAxis::None;
    bool      bDragging = false;
    FVector   DragStartDir = FVector::Zero;
    Transform OriginalTransform;
};