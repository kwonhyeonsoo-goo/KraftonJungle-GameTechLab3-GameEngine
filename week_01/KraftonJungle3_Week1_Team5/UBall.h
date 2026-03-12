#pragma once
#include "UGameObject.h"
class UCircleCollider;
class TextureRenderer;
class Animator;
class UUIImage;

class UBall : public UGameObject
{
public:
	UBall();
	~UBall() override;

	static UBall* Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Init();

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	const char* GetEditorTypeName() const override { return "UBall"; }
	void SetScale(float scale) override;

	void ApplyBoundaryCollision();

	void SetRadius(float radius);
	float GetRadius() const;

	void Release() override;

	void SetSpike(bool spike);
	void SetSpike(bool spike, FVector3 TargetPosition);

	void PlayBallPunchEffect(FVector3 effectPosition);

private:
	//UCircleCollider* Collider; //중복 변수 제거
	float Radius;
	bool isSpike = false;

	TextureRenderer* BallTextureRenderer;
	Animator* AnimatorComponent;
	UUIImage* BallTrail;
	UUIImage* BallHyper;
	UUIImage* BallPunch;


	FVector3 PreviousPosition = Position;
	FVector3 HyperPosition = Position;
	FVector3 TrailPosition = Position;

	float TrailTimer = 0.05f;
	float elapsedTime = 0.f;
	float PunchTimer = 0.0f;
	float PunchLifetime = 0.3f;
};

