#include "AssetManager.h"

TMap<FString, FStaticMesh*> FAssetManager::StaticMeshCache;
TMap<FString, FMaterial*>   FAssetManager::MaterialCache;
TMap<FString, FTexture*>    FAssetManager::TextureCache;
ID3D11Device* FAssetManager::Device = nullptr;

int32 FAssetManager::GetOrAddMaterialSlot(FStaticMesh* Mesh, const FString& MaterialName)
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
int32 FAssetManager::GetOrCreateSectionIndex(FStaticMesh* Mesh, int32 MaterialIndex)
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

bool FAssetManager::IsValidVertexRef(const FObjInfo& Obj, const FObjVertexRef& Ref)
{
	if (Ref.V < 0 || Ref.V >= static_cast<int32>(Obj.Positions.size()))
		return false;

	if (Ref.VT != -1 && (Ref.VT < 0 || Ref.VT >= static_cast<int32>(Obj.UVs.size())))
		return false;

	if (Ref.VN != -1 && (Ref.VN < 0 || Ref.VN >= static_cast<int32>(Obj.Normals.size())))
		return false;

	return true;
}

uint32 FAssetManager::GetOrCreateVertexIndex(FStaticMesh* Mesh, const FObjInfo& Obj, const FObjVertexRef& Ref,
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

void FAssetManager::AppendTriangle(FStaticMesh* Mesh, int32 SectionIndex, uint32 I0, uint32 I1, uint32 I2)
{
	Mesh->Indices.push_back(I0);
	Mesh->Indices.push_back(I1);
	Mesh->Indices.push_back(I2);
	Mesh->Sections[SectionIndex].IndexCount += 3;
}

//Relative Path Comes in
FStaticMesh* FAssetManager::LoadObjStaticMeshAsset(const FString& PathFileName)
{
	//FString RelativePath = FPaths::ToRelativePath(PathFileName);
	if (StaticMeshCache.contains(PathFileName))//캐시에 저장할때는 상대경로
	{
		return StaticMeshCache[PathFileName];
	}

	// bin 캐시가 있으면 OBJ 파싱 없이 바로 로드
	FString BinPath = ToBinPath(PathFileName);
	FStaticMesh* CachedMesh = LoadFromBin(FPaths::ToAbsolutePath(BinPath));
	if (CachedMesh)
	{
		LoadMaterialAsset(FPaths::ToAbsolutePath(CachedMesh->MtlPath));//TextureCache와 MaterialCache 채우기
		StaticMeshCache[PathFileName] = CachedMesh;
		return CachedMesh;
	}

	FObjInfo ObjInfo = FObjImporter::LoadObjFile(FPaths::ToAbsolutePath(PathFileName));
	if (ObjInfo.Positions.empty() || ObjInfo.Faces.empty())
	{
		printf("[OBJ] Invalid or empty obj : %s\n", PathFileName.c_str());
		return nullptr;
	}
	FStaticMesh* Mesh = new FStaticMesh();

	FString RelativePath = FPaths::ToRelativePath(PathFileName);
	std::filesystem::path path = FPaths::ToU8String(RelativePath);
	std::filesystem::path ParentDir = path.parent_path();
	std::filesystem::path MatfilePath = ParentDir / FPaths::ToU8String(ObjInfo.Mtllib);//mtl파일 위치
	LoadMaterialAsset(FPaths::FromPath(MatfilePath)); //TextureCache와 MaterialCache 채우기

	Mesh->Path = PathFileName;
	if (!ObjInfo.Mtllib.empty())
	{
		// MTL은 OBJ와 같은 디렉토리에 있다고 가정
		std::filesystem::path ObjAbsPath = FPaths::ToU8String(FPaths::ToRelativePath(PathFileName));
		Mesh->MtlPath = FPaths::FromPath(ObjAbsPath.parent_path() / FPaths::ToU8String(ObjInfo.Mtllib));
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

	MakeMeshData(Mesh);
	SaveAsBin(PathFileName, *Mesh);
	StaticMeshCache[PathFileName] = Mesh;
	return Mesh;
}

FMaterial* FAssetManager::GetMaterialByName(const FString& Name)
{
	auto It = MaterialCache.find(Name);
	if (It != MaterialCache.end())
		return It->second;
	return nullptr;
}

const TArray<FMaterial*> FAssetManager::GetAllMaterials()
{
	TArray<FMaterial*> Materials;
	for (const auto& Pair : MaterialCache)
	{
		Materials.push_back(Pair.second);
	}
	return Materials;
}
//Relative Path Comes in
UStaticMesh* FAssetManager::LoadObjStaticMesh(const FString& PathFileName)
{
	for (TObjectIterator<UStaticMesh> It; It; ++It)
	{
		UStaticMesh* StaticMesh = *It;
		if (StaticMesh->GetAssetPath() == PathFileName)
		{
			return *It;
		}
	}
	FStaticMesh* StaticMeshAsset = LoadObjStaticMeshAsset(PathFileName);
	UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>();
	StaticMesh->SetStaticMeshAsset(StaticMeshAsset);

	return StaticMesh;
}

FString FAssetManager::ToBinPath(const FString& PathFileName)
{
	return PathFileName.substr(0, PathFileName.find_last_of('.')) + ".bin";
}

void FAssetManager::SaveAsBin(const FString& PathFileName, const FStaticMesh& Mesh)
{
	FString BinPath = ToBinPath(PathFileName);
	FWindowsBinWriter Writer(BinPath);

	Writer.WriteString(FPaths::ToRelativePath(Mesh.Path));//String
	Writer.WriteString(FPaths::ToRelativePath(Mesh.MtlPath));//String
	Writer.WriteArray(Mesh.Vertices);//Array of FNormalVertex
	Writer.WriteArray(Mesh.Indices);//Array of uint32
	Writer.WriteStringArray(Mesh.MaterialSlotNames);//Array of String
	Writer.WriteArray(Mesh.Sections);//Array of SubMeshSection
}

//Absolute Comes in
FStaticMesh* FAssetManager::LoadFromBin(const FString& BinPath)
{

	//TODO : Delete These Time;
	auto StartTime = std::chrono::high_resolution_clock::now();

	FWindowsBinReader Reader(BinPath);
	if (!Reader.IsOpen())
		return nullptr;

	FStaticMesh* Mesh = new FStaticMesh();
	Reader.ReadString(Mesh->Path);
	Reader.ReadString(Mesh->MtlPath);
	/*Mesh->Path = FPaths::ToAbsolutePath(Mesh->Path);
	Mesh->Path = FPaths::ToAbsolutePath(Mesh->MtlPath);*/
	Reader.ReadArray(Mesh->Vertices);
	Reader.ReadArray(Mesh->Indices);
	Reader.ReadStringArray(Mesh->MaterialSlotNames);
	Reader.ReadArray(Mesh->Sections);

	MakeMeshData(Mesh);

	auto EndTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> Elapsed = EndTime - StartTime;

	// [3] 로그 출력 (초 단위)
	UE_LOG("[BIN_OBJ] Load Success: %s\n", BinPath.c_str());
	UE_LOG("[BIN_OBJ] Execution Time: %.6f seconds\n", Elapsed.count());
	return Mesh;
}

void FAssetManager::CleanUp()
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

FMaterial* FAssetManager::LoadMaterialTexture(const FString& MaterialName, const FString& TexturePath)
{
	auto MatIt = MaterialCache.find(MaterialName);
	if (MatIt != MaterialCache.end())
	{
		return MatIt->second;
	}
	else
	{
		FMaterial* Mat = new FMaterial();
		std::filesystem::path Root = FPaths::ProjectRoot();
		std::wstring VSPath = (Root / "Engine/Shaders/TextureVertexShader.hlsl").wstring();
		std::wstring PSPath = (Root / "Engine/Shaders/TexturePixelShader.hlsl").wstring();

		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, PSPath.c_str());
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
			Mat->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			// 기본값 흰색
			float White[4] = { 1.f, 1.f, 1.f, 1.f };
			Mat->SetParameterData("BaseColor", White, sizeof(White));
		}
		Mat->SetOriginName(MaterialName);
		// 텍스처 로드
		FTexture* Tex = LoadTextureAsset(TexturePath);
		if (Tex)
		{
			Mat->SetMaterialTexture(std::shared_ptr<FTexture>(Tex, [](FTexture*) {}));
		}
		MaterialCache[MaterialName] = Mat;
		return Mat;
	}
}


