#include "Sample_DXR_Common.inc"
#include "Sample_DXR_RaySimple.inc"
#include "Sample_DXR_Shaders.inc"

struct Vertex {
  float3 Position;
  float3 Normal;
  float2 Texcoord;
};

Texture2D textureDiffuse : register(t1);
Texture2D textureNormal : register(t2);
StructuredBuffer<Vertex> vertexAttributes : register(t4);

// This is the same cotangent reconstruction described at:
// http://www.thetenthplanet.de/archives/1180
float3x3 cotangent_frame( float3 N, float3 p, float2 uv ) {
  // get edge vectors of the pixel triangle
  float3 dp1 = ddx( p );
  float3 dp2 = ddy( p );
  float2 duv1 = ddx( uv );
  float2 duv2 = ddy( uv );
  // solve the linear system
  float3 dp2perp = cross( dp2, N );
  float3 dp1perp = cross( N, dp1 );
  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
  // construct a scale-invariant frame
  float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
  return float3x3( T * invmax, B * invmax, N );
}

[shader("miss")]
void PrimaryMiss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0, 0, 0);
}

[shader("closesthit")]
void PrimaryMaterialTextured(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
  // Locate and extract the vertex attributes for this triangle.
  // - Vertex attributes are uploaded into an SRV (t4).
  // - The index of the first triangle vertex is stored in InstanceID.
  Vertex v0 = vertexAttributes[InstanceID() + PrimitiveIndex() * 3 + 0];
  Vertex v1 = vertexAttributes[InstanceID() + PrimitiveIndex() * 3 + 1];
  Vertex v2 = vertexAttributes[InstanceID() + PrimitiveIndex() * 3 + 2];
  // Extract the barycentric coordinates from the intersection query.
  float barya = intersectionAttributes.Normal.x;
  float baryb = intersectionAttributes.Normal.y;
  // Compute position differences for barycentric interpolation.
  float3 dp1 = v1.Position - v0.Position;
  float3 dp2 = v2.Position - v0.Position;
  // Compute normal differences for barycentric interpolation.
  float3 dn1 = v1.Normal - v0.Normal;
  float3 dn2 = v2.Normal - v0.Normal;
  // Compute texture coordinate differences for barycentric interpolation.
  float2 duv1 = v1.Texcoord - v0.Texcoord;
  float2 duv2 = v2.Texcoord - v0.Texcoord;
  // Perform the interpolation of all components.
  float3 interpPosition = v0.Position + dp1 * barya + dp2 * baryb;
  float3 interpNormal = normalize(v0.Normal + dn1 * barya + dn2 * baryb);
  float2 interpUV = v0.Texcoord + duv1 * barya + duv2 * baryb;
  // Look up all textures using the computed texture coordinate.
  float3 texelDiffuse = SampleTextureBilinear(textureDiffuse, interpUV).rgb;
  float3 texelNormal = SampleTextureBilinear(textureNormal, interpUV).rgb;

  // Note: In the pixel shader version of this we ddx/ddy to produce a reference plane.
  // When we're raytracing it's nonsense to use ddx/ddy so we'll use the triangle directly.
  // solve the linear system
  float3 dp2perp = cross( dp2, interpNormal );
  float3 dp1perp = cross( interpNormal, dp1 );
  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
  // construct a scale-invariant frame
  float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
  float3x3 matTangentFrame = float3x3( T * invmax, B * invmax, interpNormal );

  // Calculate the normal mapped normal.
  float3 vectorNormal = normalize(mul(texelNormal * 2 - 1, matTangentFrame));

// Flip this over to use a basic shadow rendering.
#if 1
  // Use the slow version for fewer samples.
  // This converges more uniformly.
  float3x3 matTangentOrtho = TangentBasisFromViewAxis(vectorNormal);
  // Use the fast version for large numbers of samples.
  // This will exhibit "Hairy Ball" effects with any sample bias.
  //float3x3 matTangentOrtho = TangentBasisFromViewDirection(vectorNormal);
  // Use this orthonormal frame to transform hemisphere samples.
  float3 rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + interpNormal * 0.0001;
  float3 irradiance = float3(0, 0, 0);
  for (int i = 0; i < 8; ++i)
  {
      // This incredibly cheap sample jittering of i + x * y * 16 produces a high-frequency
      // Moire interference pattern that looks like old-school dithering; nice.
      uint sampleOffset = (DispatchRaysIndex().x * DispatchRaysIndex().y * 173 + i + AO_SampleSequenceOffset) % 7919;
      float3 hemisphere = HaltonSample(sampleOffset);
      float3 hemisphereInTangentFrame = mul(hemisphere, matTangentOrtho);
      // A note on this 0.05 eta offset here.
      // We're calculating the normal from the normal map which deviates from the geometric normal.
      // Since we're using geometry to perform lighting this will mean that our AO probe
      // rays may self intersect their neighborhood and reveal the geometry (and look nasty).
      // The larger this eta the more we displace the hemisphere from the surface.
      // This will smooth the lighting but will increase the risk of light leaks.
      // Tune with care.
      RayDesc rayDesc = { rayOrigin + hemisphereInTangentFrame * 0.05, 0, hemisphereInTangentFrame, DEFAULT_TMAX };
      RayPayload rayPayload;
      rayPayload.Color = float3(0, 0, 0);
      TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 1, 0, 1, rayDesc, rayPayload);
      // Fudge factor of 10 to account for heavy shadow in Sponza.
      irradiance += rayPayload.Color * 10;
  }
  rayPayload.Color = texelDiffuse * irradiance / 8;
#else
  // Fire a single shadow ray to demonstrate hit group offsets.
  float3 lightOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
  float3 lightDirection = float3(-3, 1, 0.1) - lightOrigin;
  RayDesc rayDescShadow = { lightOrigin + lightDirection * DEFAULT_TMIN, 0, lightDirection, 1 };
  RayPayload shadowRayPayload;
  shadowRayPayload.Color = float3(0, 0, 0);
  TraceRay(raytracingAccelerationStructure, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, rayDescShadow, shadowRayPayload);
  // Calculate a simple dot product lighting.
  float illum = clamp(dot(vectorNormal, normalize(lightDirection)), 0, 1);
  // Emit the color with dot product ligiting and simple shadowing.
  rayPayload.Color = texelDiffuse * illum * shadowRayPayload.Color;
#endif
}

[shader("miss")]
void ShadowMiss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(1, 1, 1);
}

[shader("closesthit")]
void ShadowMaterialTextured(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(0, 0, 0);
}