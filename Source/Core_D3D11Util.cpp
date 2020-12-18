#include "Core_D3D11Util.h"
#include "Core_D3D.h"
#include "Core_DXGIUtil.h"
#include <array>

D3D11_SAMPLER_DESC Make_D3D11_SAMPLER_DESC_DefaultBorder() {
  D3D11_SAMPLER_DESC desc = {};
  desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  desc.MaxLOD = D3D11_FLOAT32_MAX;
  return desc;
}

D3D11_SAMPLER_DESC Make_D3D11_SAMPLER_DESC_DefaultWrap() {
  D3D11_SAMPLER_DESC desc = {};
  desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  desc.MaxLOD = D3D11_FLOAT32_MAX;
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
Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(ID3D11Texture2D *texture) {
  D3D11_TEXTURE2D_DESC descTexture = {};
  texture->GetDesc(&descTexture);
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  desc.Texture2D.MipLevels = descTexture.MipLevels;
  return desc;
}

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size) {
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = size;
  desc.Usage = D3D11_USAGE_DEFAULT;
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

CComPtr<ID3D11ShaderResourceView> D3D11_Create_SRV(ID3D11DeviceContext *context,
                                                   const IImage *image) {
  CComPtr<ID3D11Device> device;
  context->GetDevice(&device.p);
  CComPtr<ID3D11Texture2D> texture2D;
  {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = image->GetWidth();
    desc.Height = image->GetHeight();
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = image->GetFormat();
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    // Yes, we're being stupid and initializing all mipmaps with the same base
    // image at level 0. We're only doing this because we need to provide
    // something to initialize those levels.
    std::array<D3D11_SUBRESOURCE_DATA, 16> filldata = {};
    for (int i = 0; i < filldata.size(); ++i) {
      filldata[i].pSysMem = image->GetData();
      filldata[i].SysMemPitch = image->GetStride();
      filldata[i].SysMemSlicePitch = filldata[i].SysMemPitch * desc.Height;
    }
    TRYD3D(device->CreateTexture2D(&desc, &filldata[0], &texture2D.p));
  }
  CComPtr<ID3D11ShaderResourceView> srv;
  {
    D3D11_TEXTURE2D_DESC descTexture = {};
    texture2D->GetDesc(&descTexture);
    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels = descTexture.MipLevels;
    TRYD3D(device->CreateShaderResourceView(texture2D, &desc, &srv.p));
  }
  // Now blow away those mipmaps and generate something sensible.
  context->GenerateMips(srv);
  return srv;
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
D3D11_Create_RTV_From_Texture2D(ID3D11Device *device,
                                ID3D11Texture2D *texture) {
  D3D11_RENDER_TARGET_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  CComPtr<ID3D11RenderTargetView> rtv;
  TRYD3D(device->CreateRenderTargetView(texture, &desc, &rtv));
  return rtv;
}

CComPtr<ID3D11UnorderedAccessView>
D3D11_Create_UAV_From_Texture2D(ID3D11Device *device,
                                ID3D11Texture2D *texture) {
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  CComPtr<ID3D11UnorderedAccessView> uav;
  TRYD3D(device->CreateUnorderedAccessView(texture, &desc, &uav));
  return uav;
}

CComPtr<ID3D11Texture2D> D3D11_Create_Texture(ID3D11Device *device,
                                              const IImage *image) {
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = image->GetWidth();
  desc.Height = image->GetHeight();
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = image->GetFormat();
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA data = {};
  data.pSysMem = image->GetData();
  data.SysMemPitch = image->GetStride();
  data.SysMemSlicePitch = image->GetStride() * image->GetHeight();
  CComPtr<ID3D11Texture2D> texture;
  TRYD3D(device->CreateTexture2D(&desc, &data, &texture.p));
  return texture;
}