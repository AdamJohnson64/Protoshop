#include "Core_D3D11Util.h"
#include "Core_D3D.h"
#include "Core_DXGIUtil.h"

D3D11_SAMPLER_DESC Make_D3D11_SAMPLER_DESC_DefaultWrap() {
  D3D11_SAMPLER_DESC desc = {};
  desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  return desc;
}

D3D11_VIEWPORT Make_D3D11_VIEWPORT(UINT width, UINT height) {
  D3D11_VIEWPORT desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MaxDepth = 1.0f;
  return desc;
}

D3D11_SHADER_RESOURCE_VIEW_DESC
Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(DXGI_FORMAT format) {
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = format;
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = 1;
  return desc;
}

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = size;
  desc.BindFlags = bindflags;
  desc.StructureByteStride = size;
  CComPtr<ID3D11Buffer> resource;
  TRYD3D(device->CreateBuffer(&desc, nullptr, &resource.p));
  return resource;
}

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size, const void *data) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = size;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = bindflags;
  desc.StructureByteStride = size;
  D3D11_SUBRESOURCE_DATA filldata = {};
  filldata.pSysMem = data;
  CComPtr<ID3D11Buffer> resource;
  TRYD3D(device->CreateBuffer(&desc, &filldata, &resource.p));
  return resource;
}

CComPtr<ID3D11Texture2D> D3D11_Create_Texture2D(ID3D11Device *device,
                                                DXGI_FORMAT format, int width,
                                                int height, const void *data) {
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = format;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA filldata = {};
  filldata.pSysMem = data;
  filldata.SysMemPitch = DXGI_FORMAT_Size(format) * width;
  filldata.SysMemSlicePitch = filldata.SysMemPitch * height;
  CComPtr<ID3D11Texture2D> resource;
  TRYD3D(device->CreateTexture2D(&desc, &filldata, &resource.p));
  return resource;
}

CComPtr<ID3D11RenderTargetView>
D3D11_Create_RTV_From_SwapChain(ID3D11Device *device,
                                IDXGISwapChain *swapchain) {
  CComPtr<ID3D11Texture2D> texture;
  TRYD3D(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&texture));
  D3D11_RENDER_TARGET_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  CComPtr<ID3D11RenderTargetView> rtv;
  TRYD3D(device->CreateRenderTargetView(texture, &desc, &rtv));
  return rtv;
}

CComPtr<ID3D11UnorderedAccessView>
D3D11_Create_UAV_From_SwapChain(ID3D11Device *device,
                                IDXGISwapChain *swapchain) {
  CComPtr<ID3D11Texture2D> texture;
  TRYD3D(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&texture));
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  CComPtr<ID3D11UnorderedAccessView> uav;
  TRYD3D(device->CreateUnorderedAccessView(texture, &desc, &uav));
  return uav;
}