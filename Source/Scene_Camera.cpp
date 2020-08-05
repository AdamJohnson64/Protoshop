#include "Scene_Camera.h"
#define _USE_MATH_DEFINES
#include <math.h>

static Matrix44 cameraVP = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, -1, 5, 1
};

Matrix44 GetCameraViewProjection()
{
    Matrix44 project = CreateProjection<float>(0.01f, 100.0f, 45 / (2 * M_PI), 45 / (2 * M_PI));
    return cameraVP * project;
}

void SetCameraViewProjection(const Matrix44& viewProjection)
{
    cameraVP = viewProjection;
}