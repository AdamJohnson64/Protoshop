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
#include "Core_ISample.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "Scene_Camera.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

#include <fstream>

class Sample_D3D11LightProbe : public Object, public ISample {
private:
  std::function<void()> m_fnRender;

public:
  Sample_D3D11LightProbe(std::shared_ptr<DXGISwapChain> m_pSwapChain,
                         std::shared_ptr<Direct3D11Device> m_pDevice) {
    // Create a compute shader.
    CComPtr<ID3D11ComputeShader> shaderCompute;
    {
      CComPtr<ID3DBlob> blobCS =
          CompileShader("cs_5_0", "main", R"SHADER(
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
      TRYD3D(m_pDevice->GetID3D11Device()->CreateComputeShader(
          blobCS->GetBufferPointer(), blobCS->GetBufferSize(),
          nullptr, &shaderCompute));
    }
    CComPtr<ID3D11Buffer> bufferConstants =
        D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                            D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
    CComPtr<ID3D11ShaderResourceView> srvLightProbe;
    {
      const int imageWidth = 1000;
      const int imageHeight = 1000;
      struct Pixel {
        float B, G, R;
      };
      const uint32_t imageStride = sizeof(Pixel) * imageWidth;
      std::ifstream readit("C:\\_\\Probes\\grace_probe.float",
                           std::ios::binary);
      std::unique_ptr<char> buffer(new char[imageStride * imageHeight]);
      for (int y = 0; y < imageHeight; ++y) {
        char *raster = reinterpret_cast<char *>(buffer.get()) +
                       imageStride * (imageHeight - y - 1);
        readit.read(raster, imageStride);
      }
      CComPtr<ID3D11Texture2D> textureLightProbe = D3D11_Create_Texture2D(
          m_pDevice->GetID3D11Device(), DXGI_FORMAT_R32G32B32_FLOAT, imageWidth,
          imageHeight, buffer.get());
      TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(
          textureLightProbe,
          &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(
              DXGI_FORMAT_R32G32B32_FLOAT),
          &srvLightProbe.p));
    }
    m_fnRender = [=]() {
      // Get the backbuffer and create a render target from it.
      CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
          D3D11_Create_UAV_From_SwapChain(m_pDevice->GetID3D11Device(),
                                          m_pSwapChain->GetIDXGISwapChain());
      m_pDevice->GetID3D11DeviceContext()->ClearState();
      // Upload the constant buffer.
      m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, &Invert(GetCameraWorldToClip()), 0,
          0);
      // Beginning of rendering.
      m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
          0, 1, &uavBackbuffer.p, nullptr);
      m_pDevice->GetID3D11DeviceContext()->CSSetShader(shaderCompute,
                                                       nullptr, 0);
      m_pDevice->GetID3D11DeviceContext()->CSSetConstantBuffers(
          0, 1, &bufferConstants.p);
      m_pDevice->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                                &srvLightProbe.p);
      m_pDevice->GetID3D11DeviceContext()->Dispatch(RENDERTARGET_WIDTH,
                                                    RENDERTARGET_HEIGHT, 1);
      m_pDevice->GetID3D11DeviceContext()->ClearState();
      m_pDevice->GetID3D11DeviceContext()->Flush();
      // End of rendering; send to display.
      m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    };
  }
  void Render() { m_fnRender(); }
};

std::shared_ptr<ISample>
CreateSample_D3D11LightProbe(std::shared_ptr<DXGISwapChain> swapchain,
                             std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<ISample>(
      new Sample_D3D11LightProbe(swapchain, device));
}