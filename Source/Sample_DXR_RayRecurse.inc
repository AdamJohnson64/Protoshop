#pragma once

#include "Sample_DXR_Common.inc"

///////////////////////////////////////////////////////////////////////////////
// Common Raytracer Structures
///////////////////////////////////////////////////////////////////////////////

// All ray hit results return a structure like this. We record the TMax (aka
// maximum intersection lambda) as a means to capture the hit distance. Since
// we're using a globally consistent maximum this makes it relatively easy to
// determine the no-hit condition.

struct RayPayload
{
    float3 Color;
    float IntersectionT;
    int RecursionLevel;
    int Flags;
};

// Initialize a ray payload of the above structure. The ray generator will link
// this statically so this decouples the ray generator from initialization.
RayPayload InitializeRayPayload()
{
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    rayPayload.IntersectionT = DEFAULT_TMAX;
    rayPayload.RecursionLevel = 1;
    rayPayload.Flags = RAY_IS_PRIMARY;
    return rayPayload;
}

///////////////////////////////////////////////////////////////////////////////
// Handy Dandy Recursion Helpers
///////////////////////////////////////////////////////////////////////////////

// Fire off another ray from the origin site with a small offset to avoid self
// intersection. We automatically bump the recursion counter but we don't
// actually test for a breach of the ray recursion limit here; the client must
// do this.
float3 RecurseRay(float3 origin, float3 direction, in RayPayload rayPayload, int rayType)
{
    RayDesc rayDesc = { origin + direction * DEFAULT_TMIN, 0, direction, DEFAULT_TMAX };
    RayPayload recurseRayPayload;
    recurseRayPayload.Color = float3(0, 0, 0);
    recurseRayPayload.IntersectionT = DEFAULT_TMAX;
    recurseRayPayload.RecursionLevel = rayPayload.RecursionLevel + 1;
    recurseRayPayload.Flags = rayPayload.Flags | rayType;
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, recurseRayPayload);
    return recurseRayPayload.Color;
}

// Fire a shadow ray and check for any hit. We use the intersection lambda
// (T) to determine if the ray hit any geometry.
bool ShadowRay(float3 origin, float3 direction)
{
    RayDesc rayDesc = { origin + direction * DEFAULT_TMIN, 0, direction, DEFAULT_TMAX };
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    rayPayload.IntersectionT = DEFAULT_TMAX;
    rayPayload.RecursionLevel = 65536; // Do not recurse.
    rayPayload.Flags = 65535; // Do not spawn new rays.
    TraceRay(raytracingAccelerationStructure, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, rayDesc, rayPayload);
    return rayPayload.IntersectionT < DEFAULT_TMAX;
}