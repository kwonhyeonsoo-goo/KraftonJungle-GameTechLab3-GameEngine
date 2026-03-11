#include "UBall.h"

#include "UCircleCollider.h"
#include "Utility.h"

#include "TextureRenderer.h"
#include "Animator.h"
#include "UUIImage.h"

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


	return instance;
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
}

void UBall::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{

	Collider->Debug_Render(context, device);

	BallTrail->Render(context, device);
	BallHyper->Render(context, device);
	BallTextureRenderer->Draw(context, device, Position, 1.f);



}

void UBall::SetScale(float scale)
{
	UGameObject::SetScale(scale);
}

void UBall::ApplyBoundaryCollision()
{
	if (Position.x > 1.f - Radius)
	{
		Velocity.x *= -1;
		Position.x = 1.f - Radius;
	}
	if (Position.x < -1.f + Radius)
	{
		Velocity.x *= -1;
		Position.x = -1.f + Radius;
	}
	if (Position.y > 1.f - Radius)
	{
		Velocity.y *= -1;
		Position.y = 1.f - Radius;
	}
	if (Position.y < -0.8f + Radius) //바닥 튕김은 GameManager 에서 관리
	{
		Velocity.y *= -1;
		Position.y = -0.8f + Radius;
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
}
