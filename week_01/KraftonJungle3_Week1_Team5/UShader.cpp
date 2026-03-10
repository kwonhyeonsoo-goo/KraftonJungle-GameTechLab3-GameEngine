#include "UShader.h"

#include <cstring>
#include <d3d11shader.h>

#include "Utility.h"

UShader::UShader() : VertexShader(nullptr), PixelShader(nullptr), InputLayout(nullptr)
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

	if (!CompileShaderFromFile(shaderFilePath, vsEntry, "vs_5_0", &vsBlob))
	{
		SafeRelease(vsBlob);
		return false;
	}

	if (!CompileShaderFromFile(shaderFilePath, psEntry, "ps_5_0", &psBlob))
	{
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	HRESULT hr = device->CreateVertexShader(
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		nullptr,
		&VertexShader
	);

	if (FAILED(hr))
	{
		Release();
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	hr = device->CreatePixelShader(
		psBlob->GetBufferPointer(),
		psBlob->GetBufferSize(),
		nullptr,
		&PixelShader
	);

	if (FAILED(hr))
	{
		Release();
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	hr = device->CreateInputLayout(
		inputElements,
		inputElementCount,
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&InputLayout
	);

	if (FAILED(hr))
	{
		Release();
		SafeRelease(psBlob);
		SafeRelease(vsBlob);
		return false;
	}

	if (!CreateConstantBuffers(device, vsBlob, psBlob))
	{
		Release();
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
	for (FConstantBufferBinding& binding : ConstantBuffers)
	{
		SafeRelease(binding.Buffer);
	}
	ConstantBuffers.clear();

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

	for (const FConstantBufferBinding& binding : ConstantBuffers)
	{
		if (binding.Buffer == nullptr)
		{
			continue;
		}

		ID3D11Buffer* buffer = binding.Buffer;
		if ((binding.ShaderStages & ShaderStageVertex) != 0u)
		{
			deviceContext->VSSetConstantBuffers(binding.Slot, 1, &buffer);
		}

		if ((binding.ShaderStages & ShaderStagePixel) != 0u)
		{
			deviceContext->PSSetConstantBuffers(binding.Slot, 1, &buffer);
		}
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

	for (const FConstantBufferBinding& binding : ConstantBuffers)
	{
		ID3D11Buffer* nullBuffer = nullptr;
		if ((binding.ShaderStages & ShaderStageVertex) != 0u)
		{
			deviceContext->VSSetConstantBuffers(binding.Slot, 1, &nullBuffer);
		}

		if ((binding.ShaderStages & ShaderStagePixel) != 0u)
		{
			deviceContext->PSSetConstantBuffers(binding.Slot, 1, &nullBuffer);
		}
	}
}

bool UShader::UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, UINT slot, const void* data, UINT dataSize)
{
	if (deviceContext == nullptr || data == nullptr)
	{
		return false;
	}

	FConstantBufferBinding* binding = FindConstantBuffer(slot);
	if (binding == nullptr || binding->Buffer == nullptr || dataSize > binding->SizeInBytes)
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE constantBufferMSR = {};
	HRESULT hr = deviceContext->Map(binding->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantBufferMSR);
	if (FAILED(hr))
	{
		return false;
	}

	std::memset(constantBufferMSR.pData, 0, binding->SizeInBytes);
	std::memcpy(constantBufferMSR.pData, data, dataSize);
	deviceContext->Unmap(binding->Buffer, 0);

	return true;
}

bool UShader::UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, const std::string& bufferName, const void* data, UINT dataSize)
{
	const FConstantBufferBinding* binding = FindConstantBuffer(bufferName);
	if (binding == nullptr)
	{
		return false;
	}

	return UpdateConstantBuffer(deviceContext, binding->Slot, data, dataSize);
}

bool UShader::UpdateTransformConstant(ID3D11DeviceContext* deviceContext, FVector3 offset, float scale)
{
	FTransformConstants constants = {
		.Offset = offset,
		.Scale = scale
	};

	if (UpdateConstantBuffer(deviceContext, "constants", constants))
	{
		return true;
	}

	return UpdateConstantBuffer(deviceContext, 0, constants);
}

void UShader::UpdateConstant(ID3D11DeviceContext* deviceContext, FVector3 offset, float scale)
{
	UpdateTransformConstant(deviceContext, offset, scale);
}

bool UShader::CreateConstantBuffers(ID3D11Device* device, ID3DBlob* vsBlob, ID3DBlob* psBlob)
{
	if (vsBlob != nullptr && !ReflectConstantBuffers(device, vsBlob, ShaderStageVertex))
	{
		return false;
	}

	if (psBlob != nullptr && !ReflectConstantBuffers(device, psBlob, ShaderStagePixel))
	{
		return false;
	}

	return true;
}

bool UShader::ReflectConstantBuffers(ID3D11Device* device, ID3DBlob* shaderBlob, UINT shaderStage)
{
	if (device == nullptr || shaderBlob == nullptr)
	{
		return false;
	}

	ID3D11ShaderReflection* reflection = nullptr;
	HRESULT hr = D3DReflect(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		reinterpret_cast<void**>(&reflection)
	);
	if (FAILED(hr))
	{
		return false;
	}

	D3D11_SHADER_DESC shaderDesc = {};
	hr = reflection->GetDesc(&shaderDesc);
	if (FAILED(hr))
	{
		SafeRelease(reflection);
		return false;
	}

	for (UINT resourceIndex = 0; resourceIndex < shaderDesc.BoundResources; ++resourceIndex)
	{
		D3D11_SHADER_INPUT_BIND_DESC bindDesc = {};
		hr = reflection->GetResourceBindingDesc(resourceIndex, &bindDesc);
		if (FAILED(hr))
		{
			SafeRelease(reflection);
			return false;
		}

		if (bindDesc.Type != D3D_SIT_CBUFFER)
		{
			continue;
		}

		ID3D11ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByName(bindDesc.Name);
		if (constantBuffer == nullptr)
		{
			SafeRelease(reflection);
			return false;
		}

		D3D11_SHADER_BUFFER_DESC bufferDesc = {};
		hr = constantBuffer->GetDesc(&bufferDesc);
		if (FAILED(hr))
		{
			SafeRelease(reflection);
			return false;
		}

		if (!CreateConstantBuffer(device, bindDesc.Name, bindDesc.BindPoint, bufferDesc.Size, shaderStage))
		{
			SafeRelease(reflection);
			return false;
		}
	}

	SafeRelease(reflection);
	return true;
}

bool UShader::CreateConstantBuffer(ID3D11Device* device, const std::string& bufferName, UINT slot, UINT sizeInBytes, UINT shaderStages)
{
	if (device == nullptr || sizeInBytes == 0u)
	{
		return false;
	}

	const UINT alignedSize = AlignConstantBufferSize(sizeInBytes);
	FConstantBufferBinding* existingBinding = FindConstantBuffer(slot);
	if (existingBinding != nullptr)
	{
		existingBinding->ShaderStages |= shaderStages;
		if (existingBinding->Name.empty())
		{
			existingBinding->Name = bufferName;
		}

		if (existingBinding->SizeInBytes >= sizeInBytes)
		{
			return true;
		}

		SafeRelease(existingBinding->Buffer);
		existingBinding->SizeInBytes = sizeInBytes;

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = alignedSize;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &existingBinding->Buffer);
		return SUCCEEDED(hr);
	}

	FConstantBufferBinding binding = {
		.Name = bufferName,
		.Slot = slot,
		.SizeInBytes = sizeInBytes,
		.ShaderStages = shaderStages,
		.Buffer = nullptr
	};

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = alignedSize;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &binding.Buffer);
	if (FAILED(hr))
	{
		return false;
	}

	ConstantBuffers.push_back(binding);
	return true;
}

UShader::FConstantBufferBinding* UShader::FindConstantBuffer(UINT slot)
{
	for (FConstantBufferBinding& binding : ConstantBuffers)
	{
		if (binding.Slot == slot)
		{
			return &binding;
		}
	}

	return nullptr;
}

const UShader::FConstantBufferBinding* UShader::FindConstantBuffer(UINT slot) const
{
	for (const FConstantBufferBinding& binding : ConstantBuffers)
	{
		if (binding.Slot == slot)
		{
			return &binding;
		}
	}

	return nullptr;
}

UShader::FConstantBufferBinding* UShader::FindConstantBuffer(const std::string& bufferName)
{
	for (FConstantBufferBinding& binding : ConstantBuffers)
	{
		if (binding.Name == bufferName)
		{
			return &binding;
		}
	}

	return nullptr;
}

const UShader::FConstantBufferBinding* UShader::FindConstantBuffer(const std::string& bufferName) const
{
	for (const FConstantBufferBinding& binding : ConstantBuffers)
	{
		if (binding.Name == bufferName)
		{
			return &binding;
		}
	}

	return nullptr;
}

UINT UShader::AlignConstantBufferSize(UINT sizeInBytes)
{
	return (sizeInBytes + 15u) & ~15u;
}
