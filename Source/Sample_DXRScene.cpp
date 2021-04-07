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
      DXR_Create_Signature_LOCAL_2SRV(device->m_pDevice);
  ////////////////////////////////////////////////////////////////////////////////
  // Map out parts of the descriptor table.
  D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase =
      descriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
  UINT descriptorElementSize =
      device->m_pDevice->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  // Our global signature has a cheeky SRV on the end; start the globals at 4.
  int descriptorOffsetLocals = 4;
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
    setup.MaxTraceRecursionDepth = 2;
    setup.HitGroups.push_back({L"PrimaryTexturedMesh",
                               D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr,
                               L"PrimaryMaterialTextured", nullptr});
    setup.HitGroups.push_back({L"ShadowTexturedMesh",
                               D3D12_HIT_GROUP_TYPE_TRIANGLES, nullptr,
                               L"ShadowMaterialTextured", nullptr});
    TRYD3D(device->m_pDevice->CreateStateObject(
        ConfigureRaytracerPipeline(setup), __uuidof(ID3D12StateObject),
        (void **)&pipelineStateObject));
    pipelineStateObject->SetName(L"DXR Pipeline State");
  }
  ////////////////////////////////////////////////////////////////////////////////
  // SHADER TABLE - Create a table of all shaders for the raytracer.
  std::vector<uint8_t> sbtData;
  RaytracingSBTHelper sbtHelper(256, 2);
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
    // Shader Index 1 - Miss Shader (Primary Rays)
    memcpy(&sbtData[0] + sbtHelper.GetShaderIdentifierOffset(1),
           stateObjectProperties->GetShaderIdentifier(L"PrimaryMiss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    // Shader Index 2 - Miss Shader (Shadow Rays)
    memcpy(&sbtData[0] + sbtHelper.GetShaderIdentifierOffset(2),
           stateObjectProperties->GetShaderIdentifier(L"ShadowMiss"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  }
#pragma region - Texture Handling -
  ////////////////////////////////////////////////////////////////////////////////
  // Texture Handling
  int countTexturesSoFar = 0;
  struct CreatedTexture {
    CComPtr<ID3D12Resource> resourceTexture;
    D3D12_GPU_DESCRIPTOR_HANDLE descriptorGPU;
  };
  // Helper function to generate a texture and inject the descriptor.
  std::function<std::shared_ptr<CreatedTexture>(IImage *)>
      createTextureFromImage = [&](IImage *image) {
        // Create the new texture entry and return the GPU VA.
        std::shared_ptr<CreatedTexture> newTexture(new CreatedTexture());
        newTexture->resourceTexture = D3D12_Create_Texture(device.get(), image);
        // Inject a view of this texture into the descriptor heap.
        int descriptorIndexToUse = descriptorOffsetLocals + countTexturesSoFar;
        device->m_pDevice->CreateShaderResourceView(
            newTexture->resourceTexture,
            &Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_Texture2D(
                DXGI_FORMAT_B8G8R8A8_UNORM),
            descriptorBase + descriptorElementSize * descriptorIndexToUse);
        newTexture->descriptorGPU =
            descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart() +
            descriptorElementSize * descriptorIndexToUse;
        ++countTexturesSoFar;
        return newTexture;
      };
  // Create a texture to use if all else fails.
  std::shared_ptr<CreatedTexture> textureLightBlue =
      createTextureFromImage(Image_SolidColor(8, 8, 0xFF8080FF).get());
  // Create a texture from a texture image object.
  std::function<std::shared_ptr<CreatedTexture>(const TextureImage *)>
      createTextureFromFileRef = [&](const TextureImage *texture) {
        if (texture == nullptr) {
          return textureLightBlue;
        }
        std::shared_ptr<IImage> image = Load_TGA(texture->Filename.c_str());
        if (image == nullptr) {
          return textureLightBlue;
        }
        return createTextureFromImage(image.get());
      };
  // Cached version to reuse texture references.
  std::map<const TextureImage *, std::shared_ptr<CreatedTexture>>
      mapImageToCreatedTexture;
  std::function<std::shared_ptr<CreatedTexture>(const TextureImage *)>
      cacheTextureFromFileRef = [&](const TextureImage *texture) {
        auto findIt = mapImageToCreatedTexture.find(texture);
        if (findIt != mapImageToCreatedTexture.end()) {
          return findIt->second;
        }
        return mapImageToCreatedTexture[texture] =
                   createTextureFromFileRef(texture);
      };
#pragma endregion
#pragma region - Material Handling -
  ////////////////////////////////////////////////////////////////////////////////
  // Material Handling
  uint32_t countMaterialsSoFar = 0;
  MutableMap<IMaterial *, uint32_t> mapMaterialToHitGroupIndex;
  mapMaterialToHitGroupIndex.fnGenerator = [&](IMaterial *material) {
    OBJMaterial *objMaterial = dynamic_cast<OBJMaterial *>(material);
    if (objMaterial == nullptr)
      return 0;
    std::shared_ptr<CreatedTexture> newTextureEntry =
        cacheTextureFromFileRef(objMaterial->DiffuseMap.get());
    std::shared_ptr<CreatedTexture> newNormalMapEntry =
        cacheTextureFromFileRef(objMaterial->NormalMap.get());
    // Set up the shader binding table entry for this material.
    int shaderTableHitGroup = countMaterialsSoFar;
    int shaderTableIndex = 3 + countMaterialsSoFar;
    memcpy(&sbtData[0] + sbtHelper.GetShaderIdentifierOffset(shaderTableIndex),
           stateObjectProperties->GetShaderIdentifier(L"PrimaryTexturedMesh"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&sbtData[0] +
               sbtHelper.GetShaderRootArgumentOffset(shaderTableIndex, 0),
           &newTextureEntry->descriptorGPU,
           sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    memcpy(&sbtData[0] +
               sbtHelper.GetShaderRootArgumentOffset(shaderTableIndex, 1),
           &newNormalMapEntry->descriptorGPU,
           sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    memcpy(&sbtData[0] +
               sbtHelper.GetShaderIdentifierOffset(shaderTableIndex + 1),
           stateObjectProperties->GetShaderIdentifier(L"ShadowTexturedMesh"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    countMaterialsSoFar += 2;
    return shaderTableHitGroup;
  };
#pragma endregion
#pragma region - Mesh Handling -
  ////////////////////////////////////////////////////////////////////////////////
  // BLAS - Build the bottom level acceleration structures.
  struct VertexAttributes {
    Vector3 Position;
    Vector3 Normal;
    Vector2 Texcoord;
  };
  std::vector<VertexAttributes> vertexAttributes;
  struct CreatedMesh {
    CComPtr<ID3D12Resource1> resourceBLAS;
    uint32_t indexIntoVertexAttributes;
  };
  std::function<CreatedMesh(const IMesh *)> createMesh = [&](const IMesh
                                                                 *mesh) {
    std::unique_ptr<uint32_t[]> dataIndex(new uint32_t[mesh->getIndexCount()]);
    mesh->copyIndices(dataIndex.get(), sizeof(uint32_t));
    std::unique_ptr<Vector3[]> dataVertex(new Vector3[mesh->getVertexCount()]);
    mesh->copyVertices(dataVertex.get(), sizeof(Vector3));
    std::unique_ptr<Vector3[]> dataNormal(new Vector3[mesh->getVertexCount()]);
    mesh->copyNormals(dataNormal.get(), sizeof(Vector3));
    std::unique_ptr<Vector2[]> dataTexcoord(
        new Vector2[mesh->getVertexCount()]);
    mesh->copyTexcoords(dataTexcoord.get(), sizeof(Vector2));
    CreatedMesh newMesh = {};
    // Record the current index into the concatenated attributes.
    newMesh.indexIntoVertexAttributes = vertexAttributes.size();
    newMesh.resourceBLAS =
        DXRCreateBLAS(device.get(), dataVertex.get(), mesh->getVertexCount(),
                      DXGI_FORMAT_R32G32B32_FLOAT, dataIndex.get(),
                      mesh->getIndexCount(), DXGI_FORMAT_R32_UINT);
    // Add all attributes to our concatenated buffer.
    int indexCount = mesh->getIndexCount();
    for (int i = 0; i < indexCount; ++i) {
      VertexAttributes v = {};
      v.Position = dataVertex[dataIndex[i]];
      v.Normal = dataNormal[dataIndex[i]];
      v.Texcoord = dataTexcoord[dataIndex[i]];
      vertexAttributes.push_back(v);
    }
    return newMesh;
  };
  // Cached version.
  std::map<const IMesh *, CreatedMesh> mapMeshToCreatedMesh;
  std::function<CreatedMesh(const IMesh *)> cacheMesh = [&](const IMesh *mesh) {
    auto findIt = mapMeshToCreatedMesh.find(mesh);
    if (findIt != mapMeshToCreatedMesh.end()) {
      return findIt->second;
    }
    return mapMeshToCreatedMesh[mesh] = createMesh(mesh);
  };
#pragma endregion
#pragma region - Instance Handling -
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    instanceDescs.resize(scene.size());
    for (int i = 0; i < scene.size(); ++i) {
      const Instance &thisInstance = scene[i];
      const uint32_t indexOfHitGroup =
          mapMaterialToHitGroupIndex(thisInstance.Material.get());
      const CreatedMesh &createdMesh = cacheMesh(thisInstance.Mesh.get());
      instanceDescs[i] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          *thisInstance.TransformObjectToWorld, indexOfHitGroup,
          createdMesh.resourceBLAS);
      // We're using InstanceID to look up into the concatenated attributes.
      instanceDescs[i].InstanceID = createdMesh.indexIntoVertexAttributes;
    }
    resourceTLAS =
        DXRCreateTLAS(device.get(), &instanceDescs[0], instanceDescs.size());
  }
#pragma endregion
  ////////////////////////////////////////////////////////////////////////////////
  // Upload the concatenated attributes.
  CComPtr<ID3D12Resource1> resourceVertexAttributes = D3D12_Create_Buffer(
      device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
      D3D12_RESOURCE_STATE_COMMON,
      sizeof(VertexAttributes) * vertexAttributes.size(),
      sizeof(VertexAttributes) * vertexAttributes.size(), &vertexAttributes[0]);
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
    auto persistBLAS = mapMeshToCreatedMesh;
    auto persistTexture = mapImageToCreatedTexture;
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
      // Create the SRV for the concatenated attributes.
      D3D12_SHADER_RESOURCE_VIEW_DESC descSRVVertexAttributes = {};
      descSRVVertexAttributes.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
      descSRVVertexAttributes.Shader4ComponentMapping =
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      descSRVVertexAttributes.Buffer.NumElements = vertexAttributes.size();
      descSRVVertexAttributes.Buffer.StructureByteStride =
          sizeof(VertexAttributes);
      device->m_pDevice->CreateShaderResourceView(
          resourceVertexAttributes.p, &descSRVVertexAttributes,
          descriptorBase + descriptorElementSize * 3);
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
            desc.MissShaderTable.SizeInBytes =
                sbtHelper.GetShaderEntrySize() * 2;
            desc.MissShaderTable.StrideInBytes = sbtHelper.GetShaderEntrySize();
            desc.HitGroupTable.StartAddress =
                resourceShaderTable->GetGPUVirtualAddress() +
                sbtHelper.GetShaderIdentifierOffset(3);
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