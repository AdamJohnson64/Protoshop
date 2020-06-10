#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Framework_Sample.h"
#include <atlbase.h>
#include <cstdint>

#define IMAGE_WIDTH 256
#define IMAGE_HEIGHT 256

class Sample_ComputeCanvas : public Sample
{
private:
    struct Constants
    {
        float
            m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33;
        int width, height;
    };
    CComPtr<ID3D11ComputeShader> pD3D11ComputeShader;
    CComPtr<ID3D11Buffer> pD3D11BufferConstants;
    CComPtr<ID3D11Buffer> pD3D11BufferImage;
    CComPtr<ID3D11UnorderedAccessView> pD3D11UnorderedAccessViewImage;
public:
    Sample_ComputeCanvas()
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
    //renderTarget[dispatchThreadId.xy] = Sample((float2)dispatchThreadId);
    //return;
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
            TRYD3D(GetID3D11Device()->CreateBuffer(&descBuffer, nullptr, &pD3D11BufferConstants));
        }
        {
            D3D11_BUFFER_DESC descBuffer = {};
            descBuffer.ByteWidth = sizeof(uint32_t) * IMAGE_WIDTH * IMAGE_HEIGHT;
            descBuffer.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            descBuffer.StructureByteStride = sizeof(uint32_t);
            uint32_t data[IMAGE_WIDTH * IMAGE_HEIGHT];
            for (int a = 0; a < IMAGE_WIDTH * IMAGE_HEIGHT; ++a)
            {
                data[a] = 0xFF0080FF + a;
            }
            data[16 + 16 * IMAGE_WIDTH] = 0xFFFF0000;
            D3D11_SUBRESOURCE_DATA descData = {};
            descData.pSysMem = data;
            TRYD3D(GetID3D11Device()->CreateBuffer(&descBuffer, &descData, &pD3D11BufferImage));
        }
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
            descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            descUAV.Format = DXGI_FORMAT_R32_UINT;
            descUAV.Buffer.NumElements = IMAGE_WIDTH * IMAGE_HEIGHT;
            TRYD3D(GetID3D11Device()->CreateUnorderedAccessView(pD3D11BufferImage, &descUAV, &pD3D11UnorderedAccessViewImage));
        }
        TRYD3D(GetID3D11Device()->CreateComputeShader(pD3DBlobCodeCS->GetBufferPointer(), pD3DBlobCodeCS->GetBufferSize(), nullptr, &pD3D11ComputeShader));
    }
    void Render()
    {
        // Get the backbuffer and create a render target from it.
        CComPtr<ID3D11UnorderedAccessView> pD3D11UnorderedAccessViewRenderTarget;
        {
            CComPtr<ID3D11Texture2D> pD3D11Texture2D;
            TRYD3D(GetIDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D));
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            TRYD3D(GetID3D11Device()->CreateUnorderedAccessView(pD3D11Texture2D, &uavDesc, &pD3D11UnorderedAccessViewRenderTarget));
        }
        // Upload the constant buffer.
        static float a = 0;
        static float t = 0;
        Constants data = {};
        data.m00 = data.m11 = data.m22 = data.m33 = 1;
        data.m00 = cosf(a); data.m10 = sinf(a);
        data.m01 = -sinf(a); data.m11 = cosf(a);
        data.m03 = -t;
        data.width = IMAGE_WIDTH;
        data.height = IMAGE_HEIGHT;
        a += 0.001f;
        //t += 1;
        if (t > 100) t -= 100;
        GetID3D11DeviceContext()->UpdateSubresource(pD3D11BufferConstants, 0, nullptr, &data, 0, 0);
        // Beginning of rendering.
        GetID3D11DeviceContext()->CSSetShader(pD3D11ComputeShader, nullptr, 0);
        GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1, &pD3D11BufferConstants.p);
        GetID3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &pD3D11UnorderedAccessViewRenderTarget.p, nullptr);
        GetID3D11DeviceContext()->CSSetUnorderedAccessViews(1, 1, &pD3D11UnorderedAccessViewImage.p, nullptr);
        GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT, 1);
        ID3D11UnorderedAccessView* makeNullptr = nullptr;
        GetID3D11DeviceContext()->CSSetUnorderedAccessViews(0, 1, &makeNullptr, nullptr);
        {
            D3D11_VIEWPORT viewportdesc = {};
            viewportdesc.Width = RENDERTARGET_WIDTH;
            viewportdesc.Height = RENDERTARGET_HEIGHT;
            viewportdesc.MaxDepth = 1.0f;
            GetID3D11DeviceContext()->RSSetViewports(1, &viewportdesc);
        }
        GetID3D11DeviceContext()->Flush();
        
        // End of rendering; send to display.
        GetIDXGISwapChain()->Present(0, 0);
    }
};

Sample* CreateSample_ComputeCanvas()
{
    return new Sample_ComputeCanvas();
}