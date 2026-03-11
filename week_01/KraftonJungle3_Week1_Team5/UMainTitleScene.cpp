#include "UMainTitleScene.h"

#include <cmath>
#include <Windows.h>

#include "SceneAutoRegister.h"
#include "UEngine.h"
#include "UUIButton.h"
#include "UUIImage.h"

REGISTER_SCENE(UMainTitleScene)

void UMainTitleScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 피카츄 배구 이미지
	UUIImage* obj = new UUIImage();
	obj->Create(device, context);
	if (!obj->SetTexture(L"Resource\\Image\\messages\\ko\\pikachu_volleyball.png"))
	{
		return;
	}
	obj->SetPosition(FVector3{ 0.3f, 0.4f, 0.0f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);

	// 포켓몬 로고 이미지
	obj = new UUIImage();
	obj->Create(device, context);
	if (!obj->SetTexture(L"Resource\\Image\\messages\\ko\\pokemon.png"))
	{
		return;
	}
	obj->SetPosition(FVector3{ 0.3f, 0.7f, 0.0f });
	obj->SetScale(2.0f);
	GameObjects.push_back(obj);

	// 싸우자 이미지 커졌다 작아졌다 함.
	FightImage = new UUIImage();
	FightImage->Create(device, context);
	if (!FightImage->SetTexture(L"Resource\\Image\\messages\\ko\\fight.png"))
	{
		return;
	}
	FightImage->SetPosition(FVector3{ -0.6f, 0.6f, 0.0f });
	FightImage->SetScale(FightBaseScale);
	GameObjects.push_back(FightImage);

	MenuButtonPositions[0] = FVector3{ 0.0f, -0.20f, 0.0f };
	MenuButtonPositions[1] = FVector3{ 0.0f, -0.38f, 0.0f };

	// 버튼 이미지들
	MenuButtons[0] = new UUIButton();
	MenuButtons[0]->Create(device, context);
	if (!MenuButtons[0]->SetTexture(L"Resource\\Image\\messages\\ko\\with_computer.png"))
	{
		return;
	}
	MenuButtons[0]->SetPosition(MenuButtonPositions[0]);
	MenuButtons[0]->SetScale(2.0f);
	GameObjects.push_back(MenuButtons[0]);

	MenuButtons[1] = new UUIButton();
	MenuButtons[1]->Create(device, context);
	if (!MenuButtons[1]->SetTexture(L"Resource\\Image\\messages\\ko\\with_friend.png"))
	{
		return;
	}
	MenuButtons[1]->SetPosition(MenuButtonPositions[1]);
	MenuButtons[1]->SetScale(2.0f);
	GameObjects.push_back(MenuButtons[1]);

	SelectedMenuIndex = 0;
	UpdateMenuVisuals();
}

void UMainTitleScene::Update(float tick)
{
	UpdateFightImageAnimation(tick);

	if (IsKeyJustPressed(VK_UP, bWasUpPressed))
	{
		SelectedMenuIndex = (SelectedMenuIndex + static_cast<int>(MenuButtons.size()) - 1) % static_cast<int>(MenuButtons.size());
		UpdateMenuVisuals();
	}

	if (IsKeyJustPressed(VK_DOWN, bWasDownPressed))
	{
		SelectedMenuIndex = (SelectedMenuIndex + 1) % static_cast<int>(MenuButtons.size());
		UpdateMenuVisuals();
	}

	if (IsKeyJustPressed(VK_RETURN, bWasEnterPressed))
	{
		ExecuteSelectedMenu();
	}
}

void UMainTitleScene::Exit()
{
}

void UMainTitleScene::UpdateFightImageAnimation(float tick)
{
	if (FightImage == nullptr)
	{
		return;
	}

	FightAnimationTime += tick;

	const float Animation = std::sin(FightAnimationTime * FightPulseSpeed);
	const float ScaleOffset = (Animation * 0.5f + 0.5f) * FightScaleAmplitude;
	FightImage->SetScale(FightBaseScale + ScaleOffset);
}

void UMainTitleScene::UpdateMenuVisuals() const
{
	for (int Index = 0; Index < static_cast<int>(MenuButtons.size()); ++Index)
	{
		if (MenuButtons[Index] == nullptr)
		{
			continue;
		}

		MenuButtons[Index]->SetSelected(Index == SelectedMenuIndex);
	}

	if (SelectionMark != nullptr)
	{
		const FVector3 ButtonPosition = MenuButtonPositions[SelectedMenuIndex];
		SelectionMark->SetPosition(FVector3{ ButtonPosition.x - 0.42f, ButtonPosition.y + 0.01f, ButtonPosition.z });
	}
}

bool UMainTitleScene::IsKeyJustPressed(int virtualKey, bool& bWasPressed) const
{
	const bool bIsPressed = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
	const bool bJustPressed = bIsPressed && !bWasPressed;
	bWasPressed = bIsPressed;
	return bJustPressed;
}

void UMainTitleScene::ExecuteSelectedMenu() const
{
	if (SelectedMenuIndex == 0)
	{
		// 1P 플레이 씬으로 전환
		//UEngine::GetInstance().GetSceneManager().RequestChangeScene("UMainGameScene");
		return;
	}

	if (SelectedMenuIndex == 1)
	{
		UEngine::GetInstance().GetSceneManager().RequestChangeScene("UMainGameScene");
		return;
	}
}
