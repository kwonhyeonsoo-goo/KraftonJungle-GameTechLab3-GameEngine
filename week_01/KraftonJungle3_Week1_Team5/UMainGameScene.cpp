#include "UMainGameScene.h"

#include "SceneAutoRegister.h"
#include "UPikachu.h"
#include "UCircleCollider.h"
#include "URectCollider.h"
#include "UBall.h"
#include "UNet.h"
#include "UUIImage.h"
#include "UUIScore.h"

REGISTER_SCENE(UMainGameScene)

void UMainGameScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 이 씬에서 사용할 오브젝트들이나 기타 초기화 작업들을 한다고 생각하시면 된다.
	InitializeUI(device, context);
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

	UBall* instance = UBall::Create(device, context);
	{
		// 속도는 -0.5 ~ 0.5로 설정
		FVector3 rendVelocity{ 0.f, -1.f, 0.f};

		float rendRadius{0.2f};

		instance->SetVelocity(rendVelocity);
		instance->SetScale(.1f);
		instance->SetRadius(0.1f); //radius 지정이 빠짐

		UCircleCollider* circlecollider = new UCircleCollider();
		circlecollider->Create(device, instance);
		instance->SetCollider(circlecollider);
		instance->SetUseGravity(true);

		GameObjects.push_back(instance);
		
	}
	Net = new UNet();
	Net->Create(device, context);
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

	for (auto& obj : GameObjects)
	{
		if (obj->GetObjectType() == ObjectType::Ball)
		{
			Net->HandleBallCollision(static_cast<UBall*>(obj));
		}
	}
}

void UMainGameScene::Exit()
{

}
// 피카츄 배구 게임에 오브젝트는 3개 (피카츄, 
void UMainGameScene::CheckCollision()
{
	for (int i = 0; i < GameObjects.size(); ++i)
	{
		for (int j = i + 1; j < GameObjects.size(); ++j)
		{
			if (GameObjects[i]->GetCollider() && GameObjects[j]->GetCollider())
			{
				ColliderType type1 = GameObjects[i]->GetCollider()->GetColliderType();
				ColliderType type2 = GameObjects[j]->GetCollider()->GetColliderType();
				if (type1 == ColliderType::ColliderType_Circle)
				{
					// 당장은 쓸 일 없음
					if (type1 == type2) //circle circle
					{
						// 구끼리 충돌 체크
						UCircleCollider* CircleCollider1 = static_cast<UCircleCollider*>(GameObjects[i]->GetCollider());
						UCircleCollider* CircleCollider2 = static_cast<UCircleCollider*>(GameObjects[j]->GetCollider());
						bool bResult = CircleCollider1->CheckCollisionCC(CircleCollider2);

						if (bResult)
						{
							if (GameObjects[i]->GetObjectType() == ObjectType::Pikachu)
							{
								UPikachu* Pikachu = static_cast<UPikachu*>(GameObjects[i]);
								UBall* Ball = static_cast<UBall*>(GameObjects[j]);
								Pikachu->HandleCollision(Ball);
							}
							else
							{
								UPikachu* pikachu = static_cast<UPikachu*>(GameObjects[j]);
								UBall* Ball = static_cast<UBall*>(GameObjects[i]);
								pikachu->HandleCollision(Ball);
							}
						}
					}
					else //circle - rect
					{
						// 원 - 박스 충돌 체크
						UCircleCollider* CircleCollider = static_cast<UCircleCollider*>(GameObjects[i]->GetCollider());
						URectCollider* RectCollider = static_cast<URectCollider*>(GameObjects[j]->GetCollider());
						bool bResult = CircleCollider->CheckCollisionCR(RectCollider);
						if (bResult)
						{
							// Hit: TODO: 반발력 발생
						}
					}
				}
				else //ColliderType_Rect
				{
					if (type1 == type2)
					{
						URectCollider* RectCollider1 = static_cast<URectCollider*>(GameObjects[i]->GetCollider());
						URectCollider* RectCollilder2 = static_cast<URectCollider*>(GameObjects[j]->GetCollider());
						bool bResult = RectCollider1->CheckCollisionRR(RectCollilder2);
						if (bResult)
						{
							// Hit: TODO: 반발력 발생
						}
					}
					else //rect circle
					{
						// 원과 박스의 충돌 체크
						URectCollider* RectCollider = static_cast<URectCollider*>(GameObjects[i]->GetCollider());
						UCircleCollider* CircleCollider = static_cast<UCircleCollider*>(GameObjects[j]->GetCollider());
						bool bResult = CircleCollider->CheckCollisionCR(RectCollider);
						if (bResult)
						{
							// Hit: TODO: 반발력 발생
						}
					}
				}
			}
		}
	}
}

void UMainGameScene::InitializeUI(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UUIImage* backGround = new UUIImage();
	backGround->Create(device, context);
	if (!backGround->SetTexture(L"Resource\\Image\\Pikachu_BG.png"))
	{
		return;
	}
	GameObjects.push_back(backGround);

	UUIScore* score_1p = new UUIScore();
	score_1p->Create(device, context);
	score_1p->SetPosition({ -0.7f, 0.75f, 0.f });
	GameObjects.push_back(score_1p);

	UUIScore* score_2p = new UUIScore();
	score_2p->Create(device, context);
	score_2p->SetPosition({ 0.7f, 0.75f, 0.f });
	GameObjects.push_back(score_2p);

	Clouds.reserve(CloudCount);

	for (int i = 0; i < CloudCount; ++i)
	{
			
	}

}

void UMainGameScene::UpdateCloudImageAnimation(float tick)
{
	
}
