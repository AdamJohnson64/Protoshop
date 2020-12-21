///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Light Probe Cross
///////////////////////////////////////////////////////////////////////////////
// This sample renders a light probe from an unfolded HDR cubemap using a
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
#include "Image_HDR.h"
#include "SampleResources.h"
#include <atlbase.h>
#include <cstdint>
#include <functional>
#include <memory>

#include <fstream>

std::function<void(const SampleResourcesD3D11 &)>
CreateSample_D3D11LightProbeCross(std::shared_ptr<Direct3D11Device> device) {
  // Create a compute shader.
  CComPtr<ID3D11ComputeShader> shaderCompute;
  {
    CComPtr<ID3DBlob> blobCS = CompileShader("cs_5_0", "main", R"SHADER(
cbuffer Constants
{
    float4x4 TransformClipToWorld;
    float2 WindowDimensions;
};

RWTexture2D<float4> renderTarget;
Texture2D<float4> userImage;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    ////////////////////////////////////////////////////////////////////////////////
    // Form up normalized screen coordinates.
    const float2 Normalized = mad(float2(2, -2) / WindowDimensions, float2(dispatchThreadId.xy), float2(-1, 1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form the world ray.
    float4 front = mul(TransformClipToWorld, float4(Normalized.xy, 0, 1));
    front /= front.w;
    float4 back = mul(TransformClipToWorld, float4(Normalized.xy, 1, 1));
    back /= back.w;
    float3 origin = front.xyz;
    float3 direction = back.xyz - front.xyz;
    ////////////////////////////////////////////////////////////////////////////////
    // Project the direction onto the unit cube (Chebyshev Norm).
    ////////////////////////////////////////////////////////////////////////////////
    float chebychev = max(abs(direction.x), max(abs(direction.y), abs(direction.z)));
    direction /= chebychev;
    ////////////////////////////////////////////////////////////////////////////////
    // Get the texture and face dimensions.
    ////////////////////////////////////////////////////////////////////////////////
    uint Width, Height, NumberOfLevels;
    userImage.GetDimensions(0, Width, Height, NumberOfLevels);
    uint FaceSize = Height / 4;
    ////////////////////////////////////////////////////////////////////////////////
    // Determine the face to look up.
    ////////////////////////////////////////////////////////////////////////////////
    float4 rgba = float4(1, 0, 0, 0);
    if (abs(direction.x) > abs(direction.y) && abs(direction.x) > abs(direction.z))
    {
        // Major X Direction; left and right of the cube.
        if (direction.x > 0)
        {
            // Right face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 2 + direction.z * FaceSize / 2, FaceSize / 2 + FaceSize * 1 - direction.y * FaceSize / 2, 0));
        }
        else
        {
            // Left face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 0 - direction.z * FaceSize / 2, FaceSize / 2 + FaceSize * 1 - direction.y * FaceSize / 2, 0));
        }

    }
    if (abs(direction.y) > abs(direction.x) && abs(direction.y) > abs(direction.z))
    {
        // Major Y Direction; top and bottom of the cube.
        if (direction.y > 0)
        {
            // Top face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 0 - direction.z * FaceSize / 2, 0));
        }
        else
        {
            // Bottom face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 2 + direction.z * FaceSize / 2, 0));
        }
    }
    if (abs(direction.z) > abs(direction.x) && abs(direction.z) > abs(direction.y))
    {
        // Major Z Direction; front and back of the cube.
        if (direction.z > 0)
        {
            // Back face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 3 + direction.y * FaceSize / 2, 0));
        }
        else
        {
            // Front face.
            rgba = userImage.Load(int3(FaceSize / 2 + FaceSize * 1 + direction.x * FaceSize / 2, FaceSize / 2 + FaceSize * 1 - direction.y * FaceSize / 2, 0));
        }
    }
    renderTarget[dispatchThreadId.xy] = rgba * 2;
})SHADER");
    TRYD3D(device->GetID3D11Device()->CreateComputeShader(
        blobCS->GetBufferPointer(), blobCS->GetBufferSize(), nullptr,
        &shaderCompute));
  }
  __declspec(align(16)) struct Constants {
    Matrix44 TransformClipToWorld;
    Vector2 WindowDimensions;
  };
  CComPtr<ID3D11Buffer> bufferConstants = D3D11_Create_Buffer(
      device->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
  CComPtr<ID3D11ShaderResourceView> srvLightProbe;
  {
    const char *hdr =
        "Submodules\\RenderToyAssets\\Environments\\grace_cross.hdr";
    CComPtr<ID3D11Texture2D> textureLightProbe =
        D3D11_Create_Texture(device->GetID3D11Device(), Load_HDR(hdr).get());
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
      constants.WindowDimensions = {static_cast<float>(RENDERTARGET_WIDTH),
                                    static_cast<float>(RENDERTARGET_HEIGHT)};
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