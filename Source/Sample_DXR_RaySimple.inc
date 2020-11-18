#pragma once

///////////////////////////////////////////////////////////////////////////////
// Common Raytracer Structures
///////////////////////////////////////////////////////////////////////////////

// All ray hit results return a structure like this. This overly simplistic
// payload only includes a color. This is the minimum you'll define for a
// stupid render.

struct RayPayload
{
    float3 Color;
};

// Initialize a ray payload of the above structure. The ray generator will link
// this statically so this decouples the ray generator from initialization.
RayPayload InitializeRayPayload()
{
    RayPayload rayPayload;
    rayPayload.Color = float3(0, 0, 0);
    return rayPayload;
}