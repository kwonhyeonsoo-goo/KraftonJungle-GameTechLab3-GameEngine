#include "RenderState.h"

// ===== FRasterizerStateOption =====

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
	desc.FrontCounterClockwise = false;
	desc.ScissorEnable = false;
	desc.MultisampleEnable = false;
	desc.AntialiasedLineEnable = false;


	HRESULT Hr = InDevice->CreateRasterizerState(&desc, RS->State.GetAddressOf());

	if (FAILED(Hr))
	{
		return nullptr;
	}
	return RS;
}

void FRasterizerState::Bind(ID3D11DeviceContext* InDeviceContext) const
{
	InDeviceContext->RSSetState(State.Get());
}

// ===== FDepthStencilStateOption =====

std::shared_ptr<FDepthStencilState> FDepthStencilState::Create(
	ID3D11Device* InDevice, 
	const FDepthStencilStateOption& InOption)
{
	if (!InDevice)
	{
		return nullptr;
	}

	std::shared_ptr<FDepthStencilState> DSS(new FDepthStencilState());

	// 맵에 없으면 새로 생성
	D3D11_DEPTH_STENCIL_DESC desc = {};
	desc.DepthEnable = InOption.DepthEnable;
	desc.DepthWriteMask = InOption.DepthWriteMask;
	desc.StencilEnable = InOption.StencilEnable;
	desc.StencilReadMask = InOption.StencilReadMask;
	desc.StencilWriteMask = InOption.StencilWriteMask;
	// 나머지 기본값 설정
	// --- 깊이 테스트 (Depth Test) 기본값 ---
	desc.DepthFunc = D3D11_COMPARISON_LESS;             // 기본: 현재보다 가까운 것만 통과

	// --- 앞면(FrontFace) 스텐실 연산 설정 ---
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;      // 기본: 실패 시 유지
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP; // 기본: 깊이 실패 시 유지
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;      // 기본: 통과 시 유지
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;      // 기본: 항상 통과
	// --- 뒷면(BackFace) 스텐실 연산 설정 (앞면과 동일) ---
	desc.BackFace = desc.FrontFace;

	HRESULT Hr = InDevice->CreateDepthStencilState(&desc, DSS->State.GetAddressOf());

	if (FAILED(Hr))
	{
		return nullptr;
	}
	return DSS;
}

void FDepthStencilState::Bind(ID3D11DeviceContext* InDeviceContext) const
{
	InDeviceContext->OMSetDepthStencilState(State.Get(), 1);	// StencilRef 값은 1 고정
}