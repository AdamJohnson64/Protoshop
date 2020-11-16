#pragma once

#include "Core_D3D12.h"
#include "Core_Math.h"
#include <atlbase.h>
#include <d3d12.h>
#include <memory>
#include <vector>

// Create a DXR compatible root signature that contains the required output UAV
// and an SRV for the acceleration structure.
CComPtr<ID3D12RootSignature> DXR_Create_Signature_1UAV1SRV(ID3D12Device* device);

// Create a standard output UAV of the correct pixel format and sized to our default resolution.
CComPtr<ID3D12Resource1> DXR_Create_Output_UAV(ID3D12Device* device);

D3D12_RAYTRACING_INSTANCE_DESC Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup, D3D12_GPU_VIRTUAL_ADDRESS tlas);

D3D12_RAYTRACING_AABB Make_D3D12_RAYTRACING_AABB(FLOAT minX, FLOAT minY, FLOAT minZ, float maxX, float maxY, float maxZ);

// Create a BLAS for a mesh from extracted vertex and index data in the most boneheaded way possible.
CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device* device, const void* vertices, int vertexCount, DXGI_FORMAT vertexFormat, const void* indices, int indexCount, DXGI_FORMAT indexFormat);

// Create a TLAS from an instance list in the most boneheaded way possible.
CComPtr<ID3D12Resource1> DXRCreateTLAS(Direct3D12Device* device, const D3D12_RAYTRACING_INSTANCE_DESC* instances, int instanceCount);