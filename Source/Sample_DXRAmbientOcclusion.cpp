#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_ISample.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "Scene_Camera.h"
#include "generated.Sample_DXRAmbientOcclusion.dxr.h"
#include <array>
#include <atlbase.h>

class Sample_DXRAmbientOcclusion : public Object, public ISample {
private:
  std::function<void()> m_fnRender;

public:
  Sample_DXRAmbientOcclusion(std::shared_ptr<DXGISwapChain> swapchain,
                             std::shared_ptr<Direct3D12Device> device) {
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
          sizeof(float[3]) + sizeof(float) + sizeof(int) +
          sizeof(int); // Size of RayPayload
      descShaderConfig.MaxAttributeSizeInBytes =
          D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
      descSubobject[setupSubobject].Type =
          D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
      descSubobject[setupSubobject].pDesc = &descShaderConfig;
      ++setupSubobject;

      const WCHAR *shaderExports[] = {
          L"RayGenerationMVPClip", L"Miss",           L"HitGroupPlane",
          L"HitGroupSphere",       L"IntersectPlane", L"IntersectSphere"};
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
      descPipelineConfig.MaxTraceRecursionDepth = 2;
      descSubobject[setupSubobject].Type =
          D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
      descSubobject[setupSubobject].pDesc = &descPipelineConfig;
      ++setupSubobject;

      D3D12_HIT_GROUP_DESC descHitGroupPlane = {};
      descHitGroupPlane.HitGroupExport = L"HitGroupPlane";
      descHitGroupPlane.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
      descHitGroupPlane.ClosestHitShaderImport = L"MaterialAmbientOcclusion";
      descHitGroupPlane.IntersectionShaderImport = L"IntersectPlane";
      descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
      descSubobject[setupSubobject].pDesc = &descHitGroupPlane;
      ++setupSubobject;

      D3D12_HIT_GROUP_DESC descHitGroupSphere = {};
      descHitGroupSphere.HitGroupExport = L"HitGroupSphere";
      descHitGroupSphere.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
      descHitGroupSphere.ClosestHitShaderImport = L"MaterialAmbientOcclusion";
      descHitGroupSphere.IntersectionShaderImport = L"IntersectSphere";
      descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
      descSubobject[setupSubobject].pDesc = &descHitGroupSphere;
      ++setupSubobject;

      D3D12_STATE_OBJECT_DESC descStateObject = {};
      descStateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
      descStateObject.NumSubobjects = setupSubobject;
      descStateObject.pSubobjects = &descSubobject[0];
      TRYD3D(device->m_pDevice->CreateStateObject(
          &descStateObject, __uuidof(ID3D12StateObject),
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
      memcpy(
          &shaderTableCPU[descriptorOffsetRayGenerationShader],
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
      // Shader Index 3 - Hit Shader 2
      memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1],
             stateObjectProperties->GetShaderIdentifier(L"HitGroupSphere"),
             D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
      resourceShaderTable = D3D12_Create_Buffer(
          device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
          D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize,
          &shaderTableCPU[0]);
      resourceShaderTable->SetName(L"DXR Shader Table");
    }
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
    // INSTANCE - Create the instancing table.
    CComPtr<ID3D12Resource1> resourceInstance;
    {
      std::array<D3D12_RAYTRACING_INSTANCE_DESC, 4> DxrInstance = {};
      DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixScale(Vector3{10, 1, 10}), 0,
          resourceBLAS->GetGPUVirtualAddress());
      DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{-2, 1, 0}), 1,
          resourceBLAS->GetGPUVirtualAddress());
      DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{0, 1, 0}), 1,
          resourceBLAS->GetGPUVirtualAddress());
      DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(
          CreateMatrixTranslate(Vector3{2, 1, 0}), 1,
          resourceBLAS->GetGPUVirtualAddress());
      resourceInstance = D3D12_Create_Buffer(
          device.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON,
          sizeof(DxrInstance), sizeof(DxrInstance), &DxrInstance);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // TLAS - Build the top level acceleration structure.
    CComPtr<ID3D12Resource1> resourceTLAS;
    {
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
      descRaytracingPrebuild = {};
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
      descRaytracingInputs = {};
      descRaytracingInputs.Type =
          D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
      descRaytracingInputs.NumDescs = 4;
      descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
      descRaytracingInputs.InstanceDescs =
          resourceInstance->GetGPUVirtualAddress();
      device->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(
          &descRaytracingInputs, &descRaytracingPrebuild);
      // Create the output and scratch buffers.
      resourceTLAS = D3D12_Create_Buffer(
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
          device.get(), [&](ID3D12GraphicsCommandList4 *UploadTLASCommandList) {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
            descBuild.DestAccelerationStructureData =
                resourceTLAS->GetGPUVirtualAddress();
            descBuild.Inputs = descRaytracingInputs;
            descBuild.ScratchAccelerationStructureData =
                ResourceASScratch->GetGPUVirtualAddress();
            UploadTLASCommandList->BuildRaytracingAccelerationStructure(
                &descBuild, 0, nullptr);
          });
    }
    m_fnRender = [=]() {
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
      auto persistAABB = resourceAABB;
      auto persistBLAS = resourceBLAS;
      ////////////////////////////////////////////////////////////////////////////////
      // Create a constant buffer view for top level data.
      CComPtr<ID3D12Resource> resourceConstants;
      resourceConstants = D3D12_Create_Buffer(
          device.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
          D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44),
          &Invert(GetCameraWorldToClip()));
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
          desc.Shader4ComponentMapping =
              D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
          desc.RaytracingAccelerationStructure.Location =
              resourceTLAS->GetGPUVirtualAddress();
          device->m_pDevice->CreateShaderResourceView(nullptr, &desc,
                                                      descriptorBase);
          descriptorBase.ptr += descriptorElementSize;
        }
        // Create the CBV for the scene constants.
        {
          D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
          desc.BufferLocation = resourceConstants->GetGPUVirtualAddress();
          desc.SizeInBytes = 256;
          device->m_pDevice->CreateConstantBufferView(&desc, descriptorBase);
          descriptorBase.ptr += descriptorElementSize;
        }
      }

      ////////////////////////////////////////////////////////////////////////////////
      // Get the next available backbuffer.
      CComPtr<ID3D12Resource> resourceBackbuffer;
      TRYD3D(swapchain->GetIDXGISwapChain()->GetBuffer(
          swapchain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(),
          __uuidof(ID3D12Resource), (void **)&resourceBackbuffer));
      resourceBackbuffer->SetName(L"D3D12Resource (Backbuffer)");
      device->m_pDevice->CreateRenderTargetView(
          resourceBackbuffer,
          &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(),
          device->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
      ////////////////////////////////////////////////////////////////////////////////
      // RAYTRACE - Finally call the raytracer and generate the frame.
      D3D12_Run_Synchronously(device.get(), [&](ID3D12GraphicsCommandList4
                                                    *RaytraceCommandList) {
        // Attach the GLOBAL signature and descriptors to the compute root.
        RaytraceCommandList->SetComputeRootSignature(rootSignatureGLOBAL);
        RaytraceCommandList->SetDescriptorHeaps(1, &descriptorHeapCBVSRVUAV.p);
        RaytraceCommandList->SetComputeRootDescriptorTable(
            0, descriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());
        // Prepare the pipeline for raytracing.
        RaytraceCommandList->SetPipelineState1(pipelineStateObject);
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
          RaytraceCommandList->DispatchRays(&desc);
          RaytraceCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(
                     resourceTarget, D3D12_RESOURCE_STATE_COMMON,
                     D3D12_RESOURCE_STATE_COPY_SOURCE));
          RaytraceCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resourceBackbuffer,
                                              D3D12_RESOURCE_STATE_COMMON,
                                              D3D12_RESOURCE_STATE_COPY_DEST));
          RaytraceCommandList->CopyResource(resourceBackbuffer, resourceTarget);
          RaytraceCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resourceTarget,
                                              D3D12_RESOURCE_STATE_COPY_SOURCE,
                                              D3D12_RESOURCE_STATE_COMMON));
          RaytraceCommandList->ResourceBarrier(
              1, &Make_D3D12_RESOURCE_BARRIER(resourceBackbuffer,
                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                              D3D12_RESOURCE_STATE_COMMON));
        }
      });
      // Swap the backbuffer and send this to the desktop composer for display.
      TRYD3D(swapchain->GetIDXGISwapChain()->Present(0, 0));
    };
  }
  void Render() override { m_fnRender(); }
};

std::shared_ptr<ISample>
CreateSample_DXRAmbientOcclusion(std::shared_ptr<DXGISwapChain> swapchain,
                                 std::shared_ptr<Direct3D12Device> device) {
  return std::shared_ptr<ISample>(
      new Sample_DXRAmbientOcclusion(swapchain, device));
}