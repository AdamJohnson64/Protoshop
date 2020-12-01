#include "Scene_Sphere.h"
#include "Core_Math.h"

Vector3 Sphere::getVertexPosition(Vector2 uv) {
  float angleU = uv.X * (2 * Pi<float>);
  float angleV = uv.Y * (1 * Pi<float>);
  return {Sin(angleU) * Sin(angleV), Cos(angleV),
          Cos(angleU) * Sin(angleV)};
}

Vector3 Sphere::getVertexNormal(Vector2 uv) { return getVertexPosition(uv); }