#include "StaticMeshComponent.h"
#include "Mesh/StaticMeshRenderData.h"
#include "Object/StaticMesh.h"
#include "Asset/AssetManager.h"
#include "Core/Paths.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialManager.h"
#include "Serializer/Archive.h"
#include "Renderer/Renderer.h"
#include "Object/Class.h"
#include <d3d11.h>

namespace
{
	constexpr const char* UVScrollEnabledParam = "UVScrollEnabled";
	constexpr const char* UVScrollSpeedParam = "UVScrollSpeed";
}

IMPLEMENT_RTTI(UStaticMeshComponent, UMeshComponent)
void UStaticMeshComponent::Initialize()
{
}



FString UStaticMeshComponent::GetStaticMeshAsset() const
{
	if (StaticMesh) return StaticMesh->GetAssetPathFileName();
	return "";
}


// 덤으로 LoadStaticMesh도 깔끔하게 정리할 수 있습니다.
void UStaticMeshComponent::LoadStaticMesh(ID3D11Device* Device, const FString& AssetName)
{
	UStaticMesh* LoadedMesh = FAssetManager::Get().LoadStaticMesh(Device, AssetName);

	// 2. 방금 위에서 훌륭하게 작성하신 SetStaticMeshData를 호출하여 컴포넌트에 적용합니다. -> 박상혁 왔다감ㅋㅋ
	SetStaticMeshData(Device, LoadedMesh);
}
void UStaticMeshComponent::SetStaticMeshData(ID3D11Device* Device, UStaticMesh* InMesh)
{
	OverrideMaterials.clear();
	DynamicMaterialOwners.clear();
	UVScrollStates.clear();

	StaticMesh = InMesh;

	// 이전 메쉬가 쓰던 다이내믹 매테리얼 찌꺼기 초기화

	if (!StaticMesh) return;

	FStaticMeshRenderData* RenderData = StaticMesh->GetStaticMeshAsset();
	if (!RenderData) return;
	// LoadStaticMesh에 있던 자동 매핑 로직 이식
	uint32 NumSlots = GetNumMaterials();
	for (uint32 i = 0; i < NumSlots; ++i)
	{
		bool bTextureLoaded = false;

		if (Device && i < RenderData->ImportedTexturePaths.size())
		{
			FString TexPath = RenderData->ImportedTexturePaths[i];
			if (!TexPath.empty())
			{
				////  fs::exists 대신, 우리가 새로 만든 안전한 ToWide()를 사용하여 파일 존재 여부 확인
				//std::wstring WidePath = FPaths::ToWide(FPaths::ToAbsolutePath(TexPath));
				//DWORD dwAttrib = GetFileAttributesW(WidePath.c_str());
				//bool bExists = (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));

				//if (bExists)
				//{
				//	LoadTextureToSlot(Device, TexPath, i);
				//	bTextureLoaded = true;
				//}
				ID3D11ShaderResourceView* CachedOrLoaded = FAssetManager::Get().LoadTexture(Device, TexPath);
				if (CachedOrLoaded)
				{
					// 캐시 확인 용도로 꺼낸 것은 참조를 줄여주고(짝 맞추기) 실제 슬롯 장착 함수 호출
					CachedOrLoaded->Release();
					LoadTextureToSlot(Device, TexPath, i);
					bTextureLoaded = true;
				}
			}
		}

		// 텍스처가 없을 경우 기본 매테리얼 적용
		if (!bTextureLoaded)
		{
			std::shared_ptr<FMaterial> BaseMat = FMaterialManager::Get().FindByName("M_StaticMesh");
			if (!BaseMat) BaseMat = FMaterialManager::Get().FindByName("M_Default");

			if (BaseMat)
			{
				// 원본을 건드리지 않기 위해 다이내믹 인스턴스로 복제
				std::shared_ptr<FDynamicMaterial> DynamicMat = BaseMat->CreateDynamicMaterial();
				if (!DynamicMat)
				{
					continue;
				}

				InitializeUVScrollParameters(i, DynamicMat);

				// 배열에서 색상(Kd) 꺼내오기
				FVector MatColor(0.5f, 0.5f, 0.5f);
				if (i < RenderData->ImportedDiffuseColors.size())
				{
					MatColor = RenderData->ImportedDiffuseColors[i];
				}
				DynamicMat->SetVector3Parameter("ColorTint", MatColor);

				// 머티리얼에 색상 전달
				// DynamicMat->SetColor(MatColor);
				DynamicMat->SetMaterialTexture(GDefaultWhiteTexture);

				SetMaterial(i, DynamicMat.get());
				DynamicMaterialOwners[i] = DynamicMat; // 찌꺼기 방지 맵에 저장
			}
		}
	}
}


