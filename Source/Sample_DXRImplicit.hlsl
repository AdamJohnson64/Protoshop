#define MAXIMUM_RAY_RECURSION_DEPTH 4

// To prevent combinatoric explosion we don't allow previously reflected rays
// to refract, nor do we allow refracted rays to reflect. These flags
// indicate that a ray has traversed this ray path.
#define RAY_FLAG_REFLECTED 1
#define RAY_FLAG_REFRACTED 2

#define IOR_VACUUM 1.00
#define IOR_GLASS 1.52
#define IOR_PLASTIC 1.46

cbuffer Constants : register(b0)
{
    float4x4 Transform;
};

struct HitInfo
{
    float3 Color;
    int RecursionLevel;
    int Flags;
};

struct Attributes
{
    float3 Normal;
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void RayGenerationDebug()
{
    RayDesc ray;
    ray.Origin = float3(0, 1, -3);
    float normx = (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    float normy = (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
    ray.Direction = normalize(float3(-1 + normx * 2, 1 - normy * 2, 1));
    ray.TMin = 0.001f;
    ray.TMax = 1000;
    HitInfo rayOut;
    rayOut.Color = float3(0, 0, 0);
    rayOut.RecursionLevel = 1;
    rayOut.Flags = 0;
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, rayOut);
    RTOutput[DispatchRaysIndex().xy] = float4(rayOut.Color, 1);
}

[shader("raygeneration")]
void RayGenerationRasterMatch()
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    uint2 LaunchDimensions = DispatchRaysDimensions().xy;
    float NormalizedX = -1 + 2 * (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    float NormalizedY = 1 - 2 * (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
    RayDesc ray;
    float4 front = mul(float4(NormalizedX, NormalizedY, 0, 1), Transform);
    front /= front.w;
    float4 back = mul(float4(NormalizedX, NormalizedY, 1, 1), Transform);
    back /= back.w;
    ray.Origin = front.xyz;
    ray.Direction = normalize(back.xyz - front.xyz);
    ray.TMin = 0.001f;
    ray.TMax = 1000;
    HitInfo rayOut;
    rayOut.Color = float3(0, 0, 0);
    rayOut.RecursionLevel = 1;
    rayOut.Flags = 0;
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, rayOut);
    RTOutput[DispatchRaysIndex().xy] = float4(rayOut.Color, 1);
}

[shader("miss")]
void Miss(inout HitInfo rayIn)
{
    rayIn.Color = float3(0.25f, 0.25f, 0.25f);
}

float lambert(float3 normal)
{
    const float3 light = normalize(float3(1, 1, -1));
    return max(0, dot(normal, light));
}

float phong(float3 reflect, float power)
{
    const float3 light = normalize(float3(1, 1, -1));
    return pow(max(0, dot(light, reflect)), power);
}

float3 refract2(float3 incident, float3 normal, float ior)
{
	float cosi = clamp(dot(incident, normal), -1, 1);
	float etai = 1, etat = ior;
	float3 n = normal;
	if (cosi < 0) { cosi = -cosi; }
	else { float tmp = etai; etai = etat; etat = tmp; n = -normal; }
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? float3(0, 0, 0) : (eta * incident + (eta * cosi - sqrt(k)) * n);
}

float schlick(float3 incident, float3 normal, float ior1, float ior2)
{
	float coeff = (ior1 - ior2) / (ior1 + ior2);
	coeff = coeff * coeff;
	return coeff + (1 - coeff) * pow((1 - dot(-incident, normal)), 5);
}

float3 RecurseRay(float3 origin, float3 direction, in HitInfo rayIn, int rayType)
{
    RayDesc ray = { origin + direction * 0.0001, 0, direction, 100000 };
    HitInfo rayOut;
    rayOut.Color = float3(0, 0, 0);
    rayOut.RecursionLevel = rayIn.RecursionLevel + 1;
    rayOut.Flags = rayIn.Flags | rayType;
    TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, rayOut);
    return rayOut.Color;
}

[shader("closesthit")]
void MaterialCheckerboard(inout HitInfo rayIn, Attributes attrib)
{
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Basic Checkerboard Albedo.
    float3 colorAlbedo;
    {
        float x = (worldHit.x - floor(worldHit.x)) * 2;
        float z = (worldHit.z - floor(worldHit.z)) * 2;
        float blackOrWhite = ((int)x + (int)z) % 2;
        colorAlbedo = float3(blackOrWhite, blackOrWhite, blackOrWhite);
    }
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(attrib.Normal);
    // Schlick Fresnel Reflection.
    float3 colorFresnel = colorDiffuse;
    float fresnel = schlick(WorldRayDirection(), attrib.Normal, IOR_VACUUM, IOR_PLASTIC);
    {
        if (rayIn.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayIn.Flags & RAY_FLAG_REFRACTED) == 0)
        {
            colorFresnel = RecurseRay(worldHit, reflect(WorldRayDirection(), attrib.Normal), rayIn, RAY_FLAG_REFLECTED);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), attrib.Normal), 64);
    // Final Color.
    rayIn.Color = lerp(colorDiffuse, colorFresnel, fresnel) + colorSpecular;
}

[shader("closesthit")]
void MaterialRedPlastic(inout HitInfo rayIn, Attributes attrib)
{
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Solid Red Albedo.
    float3 colorAlbedo = float3(1, 0, 0);
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(attrib.Normal);
    // Schlick Fresnel Reflection.
    float3 colorFresnel = colorDiffuse;
    float fresnel = schlick(WorldRayDirection(), attrib.Normal, IOR_VACUUM, IOR_PLASTIC);
    {
        if (rayIn.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayIn.Flags & RAY_FLAG_REFRACTED) == 0)
        {
            colorFresnel = RecurseRay(worldHit, reflect(WorldRayDirection(), attrib.Normal), rayIn, RAY_FLAG_REFLECTED);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), attrib.Normal), 8);
    // Final Color.
    rayIn.Color = lerp(colorDiffuse, colorFresnel, fresnel) + colorSpecular;
}

[shader("closesthit")]
void MaterialGlass(inout HitInfo rayIn, Attributes attrib)
{
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    const float3 vectorReflect = reflect(WorldRayDirection(), attrib.Normal);
    // Refraction.
    float3 colorDiffuse = float3(0, 0, 0);
    {
        if (rayIn.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayIn.Flags & RAY_FLAG_REFLECTED) == 0)
        {
            colorDiffuse = RecurseRay(worldHit, refract2(WorldRayDirection(), attrib.Normal, IOR_GLASS), rayIn, RAY_FLAG_REFRACTED);
        }
    }
    // Schlick Fresnel Reflection.
    float3 colorFresnel = colorDiffuse;
    float fresnel = schlick(WorldRayDirection(), attrib.Normal, IOR_VACUUM, IOR_GLASS);
    {
        if (rayIn.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayIn.Flags & RAY_FLAG_REFRACTED) == 0)
        {
            colorFresnel = RecurseRay(worldHit, vectorReflect, rayIn, RAY_FLAG_REFLECTED);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(vectorReflect, 256);
    // Final Color.
    rayIn.Color = lerp(colorDiffuse, colorFresnel, fresnel) + colorSpecular;
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
    attributes.Normal = plane.xyz;
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