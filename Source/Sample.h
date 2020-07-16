#pragma once

#include "Core_Object.h"
#include <stdint.h>

class Sample : public Object
{
public:
    virtual void Render() = 0;
};

class MouseListener : public Object
{
public:
    virtual void MouseMove(int32_t x, int32_t y) = 0;
    virtual void MouseDown(int32_t x, int32_t y) = 0;
    virtual void MouseUp(int32_t x, int32_t y) = 0;
};