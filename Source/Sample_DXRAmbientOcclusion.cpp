#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "Sample.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include "generated.Sample_DXRAmbientOcclusion.dxr.h"
#include <array>
#include <atlbase.h>

class Sample_DXRAmbientOcclusion : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D12Device> m_pDevice;
    CComPtr<ID3D12Resource1> m_pResourceTargetUAV;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeapCBVSRVUAV;
    CComPtr<ID3D12RootSignature> m_pRootSignatureGLOBAL;
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXRAmbientOcclusion(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D12Device> device) :
        m_pSwapChain(swapchain),
        m_pDevice(device)
    {
        m_pResourceTargetUAV = DXR_Create_Output_UAV(device->m_pDevice);
        m_pDescriptorHeapCBVSRVUAV = D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
        m_pRootSignatureGLOBAL = DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
        ////////////////////////////////////////////////////////////////////////////////
        // PIPELINE - Build the pipeline with all ray shaders.
        {
            uint32_t setupSubobject = 0;

            std::array<D3D12_STATE_SUBOBJECT, 16> descSubobject = {};

            D3D12_DXIL_LIBRARY_DESC descLibrary = {};
            descLibrary.DXILLibrary.pShaderBytecode = g_dxr_shader;
            descLibrary.DXILLibrary.BytecodeLength = sizeof(g_dxr_shader);
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            descSubobject[setupSubobject].pDesc = &descLibrary;
            ++setupSubobject;

            D3D12_RAYTRACING_SHADER_CONFIG descShaderConfig = {};
            descShaderConfig.MaxPayloadSizeInBytes = sizeof(float[3]) + sizeof(float) + sizeof(int) + sizeof(int); // Size of RayPayload
            descShaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            descSubobject[setupSubobject].pDesc = &descShaderConfig;
            ++setupSubobject;

            const WCHAR* shaderExports[] = { L"RayGenerationMVPClip", L"Miss", L"HitGroupPlane", L"HitGroupSphere", L"IntersectPlane", L"IntersectSphere" };
            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descSubobjectExports = {};
            descSubobjectExports.NumExports = _countof(shaderExports);
            descSubobjectExports.pExports = shaderExports;
            descSubobjectExports.pSubobjectToAssociate = &descSubobject[setupSubobject - 1];
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            descSubobject[setupSubobject].pDesc = &descSubobjectExports;
            ++setupSubobject;

            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            descSubobject[setupSubobject].pDesc = &m_pRootSignatureGLOBAL.p;
            ++setupSubobject;

            D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig = {};
            descPipelineConfig.MaxTraceRecursionDepth = 2;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
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
            TRYD3D(m_pDevice->m_pDevice->CreateStateObject(&descStateObject, __uuidof(ID3D12StateObject), (void**)&m_pPipelineStateObject));
            m_pPipelineStateObject->SetName(L"DXR Pipeline State");
        }
    }
    void Render() override
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Create AABBs.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource> AABBs = D3D12_Create_Buffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(D3D12_RAYTRACING_AABB), sizeof(D3D12_RAYTRACING_AABB), &Make_D3D12_RAYTRACING_AABB(-1, -1, -1, 1, 1, 1));
        ////////////////////////////////////////////////////////////////////////////////
        // BLAS - Build the bottom level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        // Create and initialize the BLAS.
        CComPtr<ID3D12Resource1> ResourceBLAS;
        {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
            descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            descRaytracingInputs.NumDescs = 1;
            descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            D3D12_RAYTRACING_GEOMETRY_DESC descGeometry[1] = {};
            descGeometry[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
            descGeometry[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            descGeometry[0].AABBs.AABBCount = 1;
            descGeometry[0].AABBs.AABBs.StartAddress = AABBs->GetGPUVirtualAddress();
            descGeometry[0].AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
            descRaytracingInputs.pGeometryDescs = &descGeometry[0];
            m_pDevice->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
            // Create the output and scratch buffers.
            ResourceBLAS = D3D12_Create_Buffer(m_pDevice->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            CComPtr<ID3D12Resource1> ResourceASScratch;
            ResourceASScratch = D3D12_Create_Buffer(m_pDevice->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            // Build the acceleration structure.
            D3D12_Run_Synchronously(m_pDevice.get(), [&](ID3D12GraphicsCommandList5* UploadBLASCommandList) {
                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
                descBuild.DestAccelerationStructureData = ResourceBLAS->GetGPUVirtualAddress();
                descBuild.Inputs = descRaytracingInputs;
                descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
                UploadBLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
                });
        }
        ////////////////////////////////////////////////////////////////////////////////
        // INSTANCE - Create the instancing table.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceInstance;
        {
            std::array<D3D12_RAYTRACING_INSTANCE_DESC, 4> DxrInstance = {};
            DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixScale(Vector3 {10, 1, 10}), 0, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 {-2, 1, 0}), 1, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 { 0, 1, 0}), 1, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 { 2, 1, 0}), 1, ResourceBLAS->GetGPUVirtualAddress());
            ResourceInstance = D3D12_Create_Buffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(DxrInstance), sizeof(DxrInstance), &DxrInstance);
        }
        ////////////////////////////////////////////////////////////////////////////////
        // TLAS - Build the top level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceTLAS;
        {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
            descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            descRaytracingInputs.NumDescs = 4;
            descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            descRaytracingInputs.InstanceDescs = ResourceInstance->GetGPUVirtualAddress();
            m_pDevice->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
            // Create the output and scratch buffers.
            ResourceTLAS = D3D12_Create_Buffer(m_pDevice->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            CComPtr<ID3D12Resource1> ResourceASScratch;
            ResourceASScratch = D3D12_Create_Buffer(m_pDevice->m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            // Build the acceleration structure.
            D3D12_Run_Synchronously(m_pDevice.get(), [&](ID3D12GraphicsCommandList4* UploadTLASCommandList) {
                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
                descBuild.DestAccelerationStructureData = ResourceTLAS->GetGPUVirtualAddress();
                descBuild.Inputs = descRaytracingInputs;
                descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
                UploadTLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
            });
        }
        // Create a constant buffer view for top level data.
        CComPtr<ID3D12Resource> ResourceConstants;
        ResourceConstants = D3D12_Create_Buffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44), &Invert(GetCameraWorldToClip()));
        // Establish resource views.
        {
    	    D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase = m_pDescriptorHeapCBVSRVUAV->GetCPUDescriptorHandleForHeapStart();
	        UINT descriptorElementSize = m_pDevice->m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            // Create the UAV for the raytracer output.
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
                descUAV.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                m_pDevice->m_pDevice->CreateUnorderedAccessView(m_pResourceTargetUAV, nullptr, &descUAV, descriptorBase);
                descriptorBase.ptr += descriptorElementSize;
            }
            // Create the SRV for the acceleration structure.
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
    	        descSRV.Format = DXGI_FORMAT_UNKNOWN;
    	        descSRV.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    	        descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    	        descSRV.RaytracingAccelerationStructure.Location = ResourceTLAS->GetGPUVirtualAddress();
    	        m_pDevice->m_pDevice->CreateShaderResourceView(nullptr, &descSRV, descriptorBase);
                descriptorBase.ptr += descriptorElementSize;
            }
            // Create the CBV for the scene constants.
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC descCBV = {};
                descCBV.BufferLocation = ResourceConstants->GetGPUVirtualAddress();
    	        descCBV.SizeInBytes = 256;
    	        m_pDevice->m_pDevice->CreateConstantBufferView(&descCBV, descriptorBase);
                descriptorBase.ptr += descriptorElementSize;
            }
        }
        ////////////////////////////////////////////////////////////////////////////////
        // SHADER TABLE - Create a table of all shaders for the raytracer.
        ////////////////////////////////////////////////////////////////////////////////
        // Our shader entry is a shader function entrypoint only; no other descriptors or data.
        const uint32_t shaderEntrySize = AlignUp(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        // Ray Generation shader comes first, aligned as necessary.
        const uint32_t descriptorOffsetRayGenerationShader = 0;
        // Miss shader comes next.
        const uint32_t descriptorOffsetMissShader = AlignUp(descriptorOffsetRayGenerationShader + shaderEntrySize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        // Then all the HitGroup shaders.
        const uint32_t descriptorOffsetHitGroup = AlignUp(descriptorOffsetMissShader + shaderEntrySize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        // The total size of the table we expect.
        const uint32_t shaderTableSize = descriptorOffsetHitGroup + shaderEntrySize * 2;
        // Now build this table.
        CComPtr<ID3D12Resource1> ResourceShaderTable;
        {
            CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
            TRYD3D(m_pPipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(&stateObjectProperties));
            std::unique_ptr<uint8_t[]> shaderTableCPU(new uint8_t[shaderTableSize]);
            memset(&shaderTableCPU[0], 0, shaderTableSize);
            // Shader Index 0 - Ray Generation Shader
            memcpy(&shaderTableCPU[descriptorOffsetRayGenerationShader], stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            // Shader Index 1 - Miss Shader
            memcpy(&shaderTableCPU[descriptorOffsetMissShader], stateObjectProperties->GetShaderIdentifier(L"Miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            // Shader Index 2 - Hit Shader 1
            memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0], stateObjectProperties->GetShaderIdentifier(L"HitGroupPlane"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            // Shader Index 3 - Hit Shader 2
            memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1], stateObjectProperties->GetShaderIdentifier(L"HitGroupSphere"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            ResourceShaderTable = D3D12_Create_Buffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize, &shaderTableCPU[0]);
            ResourceShaderTable->SetName(L"DXR Shader Table");
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Get the next available backbuffer.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource> pD3D12Resource;
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12Resource));
        pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
        m_pDevice->m_pDevice->CreateRenderTargetView(pD3D12Resource, &Make_D3D12_RENDER_TARGET_VIEW_DESC_SwapChainDefault(), m_pDevice->m_pDescriptorHeapRTV->GetCPUDescriptorHandleForHeapStart());
        ////////////////////////////////////////////////////////////////////////////////
        // RAYTRACE - Finally call the raytracer and generate the frame.
        ////////////////////////////////////////////////////////////////////////////////
        D3D12_Run_Synchronously(m_pDevice.get(), [&](ID3D12GraphicsCommandList4* RaytraceCommandList) {
            // Attach the GLOBAL signature and descriptors to the compute root.
            RaytraceCommandList->SetComputeRootSignature(m_pRootSignatureGLOBAL);
            RaytraceCommandList->SetDescriptorHeaps(1, &m_pDescriptorHeapCBVSRVUAV.p);
            RaytraceCommandList->SetComputeRootDescriptorTable(0, m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart());
            // Prepare the pipeline for raytracing.
            RaytraceCommandList->SetPipelineState1(m_pPipelineStateObject);
            {
                D3D12_DISPATCH_RAYS_DESC descDispatchRays = {};
                descDispatchRays.RayGenerationShaderRecord.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + descriptorOffsetRayGenerationShader;
                descDispatchRays.RayGenerationShaderRecord.SizeInBytes = shaderEntrySize;
                descDispatchRays.MissShaderTable.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + descriptorOffsetMissShader;
                descDispatchRays.MissShaderTable.SizeInBytes = shaderEntrySize;
                descDispatchRays.HitGroupTable.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + descriptorOffsetHitGroup;
                descDispatchRays.HitGroupTable.SizeInBytes = shaderEntrySize;
                descDispatchRays.HitGroupTable.StrideInBytes = shaderEntrySize;
                descDispatchRays.Width = RENDERTARGET_WIDTH;
                descDispatchRays.Height = RENDERTARGET_HEIGHT;
                descDispatchRays.Depth = 1;
                RaytraceCommandList->DispatchRays(&descDispatchRays);
                RaytraceCommandList->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(m_pResourceTargetUAV, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));
                RaytraceCommandList->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(pD3D12Resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
                RaytraceCommandList->CopyResource(pD3D12Resource, m_pResourceTargetUAV);
                RaytraceCommandList->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(m_pResourceTargetUAV, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
                RaytraceCommandList->ResourceBarrier(1, &Make_D3D12_RESOURCE_BARRIER(pD3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
            }
        });
        // Swap the backbuffer and send this to the desktop composer for display.
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
    }
};

std::shared_ptr<Sample> CreateSample_DXRAmbientOcclusion(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D12Device> device)
{
    return std::shared_ptr<Sample>(new Sample_DXRAmbientOcclusion(swapchain, device));
}