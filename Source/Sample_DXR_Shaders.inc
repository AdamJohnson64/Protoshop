#pragma once

#include "Sample_DXR_Common.inc"

///////////////////////////////////////////////////////////////////////////////
//
// Common Raytracer Entrypoints
//
// Before importing this file you should have:
// - struct RayPayload;
// - RayPayload InitializeRayPayload();
//
// These can be obtained from:
// - Sample_DXR_RayRecurse.inc
// OR
// - Sample_DXR_RaySimple.inc
//
///////////////////////////////////////////////////////////////////////////////

// This is the accompanying function for the ray generator. Here we get a good
// match to a raster render using the inverse of the MVP transform (general
// vertex transform) to UN-project the homogeneous coordinates of the corners
// of the clip-box back into world space. The rays generated here will match
// the "rays" that the rasterizers would generate +/- some amount of float
// precision loss.

[shader("raygeneration")]
void RayGenerationMVPClip()
{
    const float NormalizedX = -1 + 2 * (float)DispatchRaysIndex().x / (float)DispatchRaysDimensions().x;
    const float NormalizedY = 1 - 2 * (float)DispatchRaysIndex().y / (float)DispatchRaysDimensions().y;
    RayDesc rayDesc;
    float4 front = mul(TransformClipToWorld, float4(NormalizedX, NormalizedY, 0, 1));
    front /= front.w;
    float4 back = mul(TransformClipToWorld, float4(NormalizedX, NormalizedY, 1, 1));
    back /= back.w;
    rayDesc.Origin = front.xyz;
    rayDesc.TMin = DEFAULT_TMIN;
    rayDesc.Direction = normalize(back.xyz - front.xyz);
    rayDesc.TMax = DEFAULT_TMAX;
    RayPayload rayPayload = InitializeRayPayload();
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    renderTargetOutput[DispatchRaysIndex().xy] = float4(rayPayload.Color, 1);
}

// The general miss shader. All rays which hit nothing will return
// black/zero/nothingness in the ray color.

[shader("miss")]
void Miss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0, 0, 0);
}