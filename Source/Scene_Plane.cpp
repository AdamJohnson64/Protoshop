#include "Scene_Plane.h"
#include <math.h>

float3 Plane::getVertexPosition(float2 uv)
{
    return { -1 + uv.X * 2, sinf(uv.X * 20.0f) * 0.25f, -1 + uv.Y * 2 };
}