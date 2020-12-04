///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Ray March
///////////////////////////////////////////////////////////////////////////////
// This sample shows how to perform a ray march to render a "meta-ball" type
// implicit shape.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Scene_Camera.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

std::function<void(ID3D11UnorderedAccessView *)>
CreateSample_D3D11RayMarch(std::shared_ptr<Direct3D11Device> device) {
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 TransformClipToWorld;
    float Time;
};

RWTexture2D<float4> renderTarget;

float lambert(float3 normal)
{
    const float3 light = normalize(float3(1, 1, -1));
    return max(0, dot(normal, light));
}

float phong(float3 reflect, float power)
{
    const float3 light = normalize(float3(1, 1, -1));
    return pow(max(0, dot(light, reflect)), power);
}

float SignedDistanceFunctionBlob(float3 position)
{
    position.y -= 1;
    float distance = sqrt(dot(position, position)) - 1;
    distance = distance + sin(position.x * 10 + Time * 2) * 0.05;
    distance = distance + sin(position.y * 10 + Time * 3) * 0.05;
    distance = distance + sin(position.z * 10 + Time * 5) * 0.05;
    distance /= 2;
    return distance;
}

float SignedDistanceFunctionPlane(float3 position)
{
    return position.y;
}

float SignedDistanceFunction(float3 position)
{
    return min(SignedDistanceFunctionPlane(position), SignedDistanceFunctionBlob(position));
}

bool IsHit(float3 origin, float3 distance)
{
    const float EPSILON = 0.01;
    float walk = 0;
    for (int i = 0; i < 1000; ++i)
    {
        float sdf = SignedDistanceFunction(origin + distance * walk);
        if (walk > 100) return false;
        if (sdf < EPSILON) return true;
        walk = walk + sdf;
    }
    return false;
}

float3 CalculateNormal(float3 position)
{
    const float EPSILON = 0.01;
    float deltax = SignedDistanceFunction(float3(position.x + EPSILON, position.y, position.z)) - SignedDistanceFunction(float3(position.x - EPSILON, position.y, position.z));
    float deltay = SignedDistanceFunction(float3(position.x, position.y + EPSILON, position.z)) - SignedDistanceFunction(float3(position.x, position.y - EPSILON, position.z));
    float deltaz = SignedDistanceFunction(float3(position.x, position.y, position.z + EPSILON)) - SignedDistanceFunction(float3(position.x, position.y, position.z - EPSILON));
    return normalize(float3(deltax, deltay, deltaz));
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const float EPSILON = 0.001;
    const float3 light = normalize(float3(1, 1, -1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form up normalized screen coordinates.
    const float2 Normalized = mad(float2(2, -2) / float2(640, 480), float2(dispatchThreadId.xy), float2(-1, 1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form the world ray.
    float4 front = mul(TransformClipToWorld, float4(Normalized.xy, 0, 1));
    front /= front.w;
    float4 back = mul(TransformClipToWorld, float4(Normalized.xy, 1, 1));
    back /= back.w;
    float3 origin = front.xyz;
    float3 direction = normalize(back.xyz - front.xyz);
    ////////////////////////////////////////////////////////////////////////////////
    // Walk the ray
    float walk = 0;
    for (int i = 0; i < 1000; ++i)
    {
        float3 position = origin + direction * walk;
        float sdf = SignedDistanceFunction(position);
        if (walk > 100) break;
        if (sdf < EPSILON)
        {
            float3 normal = CalculateNormal(position);
            float3 illum = lambert(normal) + phong(reflect(direction, normal), 64);
            if (IsHit(position + normal * 0.08, light))
            {
                renderTarget[dispatchThreadId.xy] = float4(illum, 1) * 0.6;
                return;
            }
            renderTarget[dispatchThreadId.xy] = float4(illum, 1);
            return;
        }
        walk = walk + sdf;
    }
    renderTarget[dispatchThreadId.xy] = float4(0.4, 0.2, 0.8, 1);
})SHADER");
    TRYD3D(device->GetID3D11Device()->CreateComputeShader(
        blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
        &shaderCompute));
  }
  __declspec(align(16)) struct Constants {
    Matrix44 TransformClipToWorld;
    float Time;
  };
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
  return [=](ID3D11UnorderedAccessView *uavBackbuffer) {
    device->GetID3D11DeviceContext()->ClearState();
    // Upload the constant buffer.
    {
      static float t = 0;
      Constants constants;
      constants.TransformClipToWorld = Invert(GetCameraWorldToClip());
      constants.Time = t;
      device->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, &constants, 0, 0);
      t += 0.01;
    }
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &uavBackbuffer, nullptr);
    device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
    device->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH,
                                               RENDERTARGET_HEIGHT, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}