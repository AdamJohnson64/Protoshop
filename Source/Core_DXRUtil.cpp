#include "Core_DXRUtil.h"
#include "Core_D3D.h"
#include "Core_D3D12Util.h"
#include "Core_DXGIUtil.h"
#include "Core_Util.h"
#include <array>

CComPtr<ID3D12RootSignature>
DXR_Create_Signature(ID3D12Device *device,
                     const D3D12_ROOT_SIGNATURE_DESC &desc) {
  CComPtr<ID3DBlob> m_blob;
  CComPtr<ID3DBlob> m_blobError;
  TRYD3D(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
                                     &m_blob, &m_blobError));
  CComPtr<ID3D12RootSignature> pRootSignature;
  TRYD3D(device->CreateRootSignature(
      0, m_blob->GetBufferPointer(), m_blob->GetBufferSize(),
      __uuidof(ID3D12RootSignature), (void **)&pRootSignature.p));
  pRootSignature->SetName(L"DXR Root Signature");
  return pRootSignature;
}

std::vector<D3D12_DESCRIPTOR_RANGE> DXR_Create_Signature_Ranges(
    const std::vector<SignatureRegisterBinding> &signature) {
  std::vector<D3D12_DESCRIPTOR_RANGE> descDescriptorRange;
  for (const auto &entry : signature) {
    D3D12_DESCRIPTOR_RANGE newRange = {};
    newRange.BaseShaderRegister = entry.RegisterIndex;
    newRange.NumDescriptors = 1;
    newRange.RegisterSpace = 0;
    switch (entry.ResourceType) {
    case ShaderResourceView:
      newRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      break;
    case UnorderedAccessView:
      newRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
      break;
    case ConstantBufferView:
      newRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      break;
    }
    newRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    descDescriptorRange.push_back(newRange);
  }
  return descDescriptorRange;
}

CComPtr<ID3D12RootSignature> DXR_Create_Simple_Signature_GLOBAL(
    ID3D12Device *device,
    const std::vector<SignatureRegisterBinding> &signature) {
  std::vector<D3D12_DESCRIPTOR_RANGE> descDescriptorRange =
      DXR_Create_Signature_Ranges(signature);
  D3D12_ROOT_PARAMETER descRootParameter = {};
  descRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  descRootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  descRootParameter.DescriptorTable.NumDescriptorRanges =
      descDescriptorRange.size();
  descRootParameter.DescriptorTable.pDescriptorRanges = &descDescriptorRange[0];
  D3D12_ROOT_SIGNATURE_DESC descSignature = {};
  descSignature.NumParameters = 1;
  descSignature.pParameters = &descRootParameter;
  // descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
  return DXR_Create_Signature(device, descSignature);
}

CComPtr<ID3D12RootSignature> DXR_Create_Simple_Signature_LOCAL(
    ID3D12Device *device,
    const std::vector<SignatureRegisterBinding> &signature) {
  std::vector<D3D12_DESCRIPTOR_RANGE> descDescriptorRange =
      DXR_Create_Signature_Ranges(signature);
  std::vector<D3D12_ROOT_PARAMETER> descRootParameter;
  for (const auto &entry : descDescriptorRange) {
    D3D12_ROOT_PARAMETER newParameter = {};
    newParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    newParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    newParameter.DescriptorTable.NumDescriptorRanges = 1;
    newParameter.DescriptorTable.pDescriptorRanges = &entry;
    descRootParameter.push_back(newParameter);
  }
  D3D12_ROOT_SIGNATURE_DESC descSignature = {};
  descSignature.NumParameters = descRootParameter.size();
  descSignature.pParameters = &descRootParameter[0];
  descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
  return DXR_Create_Signature(device, descSignature);
}

CComPtr<ID3D12RootSignature>
DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(ID3D12Device *device) {
  return DXR_Create_Simple_Signature_GLOBAL(device, {{0, UnorderedAccessView},
                                                     {0, ShaderResourceView},
                                                     {0, ConstantBufferView}});
}

CComPtr<ID3D12RootSignature>
DXR_Create_Signature_LOCAL_4x32(ID3D12Device *device) {
  D3D12_ROOT_PARAMETER descRootParameter = {};
  descRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  descRootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
  descRootParameter.Constants.ShaderRegister = 1;
  descRootParameter.Constants.Num32BitValues = 4;
  D3D12_ROOT_SIGNATURE_DESC descSignature = {};
  descSignature.NumParameters = 1;
  descSignature.pParameters = &descRootParameter;
  descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
  return DXR_Create_Signature(device, descSignature);
}

