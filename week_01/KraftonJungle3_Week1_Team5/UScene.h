#pragma once
#include <d3d11.h>
#include <vector>

#include "UPrimitive.h"
class UGameObject;

class UScene : public UPrimitive
{
public:
	UScene() = default;
	~UScene() override;

	virtual void Initialize(ID3D11Device* device, ID3D11DeviceContext* context) = 0;
	virtual void Update(float tick) = 0;
	virtual void Exit() = 0;
	void Render(ID3D11Device* device, ID3D11DeviceContext* context);

	void Release();
	void CheckCollision();

protected:
	std::vector<UGameObject*> GameObjects;
};

