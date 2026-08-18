#include "AssetCooker.h"
#include "CookedAsset.h"
#include "Core/Paths.h"
#include "Debug/EngineLog.h"
#include "Mesh/ObjImporter.h"
#include "Mesh/ObjInfo.h"
#include "Mesh/ObjManager.h"
#include "Mesh/StaticMeshRenderData.h"
#include "Renderer/PrimitiveVertex.h"
#include "Primitive/PrimitiveBase.h"

#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// ──────────────────────────────────────────────
// 유틸리티
// ──────────────────────────────────────────────

uint64 FAssetCooker::ComputeFileHash(const FString& AbsolutePath)
{
	std::ifstream File(FPaths::ToWide(AbsolutePath), std::ios::binary | std::ios::ate);
	if (!File.is_open()) return 0;

	std::streamsize Size = File.tellg();
	File.seekg(0, std::ios::beg);

	std::vector<char> Buffer(static_cast<size_t>(Size));
	if (!File.read(Buffer.data(), Size)) return 0;

	// FNV-1a 64bit
	uint64 Hash = 14695981039346656037ULL;
	for (size_t i = 0; i < Buffer.size(); ++i)
	{
		Hash ^= static_cast<uint64>(static_cast<uint8>(Buffer[i]));
		Hash *= 1099511628211ULL;
	}
	return Hash;
}

uint64 FAssetCooker::GetFileTimestamp(const FString& AbsolutePath)
{
	std::error_code EC;
	auto Time = fs::last_write_time(FPaths::ToWide(AbsolutePath), EC);
	if (EC) return 0;
	return static_cast<uint64>(Time.time_since_epoch().count());
}

void FAssetCooker::EnsureDirectoryExists(const FString& FilePath)
{
	fs::path Dir = fs::path(FPaths::ToWide(FilePath)).parent_path();
	if (!Dir.empty() && !fs::exists(Dir))
	{
		fs::create_directories(Dir);
	}
}

FString FAssetCooker::GetCookedPath(const FString& SourceRelativePath)
{
	// "Assets/Meshes/Cat.obj" → "Intermediate/Cooked/Assets/Meshes/Cat.dasset"

	FString RelPath = FPaths::ToRelativePath(SourceRelativePath);
	FString Cooked = "Intermediate/Cooked/" + RelPath;
	size_t DotPos = Cooked.rfind('.');
	if (DotPos != std::string::npos)
		Cooked = Cooked.substr(0, DotPos) + ".dasset";
	return FPaths::ToAbsolutePath(Cooked);
}

