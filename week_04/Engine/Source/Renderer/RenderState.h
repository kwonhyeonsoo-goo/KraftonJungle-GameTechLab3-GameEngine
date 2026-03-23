#pragma once

#include "EngineAPI.h"
#include "Types/CoreTypes.h"
#include <d3d11.h>
#include <memory>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct FRasterizerStateOption {
	bool isDirty = true;	// 최초 1회 초기화 보장
	D3D11_FILL_MODE FillMode = D3D11_FILL_SOLID;
	D3D11_CULL_MODE CullMode = D3D11_CULL_BACK;
	bool DepthClipEnable = true;
	int DepthBias = 0; //

	uint32 ToKey() const {
		uint32 key = 0;
		key |= (static_cast<uint32>(FillMode) & 0x7);       // 3 bits
		key |= (static_cast<uint32>(CullMode) & 0x7) << 3;  // 3 bits
		key |= (DepthClipEnable ? 1 : 0) << 6;               // 1 bit
		key |= (static_cast<uint32>(DepthBias) & 0xFFFF) << 7; // 16 bits (예시)
		return key;
	}
};

struct ENGINE_API FRasterizerState
{
public:
	~FRasterizerState() = default;

	static std::shared_ptr<FRasterizerState> Create(
		ID3D11Device* Device, const FRasterizerStateOption& Option );

	void Bind(ID3D11DeviceContext* DeviceContext) const;
	void Release();

private:
	FRasterizerState() = default;
	ComPtr<ID3D11RasterizerState> State = nullptr;
};

// TODO: FStencilStateOption 추가
struct FStencilStateOption;
struct FStencilState;