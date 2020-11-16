#pragma once

#include "Sample.h"
#include "Core_DXGI.h"
#include "Core_D3D12.h"
#include <atlbase.h>
#include <memory>

class Sample_DXRBase : public Sample
{
public:
    Sample_DXRBase(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice);
    void Render() override;
    virtual void RenderSample() = 0;
    virtual void RenderPost(ID3D12GraphicsCommandList5* pCommandList);
protected:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D12Device> m_pDevice;
    CComPtr<ID3D12RootSignature> m_pRootSignature;
    CComPtr<ID3D12Resource1> m_pResourceTargetUAV;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeapCBVSRVUAV;
};