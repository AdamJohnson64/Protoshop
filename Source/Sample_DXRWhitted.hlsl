static const int MAXIMUM_RAY_RECURSION_DEPTH = 4;

// In general when we re-emit rays for reflection or refraction we need to
// offset them by some small amount so we don't just end up hitting the same
// object that spawned them at the same intersection site. We need this to be
// small, but not so small that float-precision becomes a problem. Setting
// this too low will produce a salt-n-pepper image as the precision breaks up.
static const float DEFAULT_TMIN = 0.001;

// We need a ray termination distance which is a big number initially. We
// could just use +INF but there's no clear way to define that in HLSL and I
// hate using divide-by-zero tricks in the compiler. Just use a big number.
// 1,000 kilometers ought to be enough for anyone...
static const float DEFAULT_TMAX = 1000000; 

// To prevent combinatoric explosion we don't allow previously reflected rays
// to refract, nor do we allow refracted rays to reflect. These flags
// indicate that a ray has traversed this ray path.
static const unsigned int RAY_IS_PRIMARY = 0;
static const unsigned int RAY_WAS_REFLECTED = 1;
static const unsigned int RAY_WAS_REFRACTED = 2;
// Also note that we use RAY_FLAG_NONE in places, but we don't define it. It
// turns out that HLSL provides this for us and it just so happens to have a
// nice name that conveys the right sentiment. Steal it unashamedly.

// Handy dandy indices of refraction for various materials. We'll use this for
// both refraction rays and Fresnel calculation.
static const float IOR_VACUUM = 1.00;
static const float IOR_GLASS = 1.52;
static const float IOR_PLASTIC = 1.46;

cbuffer Constants : register(b0)
{
    float4x4 Transform;
};

cbuffer Material : register(b1)
{
    float3 Albedo;
};

struct RayPayload
{
    float3 Color;
    float IntersectionT;
    int RecursionLevel;
    int Flags;
};

struct IntersectionAttributes
{
    float3 Normal;
};

RWTexture2D<float4> renderTargetOutput                          : register(u0);
RaytracingAccelerationStructure raytracingAccelerationStructure : register(t0);

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
    rayPayload.IntersectionT = DEFAULT_TMAX;
    rayPayload.RecursionLevel = 1;
    rayPayload.Flags = RAY_IS_PRIMARY;
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    renderTargetOutput[DispatchRaysIndex().xy] = float4(rayPayload.Color, 1);
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
    RayDesc rayDesc = { origin + direction * DEFAULT_TMIN, 0, direction, DEFAULT_TMAX };
    RayPayload recurseRayPayload;
    recurseRayPayload.Color = float3(0, 0, 0);
    recurseRayPayload.IntersectionT = DEFAULT_TMAX;
    recurseRayPayload.RecursionLevel = rayPayload.RecursionLevel + 1;
    recurseRayPayload.Flags = rayPayload.Flags | rayType;
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, recurseRayPayload);
    return recurseRayPayload.Color;
}

bool Shadowed(float3 origin)
{
    const float3 light = normalize(float3(1, 1, -1));
    RayDesc rayDesc = { origin + light * DEFAULT_TMIN, 0, light, DEFAULT_TMAX };
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    rayPayload.IntersectionT = DEFAULT_TMAX;
    rayPayload.RecursionLevel = 65536; // Do not recurse.
    rayPayload.Flags = 65535; // Do not spawn new rays.
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    return rayPayload.IntersectionT < DEFAULT_TMAX;
}

[shader("closesthit")]
void MaterialCheckerboard(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.IntersectionT = RayTCurrent();
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
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, reflect(WorldRayDirection(), intersectionAttributes.Normal), rayPayload, RAY_WAS_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), intersectionAttributes.Normal), 64);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
}

[shader("closesthit")]
void MaterialPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.IntersectionT = RayTCurrent();
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
    float3 colorAlbedo = Albedo;
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(intersectionAttributes.Normal);
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_PLASTIC);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, reflect(WorldRayDirection(), intersectionAttributes.Normal), rayPayload, RAY_WAS_REFLECTED), fresnel);
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
    rayPayload.IntersectionT = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    const float3 vectorReflect = reflect(WorldRayDirection(), intersectionAttributes.Normal);
    // Refraction.
    float3 colorDiffuse = float3(0, 0, 0);
    {
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFLECTED) == 0)
        {
            colorDiffuse = RecurseRay(worldHit, refract2(WorldRayDirection(), intersectionAttributes.Normal, IOR_GLASS), rayPayload, RAY_WAS_REFRACTED);
        }
    }
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_GLASS);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, vectorReflect, rayPayload, RAY_WAS_REFLECTED), fresnel);
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