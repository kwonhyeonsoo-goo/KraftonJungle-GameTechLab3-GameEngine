Texture2D<float> PreviousMip : register(t0);
RWTexture2D<float> CurrentMip : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint2 outPos = DTid.xy;
    
    uint2 inPos = outPos * 2;
    
    float d0 = PreviousMip.Load(uint3(inPos, 0));
    float d1 = PreviousMip.Load(uint3(inPos + uint2(1, 0), 0));
    float d2 = PreviousMip.Load(uint3(inPos + uint2(0, 1), 0));
    float d3 = PreviousMip.Load(uint3(inPos + uint2(1, 1), 0));
    
    float maxDepth = max(max(d0, d1), max(d2, d3));
    
    CurrentMip[outPos] = maxDepth;
}