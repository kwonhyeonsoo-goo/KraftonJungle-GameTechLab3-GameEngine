#pragma once

/*
	실제 렌더링을 담당하는 Class 입니다. (Rendering 최상위 클래스)
*/

#include "Render/Common/RenderTypes.h"

#include "Render/Scene/RenderBus.h"
#include "Render/Device/D3DDevice.h"
#include "Render/Resource/RenderResources.h"
#include "Engine/Fonts/FontCache.h"

#include <cstddef>

enum class EViewModeIndex : uint32
{
	VMI_Lit,
	VMI_Unlit,
	VMI_Wireframe,
};

enum class EEngineShowFlags : uint64
{
	SF_None = 0,
	SF_Primitives = 1 <<0 ,	//1
	SF_BillboardText = 1 << 1, //2
	Flag_OptionB = 1 << 2, //4
	Flag_OptionC = 1 << 3, //8
};

class FRenderer
{
private:
	FD3DDevice Device;
	FRenderResources Resources;

	TArray<FVertex> Grids;

	//	File Path
	const WCHAR* ShaderFilePath  = L"Assets/Shader/ShaderW0.hlsl";
	const WCHAR* FontShaderFilePath = L"Assets/Shader/FontShader.hlsl";
	const WCHAR* FontTextureFIlePath = L"Assets/Fonts/Font.dds";

	//	Primitive and Gizmo Input Layout
	D3D11_INPUT_ELEMENT_DESC PrimitiveInputLayout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  static_cast<uint32>(offsetof(FVertex, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<uint32>(offsetof(FVertex, Color)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	//	Overlay Input Layout (Screen Quad)
	D3D11_INPUT_ELEMENT_DESC OverlayInputLayout[1] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Font Input Layout
	D3D11_INPUT_ELEMENT_DESC FontInputLayout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

public:

private:
	void RenderComponentPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus);
	void RenderDepthLessPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus);

	//void RenderEditorPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus);
	//void RenderGridEditorPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus);

	void RenderLineBatchPass(ID3D11DeviceContext* InDeviceContext, FRenderBus& InRenderBus);

	void DrawString(ID3D11DeviceContext* InDeviceContext, FRenderBus& InRenderBus);
	void RenderFont(ID3D11DeviceContext* InDeviceContext);

	void RenderOverlayPass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus);
	void RenderOutlinePass(ID3D11DeviceContext* InDeviceContext, const FRenderBus& InRenderBus);

	void DrawCommand(ID3D11DeviceContext* InDeviceContext, const FRenderCommand& InCommand);

public:
	void Create(HWND hWindow);
	void Release();

	void BeginFrame();
	void Render(FRenderBus& InRenderBus);
	void RenderOverlay(const FRenderBus& InRenderBus);	//	반드시 따로 호출해야 함
	void EndFrame();

	FD3DDevice& GetFD3DDevice() { return Device; }
	FRenderResources& GetResources() { return Resources; }

public: 
	EViewModeIndex viewMode = EViewModeIndex::VMI_Unlit;
	uint64 showFlag = (uint64)EEngineShowFlags::SF_Primitives;

};

