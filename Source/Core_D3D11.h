#pragma once

#include <d3d11.h>
#include <dxgi1_6.h>

void InitializeDirect3D11(HWND hWindow);

IDXGISwapChain1*        GetIDXGISwapChain();
ID3D11Device*           GetID3D11Device();
ID3D11DeviceContext*    GetID3D11DeviceContext();