#define MAXIMUM_RAY_RECURSION_DEPTH 2

cbuffer Constants : register(b0)
{
    float4x4 transform;
};

struct HitInfo
{
    float4 ColorAndLambda;
    int RecursionClamp;
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
    payload.RecursionClamp = MAXIMUM_RAY_RECURSION_DEPTH - 1; // NOTE: Primary eye ray counts as one recursion level.
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
    payload.RecursionClamp = MAXIMUM_RAY_RECURSION_DEPTH - 1; // NOTE: Primary eye ray counts as one recursion level.
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
        float x = (worldRayOrigin.x - floor(worldRayOrigin.x)) * 2;
        float z = (worldRayOrigin.z - floor(worldRayOrigin.z)) * 2;
        float blackOrWhite = ((int)x + (int)z) % 2;
        colorAlbedo = float3(blackOrWhite, blackOrWhite, blackOrWhite);
    }
    // Basic Dot-Product Diffuse.
    float3 colorDiffuse = float3(0, 0, 0);
    {
        float light = max(0, dot(attrib.Normal.xyz, vectorLight));
        colorDiffuse = colorAlbedo * light;
    }
    // Schlick Fresnel Reflection.
    float3 colorFresnel = float3(0, 0, 0);
    float fresnel = schlick(WorldRayDirection(), attrib.Normal.xyz, 1, 1.3);
    {
        if (payload.RecursionClamp > 0 && fresnel > 0.05)
        {
            float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
            RayDesc ray = { worldRayOrigin + vectorReflect * 0.0001, 0, vectorReflect, 100000 };
            HitInfo payloadReflect;
            payloadReflect.ColorAndLambda = float4(0, 0, 0, 1);
            payloadReflect.RecursionClamp = payload.RecursionClamp - 1;
            TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, payloadReflect);
            colorFresnel = payloadReflect.ColorAndLambda.xyz;
        }
    }
    // Phong Specular.
    float3 colorSpecular = float3(0, 0, 0);
    {
        float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
        float specular = pow(max(0, dot(vectorReflect, vectorLight)), 32);
        colorSpecular = float3(specular, specular, specular);
    }
    // Final Color.
    payload.ColorAndLambda = float4(lerp(colorDiffuse, colorFresnel, fresnel) + colorSpecular, 1);
}

[shader("closesthit")]
void MaterialRedPlastic(inout HitInfo payload, Attributes attrib)
{
    const float3 vectorLight = normalize(float3(1, 1, -1));
    const float3 worldRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Solid Red Albedo.
    float3 colorAlbedo = float3(1, 0, 0);
    // Basic Dot-Product Diffuse.
    float3 colorDiffuse = float3(0, 0, 0);
    {
        float light = max(0, dot(attrib.Normal.xyz, vectorLight));
        colorDiffuse = colorAlbedo * light;
    }
    // Schlick Fresnel Reflection.
    float3 colorFresnel = float3(0, 0, 0);
    float fresnel = schlick(WorldRayDirection(), attrib.Normal.xyz, 1, 1.3);
    {
        if (payload.RecursionClamp > 0 && fresnel > 0.05)
        {
            float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
            RayDesc ray = { worldRayOrigin + vectorReflect * 0.0001, 0, vectorReflect, 100000 };
            HitInfo payloadReflect;
            payloadReflect.ColorAndLambda = float4(0, 0, 0, 1);
            payloadReflect.RecursionClamp = payload.RecursionClamp - 1;
            TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, payloadReflect);
            colorFresnel = payloadReflect.ColorAndLambda.xyz;
        }
    }
    // Phong Specular.
    float3 colorSpecular = float3(0, 0, 0);
    {
        float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal.xyz);
        float specular = pow(max(0, dot(vectorReflect, vectorLight)), 8);
        colorSpecular = float3(specular, specular, specular);
    }
    // Final Color.
    payload.ColorAndLambda = float4(lerp(colorDiffuse, colorFresnel, fresnel) + colorSpecular, 1);
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