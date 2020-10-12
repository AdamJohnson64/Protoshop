#pragma once

#include "Core_Math.h"
#include "Core_Object.h"

class ParametricUV
{
public:
    virtual Vector3 getVertexPosition(Vector2 uv) = 0;
};