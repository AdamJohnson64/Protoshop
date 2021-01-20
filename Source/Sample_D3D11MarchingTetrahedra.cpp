///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Marching Tetrahedra
///////////////////////////////////////////////////////////////////////////////
// Evaluate the boundary of an arbitrary isosurface using voxels of
// irregular tetrahedra. This is considerably less involved than pure marching
// cubes since there are fewer combinations of voxel surface and no ambiguous
// intersection configurations.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Util.h"
#include "ImageUtil.h"
#include "SampleResources.h"
#include <array>
#include <atlbase.h>
#include <functional>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11MarchingTetrahedra(std::shared_ptr<Direct3D11Device> device) {
  CComPtr<ID3D11RasterizerState> rasterizerState;
  {
    D3D11_RASTERIZER_DESC desc = {};
    desc.CullMode = D3D11_CULL_BACK;
    desc.FillMode = D3D11_FILL_SOLID;
    device->GetID3D11Device()->CreateRasterizerState(&desc, &rasterizerState);
  }
  CComPtr<ID3D11SamplerState> samplerDefaultWrap;
  TRYD3D(device->GetID3D11Device()->CreateSamplerState(
      &Make_D3D11_SAMPLER_DESC_DefaultWrap(), &samplerDefaultWrap.p));
  CComPtr<ID3D11Buffer> bufferConstants =
      D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER,
                          sizeof(ConstantsWorld));
  const char *szShaderCode = R"SHADER(
#include "Sample_D3D11_Common.inc"

struct VertexIn
{
  float4 Position : SV_Position;
  float4 Color : COLOR;
};

struct VertexOut
{
  float4 Position : SV_Position;
  float4 Color : COLOR;
};

VertexOut mainVS(VertexIn vin)
{
  VertexOut vout;
  vout.Position = mul(TransformWorldToClip, vin.Position);
  vout.Color = vin.Color;
  return vout;
}

