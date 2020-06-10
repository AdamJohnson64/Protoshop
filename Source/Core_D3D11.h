#pragma once

#include <d3d11.h>

void InitializeDirect3D11(HWND hWindow);

IDXGISwapChain*         GetIDXGISwapChain();
ID3D11Device*           GetID3D11Device();
ID3D11DeviceContext*    GetID3D11DeviceContext();