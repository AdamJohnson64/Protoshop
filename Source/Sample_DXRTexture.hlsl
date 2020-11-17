static const float DEFAULT_TMIN = 0.001;
static const float DEFAULT_TMAX = 1000000; 

cbuffer Constants : register(b0)
{
    float4x4 Transform;
};

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
Texture2D myTexture : register(t1);

[shader("raygeneration")]
void RayGenerationMVPClip()
{
    float NormalizedX = -1 + 2 * (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    float NormalizedY = 1 - 2 * (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
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
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Basic Checkerboard Albedo.
    float x = (worldHit.x - floor(worldHit.x)) * 2;
    float z = (worldHit.z - floor(worldHit.z)) * 2;
    float blackOrWhite = ((int)x + (int)z) % 2;
    rayPayload.Color = float3(blackOrWhite, blackOrWhite, blackOrWhite);
}

[shader("closesthit")]
void MaterialPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    // Load final color from texture.
    rayPayload.Color = myTexture.Load(uint3(0, 0, 0)).rgb;
}

[shader("intersection")]
void IntersectPlane()
{
    IntersectionAttributes attributes;
    float3 origin = ObjectRayOrigin();
    float3 direction = ObjectRayDirection();
    float4 plane = float4(0,1,0, 0);
    float divisor = dot(plane.xyz, direction);
    float lambda = (plane.w - dot(origin, plane.xyz)) / divisor;
    if (lambda < 0) return;
    float3 intersection = origin + direction * lambda;
    if (intersection.x < -1 || intersection.x > 1 || intersection.z < -1 || intersection.z > 1) return;
    attributes.Normal = plane.xyz;
    ReportHit(lambda, 0, attributes);
}

[shader("intersection")]
void IntersectSphere()
{
    IntersectionAttributes attributes;
    float3 origin = ObjectRayOrigin();
    float3 direction = ObjectRayDirection();
    float a = dot(direction, direction);
    float b = 2 * dot(origin, direction);
    float c = dot(origin, origin) - 1;
    float root = b * b - 4 * a * c;
    if (root < 0) return;
    float solution = sqrt(root);
    float hitFront = (-b - solution) / (2 * a);
    if (hitFront >= 0)
    {
        attributes.Normal = normalize(mul(origin + direction * hitFront, (float3x3)ObjectToWorld3x4()));
        ReportHit(hitFront, 0, attributes);
    }
    float hitBack = (-b + solution) / (2 * a);
    if (hitBack >= 0)
    {
        attributes.Normal = normalize(mul(origin + direction * hitBack, (float3x3)ObjectToWorld3x4()));
        ReportHit(hitBack, 0, attributes);
    }
}