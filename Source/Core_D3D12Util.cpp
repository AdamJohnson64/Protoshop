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

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize)
{
    D3D12_HEAP_PROPERTIES descHeapProperties = {};
    descHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC descResource = {};
    descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    descResource.Width = bufferSize;
    descResource.Height = 1;
    descResource.DepthOrArraySize = 1;
    descResource.MipLevels = 1;
    descResource.SampleDesc.Count = 1;
    descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    descResource.Flags = flags;
    ID3D12Resource1* result = nullptr;
    TRYD3D(device->GetID3D12Device()->CreateCommittedResource(&descHeapProperties, D3D12_HEAP_FLAG_NONE, &descResource, state, nullptr, __uuidof(ID3D12Resource1), (void**)&result));
    TRYD3D(result->SetName(L"D3D12CreateBuffer"));
    return result;
}

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, uint32_t dataSize, const void* data)
{
    ID3D12Resource1* result = D3D12CreateBuffer(device, flags, D3D12_RESOURCE_STATE_COPY_DEST, bufferSize);
    CComPtr<ID3D12Resource1> upload;
    D3D12_HEAP_PROPERTIES descHeapProperties = {};
    descHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC descResource = {};
    descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    descResource.Width = bufferSize;
    descResource.Height = 1;
    descResource.DepthOrArraySize = 1;
    descResource.MipLevels = 1;
    descResource.SampleDesc.Count = 1;
    descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    TRYD3D(device->GetID3D12Device()->CreateCommittedResource(&descHeapProperties, D3D12_HEAP_FLAG_NONE, &descResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource1), (void**)&upload));
    TRYD3D(upload->SetName(L"D3D12CreateBuffer_UPLOAD"));
    void *pTLAS = nullptr;
    TRYD3D(upload->Map(0, nullptr, &pTLAS));
    memcpy(pTLAS, data, dataSize);
    upload->Unmap(0, nullptr);
    // Copy this staging buffer to the GPU-only buffer.
    RunOnGPU(device, [&](ID3D12GraphicsCommandList5* uploadCommandList) {
        uploadCommandList->CopyResource(result, upload);
        {
            D3D12_RESOURCE_BARRIER descBarrier = {};
            descBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            descBarrier.Transition.pResource = result;
            descBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            descBarrier.Transition.StateAfter = state;
            uploadCommandList->ResourceBarrier(1, &descBarrier);
        }
    });
    return result;
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
