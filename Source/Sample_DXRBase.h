#pragma once

#include "Sample.h"
#include "Sample_D3D12Signature.h"
#include "Core_DXGI.h"
#include "Core_D3D12.h"
#include <atlbase.h>
#include <memory>

// TODO: We really, really, REALLY don't want to mixin the D3D12Signature here.
// All DXR samples need a CBV descriptor heap which is the only thing we really
// need from the signature. The root signature for the DXR samples is created
// here and is VERY DIFFERENT from the standard root signature of any other
// sample. We don't want two signatures in our brain.
class Sample_DXRBase : public Sample, public Sample_D3D12Signature
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
};