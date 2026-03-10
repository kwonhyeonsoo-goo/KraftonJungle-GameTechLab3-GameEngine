#pragma once
#include <d3d11.h>

#include "Enum.h"
#include "UPrimitive.h"
class UGameObject;

class UCollider : public UPrimitive
{
public:
	UCollider() = default;
	~UCollider() override = default;

	virtual void Create(ID3D11Device* device, UGameObject* owner) = 0;

	virtual ColliderType GetColliderType() const = 0;

	virtual void Update_Collider() = 0;
	virtual void Debug_Render(ID3D11DeviceContext* context, ID3D11Device* device) = 0;

	bool IsTrigger() const { return Trigger; }
	void SetTrigger(bool isTrigger) { Trigger = isTrigger; }

	UGameObject* GetOwner() const { return Owner; }

protected:
	// Trigger가 True 일 때 충돌 시 물리 연산을 하지 않고 충돌 체크만 한다.
	bool Trigger;

	UGameObject* Owner;
};

