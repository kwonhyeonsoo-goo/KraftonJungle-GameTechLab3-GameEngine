#pragma once
#include "Types/Map.h"
#include "Types/Array.h"
#include "Types/String.h"
#include "Renderer/Mesh/StaticMeshRenderData.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/ShaderMap.h"
#include "Core/Paths.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include "../ThirdParty/stb_image.h"
#include "Renderer/RenderState.h"
#include "Core/Templates/ObjectIterator.h"
#include "d3d11.h"
#include "Object/Mesh/StaticMesh.h"
#include "Object/ObjectFactory.h"

#include "Primitive/PrimitiveObj.h"
#include "Object/Class.h"
#include "Renderer/PrimitiveVertex.h"
#include "Asset/AssetManager.h"
#include "d3d11.h"
#include "Asset/FWindowsIO.h"
//TODO : Delete These Things
#include "Debug/EngineLog.h"
#include <chrono>


struct FTexture;
class UStaticMesh;

// =========================
// OBJ intermediate structs
// =========================

struct FObjVertexRef
{
	int32 V = -1;   // position index
	int32 VT = -1;  // uv index
	int32 VN = -1;  // normal index

	bool operator==(const FObjVertexRef& Other) const
	{
		return V == Other.V && VT == Other.VT && VN == Other.VN;
	}
};

struct FObjVertexRefHasher
{
	size_t operator()(const FObjVertexRef& Ref) const
	{
		size_t H1 = std::hash<int32>{}(Ref.V);
		size_t H2 = std::hash<int32>{}(Ref.VT);
		size_t H3 = std::hash<int32>{}(Ref.VN);
		return H1 ^ (H2 << 1) ^ (H3 << 2);
	}
};

struct FObjFaceContext
{
	FString GroupName;
	FString ObjectName;
	FString MaterialName;
	FString SmoothingGroup;
};

struct FObjFace
{
	TArray<FObjVertexRef> VertexRefs;
	FObjFaceContext Context;
};


struct FObjMaterialInfo
{
	FString Name; //newmtl dorumon에서 dorumon을 뜻함
	FVector Ka = FVector(0.0f, 0.0f, 0.0f);
	FVector Kd = FVector(1.0f, 1.0f, 1.0f);
	FVector Ks = FVector(0.0f, 0.0f, 0.0f);
	FVector Ke = FVector(0.0f, 0.0f, 0.0f);
	float Ns = 0.0f;
	float Ni = 1.0f;
	float D = 1.0f;
	int32 Illum = 0;
	FString MapKd;
};

struct FObjInfo
{
	FString Mtllib; //mtl 파일 이름  EX)Dorumon.mtl
	TArray<FVector> Positions;
	TArray<FVector2> UVs;
	TArray<FVector> Normals;
	TArray<FObjFace> Faces;
	//각 face가 어떤 vertex를 참조하는지, 그리고 그 face의 group, object, material, smoothing group이 무엇인지
	TArray<FObjMaterialInfo> Materials; //obj가 쓰는 Material이 어떤게 있는지
};

// =========================
// OBJ Importer
// =========================

struct FObjImporter
{
private:
	static int32 ResolveObjIndex(int32 RawIndex, int32 Count)
	{
		if (RawIndex > 0)
		{
			return RawIndex - 1;
		}
		else if (RawIndex < 0)
		{
			return Count + RawIndex;
		}
		return -1;
	}

	static FObjVertexRef ParseFaceVertex(
		const FString& Token,
		int32 PositionCount,
		int32 UVCount,
		int32 NormalCount)
	{
		FObjVertexRef Ref;

		std::stringstream SS(Token);
		FString A, B, C;

		std::getline(SS, A, '/');
		std::getline(SS, B, '/');
		std::getline(SS, C, '/');

		if (!A.empty())
		{
			Ref.V = ResolveObjIndex(std::stoi(A), PositionCount);
		}
		if (!B.empty())
		{
			Ref.VT = ResolveObjIndex(std::stoi(B), UVCount);
		}
		if (!C.empty())
		{
			Ref.VN = ResolveObjIndex(std::stoi(C), NormalCount);
		}

		return Ref;
	}

