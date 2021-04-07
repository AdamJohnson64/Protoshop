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
  float3 position = v0.Position + dp1 * barya + dp2 * baryb;
  float3 normal = v0.Normal + dn1 * barya + dn2 * baryb;
  float2 uv = v0.Texcoord + duv1 * barya + duv2 * baryb;
  // Look up all textures using the computed texture coordinate.
  float3 texelDiffuse = SampleTextureBilinear(textureDiffuse, uv).rgb;
  float3 texelNormal = SampleTextureBilinear(textureNormal, uv).rgb;

  // Note: In the pixel shader version of this we ddx/ddy to produce a reference plane.
  // When we're raytracing it's nonsense to use ddx/ddy so we'll use the triangle directly.
  // solve the linear system
  float3 dp2perp = cross( dp2, normal );
  float3 dp1perp = cross( normal, dp1 );
  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
  // construct a scale-invariant frame
  float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
  float3x3 matTangentFrame = float3x3( T * invmax, B * invmax, normal );
  float3 normalBump = normalize(mul(texelNormal * 2 - 1, matTangentFrame));
  
  // Fire a single shadow ray to demonstrate hit group offsets.
  float3 lightOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
  float3 lightDirection = float3(-3, 1, 0.1) - lightOrigin;
  RayDesc rayDescShadow = { lightOrigin + lightDirection * DEFAULT_TMIN, 0, lightDirection, 1 };
  RayPayload shadowRayPayload;
  shadowRayPayload.Color = float3(0, 0, 0);
  TraceRay(raytracingAccelerationStructure, RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, rayDescShadow, shadowRayPayload);
  // Calculate a simple dot product lighting.
  float illum = clamp(dot(normalBump, normalize(lightDirection)), 0, 1);
  // Emit the color with dot product ligiting and simple shadowing.
  rayPayload.Color = texelDiffuse * illum * shadowRayPayload.Color;
}

[shader("miss")]
void ShadowMiss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(1, 1, 1);
}

[shader("closesthit")]
void ShadowMaterialTextured(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(0.25, 0.25, 0.25);
}