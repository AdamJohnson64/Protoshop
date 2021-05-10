////////////////////////////////////////////////////////////////////////////////
// HLSL Common
//
// These functions are completely independent of rendering technology and should
// work in both rasterized and raytraced models. This file can be used for
// illumination models, radiometry, transformation support (etc).
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

// It's PI. It's everyones' favorite PI. The PI we all know and love.
// "The Joy Of PI" - David Blatner, September 1, 1999

static const float M_PI = 3.14159265f;

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

// This is the same cotangent reconstruction described at:
// http://www.thetenthplanet.de/archives/1180
float3x3 cotangent_frame(float3 N, float3 dp1, float3 dp2, float2 duv1, float2 duv2) {
  // solve the linear system
  float3 dp2perp = cross(dp2, N);
  float3 dp1perp = cross(N, dp1);
  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
  // construct a scale-invariant frame
  float invmax = rsqrt(max(dot(T,T), dot(B,B)));
  return float3x3(T * invmax, B * invmax, N);
}