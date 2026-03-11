#pragma once
#include "UGameObject.h"
class UCircleCollider;

class UBall : public UGameObject
{
public:
	UBall();
	~UBall() override;

	static UBall* Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	const char* GetEditorTypeName() const override { return "UBall"; }
	void SetScale(float scale) override;

	void ApplyBoundaryCollision();

	void SetRadius(float radius);
	float GetRadius() const;

	void Release() override;

private:
	//UCircleCollider* Collider; //중복 변수 제거
	float Radius;
};

