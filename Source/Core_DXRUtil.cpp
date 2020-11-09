#include "Core_D3D12Util.h"
#include "Core_DXRUtil.h"

D3D12_RAYTRACING_INSTANCE_DESC Make_D3D12_RAYTRACING_INSTANCE_DESC(const Matrix44 &transform, int hitgroup, D3D12_GPU_VIRTUAL_ADDRESS tlas)
{
    D3D12_RAYTRACING_INSTANCE_DESC o = {};
    o.Transform[0][0] = transform.M11;
    o.Transform[1][0] = transform.M12;
    o.Transform[2][0] = transform.M13;
    o.Transform[0][1] = transform.M21;
    o.Transform[1][1] = transform.M22;
    o.Transform[2][1] = transform.M23;
    o.Transform[0][2] = transform.M31;
    o.Transform[1][2] = transform.M32;
    o.Transform[2][2] = transform.M33;
    o.Transform[0][3] = transform.M41;
    o.Transform[1][3] = transform.M42;
    o.Transform[2][3] = transform.M43;
    o.InstanceMask = 0xFF;
    o.InstanceContributionToHitGroupIndex = hitgroup;
    o.AccelerationStructure = tlas;
    return o;
}

static int DXGIFormatSize(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32_FLOAT: return sizeof(float[3]);
    case DXGI_FORMAT_R32G32_FLOAT: return sizeof(float[2]);
    case DXGI_FORMAT_R32_UINT: return sizeof(unsigned int);
    default: throw std::exception("Cannot determine byte size of pixel/vertex format.");
    }
}

ID3D12Resource1* DXRCreateBLAS(std::shared_ptr<Direct3D12Device> device, const void* vertices, int vertexCount, DXGI_FORMAT vertexFormat, const void* indices, int indexCount, DXGI_FORMAT indexFormat)
{
    // The GPU is doing the actual acceleration structure building work so we need all the mesh data up on GPU first.
    CComPtr<ID3D12Resource1> ResourceVertices;
    ResourceVertices.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, DXGIFormatSize(vertexFormat) * vertexCount, DXGIFormatSize(vertexFormat) * vertexCount, vertices);
    CComPtr<ID3D12Resource1> ResourceIndices;
    ResourceIndices.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, DXGIFormatSize(indexFormat) * indexCount, DXGIFormatSize(indexFormat) * indexCount, indices);
    // Now create and initialize the BLAS with this data.
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
        descGeometry.Triangles.IndexFormat = indexFormat;
        descGeometry.Triangles.VertexFormat = vertexFormat;
        descGeometry.Triangles.IndexCount = indexCount;
        descGeometry.Triangles.VertexCount = vertexCount;
        descGeometry.Triangles.IndexBuffer = ResourceIndices->GetGPUVirtualAddress();
        descGeometry.Triangles.VertexBuffer.StartAddress = ResourceVertices->GetGPUVirtualAddress();
        descGeometry.Triangles.VertexBuffer.StrideInBytes = DXGIFormatSize(vertexFormat);
        descRaytracingInputs.pGeometryDescs = &descGeometry;
        device->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
        // Create the output and scratch buffers.
        ResourceBLAS.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        CComPtr<ID3D12Resource1> ResourceASScratch;
        ResourceASScratch.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        // Build the acceleration structure.
        RunOnGPU(device, [&](ID3D12GraphicsCommandList5* UploadBLASCommandList) {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
            descBuild.DestAccelerationStructureData = ResourceBLAS->GetGPUVirtualAddress();
            descBuild.Inputs = descRaytracingInputs;
            descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
            UploadBLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
            });
    }
    return ResourceBLAS.Detach();
}

ID3D12Resource1* DXRCreateTLAS(std::shared_ptr<Direct3D12Device> device, const D3D12_RAYTRACING_INSTANCE_DESC* instances, int instanceCount)
{
    ////////////////////////////////////////////////////////////////////////////////
    // INSTANCE - Create the instancing table.
    ////////////////////////////////////////////////////////////////////////////////
    CComPtr<ID3D12Resource1> ResourceInstance;
    ResourceInstance.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, instances);
    ////////////////////////////////////////////////////////////////////////////////
    // TLAS - Build the top level acceleration structure.
    ////////////////////////////////////////////////////////////////////////////////
    CComPtr<ID3D12Resource1> ResourceTLAS;
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO descRaytracingPrebuild = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS descRaytracingInputs = {};
        descRaytracingInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        descRaytracingInputs.NumDescs = instanceCount;
        descRaytracingInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        descRaytracingInputs.InstanceDescs = ResourceInstance->GetGPUVirtualAddress();
        device->m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&descRaytracingInputs, &descRaytracingPrebuild);
        // Create the output and scratch buffers.
        ResourceTLAS.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        CComPtr<ID3D12Resource1> ResourceASScratch;
        ResourceASScratch.p = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        // Build the acceleration structure.
        RunOnGPU(device, [&](ID3D12GraphicsCommandList4* UploadTLASCommandList) {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
            descBuild.DestAccelerationStructureData = ResourceTLAS->GetGPUVirtualAddress();
            descBuild.Inputs = descRaytracingInputs;
            descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
            UploadTLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
        });
    }
    return ResourceTLAS.Detach();
}