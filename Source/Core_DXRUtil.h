#pragma once

#include "Core_Math.h"
#include <atlbase.h>
#include <d3d12.h>
#include <memory>
#include <vector>

class Direct3D12Device;

// Create a DXR GLOBAL root signature that contains the required output UAV,
// an SRV for the acceleration structure, and a CBV for global constants.
// This will be attached via the compute root signature and descriptor heap.
CComPtr<ID3D12RootSignature>
DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(ID3D12Device *device);

// Create a DXR LOCAL root signature that contains data to be applied per
// material shader. This will be attached via the shader table.
CComPtr<ID3D12RootSignature>
DXR_Create_Signature_LOCAL_4x32(ID3D12Device *device);

// Create a DXR LOCAL root signature that contains only a texture reference.
CComPtr<ID3D12RootSignature>
DXR_Create_Signature_LOCAL_1SRV(ID3D12Device *device);

// Create a DXR LOCAL root signature that contains two texture reference.
CComPtr<ID3D12RootSignature>
DXR_Create_Signature_LOCAL_2SRV(ID3D12Device *device);

D3D12_RAYTRACING_INSTANCE_DESC
Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup,
                                    D3D12_GPU_VIRTUAL_ADDRESS blas);

D3D12_RAYTRACING_INSTANCE_DESC
Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup,
                                    ID3D12Resource *blasResource);

D3D12_RAYTRACING_AABB Make_D3D12_RAYTRACING_AABB(FLOAT minX, FLOAT minY,
                                                 FLOAT minZ, float maxX,
                                                 float maxY, float maxZ);

D3D12_SHADER_RESOURCE_VIEW_DESC
Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(
    D3D12_GPU_VIRTUAL_ADDRESS Location);

D3D12_SHADER_RESOURCE_VIEW_DESC
Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(ID3D12Resource *bufferResource);

// Generic acceleration struction creation.
// This handles the pre-sizing and allocation of GPU structures along with
// the actual GPU command lists to invoke structure generation.
CComPtr<ID3D12Resource1> DXRCreateAccelerationStructure(
    Direct3D12Device *device,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &inputs);

// Create a BLAS for a mesh from extracted vertex and index data in the most
// boneheaded way possible.
CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device *device,
                                       const void *vertices, int vertexCount,
                                       DXGI_FORMAT vertexFormat,
                                       const void *indices, int indexCount,
                                       DXGI_FORMAT indexFormat);

// Create a BLAS for a single axis-aligned bounding-box (AABB) in the most
// boneheaded way possible.
CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device *device,
                                       const D3D12_RAYTRACING_AABB &aabb);

// Create a TLAS from an instance list in the most boneheaded way possible.
CComPtr<ID3D12Resource1>
DXRCreateTLAS(Direct3D12Device *device,
              const D3D12_RAYTRACING_INSTANCE_DESC *instances,
              int instanceCount);

class SimpleRaytracerPipelineSetup {
public:
  ////////////////////////////////////////////////////////////////////////////////
  // Be sure to configure these user-defined settings.
  ID3D12RootSignature *GlobalRootSignature;
  ID3D12RootSignature *LocalRootSignature;
  const void *pShaderBytecode;
  SIZE_T BytecodeLength;
  int MaxPayloadSizeInBytes;
  int MaxTraceRecursionDepth;
  std::vector<D3D12_HIT_GROUP_DESC> HitGroups;
  ////////////////////////////////////////////////////////////////////////////////
  // Members below are configured automatically.
  D3D12_DXIL_LIBRARY_DESC descLibrary;
  D3D12_RAYTRACING_SHADER_CONFIG descShaderConfig;
  D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig;
  std::vector<D3D12_STATE_SUBOBJECT> descSubobjects;
  D3D12_STATE_OBJECT_DESC descStateObject;
};

const D3D12_STATE_OBJECT_DESC *
ConfigureRaytracerPipeline(SimpleRaytracerPipelineSetup &setup);

class RaytracingSBTHelper {
public:
  RaytracingSBTHelper(size_t countShaders, size_t countCBVSRVUAV);
  // Get the size of a single SBT entry.
  size_t GetShaderEntrySize() const;
  // Get the total byte size of the shader table.
  size_t GetShaderTableSize() const;
  // Get the offset of an SBT entry.
  // At this location we would normally expect to find the shader identifier as
  // extracted from the pipeline.
  size_t GetShaderIdentifierOffset(uint32_t shaderIndex) const;
  // Get the offset of the first root argument of a shader entry.
  // The bytes following the shader identifier are expected to be pointers into
  // a descriptor heap and define the arguments for the shader entry. Shader
  // arguments could be CBVs, SRVs (textures?) or UAVs.
  size_t GetShaderRootArgumentBaseOffset(uint32_t shaderIndex) const;
  // Get the offset of the n-th root argument of a shader entry.
  // Each entry is sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) bytes forming a GPU
  // pointer into a descriptor heap.
  size_t GetShaderRootArgumentOffset(uint32_t shaderIndex,
                                     uint32_t argumentIndex) const;

private:
  size_t m_shaderEntrySize;
  size_t m_shaderTableSize;
};
