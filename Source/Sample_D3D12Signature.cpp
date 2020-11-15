#include "Sample_D3D12Signature.h"

#include "Core_D3D12Util.h"

Sample_D3D12Signature::Sample_D3D12Signature(ID3D12Device* pD3D12Device)
{
    m_pRootSignature = D3D12_Create_Signature_1CBV1SRV(pD3D12Device);
    m_pDescriptorHeapCBVSRVUAV = D3D12_Create_DescriptorHeap_CBVSRVUAV(pD3D12Device, 256);
}