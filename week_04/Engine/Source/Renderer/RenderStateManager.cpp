#include "RenderStateManager.h"

inline void CRenderStatemanager::PrepareCommonStates()
{
	// 예: Solid/Wireframe x Cull None/Back/Front
	D3D11_FILL_MODE fills[] = { D3D11_FILL_SOLID, D3D11_FILL_WIREFRAME };
	D3D11_CULL_MODE culls[] = { D3D11_CULL_NONE, D3D11_CULL_FRONT, D3D11_CULL_BACK };

	for (auto f : fills) {
		for (auto c : culls) {
			FRasterizerStateOption opt;
			opt.FillMode = f;
			opt.CullMode = c;
			GetOrCreateRenderState(opt); // 내부적으로 생성 및 저장
		}
	}
}

std::shared_ptr<FRasterizerState> CRenderStatemanager::GetOrCreateRenderState(const FRasterizerStateOption& opt)
{
	uint32_t key = opt.ToKey();

	// 이미 만들어놓은 게 있다면 그것을 반환
	auto it = StateMap.find(key);
	if (it != StateMap.end()) {
		return it->second;
	}

	// 맵에 없으면 새로 생성
	StateMap[key] = FRasterizerState::Create(Device, opt);
	return StateMap[key];
}

void CRenderStatemanager::BindState(std::shared_ptr<FRasterizerState> InOption)
{
	if (CurrentRenderState != InOption) {
		InOption->Bind(DeviceContext);
		CurrentRenderState = InOption;
	}
}
