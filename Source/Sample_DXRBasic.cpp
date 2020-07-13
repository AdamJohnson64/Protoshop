#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Sample_DXRBase.h"
#include "generated.Sample_DXRBasic.dxr.h"
#include <atlbase.h>
#include <d3dcompiler.h>
#include <array>
#include <functional>
#include <memory>

class Sample_DXRBasic : public Sample_DXRBase
{
private:
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXRBasic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
        Sample_DXRBase(pSwapChain, pDevice)
    {
        ////////////////////////////////////////////////////////////////////////////////
        // PIPELINE - Build the pipeline with all ray shaders.
        {
            uint32_t setupSubobject = 0;

            std::array<D3D12_STATE_SUBOBJECT, 8> descSubobject = {};

            D3D12_DXIL_LIBRARY_DESC descLibrary = {};
            descLibrary.DXILLibrary.pShaderBytecode = g_dxr_shader;
            descLibrary.DXILLibrary.BytecodeLength = sizeof(g_dxr_shader);
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            descSubobject[setupSubobject].pDesc = &descLibrary;
            ++setupSubobject;

            D3D12_RAYTRACING_SHADER_CONFIG descShaderConfig = {};
            descShaderConfig.MaxPayloadSizeInBytes = sizeof(float[3]) + sizeof(float); // RGB + Distance
            descShaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            descSubobject[setupSubobject].pDesc = &descShaderConfig;
            ++setupSubobject;

            const WCHAR* shaderExports[] = { L"raygeneration", L"miss", L"HitGroup" };
            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descSubobjectExports = {};
            descSubobjectExports.NumExports = _countof(shaderExports);
            descSubobjectExports.pExports = shaderExports;
            descSubobjectExports.pSubobjectToAssociate = &descSubobject[setupSubobject - 1];
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            descSubobject[setupSubobject].pDesc = &descSubobjectExports;
            ++setupSubobject;

            D3D12_STATE_SUBOBJECT descRootSignature = {};
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            descSubobject[setupSubobject].pDesc = &m_pRootSignature.p;
            ++setupSubobject;

            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descShaderRootSignature = {};
            descShaderRootSignature.NumExports = _countof(shaderExports);
            descShaderRootSignature.pExports = shaderExports;
            descShaderRootSignature.pSubobjectToAssociate = &descSubobject[setupSubobject - 1];

            D3D12_STATE_SUBOBJECT descShaderRootSignatureAssociation = {};
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            descSubobject[setupSubobject].pDesc = &descShaderRootSignature;
            ++setupSubobject;

            D3D12_RAYTRACING_PIPELINE_CONFIG descPipelineConfig = {};
            descPipelineConfig.MaxTraceRecursionDepth = 1;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            descSubobject[setupSubobject].pDesc = &descPipelineConfig;
            ++setupSubobject;

            D3D12_HIT_GROUP_DESC descHitGroup = {};
            descHitGroup.HitGroupExport = L"HitGroup";
            descHitGroup.ClosestHitShaderImport = L"closesthit";
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            descSubobject[setupSubobject].pDesc = &descHitGroup;
            ++setupSubobject;

            D3D12_STATE_OBJECT_DESC descStateObject = {};
            descStateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
            descStateObject.NumSubobjects = setupSubobject;
            descStateObject.pSubobjects = &descSubobject[0];
            TRYD3D(m_pDevice->GetID3D12Device()->CreateStateObject(&descStateObject, __uuidof(ID3D12StateObject), (void**)&m_pPipelineStateObject));
            m_pPipelineStateObject->SetName(L"DXR Pipeline State");
        }
    }
    void RenderSample() override
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Create some simple geometry.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource> Vertices;
        CComPtr<ID3D12Resource> Indices;
        {
            float vertices[] =
            {
                0, 0,
                0, RENDERTARGET_HEIGHT,
                RENDERTARGET_WIDTH, 0,
            };
            Vertices.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(vertices), sizeof(vertices), vertices);
            uint32_t indices[] =
            {
                0, 1, 2
            };
            Indices.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(indices), sizeof(indices), indices);
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
            D3D12_RAYTRACING_GEOMETRY_DESC descGeometry = {};
            descGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            descGeometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            descGeometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
            descGeometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32_FLOAT;
            descGeometry.Triangles.IndexCount = 3;
            descGeometry.Triangles.VertexCount = 3;
            descGeometry.Triangles.IndexBuffer = Indices->GetGPUVirtualAddress();
            descGeometry.Triangles.VertexBuffer.StartAddress = Vertices->GetGPUVirtualAddress();
            descGeometry.Triangles.VertexBuffer.StrideInBytes = sizeof(float[2]);
            descRaytracingInputs.pGeometryDescs = &descGeometry;
            m_pDevice->GetID3D12Device()->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
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
            D3D12_RAYTRACING_INSTANCE_DESC DxrInstance = {};
            DxrInstance.Transform[0][0] = 1;
            DxrInstance.Transform[1][1] = 1;
            DxrInstance.Transform[2][2] = 1;
            DxrInstance.InstanceMask = 0xFF;
            DxrInstance.AccelerationStructure = ResourceBLAS->GetGPUVirtualAddress();
            ResourceInstance.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(DxrInstance), sizeof(DxrInstance), &DxrInstance);
        }
        ////////////////////////////////////////////////////////////////////////////////
        // TLAS - Build the top level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceTLAS;
        {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
            descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            descRaytracingInputs.NumDescs = 1;
            descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            descRaytracingInputs.InstanceDescs = ResourceInstance->GetGPUVirtualAddress();
            m_pDevice->GetID3D12Device()->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
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
        // Establish resource views.
        {
    	    D3D12_CPU_DESCRIPTOR_HANDLE descriptorBase = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetCPUDescriptorHandleForHeapStart();
	        UINT descriptorElementSize = m_pDevice->GetID3D12Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            // Create the UAV for the raytracer output.
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
                descUAV.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                m_pDevice->GetID3D12Device()->CreateUnorderedAccessView(m_pResourceTargetUAV, nullptr, &descUAV, descriptorBase);
                descriptorBase.ptr += descriptorElementSize;
            }
            // Create the SRV for the acceleration structure.
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
    	        descSRV.Format = DXGI_FORMAT_UNKNOWN;
    	        descSRV.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    	        descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    	        descSRV.RaytracingAccelerationStructure.Location = ResourceTLAS->GetGPUVirtualAddress();
    	        m_pDevice->GetID3D12Device()->CreateShaderResourceView(nullptr, &descSRV, descriptorBase);
                descriptorBase.ptr += descriptorElementSize;
            }
        }
        ////////////////////////////////////////////////////////////////////////////////
        // SHADER TABLE - Create a table of all shaders for the raytracer.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceShaderTable;
        {
            CComPtr<ID3D12StateObjectProperties> stateObjectProperties;
            TRYD3D(m_pPipelineStateObject->QueryInterface<ID3D12StateObjectProperties>(&stateObjectProperties));
            uint32_t shaderEntrySize = 64;
            uint32_t shaderTableSize = shaderEntrySize * 3;
            std::unique_ptr<uint8_t[]> shaderTableCPU(new uint8_t[shaderTableSize]);
            memset(&shaderTableCPU[0], 0, shaderTableSize);
            // Shader Index 0 - Ray Generation Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 0], stateObjectProperties->GetShaderIdentifier(L"raygeneration"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 0] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 1 - Miss Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 1], stateObjectProperties->GetShaderIdentifier(L"miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 1] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 2 - Hit Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 2], stateObjectProperties->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 2] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
            ResourceShaderTable.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize, &shaderTableCPU[0]);
            ResourceShaderTable->SetName(L"DXR Shader Table");
        }
        ////////////////////////////////////////////////////////////////////////////////
        // RAYTRACE - Finally call the raytracer and generate the frame.
        ////////////////////////////////////////////////////////////////////////////////
        RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList4* RaytraceCommandList) {
            ID3D12DescriptorHeap* descriptorHeaps[] = { m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV() };
            RaytraceCommandList->SetDescriptorHeaps(1, descriptorHeaps);
            RaytraceCommandList->SetPipelineState1(m_pPipelineStateObject);
            {
                D3D12_DISPATCH_RAYS_DESC descDispatchRays = {};
                descDispatchRays.RayGenerationShaderRecord.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + 64 * 0;
                descDispatchRays.RayGenerationShaderRecord.SizeInBytes = 64;
                descDispatchRays.MissShaderTable.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + 64 * 1;
                descDispatchRays.MissShaderTable.SizeInBytes = 64;
                descDispatchRays.MissShaderTable.StrideInBytes = 64;
                descDispatchRays.HitGroupTable.StartAddress = ResourceShaderTable->GetGPUVirtualAddress() + 64 * 2;
                descDispatchRays.HitGroupTable.SizeInBytes = 64;
                descDispatchRays.HitGroupTable.StrideInBytes = 64;
                descDispatchRays.Width = RENDERTARGET_WIDTH;
                descDispatchRays.Height = RENDERTARGET_HEIGHT;
                descDispatchRays.Depth = 1;
                RaytraceCommandList->DispatchRays(&descDispatchRays);
            }
        });
    }
};

std::shared_ptr<Sample> CreateSample_DXRBasic(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_DXRBasic(pSwapChain, pDevice));
}