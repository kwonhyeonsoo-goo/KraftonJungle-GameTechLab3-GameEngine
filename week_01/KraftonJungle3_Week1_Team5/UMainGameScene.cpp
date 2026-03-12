#include "UMainGameScene.h"

#include "SceneAutoRegister.h"
#include "UPikachu.h"
#include "UCircleCollider.h"
#include "URectCollider.h"
#include "UBall.h"
#include "UNet.h"
#include "UWave.h"
#include "Utility.h"
#include "UGameManager.h"
#include "UUIImage.h"
#include "UEngine.h"
#include "TextureRenderer.h"
#include "UShader.h"
#include "UShadow.h"
#include "UUIScore.h"

//임시
#include "ImGui/imgui.h"

REGISTER_SCENE(UMainGameScene)

void UMainGameScene::Enter()
{
	UEngine::GetInstance().GetSoundManager().PlayBGM(L"bgm.mp3", SYSTEM_VOLUME);
}

void UMainGameScene::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	// 이 씬에서 사용할 오브젝트들이나 기타 초기화 작업들을 한다고 생각하시면 된다.
	InitializeUI(device, context);

	UShadow* shadow1 = UShadow::Create(device, context);
	UShadow* shadow2 = UShadow::Create(device, context);
	UShadow* ballShadow = UShadow::Create(device, context);

	GameObjects.push_back(shadow1);
	GameObjects.push_back(shadow2);
	GameObjects.push_back(ballShadow);

	// Player1
	Player1 = new UPikachu();
	Player1->Create(device, context);
	Player1->TextureRender->SetFlipDraw(false);
	Player1->DefalutFlip = false;

	Player1->SetKeyConfig({ 'W', 'S', 'A', 'D', VK_SPACE });
	Player1->SetBoundary(-1.0f, -0.001f, 1.0f, -0.75f);
	// Player1 게임 시작 위치
	Player1->SetPosition(FVector3(-0.8f, -0.75f, 0.0f));

	UCircleCollider* Collider1 = new UCircleCollider();
	Collider1->Create(device, Player1);
	Collider1->SetRadius(2.f);

	Player1->SetCollider(Collider1);
	Player1->SetType(EPlayerType::Player1);
	shadow1->SetTarget(Player1);

	GameObjects.push_back(Player1);
  
    // Player2
	Player2 = new UPikachu();
	Player2->Create(device, context);
	Player2->TextureRender->SetFlipDraw(true); // Player2는 좌우 반전된 이미지 사용
	Player2->DefalutFlip = true;
	Player2->SetKeyConfig({ VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN });
	Player2->SetBoundary(0.001f, 1.0f, 1.0f, -0.75f);
	// Player2 게임 시작 위치
	Player2->SetPosition(FVector3(0.8f, -0.75f, 0.0f));

	UCircleCollider* Collider2 = new UCircleCollider();
	Collider2->Create(device, Player2);
	Collider2->SetRadius(2.f);
	Player2->SetCollider(Collider2);
	Player2->SetType(EPlayerType::Player2);
	shadow2->SetTarget(Player2);

	GameObjects.push_back(Player2);

	UBall* ball = UBall::Create(device, context);
	{
		// 속도는 -0.5 ~ 0.5로 설정
		FVector3 rendVelocity{ 0.f, -1.f, 0.f};

		float rendRadius{0.2f};

		ball->SetVelocity(rendVelocity);
		ball->SetScale(.1f);
		ball->SetRadius(0.1f); //radius 지정이 빠짐
		ball->SetUseGravity(true);

		GameObjects.push_back(ball);

		ballShadow->SetTarget(ball);
	}

	Player1->SetTargetBall(ball);
	Player2->SetTargetBall(ball);

	Net = new UNet();
	Net->Create(device, context);
	
	UWave* Wave = new UWave();
	Wave->Create(device, context);
	GameObjects.push_back(Wave);

	// 게임을 초기화 합니다.
	UGameManager::GetInstance().Initialize(Player1, Player2, ball,
		Player1->GetPosition(), Player2->GetPosition());

	ball->Init();
	if (UGameManager::GetInstance().IsAIMode())
	{
		Player1->SetIsAI(true);
	}

	if (UGameManager::GetInstance().IsAIVsAIMode())
	{
		Player1->SetIsAI(true);
		Player2->SetIsAI(true);
	}
}

