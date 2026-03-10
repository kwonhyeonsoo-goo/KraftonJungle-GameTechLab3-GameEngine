#pragma once
#include "UCollider.h"

class URectMesh;
class UShader;
class URectCollider : public UCollider
{
public:
	URectCollider();
	~URectCollider() override;

	void Create(ID3D11Device* device, UGameObject* owner) override;
	void CreateRect(ID3D11Device* device);
	void Release();

	ColliderType GetColliderType() const override; 
	void Update_Collider() override;
	void Debug_Render(ID3D11DeviceContext* context, ID3D11Device* device) override;

	float GetWidth() const { return Width; }
	void SetWidth(const float width) { Width = width; }

	float GetHeight() const { return Height; }
	void SetHeight(const float height) { Height = height; }

	bool CheckCollisionRR(URectCollider* other);

	
private:
	URectMesh* RectMesh;
	UShader* Shader;

	float Width = 1.f;
	float Height = 1.f;
};

