cbuffer Constants
{
    float4x4 TransformClipToWorld;
    float2 WindowDimensions;
};

RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Form up normalized screen coordinates.
    const float2 Normalized = mad(float2(2, -2) / WindowDimensions, float2(dispatchThreadId.xy), float2(-1, 1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form the world ray.
    float4 front = mul(TransformClipToWorld, float4(Normalized.xy, 0, 1));
    front /= front.w;
    float4 back = mul(TransformClipToWorld, float4(Normalized.xy, 1, 1));
    back /= back.w;
    float3 origin = front.xyz;
    float3 direction = normalize(back.xyz - front.xyz);
    // Calculate the longitude and latitude of the view direction.
    const float PI = 3.1415926535897932384626433832795029;
    direction.y = -direction.y;
    direction.z = -direction.z;
    float r = (1 / PI) * acos(direction.z) / sqrt(direction.x * direction.x + direction.y * direction.y);
    float2 uv = float2((direction.x * r + 1) / 2, (direction.y * r + 1) / 2);
    renderTarget[dispatchThreadId.xy] = userImage.Load(float3(uv.x * 1000, uv.y * 1000, 0)) * 10;
}