D3D12_RAYTRACING_INSTANCE_DESC
Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transformObjectToWorld,
                                    int hitgroup,
                                    D3D12_GPU_VIRTUAL_ADDRESS blas) {
  D3D12_RAYTRACING_INSTANCE_DESC o = {};
  o.Transform[0][0] = transformObjectToWorld.M11;
  o.Transform[1][0] = transformObjectToWorld.M12;
  o.Transform[2][0] = transformObjectToWorld.M13;
  o.Transform[0][1] = transformObjectToWorld.M21;
  o.Transform[1][1] = transformObjectToWorld.M22;
  o.Transform[2][1] = transformObjectToWorld.M23;
  o.Transform[0][2] = transformObjectToWorld.M31;
  o.Transform[1][2] = transformObjectToWorld.M32;
  o.Transform[2][2] = transformObjectToWorld.M33;
  o.Transform[0][3] = transformObjectToWorld.M41;
  o.Transform[1][3] = transformObjectToWorld.M42;
  o.Transform[2][3] = transformObjectToWorld.M43;
  o.InstanceMask = 0xFF;
  o.InstanceContributionToHitGroupIndex = hitgroup;
  o.AccelerationStructure = blas;
  return o;
}

D3D12_RAYTRACING_INSTANCE_DESC
Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup,
                                    ID3D12Resource *blasResource) {
  return Make_D3D12_RAYTRACING_INSTANCE_DESC(
      transform, hitgroup, blasResource->GetGPUVirtualAddress());
}

D3D12_RAYTRACING_AABB Make_D3D12_RAYTRACING_AABB(FLOAT minX, FLOAT minY,
                                                 FLOAT minZ, float maxX,
                                                 float maxY, float maxZ) {
  D3D12_RAYTRACING_AABB o = {};
  o.MinX = minX;
  o.MinY = minY;
  o.MinZ = minZ;
  o.MaxX = maxX;
  o.MaxY = maxY;
  o.MaxZ = maxZ;
  return o;
}

D3D12_SHADER_RESOURCE_VIEW_DESC
Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(
    D3D12_GPU_VIRTUAL_ADDRESS Location) {
  D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  desc.RaytracingAccelerationStructure.Location = Location;
  return desc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC
Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(ID3D12Resource *bufferResource) {
  return Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(
      bufferResource->GetGPUVirtualAddress());
}

CComPtr<ID3D12Resource1> DXRCreateAccelerationStructure(
    Direct3D12Device *device,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS &inputs) {
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
  descRaytracingPrebuild = {};
  device->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(
      &inputs, &descRaytracingPrebuild);
  // Create the output and scratch buffers.
  CComPtr<ID3D12Resource1> ResourceBLAS = D3D12_Create_Buffer(
      device->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
      D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
      descRaytracingPrebuild.ResultDataMaxSizeInBytes);
  CComPtr<ID3D12Resource1> ResourceASScratch = D3D12_Create_Buffer(
      device->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
      D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
      descRaytracingPrebuild.ResultDataMaxSizeInBytes);
  // Build the acceleration structure.
  D3D12_Run_Synchronously(
      device, [&](ID3D12GraphicsCommandList5 *UploadBLASCommandList) {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
        descBuild.DestAccelerationStructureData =
            ResourceBLAS->GetGPUVirtualAddress();
        descBuild.Inputs = inputs;
        descBuild.ScratchAccelerationStructureData =
            ResourceASScratch->GetGPUVirtualAddress();
        UploadBLASCommandList->BuildRaytracingAccelerationStructure(&descBuild,
                                                                    0, nullptr);
      });
  return ResourceBLAS;
}

CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device *device,
                                       const void *vertices, int vertexCount,
                                       DXGI_FORMAT vertexFormat,
                                       const void *indices, int indexCount,
                                       DXGI_FORMAT indexFormat) {
  // The GPU is doing the actual acceleration structure building work so we
  // need all the mesh data up on GPU first.
  CComPtr<ID3D12Resource1> ResourceVertices = D3D12_Create_Buffer(
      device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
      DXGI_FORMAT_Size(vertexFormat) * vertexCount,
      DXGI_FORMAT_Size(vertexFormat) * vertexCount, vertices);
  CComPtr<ID3D12Resource1> ResourceIndices;
  if (indices != nullptr) {
    ResourceIndices = D3D12_Create_Buffer(
        device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
        DXGI_FORMAT_Size(indexFormat) * indexCount,
        DXGI_FORMAT_Size(indexFormat) * indexCount, indices);
  }
  // Now create and initialize the BLAS with this data.
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs =
      {};
  descRaytracingInputs.Type =
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  descRaytracingInputs.NumDescs = 1;
  descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  D3D12_RAYTRACING_GEOMETRY_DESC descGeometry = {};
  descGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
  descGeometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
  descGeometry.Triangles.IndexFormat = indexFormat;
  descGeometry.Triangles.VertexFormat = vertexFormat;
  descGeometry.Triangles.IndexCount = indexCount;
  descGeometry.Triangles.VertexCount = vertexCount;
  descGeometry.Triangles.IndexBuffer =
      ResourceIndices != nullptr ? ResourceIndices->GetGPUVirtualAddress() : 0;
  descGeometry.Triangles.VertexBuffer.StartAddress =
      ResourceVertices->GetGPUVirtualAddress();
  descGeometry.Triangles.VertexBuffer.StrideInBytes =
      DXGI_FORMAT_Size(vertexFormat);
  descRaytracingInputs.pGeometryDescs = &descGeometry;
  return DXRCreateAccelerationStructure(device, descRaytracingInputs);
}

CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device *device,
                                       const D3D12_RAYTRACING_AABB &aabb) {
  CComPtr<ID3D12Resource> resourceAABB = D3D12_Create_Buffer(
      device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
      sizeof(D3D12_RAYTRACING_AABB), sizeof(D3D12_RAYTRACING_AABB), &aabb);
  // Now create and initialize the BLAS with this data.
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
  descRaytracingInputs = {};
  descRaytracingInputs.Type =
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  descRaytracingInputs.NumDescs = 1;
  descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  D3D12_RAYTRACING_GEOMETRY_DESC descGeometry[1] = {};
  descGeometry[0].Type =
      D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
  descGeometry[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
  descGeometry[0].AABBs.AABBCount = 1;
  descGeometry[0].AABBs.AABBs.StartAddress =
      resourceAABB->GetGPUVirtualAddress();
  descGeometry[0].AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
  descRaytracingInputs.pGeometryDescs = &descGeometry[0];
  return DXRCreateAccelerationStructure(device, descRaytracingInputs);
}

CComPtr<ID3D12Resource1>
DXRCreateTLAS(Direct3D12Device *device,
              const D3D12_RAYTRACING_INSTANCE_DESC *instances,
              int instanceCount) {
  ////////////////////////////////////////////////////////////////////////////////
  // INSTANCE - Create the instancing table.
  ////////////////////////////////////////////////////////////////////////////////
  CComPtr<ID3D12Resource1> ResourceInstance = D3D12_Create_Buffer(
      device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
      sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount,
      sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, instances);
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  ////////////////////////////////////////////////////////////////////////////////
  CComPtr<ID3D12Resource1> ResourceTLAS;
  {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
    descRaytracingInputs = {};
    descRaytracingInputs.Type =
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    descRaytracingInputs.NumDescs = instanceCount;
    descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    descRaytracingInputs.InstanceDescs =
        ResourceInstance->GetGPUVirtualAddress();
    ResourceTLAS = DXRCreateAccelerationStructure(device, descRaytracingInputs);
  }
  return ResourceTLAS;
}

const D3D12_STATE_OBJECT_DESC *
ConfigureRaytracerPipeline(SimpleRaytracerPipelineSetup &setup) {
  setup.descSubobjects.clear();

  setup.descSubobjects.push_back(
      {D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,
       &setup.GlobalRootSignature});
  setup.descSubobjects.push_back(
      {D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,
       &setup.LocalRootSignature});

  setup.descLibrary = {};
  setup.descLibrary.DXILLibrary.pShaderBytecode = setup.pShaderBytecode;
  setup.descLibrary.DXILLibrary.BytecodeLength = setup.BytecodeLength;
  setup.descSubobjects.push_back(
      {D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &setup.descLibrary});

  setup.descShaderConfig = {};
  setup.descShaderConfig.MaxPayloadSizeInBytes = setup.MaxPayloadSizeInBytes;
  setup.descShaderConfig.MaxAttributeSizeInBytes =
      D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
  setup.descSubobjects.push_back(
      {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,
       &setup.descShaderConfig});

  setup.descPipelineConfig = {};
  setup.descPipelineConfig.MaxTraceRecursionDepth =
      setup.MaxTraceRecursionDepth;
  setup.descSubobjects.push_back(
      {D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,
       &setup.descPipelineConfig});

  for (const auto &h : setup.HitGroups) {
    setup.descSubobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &h});
  }

  setup.descStateObject = {};
  setup.descStateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
  setup.descStateObject.NumSubobjects = setup.descSubobjects.size();
  setup.descStateObject.pSubobjects = &setup.descSubobjects[0];
  return &setup.descStateObject;
}

RaytracingSBTHelper::RaytracingSBTHelper(size_t countShaders,
                                         size_t countCBVSRVUAV) {
  m_shaderEntrySize =
      AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES +
                  sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * countCBVSRVUAV,
              D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  m_shaderTableSize = m_shaderEntrySize * countShaders;
}

size_t RaytracingSBTHelper::GetShaderEntrySize() const {
  return m_shaderEntrySize;
}

size_t RaytracingSBTHelper::GetShaderTableSize() const {
  return m_shaderTableSize;
}

size_t
RaytracingSBTHelper::GetShaderIdentifierOffset(uint32_t shaderIndex) const {
  return m_shaderEntrySize * shaderIndex;
}

size_t RaytracingSBTHelper::GetShaderRootArgumentBaseOffset(
    uint32_t shaderIndex) const {
  return GetShaderIdentifierOffset(shaderIndex) +
         D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
}

size_t
RaytracingSBTHelper::GetShaderRootArgumentOffset(uint32_t shaderIndex,
                                                 uint32_t argumentIndex) const {
  return GetShaderRootArgumentBaseOffset(shaderIndex) +
         sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * argumentIndex;
}