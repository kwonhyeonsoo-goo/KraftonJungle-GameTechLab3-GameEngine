#include "AssetManager.h"
#include "AssetRegistry.h"
#include "Object/StaticMesh.h"
#include "Object/ObjectFactory.h"
#include "Mesh/ObjManager.h"
#include "Mesh/StaticMeshRenderData.h"
#include "Debug/EngineLog.h"
#include "AssetCooker.h" 
#include "Primitive/PrimitiveCube.h"
#include "Primitive/PrimitiveSphere.h"
#include "Primitive/PrimitivePlane.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Material.h"

#include "Primitive/PrimitiveSky.h"
#include "ThirdParty/stb_image.h"
#include "Core/Paths.h"
#include <algorithm>

FAssetManager& FAssetManager::Get()
{
	static FAssetManager Instance;
	return Instance;
}

UStaticMesh* FAssetManager::LoadBasicShape(const FString& ShapeName)
{
	FString FullPath = "Engine/BasicShapes/" + ShapeName;

	// 1. 이미 메모리에 있는지 확인
	auto It = LoadedMeshes.find(FullPath);
	if (It != LoadedMeshes.end())
	{
		return It->second;
	}

	// 2. 코드로 Primitive 데이터 생성
	std::shared_ptr<FMeshData> MeshData = nullptr;
	if (ShapeName == "Cube")
	{
		CPrimitiveCube CubeGen;
		MeshData = CPrimitiveBase::GetCached(CPrimitiveCube::Key);
	}
	else if (ShapeName == "Sphere")
	{
		CPrimitiveSphere SphereGen(16, 16);
		MeshData = CPrimitiveBase::GetCached(CPrimitiveSphere::Key);
	}
	else if (ShapeName == "Plane")
	{
		CPrimitivePlane PlaneGen;
		MeshData = CPrimitiveBase::GetCached(CPrimitivePlane::Key);
	}
	else if (ShapeName == "SkySphere")
	{
		CPrimitiveSky SkyGen(32, 32); // 부드러운 하늘을 위해 32x32 해상도 사용
		MeshData = CPrimitiveBase::GetCached(CPrimitiveSky::Key);
	}
	if (!MeshData) return nullptr;
	if (MeshData->Sections.empty() && !MeshData->Indices.empty())
	{
		FMeshSection Sec;
		Sec.FirstIndex = 0;
		Sec.IndexCount = static_cast<uint32>(MeshData->Indices.size());
		Sec.MaterialIndex = 0;
		MeshData->Sections.push_back(Sec);
	}
	// 3. RenderData 세팅
	FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
	RenderData->SetMeshData(MeshData);
	RenderData->SetAssetPath(FullPath);


	// 4. UStaticMesh 객체 생성 및 GC 보호
	UStaticMesh* NewMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr, ShapeName);
	NewMesh->SetStaticMeshAsset(RenderData);
	NewMesh->AddFlags(EObjectFlags::Standalone);

	// 5. 캐시에 저장
	LoadedMeshes[FullPath] = NewMesh;
	UE_LOG("[AssetManager] Generated and cached basic shape: %s\n", FullPath.c_str());

	return NewMesh;
}
UStaticMesh* FAssetManager::LoadStaticMesh(ID3D11Device* Device, const FString& AssetName)
{
	bool bRenderDataOwnedByStaticMesh = true;

	if (AssetName.starts_with("Engine/BasicShapes/"))
	{
		FString ShapeName = AssetName.substr(19); // "Engine/BasicShapes/" 길이만큼 제거
		return LoadBasicShape(ShapeName);
	}
	// 레지스트리에게 에셋 정보(경로)를 물어봄
	auto FastIt = LoadedMeshes.find(AssetName);
	if (FastIt != LoadedMeshes.end())
	{
		return FastIt->second; // 4999개의 사과는 여기서 문자열 파싱도 안 하고 바로 통과!
	}
	FAssetData AssetData;
	if (!FAssetRegistry::Get().GetAssetByObjectPath(AssetName, AssetData))
	{
		UE_LOG("[AssetManager] Failed to find asset in registry: %s\n", AssetName.c_str());
		return nullptr;
	}

	// 이미 메모리에 캐싱되어 있다면 즉시 반환
	auto It = LoadedMeshes.find(AssetData.AssetPath);
	if (It != LoadedMeshes.end())
	{
		return It->second;
	}


	FStaticMeshRenderData* RenderData = nullptr;

	//  쿡된 에셋이 유효하면 바이너리에서 로드 (OBJ 파싱 스킵)
	if (AssetData.AssetPath.ends_with(".dasset"))
	{
		FString AbsDasset = FPaths::ToAbsolutePath(AssetData.AssetPath);
		RenderData = FAssetCooker::LoadCookedStaticMesh(AbsDasset);
		if (!RenderData)
		{
			UE_LOG("[AssetManager] Failed to load .dasset: %s\n", AssetData.AssetPath.c_str());
			return nullptr;
		}
	}
	else
	{
		// 기존 OBJ 파이프라인 
		FString CookedPath = FAssetCooker::GetCookedPath(AssetData.AssetPath);

		if (!FAssetCooker::NeedsCook(AssetData.AssetPath, CookedPath))
		{
			RenderData = FAssetCooker::LoadCookedStaticMesh(CookedPath);
		}
		if (!RenderData)
		{
			RenderData = FObjManager::LoadObjStaticMeshAsset(AssetData.AssetPath);
			bRenderDataOwnedByStaticMesh = false;
			if (!RenderData) return nullptr;
			FAssetCooker::SaveCookedStaticMesh(RenderData, AssetData.AssetPath, CookedPath);
		}
	}
	RenderData->SetAssetPath(AssetData.AssetPath);
	RenderData->DefaultMaterialInstances.resize(RenderData->GetSections().size());
	for (uint32 i = 0; i < RenderData->GetSections().size(); ++i)
	{
		std::shared_ptr<FMaterial> BaseMat = FMaterialManager::Get().FindByName("M_StaticMesh");
		if (!BaseMat) BaseMat = FMaterialManager::Get().FindByName("M_Default");

		if (BaseMat)
		{
			// 5000번이 아니라 메쉬 원본(apple.obj) 당 딱 1번만 만들어집니다!
			std::shared_ptr<FDynamicMaterial> MatInstance = BaseMat->CreateDynamicMaterial();

			bool bTextureLoaded = false;
			if (Device && i < RenderData->ImportedTexturePaths.size() && !RenderData->ImportedTexturePaths[i].empty())
			{
				ID3D11ShaderResourceView* SRV = LoadTexture(Device, RenderData->ImportedTexturePaths[i]);
				if (SRV)
				{
					auto MT = std::make_shared<FMaterialTexture>();
					MT->TextureSRV = SRV;
					MT->AssetPath = RenderData->ImportedTexturePaths[i];
					MatInstance->SetMaterialTexture(MT);
					bTextureLoaded = true;
					// SRV->Release(); // LoadTexture가 올려준 참조 카운트를 내리려면 활성화 (에셋 매니저 구조에 따라 다름)
				}
			}

			if (!bTextureLoaded)
			{
				MatInstance->SetMaterialTexture(GDefaultWhiteTexture);
			}

			// 색상 세팅
			if (i < RenderData->ImportedDiffuseColors.size())
			{
				MatInstance->SetVector3Parameter("ColorTint", RenderData->ImportedDiffuseColors[i]);
			}

			// 1. RenderData가 스마트 포인터를 쥐고 있게 해서 메모리 유지
			RenderData->DefaultMaterialInstances[i] = MatInstance;

			// 2. FMesh의 기본 매테리얼 포인터로 등록 (이제 컴포넌트가 이걸 갖다 씁니다!)
			RenderData->SetDefaultMaterial(i, MatInstance.get());
		}
	}
	// =========================================================================

	// 오브젝트 생성 및 GC 보호 플래그 설정
	UStaticMesh* NewMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr, AssetData.AssetName);
	NewMesh->SetStaticMeshAsset(RenderData, bRenderDataOwnedByStaticMesh);
	NewMesh->AddFlags(EObjectFlags::Standalone); // 에셋은 GC가 마음대로 죽이지 못하게 보호

	// 5. 캐시에 저장 후 반환
	LoadedMeshes[AssetData.AssetPath] = NewMesh;
	UE_LOG("[AssetManager] Successfully loaded and cached: %s\n", AssetData.AssetPath.c_str());

	return NewMesh;
}

