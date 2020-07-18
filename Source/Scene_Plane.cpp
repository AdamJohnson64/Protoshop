#include "Scene_Plane.h"
#include <math.h>

Vector3 Plane::getVertexPosition(Vector2 uv)
{
    // NOTE: We're animating on every vertex lookup - this is horrible but it's fun to look at.
    static float phase = 0;
    phase += 0.00001f;
    return { -1 + uv.X * 2, sinf(uv.X * 20.0f + phase), -1 + uv.Y * 2 };
}