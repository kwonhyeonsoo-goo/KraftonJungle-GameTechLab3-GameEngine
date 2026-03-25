#pragma once
#include "Engine/Core/CoreTypes.h"
#include "World/PrimitiveComponent.h"

class USubUVComponent :
    public UPrimitiveComponent
{
public:
    USubUVComponent(std::wstring filename = L"Assets/Effects/Explosion.PNG");
    bool GetRenderCommand(const FMatrix& viewMatrix, const FMatrix& projMatrix, FRenderCommand& OutCommand) override;
    void Update(float deltatime) override;
    EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }
    std::wstring GetFileName() { return filename; }
    void ReloadTextureResource(std::wstring filename = L"Assets/Effects/Explosion.PNG");

public:
    void UpdateWorldAABB() override;
    bool bIsBillBoard = true;
    float Timer;
    static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_SubUV;
    int CellRows, CellColumns;
    float OffsetLeftX, OffsetUpY, OffsetRightX, OffsetBottomY;
    int PlayRate = 24;
    bool bLoop = true;

private:
    const FUVMeshData* UVMeshData = nullptr;
    int Texture2DId = -1;

    int FrameCount;
    float CellSizeX, CellSizeY;

    int CurrentFrameIndex;
    std::wstring filename;

public:
    DECLARE_CLASS(USubUVComponent, UPrimitiveComponent)
    void UpdateFrame(float deltaTime);
    float currentU, currentV;
    void Serialize(json::JSON& j) const override;
    void Deserialize(const json::JSON& j) override;

private:
    bool RaycastMesh(const FRay& Ray, FHitResult& OutHitResult) override;
    float ElaspedTime;

};