#pragma once

#include "Core_D3D11.h"
#include "SampleResources.h"
#include <functional>
#include <memory>

class SampleRequestD3D11 {
public:
  std::shared_ptr<Direct3D11Device> Device;
  std::function<void(const SampleResourcesD3D11 &)> Render;
  std::function<void(char)> KeyDown;
};