#include "Core_D3D.h"
#include <atlbase.h>
#include <d3dcompiler.h>
#include <exception>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

class IncludeFromSourceDirectory : public ID3DInclude {
  std::map<std::string, std::string> filenameToString;
public:
  HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName,
               LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override {
    if (IncludeType == D3D_INCLUDE_LOCAL) {
      auto findCached = filenameToString.find(pFileName);
      if (findCached == filenameToString.end()) {
        // The default working directory in VS2019 is still the directory
        // containing the project file (vcxproj). Any include that we're looking
        // for is expected to be rooted in the source tree. I know this isn't
        // exactly a great way to encode the root but it works under our
        // assumptions.
        std::ifstream includeFileStream(
            (std::string("Source\\") + pFileName).c_str(), 0);
        if (!includeFileStream.is_open()) {
          return STG_E_FILENOTFOUND;
        }
        std::ostringstream bufferAllFile;
        bufferAllFile << includeFileStream.rdbuf();
        filenameToString[pFileName] = bufferAllFile.str();
        findCached = filenameToString.find(pFileName);
      }
      *ppData = findCached->second.c_str();
      *pBytes = findCached->second.length();
      return S_OK;
    }
    return STG_E_FILENOTFOUND;
  }
  HRESULT Close(LPCVOID pData) override { return S_OK; }
};

ID3DBlob *CompileShader(const char *profile, const char *entrypoint,
                        const char *shader) {
  ID3DBlob *pD3DBlobCode = {};
  CComPtr<ID3DBlob> pD3DBlobError;
  IncludeFromSourceDirectory includes;
  D3DCompile(shader, strlen(shader), nullptr, nullptr, &includes, entrypoint,
             profile, 0, 0, &pD3DBlobCode, &pD3DBlobError);
  if (nullptr != pD3DBlobError) {
    throw std::exception(
        reinterpret_cast<const char *>(pD3DBlobError->GetBufferPointer()));
  }
  return pD3DBlobCode;
}