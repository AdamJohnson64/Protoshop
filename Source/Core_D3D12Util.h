#pragma once

#include "Core_D3D12.h"
#include <functional>

void RunOnGPU(std::shared_ptr<Direct3D12Device> device, std::function<void(ID3D12GraphicsCommandList*)> fn);