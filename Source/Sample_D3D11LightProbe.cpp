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
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Sample.h"
#include "Scene_Camera.h"
#include <atlbase.h>
#include <cstdint>
#include <memory>

#include <fstream>

class Sample_D3D11LightProbe : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
    CComPtr<ID3D11ComputeShader> m_pComputeShader;
    CComPtr<ID3D11Buffer> m_pBufferConstants;
    CComPtr<ID3D11Texture2D> m_pTex2DImage;
    CComPtr<ID3D11ShaderResourceView> m_pSRVImage;
public:
    Sample_D3D11LightProbe(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device) :
        m_pSwapChain(swapchain),
        m_pDevice(device)
    {
        // Create a compute shader.
        CComPtr<ID3DBlob> pD3DBlobCodeCS = CompileShader("cs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 TransformClipToWorld;
};

RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
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
    // Calculate the longitude and latitude of the view direction.
    const float PI = 3.1415926535897932384626433832795029;
    direction.y = -direction.y;
    direction.z = -direction.z;
    float r = (1 / PI) * acos(direction.z) / sqrt(direction.x * direction.x + direction.y * direction.y);
    float2 uv = float2((direction.x * r + 1) / 2, (direction.y * r + 1) / 2);
    renderTarget[dispatchThreadId.xy] = userImage.Load(float3(uv.x * 1000, uv.y * 1000, 0)) * 10;
})SHADER");
        m_pBufferConstants = D3D11_Create_Buffer(m_pDevice->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
        {
            const int imageWidth = 1000;
            const int imageHeight = 1000;
            struct Pixel { float B, G, R; };
            const uint32_t imageStride = sizeof(Pixel) * imageWidth;
            std::ifstream readit("C:\\_\\Probes\\grace_probe.float", std::ios::binary);
            std::unique_ptr<char> buffer(new char[imageStride * imageHeight]);
            for (int y = 0; y < imageHeight; ++y)
            {
                char* raster = reinterpret_cast<char*>(buffer.get()) + imageStride * (imageHeight - y - 1);
                readit.read(raster, imageStride);
            }
            m_pTex2DImage = D3D11_Create_Texture2D(m_pDevice->GetID3D11Device(), DXGI_FORMAT_R32G32B32_FLOAT, imageWidth, imageHeight, buffer.get());
        }
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = 1;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(m_pTex2DImage, &desc, &m_pSRVImage.p));
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
        m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(m_pBufferConstants, 0, nullptr, &Invert(GetCameraWorldToClip()), 0, 0);
        // Beginning of rendering.
        m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &pUAVTarget.p, nullptr);
        m_pDevice->GetID3D11DeviceContext()->CSSetShader(m_pComputeShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1, &m_pBufferConstants.p);
        m_pDevice->GetID3D11DeviceContext()->CSSetShaderResources(0, 1, &m_pSRVImage.p);
        m_pDevice->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1);
        m_pDevice->GetID3D11DeviceContext()->ClearState();
        m_pDevice->GetID3D11DeviceContext()->Flush();
        
        // End of rendering; send to display.
        m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    }
};

std::shared_ptr<Sample> CreateSample_D3D11LightProbe(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device)
{
    return std::shared_ptr<Sample>(new Sample_D3D11LightProbe(swapchain, device));
}