///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 DXR Path Tracing
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render an approximation of area lights. This
// demonstration does not obey physical and optical priniciples and only
// calculates rough RGB values. In reality you would want to express
// illumination in physical units (Watts) and use compute shaders to reprocess
// lighting with more appropriate color grading.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "SampleResources.h"
#include "generated.Sample_DXRPathTrace.dxr.h"
#include <array>
#include <atlbase.h>

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRPathTrace(std::shared_ptr<Direct3D12Device> device) {
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
  // Our GLOBAL signature contains the output UAV, acceleration SRV, and some
  // constants in a CBV.
  CComPtr<ID3D12RootSignature> rootSignatureGLOBAL =
      DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
  // Out LOCAL signature contains 4xF32s that we can use for anything we like
  // in the materials.
  CComPtr<ID3D12RootSignature> rootSignatureLOCAL =
      DXR_Create_Signature_LOCAL_4x32(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // PIPELINE - Build the pipeline with all ray shaders.
  CComPtr<ID3D12StateObject> pipelineStateObject;
  {
    SimpleRaytracerPipelineSetup setup = {};
    setup.GlobalRootSignature = rootSignatureGLOBAL.p;
    setup.LocalRootSignature = rootSignatureLOCAL.p;
    setup.pShaderBytecode = g_dxr_shader;
    setup.BytecodeLength = sizeof(g_dxr_shader);
    setup.MaxPayloadSizeInBytes = sizeof(float[3]);
    setup.MaxTraceRecursionDepth = 2;
    setup.HitGroups.push_back({L"HitGroupPrimaryDiffusePlane",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"PrimaryMaterialDiffuse",
                               L"IntersectPlane"});
    setup.HitGroups.push_back({L"HitGroupShadowDiffusePlane",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"ShadowMaterialDiffuse",
                               L"IntersectPlane"});
    setup.HitGroups.push_back({L"HitGroupPrimaryDiffuseSphere",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"PrimaryMaterialDiffuse",
                               L"IntersectSphere"});
    setup.HitGroups.push_back({L"HitGroupShadowDiffuseSphere",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"ShadowMaterialDiffuse",
                               L"IntersectSphere"});
    setup.HitGroups.push_back({L"HitGroupPrimaryEmissiveSphere",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"PrimaryMaterialEmissive",
                               L"IntersectSphere"});
    setup.HitGroups.push_back({L"HitGroupShadowEmissiveSphere",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"ShadowMaterialEmissive",
                               L"IntersectSphere"});
    TRYD3D(device->m_pDevice->CreateStateObject(
        ConfigureRaytracerPipeline(setup), __uuidof(ID3D12StateObject),
        (void **)&pipelineStateObject));
    pipelineStateObject->SetName(L"DXR Pipeline State");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // SHADER TABLE - Create a table of all shaders for the raytracer.
  //
  // Our shader entry is a shader function entrypoint + 4x32bit values in
  // the signature.
  RaytracingSBTHelper sbtHelper(64, 2);
  CComPtr<ID3D12Resource1> ResourceShaderTable;
  {
    Vector4 albedoRed = {1, 0, 0, 0};
    Vector4 albedoGreen = {0, 1, 0, 0};
    Vector4 albedoBlue = {0, 0, 1, 0};
    Vector4 albedoWhite = {1, 1, 1, 0};
    CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    TRYD3D(pipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(
        &stateObjectProperties));
    std::unique_ptr<uint8_t[]> sbtData(
        new uint8_t[sbtHelper.GetShaderTableSize()]);
    memset(sbtData.get(), 0, sbtHelper.GetShaderTableSize());

    // Ray Generation and Miss Shaders
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(0),
           stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(1),
           stateObjectProperties->GetShaderIdentifier(L"PrimaryMiss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(2),
           stateObjectProperties->GetShaderIdentifier(L"ShadowMiss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Shader pair.
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(3),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryDiffusePlane"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(3, 0)) =
        albedoWhite;
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(4),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowDiffusePlane"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(4, 0)) =
        albedoWhite;

    // Shader pair.
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(5),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryDiffuseSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(5, 0)) =
        albedoWhite;

    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(6),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowDiffuseSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(6, 0)) =
        albedoWhite;

    // Shader pair.
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(7),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryEmissiveSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(7, 0)) =
        albedoRed;
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(8),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowEmissiveSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(8, 0)) =
        albedoRed;

    // Shader pair.
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(9),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryEmissiveSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(9, 0)) =
        albedoGreen;
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(10),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowEmissiveSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(10, 0)) =
        albedoGreen;

    // Shader pair.
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(11),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupPrimaryEmissiveSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(11, 0)) =
        albedoBlue;
    memcpy(sbtData.get() + sbtHelper.GetShaderIdentifierOffset(12),
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupShadowEmissiveSphere"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(sbtData.get() +
                                 sbtHelper.GetShaderRootArgumentOffset(12, 0)) =
        albedoBlue;

    ResourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, sbtHelper.GetShaderTableSize(),
        sbtHelper.GetShaderTableSize(), sbtData.get());
    ResourceShaderTable->SetName(L"DXR Shader Table");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structure.
  CComPtr<ID3D12Resource1> resourceBLAS = DXRCreateBLAS(
      device.get(), Make_D3D12_RAYTRACING_AABB(-1, -1, -1, 1, 1, 1));
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
    // TLAS - Build the top level acceleration structure.
    CComPtr<ID3D12Resource1> resourceTLAS;
    {
      static float rotate = 0;
      std::array<D3D12_RAYTRACING_INSTANCE_DESC, 5> DxrInstance = {};
      DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixScale(Vector3{10, 1, 10}), 0, resourceBLAS);
      DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{0, 1, 0}), 2, resourceBLAS);
      DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{cosf(rotate + Pi<float> * 0 / 3) * 2, 1,
                                        sinf(rotate + Pi<float> * 0 / 3) * 2}),
          4, resourceBLAS);
      DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{cosf(rotate + Pi<float> * 2 / 3) * 2, 1,
                                        sinf(rotate + Pi<float> * 2 / 3) * 2}),
          6, resourceBLAS);
      DxrInstance[4] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{cosf(rotate + Pi<float> * 4 / 3) * 2, 1,
                                        sinf(rotate + Pi<float> * 4 / 3) * 2}),
          8, resourceBLAS);
      resourceTLAS =
          DXRCreateTLAS(device.get(), &DxrInstance[0], DxrInstance.size());
      rotate += 0.01f;
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a constant buffer view for top level data.
    CComPtr<ID3D12Resource> ResourceConstants = D3D12_Create_Buffer(
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
          &Make_D3D12_CONSTANT_BUFFER_VIEW_DESC(ResourceConstants, 256),
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
                ResourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(0);
            desc.RayGenerationShaderRecord.SizeInBytes =
                sbtHelper.GetShaderEntrySize();
            desc.MissShaderTable.StartAddress =
                ResourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(1);
            desc.MissShaderTable.SizeInBytes = sbtHelper.GetShaderEntrySize();
            desc.MissShaderTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.HitGroupTable.StartAddress =
                ResourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(3);
            desc.HitGroupTable.SizeInBytes =
                sbtHelper.GetShaderEntrySize() * 16;
            desc.HitGroupTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.Width = descTarget.Width;
            desc.Height = descTarget.Height;
            desc.Depth = 1;
            commandList->DispatchRays(&desc);
          }
        });
  };
}