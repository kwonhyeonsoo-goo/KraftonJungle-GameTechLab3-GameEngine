#include "UMainGameScene.h"

#include "SceneAutoRegister.h"
#include "UTestObject.h"

REGISTER_SCENE(UMainGameScene)

void UMainGameScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 이 씬에서 사용할 오브젝트들이나 기타 초기화 작업들을 한다고 생각하시면 된다.
	UTestObject* object = new UTestObject();
	object->Create(device, context);
	GameObjects.push_back(object);
}

void UMainGameScene::Update(float tick)
{
	for (auto& gameObject : GameObjects)
	{
		gameObject->Physics_Update(tick);
	}

	
}

void UMainGameScene::Exit()
{

}
