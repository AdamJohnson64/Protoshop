#include "Core_D3D.h"
#include "Core_D3D11Util.h"

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device* device, UINT bindflags, int size)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = size;
    desc.BindFlags =  bindflags;
    desc.StructureByteStride = size;
    CComPtr<ID3D11Buffer> resource;
    TRYD3D(device->CreateBuffer(&desc, nullptr, &resource.p));
    return resource;
}

CComPtr<ID3D11Buffer> D3D11_Create_Buffer(ID3D11Device* device, UINT bindflags, int size, const void* data)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = size;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags =  bindflags;
    desc.StructureByteStride = size;
    D3D11_SUBRESOURCE_DATA filldata = {};
    filldata.pSysMem = data;
    CComPtr<ID3D11Buffer> resource;
    TRYD3D(device->CreateBuffer(&desc, &filldata, &resource.p));
    return resource;
}