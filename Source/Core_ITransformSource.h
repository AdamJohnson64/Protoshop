#pragma once

#include "Core_Math.h"

class ITransformSource {
public:
  virtual Matrix44 GetTransformWorldToClip() = 0;
  virtual Matrix44 GetTransformWorldToView() = 0;
};

ITransformSource *GetTransformSource();