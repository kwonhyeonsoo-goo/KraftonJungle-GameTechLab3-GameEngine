#pragma once
#include <string>

#include "UPrimitive.h"

enum class EResourceType
{
	None,
	Texture2D,
	Shader
};



class UResource : public UPrimitive
{
public:
	UResource() = default;
	~UResource() override = default;

	const std::wstring& GetPath() const { return Path; }


private:
	std::wstring Name;
	std::wstring Path;
	EResourceType Type = EResourceType::None;
	bool bLoaded = false;
};

