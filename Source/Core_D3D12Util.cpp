#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include <array>
#include <assert.h>
#include <atlbase.h>
#include <functional>

CComPtr<ID3D12RootSignature> D3D12_Create_Signature_1CBV1SRV(ID3D12Device* pDevice)
{
    CComPtr<ID3DBlob> pD3D12BlobSignature;
    CComPtr<ID3DBlob> pD3D12BlobError;
    D3D12_ROOT_SIGNATURE_DESC descSignature = {};
    std::array<D3D12_ROOT_PARAMETER, 3> parameters = {};
    std::array<D3D12_DESCRIPTOR_RANGE, 3> range = {};
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
    descSignature.pParameters = &parameters[0];
    descSignature.NumParameters = 3;
    descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pD3D12BlobSignature, &pD3D12BlobError);
    if (nullptr != pD3D12BlobError)
    {
        throw std::exception(reinterpret_cast<const char*>(pD3D12BlobError->GetBufferPointer()));
    }
    CComPtr<ID3D12RootSignature> pRootSignature;
    TRYD3D(pDevice->CreateRootSignature(0, pD3D12BlobSignature->GetBufferPointer(), pD3D12BlobSignature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pRootSignature.p));
    return pRootSignature;
}

CComPtr<ID3D12DescriptorHeap> D3D12_Create_DescriptorHeap_1024CBVSRVUAV(ID3D12Device* pDevice)
{
    D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
    descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descDescriptorHeap.NumDescriptors = 1024;
    descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
    TRYD3D(pDevice->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&pDescriptorHeap.p));
    pDescriptorHeap->SetName(L"D3D12DescriptorHeap (1024 CBV/SRV/UAV)");
    return pDescriptorHeap;
}

CComPtr<ID3D12DescriptorHeap> D3D12_Create_DescriptorHeap_1Sampler(ID3D12Device* pDevice)
{
    CComPtr<ID3D12DescriptorHeap> pDescriptorHeap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
        descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        descDescriptorHeap.NumDescriptors = 1;
        descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        TRYD3D(pDevice->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&pDescriptorHeap));
        pDescriptorHeap->SetName(L"D3D12DescriptorHeap (1 Sampler)");
    }
    // Convenience: Initialize this single sampler with all default settings and wrapping mode in all dimensions.
    {
        D3D12_SAMPLER_DESC descSampler = {};
        descSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        descSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        descSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        pDevice->CreateSampler(&descSampler, pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }
    return pDescriptorHeap;
}

uint32_t D3D12Align(uint32_t size, uint32_t alignSize)
{
    return size == 0 ? 0 : ((size - 1) / alignSize + 1) * alignSize;
}

CComPtr<ID3D12Resource1> D3D12CreateBuffer(Direct3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, const D3D12_HEAP_PROPERTIES* heap)
{
    D3D12_RESOURCE_DESC descResource = {};
    descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    descResource.Width = bufferSize;
    descResource.Height = 1;
    descResource.DepthOrArraySize = 1;
    descResource.MipLevels = 1;
    descResource.SampleDesc.Count = 1;
    descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    descResource.Flags = flags;
    CComPtr<ID3D12Resource1> pD3D12Resource;
    TRYD3D(device->m_pDevice->CreateCommittedResource(heap, D3D12_HEAP_FLAG_NONE, &descResource, state, nullptr, __uuidof(ID3D12Resource1), (void**)&pD3D12Resource.p));
    TRYD3D(pD3D12Resource->SetName(L"D3D12CreateBuffer"));
    return pD3D12Resource;
}

CComPtr<ID3D12Resource1> D3D12CreateBuffer(Direct3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize)
{
    D3D12_HEAP_PROPERTIES descHeapDefault = {};
    descHeapDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
    return D3D12CreateBuffer(device, flags, state, bufferSize, &descHeapDefault);
}

CComPtr<ID3D12Resource1> D3D12CreateBuffer(Direct3D12Device* device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, uint32_t dataSize, const void* data)
{
    CComPtr<ID3D12Resource1> pD3D12Resource = D3D12CreateBuffer(device, flags, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);
    {
        CComPtr<ID3D12Resource1> pD3D12ResourceUpload;
        {
            D3D12_HEAP_PROPERTIES descHeapUpload = {};
            descHeapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
            pD3D12ResourceUpload = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, bufferSize, &descHeapUpload);
        }
        void *pMapped = nullptr;
        TRYD3D(pD3D12ResourceUpload->Map(0, nullptr, &pMapped));
        memcpy(pMapped, data, dataSize);
        pD3D12ResourceUpload->Unmap(0, nullptr);
        // Copy this staging buffer to the GPU-only buffer.
        RunOnGPU(device, [&](ID3D12GraphicsCommandList5* uploadCommandList) {
            uploadCommandList->CopyResource(pD3D12Resource, pD3D12ResourceUpload);
            uploadCommandList->ResourceBarrier(1, &D3D12MakeResourceTransitionBarrier(pD3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, state));
        });
    }
    return pD3D12Resource;
}

D3D12_RECT D3D12MakeRect(LONG width, LONG height)
{
    D3D12_RECT desc = {};
    desc.right = width;
    desc.bottom = height;
    return desc;
}

D3D12_RESOURCE_BARRIER D3D12MakeResourceTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
    D3D12_RESOURCE_BARRIER desc = {};
    desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    desc.Transition.pResource = resource;
    desc.Transition.StateBefore = from;
    desc.Transition.StateAfter = to;
    desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return desc;
}

D3D12_VIEWPORT D3D12MakeViewport(FLOAT width, FLOAT height)
{
    D3D12_VIEWPORT desc = {};
    desc.Width = RENDERTARGET_WIDTH;
    desc.Height = RENDERTARGET_HEIGHT;
    desc.MaxDepth = 1.0f;
    return desc;
}

void D3D12WaitForGPUIdle(Direct3D12Device* device)
{
    CComPtr<ID3D12Fence> pD3D12Fence;
    TRYD3D(device->m_pDevice->CreateFence(1, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&pD3D12Fence));
    pD3D12Fence->SetName(L"D3D12Fence");
    device->m_pCommandQueue->Signal(pD3D12Fence, 123);
    // Set Fence to zero and wait for queue empty.
    HANDLE hWait = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    TRYD3D(pD3D12Fence->SetEventOnCompletion(123, hWait));
    WaitForSingleObject(hWait, INFINITE);
    CloseHandle(hWait);
}

void RunOnGPU(Direct3D12Device* device, std::function<void(ID3D12GraphicsCommandList5*)> fn)
{
    CComPtr<ID3D12GraphicsCommandList5> pD3D12GraphicsCommandList;
    {
        CComPtr<ID3D12CommandAllocator> pD3D12CommandAllocator;
        TRYD3D(device->m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&pD3D12CommandAllocator));
        pD3D12CommandAllocator->SetName(L"D3D12CommandAllocator");
        TRYD3D(device->m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pD3D12CommandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList5), (void**)&pD3D12GraphicsCommandList));
        pD3D12GraphicsCommandList->SetName(L"D3D12GraphicsCommandList");
    }
    fn(pD3D12GraphicsCommandList);
    pD3D12GraphicsCommandList->Close();
    device->m_pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&pD3D12GraphicsCommandList.p);
    D3D12WaitForGPUIdle(device);
};
