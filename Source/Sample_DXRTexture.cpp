///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 Whitted Raytracing
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render in classic Whitted style akin to the
// earliest 16-bit raytracers. This normally involves only 2/3 rays per pixel
// and can be done reasonably quickly.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "SampleResources.h"
#include "generated.Sample_DXRTexture.dxr.h"
#include <array>
#include <atlbase.h>

D3D12_CPU_DESCRIPTOR_HANDLE operator+(D3D12_CPU_DESCRIPTOR_HANDLE h,
                                      int offset) {
  h.ptr += offset;
  return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE operator+(D3D12_GPU_DESCRIPTOR_HANDLE h,
                                      int offset) {
  h.ptr += offset;
  return h;
}

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRTexture(std::shared_ptr<Direct3D12Device> device) {
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
  CComPtr<ID3D12RootSignature> rootSignatureGLOBAL =
      DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
  CComPtr<ID3D12RootSignature> rootSignatureLOCAL =
      DXR_Create_Signature_LOCAL_1SRV(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // PIPELINE - Build the pipeline with all ray shaders.
  CComPtr<ID3D12StateObject> pipelineStateObject;
  {
    uint32_t setupSubobject = 0;

    std::array<D3D12_STATE_SUBOBJECT, 16> descSubobject = {};

    D3D12_DXIL_LIBRARY_DESC descLibrary = {};
    descLibrary.DXILLibrary.pShaderBytecode = g_dxr_shader;
    descLibrary.DXILLibrary.BytecodeLength = sizeof(g_dxr_shader);
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    descSubobject[setupSubobject].pDesc = &descLibrary;
    ++setupSubobject;

    D3D12_RAYTRACING_SHADER_CONFIG descShaderConfig = {};
    descShaderConfig.MaxPayloadSizeInBytes =
        sizeof(float[3]); // Size of RayPayload
    descShaderConfig.MaxAttributeSizeInBytes =
        D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    descSubobject[setupSubobject].pDesc = &descShaderConfig;
    ++setupSubobject;

    const WCHAR *shaderExports[] = {
        L"RayGenerationMVPClip", L"Miss",           L"HitGroupCheckerboard",
        L"HitGroupPlastic",      L"IntersectPlane", L"IntersectSphere"};
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descSubobjectExports = {};
    descSubobjectExports.NumExports = _countof(shaderExports);
    descSubobjectExports.pExports = shaderExports;
    descSubobjectExports.pSubobjectToAssociate =
        &descSubobject[setupSubobject - 1];
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    descSubobject[setupSubobject].pDesc = &descSubobjectExports;
    ++setupSubobject;

    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    descSubobject[setupSubobject].pDesc = &rootSignatureGLOBAL.p;
    ++setupSubobject;

    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    descSubobject[setupSubobject].pDesc = &rootSignatureLOCAL.p;
    ++setupSubobject;

    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descShaderRootSignature = {};
    descShaderRootSignature.NumExports = _countof(shaderExports);
    descShaderRootSignature.pExports = shaderExports;
    descShaderRootSignature.pSubobjectToAssociate =
        &descSubobject[setupSubobject - 1];
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    descSubobject[setupSubobject].pDesc = &descShaderRootSignature;
    ++setupSubobject;

    D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig = {};
    descPipelineConfig.MaxTraceRecursionDepth = 4;
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    descSubobject[setupSubobject].pDesc = &descPipelineConfig;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroupCheckerboard = {};
    descHitGroupCheckerboard.HitGroupExport = L"HitGroupCheckerboard";
    descHitGroupCheckerboard.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    descHitGroupCheckerboard.ClosestHitShaderImport = L"MaterialCheckerboard";
    descHitGroupCheckerboard.IntersectionShaderImport = L"IntersectPlane";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroupCheckerboard;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroupRedPlastic = {};
    descHitGroupRedPlastic.HitGroupExport = L"HitGroupPlastic";
    descHitGroupRedPlastic.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    descHitGroupRedPlastic.ClosestHitShaderImport = L"MaterialPlastic";
    descHitGroupRedPlastic.IntersectionShaderImport = L"IntersectSphere";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroupRedPlastic;
    ++setupSubobject;

    D3D12_STATE_OBJECT_DESC descStateObject = {};
    descStateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    descStateObject.NumSubobjects = setupSubobject;
    descStateObject.pSubobjects = &descSubobject[0];
    TRYD3D(device->m_pDevice->CreateStateObject(&descStateObject,
                                                __uuidof(ID3D12StateObject),
                                                (void **)&pipelineStateObject));
    pipelineStateObject->SetName(L"DXR Pipeline State");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // SHADER TABLE - Create a table of all shaders for the raytracer.
  //
  // Our shader entry is a shader function entrypoint + 4x32bit values in
  // the signature.
  const uint32_t shaderEntrySize =
      AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES +
                  sizeof(D3D12_GPU_DESCRIPTOR_HANDLE),
              D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
  // Ray Generation shader comes first, aligned as necessary.
  const uint32_t descriptorOffsetRayGenerationShader = 0;
  // Miss shader comes next.
  const uint32_t descriptorOffsetMissShader =
      AlignUp(descriptorOffsetRayGenerationShader + shaderEntrySize,
              D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  // Then all the HitGroup shaders.
  const uint32_t descriptorOffsetHitGroup =
      AlignUp(descriptorOffsetMissShader + shaderEntrySize,
              D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
  // The total size of the table we expect.
  const uint32_t shaderTableSize =
      descriptorOffsetHitGroup + shaderEntrySize * 2;
  ////////////////////////////////////////////////////////////////////////////////
  // Now build this table.
  const uint32_t descriptorElementSize =
      device->m_pDevice->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const uint32_t descriptorOffsetGlobals = 0;
  const uint32_t descriptorOffsetLocals =
      descriptorOffsetGlobals + descriptorElementSize * 3;
  CComPtr<ID3D12Resource1> resourceShaderTable;
  {
    CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    TRYD3D(pipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(
        &stateObjectProperties));
    std::unique_ptr<uint8_t[]> shaderTableCPU(new uint8_t[shaderTableSize]);
    memset(&shaderTableCPU[0], 0, shaderTableSize);
    // Shader Index 0 - Ray Generation Shader
    memcpy(&shaderTableCPU[descriptorOffsetRayGenerationShader],
           stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(&shaderTableCPU[descriptorOffsetMissShader],
           stateObjectProperties->GetShaderIdentifier(L"Miss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Hit Shader 1
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0],
           stateObjectProperties->GetShaderIdentifier(L"HitGroupCheckerboard"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0 +
                           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES],
           &(descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart() +
             descriptorOffsetLocals),
           sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    // Shader Index 3 - Hit Shader 2
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1],
           stateObjectProperties->GetShaderIdentifier(L"HitGroupPlastic"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    resourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize,
        &shaderTableCPU[0]);
    resourceShaderTable->SetName(L"DXR Shader Table");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the test texture.
  CComPtr<ID3D12Resource> resourceTexture =
      D3D12_Create_Sample_Texture(device.get());
  ////////////////////////////////////////////////////////////////////////////////
  // Create AABBs.
  CComPtr<ID3D12Resource> resourceAABB = D3D12_Create_Buffer(
      device.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
      sizeof(D3D12_RAYTRACING_AABB), sizeof(D3D12_RAYTRACING_AABB),
      &Make_D3D12_RAYTRACING_AABB(-1, -1, -1, 1, 1, 1));
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structure.
  CComPtr<ID3D12Resource1> resourceBLAS;
  {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
    descRaytracingPrebuild = {};
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
    device->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(
        &descRaytracingInputs, &descRaytracingPrebuild);
    // Create the output and scratch buffers.
    resourceBLAS = D3D12_Create_Buffer(
        device->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        descRaytracingPrebuild.ResultDataMaxSizeInBytes);
    CComPtr<ID3D12Resource1> ResourceASScratch;
    ResourceASScratch = D3D12_Create_Buffer(
        device->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        descRaytracingPrebuild.ResultDataMaxSizeInBytes);
    // Build the acceleration structure.
    D3D12_Run_Synchronously(
        device.get(), [&](ID3D12GraphicsCommandList5 *UploadBLASCommandList) {
          D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
          descBuild.DestAccelerationStructureData =
              resourceBLAS->GetGPUVirtualAddress();
          descBuild.Inputs = descRaytracingInputs;
          descBuild.ScratchAccelerationStructureData =
              ResourceASScratch->GetGPUVirtualAddress();
          UploadBLASCommandList->BuildRaytracingAccelerationStructure(
              &descBuild, 0, nullptr);
        });
  }
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    std::array<D3D12_RAYTRACING_INSTANCE_DESC, 5> DxrInstance = {};
    DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixScale(Vector3{10, 1, 10}), 0,
        resourceBLAS->GetGPUVirtualAddress());
    DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixTranslate(Vector3{0, 1, 0}), 1,
        resourceBLAS->GetGPUVirtualAddress());
    resourceTLAS =
        DXRCreateTLAS(device.get(), &DxrInstance[0], DxrInstance.size());
  }
  return [=](const SampleResourcesD3D12UAV &sampleResources) {
    D3D12_RESOURCE_DESC descTarget =
        sampleResources.BackBufferResource->GetDesc();
    ////////////////////////////////////////////////////////////////////////////////
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
    //
    // This line here LOOKS innocuous and pointless but IT IS IMPORTANT.
    //
    // Anything the lambda doesn't capture could be freed when we leave the
    // constructor. The TLAS sees the BLAS only by address so we need to
    // capture the BLAS table to stop it from deallocating. Do that and your
    // TLAS will have dangling pointers to GPU memory.
    //
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
    auto persistBLAS = resourceBLAS;
    ////////////////////////////////////////////////////////////////////////////////
    // Create a constant buffer view for top level data.
    CComPtr<ID3D12Resource> resourceConstants = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44),
        &Invert(sampleResources.TransformWorldToClip));
    ////////////////////////////////////////////////////////////////////////////////
    // Establish resource views.
    {
      D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase =
          descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
      // Create the UAV for the raytracer output.
      {
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        device->m_pDevice->CreateUnorderedAccessView(
            sampleResources.BackBufferResource, nullptr, &desc,
            descriptorBase + descriptorOffsetGlobals +
                descriptorElementSize * 0);
      }
      // Create the SRV for the acceleration structure.
      {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.ViewDimension =
            D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.RaytracingAccelerationStructure.Location =
            resourceTLAS->GetGPUVirtualAddress();
        device->m_pDevice->CreateShaderResourceView(
            nullptr, &desc,
            descriptorBase + descriptorOffsetGlobals +
                descriptorElementSize * 1);
      }
      // Create the CBV for the scene constants.
      {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = resourceConstants->GetGPUVirtualAddress();
        desc.SizeInBytes = 256;
        device->m_pDevice->CreateConstantBufferView(
            &desc, descriptorBase + descriptorOffsetGlobals +
                       descriptorElementSize * 2);
      }
      // Create the SRV for the texture.
      {
        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = 1;
        device->m_pDevice->CreateShaderResourceView(
            resourceTexture, &desc,
            descriptorBase + descriptorOffsetLocals +
                descriptorElementSize * 0);
      }
    }
    ////////////////////////////////////////////////////////////////////////////////
    // RAYTRACE - Finally call the raytracer and generate the frame.
    D3D12_Run_Synchronously(
        device.get(), [&](ID3D12GraphicsCommandList5 *commandList) {
          // Attach the GLOBAL signature and descriptors to the compute root.
          commandList->SetComputeRootSignature(rootSignatureGLOBAL);
          commandList->SetDescriptorHeaps(1, &descriptorHeapCBVSRVUAV.p);
          commandList->SetComputeRootDescriptorTable(
              0, descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart() +
                     descriptorOffsetGlobals);
          // Prepare the pipeline for raytracing.
          commandList->SetPipelineState1(pipelineStateObject);
          {
            D3D12_DISPATCH_RAYS_DESC desc = {};
            desc.RayGenerationShaderRecord.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                descriptorOffsetRayGenerationShader;
            desc.RayGenerationShaderRecord.SizeInBytes = shaderEntrySize;
            desc.MissShaderTable.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                descriptorOffsetMissShader;
            desc.MissShaderTable.SizeInBytes = shaderEntrySize;
            desc.HitGroupTable.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                descriptorOffsetHitGroup;
            desc.HitGroupTable.SizeInBytes = shaderEntrySize;
            desc.HitGroupTable.StrideInBytes = shaderEntrySize;
            desc.Width = descTarget.Width;
            desc.Height = descTarget.Height;
            desc.Depth = 1;
            commandList->DispatchRays(&desc);
          }
        });
  };
}