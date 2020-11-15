#pragma once

#include <atlbase.h>
#include <cstdint>
#include <d3d12.h>

///////////////////////////////////////////////////////////////////////////////
// NOTE: This in only one potential signature we may wish to use and this
// corresponds to our most basic requirements for our samples. We're not
// too picky about our descriptor heaps and we have a few here with just
// enough space in them to be useful. These descriptor heaps have no
// controls or management for the purpose of demo without getting into
// the gritty details of descriptor allocation and reuse.
///////////////////////////////////////////////////////////////////////////////
class Sample_D3D12Signature
{
public:
    Sample_D3D12Signature(ID3D12Device* pDevice);
    CComPtr<ID3D12RootSignature> m_pRootSignature;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeapCBVSRVUAV;
};

const uint32_t DESCRIPTOR_HEAP_CBV = 0;
const uint32_t DESCRIPTOR_HEAP_SRV = 1;
const uint32_t DESCRIPTOR_HEAP_SAMPLER = 2;
//const uint32_t DESCRIPTOR_HEAP_UAV = 3;