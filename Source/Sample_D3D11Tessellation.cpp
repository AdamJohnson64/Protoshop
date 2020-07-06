#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Sample_D3D11Base.h"
#include <atlbase.h>

class Sample_D3D11Tessellation : public Sample_D3D11Base
{
private:
    CComPtr<ID3D11RasterizerState> m_pD3D11RasterizerState;
    CComPtr<ID3D11VertexShader> m_pD3D11VertexShader;
    CComPtr<ID3D11HullShader> m_pD3D11HullShader;
    CComPtr<ID3D11DomainShader> m_pD3D11DomainShader;
    CComPtr<ID3D11PixelShader> m_pD3D11PixelShader;
    CComPtr<ID3D11InputLayout> m_pD3D11InputLayout;
public:
    Sample_D3D11Tessellation(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) : Sample_D3D11Base(pSwapChain, pDevice)
    {
        {
            D3D11_RASTERIZER_DESC rasterizerdesc = {};
            rasterizerdesc.CullMode = D3D11_CULL_NONE;
            rasterizerdesc.FillMode = D3D11_FILL_WIREFRAME;
            m_pDevice->GetID3D11Device()->CreateRasterizerState(&rasterizerdesc, &m_pD3D11RasterizerState);
        }
        const char* szShaderCode = R"SHADER(
struct ControlPoint
{
    float2 vPosition : CONTROLPOINT;
};

struct PatchConstants
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
};

struct VertexOutput
{
    float4 vPosition : SV_Position;
};

ControlPoint mainVS(ControlPoint controlpoint)
{
    return controlpoint;
}

PatchConstants mainHSConst(InputPatch<ControlPoint, 3> patch)
{   
    PatchConstants Output;
    Output.Edges[2] = Output.Edges[1] = Output.Edges[0] = 10;
    Output.Inside = 10;
    return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("mainHSConst")]
ControlPoint mainHS(InputPatch<ControlPoint, 3> patch, uint i : SV_OutputControlPointID)
{
    return patch[i];
}

[domain("tri")]
VertexOutput mainDS(
    PatchConstants patchconstants, 
    float3 bary : SV_DomainLocation,
    const OutputPatch<ControlPoint, 3> patch)
{
    VertexOutput Output;
    Output.vPosition = float4(patch[0].vPosition * bary.x + patch[1].vPosition * bary.y + patch[2].vPosition * bary.z, 0, 1);
    return Output;    
}

float4 mainPS() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER";
        CComPtr<ID3DBlob> pD3DBlobCodeVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), nullptr, &m_pD3D11VertexShader));
        ID3DBlob* pD3DBlobCodeHS = CompileShader("hs_5_0", "mainHS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreateHullShader(pD3DBlobCodeHS->GetBufferPointer(), pD3DBlobCodeHS->GetBufferSize(), nullptr, &m_pD3D11HullShader));
        ID3DBlob* pD3DBlobCodeDS = CompileShader("ds_5_0", "mainDS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreateDomainShader(pD3DBlobCodeDS->GetBufferPointer(), pD3DBlobCodeDS->GetBufferSize(), nullptr, &m_pD3D11DomainShader));
        ID3DBlob* pD3DBlobCodePS = CompileShader("ps_5_0", "mainPS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(pD3DBlobCodePS->GetBufferPointer(), pD3DBlobCodePS->GetBufferSize(), nullptr, &m_pD3D11PixelShader));

        ////////////////////////////////////////////////////////////////////////////////
        // Create an input layout.
        {
            D3D11_INPUT_ELEMENT_DESC inputdesc = {};
            inputdesc.SemanticName = "CONTROLPOINT";
            inputdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            inputdesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(&inputdesc, 1, pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), &m_pD3D11InputLayout));
        }
    }
    void RenderSample()
    {
        // Create a vertex buffer.
        CComPtr<ID3D11Buffer> m_pD3D11BufferVertex;
        {
            float2 vertices[] = {
                {0, 0},
                {0, 1},
                {1, 0}
            };
            D3D11_BUFFER_DESC bufferdesc = {};
            bufferdesc.ByteWidth = sizeof(float2) * 3;
            bufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
            bufferdesc.BindFlags =  D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = vertices;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&bufferdesc, &data, &m_pD3D11BufferVertex));
        }
        m_pDevice->GetID3D11DeviceContext()->VSSetShader(m_pD3D11VertexShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->HSSetShader(m_pD3D11HullShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->DSSetShader(m_pD3D11DomainShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->RSSetState(m_pD3D11RasterizerState);
        m_pDevice->GetID3D11DeviceContext()->PSSetShader(m_pD3D11PixelShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
        m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(m_pD3D11InputLayout);
        {
            UINT uStrides[] = { sizeof(float2) };
            UINT uOffsets[] = { 0 };
            m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(0, 1, &m_pD3D11BufferVertex.p, uStrides, uOffsets);
        }
        m_pDevice->GetID3D11DeviceContext()->Draw(3, 0);
        m_pDevice->GetID3D11DeviceContext()->Flush();
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Tessellation(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Tessellation(pSwapChain, pDevice));
}