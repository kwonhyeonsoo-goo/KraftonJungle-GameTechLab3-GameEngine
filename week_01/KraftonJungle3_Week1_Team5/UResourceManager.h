#pragma once
#include <d3d11.h>
#include <memory>
#include <unordered_map>

#include "UPrimitive.h"
#include "UShader.h"
#include "UTexture2D.h"

class UResourceManager : public UPrimitive
{
public:
	UResourceManager();
	~UResourceManager() override;

	bool Initialize(ID3D11Device* device);
	void Release();

	UTexture2D* LoadTexture(const std::wstring& filePath);

	//UShader* LoadShader()

	UTexture2D* FindTexture(const std::wstring& filePath) const;
	// UShader* FindShader(const FShaderKey& shaderKey) const;

private:
	ID3D11Device* Device;
	std::unordered_map<std::wstring, std::unique_ptr<UTexture2D>> TextureMap;
	// std::unordered_map<FShaderKey, std::unique_ptr<UShader>m FShaderKeyHasher> ShaderMap;
};

