#pragma once
#include "UMesh.h"
class URectMesh : public UMesh
{
public:
	URectMesh() = default;
	~URectMesh() override = default;

	bool CreateRect(ID3D11Device* device);

	//TODO eric1306 width, height?
};

