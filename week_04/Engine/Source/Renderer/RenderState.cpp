#include "RenderState.h"

std::shared_ptr<FRasterizerState> FRasterizerState::Create(
	ID3D11Device* InDevice,
	const FRasterizerStateOption& InOption
)
{
	if (!InDevice)
	{
		return nullptr;
	}

	std::shared_ptr<FRasterizerState> RS(new FRasterizerState());

	// 맵에 없으면 새로 생성
	D3D11_RASTERIZER_DESC desc = {};
	desc.FillMode = InOption.FillMode;
	desc.CullMode = InOption.CullMode;
	desc.DepthClipEnable = InOption.DepthClipEnable;
	desc.DepthBias = InOption.DepthBias;
	// 나머지 기본값 설정
	desc.FrontCounterClockwise = FALSE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;


	HRESULT Hr = InDevice->CreateRasterizerState(&desc, RS->State.GetAddressOf());

	if (FAILED(Hr))
	{
		return nullptr;
	}
	return RS;
}

void FRasterizerState::Bind(ID3D11DeviceContext* DeviceContext) const
{
	DeviceContext->RSSetState(State.Get());
}
