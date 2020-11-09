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