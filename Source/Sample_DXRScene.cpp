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
#include "ImageUtil.h"
#include "Image_TGA.h"
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
      D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 256);
  CComPtr<ID3D12RootSignature> rootSignatureGLOBAL =
      DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
  CComPtr<ID3D12RootSignature> rootSignatureLOCAL =
      DXR_Create_Signature_LOCAL_1SRV(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // Map out parts of the descriptor table.
  D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase =
      descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
  UINT descriptorElementSize =
      device->m_pDevice->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  ////////////////////////////////////////////////////////////////////////////////
  // PIPELINE - Build the pipeline with all ray shaders.
  CComPtr<ID3D12StateObject> pipelineStateObject;
  {
    SimpleRaytracerPipelineSetup setup = {};
    setup.GlobalRootSignature = rootSignatureGLOBAL.p;
    setup.LocalRootSignature = rootSignatureLOCAL.p;
    setup.pShaderBytecode = g_dxr_shader;
    setup.BytecodeLength = sizeof(g_dxr_shader);
    setup.MaxPayloadSizeInBytes = sizeof(float[3]); // Size of RayPayload
    setup.MaxTraceRecursionDepth = 1;
    setup.HitGroups.push_back({L"HitGroupCheckerboardMesh",
                               D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr,
                               L"MaterialCheckerboard", nullptr});
    setup.HitGroups.push_back({L"HitGroupTexturedMesh",
                               D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr,
                               L"MaterialTextured", nullptr});
    TRYD3D(device->m_pDevice->CreateStateObject(
        ConfigureRaytracerPipeline(setup), __uuidof(ID3D12StateObject),
        (void **)&pipelineStateObject));
    pipelineStateObject->SetName(L"DXR Pipeline State");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // SHADER TABLE - Create a table of all shaders for the raytracer.
  std::vector<uint8_t> sbtData;
  RaytracingSBTHelper sbtHelper(256, 1);
  sbtData.resize(sbtHelper.GetShaderTableSize());
  CComPtr<ID3D12Resource1> resourceShaderTable;
  CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
  TRYD3D(pipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(
      &stateObjectProperties));
  {
    memset(&sbtData[0], 0, sbtHelper.GetShaderTableSize());
    // Shader Index 0 - Ray Generation Shader
    memcpy(&sbtData[0] + sbtHelper.GetShaderIdentifierOffset(0),
           stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 1 - Miss Shader
    memcpy(&sbtData[0] + sbtHelper.GetShaderIdentifierOffset(1),
           stateObjectProperties->GetShaderIdentifier(L"Miss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Hit Shader 1
    memcpy(
        &sbtData[0] + sbtHelper.GetShaderIdentifierOffset(2),
        stateObjectProperties->GetShaderIdentifier(L"HitGroupCheckerboardMesh"),
        D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Texture Loader
  // Given a TextureImage object we load the (assumed) TGA image into a new
  // D3D12 resource and mark the index of the loaded texture in the descriptor
  // heap.
  int countTexture = 0;
  struct TextureWithView {
    CComPtr<ID3D12Resource> resource;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
  };
  MutableMap<const TextureImage *, std::shared_ptr<TextureWithView>>
      factoryTexture;
  factoryTexture.fnGenerator = [&](const TextureImage *texture) {
    if (texture == nullptr) {
      return std::shared_ptr<TextureWithView>(nullptr);
    }
    std::shared_ptr<IImage> image = Load_TGA(texture->Filename.c_str());
    if (image == nullptr) {
      return std::shared_ptr<TextureWithView>(nullptr);
    }
    // Create the new texture entry and return the GPU VA.
    std::shared_ptr<TextureWithView> newTexture(new TextureWithView());
    newTexture->resource = D3D12_Create_Texture(device.get(), image.get());
    // Inject a view of this texture into the descriptor heap.
    int descriptorIndexToUse = 3 + countTexture;
    device->m_pDevice->CreateShaderResourceView(
        newTexture->resource,
        &Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_Texture2D(
            DXGI_FORMAT_B8G8R8A8_UNORM),
        descriptorBase + descriptorElementSize * descriptorIndexToUse);
    newTexture->gpu =
        descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart() +
        descriptorElementSize * descriptorIndexToUse;
    ++countTexture;
    return newTexture;
  };
  ////////////////////////////////////////////////////////////////////////////////
  // Material Mapper.
  // Given an IMaterial map to the index in the SBT which implements that
  // material.
  uint32_t countMaterial = 0;
  MutableMap<IMaterial *, uint32_t> factoryMaterial;
  factoryMaterial.fnGenerator = [&](IMaterial *material) mutable {
    OBJMaterial *objMaterial = dynamic_cast<OBJMaterial *>(material);
    if (objMaterial == nullptr)
      return 0;
    std::shared_ptr<TextureWithView> newTextureEntry =
        factoryTexture(objMaterial->DiffuseMap.get());
    if (newTextureEntry == nullptr)
      return 0;
    // Set up the shader binding table entry for this material.
    int shaderTableHitGroup = 1 + countMaterial;
    int shaderTableIndex = 3 + countMaterial;
    memcpy(&sbtData[0] + sbtHelper.GetShaderIdentifierOffset(shaderTableIndex),
           stateObjectProperties->GetShaderIdentifier(L"HitGroupTexturedMesh"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&sbtData[0] +
               sbtHelper.GetShaderRootArgumentOffset(shaderTableIndex, 0),
           &newTextureEntry->gpu, sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    ++countMaterial;
    return shaderTableHitGroup;
  };
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
      uint32_t materialID = factoryMaterial(instance.Material.get());
      instanceDescs[i] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          *instance.TransformObjectToWorld, materialID,
          factoryBLAS(instance.Mesh.get()));
      instanceDescs[i].InstanceID = i;
    }
    resourceTLAS =
        DXRCreateTLAS(device.get(), &instanceDescs[0], instanceDescs.size());
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create the completed shader table
  resourceShaderTable = D3D12_Create_Buffer(
      device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
      D3D12_RESOURCE_STATE_COMMON, sbtHelper.GetShaderTableSize(),
      sbtHelper.GetShaderTableSize(), &sbtData[0]);
  resourceShaderTable->SetName(L"DXR Shader Table");
  ////////////////////////////////////////////////////////////////////////////////
  // Create the rendering lambda.
  return [=](const SampleResourcesD3D12UAV &sampleResources) {
    D3D12_RESOURCE_DESC descTarget =
        sampleResources.BackBufferResource->GetDesc();
    ////////////////////////////////////////////////////////////////////////////////
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
    auto persistBLAS = factoryBLAS;
    auto persistTexture = factoryTexture;
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
      // Create the UAV for the raytracer output.
      device->m_pDevice->CreateUnorderedAccessView(
          sampleResources.BackBufferResource, nullptr,
          &Make_D3D12_UNORDERED_ACCESS_VIEW_DESC_For_Texture2D(),
          descriptorBase + descriptorElementSize * 0);
      // Create the SRV for the acceleration structure.
      device->m_pDevice->CreateShaderResourceView(
          nullptr, &Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(resourceTLAS),
          descriptorBase + descriptorElementSize * 1);
      // Create the CBV for the scene constants.
      device->m_pDevice->CreateConstantBufferView(
          &Make_D3D12_CONSTANT_BUFFER_VIEW_DESC(resourceConstants, 256),
          descriptorBase + descriptorElementSize * 2);
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
            desc.HitGroupTable.SizeInBytes = sbtHelper.GetShaderEntrySize() * 2;
            desc.HitGroupTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.Width = descTarget.Width;
            desc.Height = descTarget.Height;
            desc.Depth = 1;
            commandList->DispatchRays(&desc);
          }
        });
  };
}