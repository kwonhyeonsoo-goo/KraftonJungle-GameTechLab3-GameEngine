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

public:
    void UpdateWorldAABB() override;
    bool bIsBillBoard = true;
    float Timer;
    static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_SubUV;

private:
    const FUVMeshData* UVMeshData = nullptr;
    int Texture2DId = -1;
    float OffsetLeftX, OffsetUpY, OffsetRightX, OffsetBottomY;
    int FrameCount;
    float CellSizeX, CellSizeY;
    int CellCountX, CellCountY;
    int CurrentFrameIndex;
    std::wstring filename;
    bool bLoop = true;
    float PlayRate = 24.f;

public:
    DECLARE_CLASS(USubUVComponent, UPrimitiveComponent)
    void UpdateFrame(float deltaTime);
    float currentU, currentV;
    float ElaspedTime;

private:
    void ReloadTextureResource(std::wstring filename = L"Assets/Effects/Explosion.PNG");
    bool RaycastMesh(const FRay& Ray, FHitResult& OutHitResult) override;
    float ElaspedTime;

};