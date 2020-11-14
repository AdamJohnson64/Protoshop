#pragma once

#include "Core_D3D12.h"
#include "Core_Math.h"
#include <atlbase.h>
#include <d3d12.h>
#include <memory>
#include <vector>

D3D12_RAYTRACING_INSTANCE_DESC Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup, D3D12_GPU_VIRTUAL_ADDRESS tlas);

// Create a BLAS for a mesh from extracted vertex and index data in the most boneheaded way possible.
CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device* device, const void* vertices, int vertexCount, DXGI_FORMAT vertexFormat, const void* indices, int indexCount, DXGI_FORMAT indexFormat);

// Create a TLAS from an instance list in the most boneheaded way possible.
CComPtr<ID3D12Resource1> DXRCreateTLAS(Direct3D12Device* device, const D3D12_RAYTRACING_INSTANCE_DESC* instances, int instanceCount);