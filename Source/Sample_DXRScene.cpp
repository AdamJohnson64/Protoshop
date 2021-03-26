///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 DXR Scene
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
#include "MutableMap.h"
#include "SampleResources.h"
#include "Scene_IMaterial.h"
#include "Scene_IMesh.h"
#include "Scene_InstanceTable.h"
#include "generated.Sample_DXRScene.dxr.h"
#include <array>
#include <atlbase.h>
#include <memory>
#include <string>
#include <vector>

std::function<void(const SampleResourcesD3D12UAV &)>
CreateSample_DXRScene(std::shared_ptr<Direct3D12Device> device,
                      const std::vector<Instance> &scene) {
  CComPtr<ID3D12DescriptorHeap> descriptorHeapCBVSRVUAV =
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
  CComPtr<ID3D12RootSignature> rootSignatureGLOBAL =
      DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // PIPELINE - Build the pipeline with all ray shaders.
  CComPtr<ID3D12StateObject> pipelineStateObject;
  {
    uint32_t setupSubobject = 0;

    std::array<D3D12_STATE_SUBOBJECT, 256> descSubobject = {};

    D3D12_DXIL_LIBRARY_DESC descLibrary = {};
    descLibrary.DXILLibrary.pShaderBytecode = g_dxr_shader;
    descLibrary.DXILLibrary.BytecodeLength = sizeof(g_dxr_shader);
    descSubobject[setupSubobject].Type =
        D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    descSubobject[setupSubobject].pDesc = &descLibrary;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroup0 = {};
    descHitGroup0.HitGroupExport = L"HitGroupCheckerboardMesh";
    descHitGroup0.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    descHitGroup0.ClosestHitShaderImport = L"MaterialCheckerboard";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroup0;
    ++setupSubobject;

    D3D12_HIT_GROUP_DESC descHitGroup1 = {};
    descHitGroup1.HitGroupExport = L"HitGroupRedPlasticMesh";
    descHitGroup1.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    descHitGroup1.ClosestHitShaderImport = L"MaterialRedPlastic";
    descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    descSubobject[setupSubobject].pDesc = &descHitGroup1;
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
  // Note that we don't have any local parameters to our shaders here so
  // there are no local descriptors following any shader identifier entry.
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
      descriptorOffsetHitGroup + shaderEntrySize * 2;
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
           stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(&shaderTableCPU[descriptorOffsetMissShader],
           stateObjectProperties->GetShaderIdentifier(L"Miss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Hit Shader 1
    memcpy(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0],
        stateObjectProperties->GetShaderIdentifier(L"HitGroupCheckerboardMesh"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 3 - Hit Shader 2
    memcpy(
        &shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1],
        stateObjectProperties->GetShaderIdentifier(L"HitGroupRedPlasticMesh"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    resourceShaderTable = D3D12_Create_Buffer(
        device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize,
        &shaderTableCPU[0]);
    resourceShaderTable->SetName(L"DXR Shader Table");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structures.
  MutableMap<const IMesh *, CComPtr<ID3D12Resource1>> factoryBLAS;
  factoryBLAS.fnGenerator = [=](const IMesh *mesh) {
    int sizeVertex = sizeof(float[3]) * mesh->getVertexCount();
    std::unique_ptr<int8_t[]> dataVertex(new int8_t[sizeVertex]);
    mesh->copyVertices(reinterpret_cast<Vector3 *>(dataVertex.get()),
                       sizeof(Vector3));
    int sizeIndices = sizeof(int32_t) * mesh->getIndexCount();
    std::unique_ptr<int8_t[]> dataIndex(new int8_t[sizeIndices]);
    mesh->copyIndices(reinterpret_cast<uint32_t *>(dataIndex.get()),
                      sizeof(uint32_t));
    return DXRCreateBLAS(device.get(), dataVertex.get(), mesh->getVertexCount(),
                         DXGI_FORMAT_R32G32B32_FLOAT, dataIndex.get(),
                         mesh->getIndexCount(), DXGI_FORMAT_R32_UINT);
  };
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    instanceDescs.resize(scene.size());
    for (int i = 0; i < scene.size(); ++i) {
      const Instance &instance = scene[i];
      uint32_t materialID =
          1; // HACK: HARD CODE THE MATERIAL TO INDEX 1 (Barycentric visual)
      instanceDescs[i] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          *instance.TransformObjectToWorld, materialID,
          factoryBLAS(instance.Mesh.get()));
      instanceDescs[i].InstanceID = i;
    }
    resourceTLAS =
        DXRCreateTLAS(device.get(), &instanceDescs[0], instanceDescs.size());
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
    // constructor. The TLAS sees the BLAS only by address so we need to capture
    // the BLAS table to stop it from deallocating. Do that and your TLAS will
    // have dangling pointers to GPU memory.
    //
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
    auto persistBLAS = factoryBLAS;
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
        device.get(), [&](ID3D12GraphicsCommandList5 *commandList) {
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
            desc.Width = descTarget.Width;
            desc.Height = descTarget.Height;
            desc.Depth = 1;
            commandList->DispatchRays(&desc);
          }
        });
  };
}