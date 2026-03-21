#include "LineBatchComponent.h"
#include "PrimitiveLineBatch.h"

IMPLEMENT_RTTI(ULineBatchComponent, UPrimitiveComponent)

void ULineBatchComponent::Initialize()
{
	Primitive = std::make_shared<CPrimitiveLineBatch>();
	LocalBoundRadius = 1.0f;
}

void ULineBatchComponent::DrawLine(FVector InStart, FVector InEnd, FVector4 InColor)
{
	static_pointer_cast<CPrimitiveLineBatch>(Primitive)->AddLine(InStart, InEnd, InColor);
	UpdateLocalBoundRadius(InStart);
	UpdateLocalBoundRadius(InEnd);
}

void ULineBatchComponent::DrawCube(FVector InCenter, FQuat InRotation, FVector InScale, FVector4 InColor)
{
	auto LineBatch = static_pointer_cast<CPrimitiveLineBatch>(Primitive);
	const FVector BaseCube[12][2] = {
		{{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}},  // 왼쪽 위
		{{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}},  // 왼쪽 아래
		{{-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}},  // 오른쪽 위
		{{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}},  // 오른쪽 아래
		{{0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}},  // 앞쪽 위
		{{0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}},  // 앞쪽 아래
		{{-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},  // 뒷쪽 위
		{{-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}},  // 뒷쪽 아래
		{{0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}},  // 앞쪽 왼
		{{0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},  // 앞쪽 오른
		{{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}},  // 뒷쪽 왼
		{{-0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}}  // 뒷쪽 오른
	};
	for (int i = 0; i < 12; i++)
	{
		FVector Start = InRotation * FVector::Multiply(BaseCube[i][0], InScale) + InCenter;
		FVector End = InRotation * FVector::Multiply(BaseCube[i][1], InScale) + InCenter;
		LineBatch->AddLine(Start, End, InColor);
	}
}
