#pragma once

#include "Core_D3D11.h"
#include "Core_Math.h"
#include <d3d11.h>
#include <functional>

void CreateNewOpenVRSession(
    std::shared_ptr<Direct3D11Device> deviceD3D11,
    std::function<void(ID3D11Texture2D *, ID3D11DepthStencilView *,
                       const Matrix44 &)>
        fnRender);