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

/*
class ResourceWithView {
  CComPtr<ID3D12Resource> Resource;
  std::function<void(const D3D12_CPU_DESCRIPTOR_HANDLE&)> CreateView;
};

class DescriptorHeapManager {
public:
  std::vector<ResourceWithView> Resources;
};
*/

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
    SimpleRaytracerPipelineSetup setup = {};
    setup.GlobalRootSignature = rootSignatureGLOBAL.p;
    setup.LocalRootSignature = rootSignatureLOCAL.p;
    setup.pShaderBytecode = g_dxr_shader;
    setup.BytecodeLength = sizeof(g_dxr_shader);
    setup.MaxPayloadSizeInBytes = sizeof(float[3]);
    setup.MaxTraceRecursionDepth = 4;
    setup.HitGroups.push_back({L"HitGroupCheckerboardPlane",
                               D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
                               nullptr, L"MaterialCheckerboard",
                               L"IntersectPlane"});
    setup.HitGroups.push_back(
        {L"HitGroupPlasticSphere", D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE,
         nullptr, L"MaterialPlastic", L"IntersectSphere"});
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
           stateObjectProperties->GetShaderIdentifier(
               L"HitGroupCheckerboardPlane"),
           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0 +
                           D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES],
           &(descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart() +
             descriptorOffsetLocals),
           sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
    // Shader Index 3 - Hit Shader 2
    memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1],
           stateObjectProperties->GetShaderIdentifier(L"HitGroupPlasticSphere"),
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
  // BLAS - Build the bottom level acceleration structure.
  CComPtr<ID3D12Resource1> resourceBLAS = DXRCreateBLAS(
      device.get(), Make_D3D12_RAYTRACING_AABB(-1, -1, -1, 1, 1, 1));
  ////////////////////////////////////////////////////////////////////////////////
  // TLAS - Build the top level acceleration structure.
  CComPtr<ID3D12Resource1> resourceTLAS;
  {
    std::array<D3D12_RAYTRACING_INSTANCE_DESC, 5> DxrInstance = {};
    DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixScale(Vector3{10, 1, 10}), 0, resourceBLAS);
    DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
        CreateMatrixTranslate(Vector3{0, 1, 0}), 1, resourceBLAS);
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
      device->m_pDevice->CreateUnorderedAccessView(
          sampleResources.BackBufferResource, nullptr,
          &Make_D3D12_UNORDERED_ACCESS_VIEW_DESC_For_Texture2D(),
          descriptorBase + descriptorOffsetGlobals + descriptorElementSize * 0);
      // Create the SRV for the acceleration structure.
      device->m_pDevice->CreateShaderResourceView(
          nullptr, &Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_TLAS(resourceTLAS),
          descriptorBase + descriptorOffsetGlobals + descriptorElementSize * 1);
      // Create the CBV for the scene constants.
      device->m_pDevice->CreateConstantBufferView(
          &Make_D3D12_CONSTANT_BUFFER_VIEW_DESC(resourceConstants, 256),
          descriptorBase + descriptorOffsetGlobals + descriptorElementSize * 2);
      // Create the SRV for the texture.
      device->m_pDevice->CreateShaderResourceView(
          resourceTexture,
          &Make_D3D12_SHADER_RESOURCE_VIEW_DESC_For_Texture2D(
              DXGI_FORMAT_B8G8R8A8_UNORM),
          descriptorBase + descriptorOffsetLocals + descriptorElementSize * 0);
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