void UStaticMeshComponent::LoadTexture(ID3D11Device* Device, const FString& FilePath)
{
	for (uint32 i = 0; i < GetNumMaterials(); ++i)
	{
		LoadTextureToSlot(Device, FilePath, i);
	}
}
void UStaticMeshComponent::LoadTextureToSlot(ID3D11Device* Device, const FString& FilePath, uint32 SlotIndex)
{
	if (SlotIndex >= GetNumMaterials()) return;

	// 1. 다이내믹 머티리얼 준비
	std::shared_ptr<FDynamicMaterial> DynamicMat;
	auto It = DynamicMaterialOwners.find(SlotIndex);

	if (It != DynamicMaterialOwners.end())
	{
		DynamicMat = It->second;
	}
	else
	{
		std::shared_ptr<FMaterial> BaseMat = FMaterialManager::Get().FindByName("M_StaticMesh");
		if (!BaseMat) BaseMat = FMaterialManager::Get().FindByName("M_Default_Texture");
		if (!BaseMat) return;

		DynamicMat = BaseMat->CreateDynamicMaterial();
		if (!DynamicMat)
		{
			return;
		}

		InitializeUVScrollParameters(SlotIndex, DynamicMat);
		DynamicMaterialOwners[SlotIndex] = DynamicMat;
	}

	//AssetManager를 통해 텍스처 로드 (여기서 중복 파일 읽기가 완벽히 차단)
	ID3D11ShaderResourceView* srv = FAssetManager::Get().LoadTexture(Device, FilePath);
	if (!srv) return;

	//머티리얼 텍스처 세팅
	auto MT = std::make_shared<FMaterialTexture>();
	MT->TextureSRV = srv; // MT가 소멸될 때 srv->Release()를 부르지만, AssetManager가 원본을 쥐고 있으니 안전

	MT->AssetPath = FilePath;
	DynamicMat->SetMaterialTexture(MT);
	SetMaterial(SlotIndex, DynamicMat.get());
}

std::shared_ptr<FDynamicMaterial> UStaticMeshComponent::GetOrCreateDynamicMaterialForSlot(uint32 SlotIndex)
{
	if (SlotIndex >= GetNumMaterials())
	{
		return nullptr;
	}

	auto ExistingIt = DynamicMaterialOwners.find(SlotIndex);
	if (ExistingIt != DynamicMaterialOwners.end())
	{
		return ExistingIt->second;
	}

	std::shared_ptr<FMaterial> BaseMat = FMaterialManager::Get().FindByName("M_StaticMesh");
	if (!BaseMat)
	{
		return nullptr;
	}

	std::shared_ptr<FDynamicMaterial> DynamicMat = BaseMat->CreateDynamicMaterial();
	if (!DynamicMat)
	{
		return nullptr;
	}

	InitializeUVScrollParameters(SlotIndex, DynamicMat);
	FMaterial* CurrentMaterial = GetMaterial(SlotIndex);
	if (CurrentMaterial && CurrentMaterial->GetMaterialTexture())
	{
		DynamicMat->SetMaterialTexture(CurrentMaterial->GetMaterialTexture());
	}
	else
	{
		DynamicMat->SetMaterialTexture(GDefaultWhiteTexture);
	}

	DynamicMaterialOwners[SlotIndex] = DynamicMat;
	SetMaterial(SlotIndex, DynamicMat.get());
	return DynamicMat;
}

