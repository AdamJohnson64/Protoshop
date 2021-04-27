///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Compute Canvas
///////////////////////////////////////////////////////////////////////////////
// This sample renders to the display using only compute shaders and no
// rasterization. For interest we create a transformable display that would
// theoretically support a painting application. Texture access is handled via
// direct memory access without samplers or texture objects.
///////////////////////////////////////////////////////////////////////////////

#include "Sample_D3D11ComputeVector.inc"
#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include "generated.Sample_D3D11ComputeVector.cs.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

Vector2 CoeffSum(Vector2 hull[4], float t, float coeff[4]) {
  Vector2 sum = {};
  for (int i = 0; i < 4; ++i) {
    sum = sum + coeff[i] * hull[i];
  }
  return sum;
}

Vector2 Bezier(Vector2 hull[4], float _t) {
  float t = _t;
  float mt = 1 - _t;
  float bernstein[4] = {
      mt * mt * mt,    // (1-t)^3
      3 * t * mt * mt, // 3t(1-t)^2
      3 * t * t * mt,  // 3t^2(1-t)
      t * t * t        // t^3
  };
  return CoeffSum(hull, _t, bernstein);
}

Vector2 BezierDerivExact(Vector2 hull[4], float t) {
  float bernsteinD[4] = {
      -3 * t * t + 6 * t - 3,      // d/dt (1-t)^3   = -3t^2 + 6t - 3
      3 * (3 * t * t - 4 * t + 1), // d/dt 3t(1-t)^2 = 3(-3t^2 + 6t - 3)
      3 * (-3 * t * t + 2 * t),    // d/dt 3t^2(1-t) = 3(-3t^2 + 2t)
      3 * t * t                    // d/dt t^3       = 3t^2
  };
  return CoeffSum(hull, t, bernsteinD);
}

Vector2 BezierDerivFiniteDifference(Vector2 hull[4], float interp) {
  Vector2 p1 = Bezier(hull, interp - 0.01f);
  Vector2 p2 = Bezier(hull, interp + 0.01f);
  return (p2 - p1) * (1 / 0.02f);
}

Vector2 BezierDeriv(Vector2 hull[4], float t) {
  return BezierDerivExact(hull, t);
}

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11ComputeVector(std::shared_ptr<Direct3D11Device> device) {

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
    // Create a Bezier.
    struct Edge {
      Vector2 p[2];
    };
    std::vector<Edge> points;
    {
      static int animate = 0;
      Vector2 hull[4] = {
          {50, 50},
          {100 + animate, 100},
          {200, 400 + animate},
          {500, 500},
      };
      animate = (animate + 5) % 500;
      const int BEZIER_STEPS = 100;
      const float LINE_HALF_WIDTH = 4.0f;
      {
        Vector2 c1 = Bezier(hull, 0);
        Vector2 d1 = Normalize(BezierDeriv(hull, 0));
        Vector2 p1 = {-d1.Y, d1.X};
        points.push_back(
            {c1 - p1 * LINE_HALF_WIDTH, c1 + p1 * LINE_HALF_WIDTH});
      }
      for (int i = 0; i < BEZIER_STEPS; ++i) {
        float t1 = static_cast<float>(i + 0) / BEZIER_STEPS;
        Vector2 c1 = Bezier(hull, t1);
        Vector2 d1 = Normalize(BezierDeriv(hull, t1));
        Vector2 p1 = {-d1.Y, d1.X};
        float t2 = static_cast<float>(i + 1) / BEZIER_STEPS;
        Vector2 c2 = Bezier(hull, t2);
        Vector2 d2 = Normalize(BezierDeriv(hull, t2));
        Vector2 p2 = {-d2.Y, d2.X};
        points.push_back(
            {c1 - p1 * LINE_HALF_WIDTH, c2 - p2 * LINE_HALF_WIDTH});
        points.push_back(
            {c1 + p1 * LINE_HALF_WIDTH, c2 + p2 * LINE_HALF_WIDTH});
      }
      {
        Vector2 c1 = Bezier(hull, 1);
        Vector2 d1 = Normalize(BezierDeriv(hull, 1));
        Vector2 p1 = {-d1.Y, d1.X};
        points.push_back(
            {c1 - p1 * LINE_HALF_WIDTH, c1 + p1 * LINE_HALF_WIDTH});
      }
    }
    uint32_t dataSize = sizeof(uint32_t) + sizeof(Edge) * points.size();
    // Create a shape in a SRV buffer.
    CComPtr<ID3D11Buffer> bufferShape;
    {
      std::unique_ptr<uint8_t[]> data(new uint8_t[dataSize]);
      uint32_t *pPointCount = reinterpret_cast<uint32_t *>(data.get());
      *pPointCount = points.size();
      Edge *pPoints = reinterpret_cast<Edge *>(pPointCount + 1);
      for (int i = 0; i < points.size(); ++i) {
        *pPoints = points[i];
        ++pPoints;
      }
      D3D11_BUFFER_DESC desc = {};
      desc.ByteWidth = dataSize;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
      D3D11_SUBRESOURCE_DATA resdata = {};
      resdata.pSysMem = data.get();
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