#pragma once

#include "Core_Object.h"
#include "Scene_ParametricUV.h"

class Plane : public Object, public ParametricUV
{
public:
    float3 getVertexPosition(float2 uv) override;
};