#include "SubUV.h"
#include "Engine/EngineServices/EngineServices.h"
#include "Engine/Render/Resource/RenderResourceManager.h"
#include "World/Mesh/MeshManager.h"


void USubUVComponent::UpdateFrame(float deltaTime)
{
	ElaspedTime += deltaTime;

	if (ElaspedTime >= Timer) {
		ElaspedTime = 0;
		CurrentFrameIndex = (++CurrentFrameIndex)%FrameCount;
	}
	currentU = OffsetLeftX +(CurrentFrameIndex % CellCountX) * CellSizeX;
	currentV = OffsetUpY +(CurrentFrameIndex / CellCountX) * CellSizeY;

	//if Billboard면 
}

USubUVComponent::USubUVComponent(std::wstring filename)
{
	this->filename = filename;
	ElaspedTime = 0;
	OffsetLeftX = 24.f / 857.f; OffsetUpY = 12.f / 852.f;
	OffsetRightX = 17.f/857.f, OffsetBottomY = 22.f / 852.f;

	CellCountX = CellCountY = 6;
	FrameCount = CellCountX * CellCountY;
	CurrentFrameIndex = 0;
	Texture2DId = FEngineServices::GetResourceManager()->CreateTexture(this->filename);
	Timer = 1.f / PlayRate;

	CellSizeX = (1.f - OffsetLeftX - OffsetRightX) / CellCountX;
	CellSizeY = (1.f - OffsetUpY - OffsetBottomY) / CellCountY;

	currentU = OffsetLeftX; currentV = OffsetUpY;
	MeshData = &FMeshManager::GetUVRect();
}

bool USubUVComponent::GetRenderCommand(const FMatrix& viewMatrix, const FMatrix& projMatrix, FRenderCommand& OutCommand) {
	if (!MeshData || !bIsVisible ) {
		return false;
	}

	if (Texture2DId == -1) {
		return false;
	}
	OutCommand.Type = ERenderCommandType::SubUV;
	OutCommand.TransformConstants.Model = GetWorldMatrix();
	OutCommand.TransformConstants.View = viewMatrix;
	OutCommand.TransformConstants.Projection = projMatrix;
	OutCommand.TextureID = Texture2DId;
	OutCommand.SubUVConstants.cellSizeWidth = CellSizeX;
	OutCommand.SubUVConstants.cellSizeheight = CellSizeY;
	OutCommand.SubUVConstants.startX = currentU;
	OutCommand.SubUVConstants.startY = currentV;
	
	return UPrimitiveComponent::GetRenderCommand(viewMatrix, projMatrix, OutCommand);
}
//
void USubUVComponent::Update(float deltatime)
{
	UpdateFrame(deltatime);
}

void USubUVComponent::ReloadTextureResource(std::wstring filename)
{
	Texture2DId = FEngineServices::GetResourceManager()->CreateTexture(this->filename);

}

void USubUVComponent::UpdateWorldAABB()
{
	FVector forward = GetForwardVector();
	FVector right = GetRightVector();
	FVector up = GetUpVector();
	FVector Scale = GetScaleVector();

	float halfX = LocalExtents.X * Scale.X;
	float halfY = LocalExtents.Y * Scale.Y;
	float halfZ = LocalExtents.Z * Scale.Z;

	FVector extent = {
		abs(forward.X) * halfX + abs(right.X) * halfY + abs(up.X) * halfZ,
		abs(forward.Y) * halfX + abs(right.Y) * halfY + abs(up.Y) * halfZ,
		abs(forward.Z) * halfX + abs(right.Z) * halfY + abs(up.Z) * halfZ
	};

	extent.Z += LocalExtents.Z * 0.5f;

	FVector WorldCenter = GetWorldLocation();
	BoundingBox.WorldAABBMinLocation = WorldCenter - extent;
	BoundingBox.WorldAABBMaxLocation = WorldCenter + extent;
}
