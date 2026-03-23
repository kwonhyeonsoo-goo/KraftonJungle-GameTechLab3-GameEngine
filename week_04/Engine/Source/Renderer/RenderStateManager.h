#pragma once

#include "RenderState.h"
#include "Types/CoreTypes.h"
#include "Types/Map.h"
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class CRenderStatemanager
{
private:
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	TMap<uint32_t, std::shared_ptr<FRasterizerState>> StateMap;
	std::shared_ptr<FRasterizerState> CurrentRenderState = nullptr;

public:
	CRenderStatemanager(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext) 
		: Device(InDevice)
		, DeviceContext(InDeviceContext)
	{}

	// 자주 사용되는 상태들을 미리 생성
	void PrepareCommonStates();

	// 옵션에 따른 FRasterizerState 반환 (없으면 생성)
	std::shared_ptr<FRasterizerState> GetOrCreateRenderState(const FRasterizerStateOption& opt);

	// 실제 FRasterizerState 적용
	void BindState(std::shared_ptr<FRasterizerState> InOption);
};