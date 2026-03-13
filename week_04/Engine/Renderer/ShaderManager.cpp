#include "ShaderManager.h"
#include "ShaderType.h"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")


CShaderManager::~CShaderManager()
{
	Release();
}

bool CShaderManager::LoadVertexShader(ID3D11Device* Device, const wchar_t* FilePath)
{
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* errBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(FilePath, nullptr, nullptr, "main", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vsBlob, &errBlob);
	if (FAILED(hr))
	{
		if (errBlob) {
			OutputDebugStringA((char*)errBlob->GetBufferPointer());
			errBlob->Release();
		}
		return false;
	}
	Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VertexShader);
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	Device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &InputLayout);
	vsBlob->Release();
	D3D11_BUFFER_DESC cbd = {};
	cbd.ByteWidth = sizeof(FConstants);
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Device->CreateBuffer(&cbd, nullptr, &ConstantBuffer);
	return true;
}

bool CShaderManager::LoadPixelShader(ID3D11Device* Device, const wchar_t* FilePath)
{
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(FilePath, nullptr, nullptr, "main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &psBlob, &errBlob);

	if (FAILED(hr))
	{
		if (errBlob) {
			OutputDebugStringA((char*)errBlob->GetBufferPointer());
			errBlob->Release();
		}
		return false;
	}
	Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PixelShader);
	psBlob->Release();
	// TODO: D3DCompileFromFile → CreatePixelShader
	return true;
}

void CShaderManager::Bind(ID3D11DeviceContext* DeviceContext)
{
	DeviceContext->VSSetShader(VertexShader, nullptr, 0);
	DeviceContext->PSSetShader(PixelShader, nullptr, 0);
	DeviceContext->IASetInputLayout(InputLayout);
	DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	// TODO: VSSetShader, PSSetShader, IASetInputLayout
}

void CShaderManager::UpdateConstants(ID3D11DeviceContext* Context, const FConstants& Data)
{
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	Context->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &Data, sizeof(FConstants));
	Context->Unmap(ConstantBuffer, 0);
}

void CShaderManager::Release()
{
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
	if (InputLayout)
	{
		InputLayout->Release();
		InputLayout = nullptr;
	}
	if (PixelShader)
	{
		PixelShader->Release();
		PixelShader = nullptr;
	}
	if (VertexShader)
	{
		VertexShader->Release();
		VertexShader = nullptr;
	}


}
