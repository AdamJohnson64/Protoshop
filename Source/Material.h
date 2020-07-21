#pragma once

#include "Core_Object.h"

class Material
{
public:
    virtual ~Material() = default;
};

class Checkerboard : public Object, public Material
{
};

class RedPlastic : public Object, public Material
{
};