#ifndef DOF_COMMON_HLSLI
#define DOF_COMMON_HLSLI

cbuffer DoFBuffer : register(b2)
{
    float FocusDistance;
    float FocusRange;
    float MaxBlurRadius;
    float BokehRadiusThreshold;
    float BokehLumaThreshold;
    float BokehIntensity;
    float2 _DoFPad;
};

static const int DoFRing0Count = 6;
static const int DoFRing1Count = 10;
static const int DoFRing2Count = 16;
static const int DoFRing3Count = 24;
static const float DoFRing0Radius = 0.35f;
static const float DoFRing1Radius = 0.60f;
static const float DoFRing2Radius = 0.80f;
static const float DoFRing3Radius = 1.00f;

static const float DoFBackgroundMinCoCForRadius = 0.15f;
static const float DoFForegroundMinCoCForRadius = 0.20f;
static const float DoFBackgroundCenterWeight = 0.20f;
static const float DoFForegroundCenterWeight = 0.25f;
static const float DoFMinFocusRange = 1.0f;
static const float DoFMaxSignedCoC = 1.0f;
static const float DoFFocusProtectScale = 2.0f;
static const float DoFNearerSampleRejectWeight = 0.15f;
static const float DoFNearerSampleRejectBias = 1.0f;
static const float DoFMinAccumulatedWeight = 1.0e-4f;

static const int DoFMaxBokehSearchRadius = 64;
static const float DoFBokehMinFeather = 1.0f;
static const float DoFBokehFeatherScale = 0.04f;

static const float2 DoFRing0[DoFRing0Count] =
{
    float2( 0.350f,  0.000f), float2( 0.175f,  0.303f), float2(-0.175f,  0.303f),
    float2(-0.350f,  0.000f), float2(-0.175f, -0.303f), float2( 0.175f, -0.303f)
};

static const float2 DoFRing1[DoFRing1Count] =
{
    float2( 0.600f,  0.000f), float2( 0.485f,  0.353f), float2( 0.185f,  0.571f),
    float2(-0.185f,  0.571f), float2(-0.485f,  0.353f), float2(-0.600f,  0.000f),
    float2(-0.485f, -0.353f), float2(-0.185f, -0.571f), float2( 0.185f, -0.571f),
    float2( 0.485f, -0.353f)
};

static const float2 DoFRing2[DoFRing2Count] =
{
    float2( 0.800f,  0.000f), float2( 0.739f,  0.306f), float2( 0.566f,  0.566f), float2( 0.306f,  0.739f),
    float2( 0.000f,  0.800f), float2(-0.306f,  0.739f), float2(-0.566f,  0.566f), float2(-0.739f,  0.306f),
    float2(-0.800f,  0.000f), float2(-0.739f, -0.306f), float2(-0.566f, -0.566f), float2(-0.306f, -0.739f),
    float2( 0.000f, -0.800f), float2( 0.306f, -0.739f), float2( 0.566f, -0.566f), float2( 0.739f, -0.306f)
};

static const float2 DoFRing3[DoFRing3Count] =
{
    float2( 1.000f,  0.000f), float2( 0.966f,  0.259f), float2( 0.866f,  0.500f), float2( 0.707f,  0.707f),
    float2( 0.500f,  0.866f), float2( 0.259f,  0.966f), float2( 0.000f,  1.000f), float2(-0.259f,  0.966f),
    float2(-0.500f,  0.866f), float2(-0.707f,  0.707f), float2(-0.866f,  0.500f), float2(-0.966f,  0.259f),
    float2(-1.000f,  0.000f), float2(-0.966f, -0.259f), float2(-0.866f, -0.500f), float2(-0.707f, -0.707f),
    float2(-0.500f, -0.866f), float2(-0.259f, -0.966f), float2( 0.000f, -1.000f), float2( 0.259f, -0.966f),
    float2( 0.500f, -0.866f), float2( 0.707f, -0.707f), float2( 0.866f, -0.500f), float2( 0.966f, -0.259f)
};

float DoFHighlightLuma(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float DoFRingSampleWeight(float ringRadius)
{
    return saturate(1.0f - ringRadius * ringRadius * 0.35f);
}

float DoFBokehCandidateMask(float coc, float3 color)
{
    float radius = abs(coc) * MaxBlurRadius;
    float luma = DoFHighlightLuma(color);
    return (radius >= BokehRadiusThreshold && luma >= BokehLumaThreshold) ? 1.0f : 0.0f;
}

float3 DoFRemoveBokehEnergyForFallback(float coc, float3 color)
{
    float candidate = DoFBokehCandidateMask(coc, color);
    float3 nonBokehColor = min(color, BokehLumaThreshold);
    return lerp(color, nonBokehColor, candidate);
}

float2 DoFGetSceneTexel()
{
    uint width, height;
    SceneColorTexture.GetDimensions(width, height);
    return 1.0f / max(float2(width, height), float2(1.0f, 1.0f));
}

#endif
