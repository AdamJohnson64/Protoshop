#pragma once

#include "Core_Math.h"

class ParametricUV
{
public:
    virtual float3 getVertexPosition(float2 uv) = 0;
};