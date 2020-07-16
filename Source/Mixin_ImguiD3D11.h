#pragma once

#include "Core_D3D11.h"
#include "Mixin_ImguiBase.h"
#include <memory>

class Mixin_ImguiD3D11 : public Mixin_ImguiBase
{
public:
    Mixin_ImguiD3D11(std::shared_ptr<Direct3D11Device> pDevice);
    ~Mixin_ImguiD3D11();
protected:
    void RenderImgui();
    virtual void BuildImguiUI() = 0;
};