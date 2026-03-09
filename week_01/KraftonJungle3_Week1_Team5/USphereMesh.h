#pragma once
#include <DirectXMath.h>

#include "UMesh.h"
class USphereMesh : public UMesh
{
public:
	USphereMesh() = default;
	~USphereMesh() override = default;

	bool CreateSphere(ID3D11Device* device);

	float GetRadius() const { return Radius; }

private:
	float Radius;
};

