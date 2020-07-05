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
        // Create a root signature.
        {
            CComPtr<ID3DBlob> pD3D12BlobSignature;
            CComPtr<ID3DBlob> pD3D12BlobError;
            D3D12_ROOT_SIGNATURE_DESC descSignature = {};
            D3D12_ROOT_PARAMETER parameters[2] = {};
            D3D12_DESCRIPTOR_RANGE range[2] = {};
            range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range[0].NumDescriptors = 1;
            parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameters[0].DescriptorTable.pDescriptorRanges = &range[0];
            parameters[0].DescriptorTable.NumDescriptorRanges = 1;
            parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            range[1].NumDescriptors = 1;
            parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            parameters[1].DescriptorTable.pDescriptorRanges = &range[1];
            parameters[1].DescriptorTable.NumDescriptorRanges = 1;
            parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            descSignature.pParameters = parameters;
            descSignature.NumParameters = 2;
            descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pD3D12BlobSignature, &pD3D12BlobError);
            if (nullptr != pD3D12BlobError)
            {
                throw std::exception(reinterpret_cast<const char*>(pD3D12BlobError->GetBufferPointer()));
            }
            TRYD3D(pD3D12Device->CreateRootSignature(0, pD3D12BlobSignature->GetBufferPointer(), pD3D12BlobSignature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pD3D12RootSignature));
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
        ////////////////////////////////////////////////////////////////////////////////
        // Create a descriptor heap for the SRVs.
        {
            D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
            descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            descDescriptorHeap.NumDescriptors = 1;
            descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            TRYD3D(pD3D12Device->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&pD3D12DescriptorHeapCBVSRVUAV));
            pD3D12DescriptorHeapCBVSRVUAV->SetName(L"D3D12DescriptorHeap (CBV/SRV/UAV)");
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Create a descriptor heap for samplers.
        {
            D3D12_DESCRIPTOR_HEAP_DESC descDescriptorHeap = {};
            descDescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            descDescriptorHeap.NumDescriptors = 1;
            descDescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            TRYD3D(pD3D12Device->CreateDescriptorHeap(&descDescriptorHeap, __uuidof(ID3D12DescriptorHeap), (void**)&pD3D12DescriptorHeapSMP));
            pD3D12DescriptorHeapSMP->SetName(L"D3D12DescriptorHeap (SMP)");
        }
        {
            D3D12_SAMPLER_DESC descSampler = {};
            descSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            descSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            descSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            pD3D12Device->CreateSampler(&descSampler, pD3D12DescriptorHeapSMP->GetCPUDescriptorHandleForHeapStart());
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
    ID3D12RootSignature* GetID3D12RootSignature() override
    {
        return pD3D12RootSignature.p;
    }
    ID3D12DescriptorHeap* GetID3D12DescriptorHeapRTV() override
    {
        return pD3D12DescriptorHeapRTV.p;
    }
    ID3D12DescriptorHeap* GetID3D12DescriptorHeapCBVSRVUAV() override
    {
        return pD3D12DescriptorHeapCBVSRVUAV.p;
    }
    ID3D12DescriptorHeap* GetID3D12DescriptorHeapSMP() override
    {
        return pD3D12DescriptorHeapSMP.p;
    }
private:
    CComPtr<ID3D12Device6> pD3D12Device;
    CComPtr<ID3D12CommandQueue> pD3D12CommandQueue;
    CComPtr<ID3D12DescriptorHeap> pD3D12DescriptorHeapRTV;
    CComPtr<ID3D12DescriptorHeap> pD3D12DescriptorHeapCBVSRVUAV;
    CComPtr<ID3D12DescriptorHeap> pD3D12DescriptorHeapSMP;
    CComPtr<ID3D12RootSignature> pD3D12RootSignature;
};

std::shared_ptr<Direct3D12Device> CreateDirect3D12Device()
{
    return std::shared_ptr<Direct3D12Device>(new Direct3D12DeviceImpl());
}