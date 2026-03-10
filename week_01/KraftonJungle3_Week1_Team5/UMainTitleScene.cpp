#include "UMainTitleScene.h"

#include "SceneAutoRegister.h"
#include "UUI.h"

REGISTER_SCENE(UMainTitleScene)

void UMainTitleScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{

	UUI* obj = new UUI();
	obj->Create(device, context);
	obj->SetTexture(L"Resource\\Image\\messages\\ko\\pikachu_volleyball.png");
	GameObjects.push_back(obj);
}

void UMainTitleScene::Update(float tick)
{

}

void UMainTitleScene::Exit()
{

}
