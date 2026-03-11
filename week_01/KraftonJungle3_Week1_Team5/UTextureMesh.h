#pragma once

#include "UMesh.h"

class UTextureMesh : public UMesh
{
public:
	UTextureMesh() = default;
	~UTextureMesh() override = default;

	bool CreateRect(ID3D11Device* device);
	bool CreateRect(ID3D11Device* device, float halfWidthNdc, float halfHeightNdc);

};

