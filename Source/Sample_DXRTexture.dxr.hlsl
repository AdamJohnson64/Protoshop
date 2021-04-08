#include "Sample_DXR_Common.inc"
#include "Sample_DXR_Implicit.inc"
#include "Sample_DXR_RaySimple.inc"
#include "Sample_DXR_Shaders.inc"

Texture2D myTexture : register(t1);

[shader("closesthit")]
void MaterialCheckerboard(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    const float3 objectHit = ObjectRayOrigin() + ObjectRayDirection() * RayTCurrent();
    rayPayload.Color = SampleTextureBilinear(myTexture, float2((objectHit.x + 1) / 2, (objectHit.z + 1) / 2)).rgb;
}

[shader("closesthit")]
void MaterialPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(1, 0, 0);
}