struct HitInfo
{
    float4 ColorAndLambda;
};

struct Attributes
{
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

struct VertexAttributes
{
    float3 position;
};

[shader("raygeneration")]
void raygeneration()
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    uint2 LaunchDimensions = DispatchRaysDimensions().xy;
    RayDesc ray;
    ray.Origin = float3(LaunchIndex.x, LaunchIndex.y, -1);
    ray.Direction = float3(0, 0, 1);
    ray.TMin = 0.001f;
    ray.TMax = 1000;
    HitInfo payload;
    payload.ColorAndLambda = float4(0, 0, 0, 1);
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    RTOutput[LaunchIndex.xy] = float4(payload.ColorAndLambda.rgb, 1.f);
}

[shader("closesthit")]
void closesthit(inout HitInfo payload, Attributes attrib)
{
    payload.ColorAndLambda = float4(0, 1, 0, 1);
}

[shader("miss")]
void miss(inout HitInfo payload)
{
    payload.ColorAndLambda = float4(1, 0, 0, 1);
}