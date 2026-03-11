#include "UTestScene.h"

#include "SceneAutoRegister.h"
#include "UPikachu.h"

REGISTER_SCENE(UTestScene)

void UTestScene::Enter()
{
}

void UTestScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UPikachu* obj = new UPikachu();
	obj->Create(device, context);
	GameObjects.push_back(obj);
}

void UTestScene::Update(float tick)
{
	for (auto& gameObject : GameObjects)
	{
		gameObject->Update(tick);
	}
}

void UTestScene::Exit()
{

}
