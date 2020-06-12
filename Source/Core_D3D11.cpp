#include "Core_D3D.h"
#include "Core_D3D11.h"

#include <atlbase.h>
#include <d3d11.h>
#include <memory>

#pragma comment(lib, "d3d11.lib")

class Direct3D11DeviceImpl : public Direct3D11Device
{
public:
    Direct3D11DeviceImpl()
    {
        TRYD3D(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &pD3D11Device, nullptr, &pD3D11DeviceContext));
    }
    ID3D11Device* GetID3D11Device()
    {
        return pD3D11Device.p;
    }
    ID3D11DeviceContext* GetID3D11DeviceContext()
    {
        return pD3D11DeviceContext.p;
    }
private:
    CComPtr<ID3D11Device> pD3D11Device;
    CComPtr<ID3D11DeviceContext> pD3D11DeviceContext;
};

std::shared_ptr<Direct3D11Device> CreateDirect3D11Device()
{
    return std::shared_ptr<Direct3D11Device>(new Direct3D11DeviceImpl());
}