FTexture* FAssetManager::LoadTextureAsset(const FString& PathFileName)
{
	FString RelativePath = FPaths::ToRelativePath(PathFileName);
	if (TextureCache.contains(RelativePath))
	{
		return TextureCache[RelativePath];
	}
	/** 텍스쳐 로드 */
	int width = 0, height = 0, channels = 0;

	FString AbsolutePath = FPaths::ToAbsolutePath(PathFileName);
	std::filesystem::path path = FPaths::ToU8String(AbsolutePath);
	FILE* f;
	_wfopen_s(&f, path.c_str(), L"rb");
	if (!f)
	{
		return nullptr;
	}
	unsigned char* data = stbi_load_from_file(f, &width, &height, &channels, STBI_rgb_alpha);
	fclose(f);

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
	MT->FilePath = RelativePath;
	TextureCache[RelativePath] = MT;

	return MT;
}


FMaterial* FAssetManager::LoadMaterialAsset(const FString& PathFileName)
{
	FString RelativePath = FPaths::ToRelativePath(PathFileName);
	FString AbsolutePath = FPaths::ToAbsolutePath(PathFileName);
	if (MaterialCache.contains(RelativePath))
	{
		return MaterialCache[RelativePath];
	}

	TArray<FObjMaterialInfo> MatInfos = FObjImporter::LoadMtlFile(AbsolutePath);
	if (MatInfos.empty())
	{
		printf("[MTL] No materials found in: %s\n", AbsolutePath.c_str());
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
			Mat->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			// 기본값 흰색
			float White[4] = { 1.f, 1.f, 1.f, 1.f };
			Mat->SetParameterData("BaseColor", White, sizeof(White));
		}

		// Diffuse 텍스처 (MTL 파일과 같은 디렉토리에서 탐색)
		if (!Info.MapKd.empty())
		{
			FString RelativePath = FPaths::ToRelativePath(PathFileName);
			std::filesystem::path path = FPaths::ToU8String(RelativePath);
			std::filesystem::path MtlDir = path.parent_path();
			std::filesystem::path TexFullPath = MtlDir / FPaths::ToU8String(Info.MapKd);
			FTexture* Tex = LoadTextureAsset(FPaths::ToRelativePath(FPaths::FromPath(TexFullPath)));
			if (Tex)
			{
				Mat->SetMaterialTexture(std::shared_ptr<FTexture>(Tex, [](FTexture*) {}));
			}
		}

		MaterialCache[Info.Name] = Mat;
		if (!FirstMat)
			FirstMat = Mat;
	}

	return FirstMat;
}


void FAssetManager::MakeMeshData(FStaticMesh* OutMesh)
{
	OutMesh->MeshData = std::make_shared<FMeshData>();
	OutMesh->MeshData->Vertices.reserve(OutMesh->Vertices.size());
	for (const FNormalVertex& NV : OutMesh->Vertices)
	{
		FPrimitiveVertex PV;
		PV.Position = NV.Position;
		PV.Color = NV.Color;
		PV.Normal = NV.Normal;
		PV.UV = NV.UV;
		OutMesh->MeshData->Vertices.push_back(PV);
	}
	OutMesh->MeshData->Indices.assign(OutMesh->Indices.begin(), OutMesh->Indices.end());
	OutMesh->MeshData->Topology = EMeshTopology::EMT_TriangleList;
	OutMesh->MeshData->CreateVertexAndIndexBuffer(Device);
	OutMesh->MeshData->UpdateLocalBound();
}