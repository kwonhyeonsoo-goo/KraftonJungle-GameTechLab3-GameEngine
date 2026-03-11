#include "UCircleCollider.h"

#include "UCircle2DMesh.h"
#include "UShader.h"
#include "UGameObject.h"
#include "USphereMesh.h"
#include "Utility.h"
#include "URectCollider.h"
#include <algorithm>

UCircleCollider::UCircleCollider() : UCollider(), SphereMesh(nullptr), Shader(nullptr), Radius(1.f)
{
}

UCircleCollider::~UCircleCollider()
{
	Release();
}

void UCircleCollider::Create(ID3D11Device* device, UGameObject* owner)
{
	if (owner)
	{
		Owner = owner;
	}

	if (device)
	{
		SphereMesh = new USphereMesh();
		SphereMesh->CreateSphere(device);

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "Color", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
				D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		Shader = new UShader();
		Shader->Create(device, L"ShaderW0.hlsl", layout, ARRAYSIZE(layout), "mainVS", "mainPS");
	}
}

void UCircleCollider::Release()
{
	SafeReleaseAndDelete(SphereMesh);
	SafeReleaseAndDelete(Shader);
}

ColliderType UCircleCollider::GetColliderType() const
{
	return ColliderType::ColliderType_Circle;
}

void UCircleCollider::Update_Collider()
{
	
}

void UCircleCollider::Debug_Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	if (Shader)
	{
		SphereMesh->Bind(context);
		Shader->Bind(context);
		Shader->UpdateConstant(context, Owner->GetPosition(), Owner->GetScale() * Radius);
		SphereMesh->Draw(context);
	}
}

bool UCircleCollider::CheckCollisionCC(UCircleCollider* other)
{
	if (!Owner || !other->Owner) return false;

	FVector3 Position1 = Owner->GetPosition();
	FVector3 Position2 = other->Owner->GetPosition();

	float r1 = Radius * Owner->GetScale();
	float r2 = other->Radius * other->Owner->GetScale();

	float DistanceSquare = (Position1 - Position2).LengthSquare();
	return DistanceSquare <= (r1 + r2) * (r1 + r2);
}

bool UCircleCollider::CheckCollisionCR(URectCollider* other)
{
	if (!Owner || !other->GetOwner())
	{
		return false;
	}

	UGameObject* OtherOwner = other->GetOwner();

	FVector3 OwnerPos = Owner->GetPosition();
	FVector3 OtherPos = OtherOwner->GetPosition();

	float halfW = (other->GetWidth() / 2.f) * OtherOwner->GetScale();
	float halfH = (other->GetHeight() / 2.f) * OtherOwner->GetScale();

	float closestX = std::clamp(OwnerPos.x, OtherPos.x - halfW, OtherPos.x + halfW);
	float closestY = std::clamp(OwnerPos.y, OtherPos.y - halfH, OtherPos.y + halfH);

	float dist = (OwnerPos - FVector3{ closestX, closestY, 0.f }).LengthSquare();
	float r = Radius * Owner->GetScale();
	return dist < r * r;
}
