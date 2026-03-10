#include "UMainGameScene.h"

#include "SceneAutoRegister.h"
#include "UPikachu.h"
#include "UCircleCollider.h"
#include "URectCollider.h"

REGISTER_SCENE(UMainGameScene)

void UMainGameScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 이 씬에서 사용할 오브젝트들이나 기타 초기화 작업들을 한다고 생각하시면 된다.
	// Sample
	UPikachu* object = new UPikachu();
	{
		object->Create(device, context);

		UCircleCollider* circlecollider = new UCircleCollider();
		circlecollider->Create(device, object);

		object->SetCollider(circlecollider);
		object->SetUseGravity(true);

		GameObjects.push_back(object);

		//object->SetPosition(FVector3(0.f, 0.f, 0.f));
	}
}

void UMainGameScene::Update(float tick)
{
	for (auto& gameObject : GameObjects)
	{
		gameObject->Physics_Update(tick);
	}

	for (auto& gameObject : GameObjects)
	{
		gameObject->Update(tick);
	}

	CheckCollision();
}

void UMainGameScene::Exit()
{

}
