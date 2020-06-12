#pragma once

#include <d3d11.h>
#include <memory>

class Direct3D11Device
{
public:
    virtual ~Direct3D11Device() = default;
    virtual ID3D11Device* GetID3D11Device() = 0;
    virtual ID3D11DeviceContext* GetID3D11DeviceContext() = 0;
};

std::shared_ptr<Direct3D11Device> CreateDirect3D11Device();