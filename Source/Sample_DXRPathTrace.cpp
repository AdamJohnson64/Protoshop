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
    descShaderConfig.MaxPayloadSizeInBytes = sizeof(float[3]) + sizeof(float) +
                                             sizeof(int) +
                                             sizeof(int); // Size of RayPayload
    descShaderConfig.MaxAttributeSizeInBytes =
        D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    descSubobject[setupSubobject].pDesc = &descShaderConfig;
    ++setupSubobject;

    const WCHAR *shaderExports[] = {
        L"RayGenerationMVPClip",   L"Miss",
        L"HitGroupPlane",          L"HitGroupSphereDiffuse",
        L"HitGroupSphereEmissive", L"IntersectPlane",
        L"IntersectSphere"};
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descSubobjectExports = {};
    descSubobjectExports.NumExports = _countof(shaderExports);
    descSubobjectExports.pExports = shaderExports;
    descSubobjectExports.pSubobjectToAssociate =
        &descSubobject[setupSubobject - 1];
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    descSubobject[setupSubobject].pDesc = &descSubobjectExports;
    ++setupSubobject;

    // NOTE: This sample has both a GLOBAL and LOCAL root signature.
    // We'll be using the GLOBAL signature to pass raytracing buffers and
    // constants into every shader. The LOCAL signature will just host a few
    // material constants that we vary per instance.
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
    descPipelineConfig.MaxTraceRecursionDepth = 2;
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    descSubobject[setupSubobject].pDesc = &descPipelineConfig;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroupPlane = {};
    descHitGroupPlane.HitGroupExport = L"HitGroupPlane";
    descHitGroupPlane.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    descHitGroupPlane.ClosestHitShaderImport = L"MaterialDiffuse";
    descHitGroupPlane.IntersectionShaderImport = L"IntersectPlane";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroupPlane;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroupSphereDiffuse = {};
    descHitGroupSphereDiffuse.HitGroupExport = L"HitGroupSphereDiffuse";
    descHitGroupSphereDiffuse.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    descHitGroupSphereDiffuse.ClosestHitShaderImport = L"MaterialDiffuse";
    descHitGroupSphereDiffuse.IntersectionShaderImport = L"IntersectSphere";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroupSphereDiffuse;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroupSphereEmissive = {};
    descHitGroupSphereEmissive.HitGroupExport = L"HitGroupSphereEmissive";
    descHitGroupSphereEmissive.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    descHitGroupSphereEmissive.ClosestHitShaderImport = L"MaterialEmissive";
    descHitGroupSphereEmissive.IntersectionShaderImport = L"IntersectSphere";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroupSphereEmissive;
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
      AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(float) * 4,
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
      descriptorOffsetHitGroup + shaderEntrySize * 5;
  // Now build this table.
  CComPtr<ID3D12Resource1> ResourceShaderTable;
  {
    Vector4 albedoRed = {1, 0, 0, 0};
    Vector4 albedoGreen = {0, 1, 0, 0};
    Vector4 albedoBlue = {0, 0, 1, 0};
    Vector4 albedoWhite = {1, 1, 1, 0};
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
           stateObjectProperties->GetShaderIdentifier(L"HitGroupPlane"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0] +
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = albedoWhite;
    // Shader Index 3 - Hit Shader 2
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1],
           stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereDiffuse"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1] +
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = albedoWhite;
    // Shader Index 4 - Hit Shader 3
    memcpy(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 2],
        stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereEmissive"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 2] +
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = albedoRed;
    // Shader Index 5 - Hit Shader 4
    memcpy(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 3],
        stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereEmissive"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 3] +
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = albedoGreen;
    // Shader Index 6 - Hit Shader 5
    memcpy(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 4],
        stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereEmissive"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    *reinterpret_cast<Vector4 *>(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 4] +
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = albedoBlue;
    ResourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize,
        &shaderTableCPU[0]);
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
          CreateMatrixTranslate(Vector3{0, 1, 0}), 1, resourceBLAS);
      DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{cosf(rotate + Pi<float> * 0 / 3) * 2, 1,
                                        sinf(rotate + Pi<float> * 0 / 3) * 2}),
          2, resourceBLAS);
      DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{cosf(rotate + Pi<float> * 2 / 3) * 2, 1,
                                        sinf(rotate + Pi<float> * 2 / 3) * 2}),
          3, resourceBLAS);
      DxrInstance[4] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{cosf(rotate + Pi<float> * 4 / 3) * 2, 1,
                                        sinf(rotate + Pi<float> * 4 / 3) * 2}),
          4, resourceBLAS);
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
                descriptorOffsetRayGenerationShader;
            desc.RayGenerationShaderRecord.SizeInBytes = shaderEntrySize;
            desc.MissShaderTable.StartAddress =
                ResourceShaderTable->GetGPUVirtualAddress() +
                descriptorOffsetMissShader;
            desc.MissShaderTable.SizeInBytes = shaderEntrySize;
            desc.HitGroupTable.StartAddress =
                ResourceShaderTable->GetGPUVirtualAddress() +
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