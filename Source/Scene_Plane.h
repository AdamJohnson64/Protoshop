#pragma once

#include "Core_Object.h"
#include "Scene_ParametricUV.h"

class Plane : public Object, public ParametricUV {
public:
  Vector3 getVertexPosition(Vector2 uv) override;
  Vector3 getVertexNormal(Vector2 uv) override;
};