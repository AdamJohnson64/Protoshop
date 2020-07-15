#include "Scene_Sphere.h"
#define _USE_MATH_DEFINES
#include <math.h>

float3 Sphere::getVertexPosition(float2 uv)
{
    float angleU = uv.X * (2 * M_PI);
    float angleV = uv.Y * (1 * M_PI);
    return { sinf(angleU) * sinf(angleV), cosf(angleV), cosf(angleU) * sinf(angleV) };
}