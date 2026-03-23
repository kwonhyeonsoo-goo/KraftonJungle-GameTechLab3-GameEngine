#pragma once
#include "Engine/Core/CoreTypes.h"
#include "World/PrimitiveComponent.h"

class USubUVComponent :
    public UPrimitiveComponent
{
private:
    const FUVMeshData* MeshData = nullptr;
    int Texture2DId = -1;
    float OffsetLeftX, OffsetUpY, OffsetRightX, OffsetBottomY;
    int FrameCount;
    float CellSizeX, CellSizeY;
    int CellCountX, CellCountY;
    int CurrentFrameIndex;
    std::wstring filename;
    bool bLoop = true;
    float PlayRate = 24.f;
    bool bIsBillBoard = true;
    float currentU, currentV;
public:
    void UpdateFrame(float deltaTime);
    USubUVComponent(std::wstring filename = L"Assets/Effects/Explosion.PNG");
    bool GetRenderCommand(const FMatrix& viewMatrix, const FMatrix& projMatrix, FRenderCommand& OutCommand) override;
    void Update(float deltatime) override;

    EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }

private:
    void ReloadTextureResource(std::wstring filename = L"Assets/Effects/Explosion.PNG");

public:
    void UpdateWorldAABB() override;
    float ElaspedTime;
    float Timer;
    static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_SubUV;
};