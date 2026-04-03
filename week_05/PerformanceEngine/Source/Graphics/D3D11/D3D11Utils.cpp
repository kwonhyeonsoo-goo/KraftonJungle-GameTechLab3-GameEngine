#include "Graphics/D3D11/D3D11Utils.h"

namespace
{
	void OutputShaderCompilerMessages(ID3DBlob* InErrorBlob)
	{
		if (InErrorBlob == nullptr || InErrorBlob->GetBufferPointer() == nullptr)
		{
			return;
		}

		OutputDebugStringA(static_cast<const char*>(InErrorBlob->GetBufferPointer()));
		OutputDebugStringA("\n");
	}

	void OutputCompileFailureHeader(const char* InDebugName)
	{
		OutputDebugStringA("[D3D11Utils] Failed to compile shader");
		if (InDebugName != nullptr && InDebugName[0] != '\0')
		{
			OutputDebugStringA(": ");
			OutputDebugStringA(InDebugName);
		}

		OutputDebugStringA("\n");
	}
}

namespace D3D11Utils
{
	bool CreateImmutableBuffer(
		ID3D11Device* InDevice,
		UINT InByteWidth,
		UINT InBindFlags,
		const void* InInitialData,
		TComPtr<ID3D11Buffer>& OutBuffer)
	{
		if (InDevice == nullptr || InInitialData == nullptr)
		{
			return false;
		}

		const D3D11_BUFFER_DESC BufferDesc =
		{
			InByteWidth,
			D3D11_USAGE_IMMUTABLE,
			InBindFlags,
			0,
			0,
			0
		};

		const D3D11_SUBRESOURCE_DATA InitialData =
		{
			InInitialData,
			0,
			0
		};

		return SUCCEEDED(InDevice->CreateBuffer(&BufferDesc, &InitialData, OutBuffer.GetAddressOf()));
	}

	bool CreateDynamicConstantBuffer(
		ID3D11Device* InDevice,
		UINT InByteWidth,
		TComPtr<ID3D11Buffer>& OutBuffer)
	{
		if (InDevice == nullptr)
		{
			return false;
		}

		const D3D11_BUFFER_DESC BufferDesc =
		{
			InByteWidth,
			D3D11_USAGE_DYNAMIC,
			D3D11_BIND_CONSTANT_BUFFER,
			D3D11_CPU_ACCESS_WRITE,
			0,
			0
		};

		return SUCCEEDED(InDevice->CreateBuffer(&BufferDesc, nullptr, OutBuffer.GetAddressOf()));
	}

	bool CompileShaderFromFile(
		const std::filesystem::path& InShaderPath,
		const char* InEntryPoint,
		const char* InTarget,
		TComPtr<ID3DBlob>& OutBlob,
		const char* InDebugName)
	{
		if (InShaderPath.empty())
		{
			return false;
		}

		UINT CompileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		CompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		TComPtr<ID3DBlob> ErrorBlob;
		const HRESULT Result = D3DCompileFromFile(
			InShaderPath.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			InEntryPoint,
			InTarget,
			CompileFlags,
			0,
			OutBlob.GetAddressOf(),
			ErrorBlob.GetAddressOf());

		if (FAILED(Result))
		{
			OutputCompileFailureHeader(InDebugName);
			OutputDebugStringW(InShaderPath.c_str());
			OutputDebugStringA("\n");
			OutputShaderCompilerMessages(ErrorBlob.Get());
			return false;
		}

		return true;
	}

	bool CompileShaderFromSource(
		const char* InSource,
		const char* InEntryPoint,
		const char* InTarget,
		TComPtr<ID3DBlob>& OutBlob,
		const char* InDebugName)
	{
		if (InSource == nullptr)
		{
			return false;
		}

		UINT CompileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		CompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		TComPtr<ID3DBlob> ErrorBlob;
		const HRESULT Result = D3DCompile(
			InSource,
			std::strlen(InSource),
			nullptr,
			nullptr,
			nullptr,
			InEntryPoint,
			InTarget,
			CompileFlags,
			0,
			OutBlob.GetAddressOf(),
			ErrorBlob.GetAddressOf());

		if (FAILED(Result))
		{
			OutputCompileFailureHeader(InDebugName);
			OutputShaderCompilerMessages(ErrorBlob.Get());
			return false;
		}

		return true;
	}
}