	static bool IsValidIndex(int32 Index, int32 Count)
	{
		return Index >= 0 && Index < Count;
	}

public:
	static FObjInfo LoadObjFile(const FString& PathFileName)
	{

		//TODO : Delete These Time;
		auto StartTime = std::chrono::high_resolution_clock::now();

		FString AbsolutePath = FPaths::ToAbsolutePath(PathFileName);
		std::filesystem::path path = FPaths::ToU8String(AbsolutePath);

		std::ifstream File(path);
		if (!File.is_open())
		{
			printf("[OBJ] Failed to open : %s\n", PathFileName.c_str());
			return {};
		}

		FObjInfo ObjInfo;

		FString CurrentGroup;
		FString CurrentObject;
		FString CurrentMaterial;
		FString CurrentSmoothing;

		FString Line;
		while (std::getline(File, Line))
		{
			if (Line.empty())
				continue;

			if (!Line.empty() && Line.back() == '\r')
			{
				Line.pop_back();
			}

			std::stringstream SS(Line);
			FString Type;
			SS >> Type;

			if (Type.empty() || Type[0] == '#')
				continue;

			if (Type == "v")
			{
				FVector P;
				SS >> P.X >> P.Y >> P.Z;
				ObjInfo.Positions.push_back(P);
			}
			else if (Type == "vt")
			{
				FVector2 UV;
				SS >> UV.X >> UV.Y;
				UV.Y = 1.0f - UV.Y;
				ObjInfo.UVs.push_back(UV);
			}
			else if (Type == "vn")
			{
				FVector N;
				SS >> N.X >> N.Y >> N.Z;
				ObjInfo.Normals.push_back(N);
			}
			else if (Type == "f")
			{
				FObjFace Face;
				Face.Context.GroupName = CurrentGroup;
				Face.Context.ObjectName = CurrentObject;
				Face.Context.MaterialName = CurrentMaterial;
				Face.Context.SmoothingGroup = CurrentSmoothing;

				FString VertexToken;
				while (SS >> VertexToken)
				{
					Face.VertexRefs.push_back(ParseFaceVertex(
						VertexToken,
						static_cast<int32>(ObjInfo.Positions.size()),
						static_cast<int32>(ObjInfo.UVs.size()),
						static_cast<int32>(ObjInfo.Normals.size())
					));
				}

				if (Face.VertexRefs.size() >= 3)
				{
					ObjInfo.Faces.push_back(std::move(Face));
				}
			}
			else if (Type == "mtllib")
			{
				FString LibName;
				std::getline(SS >> std::ws, LibName);
				ObjInfo.Mtllib = LibName;
				std::filesystem::path fullPath = FPaths::MeshDir() / FPaths::ToU8String(LibName);
				TArray<FObjMaterialInfo> MatInfos = LoadMtlFile(FPaths::FromPath(fullPath));
				for (FObjMaterialInfo& MatInfo : MatInfos)
				{
					if (!MatInfo.Name.empty())
					{
						ObjInfo.Materials.push_back(MatInfo);
					}
					else
					{
						printf("[OBJ] Failed to load MTL file : %s\n", LibName.c_str());
					}
				}
			}
			else if (Type == "usemtl")
			{
				std::getline(SS >> std::ws, CurrentMaterial);
			}
			else if (Type == "g")
			{
				std::getline(SS >> std::ws, CurrentGroup);
			}
			else if (Type == "o")
			{
				std::getline(SS >> std::ws, CurrentObject);
			}
			else if (Type == "s")
			{
				SS >> CurrentSmoothing;
			}
		}

		// [2] 종료 시간 기록 및 차이 계산
		auto EndTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> Elapsed = EndTime - StartTime;

		// [3] 로그 출력 (초 단위)
		UE_LOG("[OBJ] Load Success: %s\n", PathFileName.c_str());
		UE_LOG("[OBJ] Execution Time: %.6f seconds\n", Elapsed.count());
		return ObjInfo;
	}

