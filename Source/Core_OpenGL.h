#pragma once

#include "Core_Object.h"
#include <windows.h>
#include <gl/GL.h>
#include <memory>

class OpenGLDevice : public Object
{
public:
    virtual HGLRC GetHLGRC() = 0;
};

std::shared_ptr<OpenGLDevice> CreateOpenGLDevice();