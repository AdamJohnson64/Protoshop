#include "Sample_DXR_Common.inc"
#include "Sample_DXR_RaySimple.inc"
#include "Sample_DXR_Shaders.inc"

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
void MaterialRedPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(1, 0, 0);
}