#include "Core_D3D.h"
#include "Core_D3D12Util.h"
#include "Core_DXRUtil.h"
#include <array>

CComPtr<ID3D12RootSignature> DXR_Create_Signature_1UAV1SRV(ID3D12Device* device)
{
    std::array<D3D12_DESCRIPTOR_RANGE, 3> descDescriptorRange;

    descDescriptorRange[0].BaseShaderRegister = 0;
    descDescriptorRange[0].NumDescriptors = 1;
    descDescriptorRange[0].RegisterSpace = 0;
    descDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    descDescriptorRange[1].BaseShaderRegister = 0;
    descDescriptorRange[1].NumDescriptors = 1;
    descDescriptorRange[1].RegisterSpace = 0;
    descDescriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descDescriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    descDescriptorRange[2].BaseShaderRegister = 0;
    descDescriptorRange[2].NumDescriptors = 1;
    descDescriptorRange[2].RegisterSpace = 0;
    descDescriptorRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descDescriptorRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    std::array<D3D12_ROOT_PARAMETER, 2> descRootParameter = {};
    descRootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    descRootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descRootParameter[0].DescriptorTable.NumDescriptorRanges = 3;
    descRootParameter[0].DescriptorTable.pDescriptorRanges = &descDescriptorRange[0];

    descRootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    descRootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descRootParameter[1].Constants.ShaderRegister = 1;
    descRootParameter[1].Constants.Num32BitValues = 4;

    D3D12_ROOT_SIGNATURE_DESC descSignature = {};
    descSignature.NumParameters = 2;
    descSignature.pParameters = &descRootParameter[0];
    descSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    CComPtr<ID3DBlob> m_blob;
    CComPtr<ID3DBlob> m_blobError;
    TRYD3D(D3D12SerializeRootSignature(&descSignature, D3D_ROOT_SIGNATURE_VERSION_1_0, &m_blob, &m_blobError));
    CComPtr<ID3D12RootSignature> pRootSignature;
    TRYD3D(device->CreateRootSignature(0, m_blob->GetBufferPointer(), m_blob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pRootSignature.p));
    pRootSignature->SetName(L"DXR Root Signature");
    return pRootSignature;
}

// Create a standard output UAV of the correct pixel format and sized to our default resolution.
CComPtr<ID3D12Resource1> DXR_Create_Output_UAV(ID3D12Device* device)
{
    D3D12_HEAP_PROPERTIES descHeapProperties = {};
    descHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC descResource = {};
    descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    descResource.Width = RENDERTARGET_WIDTH;
    descResource.Height = RENDERTARGET_HEIGHT;
    descResource.DepthOrArraySize = 1;
    descResource.MipLevels = 1;
    descResource.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    descResource.SampleDesc.Count = 1;
    descResource.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    CComPtr<ID3D12Resource1> pResource;
    TRYD3D(device->CreateCommittedResource(&descHeapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &descResource, D3D12_RESOURCE_STATE_COMMON, nullptr, __uuidof(ID3D12Resource1), (void**)&pResource));
    pResource->SetName(L"DXR Output Texture2D UAV");
    return pResource;
}

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

D3D12_RAYTRACING_AABB Make_D3D12_RAYTRACING_AABB(FLOAT minX, FLOAT minY, FLOAT minZ, float maxX, float maxY, float maxZ)
{
    D3D12_RAYTRACING_AABB o = {};
    o.MinX = minX;
    o.MinY = minY;
    o.MinZ = minZ;
    o.MaxX = maxX;
    o.MaxY = maxY;
    o.MaxZ = maxZ;
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

CComPtr<ID3D12Resource1> DXRCreateBLAS(Direct3D12Device* device, const void* vertices, int vertexCount, DXGI_FORMAT vertexFormat, const void* indices, int indexCount, DXGI_FORMAT indexFormat)
{
    // The GPU is doing the actual acceleration structure building work so we need all the mesh data up on GPU first.
    CComPtr<ID3D12Resource1> ResourceVertices = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, DXGIFormatSize(vertexFormat) * vertexCount, DXGIFormatSize(vertexFormat) * vertexCount, vertices);
    CComPtr<ID3D12Resource1> ResourceIndices = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, DXGIFormatSize(indexFormat) * indexCount, DXGIFormatSize(indexFormat) * indexCount, indices);
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
        ResourceBLAS = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        CComPtr<ID3D12Resource1> ResourceASScratch = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        // Build the acceleration structure.
        RunOnGPU(device, [&](ID3D12GraphicsCommandList5* UploadBLASCommandList) {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
            descBuild.DestAccelerationStructureData = ResourceBLAS->GetGPUVirtualAddress();
            descBuild.Inputs = descRaytracingInputs;
            descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
            UploadBLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
            });
    }
    return ResourceBLAS;
}

CComPtr<ID3D12Resource1> DXRCreateTLAS(Direct3D12Device* device, const D3D12_RAYTRACING_INSTANCE_DESC* instances, int instanceCount)
{
    ////////////////////////////////////////////////////////////////////////////////
    // INSTANCE - Create the instancing table.
    ////////////////////////////////////////////////////////////////////////////////
    CComPtr<ID3D12Resource1> ResourceInstance = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, instances);
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
        ResourceTLAS = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        CComPtr<ID3D12Resource1> ResourceASScratch;
        ResourceASScratch = D3D12CreateBuffer(device, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, descRaytracingPrebuild.ResultDataMaxSizeInBytes);
        // Build the acceleration structure.
        RunOnGPU(device, [&](ID3D12GraphicsCommandList4* UploadTLASCommandList) {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC descBuild = {};
            descBuild.DestAccelerationStructureData = ResourceTLAS->GetGPUVirtualAddress();
            descBuild.Inputs = descRaytracingInputs;
            descBuild.ScratchAccelerationStructureData = ResourceASScratch->GetGPUVirtualAddress();
            UploadTLASCommandList->BuildRaytracingAccelerationStructure(&descBuild, 0, nullptr);
        });
    }
    return ResourceTLAS;
}