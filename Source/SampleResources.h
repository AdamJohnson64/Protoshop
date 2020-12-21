#pragma once

#include "Core_Math.h"
#include <d3d11.h>

struct SampleResources {
  Matrix44 TransformWorldToClip;
  Matrix44 TransformWorldToView;
};

struct SampleResourcesD3D11 : public SampleResources {
  ID3D11Texture2D *BackBufferTexture;
  ID3D11DepthStencilView *DepthStencilView;
};