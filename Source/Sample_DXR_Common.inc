#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// Common HLSL Stuff
//
// Anyone and anything can import this file since it only contains constants
// and free functions. No shader entrypoints are defined here.
//
///////////////////////////////////////////////////////////////////////////////

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

RWTexture2D<float4> renderTargetOutput                          : register(u0);
RaytracingAccelerationStructure raytracingAccelerationStructure : register(t0);

// The raytracer needs only one big global constant fed into the top level
// ray generator which describes the camera transform. In order to get a good
// match to a raster render we use the inverse of the MVP transform (general
// vertex transform) to UN-project the homogeneous coordinates of the corners
// of the clip-box back into world space. The rays generated here will match
// the "rays" that the rasterizers would generate +/- some amount of float
// precision loss.

cbuffer Constants : register(b0)
{
    float4x4 TransformClipToWorld;
	// HACK: These elements are only here for the AO sample.
	float4 AO_AccumulatorRescale;
	uint AO_SampleSequenceOffset;
};

// Just to show how materials might be set up we'll patch 4 32-bit embedded
// values into a second material constant and upload that in the shader table.
// This should at least mean we can attach a color to things.

cbuffer Material : register(b1)
{
    float3 Albedo;
}

///////////////////////////////////////////////////////////////////////////////
// Structures
///////////////////////////////////////////////////////////////////////////////

// All primitive intersections for implicit surfaces will return this common
// structure. The only thing we need to know about the impact point is the
// local surface normal.

struct IntersectionAttributes
{
    float3 Normal;
};

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

// It's PI. It's everyones' favorite PI. The PI we all know and love.
// "The Joy Of PI" - David Blatner, September 1, 1999

static const float M_PI = 3.14159265f;

// In general when we re-emit rays for reflection or refraction we need to
// offset them by some small amount so we don't just end up hitting the same
// object that spawned them at the same intersection site. We need this to be
// small, but not so small that float-precision becomes a problem. Setting
// this too low will produce a salt-n-pepper image as the precision breaks up.
static const float DEFAULT_TMIN = 0.0001;

// We need a ray termination distance which is a big number initially. We
// could just use +INF but there's no clear way to define that in HLSL and I
// hate using divide-by-zero tricks in the compiler. Just use a big number.
// 1,000 kilometers ought to be enough for anyone...
static const float DEFAULT_TMAX = 1000000000;

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

///////////////////////////////////////////////////////////////////////////////
// Common Free Functions
///////////////////////////////////////////////////////////////////////////////

// Good ol' fashioned Lambert dot product lighting. So much can be said about
// this function, but I won't because I can't be bothered to type that much.
float lambert(float3 normal)
{
    const float3 light = normalize(float3(1, 1, -1));
    return max(0, dot(normal, light));
}

// Phong specular highlights rank second only to lens flares in terms of level
// of cliche.
float phong(float3 reflect, float power)
{
    const float3 light = normalize(float3(1, 1, -1));
    return pow(max(0, dot(light, reflect)), power);
}

// This refract2 function is NOT identical to the HLSL refract function. This
// version of the function can handle transitions both into and out of the
// refractive medium. In reality this is no more complicated than checking
// the dot product with the normal and flipping everything.
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

// Schlick approximation for Fresnel contribution of a non-conducting material.
float schlick(float3 incident, float3 normal, float ior1, float ior2)
{
	float coeff = (ior1 - ior2) / (ior1 + ior2);
	coeff = coeff * coeff;
	return coeff + (1 - coeff) * pow((1 - dot(-incident, normal)), 5);
}

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

////////////////////////////////////////////////////////////////////////////////
// Texture Sampling
////////////////////////////////////////////////////////////////////////////////

// Alas, we have no access to samplers in DXR so we'll have to implement our
// own texture samplers. This is painful; the hardware that normally does this
// is actually quite efficient. That means we're going to be wasting a lot of
// hardware performance doing this garbage.

// Bilinear interpolation implemented with far too much code.
float4 SampleTextureBilinear(Texture2D tex, float2 uv)
{
	uv -= floor(uv); // Clamp to [0..1] for wrapping
    uint Width, Height, NumberOfLevels;
    tex.GetDimensions(0, Width, Height, NumberOfLevels);
    float fx = uv.x * Width;
    float fy = uv.y * Height;
    uint ux = fx;
    uint uy = fy;
    float4 pix[4] = {
        tex.Load(uint3((ux + 0) % Width, (uy + 0) % Height, 0)),
        tex.Load(uint3((ux + 1) % Width, (uy + 0) % Height, 0)),
        tex.Load(uint3((ux + 0) % Width, (uy + 1) % Height, 0)),
        tex.Load(uint3((ux + 1) % Width, (uy + 1) % Height, 0)),
    };
    float ix = fx - ux;
    float iy = fy - uy;
    return (pix[0] * (1 - ix) + pix[1] * ix) * (1 - iy) + (pix[2] * (1 - ix) + pix[3] * ix) * iy;
}

// Nearest neighbor sampling for those occasions you want you textures to look
// like absolute garbage. It's cheap though.
float4 SampleTextureNearest(Texture2D tex, float2 uv)
{
	uv -= floor(uv); // Clamp to [0..1] for wrapping
    uint Width, Height, NumberOfLevels;
    tex.GetDimensions(0, Width, Height, NumberOfLevels);
    return tex.Load(uint3(uv.x * Width, uv.y * Height, 0));
}

////////////////////////////////////////////////////////////////////////////////
// TESTING FUNCTIONALITY
////////////////////////////////////////////////////////////////////////////////

// Create a really cheesy sky based on ray direction.
// Plug this into your primary and/or shadow miss to introduce an area sunlight.
float3 DXR_Fake_Sky()
{
    float sky = 1 - pow(1 - saturate(dot(WorldRayDirection(), float3(0,1,0))), 4);
    float sun = pow(saturate(dot(WorldRayDirection(), normalize(float3(0.1,1,0.1)))), 1000);
    float3 color = lerp(float3(0.75, 0.75, 0.75), float3(0.75, 0.75, 1), sky);
    color += float3(1000, 800, 800) * sun;
    return color;
}