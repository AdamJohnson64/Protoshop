#pragma once

#include "Core_D3D12.h"
#include <functional>

uint32_t D3D12Align(uint32_t size, uint32_t alignSize);

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize);

ID3D12Resource1* D3D12CreateBuffer(std::shared_ptr<Direct3D12Device> device, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, uint32_t bufferSize, uint32_t dataSize, const void* data);

D3D12_RESOURCE_BARRIER D3D12MakeResourceTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

void D3D12WaitForGPUIdle(std::shared_ptr<Direct3D12Device> device);

void RunOnGPU(std::shared_ptr<Direct3D12Device> device, std::function<void(ID3D12GraphicsCommandList5*)> fn);