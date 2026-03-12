#include "UMainTitleScene.h"

#include <algorithm>
#include <cmath>
#include <Windows.h>

#include "SceneAutoRegister.h"
#include "UEngine.h"
#include "UGameManager.h"
#include "UUIButton.h"
#include "UUIImage.h"
#include "UTexture2D.h"

REGISTER_SCENE(UMainTitleScene)

void UMainTitleScene::Enter()
{

}

void UMainTitleScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (!InitializeBackgroundTiles(device, context))
	{
		return;
	}

	UUIImage* obj = new UUIImage();
	obj->Create(device, context);
	if (!obj->SetTexture(L"Resource\\Image\\messages\\ko\\pikachu_volleyball.png"))
	{
		return;
	}
	obj->SetPosition(FVector3{ 0.3f, 0.4f, 0.0f });
	GameObjects.push_back(obj);

	obj = new UUIImage();
	obj->Create(device, context);
	if (!obj->SetTexture(L"Resource\\Image\\messages\\ko\\pokemon.png"))
	{
		return;
	}
	obj->SetPosition(FVector3{ 0.3f, 0.7f, 0.0f });
	GameObjects.push_back(obj);

	FightImage = new UUIImage();
	FightImage->Create(device, context);
	if (!FightImage->SetTexture(L"Resource\\Image\\messages\\ko\\fight.png"))
	{
		return;
	}
	FightImage->SetPosition(FVector3{ -0.6f, 0.6f, 0.0f });
	FightImage->SetScale(FightBaseScale);
	GameObjects.push_back(FightImage);

	MenuButtonPositions[0] = FVector3{ 0.16f, 0.f, 0.0f };
	MenuButtonPositions[1] = FVector3{ 0.16f, -0.27f, 0.0f };
	MenuButtonPositions[2] = FVector3{ 0.08f, -0.53f, 0.f };

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

	MenuButtons[2] = new UUIButton();
	MenuButtons[2]->Create(device, context);
	if (!MenuButtons[2]->SetTexture(L"Resource\\Image\\messages\\ko\\computer_computer.png"))
	{
		return;
	}
	MenuButtons[2]->SetPosition(MenuButtonPositions[2]);
	MenuButtons[2]->SetScale(1.0f);
	GameObjects.push_back(MenuButtons[2]);

	SelectedMenuIndex = 0;

	obj = new UUIImage();
	obj->Create(device, context);
	if (!obj->SetTexture(L"Resource\\Image\\DeveloperName.png"))
	{
		return;
	}
	obj->SetPosition(FVector3{ 0.f, -0.8f, 0.0f });
	GameObjects.push_back(obj);

	UpdateMenuVisuals();
}

