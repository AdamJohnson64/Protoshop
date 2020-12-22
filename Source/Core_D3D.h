#pragma once

#include "Core_Math.h"

#define TRYD3D(FN)                                                             \
  if (S_OK != (FN))                                                            \
    throw #FN;

__declspec(align(16)) struct ConstantsWorld {
  Matrix44 TransformWorldToClip;
  Matrix44 TransformWorldToView;
  Matrix44 TransformWorldToClipShadow;        // Only used for shadow mapping.
  Matrix44 TransformWorldToClipShadowInverse; // Only used for shadow mapping.
  Vector3 CameraPosition;
  float Pad0;
  Vector3 LightPosition;
  float Pad1;
};

__declspec(align(16)) struct ConstantsObject {
  Matrix44 TransformObjectToWorld;
};

struct VertexVS {
  Vector3 Position;
  Vector3 Normal;
  Vector2 Texcoord;
};

extern const int kSamplerRegisterDefaultWrap;
extern const int kSamplerRegisterDefaultBorder;

extern const int kTextureRegisterAlbedoMap;
extern const int kTextureRegisterNormalMap;
extern const int kTextureRegisterDepthMap;
extern const int kTextureRegisterMaskMap;
extern const int kTextureRegisterShadowMap;