#include "Core_D3D.h"
#include "Core_D3D12.h"
#include <atlbase.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>

#pragma comment(lib, "d3d12.lib")

class Direct3D12DeviceImpl : public Direct3D12Device
{
public:
    Direct3D12DeviceImpl()
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Enable D3D12 Debugging (note: DXGI may fail when tearing down the swapchain).
        {
            CComPtr<ID3D12Debug3> pD3D12Debug;
            TRYD3D(D3D12GetDebugInterface(__uuidof(ID3D12Debug3), (void**)&pD3D12Debug));
            // By all means turn this on when you're debugging, but this will crash on exit due to the swap chain.
            pD3D12Debug->EnableDebugLayer();
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Find a usable WARP adapter.
        CComPtr<IDXGIFactory7> pDXGIFactory7;
        TRYD3D(CreateDXGIFactory(__uuidof(IDXGIFactory7), (void**)&pDXGIFactory7));
        CComPtr<IDXGIAdapter4> pDXGIAdapter4Warp;
        // Comment out this line to use a real hardware GPU (and not the WARP adapter).
        //TRYD3D(pDXGIFactory7->EnumWarpAdapter(__uuidof(IDXGIAdapter4), (void**)&pDXGIAdapter4Warp));
        ////////////////////////////////////////////////////////////////////////////////
        // Create the Direct3D 12 device.
        TRYD3D(D3D12CreateDevice(pDXGIAdapter4Warp, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device6), (void**)&pD3D12Device));
        pD3D12Device->SetName(L"D3D12Device");
        ////////////////////////////////////////////////////////////////////////////////
        // Create our primary command queue.
        {
            D3D12_COMMAND_QUEUE_DESC descQueue = {};
            descQueue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            TRYD3D(pD3D12Device->CreateCommandQueue(&descQueue, __uuidof(ID3D12CommandQueue), (void**)&pD3D12CommandQueue));
            pD3D12CommandQueue->SetName(L"D3D12CommandQueue");
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Create a descriptor heap for the primary RTV.
        {
            D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
            descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            descDescriptorHeap.NumDescriptors = 1;
            TRYD3D(pD3D12Device->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&pD3D12DescriptorHeapRTV));
            pD3D12DescriptorHeapRTV->SetName(L"D3D12DescriptorHeap (RTV)");
        }
    }
    ID3D12Device6* GetID3D12Device() override
    {
        return pD3D12Device.p;
    }
    ID3D12CommandQueue* GetID3D12CommandQueue() override
    {
        return pD3D12CommandQueue.p;
    }
    ID3D12DescriptorHeap* GetID3D12DescriptorHeapRTV() override
    {
        return pD3D12DescriptorHeapRTV.p;
    }
private:
    CComPtr<ID3D12Device6> pD3D12Device;
    CComPtr<ID3D12CommandQueue> pD3D12CommandQueue;
    CComPtr<ID3D12DescriptorHeap> pD3D12DescriptorHeapRTV;
};

std::shared_ptr<Direct3D12Device> CreateDirect3D12Device()
{
    return std::shared_ptr<Direct3D12Device>(new Direct3D12DeviceImpl());
}