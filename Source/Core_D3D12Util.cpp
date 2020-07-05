#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include <atlbase.h>
#include <functional>

void RunOnGPU(std::shared_ptr<Direct3D12Device> device, std::function<void(ID3D12GraphicsCommandList*)> fn)
{
    CComPtr<ID3D12GraphicsCommandList> pD3D12GraphicsCommandList;
    {
        CComPtr<ID3D12CommandAllocator> pD3D12CommandAllocator;
        TRYD3D(device->GetID3D12Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&pD3D12CommandAllocator));
        pD3D12CommandAllocator->SetName(L"D3D12CommandAllocator");
        TRYD3D(device->GetID3D12Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pD3D12CommandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&pD3D12GraphicsCommandList));
        pD3D12GraphicsCommandList->SetName(L"D3D12GraphicsCommandList");
    }
    fn(pD3D12GraphicsCommandList);
    pD3D12GraphicsCommandList->Close();
    device->GetID3D12CommandQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)&pD3D12GraphicsCommandList.p);
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
};
