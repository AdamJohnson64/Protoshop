///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Show Texture
///////////////////////////////////////////////////////////////////////////////
// If you ever find yourself with a texture and just want to see what's in it
// then drop that image in here. We use a compute render to extract the texels
// directly - the image on screen should be a 1-1 reproduction.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

std::function<void(ID3D11Texture2D *)>
CreateSample_D3D11ShowTexture(std::shared_ptr<Direct3D11Device> device) {
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    renderTarget[dispatchThreadId.xy] = userImage.Load(int3(dispatchThreadId.xy, 0));
})SHADER");
    TRYD3D(device->GetID3D11Device()->CreateComputeShader(
        blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
        &shaderCompute));
  }
  CComPtr<ID3D11ShaderResourceView> srvImage;
  {
    CComPtr<ID3D11Texture2D> textureImage;
    {
      struct Pixel {
        uint8_t B, G, R, A;
      };
      const uint32_t imageWidth = 320;
      const uint32_t imageHeight = 200;
      const uint32_t imageStride = sizeof(Pixel) * imageWidth;
      std::unique_ptr<Pixel[]> imageRaw(new Pixel[imageWidth * imageHeight]);
      Image_Fill_Commodore64(imageRaw.get(), imageWidth, imageHeight, imageStride);
      textureImage = D3D11_Create_Texture2D(device->GetID3D11Device(),
                                            DXGI_FORMAT_B8G8R8A8_UNORM,
                                            imageWidth, imageHeight, imageRaw.get());
    }
    TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
        textureImage,
        &Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(textureImage),
        &srvImage.p));
  }
  return [=](ID3D11Texture2D *textureBackbuffer) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    textureBackbuffer->GetDesc(&descBackbuffer);
    CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
        D3D11_Create_UAV_From_Texture2D(device->GetID3D11Device(),
                                        textureBackbuffer);
    device->GetID3D11DeviceContext()->ClearState();
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &uavBackbuffer.p, nullptr);
    device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
    device->GetID3D11DeviceContext()->CSSetShaderResources(0, 1, &srvImage.p);
    device->GetID3D11DeviceContext()->Dispatch(descBackbuffer.Width,
                                               descBackbuffer.Height, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
