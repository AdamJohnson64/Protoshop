#include "Sample_DXR_Common.inc"
#include "Sample_DXR_RaySimple.inc"
#include "Sample_DXR_Shaders.inc"

Texture2D myTexture : register(t1);
StructuredBuffer<float2> concatenatedUVs : register(t4);

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
  float2 uv0 = concatenatedUVs[InstanceID() + PrimitiveIndex() * 3 + 0];
  float2 uv1 = concatenatedUVs[InstanceID() + PrimitiveIndex() * 3 + 1];
  float2 uv2 = concatenatedUVs[InstanceID() + PrimitiveIndex() * 3 + 2];
  float barya = intersectionAttributes.Normal.x;
  float baryb = intersectionAttributes.Normal.y;
  float2 uv = uv0 + (uv1 - uv0) * barya + (uv2 - uv0) * baryb;
  rayPayload.Color = SampleTextureBilinear(myTexture, uv).rgb;
}