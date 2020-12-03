///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Voxel Walking
///////////////////////////////////////////////////////////////////////////////
// This sample walks through a 3D voxel image and computes the sum contribution
// of voxels along view rays.
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
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

const int VOXEL_SIZE = 64;

class Sample_D3D11Voxel : public Object, public ISample {
private:
  std::function<void()> m_fnRender;

public:
  Sample_D3D11Voxel(std::shared_ptr<DXGISwapChain> m_pSwapChain,
                    std::shared_ptr<Direct3D11Device> m_pDevice) {
    // Create a compute shader.
    CComPtr<ID3D11ComputeShader> shaderCompute;
    {
      CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 transform;
};

RWTexture2D<float4> renderTarget;
Texture3D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ////////////////////////////////////////////////////////////////////////////////
    // The ray.
    float4 orgT = float4(dispatchThreadId.x, dispatchThreadId.y, -1, 1);
    float4 dirT = float4(0, 0, 10, 0);
    orgT = mul(transform, orgT);
    dirT = mul(transform, dirT);
    float3 org = orgT.xyz;
    float3 dir = dirT.xyz;
    ////////////////////////////////////////////////////////////////////////////////
    // Minimum and maximum dimensions of voxel space.
    int Width, Height, Depth, NumberOfLevels;
    userImage.GetDimensions(0, Width, Height, Depth, NumberOfLevels);
    int3 cellmin = int3(0, 0, 0);
    int3 cellmax = int3(Width, Height, Depth);
    ////////////////////////////////////////////////////////////////////////////////
    // Walking directions.
    float3 walk = sign(dir);
    ////////////////////////////////////////////////////////////////////////////////
    // Plane fronts.
    float3 plane;
    plane.x = dir.x > 0 ? 0 : cellmax.x;
    plane.y = dir.y > 0 ? 0 : cellmax.y;
    plane.z = dir.z > 0 ? 0 : cellmax.z;
    ////////////////////////////////////////////////////////////////////////////////
    // Intersection lambdas.
    // We intersect our ray with the nearest X, Y, and Z planes.
    float3 lambda = float3(
        dir.x == 0 ? 1.#INF : (plane.x - org.x) / dir.x,
        dir.y == 0 ? 1.#INF : (plane.y - org.y) / dir.y,
        dir.z == 0 ? 1.#INF : (plane.z - org.z) / dir.z
    );
    ////////////////////////////////////////////////////////////////////////////////
    // Start marching the ray.
    float lambdaMin = 0;
    float4 accumulate = 0;
    for (int i = 0; i < 100; ++i)
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Pick the plane intersection axis with the closest hit (lowest parameter).
        // Whichever one we pick we advance it in the walk direction and update lambda.
        float lambdaMax = 0;
        if (lambda.x < lambda.y && lambda.x < lambda.z)
        {
            lambdaMax = lambda.x;
            plane.x += walk.x;
            lambda.x = (plane.x - org.x) / dir.x;
        }
        if (lambda.y < lambda.x && lambda.y < lambda.z)
        {
            lambdaMax = lambda.y;
            plane.y += walk.y;
            lambda.y = (plane.y - org.y) / dir.y;
        }
        if (lambda.z < lambda.x && lambda.z < lambda.y)
        {
            lambdaMax = lambda.z;
            plane.z += walk.z;
            lambda.z = (plane.z - org.z) / dir.z;
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Build the line segment for this pair of parameters.
        if (lambdaMax - lambdaMin > 0.001)
        {
            float3 segmentMid = org + dir * (lambdaMin + lambdaMax) / 2;
            int3 cellAddress = int3(segmentMid);
            if (cellAddress.x >= cellmin.x && cellAddress.x < cellmax.x &&
                cellAddress.y >= cellmin.y && cellAddress.y < cellmax.y &&
                cellAddress.z >= cellmin.z && cellAddress.z < cellmax.z)
            {
                float4 voxel = userImage.Load(int4(cellAddress, 0));
                accumulate += voxel;
            }
        }
        ////////////////////////////////////////////////////////////////////////////////
        // Advance the plane in this axis for the next run.
        lambdaMin = lambdaMax;
    }
    // Done.
    renderTarget[dispatchThreadId.xy] = accumulate;
})SHADER");
      TRYD3D(m_pDevice->GetID3D11Device()->CreateComputeShader(
          blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
          &shaderCompute));
    }
    CComPtr<ID3D11Buffer> bufferConstants =
        D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                            D3D11_BIND_CONSTANT_BUFFER, sizeof(Matrix44));
    CComPtr<ID3D11ShaderResourceView> srvVoxel;
    {
      CComPtr<ID3D11Texture3D> textureVoxel;
      {
        const uint32_t imageRasterStride = 1 * VOXEL_SIZE;
        const uint32_t imagePlaneStride = imageRasterStride * VOXEL_SIZE;
        std::unique_ptr<uint8_t[]> imageRaw(
            new uint8_t[VOXEL_SIZE * VOXEL_SIZE * VOXEL_SIZE]);
        for (int z = 0; z < VOXEL_SIZE; ++z) {
          for (int y = 0; y < VOXEL_SIZE; ++y) {
            for (int x = 0; x < VOXEL_SIZE; ++x) {
              Vector3 dist = {(float)x / VOXEL_SIZE * 2 - 1,
                              (float)y / VOXEL_SIZE * 2 - 1,
                              (float)z / VOXEL_SIZE * 2 - 1};
              imageRaw[x + imageRasterStride * y + imagePlaneStride * z] = 0;
              float d = Length(dist);
              if (d > 0.9f && d < 1)
                imageRaw[x + imageRasterStride * y + imagePlaneStride * z] = 4;
            }
          }
        }
        {
          D3D11_TEXTURE3D_DESC desc = {};
          desc.Width = VOXEL_SIZE;
          desc.Height = VOXEL_SIZE;
          desc.Depth = VOXEL_SIZE;
          desc.MipLevels = 1;
          desc.Format = DXGI_FORMAT_R8_UNORM;
          desc.Usage = D3D11_USAGE_IMMUTABLE;
          desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
          D3D11_SUBRESOURCE_DATA descData = {};
          descData.pSysMem = imageRaw.get();
          descData.SysMemPitch = imageRasterStride;
          descData.SysMemSlicePitch = imagePlaneStride;
          TRYD3D(m_pDevice->GetID3D11Device()->CreateTexture3D(
              &desc, &descData, &textureVoxel.p));
        }
      }
      {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        desc.Texture2D.MipLevels = 1;
        TRYD3D(m_pDevice->GetID3D11Device()->CreateShaderResourceView(
            textureVoxel, &desc, &srvVoxel.p));
      }
    }
    m_fnRender = [=]() {
      // Get the backbuffer and create a render target from it.
      CComPtr<ID3D11UnorderedAccessView> uavBackbuffer =
          D3D11_Create_UAV_From_SwapChain(m_pDevice->GetID3D11Device(),
                                          m_pSwapChain->GetIDXGISwapChain());
      m_pDevice->GetID3D11DeviceContext()->ClearState();
      // Upload the constant buffer.
      static float angle = 0;
      const float zoom = 2 + cosf(angle);
      Matrix44 transform =
          CreateMatrixTranslate(Vector3{-VOXEL_SIZE / 2, -VOXEL_SIZE / 2, 0}) *
          CreateMatrixScale(Vector3{6, 6, 6}) *
          CreateMatrixTranslate(
              Vector3{RENDERTARGET_WIDTH / 2, RENDERTARGET_HEIGHT / 2, 0});
      transform = Invert(transform);
      angle += 0.01f;
      m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(
          bufferConstants, 0, nullptr, &transform, 0, 0);
      // Beginning of rendering.
      m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
          0, 1, &uavBackbuffer.p, nullptr);
      m_pDevice->GetID3D11DeviceContext()->CSSetShader(shaderCompute, nullptr,
                                                       0);
      m_pDevice->GetID3D11DeviceContext()->CSSetConstantBuffers(
          0, 1, &bufferConstants.p);
      m_pDevice->GetID3D11DeviceContext()->CSSetShaderResources(0, 1,
                                                                &srvVoxel.p);
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
CreateSample_D3D11Voxel(std::shared_ptr<DXGISwapChain> swapchain,
                        std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<ISample>(new Sample_D3D11Voxel(swapchain, device));
}