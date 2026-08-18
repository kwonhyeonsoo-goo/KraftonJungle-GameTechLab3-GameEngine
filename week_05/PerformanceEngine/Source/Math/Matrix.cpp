#include "Math/Matrix.h"
#include "Math/Quat.h"
const FMatrix FMatrix::Identity(
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
);

FMatrix FMatrix::MakeRotation(const FQuat& Rotation)
{
	
		FMatrix Result;
		DirectX::XMVECTOR Q = DirectX::XMVectorSet(Rotation.X, Rotation.Y, Rotation.Z, Rotation.W);
		DirectX::XMMATRIX Mat = DirectX::XMMatrixRotationQuaternion(Q);

		DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&Result), Mat);
		return Result;
	
}
