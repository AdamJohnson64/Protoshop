#include "Scene_Camera.h"
#define _USE_MATH_DEFINES
#include <math.h>

static Matrix44 TransformWorldToView = {
    // clang-format off
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, -1, 5, 1
    // clang-format on
};

static Matrix44 TransformViewToClip = CreateProjection<float>(
    0.01f, 100.0f, 45 * (M_PI / 180), 45 * (M_PI / 180));

Matrix44 GetCameraWorldToClip() {
  return TransformWorldToView * TransformViewToClip;
}

Matrix44 GetCameraWorldToView() { return TransformWorldToView; }

void SetCameraWorldToView(const Matrix44 &transformWorldToView) {
  TransformWorldToView = transformWorldToView;
}