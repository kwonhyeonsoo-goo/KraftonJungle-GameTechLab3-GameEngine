#include "ShaderMap.h"
#include "Shader.h"
#include "ShaderResource.h"

FShaderMap& FShaderMap::Get()
{
	static FShaderMap Instance;
	return Instance;
}

std::shared_ptr<FVertexShader> FShaderMap::GetOrCreateVertexShader(
	ID3D11Device* Device,
	const wchar_t* FilePath)
{
	std::wstring Key(FilePath);

	auto It = VertexShaders.find(Key);
	if (It != VertexShaders.end())
	{
		return It->second;
	}

	auto Resource = FShaderResource::GetOrCompile(FilePath, "main", "vs_5_0");
	if (!Resource)
	{
		return nullptr;
	}

	auto VS = FVertexShader::Create(Device, Resource);
	if (!VS)
	{
		return nullptr;
	}

	VertexShaders[Key] = VS;
	return VS;
}

std::shared_ptr<FPixelShader> FShaderMap::GetOrCreatePixelShader(
	ID3D11Device* Device,
	const wchar_t* FilePath)
{
	std::wstring Key(FilePath);

	auto It = PixelShaders.find(Key);
	if (It != PixelShaders.end())
	{
		return It->second;
	}

	auto Resource = FShaderResource::GetOrCompile(FilePath, "main", "ps_5_0");
	if (!Resource)
	{
		return nullptr;
	}

	auto PS = FPixelShader::Create(Device, Resource);
	if (!PS)
	{
		return nullptr;
	}

	PixelShaders[Key] = PS;
	return PS;
}

std::shared_ptr<FVertexShader> FShaderMap::GetOrCreateVertexShaderWithLayout(ID3D11Device* Device, const wchar_t* FilePath, const D3D11_INPUT_ELEMENT_DESC* Layout, UINT LayoutCount)
{
	// 인스턴싱 레이아웃을 가진 셰이더는 일반 셰이더와 구분하기 위해 키를 다르게 씁니다.
	std::wstring Key = std::wstring(FilePath) + L"_Instanced";

	auto It = VertexShaders.find(Key);
	if (It != VertexShaders.end())
	{
		return It->second;
	}

	auto Resource = FShaderResource::GetOrCompile(FilePath, "main", "vs_5_0");
	if (!Resource) return nullptr;

	auto VS = FVertexShader::CreateWithLayout(Device, Resource, Layout, LayoutCount);
	if (!VS) return nullptr;

	VertexShaders[Key] = VS;
	return VS;
}

void FShaderMap::Clear()
{
	VertexShaders.clear();
	PixelShaders.clear();
	FShaderResource::ClearCache();
}
