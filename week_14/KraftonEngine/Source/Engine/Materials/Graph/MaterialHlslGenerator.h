#pragma once

#include "Materials/Graph/MaterialGraphTypes.h"
#include "Materials/MaterialSourceTypes.h"

struct FMaterialCompileOptions
{
    FString               MaterialPath;
    FString               MaterialGuid;
    EMaterialGraphTarget  Domain            = EMaterialGraphTarget::Surface;
    ERenderPass           RenderPass        = ERenderPass::Opaque;
    EBlendState           BlendState        = EBlendState::Opaque;
    EDepthStencilState    DepthStencilState = EDepthStencilState::Default;
    ERasterizerState      RasterizerState   = ERasterizerState::SolidBackCull;
    bool                  bReceiveLighting  = false; // ParticleMesh 전용 — Ambient + Directional 조명 적용
    // Surface 도메인 라이팅 모델 — Unlit 이면 BaseColor + Emissive 만 출력 (현재 동작). 그 외엔
    // UberLit 의 ForwardLighting.hlsli 를 통해 directional/point/spot + shadow 가 적용된다.
    EMaterialShadingModel ShadingModel      = EMaterialShadingModel::DefaultLit;
};

struct FMaterialCompileResult
{
    FString GeneratedShaderPath;
    FString GeneratedHlsl;

    TMap<FString, FMaterialCompiledParameter> Parameters;
    TMap<FString, FMaterialCompiledTexture>   Textures;
    TArray<FString>                           Errors;
};

class FMaterialHlslGenerator
{
public:
    static bool Generate(const FMaterialGraph& Graph, const FMaterialCompileOptions& Options, FMaterialCompileResult& OutResult);
};