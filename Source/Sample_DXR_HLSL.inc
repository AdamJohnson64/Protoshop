#pragma once

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

// It's PI. It's everyones' favorite PI. The PI we all know and love.
// "The Joy Of PI" - David Blatner, September 1, 1999

static const float M_PI = 3.14159265f;

static const float DEFAULT_TMAX = 1000000000;

///////////////////////////////////////////////////////////////////////////////
// Common Free Functions
///////////////////////////////////////////////////////////////////////////////

// The Halton Low Discrepency Sequence in an arbitrary radix.
//
// This function generates floating point numbers in the range [0,1) by summing
// bits in the IEEE floating point standard (essentially, radix 2). We don't
// use the raw bit-hacking trick here because we want arbitrary radix support.
//
// Halton is a good sequence to use if you need a pseudo-random sequence which
// covers the entire [0,1) domain over a large number of iterations, but also
// covers an evenly distributed (non-clumpy) subsequence of values over a small
// number of samples. We use this to integrate over a square patch with total
// float-domain coverage at the total bit-width of the IEEE float.

float Halton(int radix, int x)
{
	float result = 0;
	int div = 1;
	for (int i = 0; i < 16; ++i)
    {
		const int mod = x % radix;
		div = div * radix;
		result += float(mod) / div;
		x = x / radix;
	}
	return result;
}

// Generate a direction ray in the X,Y plane and extending "upwards" in the Z.
//
// Using two values which express the azimuth (U) and elevation (V) we create
// a direction ray within the hemisphere. Azimuth describes the rotation around
// the horizon of the XY plane, and V describes the elevation looking up into
// the "sky". Input values are in the range [0,1) to match the output of the
// Halton sequence for sampling. A value of 0 for V looks directly along the
// horizon. A value of 1 for V looks directly upward into the sky.
//
// Understanding and visualizing the distributions of these functions is
// absolutely ESSENTIAL to correctly model BRDFs. For now we'll just be happy
// with our horribly misunderstood hemispherish thingy.

float3 HemisphereCosineBias(float u, float v)
{
	const float azimuth = u * 2 * M_PI;
	const float inv_v = sqrt(1 - v * v);
	return float3(sin(azimuth) * inv_v, cos(azimuth) * inv_v, v);
}

// This is another fun variant. I have no idea what the distributions for these
// samples look like right now so we'll be happy to just enjoy some fast but
// optically inaccurate pretty pictures.

float3 HemisphereCosineBias2(float u, float v)
{
	const float azimuth = u * 2 * M_PI;
	const float altitude = acos(sqrt(v));
	const float cos_alt = cos(altitude);
	return float3(sin(azimuth) * cos_alt, cos(azimuth) * cos_alt, sin(altitude));
}

// Using a single sample selector choose a direction within the hemisphere.
// We use the Halton-Radix-2 for U and Halton-Radix-3 for V.

float3 HaltonSample(int x)
{
	return HemisphereCosineBias(Halton(2, x), Halton(3, x));
}

///////////////////////////////////////////////////////////////////////////////
// Common Raytracer Structures
///////////////////////////////////////////////////////////////////////////////

// All ray hit results return a structure like this. We record the TMax (aka
// maximum intersection lambda) as a means to capture the hit distance. Since
// we're using a globally consistent maximum this makes it relatively easy to
// determine the no-hit condition.

struct RayPayload
{
    float3 Color;
    float TMax;
    int RecursionLevel;
    int Flags;
};

// All primitive intersections will return this structure. The only thing we
// need to know about the impact point is the local surface normal.

struct IntersectionAttributes
{
    float3 Normal;
};

///////////////////////////////////////////////////////////////////////////////
// Registers
///////////////////////////////////////////////////////////////////////////////

