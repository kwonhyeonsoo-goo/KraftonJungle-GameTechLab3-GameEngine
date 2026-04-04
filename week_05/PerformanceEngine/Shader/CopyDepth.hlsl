cbuffer CullingParams : register(b0)
{
    row_major float4x4 ViewProjection;
    float2 RenderTargetSize;
};

Texture2D<float> MainDepth : register(t0);
RWTexture2D<float> HiZMip0 : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= 1024 || DTid.y >= 1024)
        return;
    
    float2 uv = (float2(DTid.xy) + 0.5f) / 1024.0f;
    uint2 mainPos = uint2(uv * RenderTargetSize);
    
    float d = MainDepth.Load(uint3(mainPos, 0));
    
    HiZMip0[DTid.xy] = d;
}
