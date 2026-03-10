#pragma once
#include <DirectXMath.h>

#include "UMesh.h";

class UTextureMesh :public UMesh
{
public:
	UTextureMesh() = default;
	~UTextureMesh() override = default;

	bool CreateRect(ID3D11Device* device);

};