// Declare our global layout of registers as consumed by every shader in the
// DXR pipeline. Right now we don't use any textures so there's no need to
// define more than the raytracing acceleration structure SRV.
//
// NOTE: We're appliying a subtle but important concept here in the ordering
// of these inputs. In the root signature setup we define UAV, then SRV, then
// CBV. There's no real requirement that we order things this way but in
// general it's good practice to order your inputs by most common to least
// common, or most general to least general. For our purposes we are
// raytracing; whatever we do MUST have a UAV to output onto so this is our
// most general input/output. We must also have a SRV to contain the
// raytracing acceleration structure, but we might define effects that
// operate only on the UAV without raytracing (hence this is second).
// Finally we need a CBV to contain the camera configuration for the ray
// generation shader. Only the ray generator needs this so passing this into
// other shaders is superfluous but harmless.
//
// Otherwise the ordering here is completely unimportant. We make this
// consistent only to solidify the principle as it appears on paper.

RWTexture2D<float4> RTOutput             : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

// The raytracer needs only one big global constant fed into the top level
// ray generator which describes the camera transform. In order to get a good
// match to a raster render we use the inverse of the MVP transform (general
// vertex transform) to UN-project the homogeneous coordinates of the corners
// of the clip-box back into world space. The rays generated here will match
// the "rays" that the rasterizers would generate +/- some amount of float
// precision loss.

cbuffer Constants : register(b0)
{
    float4x4 Transform;
};

///////////////////////////////////////////////////////////////////////////////
// Common Raytracer Entrypoints
///////////////////////////////////////////////////////////////////////////////

// This is the accompanying function for the ray generator. Here we get a good
// match to a raster render using the inverse of the MVP transform (general
// vertex transform) to UN-project the homogeneous coordinates of the corners
// of the clip-box back into world space. The rays generated here will match
// the "rays" that the rasterizers would generate +/- some amount of float
// precision loss.

[shader("raygeneration")]
void RayGenerationMVPClip()
{
    const float NormalizedX = -1 + 2 * (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    const float NormalizedY = 1 - 2 * (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
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
    rayPayload.RecursionLevel = 0;
    rayPayload.Flags = 0;
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    RTOutput[DispatchRaysIndex().xy] = float4(rayPayload.Color, 1);
}

// The general miss shader. All rays which hit nothing will return
// black/zero/nothingness in the ray color.

[shader("miss")]
void Miss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0, 0, 0);
}

// Plane intersection. Use the general plane intersection equation to determine
// the intersection of an arbitrary world ray with the plane in object-space.
// The plane is XZ oriented with its normal pointing directly up along Y.

[shader("intersection")]
void IntersectPlane()
{
    IntersectionAttributes intersectionAttributes;
    const float3 origin = ObjectRayOrigin();
    const float3 direction = ObjectRayDirection();
    const float3 plane = float3(0, 1, 0);
    const float divisor = dot(plane, direction);
    const float lambda = -dot(origin, plane) / divisor;
    if (lambda < 0) return;
    const float3 intersection = origin + direction * lambda;
    if (intersection.x < -1 || intersection.x > 1 || intersection.z < -1 || intersection.z > 1) return;
    intersectionAttributes.Normal = plane;
    ReportHit(lambda, 0, intersectionAttributes);
}

// Sphere intersection. Use the general quadratic solution to find the lambda
// for the two intersection points of a ray with a sphere. These two points
// occur at the positive and negative SQRT solutions. Note that we don't
// test for ray misses which will result in a negative discriminant. Instead
// we just allow the math functions to break horribly and return NaN results.
// The remaining math will result in more NaNs and will all evaluate to false.

[shader("intersection")]
void IntersectSphere()
{
    IntersectionAttributes intersectionAttributes;
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
        intersectionAttributes.Normal = normalize(mul(origin + direction * hitFront, (float3x3)ObjectToWorld3x4()));
        ReportHit(hitFront, 0, intersectionAttributes);
    }
    float hitBack = (-b + solution) / (2 * a);
    if (hitBack >= 0)
    {
        intersectionAttributes.Normal = normalize(mul(origin + direction * hitBack, (float3x3)ObjectToWorld3x4()));
        ReportHit(hitBack, 0, intersectionAttributes);
    }
}