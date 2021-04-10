#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "SampleResources.h"
#include "generated.Sample_DXRAmbientOcclusion.dxr.h"
#include <array>
#include <atlbase.h>

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRAmbientOcclusion(std::shared_ptr<Direct3D12Device> device) {
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
    setup.MaxPayloadSizeInBytes = sizeof(float[3]);
    setup.MaxTraceRecursionDepth = 2;
    setup.HitGroups.push_back({L"HitGroupPrimaryAmbientOcclusionPlane",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"PrimaryMaterialAmbientOcclusion",
                               L"IntersectPlane"});
    setup.HitGroups.push_back({L"HitGroupShadowAmbientOcclusionPlane",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"ShadowMaterialAmbientOcclusion",
                               L"IntersectPlane"});
    setup.HitGroups.push_back({L"HitGroupPrimaryAmbientOcclusionSphere",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"PrimaryMaterialAmbientOcclusion",
                               L"IntersectSphere"});
    setup.HitGroups.push_back({L"HitGroupShadowAmbientOcclusionSphere",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"ShadowMaterialAmbientOcclusion",
                               L"IntersectSphere"});
    TRYD3D(device->m_pDevice->CreateStateObject(
        ConfigureRaytracerPipeline(setup), __uuidof(ID3D12StateObject),
        (void **)&pipelineStateObject));
    pipelineStateObject->SetName(L"DXR Pipeline State");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // SHADER TABLE - Create a table of all shaders for the raytracer.
  //
  // Our shader entry is a shader function entrypoint only; no other
  // descriptors or data.
  RaytracingSBTHelper sbtHelper(256, 0);
  // Now build this table.
  CComPtr<ID3D12Resource1> resourceShaderTable;
  {
    CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    TRYD3D(pipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(
        &stateObjectProperties));
    std::unique_ptr<uint8_t[]> shaderTableCPU(
        new uint8_t[sbtHelper.GetShaderTableSize()]);
    memset(&shaderTableCPU[0], 0, sbtHelper.GetShaderTableSize());
    // Shader Index 0 - Ray Generation Shader
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(0)],
           stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(1)],
           stateObjectProperties->GetShaderIdentifier(L"PrimaryMiss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(2)],
           stateObjectProperties->GetShaderIdentifier(L"ShadowMiss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Hit Shader 1
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(3)],
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryAmbientOcclusionPlane"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 3 - Hit Shader 1
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(4)],
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowAmbientOcclusionPlane"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 4 - Hit Shader 2
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(5)],
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryAmbientOcclusionSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 5 - Hit Shader 2
    memcpy(&shaderTableCPU[sbtHelper.GetShaderIdentifierOffset(6)],
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowAmbientOcclusionSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    resourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, sbtHelper.GetShaderTableSize(),
        sbtHelper.GetShaderTableSize(), &shaderTableCPU[0]);
    resourceShaderTable->SetName(L"DXR Shader Table");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structure.
  CComPtr<ID3D12Resource1> resourceBLAS = DXRCreateBLAS(
      device.get(), Make_D3D12_RAYTRACING_AABB(-1, -1, -1, 1, 1, 1));
  ////////////////////////////////////////////////////////////////////////////////
  // INSTANCE - Create the instancing table.
  CComPtr<ID3D12Resource1> resourceInstance;
  {
    std::array<D3D12_RAYTRACING_INSTANCE_DESC, 4> DxrInstance = {};
    DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixScale(Vector3{10, 1, 10}), 0,
        resourceBLAS->GetGPUVirtualAddress());
    DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixTranslate(Vector3{-2, 1, 0}), 2,
        resourceBLAS->GetGPUVirtualAddress());
    DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixTranslate(Vector3{0, 1, 0}), 2,
        resourceBLAS->GetGPUVirtualAddress());
    DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixTranslate(Vector3{2, 1, 0}), 2,
        resourceBLAS->GetGPUVirtualAddress());
    resourceInstance = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
        sizeof(DxrInstance), sizeof(DxrInstance), &DxrInstance);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
    descRaytracingInputs = {};
    descRaytracingInputs.Type =
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    descRaytracingInputs.NumDescs = 4;
    descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    descRaytracingInputs.InstanceDescs =
        resourceInstance->GetGPUVirtualAddress();
    resourceTLAS =
        DXRCreateAccelerationStructure(device.get(), descRaytracingInputs);
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
    CComPtr<ID3D12Resource> resourceConstants;
    resourceConstants = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44),
        &Invert(sampleResources.TransformWorldToClip));
    ////////////////////////////////////////////////////////////////////////////////
    // Establish resource views.
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
        device.get(), [&](ID3D12GraphicsCommandList4 *RaytraceCommandList) {
          // Attach the GLOBAL signature and descriptors to the compute root.
          RaytraceCommandList->SetComputeRootSignature(rootSignatureGLOBAL);
          RaytraceCommandList->SetDescriptorHeaps(1,
                                                  &descriptorHeapCBVSRVUAV.p);
          RaytraceCommandList->SetComputeRootDescriptorTable(
              0, descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());
          // Prepare the pipeline for raytracing.
          RaytraceCommandList->SetPipelineState1(pipelineStateObject);
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
            desc.MissShaderTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.HitGroupTable.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(3);
            desc.HitGroupTable.SizeInBytes = sbtHelper.GetShaderEntrySize();
            desc.HitGroupTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.Width = descTarget.Width;
            desc.Height = descTarget.Height;
            desc.Depth = 1;
            RaytraceCommandList->DispatchRays(&desc);
          }
        });
  };
}