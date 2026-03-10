#include "UMainTitleScene.h"

#include "SceneAutoRegister.h"
#include "UUIImage.h"

REGISTER_SCENE(UMainTitleScene)

void UMainTitleScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 피카츄 배구 로고
	UUIImage* obj = new UUIImage();
	obj->Create(device, context);
	obj->SetTexture(L"Resource\\Image\\messages\\ko\\pikachu_volleyball.png");
	obj->SetPosition(FVector3{ 0.3f,0.4f,0.f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);

	// 포켓몬스터 로고
	obj = new UUIImage();
	obj->Create(device, context);
	obj->SetTexture(L"Resource\\Image\\messages\\ko\\pokemon.png");
	obj->SetPosition(FVector3{ 0.3f,0.7f,0.f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);

	// 대결! 로고
	obj = new UUIImage();
	obj->Create(device, context);
	obj->SetTexture(L"Resource\\Image\\messages\\ko\\fight.png");
	obj->SetPosition(FVector3{ -0.6f,0.6f,0.f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);

	// 1P Play Button
	obj = new UUIImage();
	obj->Create(device, context);
	obj->SetTexture(L"Resource\\Image\\messages\\ko\\with_computer.png");
	obj->SetPosition(FVector3{ 0.05f,-0.2f,0.f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);

	// 2P Play Button
	obj = new UUIImage();
	obj->Create(device, context);
	obj->SetTexture(L"Resource\\Image\\messages\\ko\\with_friend.png");
	obj->SetPosition(FVector3{ 0.07f,-0.3f,0.f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);
}

void UMainTitleScene::Update(float tick)
{
	(void)tick;
}

void UMainTitleScene::Exit()
{
}
