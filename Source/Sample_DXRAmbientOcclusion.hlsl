#include "Sample_DXR_HLSL.inc"

bool Illuminated(float3 origin, float3 direction)
{
    RayDesc rayDesc = { origin, 0, direction, DEFAULT_TMAX };
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    rayPayload.TMax = DEFAULT_TMAX;
    rayPayload.Flags = 1; // Do not spawn new rays.
    TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    return rayPayload.TMax == DEFAULT_TMAX;
}

[shader("closesthit")]
void MaterialAmbientOcclusion(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    rayPayload.TMax = RayTCurrent();
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
        float3 hemisphere = HaltonSample(i + LaunchIndex.x * LaunchIndex.y * 16);
        float3 hemisphereInTangentFrame = hemisphere.x * localX + hemisphere.z * localY + hemisphere.y * localZ;
        if (Illuminated(rayOrigin, hemisphereInTangentFrame))
        {
            ++illuminated;
        }
    }
    rayPayload.Color = illuminated / 32;
}