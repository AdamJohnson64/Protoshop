////////////////////////////////////////////////////////////////////////////////
// HLSL Common
//
// These functions are completely independent of rendering technology and should
// work in both rasterized and raytraced models. This file can be used for
// illumination models, radiometry, transformation support (etc).
////////////////////////////////////////////////////////////////////////////////

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