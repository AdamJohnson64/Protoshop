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

CComPtr<ID3D12Resource> DXRCreateTLAS(std::shared_ptr<Direct3D12Device> device, const D3D12_RAYTRACING_INSTANCE_DESC* instances, int instanceCount)
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