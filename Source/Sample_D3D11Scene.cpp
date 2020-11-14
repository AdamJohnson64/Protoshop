///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Scene
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a triangle using the simplest shader
// setups and a common camera.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3DCompiler.h"
#include "Sample_D3D11Base.h"
#include "Scene_Camera.h"
#include <atlbase.h>

class Sample_D3D11Scene : public Sample_D3D11Base
{
private:
    CComPtr<ID3D11RasterizerState> m_pD3D11RasterizerState;
    CComPtr<ID3D11Buffer> m_pD3D11BufferConstants;
    CComPtr<ID3D11VertexShader> m_pD3D11VertexShader;
    CComPtr<ID3D11PixelShader> m_pD3D11PixelShader;
    CComPtr<ID3D11InputLayout> m_pD3D11InputLayout;
public:
    Sample_D3D11Scene(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        Sample_D3D11Base(pSwapChain, pDevice)
    {
        {
            D3D11_RASTERIZER_DESC rasterizerdesc = {};
            rasterizerdesc.CullMode = D3D11_CULL_NONE;
            rasterizerdesc.FillMode = D3D11_FILL_WIREFRAME;
            m_pDevice->GetID3D11Device()->CreateRasterizerState(&rasterizerdesc, &m_pD3D11RasterizerState);
        }
        {
            D3D11_BUFFER_DESC descBuffer = {};
            descBuffer.ByteWidth = 1024;
            descBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            descBuffer.StructureByteStride = sizeof(Matrix44);
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&descBuffer, nullptr, &m_pD3D11BufferConstants));
        }
        const char* szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 transform;
};

float4 mainVS(float4 pos : SV_Position) : SV_Position
{
    return mul(transform, pos);
}

float4 mainPS() : SV_Target
{
    return float4(1, 1, 1, 1);
})SHADER";
        CComPtr<ID3DBlob> pD3DBlobCodeVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), nullptr, &m_pD3D11VertexShader));
        ID3DBlob* pD3DBlobCodePS = CompileShader("ps_5_0", "mainPS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(pD3DBlobCodePS->GetBufferPointer(), pD3DBlobCodePS->GetBufferSize(), nullptr, &m_pD3D11PixelShader));
        {
            D3D11_INPUT_ELEMENT_DESC inputdesc = {};
            inputdesc.SemanticName = "SV_Position";
            inputdesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            inputdesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(&inputdesc, 1, pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), &m_pD3D11InputLayout));
        }
    }
    void RenderSample() override
    {
        // Update constant buffer.
        {
            char constants[1024];
            memcpy(constants, &GetCameraViewProjection(), sizeof(Matrix44));
            m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(m_pD3D11BufferConstants, 0, nullptr, constants, 0, 0);
        }
        // Create a vertex buffer.
        CComPtr<ID3D11Buffer> m_pD3D11BufferVertex;
        {
            Vector3 vertices[] = {
                {-10, 0,  10},
                { 10, 0,  10},
                { 10, 0, -10},
                { 10, 0, -10},
                {-10, 0, -10},
                {-10, 0,  10}
            };
            D3D11_BUFFER_DESC bufferdesc = {};
            bufferdesc.ByteWidth = sizeof(vertices);
            bufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
            bufferdesc.BindFlags =  D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = vertices;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&bufferdesc, &data, &m_pD3D11BufferVertex));
        }
        m_pDevice->GetID3D11DeviceContext()->VSSetShader(m_pD3D11VertexShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1, &m_pD3D11BufferConstants.p);
        m_pDevice->GetID3D11DeviceContext()->RSSetState(m_pD3D11RasterizerState);
        m_pDevice->GetID3D11DeviceContext()->PSSetShader(m_pD3D11PixelShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(m_pD3D11InputLayout);
        {
            UINT uStrides[] = { sizeof(Vector3) };
            UINT uOffsets[] = { 0 };
            m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(0, 1, &m_pD3D11BufferVertex.p, uStrides, uOffsets);
        }
        m_pDevice->GetID3D11DeviceContext()->Draw(6, 0);
        m_pDevice->GetID3D11DeviceContext()->Flush();
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Scene(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Scene(pSwapChain, pDevice));
}