	static TArray<FObjMaterialInfo> LoadMtlFile(const FString& PathFileName)
	{
		auto StartTime = std::chrono::high_resolution_clock::now();


		FString AbsolutePath = FPaths::ToAbsolutePath(PathFileName);
		std::filesystem::path path = FPaths::ToU8String(AbsolutePath);
		std::ifstream File(path);
		if (!File.is_open())
		{
			printf("[MTL] Failed to open : %s\n", PathFileName.c_str());
			return {};
		}
		TArray<FObjMaterialInfo> Materials;
		FString Line;
		FObjMaterialInfo MaterialInfo;
		bool bHasMaterial = false;
		while (std::getline(File, Line))
		{
			if (Line.empty())
				continue;
			if (!Line.empty() && Line.back() == '\r')
			{
				Line.pop_back();
			}
			std::stringstream SS(Line);
			FString Type;
			SS >> Type;
			if (Type.empty() || Type[0] == '#')
				continue;
			if (Type == "newmtl")
			{
				if (bHasMaterial)
				{
					Materials.push_back(MaterialInfo);
				}
				MaterialInfo = FObjMaterialInfo();
				bHasMaterial = true;
				std::getline(SS >> std::ws, MaterialInfo.Name);
			}
			else if (Type == "Ka")
			{
				SS >> MaterialInfo.Ka.X >> MaterialInfo.Ka.Y >> MaterialInfo.Ka.Z;
			}
			else if (Type == "Kd")
			{
				SS >> MaterialInfo.Kd.X >> MaterialInfo.Kd.Y >> MaterialInfo.Kd.Z;
			}
			else if (Type == "Ks")
			{
				SS >> MaterialInfo.Ks.X >> MaterialInfo.Ks.Y >> MaterialInfo.Ks.Z;
			}
			else if (Type == "Ke")
			{
				SS >> MaterialInfo.Ke.X >> MaterialInfo.Ke.Y >> MaterialInfo.Ke.Z;
			}
			else if (Type == "Ns")
			{
				SS >> MaterialInfo.Ns;
			}
			else if (Type == "Ni")
			{
				SS >> MaterialInfo.Ni;
			}
			else if (Type == "d")
			{
				SS >> MaterialInfo.D;
			}
			else if (Type == "illum")
			{
				SS >> MaterialInfo.Illum;
			}
			else if (Type == "map_Kd")
			{
				std::getline(SS >> std::ws, MaterialInfo.MapKd);
			}
		}
		// 마지막 material은 루프 안에서 push되지 않으므로 여기서 추가
		if (bHasMaterial)
		{
			Materials.push_back(MaterialInfo);
		}
		auto EndTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> Elapsed = EndTime - StartTime;

		// [3] 로그 출력 (초 단위)
		UE_LOG("[MTL] Load Success: %s\n", PathFileName.c_str());
		UE_LOG("[MTL] Execution Time: %.6f seconds\n", Elapsed.count());
		return Materials;
	};
};

// =========================
	// Asset Manager
	// =========================

class ENGINE_API FAssetManager
{
private:
	static TMap<FString, FStaticMesh*> StaticMeshCache;// EX) \\Assets\\Meshes\\Dorumon.obj 가 key가됨
	static TMap<FString, FTexture*> TextureCache; // mtl파일의 map_kd dorumon.png이면 \\Assets\\Meshes\\dorumon.png가 key가됨
	static TMap<FString, FMaterial*> MaterialCache;// mtl파일에서 newmtl Dorumon이면 Dorumon이 key가됨
	static ID3D11Device* Device;
public:
	static FStaticMesh* LoadObjStaticMeshAsset(const FString& PathFileName);
	static FMaterial* GetMaterialByName(const FString& Name);
	static const TArray<FMaterial*> GetAllMaterials();
	static UStaticMesh* LoadObjStaticMesh(const FString& PathFileName);
	static FString GetBinPath(const FString& PathFileName);
	static void SaveAsBin(const FString& PathFileName, const FStaticMesh& Mesh);
	static FStaticMesh* LoadFromBin(const FString& BinPath);
	static void CleanUp();
	/*
	* This is called at FRenderer::Initalize()
	*/
	static void InjectDevice(ID3D11Device* InDevice)
	{
		Device = InDevice;
	}
	static FMaterial* LoadMaterialTexture(const FString& MaterialName, const FString& TexturePath);
	static FTexture* LoadTextureAsset(const FString& PathFileName);
private:
	//HelperFunction
	static int32 GetOrAddMaterialSlot(FStaticMesh* Mesh, const FString& MaterialName);
	static int32 GetOrCreateSectionIndex(FStaticMesh* Mesh, int32 MaterialIndex);
	static bool IsValidVertexRef(const FObjInfo& Obj, const FObjVertexRef& Ref);
	static uint32 GetOrCreateVertexIndex(FStaticMesh* Mesh, const FObjInfo& Obj, const FObjVertexRef& Ref, std::unordered_map<FObjVertexRef, uint32, FObjVertexRefHasher>& VertexMap);
	static void AppendTriangle(FStaticMesh* Mesh, int32 SectionIndex, uint32 I0, uint32 I1, uint32 I2);
	static FMaterial* LoadMaterialAsset(const FString& PathFileName);
	static void MakeMeshData(FStaticMesh* OutMesh);
};
