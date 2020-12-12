#pragma once

#include "Core_Object.h"
#include <string>

class IMaterial {};

class ITexture {};

class Checkerboard : public Object, public IMaterial {};

class RedPlastic : public Object, public IMaterial {};

class Textured : public Object, public IMaterial {
public:
  std::string AlbedoMap;
};