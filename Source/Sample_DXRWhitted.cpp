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
#include "Mixin_ImguiD3D12.h"
#include "Sample_DXRBase.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include "generated.Sample_DXRWhitted.dxr.h"
#include <array>
#include <atlbase.h>

class Sample_DXRWhitted : public Sample_DXRBase, public Mixin_ImguiD3D12
{
private:
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXRWhitted(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
        Sample_DXRBase(pSwapChain, pDevice),
        Mixin_ImguiD3D12(pDevice)
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

            const WCHAR* shaderExports[] = { L"RayGenerationMVPClip", L"Miss", L"HitGroupCheckerboard", L"HitGroupPlastic", L"HitGroupGlass", L"IntersectPlane", L"IntersectSphere" };
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
            descPipelineConfig.MaxTraceRecursionDepth = 4;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            descSubobject[setupSubobject].pDesc = &descPipelineConfig;
            ++setupSubobject;

            D3D12_HIT_GROUP_DESC descHitGroupCheckerboard = {};
            descHitGroupCheckerboard.HitGroupExport = L"HitGroupCheckerboard";
            descHitGroupCheckerboard.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
            descHitGroupCheckerboard.ClosestHitShaderImport = L"MaterialCheckerboard";
            descHitGroupCheckerboard.IntersectionShaderImport = L"IntersectPlane";
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            descSubobject[setupSubobject].pDesc = &descHitGroupCheckerboard;
            ++setupSubobject;

            D3D12_HIT_GROUP_DESC descHitGroupRedPlastic = {};
            descHitGroupRedPlastic.HitGroupExport = L"HitGroupPlastic";
            descHitGroupRedPlastic.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
            descHitGroupRedPlastic.ClosestHitShaderImport = L"MaterialPlastic";
            descHitGroupRedPlastic.IntersectionShaderImport = L"IntersectSphere";
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            descSubobject[setupSubobject].pDesc = &descHitGroupRedPlastic;
            ++setupSubobject;

            D3D12_HIT_GROUP_DESC descHitGroupGlass = {};
            descHitGroupGlass.HitGroupExport = L"HitGroupGlass";
            descHitGroupGlass.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
            descHitGroupGlass.ClosestHitShaderImport = L"MaterialGlass";
            descHitGroupGlass.IntersectionShaderImport = L"IntersectSphere";
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            descSubobject[setupSubobject].pDesc = &descHitGroupGlass;
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
        CComPtr<ID3D12Resource> AABBs = D3D12CreateBuffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(D3D12_RAYTRACING_AABB), sizeof(D3D12_RAYTRACING_AABB), &Make_D3D12_RAYTRACING_AABB(-1, -1, -1, 1, 1, 1));
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
            ResourceBLAS = D3D12CreateBuffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            CComPtr<ID3D12Resource1> ResourceASScratch;
            ResourceASScratch = D3D12CreateBuffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            // Build the acceleration structure.
            RunOnGPU(m_pDevice.get(), [&](ID3D12GraphicsCommandList5* UploadBLASCommandList) {
                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
                descBuild.DestAccelerationStructureData = ResourceBLAS->GetGPUVirtualAddress();
                descBuild.Inputs = descRaytracingInputs;
                descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
                UploadBLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
                });
        }
        ////////////////////////////////////////////////////////////////////////////////
        // TLAS - Build the top level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceTLAS;
        {
            std::array<D3D12_RAYTRACING_INSTANCE_DESC, 5> DxrInstance = {};
            DxrInstance[0] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixScale(Vector3 {10, 1, 10}), 0, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[1] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 {-2, 1, 0}), 1, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[2] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 { 0, 1, 0}), 2, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[3] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 { 2, 1, 0}), 3, ResourceBLAS->GetGPUVirtualAddress());
            DxrInstance[4] = Make_D3D12_RAYTRACING_INSTANCE_DESC(CreateMatrixTranslate(Vector3 { 0, 3, 0}), 4, ResourceBLAS->GetGPUVirtualAddress());
            ResourceTLAS = DXRCreateTLAS(m_pDevice.get(), &DxrInstance[0], DxrInstance.size());
        }
        // Create a constant buffer view for top level data.
        CComPtr<ID3D12Resource> ResourceConstants = D3D12CreateBuffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44), &Invert(GetCameraViewProjection()));
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
	        UINT descriptorElementSize = m_pDevice->m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_GPU_DESCRIPTOR_HANDLE constantBuffer2 = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            constantBuffer2.ptr += descriptorElementSize * 3;
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
            memcpy(&shaderTableCPU[shaderEntrySize * 2], stateObjectProperties->GetShaderIdentifier(L"HitGroupCheckerboard"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 2] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 3 - Hit Shader 2
            memcpy(&shaderTableCPU[shaderEntrySize * 3], stateObjectProperties->GetShaderIdentifier(L"HitGroupPlastic"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 3] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 3] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoRed;
            // Shader Index 4 - Hit Shader 3
            memcpy(&shaderTableCPU[shaderEntrySize * 4], stateObjectProperties->GetShaderIdentifier(L"HitGroupPlastic"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 4] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 4] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoGreen;
            // Shader Index 5 - Hit Shader 4
            memcpy(&shaderTableCPU[shaderEntrySize * 5], stateObjectProperties->GetShaderIdentifier(L"HitGroupPlastic"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 5] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 5] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoBlue;
            // Shader Index 6 - Hit Shader 5
            memcpy(&shaderTableCPU[shaderEntrySize * 6], stateObjectProperties->GetShaderIdentifier(L"HitGroupGlass"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 6] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDescriptorHeapCBVSRVUAV->GetGPUDescriptorHandleForHeapStart();
            *reinterpret_cast<Vector4*>(&shaderTableCPU[shaderEntrySize * 6] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE)) = albedoRed;
            ResourceShaderTable = D3D12CreateBuffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize, &shaderTableCPU[0]);
            ResourceShaderTable->SetName(L"DXR Shader Table");
        }
        ////////////////////////////////////////////////////////////////////////////////
        // RAYTRACE - Finally call the raytracer and generate the frame.
        ////////////////////////////////////////////////////////////////////////////////
        RunOnGPU(m_pDevice.get(), [&](ID3D12GraphicsCommandList4* RaytraceCommandList) {
            ID3D12DescriptorHeap* descriptorHeaps[] = { m_pDescriptorHeapCBVSRVUAV };
            RaytraceCommandList->SetDescriptorHeaps(1, descriptorHeaps);
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
    void RenderPost(ID3D12GraphicsCommandList5* pCommandList)
    {
        RenderImgui(pCommandList);
    }
    void BuildImguiUI() override
    {
        ImGui::Text("Dear ImGui running on Direct3D DXR.");
    }
};

std::shared_ptr<Sample> CreateSample_DXRWhitted(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_DXRWhitted(pSwapChain, pDevice));
}