///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 DXR Mesh
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a Direct3D 12 scene from an abstract
// scene description using DXR raytracing.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "SampleResources.h"
#include "Scene_IMesh.h"
#include "generated.Sample_DXRMesh.dxr.h"
#include <array>
#include <atlbase.h>
#include <memory>

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRMesh(std::shared_ptr<Direct3D12Device> device) {
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
  CComPtr<ID3D12RootSignature> rootSignatureGLOBAL =
      DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // PIPELINE - Build the pipeline with all ray shaders.
  CComPtr<ID3D12StateObject> pipelineStateObject;
  {
    SimpleRaytracerPipelineSetup setup = {};
    setup.GlobalRootSignature = rootSignatureGLOBAL.p;
    setup.pShaderBytecode = g_dxr_shader;
    setup.BytecodeLength = sizeof(g_dxr_shader);
    setup.MaxPayloadSizeInBytes = sizeof(float[3]); // Size of RayPayload
    setup.MaxTraceRecursionDepth = 1;
    setup.HitGroups.push_back({L"HitGroup", D3D12_HIT_GROUP_TYPE_TRIANGLES,
                               nullptr, L"ClosestHit", nullptr});
    TRYD3D(device->m_pDevice->CreateStateObject(
        ConfigureRaytracerPipeline(setup), __uuidof(ID3D12StateObject),
        (void **)&pipelineStateObject));
    pipelineStateObject->SetName(L"DXR Pipeline State");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // SHADER TABLE - Create a table of all shaders for the raytracer.
  //
  // Note that we don't have any local parameters to our shaders here so
  // there are no local descriptors following any shader identifier entry.
  //
  // Our shader entry is a shader function entrypoint only; no other
  // descriptors or data.
  RaytracingSBTHelper sbtHelper(3, 0);
  CComPtr<ID3D12Resource1> resourceShaderTable;
  {
    CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    TRYD3D(pipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(
        &stateObjectProperties));
    std::unique_ptr<uint8_t[]> sbtData(
        new uint8_t[sbtHelper.GetShaderTableSize()]);
    memset(sbtData.get(), 0, sbtHelper.GetShaderTableSize());
    // Shader Index 0 - Ray Generation Shader
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(0),
           stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(1),
           stateObjectProperties->GetShaderIdentifier(L"Miss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Hit Shader 1
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(2),
           stateObjectProperties->GetShaderIdentifier(L"HitGroup"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    resourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, sbtHelper.GetShaderTableSize(),
        sbtHelper.GetShaderTableSize(), sbtData.get());
    resourceShaderTable->SetName(L"DXR Shader Table");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structures.
  CComPtr<ID3D12Resource1> resourceBLAS;
  {
    Vector3 vertices[] = {{-10, 0, 10}, {10, 0, 10},   {10, 0, -10},
                          {10, 0, -10}, {-10, 0, -10}, {-10, 0, 10}};
    resourceBLAS = DXRCreateBLAS(device.get(), vertices, _countof(vertices),
                                 DXGI_FORMAT_R32G32B32_FLOAT, nullptr, 0,
                                 DXGI_FORMAT_UNKNOWN);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc =
        Make_D3D12_RAYTRACING_INSTANCE_DESC(
            CreateMatrixTranslate(Vector3{0, 0, 0}), 0, resourceBLAS);
    resourceTLAS = DXRCreateTLAS(device.get(), &instanceDesc, 1);
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
    // Build the descriptor table to establish resource views.
    //
    // These descriptors will be attached via the GLOBAL signature as they are
    // all shared throughout all shaders. The GLOBAL signature is established
    // by the compute root signature and descriptor heap.
    {
      D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase =
          descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
      UINT descriptorElementSize =
          device->m_pDevice->GetDescriptorHandleIncrementSize(
              D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
      // Create the UAV for the raytracer output.
      device->m_pDevice->CreateUnorderedAccessView(
          sampleResources.BackBufferResource, nullptr,
          &Make_D3D12_UNORDERED_ACCESS_VIEW_DESC_For_Texture2D(),
          descriptorBase);
      descriptorBase.ptr += descriptorElementSize;
      // Create the SRV for the acceleration structure.
      device->m_pDevice->CreateShaderResourceView(
          nullptr, &Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(resourceTLAS),
          descriptorBase);
      descriptorBase.ptr += descriptorElementSize;
      // Create the CBV for the scene constants.
      device->m_pDevice->CreateConstantBufferView(
          &Make_D3D12_CONSTANT_BUFFER_VIEW_DESC(resourceConstants, 256),
          descriptorBase);
      descriptorBase.ptr += descriptorElementSize;
    }
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
                sbtHelper.GetShaderIdentifierOffset(0);
            desc.RayGenerationShaderRecord.SizeInBytes =
                sbtHelper.GetShaderEntrySize();
            desc.MissShaderTable.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(1);
            desc.MissShaderTable.SizeInBytes = sbtHelper.GetShaderEntrySize();
            desc.HitGroupTable.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(2);
            desc.HitGroupTable.SizeInBytes = sbtHelper.GetShaderEntrySize();
            desc.HitGroupTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.Width = descTarget.Width;
            desc.Height = descTarget.Height;
            desc.Depth = 1;
            commandList->DispatchRays(&desc);
          }
        });
  };
}