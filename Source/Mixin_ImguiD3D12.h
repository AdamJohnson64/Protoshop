#pragma once

#include "Core_D3D12.h"
#include "Mixin_ImguiBase.h"
#include <atlbase.h>
#include <memory>

class Mixin_ImguiD3D12 : public Mixin_ImguiBase
{
public:
    Mixin_ImguiD3D12(std::shared_ptr<Direct3D12Device> pDevice);
    ~Mixin_ImguiD3D12();
protected:
    void RenderImgui(ID3D12GraphicsCommandList5* pD3D12GraphicsCommandList);
    std::shared_ptr<Direct3D12Device> m_pDeviceImgui;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeapCBVSRVUAVImGui;
};