static const float DEFAULT_TMIN = 0.0001;
static const float DEFAULT_TMAX = 1000000000;

struct RayPayload
{
    float3 Color;
};

struct IntersectionAttributes
{
    float3 Normal;
};

RWTexture2D<float4> renderTargetOutput                          : register(u0);
RaytracingAccelerationStructure raytracingAccelerationStructure : register(t0);

cbuffer Constants : register(b0)
{
    float4x4 Transform;
};

[shader("raygeneration")]
void RayGenerationMVPClip()
{
    const float NormalizedX = -1 + 2 * (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    const float NormalizedY = 1 - 2 * (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
    RayDesc rayDesc;
    float4 front = mul(Transform, float4(NormalizedX, NormalizedY, 0, 1));
    front /= front.w;
    float4 back = mul(Transform, float4(NormalizedX, NormalizedY, 1, 1));
    back /= back.w;
    rayDesc.Origin = front.xyz;
    rayDesc.Direction = normalize(back.xyz - front.xyz);
    rayDesc.TMin = DEFAULT_TMIN;
    rayDesc.TMax = DEFAULT_TMAX;
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    renderTargetOutput[DispatchRaysIndex().xy] = float4(rayPayload.Color, 1);
}

[shader("miss")]
void Miss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0.25f, 0.25f, 0.25f);
}

[shader("closesthit")]
void MaterialCheckerboard(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    float3 worldRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float x = worldRayOrigin.x * 4;
    float z = worldRayOrigin.z * 4;
    x -= floor(x);
    z -= floor(z);
    x *= 2;
    z *= 2;
    float checker = ((int)x + (int)z) % 2;
    rayPayload.Color = float3(checker, checker, checker);
}

[shader("closesthit")]
void MaterialRedPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(1, 0, 0);
}