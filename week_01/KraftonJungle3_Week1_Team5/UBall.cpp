#include "UBall.h"

#include "UCircleCollider.h"
#include "Utility.h"

UBall::UBall() : Collider(nullptr), Radius(1.f)
{

}

UBall::~UBall()
= default;

UBall* UBall::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UBall* instance = new UBall();
	instance->Collider = new UCircleCollider();
	instance->Collider->Create(device, instance);

	return instance;
}

void UBall::Physics_Update(const float tick)
{
	// UGameObject에서 설명한 것과 같이 속도 변화 반영(중력 적용) -> 속도 반영 -> 충돌 반영
	ApplyGravity(tick);
	ApplyVelocity(tick);
	ApplyBoundaryCollision();
}

void UBall::Update(float tick)
{

}

void UBall::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	Collider->Debug_Render(context, device);
}

void UBall::SetScale(const float scale)
{
	UGameObject::SetScale(scale);
	Radius = scale;

	if (Collider)
	{
		Collider->SetRadius(1.f);
	}
}

void UBall::ApplyBoundaryCollision()
{
	const float RadiusValue = GetScale();

	if (Position.x > 1.f - RadiusValue)
	{
		Velocity.x *= -1;
		Position.x = 1.f - RadiusValue;
	}
	if (Position.x < -1.f + RadiusValue)
	{
		Velocity.x *= -1;
		Position.x = -1.f + RadiusValue;
	}
	if (Position.y > 1.f - RadiusValue)
	{
		Velocity.y *= -1;
		Position.y = 1.f - RadiusValue;
	}
	if (Position.y < -1.f + RadiusValue)
	{
		Velocity.y *= -1;
		Position.y = -1.f + RadiusValue;
	}
}

void UBall::SetRadius(const float radius)
{
	SetScale(radius);
}

float UBall::GetRadius() const
{
	return GetScale();
}

void UBall::Release()
{
	SafeReleaseAndDelete(Collider);
}
