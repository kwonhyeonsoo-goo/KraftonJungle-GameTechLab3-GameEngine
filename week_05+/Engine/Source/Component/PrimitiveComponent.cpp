#include "PrimitiveComponent.h"
#include "Object/Class.h"
#include "Debug/EngineLog.h"
#include "Serializer/Archive.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Material.h"
IMPLEMENT_RTTI(UPrimitiveComponent, USceneComponent)

FBoxSphereBounds UPrimitiveComponent::GetWorldBounds() const
{
	FVector Center = GetWorldLocation();

	if (!Primitive)
	{
		return { Center, 1, FVector(1, 1, 1) };
	}

	FMeshData* MeshData = Primitive->GetMeshData();

	if (MeshData)
	{
		FVector LocalBoxExtent = (MeshData->GetMaxCoord() - MeshData->GetMinCoord()) * 0.5;
		Center = GetWorldTransform().TransformPosition(MeshData->GetCenterCoord());
		FVector S = GetWorldTransform().GetScaleVector();
		FVector ScaledExtent = FVector::Multiply(LocalBoxExtent, S);
		FMatrix AbsR = FMatrix::Abs(GetWorldTransform().GetRotationMatrix());

		FVector WorldBoxExtent;
		// S * R (row-major)
		WorldBoxExtent.X = AbsR.M[0][0] * ScaledExtent.X
			+ AbsR.M[1][0] * ScaledExtent.Y 
			+ AbsR.M[2][0] * ScaledExtent.Z;

		WorldBoxExtent.Y = AbsR.M[0][1] * ScaledExtent.X
			+ AbsR.M[1][1] * ScaledExtent.Y
			+ AbsR.M[2][1] * ScaledExtent.Z;

		WorldBoxExtent.Z = AbsR.M[0][2] * ScaledExtent.X
			+ AbsR.M[1][2] * ScaledExtent.Y
			+ AbsR.M[2][2] * ScaledExtent.Z;

		return { Center, WorldBoxExtent.Size(), WorldBoxExtent };
	}

	/** MeshData 가 없을 때 어떻게 처리할 지는 생각이 필요할듯 */
	return { Center, 1, FVector(1, 1, 1) };
}

void UPrimitiveComponent::UpdateLocalBound()
{
	FMeshData* MeshData = Primitive->GetMeshData();

	if (MeshData)
	{
		MeshData->UpdateLocalBound();
	}
}

void UPrimitiveComponent::Serialize(FArchive& Ar)
{
	// 1. 부모(USceneComponent) 직렬화 먼저 호출! (매우 중요)
	// (Transform(위치/회전/스케일) 정보는 USceneComponent가 알아서 저장하게 될 겁니다.)
	USceneComponent::Serialize(Ar);

	// 2. 내 데이터(매테리얼, 바운드 디버그 여부 등) 직렬화
	if (Ar.IsSaving())
	{
		// 매테리얼 이름 저장
		FString MatName = Material ? Material->GetOriginName() : "";
		Ar.Serialize("Material", MatName);

		// 기본 도형(Primitive) 이름 저장 (Cube, Sphere 등)
		FString PrimName = GetPrimitiveFileName();
		Ar.Serialize("PrimitiveFileName", PrimName);

		// 디버그 바운드 박스 플래그 저장
		Ar.Serialize("bDrawDebugBounds", bDrawDebugBounds);
	}
	else // IsLoading()
	{
		// 매테리얼 이름 불러오기 및 렌더러 등록
		if (Ar.Contains("Material"))
		{
			FString MatName;
			Ar.Serialize("Material", MatName);
			if (!MatName.empty())
			{
				if (auto LoadedMat = FMaterialManager::Get().FindByName(MatName))
				{
					SetMaterial(LoadedMat.get());
				}
			}
		}

		// (참고: PrimitiveFileName 복원 로직은 엔진의 기본 도형 생성 구조에 맞춰서 
		// 나중에 FPrimitiveFactory 같은 곳에서 불러와 세팅해주면 됩니다.)

		// 디버그 바운드 박스 플래그 불러오기
		if (Ar.Contains("bDrawDebugBounds"))
		{
			Ar.Serialize("bDrawDebugBounds", bDrawDebugBounds);
		}
	}
}
