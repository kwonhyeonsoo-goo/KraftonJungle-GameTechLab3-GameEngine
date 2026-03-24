#pragma once

#include "EngineAPI.h"
#include "Types/CoreTypes.h"
#include <d3d11.h>
#include <memory>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct FRasterizerStateOption {
	bool isDirty = true;	// 최초 1회 초기화 보장
	D3D11_FILL_MODE FillMode = D3D11_FILL_SOLID;	// 기본값 Fill
	D3D11_CULL_MODE CullMode = D3D11_CULL_BACK;
	bool DepthClipEnable = true;
	int DepthBias = 0;

	uint32 ToKey() const {
		uint32 key = 0;
		key |= (static_cast<uint32>(FillMode) & 0x7);				// 3 bit
		key |= (static_cast<uint32>(CullMode) & 0x7) << 3;			// 3 bit
		key |= (DepthClipEnable ? 1 : 0) << 6;						// 1 bit
		key |= (static_cast<uint32>(DepthBias) & 0xFFFF) << 7;		// 16 bit
		return key;
	}
};

struct ENGINE_API FRasterizerState
{
public:
	~FRasterizerState() = default;

	static std::shared_ptr<FRasterizerState> Create(
		ID3D11Device* InDevice, const FRasterizerStateOption& InOption );

	void Bind(ID3D11DeviceContext* InDeviceContext) const;
	void Release();

private:
	FRasterizerState() = default;
	ComPtr<ID3D11RasterizerState> State = nullptr;
};

struct FDepthStencilStateOption {
	bool isDirty = true;	// 최초 1회 초기화 보장
	bool DepthEnable = true;
	D3D11_DEPTH_WRITE_MASK DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	bool StencilEnable = false;
	uint8 StencilReadMask = 0;
	uint8 StencilWriteMask = 0;

	uint32 ToKey() const {
		uint32 key = 0;
		key |= (DepthEnable ? 1 : 0);									// 1 bit
		key |= (DepthEnable ? 1 : 0) << 1;								// 1 bit
		key |= (static_cast<uint32>(DepthWriteMask) & 0x1) << 2;		// 1 bit
		key |= (StencilEnable ? 1 : 0) << 3;							// 3 bit
		key |= (static_cast<uint32>(StencilReadMask) & 0xFF) << 6;		// 8 bit
		key |= (static_cast<uint32>(StencilWriteMask) & 0xFF) << 14;	// 8 bit
		return key;
	}
};

struct FDepthStencilState {
	public:
		~FDepthStencilState() = default;

		static std::shared_ptr<FDepthStencilState> Create(
			ID3D11Device* InDevice, const FDepthStencilStateOption& InOption);

		void Bind(ID3D11DeviceContext* InDeviceContext) const;
		void Release();

	private:
		FDepthStencilState() = default;
		ComPtr<ID3D11DepthStencilState> State = nullptr;
};