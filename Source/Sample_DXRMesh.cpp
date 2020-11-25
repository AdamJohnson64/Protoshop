///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 12 DXR Mesh
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
#include "Core_Object.h"
#include "Core_Util.h"
#include "Sample.h"
#include "Scene_Camera.h"
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

class Sample_DXRMesh : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D12Device> m_pDevice;
    CComPtr<ID3D12Resource1> m_pResourceTargetUAV;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeapCBVSRVUAV;
    CComPtr<ID3D12RootSignature> m_pRootSignatureGLOBAL;
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXRMesh(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D12Device> device) :
        m_pSwapChain(swapchain),
        m_pDevice(device)
    {
        m_pResourceTargetUAV = DXR_Create_Output_UAV(device->m_pDevice);
        m_pDescriptorHeapCBVSRVUAV = D3D12_Create_DescriptorHeap_CBVSRVUAV(device->m_pDevice, 8);
        m_pRootSignatureGLOBAL = DXR_Create_Signature_GLOBAL_1UAV1SRV1CBV(device->m_pDevice);
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
            shaderExports.push_back(L"RayGenerationMVPClip");
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
            descShaderConfig.MaxPayloadSizeInBytes = sizeof(float[3]); // Size of RayPayload
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

            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
            descSubobject[setupSubobject].pDesc = &m_pRootSignatureGLOBAL.p;
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
            TRYD3D(m_pDevice->m_pDevice->CreateStateObject(&descStateObject, __uuidof(ID3D12StateObject), (void**)&m_pPipelineStateObject));
            m_pPipelineStateObject->SetName(L"DXR Pipeline State");
        }
    }
    void Render() override
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
            int sizeVertex = sizeof(float[3]) * scene->Meshes[i]->getVertexCount();
            std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
            scene->Meshes[i]->copyVertices(reinterpret_cast<Vector3*>(vertices.get()), sizeof(Vector3));
            int sizeIndices = sizeof(int32_t) * scene->Meshes[i]->getIndexCount();
            std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
            scene->Meshes[i]->copyIndices(reinterpret_cast<uint32_t*>(indices.get()), sizeof(uint32_t));
            BottomLevelAS[i] = DXRCreateBLAS(m_pDevice.get(), vertices.get(), scene->Meshes[i]->getVertexCount(), DXGI_FORMAT_R32G32B32_FLOAT, indices.get(), scene->Meshes[i]->getIndexCount(), DXGI_FORMAT_R32_UINT);
        }
        ////////////////////////////////////////////////////////////////////////////////
        // TLAS - Build the top level acceleration structure.
        ////////////////////////////////////////////////////////////////////////////////
        CComPtr<ID3D12Resource1> ResourceTLAS;
        {
            std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
            instanceDescs.resize(scene->Instances.size());
            for (int i = 0; i < scene->Instances.size(); ++i)
            {
                instanceDescs[i] = Make_D3D12_RAYTRACING_INSTANCE_DESC(scene->Instances[i].Transform, scene->Instances[i].MaterialIndex, BottomLevelAS[scene->Instances[i].GeometryIndex]->GetGPUVirtualAddress());
                instanceDescs[i].InstanceID = i;
            }
            ResourceTLAS = DXRCreateTLAS(m_pDevice.get(), &instanceDescs[0], instanceDescs.size());
        }
        // Create a constant buffer view for top level data.
        CComPtr<ID3D12Resource> ResourceConstants = D3D12_Create_Buffer(m_pDevice.get(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, 256, sizeof(Matrix44), &Invert(GetCameraViewProjection()));
        ////////////////////////////////////////////////////////////////////////////////
        // Build the descriptor table to establish resource views.
        //
        // These descriptors will be attached via the GLOBAL signature as they are all
        // shared throughout all shaders. The GLOBAL signature is established by the
        // compute root signature and descriptor heap.
        ////////////////////////////////////////////////////////////////////////////////
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
        //
        // Note that we don't have any local parameters to our shaders here so there are
        // no local descriptors following any shader identifier entry.
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
            memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 0], stateObjectProperties->GetShaderIdentifier(L"HitGroup0"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            // Shader Index 3 - Hit Shader 2
            memcpy(&shaderTableCPU[descriptorOffsetHitGroup + shaderEntrySize * 1], stateObjectProperties->GetShaderIdentifier(L"HitGroup1"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
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

std::shared_ptr<Sample> CreateSample_DXRMesh(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D12Device> device)
{
    return std::shared_ptr<Sample>(new Sample_DXRMesh(swapchain, device));
}