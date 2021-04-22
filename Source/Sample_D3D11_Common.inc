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

sampler SamplerDefaultWrap : register(s0);
sampler SamplerDefaultBorder : register(s1);

Texture2D<float4> TextureAlbedoMap : register(t0);
Texture2D<float4> TextureNormalMap : register(t1);
Texture2D<float4> TextureDepthMap : register(t2);
Texture2D<float4> TextureMaskMap : register(t2);
Texture2D<float4> TextureShadowMap : register(t3);

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

float CalculateSpotlightShadowMap(float3 worldSpacePosition)
{
    float4 worldInShadowSpaceHomogeneous = mul(TransformWorldToClipShadow, float4(worldSpacePosition, 1));
    float4 worldInShadowSpaceHomogeneousWDIV = worldInShadowSpaceHomogeneous / worldInShadowSpaceHomogeneous.w;
    float spotlight = clamp(1 - pow(sqrt(dot(worldInShadowSpaceHomogeneousWDIV.xy, worldInShadowSpaceHomogeneousWDIV.xy)), 10), 0, 1);
    // Map each world point into the shadow map buffer.
    // This will take clip space and map it to texture UV space.
    // The Y axis is upside down.
    float2 shadowuv = (worldInShadowSpaceHomogeneousWDIV.xy + 1) / 2;
    shadowuv.y = 1 - shadowuv.y;
    // Read the depth value out of the shadow map.
    float depth = TextureShadowMap.Sample(SamplerDefaultWrap, shadowuv).x;
    // If the w adjusted depth of this fragment is beyond the shadow then we are shadowed.
    if (worldInShadowSpaceHomogeneousWDIV.z > (depth + 0.00001))
    {
        return 0;
    }
    return spotlight;
}