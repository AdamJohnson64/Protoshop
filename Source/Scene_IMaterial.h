#pragma once

#include "Core_Object.h"
#include <string>

class IMaterial {
public:
  virtual ~IMaterial() = default;
};

class Checkerboard : public Object, public IMaterial {};

class RedPlastic : public Object, public IMaterial {};

class TextureImage : public Object {
public:
  std::string Filename;
};

class Textured : public Object, public IMaterial {
public:
  std::shared_ptr<TextureImage> AlbedoMap;
};