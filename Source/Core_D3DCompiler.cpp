#include <exception>

#include <atlbase.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

static const char Shader_D3D11_Common[] = R"SHADER(
// This is the same cotangent reconstruction described at:
// http://www.thetenthplanet.de/archives/1180
float3x3 cotangent_frame( float3 N, float3 p, float2 uv ) {
  // get edge vectors of the pixel triangle
  float3 dp1 = ddx( p );
  float3 dp2 = ddy( p );
  float2 duv1 = ddx( uv );
  float2 duv2 = ddy( uv );
  // solve the linear system
  float3 dp2perp = cross( dp2, N );
  float3 dp1perp = cross( N, dp1 );
  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
  // construct a scale-invariant frame
  float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
  return float3x3( T * invmax, B * invmax, N );
}
)SHADER";

class InjectedIncludes : public ID3DInclude {
public:
  HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override {
    if (IncludeType == D3D_INCLUDE_LOCAL && !strcmp(pFileName, "Sample_D3D11_Common.inc")) {
      *ppData = Shader_D3D11_Common;
      *pBytes = sizeof(Shader_D3D11_Common) - 1; // This will have a NULL terminator which will screw up parsing; remove it.
      return S_OK;
    }
    return STG_E_FILENOTFOUND;
  }
  HRESULT Close(LPCVOID pData) override {
    return S_OK;
  }
};

ID3DBlob *CompileShader(const char *profile, const char *entrypoint,
                        const char *shader) {
  ID3DBlob *pD3DBlobCode = {};
  CComPtr<ID3DBlob> pD3DBlobError;
  InjectedIncludes includes;
  D3DCompile(shader, strlen(shader), nullptr, nullptr, &includes, entrypoint,
             profile, 0, 0, &pD3DBlobCode, &pD3DBlobError);
  if (nullptr != pD3DBlobError) {
    throw std::exception(
        reinterpret_cast<const char *>(pD3DBlobError->GetBufferPointer()));
  }
  return pD3DBlobCode;
}