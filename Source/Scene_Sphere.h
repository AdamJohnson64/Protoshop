#pragma once

#include "Core_Object.h"
#include "Scene_IParametricUV.h"

class Sphere : public Object, public IParametricUV {
public:
  Vector3 getVertexPosition(Vector2 uv) override;
  Vector3 getVertexNormal(Vector2 uv) override;
};