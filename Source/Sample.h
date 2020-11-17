#pragma once

#include "Core_Object.h"
#include <stdint.h>

class Sample : public Object
{
public:
    virtual void Render() = 0;
};