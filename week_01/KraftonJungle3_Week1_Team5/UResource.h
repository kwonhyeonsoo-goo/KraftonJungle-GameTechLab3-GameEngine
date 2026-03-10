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
	const std::wstring& GetName() const { return Name; }

	virtual EResourceType GetType() = 0;
	bool IsLoaded() const { return bLoaded; }

private:
	std::wstring Name;
	std::wstring Path;
	bool bLoaded = false;
};

