#pragma once

#include "Core_Object.h"
#include <string>

class Material {
public:
  virtual ~Material() = default;
};

class Checkerboard : public Object, public Material {};

class RedPlastic : public Object, public Material {};

class Textured : public Object, public Material {
public:
  std::string AlbedoMap;
};