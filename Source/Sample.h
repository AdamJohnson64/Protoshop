#pragma once

#include "Core_Object.h"

class Sample : public Object
{
public:
    virtual void Render() = 0;
};