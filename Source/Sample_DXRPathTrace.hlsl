#include "Sample_DXR_HLSL.inc"

[shader("closesthit")]
void MaterialDiffuse(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    if (rayPayload.RecursionLevel >= 1)
    {
        rayPayload.Color = float3(0, 0, 0);
        return;
    }
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    rayPayload.IntersectionT = RayTCurrent();
    if (rayPayload.Flags != RAY_FLAG_NONE) return;
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
        recurseRayPayload.IntersectionT = DEFAULT_TMAX;
        recurseRayPayload.Flags = 1; // Do not spawn new rays.
        recurseRayPayload.RecursionLevel = rayPayload.RecursionLevel + 1;
        TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, newRayDesc, recurseRayPayload);
        accumulatedIrradiance += recurseRayPayload.Color;
    }
    rayPayload.Color = accumulatedIrradiance / 32;
}

[shader("closesthit")]
void MaterialEmissiveRed(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(1, 0, 0);
}

[shader("closesthit")]
void MaterialEmissiveGreen(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.Color = float3(0, 1, 0);
}