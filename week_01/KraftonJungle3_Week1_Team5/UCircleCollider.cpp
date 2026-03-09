#include "UCircleCollider.h"

#include "UCircle2DMesh.h"
#include "UShader.h"
#include "UGameObject.h"
#include "USphereMesh.h"
#include "Utility.h"

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
		Shader->Bind(context);
		Shader->UpdateConstant(context, Owner->GetPosition(), Owner->GetScale() * Radius);

		SphereMesh->Draw(context);
	}
}
