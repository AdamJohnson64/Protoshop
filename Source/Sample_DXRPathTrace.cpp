#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_DXGI.h"
#include "Core_DXRUtil.h"
#include "Core_Math.h"
#include "Sample_DXRBase.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include "generated.Sample_DXRPathTrace.dxr.h"
#include <array>
#include <atlbase.h>

class Sample_DXRPathTrace : public Sample_DXRBase
{
private:
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXRPathTrace(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
        Sample_DXRBase(pSwapChain, pDevice)
    {
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

            const WCHAR* shaderExports[] = { L"RayGenerationMVPClip", L"Miss", L"HitGroupPlane", L"HitGroupSphereDiffuse", L"HitGroupSphereEmissive", L"IntersectPlane", L"IntersectSphere" };
            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descSubobjectExports = {};
            descSubobjectExports.NumExports = _countof(shaderExports);
            descSubobjectExports.pExports = shaderExports;
            descSubobjectExports.pSubobjectToAssociate = &descSubobject[setupSubobject - 1];
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            descSubobject[setupSubobject].pDesc = &descSubobjectExports;
            ++setupSubobject;

            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            descSubobject[setupSubobject].pDesc = &m_pRootSignature.p;
            ++setupSubobject;

            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descShaderRootSignature = {};
            descShaderRootSignature.NumExports = _countof(shaderExports);
            descShaderRootSignature.pExports = shaderExports;
            descShaderRootSignature.pSubobjectToAssociate = &descSubobject[setupSubobject - 1];
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            descSubobject[setupSubobject].pDesc = &descShaderRootSignature;
            ++setupSubobject;

            D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig = {};
            descPipelineConfig.MaxTraceRecursionDepth = 2;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
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
            TRYD3D(m_pDevice->m_pDevice->CreateStateObject(&descStateObject, __uuidof(ID3D12StateObject), (void**)&m_pPipelineStateObject));
            m_pPipelineStateObject->SetName(L"DXR Pipeline State");
        }
    }
    void RenderSample() override
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Create AABBs.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource> AABBs;
        {
            D3D12_RAYTRACING_AABB aabb = {};
            aabb.MinX = -1;
            aabb.MinY = -1;
            aabb.MinZ = -1;
            aabb.MaxX = 1;
            aabb.MaxY = 1;
            aabb.MaxZ = 1;
            AABBs.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(aabb), sizeof(aabb), &aabb);
        }
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
            ResourceBLAS.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            CComPtr<ID3D12Resource1> ResourceASScratch;
            ResourceASScratch.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            // Build the acceleration structure.
            RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList5* UploadBLASCommandList) {
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
            static float rotate = 0;
            std::array<D3D12_RAYTRACING_INSTANCE_DESC, 5> DxrInstance = {};
            DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixScale(Vector3 {10, 1, 10}), 0, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 {0, 1, 0}), 1, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 {cosf(rotate + Pi<float> * 0 / 3) * 2, 1, sinf(rotate + Pi<float> * 0 / 3) * 2}), 2, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 {cosf(rotate + Pi<float> * 2 / 3) * 2, 1, sinf(rotate + Pi<float> * 2 / 3) * 2}), 3, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[4] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 {cosf(rotate + Pi<float> * 4 / 3) * 2, 1, sinf(rotate + Pi<float> * 4 / 3) * 2}), 4, ResourceBLAS->GetGPUVirtualAddress());
            ResourceInstance.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(DxrInstance), sizeof(DxrInstance), &DxrInstance);
            rotate += 0.01f;
        }
        ////////////////////////////////////////////////////////////////////////////////
        // TLAS - Build the top level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceTLAS;
        {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
            descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            descRaytracingInputs.NumDescs = 5;
            descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            descRaytracingInputs.InstanceDescs = ResourceInstance->GetGPUVirtualAddress();
            m_pDevice->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
            // Create the output and scratch buffers.
            ResourceTLAS.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            CComPtr<ID3D12Resource1> ResourceASScratch;
            ResourceASScratch.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            // Build the acceleration structure.
            RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList4* UploadTLASCommandList) {
                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
                descBuild.DestAccelerationStructureData = ResourceTLAS->GetGPUVirtualAddress();
                descBuild.Inputs = descRaytracingInputs;
                descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
                UploadTLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
            });
        }
        // Create a constant buffer view for top level data.
        CComPtr<ID3D12Resource> ResourceConstants;
        ResourceConstants.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44), &Transpose(Invert(GetCameraViewProjection())));
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
        CComPtr<ID3D12Resource1> ResourceShaderTable;
        const uint32_t shaderEntrySize = 128;
        {
            Vector4 albedoRed = { 1, 0, 0, 0 };
            Vector4 albedoGreen = { 0, 1, 0, 0 };
            Vector4 albedoBlue = { 0, 0, 1, 0 };
            Vector4 albedoWhite = { 1, 1, 1, 0 };
            CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
            TRYD3D(m_pPipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(&stateObjectProperties));
            uint32_t shaderTableSize = shaderEntrySize * 7;
            std::unique_ptr<uint8_t[]> shaderTableCPU(new uint8_t[shaderTableSize]);
            memset(&shaderTableCPU[0], 0, shaderTableSize);
            // Shader Index 0 - Ray Generation Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 0], stateObjectProperties->GetShaderIdentifier(L"RayGenerationMVPClip"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 0] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 1 - Miss Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 1], stateObjectProperties->GetShaderIdentifier(L"Miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 1] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 2 - Hit Shader 1
            memcpy(&shaderTableCPU[shaderEntrySize * 2], stateObjectProperties->GetShaderIdentifier(L"HitGroupPlane"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 2] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 2] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoWhite;
            // Shader Index 3 - Hit Shader 2
            memcpy(&shaderTableCPU[shaderEntrySize * 3], stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereDiffuse"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 3] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 3] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoWhite;
            // Shader Index 4 - Hit Shader 3
            memcpy(&shaderTableCPU[shaderEntrySize * 4], stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereEmissive"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 4] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 4] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoRed;
            // Shader Index 5 - Hit Shader 4
            memcpy(&shaderTableCPU[shaderEntrySize * 5], stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereEmissive"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 5] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 5] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoGreen;
            // Shader Index 6 - Hit Shader 5
            memcpy(&shaderTableCPU[shaderEntrySize * 6], stateObjectProperties->GetShaderIdentifier(L"HitGroupSphereEmissive"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 6] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 6] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoBlue;
            ResourceShaderTable.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize, &shaderTableCPU[0]);
            ResourceShaderTable->SetName(L"DXR Shader Table");
        }
        ////////////////////////////////////////////////////////////////////////////////
        // RAYTRACE - Finally call the raytracer and generate the frame.
        ////////////////////////////////////////////////////////////////////////////////
        RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList4* RaytraceCommandList) {
            ID3D12DescriptorHeap* descriptorHeaps[] = { m_pDescriptorHeapCBVSRVUAV };
            RaytraceCommandList->SetDescriptorHeaps(1, descriptorHeaps);
            {
                //RaytraceCommandList->SetComputeRootConstantBufferView(0, ResourceShaderTable->GetGPUVirtualAddress());
            }
            RaytraceCommandList->SetPipelineState1(m_pPipelineStateObject);
            {
                D3D12_DISPATCH_RAYS_DESC descDispatchRays = {};
                descDispatchRays.RayGenerationShaderRecord.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + shaderEntrySize * 0;
                descDispatchRays.RayGenerationShaderRecord.SizeInBytes = shaderEntrySize;
                descDispatchRays.MissShaderTable.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + shaderEntrySize * 1;
                descDispatchRays.MissShaderTable.SizeInBytes = shaderEntrySize;
                descDispatchRays.HitGroupTable.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + shaderEntrySize * 2;
                descDispatchRays.HitGroupTable.SizeInBytes = shaderEntrySize;
                descDispatchRays.HitGroupTable.StrideInBytes = shaderEntrySize;
                descDispatchRays.Width = RENDERTARGET_WIDTH;
                descDispatchRays.Height = RENDERTARGET_HEIGHT;
                descDispatchRays.Depth = 1;
                RaytraceCommandList->DispatchRays(&descDispatchRays);
            }
        });
    }
};

std::shared_ptr<Sample> CreateSample_DXRPathTrace(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_DXRPathTrace(pSwapChain, pDevice));
}