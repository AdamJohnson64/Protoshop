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

class OBJMaterial : public Object, public IMaterial {
public:
  // Don't blame me for the names here; these names are legacy OBJ standard.
  std::shared_ptr<TextureImage> DiffuseMap; // This is albedo, not diffuse.
  std::shared_ptr<TextureImage> NormalMap; // Sometimes called "Bump Map" in OBJ.
  std::shared_ptr<TextureImage> DissolveMap; // "Dissolve" is a very strange name for "Alpha Mask".
};