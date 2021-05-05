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

float4 _RayGenerationMVPClipBase()
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
    return float4(rayPayload.Color, 1);
}

[shader("raygeneration")]
void RayGenerationMVPClip()
{
    renderTargetOutput[DispatchRaysIndex().xy] = _RayGenerationMVPClipBase();
}

[shader("raygeneration")]
void RayGenerationMVPClip_ACCUMULATE()
{
    renderTargetOutput[DispatchRaysIndex().xy] += _RayGenerationMVPClipBase();
}

// The general miss shader. All rays which hit nothing will return
// black/zero/nothingness in the ray color.

[shader("miss")]
void Miss(inout RayPayload rayPayload)
{
    rayPayload.Color = float3(0, 0, 0);
}

float3 AmbientOcclusion(float3 origin, float3 normal)
{
    float3 localX = float3(1, 0, 0);
    float3 localY = normal;
    float3 localZ = normalize(cross(localX, localY));
    float3x3 tbn = float3x3(localX, localY, localZ);
    localX = normalize(cross(localY, localZ));
    float illuminated = 0;
    int offset = (DispatchRaysIndex().x * DispatchRaysIndex().y * 16);
    for (int i = 0; i < 8; ++i)
    {
        // This incredibly cheap sample jittering of i + x * y * 16 produces a high-frequency
        // Moire interference pattern that looks like old-school dithering; nice.
        float3 hemisphere = HaltonSample(i + offset).xzy;
        float3 hemisphereInTangentFrame = mul(hemisphere, tbn);
        RayDesc rayDesc = { origin + hemisphereInTangentFrame * 0.01, 0, hemisphereInTangentFrame, DEFAULT_TMAX };
        RayPayload rayPayload;
        rayPayload.Color = float3(0, 0, 0);
        TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 1, 0, 1, rayDesc, rayPayload);
        illuminated += rayPayload.Color.r;
    }
    return illuminated / 8;
}

////////////////////////////////////////////////////////////////////////////////
// Tangent Spaces
////////////////////////////////////////////////////////////////////////////////
// Here's the rub with tangent spaces.
// In general there is no good choice of a tangent space that won't exhibit some
// form of discontinuity at some point in space. We'll observe this as a whorl.
// This discontinuity will cause the vector field to spin around a point but the
// basis is still technically orthonormal. For larger numbers of samples The
// directionality will eventually "wash out". For small numbers of samples we
// will observe any bias in the sampling direction as that same directionality.

// Technically speaking this means that ANY choice of orthonormal tangent space
// will *eventually* work. We'll provide alternatives that work reasonably for
// small numbers of samples.

// FAST: Orient along the view direction (world ray direction).
// For small numbers of samples this will produce a discontinuity at V==N.
// This will appear as an annoying whorl in the middle of the screen.
float3x3 TangentBasisFromViewDirection(float3 vectorNormal)
{
  float3 orthoN = vectorNormal;
  float3 orthoT = normalize(cross(orthoN, WorldRayDirection()));
  float3 orthoB = normalize(cross(orthoN, orthoT));
  return float3x3(orthoT, orthoB, orthoN);
}

// SLOW: Orient along some arbitrary axis in the space of the eye.
// For small numbers of samples this will be consistent across the frame.
// Strange things will occur under animation but it should converge cleanly.
float3x3 TangentBasisFromViewAxis(float3 vectorNormal)
{
  float4 eyeFarMidH = mul(TransformClipToWorld, float4(0, 0, 1, 1));
  float3 eyeFarMid = eyeFarMidH.xyz / eyeFarMidH.w;
  float4 eyeFarOtherH = mul(TransformClipToWorld, float4(1, 1, 1, 1));
  float3 eyeFarOther = eyeFarOtherH.xyz / eyeFarOtherH.w;
  float3 eyeAxis = normalize(eyeFarOther - eyeFarMid);
  float3 orthoN = vectorNormal;
  float3 orthoT = normalize(cross(orthoN, eyeAxis));
  float3 orthoB = normalize(cross(orthoN, orthoT));
  return float3x3(orthoT, orthoB, orthoN);
}