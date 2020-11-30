#pragma once

#include "Core_D3D11.h"
#include <atlbase.h>
#include <dxgi1_6.h>

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device* device, UINT bindflags, int size);

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device* device, UINT bindflags, int size, const void* data);

CComPtr<ID3D11Texture2D> D3D11_Create_Texture2D(ID3D11Device* device, DXGI_FORMAT format, int width, int height, const void* data);

CComPtr<ID3D11UnorderedAccessView> D3D11_Create_UAV_From_SwapChain(ID3D11Device* device, IDXGISwapChain* dxgi);