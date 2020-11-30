#include <exception>

#include <atlbase.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

ID3DBlob *CompileShader(const char *profile, const char *entrypoint,
                        const char *shader) {
  ID3DBlob *pD3DBlobCode = {};
  CComPtr<ID3DBlob> pD3DBlobError;
  D3DCompile(shader, strlen(shader), nullptr, nullptr, nullptr, entrypoint,
             profile, 0, 0, &pD3DBlobCode, &pD3DBlobError);
  if (nullptr != pD3DBlobError) {
    throw std::exception(
        reinterpret_cast<const char *>(pD3DBlobError->GetBufferPointer()));
  }
  return pD3DBlobCode;
}