void UMainGameScene::Update(float tick)
{
	// 테스트를 위한 Game Manager 관련 코드 주석 처리

	UGameManager& GM = UGameManager::GetInstance();
	GM.Update(tick);

	P1_Score->SetScore(GM.GetP1Point());
	P2_Score->SetScore(GM.GetP2Point());
	P1_Score->Update(tick);
	P2_Score->Update(tick);

	if ((GM.GetGameState() == EGameState::GameOver) && (GetAsyncKeyState(VK_RETURN) & 0x8000))
	{
		UEngine::GetInstance().GetSceneManager().RequestChangeScene("UMainTitleScene");
	}

	// 포인트 획득 상태나 게임 종료시 Update 종료
	//if (GM.GetGameState() == EGameState::GameOver)
	//{
	//	{
	//		//여기에 승리 실패 시 출력될 애니메이션 코드를 넣습니다.
	//		Player1->Update(tick);
	//		Player2->Update(tick);
	//	}

	//	return;
	//}

	//if (GM.GetGameState() == EGameState::Serving)
	//{
	//	{
	//		// 여기에 새로운 게임 시작시 출력될
	//		// 게임 시작! "READY?" 등의 문구를 출력합니다.
	//	}
	//	return;
	//}
	
	for (auto& gameObject : GameObjects)
	{
		if (GM.GetGameState() == EGameState::GameOver)
		{
			break;
		}

		if(GM.GetGameState() == EGameState::PointScored)
			gameObject->Physics_Update(tick/3.f);
		else
			gameObject->Physics_Update(tick);
	}
	if (GM.GetGameState() == EGameState::GameOver)
	{
		for (auto& cloud : Clouds)
		{
			cloud->Physics_Update(tick);
		}

		GameSetUI->SetVisible(true);
		GameSetUI->SetScale(LerpToTarget(GameSetBaseScale, GameSetTargetScale, GameSetUIAnimationDurationTime, GameSetLerpTime));
		GameSetUIAnimationDurationTime += tick;
	}
	/*if (GM.GetGameState() != EGameState::Playing && GM.GetGameState() != EGameState::GameOver)
		return;*/

	for (auto& gameObject : GameObjects)
	{
		if (GM.GetGameState() == EGameState::GameOver)
		{
			if (gameObject->GetObjectType() != ObjectType::Pikachu)
			{
				continue;
			}
				
			gameObject->Update(tick);
		}
		else
		{
			gameObject->Update(tick);
		}
	}
	/*if (GM.GetGameState() == EGameState::GameOver)
		return;*/

	CheckCollision();

	for (auto& gameObject : GameObjects)
	{
		if (gameObject->GetObjectType() == ObjectType::Ball)
		{
			Net->HandleBallCollision(static_cast<UBall*>(gameObject));
		}
	}

	if (GM.GetGameState() == EGameState::PointScored)
	{
		UpdateCloudImageAnimation(tick / 3.f);
	}

	else
	{
		UpdateCloudImageAnimation(tick);
	}
}

void UMainGameScene::Exit()
{
	UEngine::GetInstance().GetSoundManager().StopAll();
}
// 피카츄 배구 게임에 오브젝트는 3개
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
							ObjectType type1 = GameObjects[i]->GetObjectType();
							ObjectType type2 = GameObjects[j]->GetObjectType();

							if (type1 == ObjectType::Pikachu && type2 == ObjectType::Ball)
							{
								UPikachu* Pikachu = static_cast<UPikachu*>(GameObjects[i]);
								UBall* Ball = static_cast<UBall*>(GameObjects[j]);
								Pikachu->HandleCollision(Ball);
							}
							else if(type1 == ObjectType::Ball && type2 == ObjectType::Pikachu)
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

	Clouds.reserve(CloudCount);
	CloudAnimationTime.reserve(CloudCount);

	for (int i = 0; i < CloudCount; ++i)
	{
		UUIImage* cloud = new UUIImage();
		cloud->Create(device, context);
		if (!cloud->SetTexture(L"Resource\\Image\\objects\\cloud.png"))
		{
			return;
		}
		// 랜덤한 높이
		float yPos = static_cast<float>(RandomRange(0, 1));
		float xPos = static_cast<float>(RandomRange(-1, 1));
		cloud->SetPosition({  xPos, yPos, 0.f });
		
		// 랜덤한 애니메이션 시작 타이밍
		float randomAnimationTime = static_cast<float>(RandomRange(0, 1));
		CloudAnimationTime.push_back(randomAnimationTime);
		
		// 랜덤한 속도
		float randomVelocity = static_cast<float>(RandomRange(0.1, 0.15));
		cloud->SetVelocity({ randomVelocity,0.f,0.f });

		GameObjects.push_back(cloud);
		Clouds.push_back(cloud);
	}

	P1_Score = new UUIScore();
	P1_Score->Create(device, context);
	P1_Score->SetPosition({ -0.7f, 0.75f, 0.f });
	GameObjects.push_back(P1_Score);

	P2_Score = new UUIScore();
	P2_Score->Create(device, context);
	P2_Score->SetPosition({ 0.7f, 0.75f, 0.f });
	GameObjects.push_back(P2_Score);

	GameSetUI = new UUIImage();
	GameSetUI->Create(device, context);
	GameSetUI->SetTexture(L"Resource\\Image\\messages\\common\\game_end.png");
	GameSetUI->SetPosition({ 0.f, 0.3f,0.f });
	GameSetUI->SetVisible(false);
	GameObjects.push_back(GameSetUI);
}

void UMainGameScene::UpdateCloudImageAnimation(const float tick)
{
	for (int i = 0; i < CloudCount; ++i)
	{
		if (auto& cloud = Clouds[i])
		{
			auto& cloudAnimationTime = CloudAnimationTime[i];
			cloudAnimationTime += tick;

			const float animation = std::sin(cloudAnimationTime * CloudPulseSpeed);
			const float scaleOffset = (animation * 0.5f + 0.5f) * CloudScaleAmplitude;
			cloud->SetScale(CloudBaseScale + scaleOffset);

			UpdateCloudMovement(cloud);
		}
	}
}

void UMainGameScene::UpdateCloudMovement(UUIImage* cloud)
{
	if (cloud)
	{
		float cloud_xPos = cloud->GetPosition().GetX();
		if (cloud_xPos >= 1.2f)
		{
			// 랜덤한 높이
			float yPos = static_cast<float>(RandomRange(0, 1));

			cloud->SetPosition({-1.2f, yPos, 0.f});
		}
	}
}

// 임시
void UMainGameScene::OnImGuiRender()
{
	if (Player1 == nullptr || Player2 == nullptr) return;

	if (ImGui::Begin("Pikachu AI"))
	{
		bool bP1IsAI = Player1->GetIsAI();
		bool bP2IsAI = Player2->GetIsAI();

		if (ImGui::Checkbox("Enable Player 1 AI", &bP1IsAI))
		{
			Player1->SetIsAI(bP1IsAI);
		}

		if (ImGui::Checkbox("Enable Player 2 AI", &bP2IsAI))
		{
			Player2->SetIsAI(bP2IsAI);
		}

		ImGui::End();
	}
}