void UStaticMeshComponent::InitializeUVScrollParameters(uint32 SlotIndex, const std::shared_ptr<FDynamicMaterial>& DynamicMat)
{
	if (!DynamicMat)
	{
		return;
	}

	FUVScrollState& State = UVScrollStates[SlotIndex];
	const float EnabledValue = State.bEnabled ? 1.0f : 0.0f;
	DynamicMat->SetScalarParameter(UVScrollEnabledParam, EnabledValue);
	DynamicMat->SetParameterData(UVScrollSpeedParam, &State.Speed, sizeof(FVector2));
}

bool UStaticMeshComponent::IsUVScrollSupported(uint32 SlotIndex) const
{
	if (SlotIndex >= GetNumMaterials())
	{
		return false;
	}

	return FMaterialManager::Get().FindByName("M_StaticMesh") != nullptr;
}

bool UStaticMeshComponent::IsUVScrollEnabled(uint32 SlotIndex) const
{
	auto It = UVScrollStates.find(SlotIndex);
	if (It != UVScrollStates.end())
	{
		return It->second.bEnabled;
	}

	return false;
}

void UStaticMeshComponent::SetUVScrollEnabled(uint32 SlotIndex, bool bEnabled)
{
	UVScrollStates[SlotIndex].bEnabled = bEnabled;
	if (std::shared_ptr<FDynamicMaterial> DynamicMat = GetOrCreateDynamicMaterialForSlot(SlotIndex))
	{
		const float EnabledValue = bEnabled ? 1.0f : 0.0f;
		DynamicMat->SetScalarParameter(UVScrollEnabledParam, EnabledValue);
	}
}

FVector2 UStaticMeshComponent::GetUVScrollSpeed(uint32 SlotIndex) const
{
	auto It = UVScrollStates.find(SlotIndex);
	if (It != UVScrollStates.end())
	{
		return It->second.Speed;
	}

	return FVector2(0.0f, 0.0f);
}

void UStaticMeshComponent::SetUVScrollSpeed(uint32 SlotIndex, const FVector2& Speed)
{
	UVScrollStates[SlotIndex].Speed = Speed;
	if (std::shared_ptr<FDynamicMaterial> DynamicMat = GetOrCreateDynamicMaterialForSlot(SlotIndex))
	{
		DynamicMat->SetParameterData(UVScrollSpeedParam, &Speed, sizeof(FVector2));
	}
}

FMeshData* UStaticMeshComponent::GetMeshData() const
{
	if (StaticMesh && StaticMesh->GetStaticMeshAsset())
		return StaticMesh->GetStaticMeshAsset()->GetMeshData();
	return nullptr;
}

const TArray<FMeshSection>& UStaticMeshComponent::GetSections() const
{
	if (StaticMesh && StaticMesh->GetStaticMeshAsset())
		return StaticMesh->GetStaticMeshAsset()->GetSections();
	static TArray<FMeshSection> Empty;
	return Empty;
}

FMaterial* UStaticMeshComponent::GetMaterial(uint32 SlotIndex) const
{
	// 1순위: 오버라이드 매테리얼
	if (SlotIndex < OverrideMaterials.size() && OverrideMaterials[SlotIndex])
		return OverrideMaterials[SlotIndex];

	// 2순위: 렌더 데이터 기본 매테리얼
	if (StaticMesh && StaticMesh->GetStaticMeshAsset())
		return StaticMesh->GetStaticMeshAsset()->GetDefaultMaterial(SlotIndex);
	return nullptr;
}

uint32 UStaticMeshComponent::GetNumMaterials() const
{
	return static_cast<uint32>(GetSections().size());
}


