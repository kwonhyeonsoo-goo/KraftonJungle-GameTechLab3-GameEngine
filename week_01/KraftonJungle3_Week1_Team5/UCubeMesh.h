#pragma once
#include <DirectXMath.h>

#include "UMesh.h"
class UCubeMesh : public UMesh
{
public:
	UCubeMesh() = default;
	~UCubeMesh() override = default;

	bool CreateCube(ID3D11Device* device);

	float GetSize() const { return Size; }

private:
	float Size;
};

