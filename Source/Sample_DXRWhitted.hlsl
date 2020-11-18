#include "Sample_DXR_Common.inc"
#include "Sample_DXR_Implicit.inc"
#include "Sample_DXR_RayRecurse.inc"
#include "Sample_DXR_Shaders.inc"

static const int MAXIMUM_RAY_RECURSION_DEPTH = 4;

[shader("closesthit")]
void MaterialCheckerboard(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    static const float3 lightDirection = normalize(float3(1, 1, -1));
    rayPayload.IntersectionT = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Shadow Ray.
    if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && rayPayload.Flags != 65535)
    {
        if (ShadowRay(worldHit, lightDirection))
        {
            rayPayload.Color = float3(0, 0, 0);
            return;
        }
    }
    // Basic Checkerboard Albedo.
    float3 colorAlbedo;
    {
        float x = (worldHit.x - floor(worldHit.x)) * 2;
        float z = (worldHit.z - floor(worldHit.z)) * 2;
        float blackOrWhite = ((int)x + (int)z) % 2;
        colorAlbedo = float3(blackOrWhite, blackOrWhite, blackOrWhite);
    }
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(intersectionAttributes.Normal);
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_PLASTIC);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, reflect(WorldRayDirection(), intersectionAttributes.Normal), rayPayload, RAY_WAS_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), intersectionAttributes.Normal), 64);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
}

[shader("closesthit")]
void MaterialPlastic(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    static const float3 lightDirection = normalize(float3(1, 1, -1));
    rayPayload.IntersectionT = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // Shadow Ray.
    if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && rayPayload.Flags != 65535)
    {
        if (ShadowRay(worldHit, lightDirection))
        {
            rayPayload.Color = float3(0, 0, 0);
            return;
        }
    }
    // Solid Red Albedo.
    float3 colorAlbedo = Albedo;
    // Basic Lambertian Diffuse.
    float3 colorDiffuse = colorAlbedo * lambert(intersectionAttributes.Normal);
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_PLASTIC);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, reflect(WorldRayDirection(), intersectionAttributes.Normal), rayPayload, RAY_WAS_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(reflect(WorldRayDirection(), intersectionAttributes.Normal), 8);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
}

[shader("closesthit")]
void MaterialGlass(inout RayPayload rayPayload, in IntersectionAttributes intersectionAttributes)
{
    rayPayload.IntersectionT = RayTCurrent();
    const float3 worldHit = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    const float3 vectorReflect = reflect(WorldRayDirection(), intersectionAttributes.Normal);
    // Refraction.
    float3 colorDiffuse = float3(0, 0, 0);
    {
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFLECTED) == 0)
        {
            colorDiffuse = RecurseRay(worldHit, refract2(WorldRayDirection(), intersectionAttributes.Normal, IOR_GLASS), rayPayload, RAY_WAS_REFRACTED);
        }
    }
    // Schlick Fresnel Reflection.
    {
        float fresnel = schlick(WorldRayDirection(), intersectionAttributes.Normal, IOR_VACUUM, IOR_GLASS);
        if (rayPayload.RecursionLevel < MAXIMUM_RAY_RECURSION_DEPTH && (rayPayload.Flags & RAY_WAS_REFRACTED) == 0)
        {
            colorDiffuse = lerp(colorDiffuse, RecurseRay(worldHit, vectorReflect, rayPayload, RAY_WAS_REFLECTED), fresnel);
        }
    }
    // Phong Specular.
    float3 colorSpecular = phong(vectorReflect, 256);
    // Final Color.
    rayPayload.Color = colorDiffuse + colorSpecular;
}