cbuffer Constants
{
    float4x4 TransformPixelToImage;
};

RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

float4 Checkerboard(uint2 pixel)
{
    float modulo = ((uint)pixel.x / 32 + (uint)pixel.y / 32) % 2;
    return lerp(float4(0.25, 0.25, 0.25, 1), float4(0.75, 0.75, 0.75, 1), modulo);
}

float4 Sample(float2 pixel)
{
    float4 checkerboard = Checkerboard((uint2)pixel);
    float2 pos = mul(TransformPixelToImage, float4(pixel.x, pixel.y, 0, 1)).xy;
    uint Width, Height, NumberOfLevels;
    userImage.GetDimensions(0, Width, Height, NumberOfLevels);
    if (pos.x >= 0 && pos.x < (int)Width && pos.y >= 0 && pos.y < (int)Height)
    {
        return userImage.Load(int3(pos.x, pos.y, 0));
    }
    else
    {
        return checkerboard;
    }
}

[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ////////////////////////////////////////////////////////////////////////////////
    // No Supersampling.
    //renderTarget[dispatchThreadId.xy] = Sample((float2)dispatchThreadId);
    //return;
    
    ////////////////////////////////////////////////////////////////////////////////
    // 8x8 (64 TAP) Supersampling.
    const int superCountX = 8;
    const int superCountY = 8;
    const float superSpacingX = 1.0 / superCountX;
    const float superSpacingY = 1.0 / superCountY;
    const float superOffsetX = superSpacingX / 2;
    const float superOffsetY = superSpacingY / 2;
    float4 accumulateSamples;
    for (int superSampleY = 0; superSampleY < superCountY; ++superSampleY)
    {
        for (int superSampleX = 0; superSampleX < superCountX; ++superSampleX)
        {
            accumulateSamples += Sample(float2(
                dispatchThreadId.x + superOffsetX + superSampleX * superSpacingX,
                dispatchThreadId.y + superOffsetY + superSampleY * superSpacingY));
        }
    }
    accumulateSamples /= superCountX * superCountY;
    renderTarget[dispatchThreadId.xy] = accumulateSamples;
 }