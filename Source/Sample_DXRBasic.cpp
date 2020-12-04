///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 DXR Basic
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a triangle using raytracing and the
// most basic shading (constant color). Be warned that raytracer setup can be a
// bit complicated but we cover all of that here in a simplistic manner.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "generated.Sample_DXRBasic.dxr.h"
#include <array>
#include <atlbase.h>
#include <d3dcompiler.h>
#include <functional>
#include <memory>

std::function<void(ID3D12Resource *)>
CreateSample_DXRBasic(std::shared_ptr<Direct3D12Device> device) {
  CComPtr<ID3D12Resource1> resourceTarget =
      DXR_Create_Output_UAV(device->m_pDevice);
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
  CComPtr<ID3D12RootSignature> rootSignatureGLOBAL =
      DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // PIPELINE - Build the pipeline with all ray shaders.
  CComPtr<ID3D12StateObject> pipelineStateObject;
  {
    uint32_t setupSubobject = 0;

    std::array<D3D12_STATE_SUBOBJECT, 8> descSubobject = {};

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

    const WCHAR *shaderExports[] = {L"RayGeneration", L"Miss", L"HitGroup"};
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

    D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig = {};
    descPipelineConfig.MaxTraceRecursionDepth = 1;
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    descSubobject[setupSubobject].pDesc = &descPipelineConfig;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroup = {};
    descHitGroup.HitGroupExport = L"HitGroup";
    descHitGroup.ClosestHitShaderImport = L"ClosestHit";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroup;
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
  // Our shader entry is a shader function entrypoint only; no other
  // descriptors or data.
  const uint32_t shaderEntrySize =
      AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
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
      descriptorOffsetHitGroup + shaderEntrySize * 1;
  // Now build this table.
  CComPtr<ID3D12Resource1> resourceShaderTable;
  {
    CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    TRYD3D(pipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(
        &stateObjectProperties));
    std::unique_ptr<uint8_t[]> shaderTableCPU(new uint8_t[shaderTableSize]);
    memset(&shaderTableCPU[0], 0, shaderTableSize);
    // Shader Index 0 - Ray Generation Shader
    memcpy(&shaderTableCPU[descriptorOffsetRayGenerationShader],
           stateObjectProperties->GetShaderIdentifier(L"RayGeneration"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(&shaderTableCPU[descriptorOffsetMissShader],
           stateObjectProperties->GetShaderIdentifier(L"Miss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Hit Shader
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0],
           stateObjectProperties->GetShaderIdentifier(L"HitGroup"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    resourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize,
        &shaderTableCPU[0]);
    resourceShaderTable->SetName(L"DXR Shader Table");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structure.
  CComPtr<ID3D12Resource1> resourceBLAS;
  {
    // Create some simple geometry.
    Vector2 vertices[] = {
        {0, 0}, {0, RENDERTARGET_HEIGHT}, {RENDERTARGET_WIDTH, 0}};
    uint32_t indices[] = {0, 1, 2};
    resourceBLAS =
        DXRCreateBLAS(device.get(), vertices, 3, DXGI_FORMAT_R32G32_FLOAT,
                      indices, 3, DXGI_FORMAT_R32_UINT);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    D3D12_RAYTRACING_INSTANCE_DESC DxrInstance =
        Make_D3D12_RAYTRACING_INSTANCE_DESC(
            Identity<float>, 0, resourceBLAS->GetGPUVirtualAddress());
    resourceTLAS = DXRCreateTLAS(device.get(), &DxrInstance, 1);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Establish resource views.
  {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase =
        descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
    UINT descriptorElementSize =
        device->m_pDevice->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // Create the UAV for the raytracer output.
    {
      D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
      desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
      device->m_pDevice->CreateUnorderedAccessView(resourceTarget, nullptr,
                                                   &desc, descriptorBase);
      descriptorBase.ptr += descriptorElementSize;
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
      device->m_pDevice->CreateShaderResourceView(nullptr, &desc,
                                                  descriptorBase);
      descriptorBase.ptr += descriptorElementSize;
    }
  }
  return [=](ID3D12Resource *resourceBackbuffer) {
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
    auto persistTLAS = resourceTLAS;
    ////////////////////////////////////////////////////////////////////////////////
    // RAYTRACE - Finally call the raytracer and generate the frame.
    D3D12_Run_Synchronously(
        device.get(), [&](ID3D12GraphicsCommandList4 *commandList) {
          // Attach the GLOBAL signature and descriptors to the compute root.
          commandList->SetComputeRootSignature(rootSignatureGLOBAL);
          commandList->SetDescriptorHeaps(1, &descriptorHeapCBVSRVUAV.p);
          commandList->SetComputeRootDescriptorTable(
              0, descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());
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
            desc.Width = RENDERTARGET_WIDTH;
            desc.Height = RENDERTARGET_HEIGHT;
            desc.Depth = 1;
            commandList->DispatchRays(&desc);
          }
          commandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(
                     resourceTarget, D3D12_RESOURCE_STATE_COMMON,
                     D3D12_RESOURCE_STATE_COPY_SOURCE));
          commandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resourceBackbuffer,
                                              D3D12_RESOURCE_STATE_COMMON,
                                              D3D12_RESOURCE_STATE_COPY_DEST));
          commandList->CopyResource(resourceBackbuffer, resourceTarget);
          commandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resourceTarget,
                                              D3D12_RESOURCE_STATE_COPY_SOURCE,
                                              D3D12_RESOURCE_STATE_COMMON));
          commandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resourceBackbuffer,
                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                              D3D12_RESOURCE_STATE_COMMON));
        });
  };
}