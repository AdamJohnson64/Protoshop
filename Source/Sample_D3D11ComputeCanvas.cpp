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
#include "SampleResources.h"
#include "generated.Sample_D3D11ComputeCanvas.cs.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

const int IMAGE_WIDTH = 320;
const int IMAGE_HEIGHT = 200;

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11ComputeCanvas(std::shared_ptr<Direct3D11Device> device) {

  ////////////////////////////////////////////////////////////////////////////////
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  TRYD3D(device->GetID3D11Device()->CreateComputeShader(
      g_cs_shader, sizeof(g_cs_shader), nullptr, &shaderCompute));
  ////////////////////////////////////////////////////////////////////////////////
  // Create constant buffer.
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
  ////////////////////////////////////////////////////////////////////////////////
  // Create an image to be used as the canvas.
  CComPtr<ID3D11ShaderResourceView> srvCanvasImage =
      D3D11_Create_SRV(device->GetID3D11DeviceContext(),
                       Image_Commodore64(IMAGE_WIDTH, IMAGE_HEIGHT).get());
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
        D3D11_Create_UAV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
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
    device->GetID3D11DeviceContext()->Dispatch(descBackbuffer.Width / 16,
                                               descBackbuffer.Height / 16, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}