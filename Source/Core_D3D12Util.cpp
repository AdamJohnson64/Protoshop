#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include <assert.h>
#include <atlbase.h>
#include <functional>

uint32_t D3D12Align(uint32_t size, uint32_t alignSize)
{
    return size == 0 ? 0 : ((size - 1) / alignSize + 1) * alignSize;
}

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, const D3D12_HEAP_PROPERTIES* heap)
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
    TRYD3D(device->GetID3D12Device()->CreateCommittedResource(heap, D3D12_HEAP_FLAG_NONE, &descResource, state, nullptr, __uuidof(ID3D12Resource1), (void**)&pD3D12Resource.p));
    TRYD3D(pD3D12Resource->SetName(L"D3D12CreateBuffer"));
    return pD3D12Resource.Detach();
}

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize)
{
    D3D12_HEAP_PROPERTIES descHeapDefault = {};
    descHeapDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
    return D3D12CreateBuffer(device, flags, state, bufferSize, &descHeapDefault);
}

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, uint32_t dataSize, const void* data)
{
    CComPtr<ID3D12Resource1> pD3D12Resource;
    pD3D12Resource.p = D3D12CreateBuffer(device, flags, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);
    {
        CComPtr<ID3D12Resource1> pD3D12ResourceUpload;
        {
            D3D12_HEAP_PROPERTIES descHeapUpload = {};
            descHeapUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
            pD3D12ResourceUpload.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, bufferSize, &descHeapUpload);
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
    return pD3D12Resource.Detach();
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

void D3D12WaitForGPUIdle(std::shared_ptr<Direct3D12Device> device)
{
    CComPtr<ID3D12Fence> pD3D12Fence;
    TRYD3D(device->GetID3D12Device()->CreateFence(1, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&pD3D12Fence));
    pD3D12Fence->SetName(L"D3D12Fence");
    device->GetID3D12CommandQueue()->Signal(pD3D12Fence, 123);
    // Set Fence to zero and wait for queue empty.
    HANDLE hWait = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    TRYD3D(pD3D12Fence->SetEventOnCompletion(123, hWait));
    WaitForSingleObject(hWait, INFINITE);
    CloseHandle(hWait);
}

void RunOnGPU(std::shared_ptr<Direct3D12Device> device, std::function<void(ID3D12GraphicsCommandList5*)> fn)
{
    CComPtr<ID3D12GraphicsCommandList5> pD3D12GraphicsCommandList;
    {
        CComPtr<ID3D12CommandAllocator> pD3D12CommandAllocator;
        TRYD3D(device->GetID3D12Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&pD3D12CommandAllocator));
        pD3D12CommandAllocator->SetName(L"D3D12CommandAllocator");
        TRYD3D(device->GetID3D12Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pD3D12CommandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList5), (void**)&pD3D12GraphicsCommandList));
        pD3D12GraphicsCommandList->SetName(L"D3D12GraphicsCommandList");
    }
    fn(pD3D12GraphicsCommandList);
    pD3D12GraphicsCommandList->Close();
    device->GetID3D12CommandQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)&pD3D12GraphicsCommandList.p);
    D3D12WaitForGPUIdle(device);
};
