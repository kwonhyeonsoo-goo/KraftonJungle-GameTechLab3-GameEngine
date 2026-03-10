#include "UTestScene.h"

#include "SceneAutoRegister.h"
#include "UTestObject_2.h"

REGISTER_SCENE(UTestScene)

void UTestScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UTestObject_2* obj = new UTestObject_2();
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