bool FAssetCooker::SaveCookedStaticMesh(const FStaticMeshRenderData* RenderData, const FString& SourcePath, const FString& OutputPath)
{
	FString AbsSource = FPaths::ToAbsolutePath(SourcePath);

	const FMeshData* MD = RenderData->GetMeshData();
	if (!MD) return false;

	EnsureDirectoryExists(OutputPath);

	std::ofstream Out(FPaths::ToWide(OutputPath), std::ios::binary | std::ios::trunc);
	if (!Out.is_open()) return false;

	uint32 VertexCount = static_cast<uint32>(MD->Vertices.size());
	uint32 IndexCount = static_cast<uint32>(MD->Indices.size());
	uint32 SectionCount = static_cast<uint32>(MD->Sections.size());

	std::vector<std::string> TexPaths(SectionCount);
	for (uint32 i = 0; i < SectionCount; ++i)
	{
		if (i < RenderData->ImportedTexturePaths.size())
			TexPaths[i] = RenderData->ImportedTexturePaths[i];
	}

	FCookedAssetHeader Header{};
	Header.Magic = DINO_COOKED_MAGIC;
	Header.Version = COOKED_STATIC_MESH_VERSION;
	Header.AssetType = static_cast<uint32>(ECookedAssetType::StaticMesh);
	Header.SourceHash = ComputeFileHash(AbsSource);
	Header.SourceTimestamp = GetFileTimestamp(AbsSource);
	Header.DataOffset = sizeof(FCookedAssetHeader);
	Header.DataSize = 0;
	memset(Header.Reserved, 0, sizeof(Header.Reserved));

	auto HeaderPos = Out.tellp();
	Out.write(reinterpret_cast<const char*>(&Header), sizeof(Header));

	auto DataStart = Out.tellp();

	Out.write(reinterpret_cast<const char*>(&VertexCount), sizeof(uint32));
	Out.write(reinterpret_cast<const char*>(&IndexCount), sizeof(uint32));
	Out.write(reinterpret_cast<const char*>(&SectionCount), sizeof(uint32));

	uint32 Topo = static_cast<uint32>(MD->Topology);
	Out.write(reinterpret_cast<const char*>(&Topo), sizeof(uint32));

	FVector MinC = MD->GetMinCoord();
	FVector MaxC = MD->GetMaxCoord();
	float Radius = MD->GetLocalBoundRadius();
	Out.write(reinterpret_cast<const char*>(&MinC), sizeof(FVector));
	Out.write(reinterpret_cast<const char*>(&MaxC), sizeof(FVector));
	Out.write(reinterpret_cast<const char*>(&Radius), sizeof(float));

	Out.write(reinterpret_cast<const char*>(MD->Vertices.data()),
		VertexCount * sizeof(FPrimitiveVertex));

	Out.write(reinterpret_cast<const char*>(MD->Indices.data()),
		IndexCount * sizeof(uint32));

	for (uint32 i = 0; i < SectionCount; ++i)
	{
		FCookedMeshSection CS{};
		CS.FirstIndex = MD->Sections[i].FirstIndex;
		CS.IndexCount = MD->Sections[i].IndexCount;
		CS.MaterialIndex = MD->Sections[i].MaterialIndex;
		CS.TexturePathLength = static_cast<uint32>(TexPaths[i].size());

		Out.write(reinterpret_cast<const char*>(&CS), sizeof(FCookedMeshSection));

		if (CS.TexturePathLength > 0)
		{
			Out.write(TexPaths[i].data(), CS.TexturePathLength);
		}

		FVector Color(1.0f, 1.0f, 1.0f);
		if (i < RenderData->ImportedDiffuseColors.size())
		{
			Color = RenderData->ImportedDiffuseColors[i];
		}
		Out.write(reinterpret_cast<const char*>(&Color), sizeof(FVector));
	}

	auto DataEnd = Out.tellp();
	Header.DataSize = static_cast<uint32>(DataEnd - DataStart);
	Out.seekp(HeaderPos);
	Out.write(reinterpret_cast<const char*>(&Header), sizeof(Header));

	Out.close();

	UE_LOG("[AssetCooker] Saved cooked: %s (%u verts, %u indices)\n",
		OutputPath.c_str(), VertexCount, IndexCount);
	return true;
}

// ──────────────────────────────────────────────
// 변경 감지
// ──────────────────────────────────────────────

bool FAssetCooker::NeedsCook(const FString& SourcePath, const FString& CookedPath)
{
	FString AbsSource = FPaths::ToAbsolutePath(SourcePath);

	// 1) 쿡 파일이 없으면 무조건 쿡
	if (!fs::exists(FPaths::ToWide(CookedPath)))
	{
		return true;
	}

	// 2) 헤더만 읽어서 비교
	std::ifstream File(FPaths::ToWide(CookedPath), std::ios::binary);
	if (!File.is_open()) return true;

	FCookedAssetHeader Header{};
	File.read(reinterpret_cast<char*>(&Header), sizeof(Header));
	if (File.gcount() != sizeof(Header)) return true;

	// 매직 넘버 검증
	if (Header.Magic != DINO_COOKED_MAGIC) return true;

	// 버전이 다르면 재쿡 (구버전이든 미래버전이든)
	if (Header.Version != COOKED_STATIC_MESH_VERSION) return true;

	// 3) 타임스탬프 비교 (빠른 경로)
	uint64 CurrentTimestamp = GetFileTimestamp(AbsSource);
	if (CurrentTimestamp == Header.SourceTimestamp)
	{
		return false; // 타임스탬프 동일 → 변경 없음
	}

	// 4) 타임스탬프 다름 → 해시로 정밀 비교
	//    (파일 복사 등으로 타임스탬프만 바뀌었을 수 있음)
	uint64 CurrentHash = ComputeFileHash(AbsSource);
	if (CurrentHash == Header.SourceHash)
	{
		return false; // 내용 동일 → 타임스탬프만 바뀐 것
	}

	return true; // 내용이 실제로 바뀜 → 재쿡
}

// ──────────────────────────────────────────────
// Cook: OBJ → .dasset 바이너리 쓰기
// ──────────────────────────────────────────────

bool FAssetCooker::CookStaticMesh(const FString& ObjSourcePath, const FString& OutputPath)
{
	FObjInfo Info;
	if (!FObjImporter::ParseObj(ObjSourcePath, Info))
	{
		UE_LOG("[AssetCooker] ParseObj failed: %s\n", ObjSourcePath.c_str());
		return false;
	}

	FStaticMeshRenderData* RenderData = FObjImporter::Cook(Info);
	if (!RenderData)
	{
		UE_LOG("[AssetCooker] Cook failed: %s\n", ObjSourcePath.c_str());
		return false;
	}

	bool Result = SaveCookedStaticMesh(RenderData, ObjSourcePath, OutputPath);
	delete RenderData;
	return Result;
}

