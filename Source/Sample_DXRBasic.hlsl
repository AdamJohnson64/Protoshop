struct HitInfo
{
    float3 Color;
};

struct Attributes
{
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void raygeneration()
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    uint2 LaunchDimensions = DispatchRaysDimensions().xy;
    RayDesc ray;
    ray.Origin = float3(LaunchIndex.x + 0.5f, LaunchIndex.y + 0.5f, -1); // +0.5f for center-pixel sampling.
    ray.Direction = float3(0, 0, 1);
    ray.TMin = 0.001f;
    ray.TMax = 1000;
    HitInfo payload;
    payload.Color = float3(0, 0, 0);
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    RTOutput[LaunchIndex.xy] = float4(payload.Color, 1);
}

[shader("closesthit")]
void closesthit(inout HitInfo payload, Attributes attrib)
{
    payload.Color = float3(0, 1, 0);
}

[shader("miss")]
void miss(inout HitInfo payload)
{
    payload.Color = float3(1, 0, 0);
}