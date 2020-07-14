#include "Scene_Plane.h"
#include <math.h>

float3 Plane::getVertexPosition(float2 uv)
{
    static float phase = 0;
    phase += 0.00001f;
    return { -1 + uv.X * 2, sinf(uv.X * 20.0f + phase) * 0.25f, -1 + uv.Y * 2 };
}