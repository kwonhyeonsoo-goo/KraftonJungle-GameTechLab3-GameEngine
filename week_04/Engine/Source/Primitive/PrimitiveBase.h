#pragma once

#include "CoreMinimal.h"
#include "Renderer/PrimitiveVertex.h"
#include <d3d11.h>

struct ENGINE_API FMeshData
{
	FMeshData() : SortId(NextSortId++) {}
	~FMeshData() { Release(); }

	uint32 GetSortId() const { return SortId; }

	bool CreateBuffers(ID3D11Device* Device);
	void Bind(ID3D11DeviceContext* Context);
	void Release();

	// CPU 데이터
	TArray<FPrimitiveVertex> Vertices;
	TArray<uint32> Indices;

	// GPU 버퍼
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	uint32 IndexCount = 0;

private:
	uint32 SortId = 0;
	static inline uint32 NextSortId = 0;
};

class ENGINE_API CPrimitiveBase
{
public:
	CPrimitiveBase() = default;
	virtual ~CPrimitiveBase() = default;

	FMeshData* GetMeshData() const { return MeshData.get(); }

	// 캐시에서 가져오거나 파일에서 로드
	static std::shared_ptr<FMeshData> GetOrLoad(const FString& Key, const FString& FilePath);
	// 캐시에서만 조회
	static std::shared_ptr<FMeshData> GetCached(const FString& Key);
	// 코드로 생성한 데이터를 캐시에 등록
	static void RegisterMeshData(const FString& Key, std::shared_ptr<FMeshData> Data);
	static void ClearCache();

protected:
	std::shared_ptr<FMeshData> MeshData;

private:
	static TMap<FString, std::shared_ptr<FMeshData>> MeshCache;

	static std::shared_ptr<FMeshData> LoadFromFile(const FString& FilePath);
};
