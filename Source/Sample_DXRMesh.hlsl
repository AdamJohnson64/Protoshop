struct HitInfo
{
    float4 ColorAndLambda;
};

struct Attributes
{
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void RayGeneration()
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    uint2 LaunchDimensions = DispatchRaysDimensions().xy;
    RayDesc ray;
    ray.Origin = float3(0, 1, -3);
    float normx = (float)LaunchIndex.x / (float)LaunchDimensions.x;
    float normy = (float)LaunchIndex.y / (float)LaunchDimensions.y;
    ray.Direction = normalize(float3(-1 + normx * 2, 1 - normy * 2, 1));
    ray.TMin = 0.001f;
    ray.TMax = 1000;
    HitInfo payload;
    payload.ColorAndLambda = float4(0, 0, 0, 1);
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    RTOutput[LaunchIndex.xy] = float4(payload.ColorAndLambda.rgb, 1.f);
}

[shader("miss")]
void Miss(inout HitInfo payload)
{
    payload.ColorAndLambda = float4(0.25f, 0.25f, 0.25f, 1);
}

[shader("closesthit")]
void MaterialCheckerboard(inout HitInfo payload, Attributes attrib)
{
    float3 worldRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float x = worldRayOrigin.x * 4;
    float z = worldRayOrigin.z * 4;
    x -= floor(x);
    z -= floor(z);
    x *= 2;
    z *= 2;
    float checker = ((int)x + (int)z) % 2;
    payload.ColorAndLambda = float4(checker, checker, checker, 1);
}

[shader("closesthit")]
void MaterialRedPlastic(inout HitInfo payload, Attributes attrib)
{
    payload.ColorAndLambda = float4(1, 0, 0, 1);
}