#include "BillBoardComponent.h"
#include "World/Mesh/MeshManager.h"
#include "Engine/EngineServices/EngineServices.h"
#include "Engine/Render/Resource/RenderResourceManager.h"

DEFINE_CLASS(UBillBoardComponent, UPrimitiveComponent)
REGISTER_FACTORY(UBillBoardComponent)

UBillBoardComponent::UBillBoardComponent() {
	LocalExtents = { 0.5f, 0.5f, 0.5f };
	UVMeshData = &FMeshManager::GetUVRect();
}

void UBillBoardComponent::LoadTexture(const std::wstring& InTexturePath) { 
	TextureID = FEngineServices::GetResourceManager()->CreateTexture(InTexturePath);
}

void UBillBoardComponent::UpdateWorldAABB() {
	FVector WorldCenter = GetWorldLocation();
	FVector Extents = LocalExtents;
	BoundingBox.WorldAABBMaxLocation = WorldCenter + Extents;
	BoundingBox.WorldAABBMinLocation = WorldCenter - Extents;
}

bool UBillBoardComponent::GetRenderCommand(const FMatrix& ViewMatrix, const FMatrix& ProjMatrix, FRenderCommand& OutCommand) {
	if (!UVMeshData || !bIsVisible || TextureID == -1) { return false; }

	// Project camera right onto the world XY plane and renormalize.
	// This prevents tilt when the camera is pitched (M[0][2] would otherwise bleed into world Z).


	// Get CameraPosition from ViewMatrix / Carmera World를 가져올 쉬운 방법이 필요함
	FMatrix Scale = FMatrix::MakeScaleMatrix(FVector(Size, Size, Size));
	FMatrix Translation = FMatrix::MakeTranslationMatrix(GetWorldLocation());

	float Tx = ViewMatrix.M[3][0];
	float Ty = ViewMatrix.M[3][1];
	float Tz = ViewMatrix.M[3][2];

	FVector CameraPosition = FVector(
		-(Tx * ViewMatrix.M[0][0] + Ty * ViewMatrix.M[0][1] + Tz * ViewMatrix.M[0][2]),
		-(Tx * ViewMatrix.M[1][0] + Ty * ViewMatrix.M[1][1] + Tz * ViewMatrix.M[1][2]),
		-(Tx * ViewMatrix.M[2][0] + Ty * ViewMatrix.M[2][1] + Tz * ViewMatrix.M[2][2])
	);

	FVector BillBoardForward = CameraPosition - GetWorldLocation(); BillBoardForward.Normalize();
	FVector BillBoardUp = FVector(0.f, 0.f, 1.f);
	FVector BillBoardRight = BillBoardUp.Cross(BillBoardForward); BillBoardRight.Normalize();  BillBoardRight;


	FMatrix BillBoarding{
		BillBoardForward.X, BillBoardForward.Y, BillBoardForward.Z , 0.f , 
		BillBoardRight.X, BillBoardRight.Y, BillBoardRight.Z , 0.f ,
		BillBoardUp.X, BillBoardUp.Y, BillBoardUp.Z , 0.f ,
		0, 0, 0, 1
	};

	OutCommand.Type = ERenderCommandType::SubUV;
	OutCommand.TransformConstants.Model = Scale * BillBoarding * Translation; // SRT
	OutCommand.TransformConstants.View = ViewMatrix;
	OutCommand.TransformConstants.Projection = ProjMatrix;
	OutCommand.SubUVConstants = { 0, 0, 1, 1 };
	OutCommand.TextureID = TextureID;
	return true;
}

bool UBillBoardComponent::RaycastMesh(const FRay& Ray, FHitResult& OutHitResult) {
	if (!UVMeshData || UVMeshData->Indices.empty())
	{
		return false;
	}

	FMatrix invWorld = CachedWorldMatrix.GetInverse();
	FVector localOrigin = invWorld.TransformPositionWithW(Ray.Origin);
	FVector localDirection = invWorld.TransformVector(Ray.Direction);
	localDirection.Normalize();

	bool bHit = false;
	float closestT = FLT_MAX;

	for (size_t i = 0; i < UVMeshData->Indices.size(); i += 3)
	{
		FVector v0 = UVMeshData->Vertices[UVMeshData->Indices[i]].Position;
		FVector v1 = UVMeshData->Vertices[UVMeshData->Indices[i + 1]].Position;
		FVector v2 = UVMeshData->Vertices[UVMeshData->Indices[i + 2]].Position;

		float t = 0.0f;

		if (IntersectTriangle(localOrigin, localDirection, v0, v1, v2, t))
		{
			if (t > 0.0f && t < closestT)
			{
				closestT = t;
				bHit = true;
				OutHitResult.FaceIndex = i;
			}
		}
	}

	OutHitResult.bHit = bHit;

	if (bHit)
	{
		FVector localHitPoint = localOrigin + (localDirection * closestT);
		FVector worldHitPoint = CachedWorldMatrix.TransformPositionWithW(localHitPoint);
		OutHitResult.Distance = FVector::Distance(Ray.Origin, worldHitPoint);
		OutHitResult.HitComponent = this;
		return true;
	}

	return false;
}

// Because, icons do not have a real volume
const FBoundingBox& UBillBoardComponent::GetBoundingBox() const {
	FBoundingBox FakeBoundingBox = { FVector(0, 0, 0), FVector(0, 0, 0) };
	return FakeBoundingBox;
}