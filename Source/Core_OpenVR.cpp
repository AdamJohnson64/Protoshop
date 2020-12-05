#include "Core_OpenVR.h"
#include "Core_D3D.h"
#include <array>
#include <atlbase.h>
#include <openvr.h>

Matrix44 ConvertMatrix(vr::HmdMatrix34_t &m) {
  Matrix44 o = {};
  // clang-format off
  o.M11 = m.m[0][0]; o.M12 = m.m[1][0]; o.M13 = m.m[2][0]; o.M14 = 0;
  o.M21 = m.m[0][1]; o.M22 = m.m[1][1]; o.M23 = m.m[2][1]; o.M24 = 0;
  o.M31 = m.m[0][2]; o.M32 = m.m[1][2]; o.M33 = m.m[2][2]; o.M34 = 0;
  o.M41 = m.m[0][3]; o.M42 = m.m[1][3]; o.M43 = m.m[2][3]; o.M44 = 1;
  // clang-format on
  return o;
}

Matrix44 ConvertMatrix(vr::HmdMatrix44_t &m) {
  Matrix44 o = {};
  // clang-format off
  o.M11 = m.m[0][0]; o.M12 = m.m[1][0]; o.M13 = m.m[2][0]; o.M14 = m.m[3][0];
  o.M21 = m.m[0][1]; o.M22 = m.m[1][1]; o.M23 = m.m[2][1]; o.M24 = m.m[3][1];
  o.M31 = m.m[0][2]; o.M32 = m.m[1][2]; o.M33 = m.m[2][2]; o.M34 = m.m[3][2];
  o.M41 = m.m[0][3]; o.M42 = m.m[1][3]; o.M43 = m.m[2][3]; o.M44 = m.m[3][3];
  // clang-format on
  return o;
}

void CreateNewOpenVRSession(
    std::shared_ptr<Direct3D11Device> device,
    std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                       const Matrix44 &)>
        fnRender) {

  vr::IVRSystem *vrsystem = vr::VR_Init(
      nullptr, vr::EVRApplicationType::VRApplication_Scene, nullptr);
  if (vrsystem == nullptr) {
    throw std::exception("OpenVR failed to initialize.");
  }
  vr::IVRCompositor *vrcompositor = vr::VRCompositor();
  uint32_t viewwidth, viewheight;
  vrsystem->GetRecommendedRenderTargetSize(&viewwidth, &viewheight);
  CComPtr<ID3D11Texture2D> textureVR;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = viewwidth;
    desc.Height = viewheight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                      &textureVR.p));
  }
  CComPtr<ID3D11Texture2D> textureDepth;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = viewwidth;
    desc.Height = viewheight;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    TRYD3D(device->GetID3D11Device()->CreateTexture2D(&desc, nullptr,
                                                      &textureDepth));
  }
  CComPtr<ID3D11DepthStencilView> dsvDepth;
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    TRYD3D(device->GetID3D11Device()->CreateDepthStencilView(textureDepth,
                                                             &desc, &dsvDepth));
  }
  std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount>
      poseSystem = {};
  std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> poseGame =
      {};
  while (true) {
    if (vr::VRCompositorError_None !=
        vrcompositor->WaitGetPoses(&poseSystem[0], poseSystem.size(),
                                   &poseGame[0], poseGame.size()))
      continue;
    if (!poseSystem[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
      continue;

    Matrix44 world = CreateMatrixScale(Vector3{0.5f, 0.5f, -0.5f});
    Matrix44 head = Invert(ConvertMatrix(
        poseSystem[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking));
    Matrix44 transformHeadToLeftEye =
        Invert(ConvertMatrix(vrsystem->GetEyeToHeadTransform(vr::Eye_Left)));
    Matrix44 transformHeadToRightEye =
        Invert(ConvertMatrix(vrsystem->GetEyeToHeadTransform(vr::Eye_Right)));
    Matrix44 projectionLeftEye = ConvertMatrix(
        vrsystem->GetProjectionMatrix(vr::Eye_Left, 0.001f, 100.0f));
    Matrix44 projectionRightEye = ConvertMatrix(
        vrsystem->GetProjectionMatrix(vr::Eye_Right, 0.001f, 100.0f));

    fnRender(textureVR, dsvDepth,
             world * head * transformHeadToLeftEye * projectionLeftEye);
    {
      vr::Texture_t texture = {};
      texture.handle = textureVR.p;
      texture.eColorSpace = vr::ColorSpace_Auto;
      texture.eType = vr::TextureType_DirectX;
      vrcompositor->Submit(vr::Eye_Left, &texture);
    }
    fnRender(textureVR, dsvDepth,
             world * head * transformHeadToRightEye * projectionRightEye);
    {
      vr::Texture_t texture = {};
      texture.handle = textureVR.p;
      texture.eColorSpace = vr::ColorSpace_Auto;
      texture.eType = vr::TextureType_DirectX;
      vrcompositor->Submit(vr::Eye_Right, &texture);
    }
  }
}