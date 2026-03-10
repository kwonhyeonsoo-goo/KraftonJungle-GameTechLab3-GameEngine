#pragma once
#include "UGameObject.h"

class UUI : public UGameObject
{
public:
	UUI() = default;
	~UUI() override = default;

	virtual void Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	const char* GetEditorTypeName() const override { return "UUI"; }
	void Release() override;

	bool IsVisible() const { return bVisible; }
	void SetVisible(bool bVisibleIn) { bVisible = bVisibleIn; }

protected:
	virtual void OnCreate(ID3D11Device* device, ID3D11DeviceContext* context);
	virtual void OnUpdate(float tick);
	virtual void OnRender(ID3D11DeviceContext* context, ID3D11Device* device) = 0;
	virtual void OnRelease();

private:
	bool bVisible = true;
};

