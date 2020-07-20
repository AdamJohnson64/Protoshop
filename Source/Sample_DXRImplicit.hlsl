cbuffer Constants : register(b0)
{
    float4x4 transform;
};

struct HitInfo
{
    float4 ColorAndLambda;
};

struct Attributes
{
    float4 Normal;
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void RayGenerationDebug()
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

[shader("raygeneration")]
void RayGenerationRasterMatch()
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    uint2 LaunchDimensions = DispatchRaysDimensions().xy;
    float normx = (float)LaunchIndex.x / (float)LaunchDimensions.x;
    float normy = (float)LaunchIndex.y / (float)LaunchDimensions.y;
    RayDesc ray;
    float4 front = mul(float4(-1 + 2 * normx, 1 - 2 * normy, 0, 1), transform);
    front /= front.w;
    float4 back = mul(float4(-1 + 2 * normx, 1 - 2 * normy, 1, 1), transform);
    back /= back.w;
    ray.Origin = front.xyz;
    ray.Direction = normalize(back.xyz - front.xyz);
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

float3 reflect(float3 i, float3 n)
{
     return i - 2 * n * dot(i, n);
}

float schlick(float3 incident, float3 normal, float ior1, float ior2)
{
	float coeff = (ior1 - ior2) / (ior1 + ior2);
	coeff = coeff * coeff;
	return coeff + (1 - coeff) * pow((1 - dot(-incident, normal)), 5);
}

[shader("closesthit")]
void MaterialCheckerboard(inout HitInfo payload, Attributes attrib)
{
    const float3 vectorLight = normalize(float3(1, 1, -1));
    const float3 worldRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Basic Checkerboard Albedo.
    float3 colorAlbedo;
    {
        float x = worldRayOrigin.x * 4;
        float z = worldRayOrigin.z * 4;
        x -= floor(x);
        z -= floor(z);
        x *= 2;
        z *= 2;
        float blackOrWhite = ((int)x + (int)z) % 2;
        colorAlbedo = float3(blackOrWhite, blackOrWhite, blackOrWhite);
    }
    // Basic Dot-Product Lighting.
    float3 colorDiffuse;
    {
        float light = max(0, dot(attrib.Normal.xyz, vectorLight));
        colorDiffuse = colorAlbedo * light;
    }
    // Schlick Fresnel Reflection.
    float fresnel = schlick(WorldRayDirection(), attrib.Normal.xyz, 1, 1.5);
    if (fresnel < 0.05)
    {
        payload.ColorAndLambda = float4(colorDiffuse, 1);
        return;
    }
    // Basic Reflection.
    float3 colorReflect;
    {
        float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
        RayDesc ray = { worldRayOrigin + vectorReflect * 0.0001, 0, vectorReflect, 100000 };
        HitInfo payloadReflect;
        payloadReflect.ColorAndLambda = float4(0, 0, 0, 1);
        TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, payloadReflect);
        colorReflect = payloadReflect.ColorAndLambda.xyz;
    }
    // Basic Specular Reflection.
    float3 colorSpecular;
    {
        float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
        RayDesc ray = { worldRayOrigin + vectorReflect * 0.0001, 0, vectorReflect, 100000 };
        HitInfo payloadReflect;
        payloadReflect.ColorAndLambda = float4(0, 0, 0, 1);
        TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, payloadReflect);
        colorSpecular = payloadReflect.ColorAndLambda.xyz;
    }
    // Final Color.
    payload.ColorAndLambda = float4(lerp(colorDiffuse, colorSpecular, fresnel), 1);
}

[shader("closesthit")]
void MaterialRedPlastic(inout HitInfo payload, Attributes attrib)
{
    const float3 vectorLight = normalize(float3(1, 1, -1));
    const float3 worldRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Solid Red Albedo.
    float3 colorAlbedo = float3(1, 0, 0);
    // Basic Dot-Product Lighting.
    float3 colorDiffuse;
    {
        float light = max(0, dot(attrib.Normal.xyz, vectorLight));
        colorDiffuse = colorAlbedo * light;
    }
    // Schlick Fresnel Reflection.
    float fresnel = schlick(WorldRayDirection(), attrib.Normal.xyz, 1, 1.3);
    if (fresnel < 0.05)
    {
        payload.ColorAndLambda = float4(colorDiffuse, 1);
        return;
    }
    // Basic Specular Reflection.
    float3 colorSpecular;
    {
        float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
        RayDesc ray = { worldRayOrigin + vectorReflect * 0.0001, 0, vectorReflect, 100000 };
        HitInfo payloadReflect;
        payloadReflect.ColorAndLambda = float4(0, 0, 0, 1);
        TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, payloadReflect);
        colorSpecular = payloadReflect.ColorAndLambda.xyz;
    }
    // Final Color.
    payload.ColorAndLambda = float4(lerp(colorDiffuse, colorSpecular, fresnel), 1);
}

[shader("intersection")]
void IntersectPlane()
{
    Attributes attributes;
    float3 origin = ObjectRayOrigin();
    float3 direction = ObjectRayDirection();
    float4 plane = float4(0,1,0, 0);
    float divisor = dot(plane.xyz, direction);
    float lambda = (plane.w - dot(origin, plane.xyz)) / divisor;
    if (lambda < 0) return;
    float3 intersection = origin + direction * lambda;
    if (intersection.x < -1 || intersection.x > 1 || intersection.z < -1 || intersection.z > 1) return;
    attributes.Normal = float4(plane.xyz, 0);
    ReportHit(lambda, 0, attributes);
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
    float solution = sqrt(root);
    float hitFront = (-b - solution) / (2 * a);
    if (hitFront >= 0)
    {
        attributes.Normal = float4(normalize(mul(origin + direction * hitFront, (float3x3)ObjectToWorld3x4())), 0);
        ReportHit(hitFront, 0, attributes);
    }
    float hitBack = (-b + solution) / (2 * a);
    if (hitBack >= 0)
    {
        attributes.Normal = float4(normalize(mul(origin + direction * hitBack, (float3x3)ObjectToWorld3x4())), 0);
        ReportHit(hitBack, 0, attributes);
    }
}