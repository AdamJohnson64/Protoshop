#include "Sample_DXR_Common.inc"
#include "Sample_DXR_RaySimple.inc"
#include "Sample_DXR_Shaders.inc"

struct Vertex {
  float3 Normal;
  float2 Texcoord;
};

Texture2D myTexture : register(t1);
StructuredBuffer<Vertex> vertexAttributes : register(t4);

[shader("closesthit")]
void MaterialCheckerboard(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    float3 worldRayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float x = worldRayOrigin.x * 4;
    float z = worldRayOrigin.z * 4;
    x -= floor(x);
    z -= floor(z);
    x *= 2;
    z *= 2;
    float checker = ((int)x + (int)z) % 2;
    rayPayload.Color = float3(checker, checker, checker);
}

[shader("closesthit")]
void MaterialTextured(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
  Vertex v0 = vertexAttributes[InstanceID() + PrimitiveIndex() * 3 + 0];
  Vertex v1 = vertexAttributes[InstanceID() + PrimitiveIndex() * 3 + 1];
  Vertex v2 = vertexAttributes[InstanceID() + PrimitiveIndex() * 3 + 2];
  float barya = intersectionAttributes.Normal.x;
  float baryb = intersectionAttributes.Normal.y;
  float3 normal = v0.Normal + (v1.Normal - v0.Normal) * barya + (v2.Normal - v0.Normal) * baryb;
  float2 uv = v0.Texcoord + (v1.Texcoord - v0.Texcoord) * barya + (v2.Texcoord - v0.Texcoord) * baryb;
  float illum = clamp(dot(normal, normalize(float3(1, 1, -1))), 0, 1);
  rayPayload.Color = illum * SampleTextureBilinear(myTexture, uv).rgb;
}