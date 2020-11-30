///////////////////////////////////////////////////////////////////////////////
// Sample - Direct3D 11 Mesh
///////////////////////////////////////////////////////////////////////////////
// This sample demonstrates how to render a triangle using the simplest shader
// setups and a common camera.
///////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_Util.h"
#include "Sample_D3D11Base.h"
#include "Scene_Camera.h"
#include "Scene_InstanceTable.h"
#include <array>
#include <atlbase.h>
#include <vector>

class Sample_D3D11Scene : public Sample_D3D11Base
{
private:
    CComPtr<ID3D11Texture2D> m_pD3D11DepthStencilResource;
    CComPtr<ID3D11DepthStencilView> m_pD3D11DepthStencilView;
    CComPtr<ID3D11RasterizerState> m_pD3D11RasterizerState;
    CComPtr<ID3D11VertexShader> m_pD3D11VertexShader;
    CComPtr<ID3D11PixelShader> m_pD3D11PixelShader;
    CComPtr<ID3D11InputLayout> m_pD3D11InputLayout;
    struct Constants
    {
        Matrix44 TransformWorldToClip;
        Matrix44 TransformObjectToWorld;
    };
    struct VertexFormat
    {
        Vector3 Position;
        Vector3 Normal;
    };
public:
    Sample_D3D11Scene(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device) :
        Sample_D3D11Base(swapchain, device)
    {
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = RENDERTARGET_WIDTH;
            desc.Height = RENDERTARGET_HEIGHT;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            desc.SampleDesc.Count = 1;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateTexture2D(&desc, nullptr, &m_pD3D11DepthStencilResource));
        }
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateDepthStencilView(m_pD3D11DepthStencilResource, &desc, &m_pD3D11DepthStencilView));
        }
        {
            D3D11_RASTERIZER_DESC rasterizerdesc = {};
            rasterizerdesc.CullMode = D3D11_CULL_BACK;
            rasterizerdesc.FillMode = D3D11_FILL_SOLID;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateRasterizerState(&rasterizerdesc, &m_pD3D11RasterizerState));
        }
        const char* szShaderCode = R"SHADER(
cbuffer Constants
{
    float4x4 TransformWorldToClip;
    float4x4 TransformObjectToWorld;
};

struct VertexVS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
};

struct VertexPS
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float3 WorldPosition : POSITION1;
};

VertexPS mainVS(VertexVS vin)
{
    VertexPS vout;
    vout.Position = mul(TransformWorldToClip, vin.Position);
    vout.Normal = normalize(mul(TransformObjectToWorld, float4(vin.Normal, 0)).xyz);
    vout.WorldPosition = mul(TransformObjectToWorld, vin.Position).xyz;
    return vout;
}

