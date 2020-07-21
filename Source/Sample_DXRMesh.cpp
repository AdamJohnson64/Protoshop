#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Sample_DXRBase.h"
#include "Scene_InstanceTable.h"
#include "Scene_Mesh.h"
#include "Scene_ParametricUV.h"
#include "Scene_ParametricUVToMesh.h"
#include "Scene_Plane.h"
#include "Scene_Sphere.h"
#include "generated.Sample_DXRMesh.dxr.h"
#include <atlbase.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

class Sample_DXRMesh : public Sample_DXRBase
{
private:
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXRMesh(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
        Sample_DXRBase(pSwapChain, pDevice)
    {
        std::shared_ptr<InstanceTable> scene(InstanceTable::Default());
        ////////////////////////////////////////////////////////////////////////////////
        // PIPELINE - Build the pipeline with all ray shaders.
        {
            uint32_t setupSubobject = 0;

            std::array<D3D12_STATE_SUBOBJECT, 256> descSubobject = {};

            D3D12_DXIL_LIBRARY_DESC descLibrary = {};
            descLibrary.DXILLibrary.pShaderBytecode = g_dxr_shader;
            descLibrary.DXILLibrary.BytecodeLength = sizeof(g_dxr_shader);
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            descSubobject[setupSubobject].pDesc = &descLibrary;
            ++setupSubobject;

            std::vector<std::shared_ptr<std::wstring>> hitGroupNames;
            std::vector<std::shared_ptr<D3D12_HIT_GROUP_DESC>> hitGroups;
            std::vector<const WCHAR*> shaderExports;
            shaderExports.push_back(L"RayGeneration");
            shaderExports.push_back(L"Miss");
            for (int i = 0; i < scene->Materials.size(); ++i)
            {
                const Material* pMaterial = scene->Materials[i].get();
                std::shared_ptr<std::wstring> descHitGroupName(new std::wstring(std::wstring(L"HitGroup") + std::to_wstring(i)));
                hitGroupNames.push_back(descHitGroupName);
                std::shared_ptr<D3D12_HIT_GROUP_DESC> descHitGroup(new D3D12_HIT_GROUP_DESC());
                hitGroups.push_back(descHitGroup);
                shaderExports.push_back(descHitGroupName->c_str());
                descHitGroup->HitGroupExport = descHitGroupName->c_str();
                descHitGroup->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
                if (dynamic_cast<const Checkerboard*>(pMaterial))
                {
                    descHitGroup->ClosestHitShaderImport = L"MaterialCheckerboard";
                }
                else if (dynamic_cast<const RedPlastic*>(pMaterial))
                {
                    descHitGroup->ClosestHitShaderImport = L"MaterialRedPlastic";
                }
                else
                {
                    throw std::exception("Unable to construct material.");
                }
                descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
                descSubobject[setupSubobject].pDesc = descHitGroup.get();
                ++setupSubobject;
            }

            D3D12_RAYTRACING_SHADER_CONFIG descShaderConfig = {};
            descShaderConfig.MaxPayloadSizeInBytes = sizeof(float[3]) + sizeof(float); // RGB + Distance
            descShaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            descSubobject[setupSubobject].pDesc = &descShaderConfig;
            ++setupSubobject;

            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descSubobjectExports = {};
            descSubobjectExports.NumExports = shaderExports.size();
            descSubobjectExports.pExports = &shaderExports[0];
            descSubobjectExports.pSubobjectToAssociate = &descSubobject[setupSubobject - 1];
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            descSubobject[setupSubobject].pDesc = &descSubobjectExports;
            ++setupSubobject;

            D3D12_STATE_SUBOBJECT descRootSignature = {};
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
            descSubobject[setupSubobject].pDesc = &m_pRootSignature.p;
            ++setupSubobject;

            D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION descShaderRootSignature = {};
            descShaderRootSignature.NumExports = shaderExports.size();
            descShaderRootSignature.pExports = &shaderExports[0];
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
        std::shared_ptr<InstanceTable> scene(InstanceTable::Default());
        ////////////////////////////////////////////////////////////////////////////////
        // BLAS - Build the bottom level acceleration structures.
        ////////////////////////////////////////////////////////////////////////////////
        // Create and initialize the BLAS.
        std::vector<CComPtr<ID3D12Resource1>> BottomLevelAS;
        BottomLevelAS.resize(scene->Meshes.size());
        for (int i = 0; i < scene->Meshes.size(); ++i)
        {
            CComPtr<ID3D12Resource> vertexBuffer;
            CComPtr<ID3D12Resource> indexBuffer;
            {
                int sizeVertex = sizeof(float[3]) * scene->Meshes[i]->getVertexCount();
                std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
                scene->Meshes[i]->copyVertices(reinterpret_cast<Vector3*>(vertices.get()), sizeof(Vector3));
                vertexBuffer.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeVertex, sizeVertex, vertices.get());
            }
            {
                int sizeIndices = sizeof(int32_t) * scene->Meshes[i]->getIndexCount();
                std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
                scene->Meshes[i]->copyIndices(reinterpret_cast<uint32_t*>(indices.get()), sizeof(uint32_t));
                indexBuffer.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeIndices, sizeIndices, indices.get());
            }
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
            descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            descRaytracingInputs.NumDescs = 1;
            descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            D3D12_RAYTRACING_GEOMETRY_DESC descGeometry = {};
            descGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            descGeometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            descGeometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
            descGeometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            descGeometry.Triangles.IndexCount = scene->Meshes[i]->getIndexCount();
            descGeometry.Triangles.VertexCount = scene->Meshes[i]->getVertexCount();
            descGeometry.Triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress();
            descGeometry.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
            descGeometry.Triangles.VertexBuffer.StrideInBytes = sizeof(float[3]);
            descRaytracingInputs.pGeometryDescs = &descGeometry;
            m_pDevice->GetID3D12Device()->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
            // Create the output and scratch buffers.
            BottomLevelAS[i].p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            CComPtr<ID3D12Resource1> ResourceASScratch;
            ResourceASScratch.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
            // Build the acceleration structure.
            RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList5* UploadBLASCommandList) {
                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
                descBuild.DestAccelerationStructureData = BottomLevelAS[i]->GetGPUVirtualAddress();
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
            std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
            instanceDescs.resize(scene->Instances.size());
            for (int i = 0; i < scene->Instances.size(); ++i)
            {
                instanceDescs[i].Transform[0][0] = scene->Instances[i].Transform.M11;
                instanceDescs[i].Transform[0][1] = scene->Instances[i].Transform.M21;
                instanceDescs[i].Transform[0][2] = scene->Instances[i].Transform.M31;
                instanceDescs[i].Transform[0][3] = scene->Instances[i].Transform.M41;
                instanceDescs[i].Transform[1][0] = scene->Instances[i].Transform.M12;
                instanceDescs[i].Transform[1][1] = scene->Instances[i].Transform.M22;
                instanceDescs[i].Transform[1][2] = scene->Instances[i].Transform.M32;
                instanceDescs[i].Transform[1][3] = scene->Instances[i].Transform.M42;
                instanceDescs[i].Transform[2][0] = scene->Instances[i].Transform.M13;
                instanceDescs[i].Transform[2][1] = scene->Instances[i].Transform.M23;
                instanceDescs[i].Transform[2][2] = scene->Instances[i].Transform.M33;
                instanceDescs[i].Transform[2][3] = scene->Instances[i].Transform.M43;
                instanceDescs[i].InstanceID = i;
                instanceDescs[i].InstanceMask = 0xFF;
                instanceDescs[i].InstanceContributionToHitGroupIndex = scene->Instances[i].MaterialIndex;
                instanceDescs[i].AccelerationStructure = BottomLevelAS[scene->Instances[i].GeometryIndex]->GetGPUVirtualAddress();
            }
            ResourceInstance.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * scene->Instances.size(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * scene->Instances.size(), &instanceDescs[0]);
        }
        ////////////////////////////////////////////////////////////////////////////////
        // TLAS - Build the top level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceTLAS;
        {
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
            descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            descRaytracingInputs.NumDescs = scene->Instances.size();
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
            uint32_t shaderTableSize = shaderEntrySize * 4;
            std::unique_ptr<uint8_t[]> shaderTableCPU(new uint8_t[shaderTableSize]);
            memset(&shaderTableCPU[0], 0, shaderTableSize);
            // Shader Index 0 - Ray Generation Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 0], stateObjectProperties->GetShaderIdentifier(L"RayGeneration"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 0] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 1 - Miss Shader
            memcpy(&shaderTableCPU[shaderEntrySize * 1], stateObjectProperties->GetShaderIdentifier(L"Miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 1] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 2 - Hit Shader 1
            memcpy(&shaderTableCPU[shaderEntrySize * 2], stateObjectProperties->GetShaderIdentifier(L"HitGroup0"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 2] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
            // Shader Index 3 - Hit Shader 2
            memcpy(&shaderTableCPU[shaderEntrySize * 3], stateObjectProperties->GetShaderIdentifier(L"HitGroup1"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 3] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
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
                descDispatchRays.MissShaderTable.StrideInBytes = 0;
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

std::shared_ptr<Sample> CreateSample_DXRMesh(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_DXRMesh(pSwapChain, pDevice));
}