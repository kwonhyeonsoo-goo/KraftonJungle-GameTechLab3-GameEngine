#include "BillboardComponent.h"
#include "Object/Class.h"
#include "Serializer/Archive.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/PrimitiveVertex.h"
#include "Renderer/MaterialManager.h"
#include "Asset/AssetManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/Material.h"
#include "Math/MathUtility.h"
IMPLEMENT_RTTI(UBillboardComponent, UPrimitiveComponent)

void UBillboardComponent::Initialize()
{
	bDrawDebugBounds = false;
	BillboardMesh = std::make_shared<FMeshData>();
	BillboardMesh->Topology = EMeshTopology::EMT_TriangleList;

	FPrimitiveVertex V0, V1, V2, V3;
	V0.Position = FVector(-0.5f, 0.5f, 0.0f);   V0.UV = FVector2(0.0f, 0.0f);
	V1.Position = FVector(0.5f, 0.5f, 0.0f);    V1.UV = FVector2(1.0f, 0.0f);
	V2.Position = FVector(0.5f, -0.5f, 0.0f);   V2.UV = FVector2(1.0f, 1.0f);
	V3.Position = FVector(-0.5f, -0.5f, 0.0f);  V3.UV = FVector2(0.0f, 1.0f);

	FVector Normal(0.0f, 0.0f, -1.0f);
	V0.Normal = V1.Normal = V2.Normal = V3.Normal = Normal;
	V0.Color = V1.Color = V2.Color = V3.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

	BillboardMesh->Vertices = { V0, V1, V2, V3 };
	BillboardMesh->Indices = { 0, 1, 2, 0, 2, 3 };

	BillboardMesh->UpdateLocalBound();
	BillboardMesh->MarkDirty();
}



FBoxSphereBounds UBillboardComponent::GetWorldBounds() const
{
	const FVector Center = GetWorldLocation();
	const FVector WorldScale = GetWorldTransform().GetScaleVector();
	const float MaxScale = FMath::Max(Size.X * WorldScale.X, Size.Y * WorldScale.Y) * 0.5f;
	const FVector BoxExtent(MaxScale, MaxScale, MaxScale);
	return { Center, BoxExtent.Size(), BoxExtent };
}

void UBillboardComponent::Serialize(FArchive& Ar)
{
	UPrimitiveComponent::Serialize(Ar);

	if (Ar.IsSaving())
	{
		Ar.Serialize("TexturePath", TexturePath);
		Ar.Serialize("Size", Size);
	}
	else // IsLoading
	{
		if (Ar.Contains("TexturePath")) { Ar.Serialize("TexturePath", TexturePath); }
		if (Ar.Contains("Size")) { Ar.Serialize("Size", Size); }

		extern ENGINE_API class FRenderer* GRenderer;
		if (!TexturePath.empty() && GRenderer)
		{
			SetTexturePath(GRenderer->GetDevice(), TexturePath);
		}
	}
}

void UBillboardComponent::SetTexturePath(ID3D11Device* Device, const FString& InPath)
{
	TexturePath = InPath;
	if (TexturePath.empty()) return;

	if (auto BaseMat = FMaterialManager::Get().FindByName("M_Default_Texture"))
	{
		DynamicMaterial = BaseMat->CreateDynamicMaterial();

		// AssetManager를 통해 텍스처 로드 (raw pointer 반환)
		if (ID3D11ShaderResourceView* LoadedSRV = FAssetManager::Get().LoadTexture(Device, TexturePath))
		{
			//  에러 수정: 날것의 SRV를 FMaterialTexture 구조체로 안전하게 래핑합니다.
			auto MatTex = std::make_shared<FMaterialTexture>();
			MatTex->TextureSRV = LoadedSRV;
			MatTex->AssetPath = TexturePath;

			// 래핑된 객체를 머티리얼에 주입
			DynamicMaterial->SetMaterialTexture(MatTex);
		}
		SetMaterial(DynamicMaterial.get());
	}
	MarkTransformDirty();
}
