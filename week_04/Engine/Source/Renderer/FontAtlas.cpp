#include "FontAtlas.h"
#include <WICTextureLoader.h>
#include <fstream>
#include <filesystem>

FFontAtlas::~FFontAtlas()
{
	Release();
}

bool FFontAtlas::Initialize(
	ID3D11Device* Device,
	ID3D11DeviceContext* DeviceContext,
	const std::wstring& TexturePath)
{
	Release();

	if (!Device || !DeviceContext || TexturePath.empty())
	{
		MessageBox(0, L"FontAtlas: invalid Device / DeviceContext / TexturePath", 0, 0);
		return false;
	}

	MessageBox(0, TexturePath.c_str(), L"FontAtlas TexturePath", 0);

	std::ifstream TestFile(std::filesystem::path(TexturePath), std::ios::binary);
	if (!TestFile.is_open())
	{
		MessageBox(0, TexturePath.c_str(), L"FontAtlas: std::ifstream open failed", 0);
		return false;
	}
	TestFile.close();

	HRESULT Hr = DirectX::CreateWICTextureFromFile(
		Device,
		DeviceContext,
		TexturePath.c_str(),
		nullptr,
		&TextureSRV
	);

	if (FAILED(Hr) || !TextureSRV)
	{
		wchar_t Buffer[256];
		swprintf_s(Buffer, L"FontAtlas: CreateWICTextureFromFile failed\nHRESULT = 0x%08X", Hr);
		MessageBox(0, Buffer, L"FontAtlas Error", 0);
		return false;
	}

	D3D11_SAMPLER_DESC SampDesc = {};
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.MipLODBias = 0.0f;
	SampDesc.MaxAnisotropy = 1;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampDesc.BorderColor[0] = 0.0f;
	SampDesc.BorderColor[1] = 0.0f;
	SampDesc.BorderColor[2] = 0.0f;
	SampDesc.BorderColor[3] = 0.0f;
	SampDesc.MinLOD = 0.0f;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Hr = Device->CreateSamplerState(&SampDesc, &SamplerState);
	if (FAILED(Hr) || !SamplerState)
	{
		MessageBox(0, L"FontAtlas: CreateSamplerState failed", 0, 0);
		Release();
		return false;
	}

	BuildGridAtlas128x128();
	return true;
}

void FFontAtlas::BuildGridAtlas128x128()
{
	const float CellU = 1.0f / static_cast<float>(CellsPerRow);
	const float CellV = 1.0f / static_cast<float>(Rows);

	for (int32 Index = 0; Index < GlyphCount; ++Index)
	{
		const int32 Row = Index / CellsPerRow;
		const int32 Col = Index % CellsPerRow;

		FFontGlyph& Glyph = Glyphs[Index];
		Glyph.U0 = static_cast<float>(Col) * CellU;
		Glyph.V0 = static_cast<float>(Row) * CellV;
		Glyph.U1 = static_cast<float>(Col + 1) * CellU;
		Glyph.V1 = static_cast<float>(Row + 1) * CellV;

		Glyph.Width = 1.0f;
		Glyph.Height = 1.0f;
		Glyph.Advance = 1.0f;
	}

	// 공백 U+0020
	if (32 < GlyphCount)
	{
		Glyphs[32].Width = 0.0f;
		Glyphs[32].Height = 0.0f;
		Glyphs[32].Advance = 0.35f;
	}
}

const FFontGlyph& FFontAtlas::GetGlyph(uint32 Codepoint) const
{
	static FFontGlyph Fallback;

	if (Codepoint >= GlyphCount)
	{
		return Fallback;
	}

	return Glyphs[Codepoint];
}

void FFontAtlas::Release()
{
	if (SamplerState)
	{
		SamplerState->Release();
		SamplerState = nullptr;
	}

	if (TextureSRV)
	{
		TextureSRV->Release();
		TextureSRV = nullptr;
	}
}