ID3D11ShaderResourceView* FAssetManager::LoadTexture(ID3D11Device* Device, const FString& FilePath)
{
	if (!Device || FilePath.empty()) return nullptr;

	// 이미 메모리(캐시)에 로드된 텍스처인지 확인
	auto It = LoadedTextures.find(FilePath);
	if (It != LoadedTextures.end())
	{
		//텍스처를 반환할 때 레퍼런스 카운트를 올려줍니다.
		// (FMaterialTexture 소멸자에서 Release()를 호출하더라도 원본이 파괴되지 않게 보호)
		if (It->second)
		{
			It->second->AddRef();
		}
		return It->second;
	}

	//캐시에 없으면 디스크에서 로드 (기존 컴포넌트에 있던 로직 이동)
	int width = 0, height = 0, channels = 0;
	FILE* f = nullptr;

	std::wstring WidePath = FPaths::ToWide(FPaths::ToAbsolutePath(FilePath));
	_wfopen_s(&f, WidePath.c_str(), L"rb");

	unsigned char* data = nullptr;
	if (f)
	{
		data = stbi_load_from_file(f, &width, &height, &channels, STBI_rgb_alpha);
		fclose(f);
	}

	if (!data)
	{
		UE_LOG("[AssetManager] Failed to load texture file: %s\n", FilePath.c_str());
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = data;
	initData.SysMemPitch = width * 4;

	ID3D11Texture2D* texture = nullptr;
	HRESULT hr = Device->CreateTexture2D(&desc, &initData, &texture);
	if (FAILED(hr)) { stbi_image_free(data); return nullptr; }

	ID3D11ShaderResourceView* srv = nullptr;
	hr = Device->CreateShaderResourceView(texture, nullptr, &srv);

	texture->Release();
	stbi_image_free(data);

	if (FAILED(hr)) return nullptr;

	// 생성된 SRV를 캐시에 저장
	LoadedTextures[FilePath] = srv;
	UE_LOG("[AssetManager] Successfully loaded and cached texture: %s\n", FilePath.c_str());

	// 캐시에서 꺼내주는 것과 동일하게 참조 카운트를 증가시켜서 반환
	srv->AddRef();
	return srv;
}

void FAssetManager::InvalidateStaticMesh(const FString& AssetName)
{
	FString CacheKey = AssetName;

	if (!AssetName.starts_with("Engine/BasicShapes/"))
	{
		FAssetData AssetData;
		if (FAssetRegistry::Get().GetAssetByObjectPath(AssetName, AssetData))
		{
			CacheKey = AssetData.AssetPath;
		}
	}

	auto It = LoadedMeshes.find(CacheKey);
	if (It != LoadedMeshes.end())
	{
		if (It->second)
		{
			It->second->SetStaticMeshAsset(nullptr, false);
		}

		LoadedMeshes.erase(It);
		UE_LOG("[AssetManager] Invalidated static mesh cache: %s\n", CacheKey.c_str());
	}
}

void FAssetManager::ClearCache()
{
	for (auto& pair : LoadedTextures)
	{
		if (pair.second)
		{
			pair.second->Release();
		}
	}
	LoadedTextures.clear();


	LoadedMeshes.clear();

}

FAssetManager::~FAssetManager()
{
	ClearCache();
}
