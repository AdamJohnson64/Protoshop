#include "Sample_DXR_HLSL.inc"

[shader("closesthit")]
void MaterialDiffuse(inout HitInfo rayIn, Attributes attrib)
{
    if (rayIn.RecursionLevel >= 1)
    {
        rayIn.Color = float3(0, 0, 0);
        return;
    }
    uint2 LaunchIndex = DispatchRaysIndex().xy;
    rayIn.TMax = RayTCurrent();
    if (rayIn.Flags != 0) return;
    float3 localX = float3(1, 0, 0);
    float3 localY = attrib.Normal;
    float3 localZ = normalize(cross(localX, localY));
    localX = normalize(cross(localY, localZ));
    float3 rayOrigin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + attrib.Normal * 0.001;
    float3 accumulatedIrradiance = 0;
    for (int i = 0; i < 32; ++i)
    {
        // This incredibly cheap sample jittering of i + x * y * 16 produces a high-frequency
        // Moire interference pattern that looks like old-school dithering; nice.
        float3 hemisphere = HaltonSample(i + LaunchIndex.x * LaunchIndex.y * 16);
        float3 hemisphereInTangentFrame = hemisphere.x * localX + hemisphere.z * localY + hemisphere.y * localZ;
        RayDesc ray = { rayOrigin, 0, hemisphereInTangentFrame, DEFAULT_TMAX };
        HitInfo rayOut;
        rayOut.Color = float3(0, 0, 0);
        rayOut.TMax = DEFAULT_TMAX;
        rayOut.Flags = 1; // Do not spawn new rays.
        rayOut.RecursionLevel = rayIn.RecursionLevel + 1;
        TraceRay(SceneBVH, 0, 0xFF, 0, 0, 0, ray, rayOut);
        accumulatedIrradiance += rayOut.Color;
    }
    rayIn.Color = accumulatedIrradiance / 32;
}

[shader("closesthit")]
void MaterialEmissiveRed(inout HitInfo rayIn, Attributes attrib)
{
    rayIn.Color = float3(1, 0, 0);
}

[shader("closesthit")]
void MaterialEmissiveGreen(inout HitInfo rayIn, Attributes attrib)
{
    rayIn.Color = float3(0, 1, 0);
}