#pragma once

#include "Core_IImage.h"
#include <atlbase.h>
#include <d3d11.h>

D3D11_SAMPLER_DESC Make_D3D11_SAMPLER_DESC_DefaultWrap();

D3D11_VIEWPORT Make_D3D11_VIEWPORT(UINT width, UINT height);

D3D11_SHADER_RESOURCE_VIEW_DESC
Make_D3D11_SHADER_RESOURCE_VIEW_DESC_Texture2D(ID3D11Texture2D *texture);

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size);

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device *device, UINT bindflags,
                                          int size, const void *data);

CComPtr<ID3D11ShaderResourceView> D3D11_Create_SRV(ID3D11DeviceContext *context,
                                                   const IImage *image);

CComPtr<ID3D11Texture2D> D3D11_Create_Texture2D(ID3D11Device *device,
                                                DXGI_FORMAT format, int width,
                                                int height, const void *data);

CComPtr<ID3D11RenderTargetView>
D3D11_Create_RTV_From_Texture2D(ID3D11Device *device, ID3D11Texture2D *texture);

CComPtr<ID3D11UnorderedAccessView>
D3D11_Create_UAV_From_Texture2D(ID3D11Device *device, ID3D11Texture2D *texture);

CComPtr<ID3D11Texture2D> D3D11_Create_Texture(ID3D11Device *device,
                                              const IImage *image);