// ──────────────────────────────────────────────
// Load: .dasset → FStaticMeshRenderData (파싱 없이 바이너리 읽기)
// ──────────────────────────────────────────────

FStaticMeshRenderData* FAssetCooker::LoadCookedStaticMesh(const FString& CookedPath)
{
	std::ifstream In(FPaths::ToWide(CookedPath), std::ios::binary);
	if (!In.is_open()) return nullptr;

	// ── 헤더 읽기 ──
	FCookedAssetHeader Header{};
	In.read(reinterpret_cast<char*>(&Header), sizeof(Header));

	if (Header.Magic != DINO_COOKED_MAGIC)
	{
		UE_LOG("[AssetCooker] Invalid magic in: %s\n", CookedPath.c_str());
		return nullptr;
	}

	if (Header.Version != COOKED_STATIC_MESH_VERSION)
	{
		// 구버전 또는 미래 버전 → 재쿡 필요 (호출자가 처리)
		UE_LOG("[AssetCooker] Version mismatch (file=%u, current=%u): %s\n",
			Header.Version, COOKED_STATIC_MESH_VERSION, CookedPath.c_str());
		return nullptr;
	}

	// ── 데이터 영역으로 이동 ──
	In.seekg(Header.DataOffset, std::ios::beg);

	// ── 메시 기본 정보 ──
	uint32 VertexCount = 0, IndexCount = 0, SectionCount = 0, Topo = 0;
	In.read(reinterpret_cast<char*>(&VertexCount), sizeof(uint32));
	In.read(reinterpret_cast<char*>(&IndexCount), sizeof(uint32));
	In.read(reinterpret_cast<char*>(&SectionCount), sizeof(uint32));
	In.read(reinterpret_cast<char*>(&Topo), sizeof(uint32));

	FVector MinC, MaxC;
	float Radius = 0.f;
	In.read(reinterpret_cast<char*>(&MinC), sizeof(FVector));
	In.read(reinterpret_cast<char*>(&MaxC), sizeof(FVector));
	In.read(reinterpret_cast<char*>(&Radius), sizeof(float));

	// ── MeshData 복원 ──
	auto MeshData = std::make_shared<FMeshData>();
	MeshData->Topology = static_cast<EMeshTopology>(Topo);

	// Vertex 배열 통째로 읽기
	MeshData->Vertices.resize(VertexCount);
	In.read(reinterpret_cast<char*>(MeshData->Vertices.data()),
		VertexCount * sizeof(FPrimitiveVertex));

	// Index 배열 통째로 읽기
	MeshData->Indices.resize(IndexCount);
	In.read(reinterpret_cast<char*>(MeshData->Indices.data()),
		IndexCount * sizeof(uint32));

	// Bound는 직접 세팅 못하니 UpdateLocalBound() 호출
	// (MinCoord/MaxCoord가 private이라서)
	MeshData->UpdateLocalBound();

	// ── RenderData 생성 ──
	FStaticMeshRenderData* Result = new FStaticMeshRenderData();
	Result->SetMeshData(MeshData);

	// ── Section + 텍스처 + 색상 ──
	for (uint32 i = 0; i < SectionCount; ++i)
	{
		FCookedMeshSection CS{};
		In.read(reinterpret_cast<char*>(&CS), sizeof(FCookedMeshSection));

		FMeshSection Sec;
		Sec.FirstIndex = CS.FirstIndex;
		Sec.IndexCount = CS.IndexCount;
		Sec.MaterialIndex = CS.MaterialIndex;
		MeshData->Sections.push_back(Sec);

		// 텍스처 경로
		FString TexPath;
		if (CS.TexturePathLength > 0)
		{
			TexPath.resize(CS.TexturePathLength);
			In.read(TexPath.data(), CS.TexturePathLength);
		}
		Result->ImportedTexturePaths.push_back(TexPath);

		// Diffuse Color
		FVector Color;
		In.read(reinterpret_cast<char*>(&Color), sizeof(FVector));
		Result->ImportedDiffuseColors.push_back(Color);
	}

	UE_LOG("[AssetCooker] Loaded cooked: %s (%u verts)\n", CookedPath.c_str(), VertexCount);
	return Result;
}