float4 mainPS(VertexOut vin) : SV_Target
{
    return float4(vin.Color.rgb, 1);
})SHADER";
  CComPtr<ID3D11VertexShader> shaderVertex;
  CComPtr<ID3D11InputLayout> inputLayout;
  {
    CComPtr<ID3DBlob> blobVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreateVertexShader(
        blobVS->GetBufferPointer(), blobVS->GetBufferSize(), nullptr,
        &shaderVertex));
    {
      std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputdesc = {};
      inputdesc[0].SemanticName = "SV_Position";
      inputdesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
      inputdesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[0].AlignedByteOffset = offsetof(VertexCol, Position);
      inputdesc[1].SemanticName = "COLOR";
      inputdesc[1].Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      inputdesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
      inputdesc[1].AlignedByteOffset = offsetof(VertexCol, Color);
      TRYD3D(device->GetID3D11Device()->CreateInputLayout(
          &inputdesc[0], inputdesc.size(), blobVS->GetBufferPointer(),
          blobVS->GetBufferSize(), &inputLayout));
    }
  }
  CComPtr<ID3D11PixelShader> shaderPixel;
  {
    ID3DBlob *blobPS = CompileShader("ps_5_0", "mainPS", szShaderCode);
    TRYD3D(device->GetID3D11Device()->CreatePixelShader(
        blobPS->GetBufferPointer(), blobPS->GetBufferSize(), nullptr,
        &shaderPixel));
  }
  ////////////////////////////////////////////////////////////////////////////////
  // Create a vertex buffer.
  int vertexCount = 0;
  CComPtr<ID3D11Buffer> bufferVertex;
  {
    std::function<float(const Vector3 &p)> DistanceFunction =
        [=](const Vector3 &p) {
          Vector3 p2 = p - Vector3{32, 32, 32};
          p2.Y += sinf(p.X) * 0.1f;
          return sqrtf(Dot(p2, p2)) - 32;
        };
    std::vector<VertexCol> vertices;
    for (int x = 0; x < 64; ++x) {
      for (int y = 0; y < 64; ++y) {
        for (int z = 0; z < 64; ++z) {
          // Define the voxel corners.
          // The corners are defined using XYZ as a min/max bitcode.
          // 0 = min, 1 = max (e.g. 010 = Min X, Max y, Min Z).
          Vector3 vertexCube[8];
          for (int i = 0; i < 8; ++i) {
            vertexCube[i] = Vector3{static_cast<float>(x + ((i & 1) >> 0)),
                                    static_cast<float>(y + ((i & 2) >> 1)),
                                    static_cast<float>(z + ((i & 4) >> 2))};
          }
          // Define parity.
          // We need to keep a good check on what is inside and outside the
          // isosurface. Each time we flip an axis we need to invert parity
          // since "inside" becomes "outside".
          bool parity = false;
          // Mirror the voxel on odd X, Y, and Z cells to prevent T-Junctions.
          if ((x & 1) == 1) {
            Vector3 t;
            t = vertexCube[0];
            vertexCube[0] = vertexCube[1];
            vertexCube[1] = t;
            t = vertexCube[2];
            vertexCube[2] = vertexCube[3];
            vertexCube[3] = t;
            t = vertexCube[4];
            vertexCube[4] = vertexCube[5];
            vertexCube[5] = t;
            t = vertexCube[6];
            vertexCube[6] = vertexCube[7];
            vertexCube[7] = t;
            parity = !parity;
          }
          if ((y & 1) == 1) {
            Vector3 t;
            t = vertexCube[0];
            vertexCube[0] = vertexCube[2];
            vertexCube[2] = t;
            t = vertexCube[1];
            vertexCube[1] = vertexCube[3];
            vertexCube[3] = t;
            t = vertexCube[4];
            vertexCube[4] = vertexCube[6];
            vertexCube[6] = t;
            t = vertexCube[5];
            vertexCube[5] = vertexCube[7];
            vertexCube[7] = t;
            parity = !parity;
          }
          if ((z & 1) == 1) {
            Vector3 t;
            t = vertexCube[0];
            vertexCube[0] = vertexCube[4];
            vertexCube[4] = t;
            t = vertexCube[1];
            vertexCube[1] = vertexCube[5];
            vertexCube[5] = t;
            t = vertexCube[2];
            vertexCube[2] = vertexCube[6];
            vertexCube[6] = t;
            t = vertexCube[3];
            vertexCube[3] = vertexCube[7];
            vertexCube[7] = t;
            parity = !parity;
          }
          // Define tetrahedra using the indices of the voxel corners.
          struct TetraIndex {
            int Index[4];
          };
          const TetraIndex indexTetra[5] = {{0, 2, 1, 4},
                                            {1, 4, 2, 7},
                                            {2, 7, 4, 6},
                                            {1, 7, 2, 3},
                                            {1, 4, 7, 5}};
          // Go through each tetrahedron defined above and intersect.
          for (int eachTetra = 0; eachTetra < 5; ++eachTetra) {
            const Vector3 vertexTetra[4] = {
                vertexCube[indexTetra[eachTetra].Index[0]],
                vertexCube[indexTetra[eachTetra].Index[1]],
                vertexCube[indexTetra[eachTetra].Index[2]],
                vertexCube[indexTetra[eachTetra].Index[3]],
            };
            // Compute the distance of each tetrahedron point to the isosurface.
            float vertexDistance[4];
            for (int i = 0; i < 4; ++i) {
              vertexDistance[i] = DistanceFunction(vertexTetra[i]);
            }
            // Emit a triangle.
            // This adjusts for parity as defined above.
            std::function<void(const Vector3 &, const Vector3 &,
                               const Vector3 &)>
                EmitTriangle = [&](const Vector3 &p0, const Vector3 &p1,
                                   const Vector3 &p2) {
                  vertices.push_back(VertexCol{p0, 0xFFFF0000});
                  if (parity) {
                    vertices.push_back(VertexCol{p2, 0xFF00FF00});
                    vertices.push_back(VertexCol{p1, 0xFF0000FF});
                  } else {
                    vertices.push_back(VertexCol{p1, 0xFF00FF00});
                    vertices.push_back(VertexCol{p2, 0xFF0000FF});
                  }
                };
            // Compute the intersection point of the tetrahedron point pair on
            // the isosurface.
            std::function<Vector3(int, int)> Interpolate = [&](int p0, int p1) {
              float distance = -vertexDistance[p0] /
                               (vertexDistance[p1] - vertexDistance[p0]);
              return vertexTetra[p0] +
                     (vertexTetra[p1] - vertexTetra[p0]) * distance;
            };
            // Generate a triangle if 1 point is in and 3 points are out.
            std::function<void(int, int, int, int)> GenerateTriangle =
                [&](int p0, int p1, int p2, int p3) {
                  if (vertexDistance[p0] < 0 && vertexDistance[p1] >= 0 &&
                      vertexDistance[p2] >= 0 && vertexDistance[p3] >= 0) {
                    EmitTriangle(Interpolate(p0, p1), Interpolate(p0, p2),
                                 Interpolate(p0, p3));
                  }
                  // While we're here we can perform the 1-out-3-in test but
                  // flip the parity on these triangles.
                  if (vertexDistance[p0] >= 0 && vertexDistance[p1] < 0 &&
                      vertexDistance[p2] < 0 && vertexDistance[p3] < 0) {
                    EmitTriangle(Interpolate(p0, p1), Interpolate(p0, p3),
                                 Interpolate(p0, p2));
                  }
                };
            // Test all potential triangle triples.
            GenerateTriangle(0, 1, 3, 2);
            GenerateTriangle(1, 0, 2, 3);
            GenerateTriangle(2, 0, 3, 1);
            GenerateTriangle(3, 0, 1, 2);
            // Generate a quad if 2 points are in and 2 points are out.
            std::function<void(int, int, int, int)> GenerateQuad =
                [&](int p0, int p1, int p2, int p3) {
                  if (vertexDistance[p0] < 0 && vertexDistance[p1] < 0 &&
                      vertexDistance[p2] >= 0 && vertexDistance[p3] >= 0) {
                    EmitTriangle(Interpolate(p0, p2), Interpolate(p0, p3),
                                 Interpolate(p1, p3));
                    EmitTriangle(Interpolate(p1, p3), Interpolate(p1, p2),
                                 Interpolate(p0, p2));
                  }
                };
            // Test all potential quad configurations.
            GenerateQuad(0, 1, 3, 2);
            GenerateQuad(0, 2, 1, 3);
            GenerateQuad(0, 3, 2, 1);
            GenerateQuad(1, 2, 3, 0);
            GenerateQuad(1, 3, 0, 2);
            GenerateQuad(2, 3, 1, 0);
          }
        }
      }
    }
    vertexCount = vertices.size();
    bufferVertex =
        D3D11_Create_Buffer(device->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER,
                            sizeof(VertexCol) * vertexCount, &vertices[0]);
  }
  return [=](const SampleResourcesD3D11 &sampleResources) {
    D3D11_TEXTURE2D_DESC descBackbuffer = {};
    sampleResources.BackBufferTexture->GetDesc(&descBackbuffer);
    CComPtr<ID3D11RenderTargetView> rtvBackbuffer =
        D3D11_Create_RTV_From_Texture2D(device->GetID3D11Device(),
                                        sampleResources.BackBufferTexture);
    device->GetID3D11DeviceContext()->ClearState();
    ////////////////////////////////////////////////////////////////////////////////
    // Beginning of rendering.
    device->GetID3D11DeviceContext()->ClearRenderTargetView(
        rtvBackbuffer, &std::array<FLOAT, 4>{0.1f, 0.1f, 0.1f, 1.0f}[0]);
    device->GetID3D11DeviceContext()->ClearDepthStencilView(
        sampleResources.DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device->GetID3D11DeviceContext()->RSSetViewports(
        1, &Make_D3D11_VIEWPORT(descBackbuffer.Width, descBackbuffer.Height));
    device->GetID3D11DeviceContext()->OMSetRenderTargets(
        1, &rtvBackbuffer.p, sampleResources.DepthStencilView);
    ////////////////////////////////////////////////////////////////////////////////
    // Update constant buffer.
    {
      static float angle = 0;
      ConstantsWorld data = {};
      data.TransformWorldToClip = sampleResources.TransformWorldToClip;
      data.LightPosition = Normalize(Vector3{sinf(angle), 1, -cosf(angle)});
      angle += 0.05f;
      device->GetID3D11DeviceContext()->UpdateSubresource(bufferConstants, 0,
                                                          nullptr, &data, 0, 0);
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Setup and draw.
    device->GetID3D11DeviceContext()->VSSetShader(shaderVertex, nullptr, 0);
    device->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->RSSetState(rasterizerState);
    device->GetID3D11DeviceContext()->PSSetShader(shaderPixel, nullptr, 0);
    device->GetID3D11DeviceContext()->PSSetConstantBuffers(0, 1,
                                                           &bufferConstants.p);
    device->GetID3D11DeviceContext()->PSSetSamplers(kSamplerRegisterDefaultWrap,
                                                    1, &samplerDefaultWrap.p);
    device->GetID3D11DeviceContext()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device->GetID3D11DeviceContext()->IASetInputLayout(inputLayout);
    {
      UINT uStrides[] = {sizeof(VertexCol)};
      UINT uOffsets[] = {0};
      device->GetID3D11DeviceContext()->IASetVertexBuffers(
          0, 1, &bufferVertex.p, uStrides, uOffsets);
    }
    device->GetID3D11DeviceContext()->Draw(vertexCount, 0);
    device->GetID3D11DeviceContext()->ClearState();
    device->GetID3D11DeviceContext()->Flush();
  };
}
