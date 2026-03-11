#include "UWeek0Scene.h"

#include "SceneAutoRegister.h"
#include "UBall.h"
#include "UGameObject.h"

REGISTER_SCENE(UWeek0Scene)

void UWeek0Scene::Enter()
{
}

void UWeek0Scene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// TODO : 여기에 추가할 오브젝트들 하드 코딩으로 추가.
	for (int i = 0; i < 10; ++i)
	{
		UGameObject* instance = UBall::Create(device, context);

		FVector3 rendPosition{ // 위치의 범위는 -0.5 ~ 0.5로 설정
			(static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 1.0f - 0.5f,
			(static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 1.0f - 0.5f,
			0.f
		};

		FVector3 rendVelocity{ // 속도는 -0.5 ~ 0.5로 설정
			(static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 1.f - 0.5f, (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 1.f - 0.5f, 0.f
		};

		float rendRadius{ (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 0.1f + 0.1f };
		
		instance->SetPosition(rendPosition);
		instance->SetVelocity(rendVelocity);
		instance->SetScale(rendRadius);

		GameObjects.push_back(instance);
	}
}

void UWeek0Scene::Update(float tick)
{
	// TODO : 여기에 오브젝트들로 뭘 할지 하드 코딩으로 추가.
	for (auto& gameObject : GameObjects)
	{
		gameObject->Physics_Update(tick);
	}

	for (auto& gameObject : GameObjects)
	{
		gameObject->Update(tick);
	}
}

void UWeek0Scene::Exit()
{

}
