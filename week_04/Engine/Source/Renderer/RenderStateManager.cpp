#include "RenderStateManager.h"
#include "RenderState.h"

void CRenderStateManager::PrepareCommonStates()
{
	// 예: Solid/Wireframe x Cull None/Back/Front
	D3D11_FILL_MODE fills[] = { D3D11_FILL_SOLID, D3D11_FILL_WIREFRAME };
	D3D11_CULL_MODE culls[] = { D3D11_CULL_NONE, D3D11_CULL_FRONT, D3D11_CULL_BACK };

	for (auto f : fills) {
		for (auto c : culls) {
			FRasterizerStateOption opt;
			opt.FillMode = f;
			opt.CullMode = c;
			GetOrCreateRenderState(opt);
		}
	}
}

std::shared_ptr<FRasterizerState> CRenderStateManager::GetOrCreateRenderState(const FRasterizerStateOption& opt)
{
	uint32_t key = opt.ToKey();

	// 이미 만들어놓은 게 있다면 그것을 반환
	auto it = RasterizerStateMap.find(key);
	if (it != RasterizerStateMap.end()) {
		return it->second;
	}

	// 맵에 없으면 새로 생성
	RasterizerStateMap[key] = FRasterizerState::Create(Device, opt);
	return RasterizerStateMap[key];
}

std::shared_ptr<FDepthStencilState> CRenderStateManager::GetOrCreateDepthStencilState(const FDepthStencilStateOption& opt)
{
	uint32_t key = opt.ToKey();

	// 이미 만들어놓은 게 있다면 그것을 반환
	auto it = DepthStencilStateMap.find(key);
	if (it != DepthStencilStateMap.end()) {
		return it->second;
	}

	// 맵에 없으면 새로 생성
	DepthStencilStateMap[key] = FDepthStencilState::Create(Device, opt);
	return DepthStencilStateMap[key];
}

void CRenderStateManager::BindState(std::shared_ptr<FRasterizerState> InRS)
{
	if (InRS != nullptr && CurrentRasterizerState != InRS)
	{
		InRS->Bind(DeviceContext);
		CurrentRasterizerState = InRS;
	}
}

void CRenderStateManager::BindState(std::shared_ptr<FDepthStencilState> InDSS)
{
	if (InDSS != nullptr && CurrentDepthStencilState != InDSS)
	{
		InDSS->Bind(DeviceContext);
		CurrentDepthStencilState = InDSS;
	}
}

void CRenderStateManager::RebindState()
{
	if(CurrentRasterizerState)
		CurrentRasterizerState->Bind(DeviceContext);
	if (CurrentDepthStencilState)
		CurrentDepthStencilState->Bind(DeviceContext);
}