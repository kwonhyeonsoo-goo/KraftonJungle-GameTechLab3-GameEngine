#include "URectMesh.h"
#include "Rect.h"

bool URectMesh::CreateRect(ID3D11Device* device)
{
	if (device == nullptr)
	{
		return false;
	}

	UINT numVerticesRect = sizeof(rect_vertices) / sizeof(FVertexColor);
	SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return CreateVertexBuffer(device, rect_vertices, sizeof(FVertexColor), numVerticesRect);
}
