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
void MaterialRedPlastic(inout HitInfo payload, Attributes attrib)
{
    payload.ColorAndLambda = float4(1, 0, 0, 1);
}

[shader("intersection")]
void IntersectSphere()
{
    Attributes attributes;
    float3 origin = ObjectRayOrigin();
    float3 direction = ObjectRayDirection();
    float a = dot(direction, direction);
    float b = 2 * dot(origin, direction);
    float c = dot(origin, origin) - 1;
    float root = b * b - 4 * a * c;
    if (root < 0) return;
    float solution = sqrt(root) / (2 * a);
    float hitFront = -b - solution;
    float rayLambdaStart = RayTCurrent();
    if (hitFront <= rayLambdaStart) ReportHit(hitFront, 0, attributes);
    float hitBack = -b + solution;
    if (hitBack <= rayLambdaStart) ReportHit(hitBack, 0, attributes);
}