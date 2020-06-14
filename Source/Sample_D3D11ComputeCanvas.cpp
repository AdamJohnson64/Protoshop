#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Sample.h"
#include <atlbase.h>
#include <cstdint>
#include <memory>

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240

class Sample_D3D11ComputeCanvas : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
    CComPtr<ID3D11ComputeShader> m_pD3D11ComputeShader;
    CComPtr<ID3D11Buffer> m_pD3D11BufferConstants;
    CComPtr<ID3D11Buffer> m_pD3D11BufferImage;
    CComPtr<ID3D11UnorderedAccessView> m_pD3D11UnorderedAccessViewImage;
    struct Constants
    {
        matrix44 Transform;
        int Width, Height;
    };
public:
    Sample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        m_pSwapChain(pSwapChain),
        m_pDevice(pDevice)
    {
        // Create a compute shader.
        CComPtr<ID3DBlob> pD3DBlobCodeCS = CompileShader("cs_5_0", R"SHADER(
cbuffer Constants
{
    float4x4 transform;
    int width;
    int height;
};

RWTexture2D<float4> renderTarget;
RWBuffer<uint> buffer;

float4 Checkerboard(uint2 pixel)
{
    float modulo = ((uint)pixel.x / 32 + (uint)pixel.y / 32) % 2;
    return lerp(float4(0.25, 0.25, 0.25, 1), float4(0.75, 0.75, 0.75, 1), modulo);
}

float4 unpackPixel(uint p)
{
    float4 result;
    result.r = ((p >> 0) & 0xFF) / 255.0f;
    result.g = ((p >> 8) & 0xFF) / 255.0f;
    result.b = ((p >> 16) & 0xFF) / 255.0f;
    result.a = ((p >> 24) & 0xFF) / 255.0f;
    return result;
}

float4 UserImage(float2 pixel)
{
    int2 iPixel = (int2)pixel;
    return unpackPixel(buffer[iPixel.x + iPixel.y * width]);    

    float2 fPixel = (float2)pixel - float2(128, 128);
    float dist = sqrt(dot(fPixel, fPixel));
    return (dist > 96 && dist < 112) ? float4(1, 0, 0, 0) : float4(0, 0, 0, 0);
    
    //return float4(1, 0, 0, 1);
}

float4 Sample(float2 pixel)
{
    float4 checkerboard = Checkerboard((uint2)pixel);
    float2 pos = mul(float4(pixel.x, pixel.y, 0, 1), transform).xy;
    if (pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height)
    {
        return UserImage((float2)pos);
    }
    else
    {
        return checkerboard;
    }
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    renderTarget[dispatchThreadId.xy] = Sample((float2)dispatchThreadId);
    return;
    float4 accumulateSamples;
    for (int superSampleY = 0; superSampleY < 4; ++superSampleY)
    {
        for (int superSampleX = 0; superSampleX < 4; ++superSampleX)
        {
            accumulateSamples += Sample(float2(dispatchThreadId.x + 0.125 + superSampleX * 0.25, dispatchThreadId.y + 0.125 + superSampleY * 0.25));
        }
    }
    accumulateSamples /= 16;
    renderTarget[dispatchThreadId.xy] = accumulateSamples;
})SHADER");
        {
            D3D11_BUFFER_DESC descBuffer = {};
            descBuffer.ByteWidth = 1024;
            descBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            descBuffer.StructureByteStride = sizeof(Constants);
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&descBuffer, nullptr, &m_pD3D11BufferConstants));
        }
        {
            D3D11_BUFFER_DESC descBuffer = {};
            descBuffer.ByteWidth = sizeof(uint32_t) * IMAGE_WIDTH * IMAGE_HEIGHT;
            descBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            descBuffer.StructureByteStride = sizeof(uint32_t);
            uint32_t data[IMAGE_WIDTH * IMAGE_HEIGHT];
            char stamp[] =
            {
                0, 0, 0, 1, 1, 0, 0, 0,
                0, 0, 1, 0, 0, 1, 0, 0,
                0, 1, 0, 0, 0, 0, 1, 0,
                0, 1, 0, 0, 0, 0, 1, 0,
                0, 1, 1, 1, 1, 1, 1, 0,
                0, 1, 0, 0, 0, 0, 1, 0,
                0, 1, 0, 0, 0, 0, 1, 0,
                0, 1, 0, 0, 0, 0, 1, 0,
            };
            for (int a = 0; a < IMAGE_WIDTH * IMAGE_HEIGHT; ++a)
            {
                data[a] = 0xFF0080FF + a;
            }
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    if (stamp[x + y * 8])
                    {
                        data[(16 + x) + (16 + y) * IMAGE_WIDTH] = 0xFFFF0000;
                    }
                }
            }
            D3D11_SUBRESOURCE_DATA descData = {};
            descData.pSysMem = data;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&descBuffer, &descData, &m_pD3D11BufferImage));
        }
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
            descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            descUAV.Format = DXGI_FORMAT_R32_UINT;
            descUAV.Buffer.NumElements = IMAGE_WIDTH * IMAGE_HEIGHT;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateUnorderedAccessView(m_pD3D11BufferImage, &descUAV, &m_pD3D11UnorderedAccessViewImage));
        }
        TRYD3D(m_pDevice->GetID3D11Device()->CreateComputeShader(pD3DBlobCodeCS->GetBufferPointer(), pD3DBlobCodeCS->GetBufferSize(), nullptr, &m_pD3D11ComputeShader));
    }
    void Render()
    {
        // Get the backbuffer and create a render target from it.
        CComPtr<ID3D11UnorderedAccessView> pD3D11UnorderedAccessViewRenderTarget;
        {
            CComPtr<ID3D11Texture2D> pD3D11Texture2D;
            TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D));
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateUnorderedAccessView(pD3D11Texture2D, &uavDesc, &pD3D11UnorderedAccessViewRenderTarget));
        }
        // Upload the constant buffer.
        static float a = 0;
        Constants data = {};
        data.Transform.M11 = data.Transform.M22 = data.Transform.M33 = data.Transform.M44 = 1;
        data.Transform.M11 = cosf(a); data.Transform.M21 = sinf(a);
        data.Transform.M12 = -sinf(a); data.Transform.M22 = cosf(a);
        data.Transform.M14 = (RENDERTARGET_WIDTH - IMAGE_WIDTH * cosf(a) + IMAGE_HEIGHT * sinf(a)) / 2.0f;
        data.Transform.M24 = (RENDERTARGET_HEIGHT - IMAGE_WIDTH * sinf(a) - IMAGE_HEIGHT * cosf(a)) / 2.0f;
        data.Transform = Invert(data.Transform);
        data.Width = IMAGE_WIDTH;
        data.Height = IMAGE_HEIGHT;
        a += 0.01f;
        m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(m_pD3D11BufferConstants, 0, nullptr, &data, 0, 0);
        // Beginning of rendering.
        m_pDevice->GetID3D11DeviceContext()->CSSetShader(m_pD3D11ComputeShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1, &m_pD3D11BufferConstants.p);
        m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &pD3D11UnorderedAccessViewRenderTarget.p, nullptr);
        m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(1, 1, &m_pD3D11UnorderedAccessViewImage.p, nullptr);
        m_pDevice->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1);
        ID3D11UnorderedAccessView* makeNullptr = nullptr;
        m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &makeNullptr, nullptr);
        {
            D3D11_VIEWPORT viewportdesc = {};
            viewportdesc.Width = RENDERTARGET_WIDTH;
            viewportdesc.Height = RENDERTARGET_HEIGHT;
            viewportdesc.MaxDepth = 1.0f;
            m_pDevice->GetID3D11DeviceContext()->RSSetViewports(1, &viewportdesc);
        }
        m_pDevice->GetID3D11DeviceContext()->Flush();
        
        // End of rendering; send to display.
        m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    }
};

std::shared_ptr<Sample> CreateSample_D3D11ComputeCanvas(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11ComputeCanvas(pSwapChain, pDevice));
}