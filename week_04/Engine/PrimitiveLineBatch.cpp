#include "PrimitiveLineBatch.h"

CPrimitiveLineBatch::CPrimitiveLineBatch()
{
	MeshData = std::make_shared<FMeshData>();
	MeshData->Topology = EMeshTopology::EMT_LineList;
}

uint32 CPrimitiveLineBatch::AddLine(FVector InStart, FVector InEnd, FVector4 InColor, uint32 InBatchID)
{
	uint32 meshDataSize = MeshData->Vertices.size();
	MeshData->Vertices.push_back({ InStart, InColor, FVector::ZeroVector });
	MeshData->Vertices.push_back({ InEnd, InColor, FVector::ZeroVector });
	MeshData->Indices.push_back(meshDataSize);
	MeshData->Indices.push_back(meshDataSize + 1);

	// TODO: batchID 구현 (추가한 batchLine 수동으로 삭제하는 용도)
	return InBatchID;
}
