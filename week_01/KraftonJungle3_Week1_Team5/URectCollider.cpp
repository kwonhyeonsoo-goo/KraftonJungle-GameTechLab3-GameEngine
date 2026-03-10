#include "URectCollider.h"

#include "URectMesh.h"
#include "UShader.h"
#include "UGameObject.h"
#include "Utility.h"

URectCollider::URectCollider()
	: UCollider(), RectMesh(nullptr), Shader(nullptr)
{

}

URectCollider::~URectCollider()
{
	Release();
}

void URectCollider::Create(ID3D11Device* device, UGameObject* owner)
{
	if (owner)
	{
		Owner = owner;
	}

	if (device)
	{
		RectMesh = new URectMesh();
		RectMesh->CreateRect(device);

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

void URectCollider::Release()
{
	SafeReleaseAndDelete(RectMesh);
	SafeReleaseAndDelete(Shader);
}

ColliderType URectCollider::GetColliderType() const
{
	return ColliderType::ColliderType_Rect;
}

void URectCollider::Update_Collider()
{
	// TODO
}

void URectCollider::Debug_Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	if (Shader)
	{
		Shader->Bind(context);
		Shader->UpdateConstant(context, Owner->GetPosition(), Owner->GetScale());
		
		RectMesh->Draw(context);
	}
}
// Rect-Rect 충돌을 처리하는 함수
bool URectCollider::CheckCollisionRR(URectCollider* other)
{
	if (!Owner || !other->Owner)
	{
		return false;
	}
	
	FVector3 Position1 = Owner->GetPosition();
	FVector3 Position2 = other->Owner->GetPosition();
	
	float HalfWidth1 = (Width / 2.f) * Owner->GetScale();
	float HalfHeight1 = (Height / 2.f) * Owner->GetScale();

	float HalfWidth2 = (other->Width / 2.f) * other->Owner->GetScale();
	float HalfHeight2 = (other->Height / 2.f) * other->Owner->GetScale();

	bool xCollider = ((Position1.x + HalfWidth1 >= Position2.x - HalfWidth2)
						&& (Position2.x + HalfWidth2 >= Position1.x - HalfWidth1));
	bool yCollider = ((Position1.y + HalfHeight1 >= Position2.y - HalfHeight2) 
						&& (Position2.y + HalfHeight2 >= Position1.y - HalfHeight1));
		
	return (xCollider && yCollider);
}
