#define DEFAULT_TMAX 1000000000

static const float M_PI = 3.14159265f;

float Halton(int radix, int x)
{
	float result = 0;
	int div = 1;
	for (int i = 0; i < 16; ++i)
    {
		int mod = x % radix;
		div = div * radix;
		result += float(mod) / div;
		x = x / radix;
	}
	return result;
}

float3 HemisphereCosineBias(float u, float v)
{
	float azimuth = u * 2 * M_PI;
	float altitude = acos(sqrt(v));
	float cos_alt = cos(altitude);
	return float3(sin(azimuth) * cos_alt, cos(azimuth) * cos_alt, sin(altitude));
}

float3 HaltonSample(int x)
{
	return HemisphereCosineBias(Halton(2, x), Halton(3, x));
}

cbuffer Constants : register(b0)
{
    float4x4 Transform;
};

struct HitInfo
{
    float3 Color;
    float TMax;
    int Flags;
};

struct Attributes
{
    float3 Normal;
};

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

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
    ray.TMax = DEFAULT_TMAX;
    HitInfo rayOut;
    rayOut.Color = float3(0, 0, 0);
    rayOut.TMax = DEFAULT_TMAX;
    rayOut.Flags = 0;
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, rayOut);
    RTOutput[DispatchRaysIndex().xy] = float4(rayOut.Color, 1);
}

[shader("miss")]
void Miss(inout HitInfo rayIn)
{
    rayIn.Color = float3(0.25f, 0.25f, 0.25f);
}

bool Illuminated(float3 origin, float3 direction)
{
    RayDesc ray = { origin, 0, direction, DEFAULT_TMAX };
    HitInfo rayOut;
    rayOut.Color = float3(0, 0, 0);
    rayOut.TMax = DEFAULT_TMAX;
    rayOut.Flags = 1; // Do not spawn new rays.
    TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, rayOut);
    return rayOut.TMax == DEFAULT_TMAX;
}

[shader("closesthit")]
void MaterialAmbientOcclusion(inout HitInfo rayIn, Attributes attrib)
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    rayIn.TMax = RayTCurrent();
    if (rayIn.Flags != 0) return;
    float3 localX = float3(1, 0, 0);
    float3 localY = attrib.Normal;
    float3 localZ = normalize(cross(localX, localY));
    localX = normalize(cross(localY, localZ));
    float3 rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + attrib.Normal * 0.001;
    float illuminated = 0;
    for (int i = 0; i < 32; ++i)
    {
        // This incredibly cheap sample jittering of i + x * y * 16 produces a high-frequency
        // Moire interference pattern that looks like old-school dithering; nice.
        float3 hemisphere = HaltonSample(i + LaunchIndex.x * LaunchIndex.y * 16);
        float3 hemisphereInTangentFrame = hemisphere.x * localX + hemisphere.z * localY + hemisphere.y * localZ;
        if (Illuminated(rayOrigin, hemisphereInTangentFrame))
        {
            ++illuminated;
        }
    }
    rayIn.Color = illuminated / 32;
}

[shader("intersection")]
void IntersectPlane()
{
    Attributes attributes;
    float3 origin = ObjectRayOrigin();
    float3 direction = ObjectRayDirection();
    float3 plane = float3(0, 1, 0);
    float divisor = dot(plane, direction);
    float lambda = -dot(origin, plane) / divisor;
    if (lambda < 0) return;
    float3 intersection = origin + direction * lambda;
    if (intersection.x < -1 || intersection.x > 1 || intersection.z < -1 || intersection.z > 1) return;
    attributes.Normal = plane;
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