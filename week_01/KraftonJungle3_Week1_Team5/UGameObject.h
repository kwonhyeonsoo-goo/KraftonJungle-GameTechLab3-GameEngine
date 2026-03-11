#pragma once
#include <d3d11.h>

#include "FVector3.h"
#include "UPrimitive.h"
#include "Enum.h"

class UCollider;

class UGameObject : public UPrimitive
{
public:
	UGameObject();
	~UGameObject() override = default;

	// Physics Update 내부 구현은 중력 반영 -> 속도 반영 -> 충돌 반영 순서로 하면 좋음.
	virtual void Physics_Update(float tick) = 0;
	virtual void Update(float tick) = 0; // tick = delta time 이라고 보면 됨.
	virtual void Render(ID3D11DeviceContext* context, ID3D11Device* device) = 0;
	virtual const char* GetEditorTypeName() const { return "UGameObject"; }

	void SetPosition(const FVector3& position);
	FVector3 GetPosition() const;

	virtual void SetScale(float scale);
	float GetScale() const;

	void SetVelocity(const FVector3& velocity);
	FVector3 GetVelocity() const;

	virtual void Release() = 0;
	
	bool GetUseGravity() const { return UseGravity; }
	void SetUseGravity(bool useGravity) { UseGravity = useGravity; }

	UCollider* GetCollider() const { return Collider; }
	void SetCollider(UCollider* collider) { Collider = collider; }

	ObjectType GetObjectType() const { return type; }
	void SetObjectType(ObjectType inType) { type = inType; }

protected:
	// 이 아래에 있는 함수들은 나중에 물리용 클래스로 따로 빼야함.
	// GameObject라고 해서 다 물리 처리를 하는 것이 아니기 때문.
	void ApplyGravity(float tick);
	void ApplyVelocity(float tick);

protected:
	// Transform
	FVector3	Position;
	float		Scale;

	// Rigidbody
	FVector3	Velocity;
	float		Mass;
	bool		UseGravity;

	// Collision
	UCollider* Collider;
	ObjectType type;
};

