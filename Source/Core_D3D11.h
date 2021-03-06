#pragma once

#include "Core_Object.h"
#include <memory>

class ID3D11Device;
class ID3D11DeviceContext;

class Direct3D11Device : public Object {
public:
  virtual ID3D11Device *GetID3D11Device() = 0;
  virtual ID3D11DeviceContext *GetID3D11DeviceContext() = 0;
};

std::shared_ptr<Direct3D11Device> CreateDirect3D11Device();