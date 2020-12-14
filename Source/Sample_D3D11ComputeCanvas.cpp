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
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

const int IMAGE_WIDTH = 320;
const int IMAGE_HEIGHT = 200;

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11ComputeCanvas(std::shared_ptr<Direct3D11Device> device) {

  ////////////////////////////////////////////////////////////////////////////////
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> pD3DBlobCodeCS =
        CompileShader("cs_5_0", "main", R"SHADER(
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
    TRYD3D(device->GetID3D11Device()->CreateComputeShader(
        pD3DBlobCodeCS->GetBufferPointer(), pD3DBlobCodeCS->GetBufferSize(),
        nullptr, &shaderCompute));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create constant buffer.
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
  ////////////////////////////////////////////////////////////////////////////////
  // Create an image to be used as the canvas.
  CComPtr<ID3D11ShaderResourceView> srvCanvasImage;
  {
    CComPtr<ID3D11Texture2D> textureCanvasImage;
    {
      struct Pixel {
        uint8_t B, G, R, A;
      };
      const uint32_t imageStride = sizeof(Pixel) * IMAGE_WIDTH;
      Pixel imageRaw[IMAGE_WIDTH * IMAGE_HEIGHT];
      Image_Fill_Commodore64(imageRaw, IMAGE_WIDTH, IMAGE_HEIGHT, imageStride);
      textureCanvasImage = D3D11_Create_Texture2D(
          device->GetID3D11Device(), DXGI_FORMAT_B8G8R8A8_UNORM, IMAGE_WIDTH,
          IMAGE_HEIGHT, imageRaw);
    }
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        textureCanvasImage,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(textureCanvasImage),
        &srvCanvasImage.p));
  }
  return [=](ID3D11Texture2D *textureBackbuffer) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
        D3D11_Create_UAV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    ////////////////////////////////////////////////////////////////////////////////
    // Start rendering.
    device->GetID3D11DeviceContext()->ClearState();
    // Upload the constant buffer.
    static float angle = 0;
    const float zoom = 2 + Cos(angle);
    Matrix44 transformImageToPixel =
        CreateMatrixTranslate(Vector3{-IMAGE_WIDTH / 2, -IMAGE_HEIGHT / 2, 0}) *
        CreateMatrixScale(Vector3{zoom, zoom, 1}) *
        CreateMatrixRotationZ(angle) *
        CreateMatrixTranslate(Vector3{descBackbuffer.Width / 2.0f,
                                      descBackbuffer.Height / 2.0f, 0});
    angle += 0.01f;
    device->GetID3D11DeviceContext()->UpdateSubresource(
        bufferConstants, 0, nullptr, &Invert(transformImageToPixel), 0, 0);
    // Beginning of draw.
    device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &uavBackbuffer.p, nullptr);
    device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
    device->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                           &srvCanvasImage.p);
    device->GetID3D11DeviceContext()->Dispatch(descBackbuffer.Width,
                                               descBackbuffer.Height, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}