void UMainTitleScene::Update(float tick)
{
	UpdateBackgroundAnimation(tick);
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

bool UMainTitleScene::InitializeBackgroundTiles(ID3D11Device* device, ID3D11DeviceContext* context)
{
	constexpr const wchar_t* BackgroundTexturePath = L"Resource\\Image\\sitting_pikachu.png";

	UTexture2D* BackgroundTexture = UEngine::GetInstance().GetResourceManager().LoadTexture(BackgroundTexturePath);
	if (BackgroundTexture == nullptr)
	{
		return false;
	}

	const D3D11_VIEWPORT& Viewport = UEngine::GetInstance().GetRenderer().GetViewportInfo();
	if (Viewport.Width <= 0.0f || Viewport.Height <= 0.0f)
	{
		return false;
	}

	const float TileWidth = (static_cast<float>(BackgroundTexture->GetWidth()) / Viewport.Width) * 2.0f * BackgroundTileScale;
	const float TileHeight = (static_cast<float>(BackgroundTexture->GetHeight()) / Viewport.Height) * 2.0f * BackgroundTileScale;
	if (TileWidth <= 0.0f || TileHeight <= 0.0f)
	{
		return false;
	}

	BackgroundTileStep = FVector3{ TileWidth, TileHeight, 0.0f };
	BackgroundTileColumns = (std::max)(1, static_cast<int>(std::ceil(2.0f / TileWidth)) + 3);
	BackgroundTileRows = (std::max)(1, static_cast<int>(std::ceil(2.0f / TileHeight)) + 3);
	BackgroundTiles.reserve(BackgroundTileColumns * BackgroundTileRows);

	for (int Row = 0; Row < BackgroundTileRows; ++Row)
	{
		for (int Column = 0; Column < BackgroundTileColumns; ++Column)
		{
			UUIImage* Tile = new UUIImage();
			Tile->Create(device, context);
			if (!Tile->SetTexture(BackgroundTexturePath))
			{
				return false;
			}

			Tile->SetScale(BackgroundTileScale);
			GameObjects.push_back(Tile);
			BackgroundTiles.push_back(Tile);
		}
	}

	UpdateBackgroundTilePositions();
	return true;
}

void UMainTitleScene::UpdateBackgroundAnimation(float tick)
{
	if (BackgroundTiles.empty() || BackgroundTileStep.x <= 0.0f || BackgroundTileStep.y <= 0.0f)
	{
		return;
	}

	BackgroundScrollOffset.x -= BackgroundScrollSpeed * tick;
	BackgroundScrollOffset.y += BackgroundScrollSpeed * tick;

	while (BackgroundScrollOffset.x <= -BackgroundTileStep.x)
	{
		BackgroundScrollOffset.x += BackgroundTileStep.x;
	}

	while (BackgroundScrollOffset.y >= BackgroundTileStep.y)
	{
		BackgroundScrollOffset.y -= BackgroundTileStep.y;
	}

	UpdateBackgroundTilePositions();
}

void UMainTitleScene::UpdateBackgroundTilePositions() const
{
	if (BackgroundTiles.empty())
	{
		return;
	}

	const float StartX = -1.0f - (BackgroundTileStep.x * 0.5f);
	const float StartY = -1.0f - (BackgroundTileStep.y * 0.5f);

	for (int Row = 0; Row < BackgroundTileRows; ++Row)
	{
		for (int Column = 0; Column < BackgroundTileColumns; ++Column)
		{
			const int TileIndex = Row * BackgroundTileColumns + Column;
			UUIImage* Tile = BackgroundTiles[TileIndex];
			if (Tile == nullptr)
			{
				continue;
			}

			const float X = StartX + (BackgroundTileStep.x * Column) + BackgroundScrollOffset.x;
			const float Y = StartY + (BackgroundTileStep.y * Row) + BackgroundScrollOffset.y;
			Tile->SetPosition(FVector3{ X, Y, 0.0f });
		}
	}
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
		UEngine::GetInstance().GetSoundManager().PlaySound(L"Pikachu_Button.wav", SOUND_UI, SYSTEM_VOLUME);
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
		UEngine::GetInstance().GetSceneManager().RequestChangeScene("UMainGameScene");
		UEngine::GetInstance().GetSoundManager().PlaySound(L"Pikachu_GameStart.wav", SOUND_UI, SYSTEM_VOLUME);
		UGameManager::GetInstance().SetAIMode(true);
		UGameManager::GetInstance().SetAIVsAIMode(false);
		return;
	}

	if (SelectedMenuIndex == 1)
	{
		UEngine::GetInstance().GetSceneManager().RequestChangeScene("UMainGameScene");
		UEngine::GetInstance().GetSoundManager().PlaySound(L"Pikachu_GameStart.wav",SOUND_UI, SYSTEM_VOLUME);
		UGameManager::GetInstance().SetAIMode(false);
		UGameManager::GetInstance().SetAIVsAIMode(false);
		return;
	}

	if (SelectedMenuIndex == 2)
	{
		UEngine::GetInstance().GetSceneManager().RequestChangeScene("UMainGameScene");
		UEngine::GetInstance().GetSoundManager().PlaySound(L"Pikachu_GameStart.wav", SOUND_UI, SYSTEM_VOLUME);
		UGameManager::GetInstance().SetAIMode(false);
		UGameManager::GetInstance().SetAIVsAIMode(true);
		return;
	}
}
