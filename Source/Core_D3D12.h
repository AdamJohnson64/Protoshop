#pragma once

#include "Core_Object.h"
#include <d3d12.h>
#include <memory>

class Direct3D12Device : public Object
{
public:
    virtual ID3D12Device6* GetID3D12Device() = 0;
    virtual ID3D12CommandQueue* GetID3D12CommandQueue() = 0;
    virtual ID3D12DescriptorHeap* GetID3D12DescriptorHeapRTV() = 0;
};

std::shared_ptr<Direct3D12Device> CreateDirect3D12Device();