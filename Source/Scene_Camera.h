#pragma once

#include "Core_Math.h"

Matrix44 GetCameraWorldToClip();
void SetCameraWorldToView(const Matrix44& viewProjection);