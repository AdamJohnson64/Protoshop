#pragma once

#include "Core_D3D11.h"
#include <atlbase.h>

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device* device, UINT bindflags, int size);

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device* device, UINT bindflags, int size, const void* data);