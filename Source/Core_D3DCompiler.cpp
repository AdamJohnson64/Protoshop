#include <exception>

#include <atlbase.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

ID3DBlob* CompileShader(const char* szProfile, const char* szEntryPoint, const char* szShaderCode)
{
    ID3DBlob* pD3DBlobCode = {};
    CComPtr<ID3DBlob> pD3DBlobError;
    D3DCompile(szShaderCode, strlen(szShaderCode), nullptr, nullptr, nullptr, szEntryPoint, szProfile, 0, 0, &pD3DBlobCode, &pD3DBlobError);
    if (nullptr != pD3DBlobError)
    {
        throw std::exception(reinterpret_cast<const char*>(pD3DBlobError->GetBufferPointer()));
    }
    return pD3DBlobCode;
}