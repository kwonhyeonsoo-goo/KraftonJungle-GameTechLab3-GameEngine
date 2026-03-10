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
}

void UScene::CheckCollision()
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
						// Hit: TODO: 반발력 발생
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
