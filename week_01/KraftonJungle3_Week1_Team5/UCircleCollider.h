#pragma once
#include <d3d11.h>

#include "UCollider.h"
class USphereMesh;
class UShader;
class URectCollider;

class UCircleCollider : public UCollider
{
public:
	UCircleCollider();
	~UCircleCollider() override;

	void Create(ID3D11Device* device, UGameObject* owner) override;
	void Release();

	ColliderType GetColliderType() const override;
	void Update_Collider() override;
	void Debug_Render(ID3D11DeviceContext* context, ID3D11Device* device) override;

	bool CheckCollisionCC(UCircleCollider* other);
	bool CheckCollisionCR(URectCollider* other);

	void SetRadius(const float radius) { Radius = radius; }
	float GetRadius() const { return Radius; }

private:
	USphereMesh* SphereMesh;
	UShader* Shader;

	float Radius;
};

