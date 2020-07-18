#pragma once

#include "Core_Math.h"

class ParametricUV
{
public:
    virtual Vector3 getVertexPosition(Vector2 uv) = 0;
};