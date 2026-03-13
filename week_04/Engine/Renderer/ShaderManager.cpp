#include "ShaderManager.h"
#include "Renderer/PrimitiveVertex.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

CShaderManager::~CShaderManager()
{
	Release();
}

bool CShaderManager::LoadVertexShader(ID3D11Device* Device, const wchar_t* FilePath)
{
	ID3DBlob* ShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT Hr = D3DCompileFromFile(
		FilePath, nullptr, nullptr,
		"main", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &ShaderBlob, &ErrorBlob
	);

	if (FAILED(Hr))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
			ErrorBlob->Release();
		}
		return false;
	}

	Hr = Device->CreateVertexShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr, &VertexShader
	);

	if (FAILED(Hr))
	{
		ShaderBlob->Release();
		return false;
	}

	// InputLayout: Position(float3) + Color(float4) + Normal(float3)
	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	Hr = Device->CreateInputLayout(
		Layout, 3,
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		&InputLayout
	);

	ShaderBlob->Release();
	return SUCCEEDED(Hr);
}

bool CShaderManager::LoadPixelShader(ID3D11Device* Device, const wchar_t* FilePath)
{
	ID3DBlob* ShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	HRESULT Hr = D3DCompileFromFile(
		FilePath, nullptr, nullptr,
		"main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &ShaderBlob, &ErrorBlob
	);

	if (FAILED(Hr))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
			ErrorBlob->Release();
		}
		return false;
	}

	Hr = Device->CreatePixelShader(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr, &PixelShader
	);

	ShaderBlob->Release();
	return SUCCEEDED(Hr);
}

void CShaderManager::Bind(ID3D11DeviceContext* DeviceContext)
{
	DeviceContext->IASetInputLayout(InputLayout);
	DeviceContext->VSSetShader(VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(PixelShader, nullptr, 0);
}

void CShaderManager::Release()
{
	if (InputLayout) { InputLayout->Release(); InputLayout = nullptr; }
	if (PixelShader) { PixelShader->Release(); PixelShader = nullptr; }
	if (VertexShader) { VertexShader->Release(); VertexShader = nullptr; }
}
