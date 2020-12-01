#include "Scene_Plane.h"

Vector3 Plane::getVertexPosition(Vector2 uv) {
  return {-1 + uv.X * 2, 0, -1 + uv.Y * 2};
}

Vector3 Plane::getVertexNormal(Vector2 uv) { return {0, 1, 0}; }