#include "UShader.h"

#include "Utility.h"

UShader::UShader() : VertexShader(nullptr), PixelShader(nullptr), InputLayout(nullptr), ConstantBuffer(nullptr)
{
}

UShader::~UShader()
{
	Release();
}

bool UShader::Create(ID3D11Device* device, const std::wstring& shaderFilePath,
	const D3D11_INPUT_ELEMENT_DESC* inputElements, UINT inputElementCount, const char* vsEntry, const char* psEntry)
{
	if (device == nullptr)
	{
		return false;
	}

	Release();

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;

	// 1. Vertex Shader 哪颇老
	if (!CompileShaderFromFile(shaderFilePath, vsEntry, "vs_5_0", &vsBlob))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	// 2. Pixel Shader 哪颇老
	if (!CompileShaderFromFile(shaderFilePath, psEntry, "ps_5_0", &psBlob))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	// 3. Vertex Shader 积己
	HRESULT hr = device->CreateVertexShader(
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		nullptr,
		&VertexShader
	);

	if (FAILED(hr))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	// 4. Pixel Shader 积己
	hr = device->CreatePixelShader(
		psBlob->GetBufferPointer(),
		psBlob->GetBufferSize(),
		nullptr,
		&PixelShader
	);

	if (FAILED(hr))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	// 5. Input Layout 积己
	hr = device->CreateInputLayout(
		inputElements,
		inputElementCount,
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&InputLayout
	);

	if (FAILED(hr))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	// 6. Constant Buffer 积己
	if (!CreateConstantBuffers(device))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	SafeRelease(psBlob);
	SafeRelease(vsBlob);

	return true;
}

void UShader::Release()
{
	SafeRelease(ConstantBuffer);
	SafeRelease(InputLayout);
	SafeRelease(PixelShader);
	SafeRelease(VertexShader);
}

void UShader::Bind(ID3D11DeviceContext* deviceContext)
{
	if (deviceContext == nullptr)
	{
		return;
	}

	deviceContext->IASetInputLayout(InputLayout);
	deviceContext->VSSetShader(VertexShader, nullptr, 0);
	deviceContext->PSSetShader(PixelShader, nullptr, 0);

	if (ConstantBuffer)
	{
		deviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	}
}

void UShader::UnBind(ID3D11DeviceContext* deviceContext)
{
	if (deviceContext == nullptr)
	{
		return;
	}

	deviceContext->VSSetShader(nullptr, nullptr, 0);
	deviceContext->PSSetShader(nullptr, nullptr, 0);
	deviceContext->IASetInputLayout(nullptr);
}

void UShader::UpdateConstant(ID3D11DeviceContext* deviceContext, FVector3 offset, float scale)
{
	if (ConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE constantBufferMSR;
		deviceContext->Map(
			ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0,
			&constantBufferMSR); // update constant buffer every frame
		FConstants* constants = static_cast<FConstants*>(constantBufferMSR.pData);
		{
			constants->Offset = offset;
			constants->Scale = scale;
		}
		deviceContext->Unmap(ConstantBuffer, 0);
	}
}

bool UShader::CreateConstantBuffers(ID3D11Device* device)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(FConstants);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &ConstantBuffer);
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}
