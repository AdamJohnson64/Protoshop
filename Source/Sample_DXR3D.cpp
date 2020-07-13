#include "Core_D3D.h"
#include "Core_D3D12.h"
#include "Core_D3D12Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Sample_DXRBase.h"
#include "generated.Sample_DXR3D.dxr.h"
#include <atlbase.h>
#include <array>
#include <memory>

class ParametricUV
{
public:
    virtual float3 getVertexPosition(float2 uv) = 0;
};

class Plane : public Object, public ParametricUV
{
public:
    float3 getVertexPosition(float2 uv);
};

float3 Plane::getVertexPosition(float2 uv)
{
    return { -1 + uv.X * 2, sinf(uv.X * 20.0f) * 0.25f, -1 + uv.Y * 2 };
}

class ParametricUVToMesh
{
public:
    ParametricUVToMesh(std::shared_ptr<ParametricUV> shape, uint32_t stepsInU, uint32_t stepsInV);
    uint32_t getVertexCount();
    uint32_t getIndexCount();
    void copyVertices(float3* to);
    void copyIndices(uint32_t* to);
private:
    std::shared_ptr<ParametricUV> m_shape;
    uint32_t m_stepsInU;
    uint32_t m_stepsInV;
};

ParametricUVToMesh::ParametricUVToMesh(std::shared_ptr<ParametricUV> shape, uint32_t stepsInU, uint32_t stepsInV) :
    m_shape(shape),
    m_stepsInU(stepsInU),
    m_stepsInV(stepsInV)
{
}

uint32_t ParametricUVToMesh::getVertexCount()
{
    return (m_stepsInU + 1) * (m_stepsInV + 1);
}

uint32_t ParametricUVToMesh::getIndexCount()
{
    return 3 * 2 * m_stepsInU * m_stepsInV;
}

void ParametricUVToMesh::copyVertices(float3* to)
{
    for (int32_t v = 0; v <= m_stepsInV; ++v)
    {
        for (int32_t u = 0; u <= m_stepsInU; ++u)
        {
            *(to++) = m_shape->getVertexPosition({(float)u / m_stepsInU, (float)v / m_stepsInV });
        }
    }
}

void ParametricUVToMesh::copyIndices(uint32_t* to)
{
    for (int32_t v = 0; v < m_stepsInV; ++v)
    {
        for (int32_t u = 0; u < m_stepsInU; ++u)
        {
            *(to++) = (u + 0) + (v + 0) * (m_stepsInU + 1);
            *(to++) = (u + 0) + (v + 1) * (m_stepsInU + 1);
            *(to++) = (u + 1) + (v + 1) * (m_stepsInU + 1);
            *(to++) = (u + 0) + (v + 0) * (m_stepsInU + 1);
            *(to++) = (u + 1) + (v + 1) * (m_stepsInU + 1);
            *(to++) = (u + 1) + (v + 0) * (m_stepsInU + 1);
        }
    }
}

class Sample_DXR3D : public Sample_DXRBase
{
private:
    CComPtr<ID3D12StateObject> m_pPipelineStateObject;
public:
    Sample_DXR3D(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice) :
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

            const WCHAR* shaderExports[] = { L"RayGeneration", L"Miss", L"HitGroup", L"HitGroup2" };
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
            descHitGroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            descHitGroup.ClosestHitShaderImport = L"MaterialCheckerboard";
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            descSubobject[setupSubobject].pDesc = &descHitGroup;
            ++setupSubobject;

            D3D12_HIT_GROUP_DESC descHitGroup2 = {};
            descHitGroup2.HitGroupExport = L"HitGroup2";
            descHitGroup2.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
            descHitGroup2.ClosestHitShaderImport = L"MaterialRedPlastic";
            descSubobject[setupSubobject].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            descSubobject[setupSubobject].pDesc = &descHitGroup2;
            ++setupSubobject;

