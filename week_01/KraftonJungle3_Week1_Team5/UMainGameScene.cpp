#include "UMainGameScene.h"

#include "SceneAutoRegister.h"
#include "UPikachu.h"
#include "UCircleCollider.h"
#include "URectCollider.h"
#include "FVector3.h"

REGISTER_SCENE(UMainGameScene)

void UMainGameScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 이 씬에서 사용할 오브젝트들이나 기타 초기화 작업들을 한다고 생각하시면 된다.
	// Player1
	UPikachu* Player1 = new UPikachu();
	Player1->Create(device, context);

	Player1->SetKeyConfig({ 'W', 'S', 'A', 'D', VK_SPACE });
	Player1->SetBoundary(-1.0f, -0.02f, 1.0f, -1.0f);
	Player1->SetPosition(FVector3(-0.5f, 0.0f, 0.0f));

	UCircleCollider* Collider1 = new UCircleCollider();
	Collider1->Create(device, Player1);
	Player1->SetCollider(Collider1);

	GameObjects.push_back(Player1);


	// Player2
	UPikachu* Player2 = new UPikachu();
	Player2->Create(device, context);

	Player2->SetKeyConfig({ VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN });
	Player2->SetBoundary(0.02f, 1.0f, 1.0f, -1.0f);
	Player2->SetPosition(FVector3(0.5f, 0.0f, 0.0f));

	UCircleCollider* Collider2 = new UCircleCollider();
	Collider2->Create(device, Player2);
	Player2->SetCollider(Collider2);

	GameObjects.push_back(Player2);
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
