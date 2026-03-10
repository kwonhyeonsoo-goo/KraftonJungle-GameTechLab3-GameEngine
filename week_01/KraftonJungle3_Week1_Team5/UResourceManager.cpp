#include "UResourceManager.h"

UResourceManager::UResourceManager() : Device(nullptr)
{
}

UResourceManager::~UResourceManager()
{
	Release();
}

bool UResourceManager::Initialize(ID3D11Device* device)
{
	Device = device;
	return Device != nullptr;
}

void UResourceManager::Release()
{
	//ShaderMap.clear();
	TextureMap.clear();
	Device = nullptr;
}

UTexture2D* UResourceManager::LoadTexture(const std::wstring& filePath)
{
	if (Device == nullptr)
	{
		return nullptr;
	}

	auto it = TextureMap.find(filePath);
	if (it != TextureMap.end())
	{
		return it->second.get();
	}

	std::unique_ptr<UTexture2D> texture = std::make_unique<UTexture2D>();
	if (!texture->CreateFromFile(Device, filePath))
	{
		return nullptr;
	}

	UTexture2D* result = texture.get();
	TextureMap.emplace(filePath, std::move(texture));
	return result;
}

UTexture2D* UResourceManager::FindTexture(const std::wstring& filePath) const
{
	auto it = TextureMap.find(filePath);
	if (it == TextureMap.end())
	{
		return nullptr;
	}

	return it->second.get();
}
