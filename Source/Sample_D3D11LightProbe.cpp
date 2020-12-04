///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Light Probe
///////////////////////////////////////////////////////////////////////////////
// This sample renders a spherical light probe map from float data using a
// compute shader.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_ITransformSource.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

#include <fstream>

std::function<void(ID3D11UnorderedAccessView *)>
CreateSample_D3D11LightProbe(std::shared_ptr<Direct3D11Device> device) {
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
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
    TRYD3D(device->GetID3D11Device()->CreateComputeShader(
        blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
        &shaderCompute));
  }
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
  CComPtr<ID3D11ShaderResourceView> srvLightProbe;
  {
    const int imageWidth = 1000;
    const int imageHeight = 1000;
    struct Pixel {
      float B, G, R;
    };
    const uint32_t imageStride = sizeof(Pixel) * imageWidth;
    std::ifstream readit("C:\\_\\Probes\\grace_probe.float", std::ios::binary);
    std::unique_ptr<char> buffer(new char[imageStride * imageHeight]);
    for (int y = 0; y < imageHeight; ++y) {
      char *raster = reinterpret_cast<char *>(buffer.get()) +
                     imageStride * (imageHeight - y - 1);
      readit.read(raster, imageStride);
    }
    CComPtr<ID3D11Texture2D> textureLightProbe = D3D11_Create_Texture2D(
        device->GetID3D11Device(), DXGI_FORMAT_R32G32B32_FLOAT, imageWidth,
        imageHeight, buffer.get());
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        textureLightProbe,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
            DXGI_FORMAT_R32G32B32_FLOAT),
        &srvLightProbe.p));
  }
  return [=](ID3D11UnorderedAccessView *uavBackbuffer) {
    device->GetID3D11DeviceContext()->ClearState();
    // Upload the constant buffer.
    device->GetID3D11DeviceContext()->UpdateSubresource(
        bufferConstants, 0, nullptr, &Invert(GetTransformSource()->GetTransformWorldToClip()), 0, 0);
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &uavBackbuffer, nullptr);
    device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
    device->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                           &srvLightProbe.p);
    device->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH,
                                               RENDERTARGET_HEIGHT, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