            D3D12_STATE_OBJECT_DESC descStateObject = {};
            descStateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
            descStateObject.NumSubobjects = setupSubobject;
            descStateObject.pSubobjects = &descSubobject[0];
            TRYD3D(m_pDevice->GetID3D12Device()->CreateStateObject(&descStateObject, __uuidof(ID3D12StateObject), (void**)&m_pPipelineStateObject));
            m_pPipelineStateObject->SetName(L"DXR Pipeline State");
        }
    }
    void Render() override
    {
        RunOnGPU(m_pDevice, [&](ID3D12GraphicsCommandList* pD3D12GraphicsCommandList)
        {
            ////////////////////////////////////////////////////////////////////////////////
            // Create some simple geometry.
            ////////////////////////////////////////////////////////////////////////////////
            std::shared_ptr<ParametricUV> shape(new Plane());
            std::shared_ptr<ParametricUVToMesh> mesh(new ParametricUVToMesh(shape, 100, 100));
            CComPtr<ID3D12Resource> Vertices;
            CComPtr<ID3D12Resource> Indices;
            {
                {
                    int sizeVertex = sizeof(float[3]) * mesh->getVertexCount();
                    std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
                    mesh->copyVertices(reinterpret_cast<float3*>(vertices.get()));
                    Vertices.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeVertex, sizeVertex, vertices.get());
                }
                {
                    int sizeIndices = sizeof(int32_t) * mesh->getIndexCount();
                    std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
                    mesh->copyIndices(reinterpret_cast<uint32_t*>(indices.get()));
                    Indices.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeIndices, sizeIndices, indices.get());
                }
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
                descGeometry[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
                descGeometry[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
                descGeometry[0].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
                descGeometry[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                descGeometry[0].Triangles.IndexCount = mesh->getIndexCount();
                descGeometry[0].Triangles.VertexCount = mesh->getVertexCount();
                descGeometry[0].Triangles.IndexBuffer = Indices->GetGPUVirtualAddress();
                descGeometry[0].Triangles.VertexBuffer.StartAddress = Vertices->GetGPUVirtualAddress();
                descGeometry[0].Triangles.VertexBuffer.StrideInBytes = sizeof(float[3]);
                descRaytracingInputs.pGeometryDescs = &descGeometry[0];
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
                D3D12_RAYTRACING_INSTANCE_DESC DxrInstance[1] = {};
                DxrInstance[0].Transform[0][0] = 1;
                DxrInstance[0].Transform[1][1] = 1;
                DxrInstance[0].Transform[2][2] = 1;
                DxrInstance[0].InstanceMask = 0xFF;
                DxrInstance[0].AccelerationStructure = ResourceBLAS->GetGPUVirtualAddress();
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
                memcpy(&shaderTableCPU[shaderEntrySize * 2], stateObjectProperties->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 2] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
                // Shader Index 3 - Hit Shader 2
                memcpy(&shaderTableCPU[shaderEntrySize * 3], stateObjectProperties->GetShaderIdentifier(L"HitGroup2"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(&shaderTableCPU[shaderEntrySize * 3] + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = m_pDevice->GetID3D12DescriptorHeapCBVSRVUAV()->GetGPUDescriptorHandleForHeapStart();
                ResourceShaderTable.p = D3D12CreateBuffer(m_pDevice, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, shaderTableSize, shaderTableSize, &shaderTableCPU[0]);
                ResourceShaderTable->SetName(L"DXR Shader Table");
            }
            ////////////////////////////////////////////////////////////////////////////////
            // Get the next available backbuffer.
            ////////////////////////////////////////////////////////////////////////////////
            CComPtr<ID3D12Resource> pD3D12Resource;
            TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12Resource));
            pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
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
                {
                    D3D12_RESOURCE_BARRIER descResourceBarrier = {};
                    descResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    descResourceBarrier.Transition.pResource = m_pResourceTargetUAV;
                    descResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                    descResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
                    descResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    RaytraceCommandList->ResourceBarrier(1, &descResourceBarrier);
                }
                {
                    D3D12_RESOURCE_BARRIER descResourceBarrier = {};
                    descResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    descResourceBarrier.Transition.pResource = pD3D12Resource;
                    descResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                    descResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                    descResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    RaytraceCommandList->ResourceBarrier(1, &descResourceBarrier);
                }
                RaytraceCommandList->CopyResource(pD3D12Resource, m_pResourceTargetUAV);
                {
                    D3D12_RESOURCE_BARRIER descResourceBarrier = {};
                    descResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    descResourceBarrier.Transition.pResource = m_pResourceTargetUAV;
                    descResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
                    descResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
                    descResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    RaytraceCommandList->ResourceBarrier(1, &descResourceBarrier);
                }
                {
                    D3D12_RESOURCE_BARRIER descResourceBarrier = {};
                    descResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    descResourceBarrier.Transition.pResource = pD3D12Resource;
                    descResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                    descResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
                    descResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    RaytraceCommandList->ResourceBarrier(1, &descResourceBarrier);
                }
            });
        });
        // Swap the backbuffer and send this to the desktop composer for display.
        CComPtr<ID3D12Resource> pD3D12Resource;
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(m_pSwapChain->GetIDXGISwapChain()->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), (void**)&pD3D12Resource));
        pD3D12Resource->SetName(L"D3D12Resource (Backbuffer)");
        TRYD3D(m_pSwapChain->GetIDXGISwapChain()->Present(0, 0));
    }
};

std::shared_ptr<Sample> CreateSample_DXR3D(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D12Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_DXR3D(pSwapChain, pDevice));
}