#pragma once

#include "Core_D3D12.h"
#include "Core_Math.h"
#include <atlbase.h>
#include <d3d12.h>
#include <memory>
#include <vector>

D3D12_RAYTRACING_INSTANCE_DESC Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup, D3D12_GPU_VIRTUAL_ADDRESS tlas);

CComPtr<ID3D12Resource> DXRCreateTLAS(std::shared_ptr<Direct3D12Device> device, const D3D12_RAYTRACING_INSTANCE_DESC* instances, int instanceCount);