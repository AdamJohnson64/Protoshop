///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Compute Canvas
///////////////////////////////////////////////////////////////////////////////
// This sample renders to the display using only compute shaders and no
// rasterization. For interest we create a transformable display that would
// theoretically support a painting application. Texture access is handled via
// direct memory access without samplers or texture objects.
///////////////////////////////////////////////////////////////////////////////

#include "Sample_D3D11DrawingContextCompute.inc"
#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include "generated.Sample_D3D11DrawingContextCompute.cs.h"
#include <assert.h>
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

template <class T> void Append(std::vector<uint8_t> &buffer, const T &data) {
  size_t location = buffer.size();
  buffer.resize(buffer.size() + sizeof(T));
  memcpy(&buffer[location], &data, sizeof(T));
}

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11DrawingContextCompute(
    std::shared_ptr<Direct3D11Device> device) {

  ////////////////////////////////////////////////////////////////////////////////
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  TRYD3D(device->GetID3D11Device()->CreateComputeShader(
      g_cs_shader, sizeof(g_cs_shader), nullptr, &shaderCompute));
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
        D3D11_Create_UAV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    ////////////////////////////////////////////////////////////////////////////////
    // Create a drawing definition.
    std::vector<uint32_t> dataShapeOffset;
    std::vector<uint8_t> dataShapeDefinition;
    {
      // Define a collection of functions to serialize the various shapes.
      std::function<void(const Vector2&, float)> fillCircle = [&](const Vector2& center, float radius) {
        dataShapeOffset.push_back(dataShapeDefinition.size());
        Append(dataShapeDefinition, static_cast<uint32_t>(SHAPE_FILLED_CIRCLE));
        Append(dataShapeDefinition, center);
        Append(dataShapeDefinition, radius);
      };
      std::function<void(const Vector2&, float, float)> strokeCircle = [&](const Vector2& center, float radius, float line_half_width) {
        dataShapeOffset.push_back(dataShapeDefinition.size());
        Append(dataShapeDefinition, static_cast<uint32_t>(SHAPE_STROKED_CIRCLE));
        Append(dataShapeDefinition, center);
        Append(dataShapeDefinition, radius);
        Append(dataShapeDefinition, line_half_width);
      };
      std::function<void(const Vector2&, const Vector2&, float)> strokeLine = [&](const Vector2& p1, const Vector2& p2, float line_half_width) {
        dataShapeOffset.push_back(dataShapeDefinition.size());
        Append(dataShapeDefinition, static_cast<uint32_t>(SHAPE_STROKED_LINE));
        Append(dataShapeDefinition, p1);
        Append(dataShapeDefinition, p2);
        Append(dataShapeDefinition, line_half_width);
      };
      // Use these functions to draw something interesting.
      static uint32_t animate = 0;
      strokeLine({64, 64}, {512, 512}, 32);
      Vector2 position = { 512.0f + sinf(animate * 0.01f) * 256.0f, 512.0f - cosf(animate * 0.01f) * 256.0f };
      fillCircle(position, 32);
      for (int i = 0; i <= 16; ++i)
      {
        strokeCircle(position, 64 + 32 * i, (16 - i) * 0.1f);
      }
      animate = (animate + 1) % 500;
    }
    uint32_t dataOffsetHeader = 0;
    uint32_t dataOffsetTable = dataOffsetHeader + sizeof(uint32_t);
    uint32_t dataOffsetShape = dataOffsetTable + sizeof(uint32_t) * dataShapeOffset.size();
    uint32_t dataSize = dataOffsetShape + dataShapeDefinition.size();
    ////////////////////////////////////////////////////////////////////////////////
    // Create a shape in a SRV buffer.
    CComPtr<ID3D11Buffer> bufferShape;
    {
      std::vector<uint8_t> data;
      // Emit an offset table to each of the shape definitions.
      // Adjust each offset to the beginning of the shape section.
      Append(data, static_cast<uint32_t>(dataShapeOffset.size()));
      for (uint32_t offset : dataShapeOffset) {
        Append(data, dataOffsetShape + offset);
      }
      // Append all of the shape definitions.
      // The offsets above will point to these records.
      data.insert(data.end(), dataShapeDefinition.begin(), dataShapeDefinition.end());
      assert(data.size() == dataSize);
      // Now create the SRV resource for the shader.
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth = dataSize;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
      D3D11_SUBRESOURCE_DATA resdata = {};
      resdata.pSysMem = &data[0];
      TRYD3D(device->GetID3D11Device()->CreateBuffer(&desc, &resdata,
                                                     &bufferShape.p));
    }
    // Create a view of this buffer.
    CComPtr<ID3D11ShaderResourceView> bufferShapeView;
    {
      D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
      desc.Format = DXGI_FORMAT_R32_TYPELESS;
      desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
      desc.BufferEx.NumElements = dataSize / 4;
      desc.BufferEx.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      TRYD3D(device->GetID3D11Device()->CreateShaderResourceView(
          bufferShape, &desc, &bufferShapeView));
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Start rendering.
    device->GetID3D11DeviceContext()->ClearState();
    // Beginning of draw.
    device->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
        0, 1, &uavBackbuffer.p, nullptr);
    device->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr, 0);
    device->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                           &bufferShapeView.p);
    device->GetID3D11DeviceContext()->Dispatch(
        descBackbuffer.Width / THREADCOUNT_X,
        descBackbuffer.Height / THREADCOUNT_Y, 1);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}