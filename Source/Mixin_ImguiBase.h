#pragma once

#include "Sample.h"
#include <imgui.h>

class Mixin_ImguiBase : public MouseListener
{
public:
    Mixin_ImguiBase();
    ~Mixin_ImguiBase();
protected:
    virtual void BuildImguiUI() = 0;
    void MouseMove(int32_t x, int32_t y);
    void MouseDown(int32_t x, int32_t y);
    void MouseUp(int32_t x, int32_t y);
    ImGuiContext* pImgui;
};