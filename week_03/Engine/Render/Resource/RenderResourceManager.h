#pragma once
#include "Engine/Core/CoreTypes.h"
#include "Engine/Render/Common/RenderTypes.h"

class FD3DDevice;

struct TextureData
{
	ID3D11Texture2D* Texture;
	ID3D11ShaderResourceView* ShaderResourceView;
};


class FRenderResourceManager
{
public:
	void Create(FD3DDevice* device);
	void Release();
	TextureData GetTexture(int id) { return TextureMap.at(id); }
	int CreateTexture(std::wstring filepath);
	int GetTextureSize() { return TextureMap.size(); }
	int GetDefaultTexture() { return TextureMap.begin()->first; }
	static std::wstring LoadResourceFilePath();
private:
	TMap<int, TextureData> TextureMap; //id, 텍스쳐 맵
	TMap<std::wstring, int> FilePathMap; //file path, texture id
	uint8 nextID = 0;
	FD3DDevice* device;

	static std::wstring GetFileDialoguePath();
};

