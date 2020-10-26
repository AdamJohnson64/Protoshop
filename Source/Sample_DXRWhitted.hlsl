#define MAXIMUM_RAY_RECURSION_DEPTH 4

#define DEFAULT_TMAX 1000000000

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

struct RayPayload
{
    float3 Color;
    float TMax;
    int RecursionLevel;
    int Flags;
};

struct IntersectionAttributes
{
    float3 Normal;
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void RayGenerationMVPClip()
{
    float NormalizedX = -1 + 2 * (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    float NormalizedY = 1 - 2 * (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
    RayDesc rayDesc;
    float4 front = mul(float4(NormalizedX, NormalizedY, 0, 1), Transform);
    front /= front.w;
    float4 back = mul(float4(NormalizedX, NormalizedY, 1, 1), Transform);
    back /= back.w;
    rayDesc.Origin = front.xyz;
    rayDesc.Direction = normalize(back.xyz - front.xyz);
    rayDesc.TMin = 0.001;
    rayDesc.TMax = DEFAULT_TMAX;
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    rayPayload.TMax = DEFAULT_TMAX;
    rayPayload.RecursionLevel = 1;
    rayPayload.Flags = 0;
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    RTOutput[DispatchRaysIndex().xy] = float4(rayPayload.Color, 1);
}

[shader("miss")]
void Miss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0.25f, 0.25f, 0.25f);
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

float3 RecurseRay(float3 origin, float3 direction, in RayPayload rayPayload, int rayType)
{
    RayDesc rayDesc = { origin + direction * 0.0001, 0, direction, DEFAULT_TMAX };
    RayPayload recurseRayPayload;
    recurseRayPayload.Color = float3(0, 0, 0);
    recurseRayPayload.TMax = DEFAULT_TMAX;
    recurseRayPayload.RecursionLevel = rayPayload.RecursionLevel + 1;
    recurseRayPayload.Flags = rayPayload.Flags | rayType;
    TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, rayDesc, recurseRayPayload);
    return recurseRayPayload.Color;
}

bool Shadowed(float3 origin)
{
    const float3 light = normalize(float3(1, 1, -1));
    RayDesc rayDesc = { origin + light * 0.0001, 0, light, DEFAULT_TMAX };
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    rayPayload.TMax = DEFAULT_TMAX;
    rayPayload.RecursionLevel = 65536; // Do not recurse.
    rayPayload.Flags = 65535; // Do not spawn new rays.
    TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    return rayPayload.TMax < DEFAULT_TMAX;
}

[shader("closesthit")]
void MaterialCheckerboard(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.TMax = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Shadow Ray.
    if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && rayPayload.Flags != 65535)
    {
        if (Shadowed(worldHit))
        {
            rayPayload.Color = float3(0, 0, 0);
            return;
        }
    }
    // Basic Checkerboard Albedo.
    float3 colorAlbedo;
    {
        float x = (worldHit.x - floor(worldHit.x)) * 2;
        float z = (worldHit.z - floor(worldHit.z)) * 2;
        float blackOrWhite = ((int)x + (int)z) % 2;
        colorAlbedo = float3(blackOrWhite, blackOrWhite, blackOrWhite);
    }
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(intersectionAttributes.Normal);
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_PLASTIC);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_FLAG_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, reflect(WorldRayDirection(), intersectionAttributes.Normal), rayPayload, RAY_FLAG_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), intersectionAttributes.Normal), 64);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
}

[shader("closesthit")]
void MaterialRedPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.TMax = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Shadow Ray.
    if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && rayPayload.Flags != 65535)
    {
        if (Shadowed(worldHit))
        {
            rayPayload.Color = float3(0, 0, 0);
            return;
        }
    }
    // Solid Red Albedo.
    float3 colorAlbedo = float3(1, 0, 0);
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(intersectionAttributes.Normal);
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_PLASTIC);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_FLAG_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, reflect(WorldRayDirection(), intersectionAttributes.Normal), rayPayload, RAY_FLAG_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), intersectionAttributes.Normal), 8);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
}

[shader("closesthit")]
void MaterialGlass(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.TMax = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    const float3 vectorReflect = reflect(WorldRayDirection(), intersectionAttributes.Normal);
    // Refraction.
    float3 colorDiffuse = float3(0, 0, 0);
    {
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_FLAG_REFLECTED) == 0)
        {
            colorDiffuse = RecurseRay(worldHit, refract2(WorldRayDirection(), intersectionAttributes.Normal, IOR_GLASS), rayPayload, RAY_FLAG_REFRACTED);
        }
    }
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_GLASS);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_FLAG_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, vectorReflect, rayPayload, RAY_FLAG_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(vectorReflect, 256);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
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