float4 mainPS(VertexPS vin) : SV_Target
{
    float3 p = vin.WorldPosition;
    float3 n = vin.Normal;
    float3 l = normalize(float3(4, 1, -4) - p);
    float illum = dot(n, l);
    return float4(illum, illum, illum, 1);
})SHADER";
        CComPtr<ID3DBlob> pD3DBlobCodeVS = CompileShader("vs_5_0", "mainVS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), nullptr, &m_pD3D11VertexShader));
        ID3DBlob* pD3DBlobCodePS = CompileShader("ps_5_0", "mainPS", szShaderCode);
        TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(pD3DBlobCodePS->GetBufferPointer(), pD3DBlobCodePS->GetBufferSize(), nullptr, &m_pD3D11PixelShader));
        {
            std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputdesc = {};
            inputdesc[0].SemanticName = "SV_Position";
            inputdesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
            inputdesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            inputdesc[0].AlignedByteOffset = 0;
            inputdesc[1].SemanticName = "NORMAL";
            inputdesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
            inputdesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            inputdesc[1].AlignedByteOffset = sizeof(Vector3);
            TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(&inputdesc[0], 2, pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), &m_pD3D11InputLayout));
        }
    }
    void RenderSample() override
    {
        m_pDevice->GetID3D11DeviceContext()->ClearDepthStencilView(m_pD3D11DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);
        CComPtr<ID3D11RenderTargetView> pRenderTargetViewOld;
        m_pDevice->GetID3D11DeviceContext()->OMGetRenderTargets(1, &pRenderTargetViewOld.p, nullptr);
        m_pDevice->GetID3D11DeviceContext()->OMSetRenderTargets(1, &pRenderTargetViewOld.p, m_pD3D11DepthStencilView.p);
        std::shared_ptr<InstanceTable> scene(InstanceTable::Default());
        // Create a vertex buffer.
        m_pDevice->GetID3D11DeviceContext()->VSSetShader(m_pD3D11VertexShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->RSSetState(m_pD3D11RasterizerState);
        m_pDevice->GetID3D11DeviceContext()->PSSetShader(m_pD3D11PixelShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(m_pD3D11InputLayout);
        std::vector<CComPtr<ID3D11Buffer>> m_pD3D11BufferVertex;
        std::vector<CComPtr<ID3D11Buffer>> m_pD3D11BufferIndex;
        for (int meshIndex = 0; meshIndex < scene->Meshes.size(); ++meshIndex)
        {

            {
                // Create the vertex buffer.
                int sizeVertex = sizeof(VertexFormat) * scene->Meshes[meshIndex]->getVertexCount();
                std::unique_ptr<int8_t[]> vertices(new int8_t[sizeVertex]);
                scene->Meshes[meshIndex]->copyVertices(reinterpret_cast<Vector3*>(vertices.get()), sizeof(VertexFormat));
                scene->Meshes[meshIndex]->copyNormals(reinterpret_cast<Vector3*>(vertices.get() + sizeof(Vector3)), sizeof(VertexFormat));
                m_pD3D11BufferVertex.push_back(D3D11_Create_Buffer(m_pDevice->GetID3D11Device(), D3D11_BIND_VERTEX_BUFFER, sizeVertex, vertices.get()));
            }
            {
                // Create the index buffer.
                int sizeIndices = sizeof(int32_t) * scene->Meshes[meshIndex]->getIndexCount();
                std::unique_ptr<int8_t[]> indices(new int8_t[sizeIndices]);
                scene->Meshes[meshIndex]->copyIndices(reinterpret_cast<uint32_t*>(indices.get()), sizeof(uint32_t));
                m_pD3D11BufferIndex.push_back(D3D11_Create_Buffer(m_pDevice->GetID3D11Device(), D3D11_BIND_INDEX_BUFFER, sizeIndices, indices.get()));
            }
        }
        // Now go through all the instances, create the constants, and draw.
        std::vector<CComPtr<ID3D11Buffer>> m_pD3D11BufferConstant;
        for (int instanceIndex = 0; instanceIndex < scene->Instances.size(); ++instanceIndex)
        {
            const Instance& instance = scene->Instances[instanceIndex];
            // Create the constant buffer.
            {
                Constants constants = {};
                constants.TransformWorldToClip = instance.TransformObjectToWorld * GetCameraWorldToClip();
                constants.TransformObjectToWorld = instance.TransformObjectToWorld;
                m_pD3D11BufferConstant.push_back(D3D11_Create_Buffer(m_pDevice->GetID3D11Device(), D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants), &constants));
            }
            m_pDevice->GetID3D11DeviceContext()->VSSetConstantBuffers(0, 1, &m_pD3D11BufferConstant[instanceIndex].p);
            {
                const UINT uStrides[] = { sizeof(VertexFormat) };
                const UINT uOffsets[] = { 0 };
                m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(0, 1, &m_pD3D11BufferVertex[instance.GeometryIndex].p, uStrides, uOffsets);
            }
            m_pDevice->GetID3D11DeviceContext()->IASetIndexBuffer(m_pD3D11BufferIndex[instance.GeometryIndex].p, DXGI_FORMAT_R32_UINT, 0);
            m_pDevice->GetID3D11DeviceContext()->DrawIndexed(scene->Meshes[instance.GeometryIndex]->getIndexCount(), 0, 0);
        }
        m_pDevice->GetID3D11DeviceContext()->Flush();
    }
};

std::shared_ptr<Sample> CreateSample_D3D11Scene(std::shared_ptr<DXGISwapChain> swapchain, std::shared_ptr<Direct3D11Device> device)
{
    return std::shared_ptr<Sample>(new Sample_D3D11Scene(swapchain, device));
}