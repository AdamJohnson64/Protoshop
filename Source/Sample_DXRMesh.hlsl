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
void RayGeneration()
{
    float normx = (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    float normy = (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
    RayDesc rayDesc = { float3(0, 1, -3), 0.001, normalize(float3(-1 + normx * 2, 1 - normy * 2, 1)), 1000 };
    RayPayload payload;
    payload.Color = float3(0, 0, 0);
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, payload);
    RTOutput[DispatchRaysIndex().xy] = float4(payload.Color, 1.f);
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