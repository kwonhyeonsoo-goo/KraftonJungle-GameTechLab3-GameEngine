#include "UUI.h"

void UUI::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	SetUseGravity(false);
	SetCollider(nullptr);
	SetScale(1.0f);
	OnCreate(device, context);
}

void UUI::Physics_Update(float tick)
{
	(void)tick;
}

void UUI::Update(float tick)
{
	OnUpdate(tick);
}

void UUI::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	if (!bVisible)
	{
		return;
	}

	OnRender(context, device);
}

void UUI::Release()
{
	OnRelease();
}

void UUI::OnCreate(ID3D11Device* device, ID3D11DeviceContext* context)
{
	(void)device;
	(void)context;
}

void UUI::OnUpdate(float tick)
{
	(void)tick;
}

void UUI::OnRelease()
{
}
