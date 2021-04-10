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
void PrimaryMaterialAmbientOcclusion(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    float3 localX = float3(1, 0, 0);
    float3 localY = intersectionAttributes.Normal;
    float3 localZ = normalize(cross(localX, localY));
    localX = normalize(cross(localY, localZ));
    float3 rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + intersectionAttributes.Normal * 0.001;
    float illuminated = 0;
    for (int i = 0; i < 32; ++i)
    {
        // This incredibly cheap sample jittering of i + x * y * 16 produces a high-frequency
        // Moire interference pattern that looks like old-school dithering; nice.
        float3 hemisphere = HaltonSample(i + DispatchRaysIndex().x * DispatchRaysIndex().y * 16);
        float3 hemisphereInTangentFrame = hemisphere.x * localX + hemisphere.z * localY + hemisphere.y * localZ;
        RayDesc rayDesc = { rayOrigin + hemisphereInTangentFrame * DEFAULT_TMIN, 0, hemisphereInTangentFrame, DEFAULT_TMAX };
        RayPayload rayPayload;
        rayPayload.Color = float3(0, 0, 0);
        TraceRay(raytracingAccelerationStructure, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, rayDesc, rayPayload);
        illuminated += rayPayload.Color.r;
    }
    rayPayload.Color = illuminated / 32;
}

[shader("miss")]
void ShadowMiss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(1, 1, 1);
}

[shader("closesthit")]
void ShadowMaterialAmbientOcclusion(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(0, 0, 0);
}