void UStaticMeshComponent::Serialize(FArchive& Ar)
{
	UMeshComponent::Serialize(Ar);

	FString MeshPath = GetStaticMeshAsset();
	Ar.Serialize("ObjStaticMeshAsset", MeshPath);

	if (Ar.IsLoading() && !MeshPath.empty() && GRenderer)
	{
		LoadStaticMesh(GRenderer->GetDevice(), MeshPath);
	}

	TArray<FString> MaterialNames, TexturePaths;

	if (Ar.IsSaving())
	{
		for (uint32 i = 0; i < GetNumMaterials(); ++i)
		{
			FMaterial* Mat = GetMaterial(i);
			MaterialNames.push_back(Mat ? Mat->GetOriginName() : "M_StaticMesh");

			FString TexPath = "";
			auto It = DynamicMaterialOwners.find(i);
			if (It != DynamicMaterialOwners.end() && It->second && It->second->GetMaterialTexture())
				TexPath = It->second->GetMaterialTexture()->AssetPath;
			TexturePaths.push_back(TexPath);
		}
	}

	Ar.SerializeStringArray("SlotMaterials", MaterialNames);
	Ar.SerializeStringArray("SlotTextures", TexturePaths);

	if (Ar.IsLoading())
	{
		FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetStaticMeshAsset() : nullptr;
		for (uint32 i = 0; i < MaterialNames.size(); ++i)
		{
			FString SavedMatName = MaterialNames[i];
			FMaterial* CurrentMat = GetMaterial(i);
			FString CurrentMatName = CurrentMat ? CurrentMat->GetOriginName() : "";

		
			// LoadStaticMesh가 세팅해둔 매테리얼과 세이브 파일의 이름이 다를 때만 덮
			if (CurrentMatName != SavedMatName)
			{
				if (auto Mat = FMaterialManager::Get().FindByName(SavedMatName))
				{
					if (std::shared_ptr<FDynamicMaterial> DynMat = Mat->CreateDynamicMaterial())
					{
						// 기존 텍스처 보존
						std::shared_ptr<FMaterialTexture> OldTex = GDefaultWhiteTexture;
						auto It = DynamicMaterialOwners.find(i);
						if (It != DynamicMaterialOwners.end() && It->second && It->second->GetMaterialTexture())
						{
							OldTex = It->second->GetMaterialTexture();
						}
						DynMat->SetMaterialTexture(OldTex);

						//  메쉬의 원본 .mtl 색상이 있다면 새 매테리얼에도 똑같이 주입
					

						DynamicMaterialOwners[i] = DynMat;
						SetMaterial(i, DynMat.get());
						InitializeUVScrollParameters(i, DynMat);
					}
				}
			}

			// 텍스처 경로가 있다면 로드
			if (i < TexturePaths.size() && !TexturePaths[i].empty() && GRenderer)
			{
				FString DefaultTexPath = "";
				FMaterial* DefaultMat = StaticMesh ? StaticMesh->GetStaticMeshAsset()->GetDefaultMaterial(i) : nullptr;
				if (DefaultMat && DefaultMat->GetMaterialTexture())
				{
					DefaultTexPath = DefaultMat->GetMaterialTexture()->AssetPath;
				}

				// 저장된 텍스처가 원본과 다를 때만 개별(Dynamic) 머티리얼을 생성해서 덮어씌움
				if (TexturePaths[i] != DefaultTexPath)
				{
					LoadTextureToSlot(GRenderer->GetDevice(), TexturePaths[i], i);
				}
			}
		}
	}
	uint32 NumSlots = GetNumMaterials();
	for (uint32 i = 0; i < NumSlots; ++i)
	{
		FString EnabledKey = "UVEnabled_" + std::to_string(i);
		FString SpeedKey = "UVSpeed_" + std::to_string(i);

		if (Ar.IsSaving())
		{
			bool bEnabled = IsUVScrollEnabled(i);
			FVector2 Speed = GetUVScrollSpeed(i);
			FVector SpeedVec3(Speed.X, Speed.Y, 0.0f); // FArchive 저장을 위해 FVector로 임시 변환

			Ar.Serialize(EnabledKey, bEnabled);
			Ar.Serialize(SpeedKey, SpeedVec3);
		}
		else // IsLoading
		{
			if (Ar.Contains(EnabledKey))
			{
				bool bEnabled = false;
				Ar.Serialize(EnabledKey, bEnabled);
				SetUVScrollEnabled(i, bEnabled);
			}

			if (Ar.Contains(SpeedKey))
			{
				FVector SpeedVec3;
				Ar.Serialize(SpeedKey, SpeedVec3);
				SetUVScrollSpeed(i, FVector2(SpeedVec3.X, SpeedVec3.Y)); // 다시 FVector2로 변환해서 세팅
			}
		}
	}
}