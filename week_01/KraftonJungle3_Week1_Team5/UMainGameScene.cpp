#include "UMainGameScene.h"

#include "SceneAutoRegister.h"
#include "UPikachu.h"
#include "UCircleCollider.h"
#include "URectCollider.h"
#include "UBall.h"

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

		object->SetPosition(FVector3(0.f, -0.9f, 0.f));
	}

	UGameObject* instance = UBall::Create(device, context);
	{
		FVector3 rendVelocity{ // 속도는 -0.5 ~ 0.5로 설정
			(static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 1.f - 0.5f, (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 1.f - 0.5f, 0.f
		};

		float rendRadius{ (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 0.1f + 0.1f };

		instance->SetVelocity(rendVelocity);
		instance->SetScale(rendRadius);
		GameObjects.push_back(instance);

		instance->SetUseGravity(true);
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
// 피카츄 배구 게임에 오브젝트는 3개 (피카츄, 
void UMainGameScene::CheckCollision()
{
	for (int i = 0; i < GameObjects.size(); ++i)
	{
		for (int j = i + 1; j < GameObjects.size(); ++j)
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