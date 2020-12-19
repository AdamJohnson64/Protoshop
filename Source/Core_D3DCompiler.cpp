#include <exception>

#include <atlbase.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

static const char Shader_D3D11_Common[] = R"SHADER(

////////////////////////////////////////////////////////////////////////////////
// Common functions

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

////////////////////////////////////////////////////////////////////////////////
// Common structures
// Most of our extended 3D samples use a common vertex format for simplicity.
// The vertex shader in many cases can also be the same for these samples as
// long as they have access to the constants.

cbuffer ConstantsWorld : register(b0)
{
    float4x4 TransformWorldToClip;
    float4x4 TransformWorldToView;
    float4x4 TransformWorldToClipShadow;
    float4x4 TransformWorldToClipShadowInverse;
    float3 CameraPosition;
    float3 LightPosition;
};

cbuffer ConstantsObject : register(b1)
{
    float4x4 TransformObjectToWorld;
};

struct VertexVS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
};

struct VertexPS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 Texcoord : TEXCOORD;
    float3 WorldPosition : POSITION1;
};

////////////////////////////////////////////////////////////////////////////////
// Common resources
// We keep map types in designated slots so we can indicate them easily.

Texture2D<float4> TextureAlbedoMap : register(t0);
Texture2D<float4> TextureNormalMap : register(t1);
Texture2D<float4> TextureDepthMap : register(t2);
sampler userSampler;

////////////////////////////////////////////////////////////////////////////////
// Common entrypoints

// All of the structure above is captured and initialized here.

VertexPS mainVS(VertexVS vin)
{
    VertexPS vout;
    vout.Position = mul(TransformWorldToClip, mul(TransformObjectToWorld, vin.Position));
    vout.Normal = normalize(mul(TransformObjectToWorld, float4(vin.Normal, 0)).xyz);
    vout.Texcoord = vin.Texcoord;
    vout.WorldPosition = mul(TransformObjectToWorld, vin.Position).xyz;
    return vout;
}

// No object transforms are used here if you want to omit that code.

VertexPS mainVS_NOOBJTRANSFORM(VertexVS vin)
{
    VertexPS vout;
    vout.Position = mul(TransformWorldToClip, vin.Position);
    vout.Normal = vin.Normal;
    vout.Texcoord = vin.Texcoord * 10;
    vout.WorldPosition = vin.Position.xyz;
    return vout;
}

)SHADER";

class InjectedIncludes : public ID3DInclude {
public:
  HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName,
               LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override {
    if (IncludeType == D3D_INCLUDE_LOCAL &&
        !strcmp(pFileName, "Sample_D3D11_Common.inc")) {
      *ppData = Shader_D3D11_Common;
      *pBytes = sizeof(Shader_D3D11_Common) - 1; // Remove NULL terminator!
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
  InjectedIncludes includes;
  D3DCompile(shader, strlen(shader), nullptr, nullptr, &includes, entrypoint,
             profile, 0, 0, &pD3DBlobCode, &pD3DBlobError);
  if (nullptr != pD3DBlobError) {
    throw std::exception(
        reinterpret_cast<const char *>(pD3DBlobError->GetBufferPointer()));
  }
  return pD3DBlobCode;
}