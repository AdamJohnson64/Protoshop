#include "Sample_DXR_Common.inc"
#include "Sample_DXR_Implicit.inc"
#include "Sample_DXR_RaySimple.inc"
#include "Sample_DXR_Shaders.inc"

[shader("miss")]
void PrimaryMiss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0, 0, 0);
}

[shader("closesthit")]
void PrimaryMaterialDiffuse(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    float3 localX = float3(1, 0, 0);
    float3 localY = intersectionAttributes.Normal;
    float3 localZ = normalize(cross(localX, localY));
    localX = normalize(cross(localY, localZ));
    float3 rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + intersectionAttributes.Normal * 0.001;
    float3 accumulatedIrradiance = 0;
    for (int i = 0; i < 32; ++i)
    {
        // This incredibly cheap sample jittering of i + x * y * 16 produces a high-frequency
        // Moire interference pattern that looks like old-school dithering; nice.
        float3 hemisphere = HaltonSample(i + LaunchIndex.x * LaunchIndex.y * 16);
        float3 hemisphereInTangentFrame = hemisphere.x * localX + hemisphere.z * localY + hemisphere.y * localZ;
        RayDesc newRayDesc = { rayOrigin, DEFAULT_TMIN, hemisphereInTangentFrame, DEFAULT_TMAX };
        RayPayload recurseRayPayload;
        recurseRayPayload.Color = float3(0, 0, 0);
        TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 1, 0, 1, newRayDesc, recurseRayPayload);
        accumulatedIrradiance += Albedo * recurseRayPayload.Color;
    }
    rayPayload.Color = accumulatedIrradiance / 32;
}

[shader("closesthit")]
void PrimaryMaterialEmissive(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = Albedo;
}

[shader("miss")]
void ShadowMiss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0, 0, 0);
}

[shader("closesthit")]
void ShadowMaterialDiffuse(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(0, 0, 0);
}

[shader("closesthit")]
void ShadowMaterialEmissive(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = Albedo;
}