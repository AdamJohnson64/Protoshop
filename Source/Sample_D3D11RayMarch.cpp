///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Compute Canvas
///////////////////////////////////////////////////////////////////////////////
// This sample renders to the display using only compute shaders and no
// rasterization. For interest we create a transformable display that would
// theoretically support a painting application. Texture access is handled via
// direct memory access without samplers or texture objects.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "ImageUtil.h"
#include "Sample.h"
#include "Scene_Camera.h"
#include <atlbase.h>
#include <cstdint>
#include <memory>

class Sample_D3D11RayMarch : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
    CComPtr<ID3D11ComputeShader> m_pComputeShader;
    CComPtr<ID3D11Buffer> m_pBufferConstants;
public:
    Sample_D3D11RayMarch(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        m_pSwapChain(pSwapChain),
        m_pDevice(pDevice)
    {
        // Create a compute shader.
        CComPtr<ID3DBlob> pD3DBlobCodeCS = CompileShader("cs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 Transform;
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
    float4 front = mul(Transform, float4(Normalized.xy, 0, 1));
    front /= front.w;
    float4 back = mul(Transform, float4(Normalized.xy, 1, 1));
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
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = 1024;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.StructureByteStride = sizeof(Matrix44);
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&desc, nullptr, &m_pBufferConstants));
        }
        TRYD3D(m_pDevice->GetID3D11Device()->CreateComputeShader(pD3DBlobCodeCS->GetBufferPointer(), pD3DBlobCodeCS->GetBufferSize(), nullptr, &m_pComputeShader));
    }
    void Render()
    {
        // Get the backbuffer and create a render target from it.
        CComPtr<ID3D11UnorderedAccessView> pUAVTarget;
        {
            CComPtr<ID3D11Texture2D> pD3D11Texture2D;
            TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D));
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateUnorderedAccessView(pD3D11Texture2D, &uavDesc, &pUAVTarget));
        }
        m_pDevice->GetID3D11DeviceContext()->ClearState();
        // Upload the constant buffer.
        {
            static float t = 0;
            struct Constants
            {
                Matrix44 Transform;
                float Time;
            };
            Constants constants;
            constants.Transform = Invert(GetCameraViewProjection());
            constants.Time = t;
            m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(m_pBufferConstants, 0, nullptr, &constants, 0, 0);
            t += 0.01;
        }
        // Beginning of rendering.
        m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &pUAVTarget.p, nullptr);
        m_pDevice->GetID3D11DeviceContext()->CSSetShader(m_pComputeShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1, &m_pBufferConstants.p);
        m_pDevice->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1);
        m_pDevice->GetID3D11DeviceContext()->ClearState();
        m_pDevice->GetID3D11DeviceContext()->Flush();
        
        // End of rendering; send to display.
        m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    }
};

std::shared_ptr<Sample> CreateSample_D3D11RayMarch(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11RayMarch(pSwapChain, pDevice));
}