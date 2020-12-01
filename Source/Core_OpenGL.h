#pragma once

#include "Core_Object.h"
#include <Windows.h>
#include <memory>

// This GL header (annoyingly) has a dependency on windows.h
#include <gl/GL.h>

class OpenGLDevice : public Object {
public:
  virtual HGLRC GetHLGRC() = 0;
};

std::shared_ptr<OpenGLDevice> CreateOpenGLDevice();