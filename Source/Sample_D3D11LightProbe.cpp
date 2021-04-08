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
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include "generated.Sample_D3D11LightProbe.cs.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

#include <fstream>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11LightProbe(std::shared_ptr<Direct3D11Device> device) {
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  TRYD3D(device->GetID3D11Device()->CreateComputeShader(
      g_cs_shader, sizeof(g_cs_shader), nullptr, &shaderCompute));
  __declspec(align(16)) struct Constants {
    Matrix44 TransformClipToWorld;
    Vector2 WindowDimensions;
  };
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
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
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(textureLightProbe),
        &srvLightProbe.p));
  }
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
        D3D11_Create_UAV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    device->GetID3D11DeviceContext()->ClearState();
    // Upload the constant buffer.
    {
      Constants constants;
      constants.TransformClipToWorld =
          Invert(sampleResources.TransformWorldToClip);
      constants.WindowDimensions = {static_cast<float>(descBackbuffer.Width),
                                    static_cast<float>(descBackbuffer.Height)};
      device->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, &constants, 0, 0);
    }
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &uavBackbuffer.p, nullptr);
    device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
    device->GetID3D11DeviceContext()->CSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                           &srvLightProbe.p);
    device->GetID3D11DeviceContext()->Dispatch(descBackbuffer.Width,
                                               descBackbuffer.Height, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
