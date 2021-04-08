#include "Sample_DXR_Common.inc"
#include "Sample_DXR_Implicit.inc"
#include "Sample_DXR_RayRecurse.inc"
#include "Sample_DXR_Shaders.inc"

[shader("closesthit")]
void MaterialAmbientOcclusion(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.IntersectionT = RayTCurrent();
    if (rayPayload.Flags != 0) return;
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
        if (!ShadowRay(rayOrigin, hemisphereInTangentFrame))
        {
            ++illuminated;
        }
    }
    rayPayload.Color = illuminated / 32;
}