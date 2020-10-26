struct RayPayload
{
    float3 Color;
};

struct IntersectionAttributes
{
    float3 Normal;
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void raygeneration()
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    uint2 LaunchDimensions = DispatchRaysDimensions().xy;
    RayDesc rayDesc = { float3(LaunchIndex.x + 0.5f, LaunchIndex.y + 0.5f, -1), 0.001, float3(0, 0, 1), 1000 };
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    RTOutput[LaunchIndex.xy] = float4(rayPayload.Color, 1);
}

[shader("closesthit")]
void closesthit(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(0, 1, 0);
}

[shader("miss")]
void miss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(1, 0, 0);
}