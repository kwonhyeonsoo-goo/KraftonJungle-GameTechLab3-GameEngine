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
	TArray<FObjMaterialInfo> Materials;
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
		std::ifstream File(FPaths::ToAbsolutePath(PathFileName));
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

			// [FIX] Windows CRLF 처리: \r 제거
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
				std::filesystem::path fullPath = FPaths::MeshDir() / LibName;
				TArray<FObjMaterialInfo> MatInfos = LoadMtlFile(fullPath.string());
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

		return ObjInfo;
	}

	static TArray<FObjMaterialInfo> LoadMtlFile(const FString& PathFileName)
	{
		std::ifstream File(FPaths::ToAbsolutePath(PathFileName));
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

private:
	static int32 GetOrAddMaterialSlot(FStaticMesh* Mesh, const FString& MaterialName)
	{
		for (int32 i = 0; i < static_cast<int32>(Mesh->MaterialSlotNames.size()); ++i)
		{
			if (Mesh->MaterialSlotNames[i] == MaterialName)
			{
				return i;
			}
		}

		Mesh->MaterialSlotNames.push_back(MaterialName);
		return static_cast<int32>(Mesh->MaterialSlotNames.size()) - 1;
	}

	static int32 GetOrCreateSectionIndex(FStaticMesh* Mesh, int32 MaterialIndex)
	{
		for (int32 i = 0; i < static_cast<int32>(Mesh->Sections.size()); ++i)
		{
			if (Mesh->Sections[i].MaterialIndex == MaterialIndex)
			{
				return i;
			}
		}

		SubMeshSection NewSection;
		NewSection.IndexStart = static_cast<int32>(Mesh->Indices.size());
		NewSection.IndexCount = 0;
		NewSection.MaterialIndex = MaterialIndex;
		Mesh->Sections.push_back(NewSection);
		return static_cast<int32>(Mesh->Sections.size()) - 1;
	}

	static bool IsValidVertexRef(const FObjInfo& Obj, const FObjVertexRef& Ref)
	{
		if (Ref.V < 0 || Ref.V >= static_cast<int32>(Obj.Positions.size()))
			return false;

		if (Ref.VT != -1 && (Ref.VT < 0 || Ref.VT >= static_cast<int32>(Obj.UVs.size())))
			return false;

		if (Ref.VN != -1 && (Ref.VN < 0 || Ref.VN >= static_cast<int32>(Obj.Normals.size())))
			return false;

		return true;
	}

	static uint32 GetOrCreateVertexIndex(
		FStaticMesh* Mesh,
		const FObjInfo& Obj,
		const FObjVertexRef& Ref,
		std::unordered_map<FObjVertexRef, uint32, FObjVertexRefHasher>& VertexMap)
	{
		//Vertex가 이미 추가된적 있는 Vertex라면 Vertices의 몇번째 인덱스에 있는지를 리턴
		auto It = VertexMap.find(Ref);
		if (It != VertexMap.end())
		{
			return It->second;
		}


		//Vertex가 추가된적 없다면 Vertex정보를 생성하고 새로운 인덱스를 리턴
		FNormalVertex Vertex{};

		Vertex.Position = Obj.Positions[Ref.V];

		if (Ref.VN != -1)
		{
			Vertex.Normal = Obj.Normals[Ref.VN];
		}
		else
		{
			Vertex.Normal = FVector(0.0f, 0.0f, 1.0f);
		}

		if (Ref.VT != -1)
		{
			Vertex.UV = Obj.UVs[Ref.VT];
		}
		else
		{
			Vertex.UV = FVector2(0.0f, 0.0f);
		}

		Vertex.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

		uint32 NewIndex = static_cast<uint32>(Mesh->Vertices.size());
		Mesh->Vertices.push_back(Vertex);
		VertexMap.emplace(Ref, NewIndex);

		return NewIndex;
	}

	// [FIX] Section을 레퍼런스가 아닌 인덱스로 받아 안전하게 접근.
	static void AppendTriangle(
		FStaticMesh* Mesh,
		int32 SectionIndex,
		uint32 I0,
		uint32 I1,
		uint32 I2)
	{
		Mesh->Indices.push_back(I0);
		Mesh->Indices.push_back(I1);
		Mesh->Indices.push_back(I2);
		Mesh->Sections[SectionIndex].IndexCount += 3;
	}

public:
	static FStaticMesh* LoadObjStaticMeshAsset(const FString& PathFileName, ID3D11Device* Device)
	{
		if (StaticMeshCache.contains(PathFileName))
		{
			return StaticMeshCache[PathFileName];
		}

		FObjInfo ObjInfo = FObjImporter::LoadObjFile(PathFileName);
		if (ObjInfo.Positions.empty() || ObjInfo.Faces.empty())
		{
			printf("[OBJ] Invalid or empty obj : %s\n", PathFileName.c_str());
			return nullptr;
		}
		FStaticMesh* Mesh = new FStaticMesh();
		std::filesystem::path ParentDir = std::filesystem::path(PathFileName).parent_path();
		std::filesystem::path MatfilePath = ParentDir / ObjInfo.Mtllib;//mtl파일 위치
		LoadMaterialAsset(MatfilePath.string(), Device); //TextureCache와 MaterialCache 채우기

		Mesh->Path = PathFileName;
		if (!ObjInfo.Mtllib.empty())
		{
			// MTL은 OBJ와 같은 디렉토리에 있다고 가정
			std::filesystem::path ObjAbsPath = FPaths::ToAbsolutePath(PathFileName);
			Mesh->MtlPath = (ObjAbsPath.parent_path() / ObjInfo.Mtllib).string();
		}

		//Mesh->Sections.reserve(Mesh->MaterialSlotNames.size() + 16);

		std::unordered_map<FObjVertexRef, uint32, FObjVertexRefHasher> VertexMap;

		for (const FObjFace& Face : ObjInfo.Faces)
		{
			const int32 FaceVertexCount = static_cast<int32>(Face.VertexRefs.size());
			if (FaceVertexCount < 3)
			{
				continue;
			}


			const int32 MaterialIndex = GetOrAddMaterialSlot(Mesh, Face.Context.MaterialName);
			//같은 매터리얼(png파일?)이면 같은 섹션에 추가. 매터리얼이 바뀌면 새로운 섹션 생성.
			const int32 SectionIndex = GetOrCreateSectionIndex(Mesh, MaterialIndex);

			// fan triangulation: (0,1,2), (0,2,3), (0,3,4), ...
			for (int32 i = 1; i + 1 < FaceVertexCount; ++i)
			{
				const FObjVertexRef& R0 = Face.VertexRefs[0];
				const FObjVertexRef& R1 = Face.VertexRefs[i];
				const FObjVertexRef& R2 = Face.VertexRefs[i + 1];

				if (!IsValidVertexRef(ObjInfo, R0) ||
					!IsValidVertexRef(ObjInfo, R1) ||
					!IsValidVertexRef(ObjInfo, R2))
				{
					printf("[OBJ] Invalid face vertex ref in : %s\n", PathFileName.c_str());
					continue;
				}

				const uint32 I0 = GetOrCreateVertexIndex(Mesh, ObjInfo, R0, VertexMap);
				const uint32 I1 = GetOrCreateVertexIndex(Mesh, ObjInfo, R1, VertexMap);
				const uint32 I2 = GetOrCreateVertexIndex(Mesh, ObjInfo, R2, VertexMap);

				AppendTriangle(Mesh, SectionIndex, I0, I1, I2);
			}
		}

		Mesh->MeshData = std::make_shared<FMeshData>();

		Mesh->MeshData->Vertices.reserve(Mesh->Vertices.size());
		for (const FNormalVertex& NV : Mesh->Vertices)
		{
			FPrimitiveVertex PV;
			PV.Position = NV.Position;
			PV.Color = NV.Color;
			PV.Normal = NV.Normal;
			PV.UV = NV.UV;
			Mesh->MeshData->Vertices.push_back(PV);
		}

		Mesh->MeshData->Indices.assign(Mesh->Indices.begin(), Mesh->Indices.end());
		Mesh->MeshData->Topology = EMeshTopology::EMT_TriangleList;
		Mesh->MeshData->CreateVertexAndIndexBuffer(Device);
		Mesh->MeshData->UpdateLocalBound();

		StaticMeshCache[PathFileName] = Mesh;
		return Mesh;
	}
	static FTexture* LoadTextureAsset(const FString& PathFileName, ID3D11Device* Device)
	{
		if (TextureCache.contains(PathFileName))
		{
			return TextureCache[PathFileName];
		}
		/** 텍스쳐 로드 */
		int width = 0, height = 0, channels = 0;

		unsigned char* data = stbi_load(
			FPaths::ToAbsolutePath(PathFileName).c_str(),
			&width,
			&height,
			&channels,
			STBI_rgb_alpha // 강제 RGBA
		);

		if (!data)
		{
			// TODO: fallback texture
			return nullptr;
		}

		ID3D11Texture2D* texture = nullptr;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ⭐ diffuse면 SRGB 추천
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = data;
		initData.SysMemPitch = width * 4;

		HRESULT hr = Device->CreateTexture2D(&desc, &initData, &texture);

		if (FAILED(hr))
		{
			stbi_image_free(data);
			return nullptr;
		}

		ID3D11ShaderResourceView* srv = nullptr;

		hr = Device->CreateShaderResourceView(texture, nullptr, &srv);

		// Texture는 SRV 만들었으면 바로 버려도 됨
		texture->Release();

		if (FAILED(hr))
		{
			stbi_image_free(data);
			return nullptr;
		}

		stbi_image_free(data);


		ID3D11SamplerState* sampler = nullptr;
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		Device->CreateSamplerState(&samplerDesc, &sampler);

		FTexture* MT = new FTexture();
		MT->TextureSRV = srv;
		MT->SamplerState = sampler;
		TextureCache[PathFileName] = MT;

		return MT;
	}
	static FMaterial* LoadMaterialAsset(const FString& PathFileName, ID3D11Device* Device)
	{
		if (MaterialCache.contains(PathFileName))
		{
			return MaterialCache[PathFileName];
		}

		TArray<FObjMaterialInfo> MatInfos = FObjImporter::LoadMtlFile(PathFileName);
		if (MatInfos.empty())
		{
			printf("[MTL] No materials found in: %s\n", PathFileName.c_str());
			return nullptr;
		}

		// 텍스처 셰이더 경로
		std::filesystem::path Root = FPaths::ProjectRoot();
		std::wstring VSPath = (Root / "Engine/Shaders/TextureVertexShader.hlsl").wstring();
		std::wstring PSPath = (Root / "Engine/Shaders/TexturePixelShader.hlsl").wstring();

		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, PSPath.c_str());

		TArray<FMaterial*> Materials;
		FMaterial* FirstMat = nullptr;
		for (const FObjMaterialInfo& Info : MatInfos)
		{
			if (MaterialCache.contains(Info.Name))
			{
				if (!FirstMat)
					FirstMat = MaterialCache[Info.Name];
				continue;
			}

			FMaterial* Mat = new FMaterial();
			Mat->SetOriginName(Info.Name);
			Mat->SetVertexShader(VS);
			Mat->SetPixelShader(PS);

			// RasterizerState 명시 설정 (없으면 이전 프레임 상태 상속되는 문제 방지)
			{
				FRasterizerStateOption RSOption;
				RSOption.FillMode = D3D11_FILL_SOLID;
				RSOption.CullMode = D3D11_CULL_NONE;  // blank spots 원인 확인용: culling 완전 비활성화
				RSOption.DepthClipEnable = true;
				auto RS = FRasterizerState::Create(Device, RSOption);
				Mat->SetRasterizerOption(RSOption);
				Mat->SetRasterizerState(RS);

				FDepthStencilStateOption DSOption;
				DSOption.DepthEnable = true;
				DSOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
				auto DSS = FDepthStencilState::Create(Device, DSOption);
				Mat->SetDepthStencilOption(DSOption);
				Mat->SetDepthStencilState(DSS);
			}

			// b2: ColorTint(VS) + BaseColor(PS) — float4 하나 공유
			int32 SlotIndex = Mat->CreateConstantBuffer(Device, 16);
			if (SlotIndex >= 0)
			{
				Mat->RegisterParameter("ColorTint", SlotIndex, 0, 16);
				Mat->RegisterParameter("BaseColor", SlotIndex, 0, 16);

				// 기본값 흰색
				float White[4] = { 1.f, 1.f, 1.f, 1.f };
				Mat->GetConstantBuffer(SlotIndex)->SetData(White, sizeof(White), 0);
			}

			// Diffuse 텍스처 (MTL 파일과 같은 디렉토리에서 탐색)
			if (!Info.MapKd.empty())
			{
				std::filesystem::path MtlDir = std::filesystem::path(PathFileName).parent_path();
				std::filesystem::path TexFullPath = MtlDir / Info.MapKd;
				FTexture* Tex = LoadTextureAsset(FPaths::ToRelativePath(TexFullPath.string()), Device);
				if (Tex)
				{
					Mat->SetMaterialTexture(std::shared_ptr<FTexture>(Tex, [](FTexture*) {}));
				}
			}

			MaterialCache[Info.Name] = Mat;
			if (!FirstMat)
				FirstMat = Mat;
		}

		//MaterialCache[PathFileName] = FirstMat;
		return FirstMat;
	}
	static FMaterial* GetMaterialByName(const FString& Name)
	{
		auto It = MaterialCache.find(Name);
		if (It != MaterialCache.end())
			return It->second;
		return nullptr;
	}
	static const TArray<FMaterial*> GetAllMaterials()
	{
		TArray<FMaterial*> Materials;
		for (const auto& Pair : MaterialCache)
		{
			Materials.push_back(Pair.second);
		}
		return Materials;
	}

	static UStaticMesh* LoadObjStaticMesh(const FString& PathFileName, ID3D11Device* Device)
	{
		for (TObjectIterator<UStaticMesh> It; It; ++It)
		{
			UStaticMesh* StaticMesh = *It;
			if (StaticMesh->GetAssetPath() == PathFileName)
			{
				return *It;
			}
		}

		FStaticMesh* StaticMeshAsset = LoadObjStaticMeshAsset(PathFileName, Device);
		UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>();
		StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

		return StaticMesh;
	}

	static void CleanUp()
	{
		for (auto& Pair : StaticMeshCache)
		{
			delete Pair.second;
		}
		StaticMeshCache.clear();
		for (auto& Pair : MaterialCache)
		{
			delete Pair.second;
		}
		MaterialCache.clear();
		for (auto& Pair : TextureCache)
		{
			delete Pair.second;
		}
		TextureCache.clear();
	}
};