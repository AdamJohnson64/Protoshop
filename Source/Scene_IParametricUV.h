#pragma once

#include "Core_Math.h"

class IParametricUV {
public:
  virtual ~IParametricUV() = default;
  virtual Vector3 getVertexPosition(Vector2 uv) = 0;
  virtual Vector3 getVertexNormal(Vector2 uv) = 0;
};