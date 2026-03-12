#include "UBall.h"

#include "UCircleCollider.h"
#include "Utility.h"

#include "TextureRenderer.h"
#include "Animator.h"
#include "UEngine.h"
#include "UUIImage.h"
#include "UGameManager.h"

UBall::UBall() : Radius(1.f)
{
	SetObjectType(ObjectType::Ball);
}

UBall::~UBall()
= default;

UBall* UBall::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	UBall* instance = new UBall();
	instance->Collider = new UCircleCollider();
	instance->Collider->Create(device, instance);

	//스프라이트 애니메이션
	instance->BallTextureRenderer = new TextureRenderer();
	instance->BallTextureRenderer->Create(device, context);
	instance->BallTextureRenderer->Init(device, context, L"sprite_sheet.png");

	instance->AnimatorComponent = new Animator();
	instance->AnimatorComponent->SetFrameDuration(0.1f);

	std::vector<std::wstring> idleFrames = {
L"Resource/Image/ball/ball_0.png",
L"Resource/Image/ball/ball_1.png",
L"Resource/Image/ball/ball_2.png",
L"Resource/Image/ball/ball_3.png",
L"Resource/Image/ball/ball_4.png"
	};
	instance->AnimatorComponent->AddFrames("Idle", idleFrames);
	instance->AnimatorComponent->Play("Idle", AnimationMode::Loop);

	//잔상
	instance->BallHyper = new UUIImage();
	instance->BallHyper->Create(device, context);
	instance->BallHyper->SetTexture(L"Resource/Image/ball/ball_hyper.png");

	instance->BallTrail = new UUIImage();
	instance->BallTrail->Create(device, context);	
	instance->BallTrail->SetTexture(L"Resource/Image/ball/ball_trail.png");

	//충돌 이펙트(펀치)
	instance->BallPunch = new UUIImage();
	instance->BallPunch->Create(device, context);
	instance->BallPunch->SetTexture(L"Resource/Image/ball/ball_punch.png");

	return instance;
}

void UBall::Init()
{
	elapsedTime = 0.f;
	PunchTimer = 0.f;
	isSpike = false;
	PreviousPosition = HyperPosition = TrailPosition = Position;
}

void UBall::Physics_Update(const float tick)
{
	// UGameObject에서 설명한 것과 같이 속도 변화 반영(중력 적용) -> 속도 반영 -> 충돌 반영
	ApplyGravity(tick);
	ApplyVelocity(tick);
	ApplyBoundaryCollision();
}

void UBall::Update(float tick)
{
	elapsedTime += tick;
	PunchTimer -= tick;

	//잔상
	if(elapsedTime > TrailTimer)
	{
		elapsedTime = 0;

		TrailPosition = HyperPosition;
		HyperPosition = PreviousPosition;
		PreviousPosition = Position;

	}

	AnimatorComponent->Update(BallTextureRenderer, tick);
	BallHyper->SetPosition(HyperPosition);
	BallTrail->SetPosition(TrailPosition);

	if (isSpike) {
		BallHyper->SetVisible(true);
		BallTrail->SetVisible(true);

	}
	else {
		BallHyper->SetVisible(false);
		BallTrail->SetVisible(false);
	}

	//충돌
	if (PunchTimer > 0)
	{
		BallPunch->SetVisible(true);
		BallPunch->SetScale(BallPunch->GetScale() - tick * 3.0f);
	}
	else
		BallPunch->SetVisible(false);

}

void UBall::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{

	Collider->Debug_Render(context, device);

	BallTrail->Render(context, device);
	BallHyper->Render(context, device);
	BallTextureRenderer->Draw(context, device, Position, 1.f);
	BallPunch->Render(context, device);


}

void UBall::SetScale(float scale)
{
	UGameObject::SetScale(scale);
}

void UBall::ApplyBoundaryCollision()
{
	if (Position.x > 1.f - Radius)
	{
		Velocity.x *= -0.9f;
		//Velocity.x *= -1.f;
		Position.x = 1.f - Radius;
	}
	if (Position.x < -1.f + Radius)
	{
		Velocity.x *= -0.9f;
		//Velocity.x *= -1.f;
		Position.x = -1.f + Radius;
	}
	if (Position.y > 1.f - Radius)
	{
		Velocity.y *= -1.f;
		Position.y = 1.f - Radius;
	}
	if (Position.y < -0.8f + Radius) //바닥 튕김은 GameManager 에서 관리
	{
		Velocity.y *= -1;
		Position.y = -0.8f + Radius;
		UGameManager::GetInstance().CheckScore();
		//bIsCollide = false;
		PlayBallPunchEffect({ Position.x, -0.8f, 0.0f });
		// 공 바닥에 닿을 때 소리 출력이 여러번 되는 문제가 있어서 스코어 올릴 때 처리 해주는 곳으로 옮김.
		//		UEngine::GetInstance().GetSoundManager().PlaySound(L"Ball_GroundHit.wav", SOUND_EFFECT, SYSTEM_VOLUME);
	}
}

void UBall::SetRadius(const float radius)
{
	Radius = radius;

	if (Collider->GetColliderType() == ColliderType::ColliderType_Circle)
	{
		static_cast<UCircleCollider*>(Collider)->SetRadius(radius);
	}
}

float UBall::GetRadius() const
{
	return Radius;
}

void UBall::Release()
{
	SafeDelete(Collider);
	SafeDelete(BallTextureRenderer);
	SafeReleaseAndDelete(BallTrail);
	SafeReleaseAndDelete(BallHyper);
}

void UBall::SetSpike(bool spike)
{
	isSpike = spike;
	if(spike) 
	PlayBallPunchEffect(Position);
}

void UBall::SetSpike(bool spike, FVector3 TargetPosition)
{
	isSpike = spike;

	FVector3 direction = (TargetPosition - Position).Normalize()*Radius;

	if(spike)
	{
		PlayBallPunchEffect(Position + direction);
		UEngine::GetInstance().GetSoundManager().PlaySound(L"Ball_Smash.wav", SOUND_EFFECT, SYSTEM_VOLUME);
	}
}

void UBall::PlayBallPunchEffect(FVector3 effectPosition)
{
	PunchTimer = PunchLifetime;
	BallPunch->SetPosition(effectPosition);
	BallPunch->SetScale(1.0f);
}

