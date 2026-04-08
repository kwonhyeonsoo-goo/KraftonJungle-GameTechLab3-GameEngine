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
	// YZ 평면 (X=0) — MakeBillboard이 Y→Right, Z→Up으로 변환
	V0.Position = FVector(0.0f, -0.5f, 0.5f);  V0.UV = FVector2(0.0f, 0.0f);
	V1.Position = FVector(0.0f, 0.5f, 0.5f);  V1.UV = FVector2(1.0f, 0.0f);
	V2.Position = FVector(0.0f, 0.5f, -0.5f);  V2.UV = FVector2(1.0f, 1.0f);
	V3.Position = FVector(0.0f, -0.5f, -0.5f);  V3.UV = FVector2(0.0f, 1.0f);

	FVector Normal(1.0f, 0.0f, 0.0f);  // +X (Forward 방향)
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

	
		extern ENGINE_API FRenderer* GRenderer;
		if (GRenderer)
		{
			auto& RSM = GRenderer->GetRenderStateManager();

			
			FRasterizerStateOption RasterOpt = DynamicMaterial->GetRasterizerOption();
			RasterOpt.CullMode = D3D11_CULL_NONE;
			DynamicMaterial->SetRasterizerOption(RasterOpt);
			DynamicMaterial->SetRasterizerState(RSM->GetOrCreateRasterizerState(RasterOpt));

	
			FBlendStateOption BlendOpt;
			BlendOpt.BlendEnable = true;
			BlendOpt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			BlendOpt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			BlendOpt.BlendOp = D3D11_BLEND_OP_ADD;
			BlendOpt.SrcBlendAlpha = D3D11_BLEND_ONE;
			BlendOpt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			BlendOpt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			DynamicMaterial->SetBlendOption(BlendOpt);
			DynamicMaterial->SetBlendState(RSM->GetOrCreateBlendState(BlendOpt));
		}

	
		if (ID3D11ShaderResourceView* LoadedSRV = FAssetManager::Get().LoadTexture(Device, TexturePath))
		{
			auto MatTex = std::make_shared<FMaterialTexture>();
			MatTex->TextureSRV = LoadedSRV;
			MatTex->AssetPath = TexturePath;

			D3D11_SAMPLER_DESC SampDesc = {};
			SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			Device->CreateSamplerState(&SampDesc, &MatTex->SamplerState);

			DynamicMaterial->SetMaterialTexture(MatTex);
		}
		SetMaterial(DynamicMaterial.get());
	}
	MarkTransformDirty();
}
