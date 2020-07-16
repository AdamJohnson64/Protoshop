#include "Mixin_ImguiBase.h"
#include <imgui.h>

Mixin_ImguiBase::Mixin_ImguiBase() :
    pImgui(ImGui::CreateContext())
{
}

Mixin_ImguiBase::~Mixin_ImguiBase()
{
    ImGui::DestroyContext(pImgui);
}

void Mixin_ImguiBase::MouseMove(int32_t x, int32_t y)
{
    ImGui::SetCurrentContext(pImgui);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos.x = x;
    io.MousePos.y = y;
}

void Mixin_ImguiBase::MouseDown(int32_t x, int32_t y)
{
    ImGui::SetCurrentContext(pImgui);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = true;
}

void Mixin_ImguiBase::MouseUp(int32_t x, int32_t y)
{
    ImGui::SetCurrentContext(pImgui);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = false;
}