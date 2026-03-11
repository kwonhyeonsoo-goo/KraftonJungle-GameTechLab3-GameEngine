#include "UGameObject.h"
#include "Enum.h"

UGameObject::UGameObject() : Scale(.1f), Mass(1), UseGravity(false), Collider(nullptr), type(ObjectType::None)
{
}

void UGameObject::SetPosition(const FVector3& position)
{
	Position = position;
}

FVector3 UGameObject::GetPosition() const
{
	return Position;
}

void UGameObject::SetScale(const float scale)
{
	Scale = scale;
}

float UGameObject::GetScale() const
{
	return Scale;
}

void UGameObject::SetVelocity(const FVector3& velocity)
{
	Velocity = velocity;
}

FVector3 UGameObject::GetVelocity() const
{
	return Velocity;
}

void UGameObject::ApplyGravity(const float tick)
{
	if (UseGravity)
	{
		Velocity.y -= 4.5f * tick;
	}
}

void UGameObject::ApplyVelocity(const float tick)
{
	Position += Velocity * tick;
}
