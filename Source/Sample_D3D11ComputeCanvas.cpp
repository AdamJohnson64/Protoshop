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
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Sample.h"
#include <atlbase.h>
#include <cstdint>
#include <memory>

const int IMAGE_WIDTH = 320;
const int IMAGE_HEIGHT = 200;

class Sample_D3D11ComputeCanvas : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
    CComPtr<ID3D11ComputeShader> m_pComputeShader;
    CComPtr<ID3D11Buffer> m_pBufferConstants;
    CComPtr<ID3D11Texture2D> m_pTex2DImage;
    CComPtr<ID3D11ShaderResourceView> m_pSRVImage;
public:
    Sample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device) :
        m_pSwapChain(swapchain),
        m_pDevice(device)
    {
        // Create a compute shader.
        CComPtr<ID3DBlob> pD3DBlobCodeCS = CompileShader("cs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 TransformPixelToImage;
};

RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

float4 Checkerboard(uint2 pixel)
{
    float modulo = ((uint)pixel.x / 32 + (uint)pixel.y / 32) % 2;
    return lerp(float4(0.25, 0.25, 0.25, 1), float4(0.75, 0.75, 0.75, 1), modulo);
}

float4 Sample(float2 pixel)
{
    float4 checkerboard = Checkerboard((uint2)pixel);
    float2 pos = mul(TransformPixelToImage, float4(pixel.x, pixel.y, 0, 1)).xy;
    uint Width, Height, NumberOfLevels;
    userImage.GetDimensions(0, Width, Height, NumberOfLevels);
    if (pos.x >= 0 && pos.x < (int)Width && pos.y >= 0 && pos.y < (int)Height)
    {
        return userImage.Load(int3(pos.x, pos.y, 0));
    }
    else
    {
        return checkerboard;
    }
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ////////////////////////////////////////////////////////////////////////////////
    // No Supersampling.
    renderTarget[dispatchThreadId.xy] = Sample((float2)dispatchThreadId);
    return;
    
    ////////////////////////////////////////////////////////////////////////////////
    // 8x8 (64 TAP) Supersampling.
    const int superCountX = 8;
    const int superCountY = 8;
    const float superSpacingX = 1.0 / superCountX;
    const float superSpacingY = 1.0 / superCountY;
    const float superOffsetX = superSpacingX / 2;
    const float superOffsetY = superSpacingY / 2;
    float4 accumulateSamples;
    for (int superSampleY = 0; superSampleY < superCountY; ++superSampleY)
    {
        for (int superSampleX = 0; superSampleX < superCountX; ++superSampleX)
        {
            accumulateSamples += Sample(float2(
                dispatchThreadId.x + superOffsetX + superSampleX * superSpacingX,
                dispatchThreadId.y + superOffsetY + superSampleY * superSpacingY));
        }
    }
    accumulateSamples /= superCountX * superCountY;
    renderTarget[dispatchThreadId.xy] = accumulateSamples;
})SHADER");
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = 1024;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.StructureByteStride = sizeof(Matrix44);
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&desc, nullptr, &m_pBufferConstants));
        }
        {
            struct Pixel { uint8_t B, G, R, A; };
            const uint32_t imageStride = 4 * IMAGE_WIDTH;
            Pixel imageRaw[IMAGE_WIDTH * IMAGE_HEIGHT];
            Image_Fill_Commodore64(imageRaw, IMAGE_WIDTH, IMAGE_HEIGHT, imageStride);
            {
                D3D11_TEXTURE2D_DESC desc = {};
                desc.Width = IMAGE_WIDTH;
                desc.Height = IMAGE_HEIGHT;
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_IMMUTABLE;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                D3D11_SUBRESOURCE_DATA descData = {};
                descData.pSysMem = imageRaw;
                descData.SysMemPitch = sizeof(Pixel) * IMAGE_WIDTH;
                descData.SysMemSlicePitch = sizeof(Pixel) * IMAGE_WIDTH * IMAGE_HEIGHT;
                TRYD3D(m_pDevice->GetID3D11Device()->CreateTexture2D(&desc, &descData, &m_pTex2DImage.p));
            }
        }
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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
        // Upload the constant buffer.
        static float angle = 0;
        const float zoom = 2 + cosf(angle);
        Matrix44 rotate = {};
        rotate.M11 = rotate.M22 = rotate.M33 = rotate.M44 = 1;
        rotate.M11 = cosf(angle); rotate.M12 = sinf(angle);
        rotate.M21 = -sinf(angle); rotate.M22 = cosf(angle);
        Matrix44 transformImageToPixel =
            CreateMatrixTranslate(Vector3 { -IMAGE_WIDTH / 2, -IMAGE_HEIGHT / 2, 0 })
            * CreateMatrixScale(Vector3 { zoom, zoom, 1 })
            * rotate
            * CreateMatrixTranslate(Vector3 { RENDERTARGET_WIDTH / 2, RENDERTARGET_HEIGHT / 2, 0 });
        angle += 0.01f;
        m_pDevice->GetID3D11DeviceContext()->ClearState();
        m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(m_pBufferConstants, 0, nullptr, &Invert(transformImageToPixel), 0, 0);
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

std::shared_ptr<Sample> CreateSample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device)
{
    return std::shared_ptr<Sample>(new Sample_D3D11ComputeCanvas(swapchain, device));
}