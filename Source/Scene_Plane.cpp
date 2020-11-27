#include "Scene_Plane.h"
#include <math.h>

Vector3 Plane::getVertexPosition(Vector2 uv)
{
    return { -1 + uv.X * 2, 0, -1 + uv.Y * 2 };
}