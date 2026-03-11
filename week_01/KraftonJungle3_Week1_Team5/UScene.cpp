#include "UScene.h"

#include "Utility.h"
#include "UGameObject.h"
#include "UCollider.h"
#include "UCircleCollider.h"
#include "URectCollider.h"

UScene::~UScene()
{
	Release();
}

void UScene::Render(ID3D11Device* device, ID3D11DeviceContext* context)
{
	for (auto& gameObject : GameObjects)
	{
		gameObject->Render(context, device);
	}
}

void UScene::Release()
{
	for (auto& gameObject : GameObjects)
	{
		SafeReleaseAndDelete(gameObject);
	}

	GameObjects.clear();
}
