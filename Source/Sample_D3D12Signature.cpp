#include "Sample_D3D12Signature.h"

#include "Core_D3D.h"
#include <exception>

Sample_D3D12Signature::Sample_D3D12Signature(ID3D12Device* pD3D12Device)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Create a root signature.
    {
        CComPtr<ID3DBlob> pD3D12BlobSignature;
        CComPtr<ID3DBlob> pD3D12BlobError;
        D3D12_ROOT_SIGNATURE_DESC descSignature = {};
        D3D12_ROOT_PARAMETER parameters[3] = {};
        D3D12_DESCRIPTOR_RANGE range[3] = {};
        range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        range[0].NumDescriptors = 1;
        parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[0].DescriptorTable.pDescriptorRanges = &range[0];
        parameters[0].DescriptorTable.NumDescriptorRanges = 1;
        parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range[1].NumDescriptors = 1;
        parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[1].DescriptorTable.pDescriptorRanges = &range[1];
        parameters[1].DescriptorTable.NumDescriptorRanges = 1;
        parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        range[2].NumDescriptors = 1;
        parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[2].DescriptorTable.pDescriptorRanges = &range[2];
        parameters[2].DescriptorTable.NumDescriptorRanges = 1;
        parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        descSignature.pParameters = parameters;
        descSignature.NumParameters = 3;
        descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pD3D12BlobSignature, &pD3D12BlobError);
        if (nullptr != pD3D12BlobError)
        {
            throw std::exception(reinterpret_cast<const char*>(pD3D12BlobError->GetBufferPointer()));
        }
        TRYD3D(pD3D12Device->CreateRootSignature(0, pD3D12BlobSignature->GetBufferPointer(), pD3D12BlobSignature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_pRootSignature));
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a descriptor heap for the SRVs.
    {
        D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
        descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descDescriptorHeap.NumDescriptors = 1024;
        descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        TRYD3D(pD3D12Device->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeapCBVSRVUAV));
        m_pDescriptorHeapCBVSRVUAV->SetName(L"D3D12DescriptorHeap (CBV/SRV/UAV)");
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a descriptor heap for samplers.
    {
        D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
        descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        descDescriptorHeap.NumDescriptors = 1;
        descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        TRYD3D(pD3D12Device->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeapSMP));
        m_pDescriptorHeapSMP->SetName(L"D3D12DescriptorHeap (SMP)");
    }
    {
        D3D12_SAMPLER_DESC descSampler = {};
        descSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        descSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        descSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pD3D12Device->CreateSampler(&descSampler, m_pDescriptorHeapSMP->GetCPUDescriptorHandleForHeapStart());
    }
}