#include "Core_DXGIUtil.h"
#include <exception>

int DXGI_FORMAT_Size(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32_FLOAT: return sizeof(float[3]);
    case DXGI_FORMAT_R32G32_FLOAT: return sizeof(float[2]);
    case DXGI_FORMAT_R32_UINT: return sizeof(unsigned int);
    case DXGI_FORMAT_B8G8R8A8_UNORM: return sizeof(unsigned int);
    default: throw std::exception("Cannot determine byte size of pixel/vertex format.");
    }
}