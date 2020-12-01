#pragma once

#include "Core_Math.h"

Matrix44 GetCameraWorldToClip();

Matrix44 GetCameraWorldToView();

void SetCameraWorldToView(const Matrix44 &viewProjection);