#pragma once

#include "Core_Object.h"
#include "Scene_IParametricUV.h"

class Plane : public Object, public IParametricUV {
public:
  Vector3 getVertexPosition(Vector2 uv) override;
  Vector3 getVertexNormal(Vector2 uv) override;
};