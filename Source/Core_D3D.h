#pragma once

#include "Core_Math.h"

#define TRYD3D(FN)                                                             \
  if (S_OK != (FN))                                                            \
    throw #FN;

struct VertexVS {
  Vector3 Position;
  Vector3 Normal;
  Vector2 Texcoord;
};