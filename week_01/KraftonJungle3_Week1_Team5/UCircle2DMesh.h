#pragma once
#include <DirectXMath.h>

#include "UMesh.h"

class UCircle2DMesh : public UMesh
{
public:
	//struct FVertexColor
	//{
	//	float x, y, z;
	//	float r, g, b, a;
	//};

	UCircle2DMesh();
	~UCircle2DMesh() override = default;

	bool CreateCircle(ID3D11Device* device, float radius, UINT segmentCount, DirectX::XMFLOAT4 color = {0.f, 1.f, 0.f, 1.f});

	float GetRadius() const { return Radius; }
	
private:
	float Radius;
	UINT SegmentCount;
};