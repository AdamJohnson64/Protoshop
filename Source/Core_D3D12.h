#pragma once

#include "Core_Object.h"
#include <d3d12.h>
#include <memory>

class Direct3D12Device : public Object
{
public:
    virtual ID3D12Device6* GetID3D12Device() = 0;
    virtual ID3D12CommandQueue* GetID3D12CommandQueue() = 0;
    virtual ID3D12RootSignature* GetID3D12RootSignature() = 0;
    virtual ID3D12DescriptorHeap* GetID3D12DescriptorHeapRTV() = 0;
    virtual ID3D12DescriptorHeap* GetID3D12DescriptorHeapCBVSRVUAV() = 0;
    virtual ID3D12DescriptorHeap* GetID3D12DescriptorHeapSMP() = 0;
};

const uint32_t DESCRIPTOR_HEAP_CBV = 0;
const uint32_t DESCRIPTOR_HEAP_SRV = 1;
const uint32_t DESCRIPTOR_HEAP_SAMPLER = 2;
//const uint32_t DESCRIPTOR_HEAP_UAV = 3;

std::shared_ptr<Direct3D12Device> CreateDirect3D12Device();