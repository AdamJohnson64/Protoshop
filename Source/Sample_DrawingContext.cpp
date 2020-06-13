#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3DCompiler.h"
#include "Sample.h"
#include <atlbase.h>
#include <functional>
#include <vector>

struct float2 { float x, y; };

float2 operator*(const float& lhs, const float2& rhs) { return { lhs * rhs.x, lhs * rhs.y }; }
float2 operator*(const float2& lhs, const float& rhs) { return { lhs.x * rhs, lhs.y * rhs }; }
float2 operator+(const float2& lhs, const float2& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
float2 operator-(const float2& lhs, const float2& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y }; }
float Dot(const float2& lhs, const float2& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }
float Length(const float2& lhs) { return sqrtf(Dot(lhs, lhs)); }
float2 Normalize(const float2& lhs) { return lhs * (1 / Length(lhs)); }
float2 Perpendicular(const float2& lhs) { return { -lhs.y, lhs.x }; }

class Sample_DrawingContext : public Sample
{
private:
    std::shared_ptr<DXGISwapChain> m_pSwapChain;
    std::shared_ptr<Direct3D11Device> m_pDevice;
    CComPtr<ID3D11VertexShader> pD3D11VertexShader;
    CComPtr<ID3D11PixelShader> pD3D11PixelShader;
    CComPtr<ID3D11InputLayout> pD3D11InputLayout;
public:
    Sample_DrawingContext(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice) :
        m_pSwapChain(pSwapChain),
        m_pDevice(pDevice)
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Create a vertex shader.
        CComPtr<ID3DBlob> pD3DBlobCodeVS = CompileShader("vs_5_0", R"SHADER(
float4 main(float4 pos : SV_Position) : SV_Position
{
        return float4(-1 + pos.x * 2 / 320, 1 - pos.y * 2 / 240, 0, 1);
})SHADER");
        TRYD3D(m_pDevice->GetID3D11Device()->CreateVertexShader(pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), nullptr, &pD3D11VertexShader));

        ////////////////////////////////////////////////////////////////////////////////
        // Create a pixel shader.
        ID3DBlob* pD3DBlobCodePS = CompileShader("ps_5_0", R"SHADER(
float4 main() : SV_Target
{
        return float4(1, 1, 1, 1);
})SHADER");
        TRYD3D(m_pDevice->GetID3D11Device()->CreatePixelShader(pD3DBlobCodePS->GetBufferPointer(), pD3DBlobCodePS->GetBufferSize(), nullptr, &pD3D11PixelShader));

        ////////////////////////////////////////////////////////////////////////////////
        // Create an input layout.
        {
            D3D11_INPUT_ELEMENT_DESC inputdesc = {};
            inputdesc.SemanticName = "SV_Position";
            inputdesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            inputdesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateInputLayout(&inputdesc, 1, pD3DBlobCodeVS->GetBufferPointer(), pD3DBlobCodeVS->GetBufferSize(), &pD3D11InputLayout));
        }
    }
    void Render()
    {
        // Simple DrawingContext.
        std::vector<float2> vertices;
        std::function<void(const float2&, const float2&)> DCDrawLine = [&](const float2& p0, const float2& p1)
        {
            float2 bitangent = Normalize(Perpendicular(p1 - p0));
            vertices.push_back(p0 - bitangent);
            vertices.push_back(p1 - bitangent);
            vertices.push_back(p1 + bitangent);
            vertices.push_back(p1 + bitangent);
            vertices.push_back(p0 + bitangent);
            vertices.push_back(p0 - bitangent);
        };
        std::function<void()> DCFlush = [&]()
        {
            CComPtr<ID3D11Buffer> pD3D11BufferVertex;
            {
                D3D11_BUFFER_DESC bufferdesc = {};
                bufferdesc.ByteWidth = sizeof(float2) * vertices.size();
                bufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
                bufferdesc.BindFlags =  D3D11_BIND_VERTEX_BUFFER;
                D3D11_SUBRESOURCE_DATA data = {};
                data.pSysMem = &vertices.front();
                TRYD3D(m_pDevice->GetID3D11Device()->CreateBuffer(&bufferdesc, &data, &pD3D11BufferVertex));
            }
            {
                UINT uStrides[] = { sizeof(float2) };
                UINT uOffsets[] = { 0 };
                m_pDevice->GetID3D11DeviceContext()->IASetVertexBuffers(0, 1, &pD3D11BufferVertex.p, uStrides, uOffsets);
            }
            m_pDevice->GetID3D11DeviceContext()->Draw(vertices.size(), 0);
            m_pDevice->GetID3D11DeviceContext()->Flush();
            vertices.clear();
        };
        // Get the backbuffer and create a render target from it.
        CComPtr<ID3D11RenderTargetView> pD3D11RenderTargetView;
        {
            CComPtr<ID3D11Texture2D> pD3D11Texture2D;
            TRYD3D(m_pSwapChain->GetIDXGISwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D));
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            TRYD3D(m_pDevice->GetID3D11Device()->CreateRenderTargetView(pD3D11Texture2D, &rtvDesc, &pD3D11RenderTargetView));
        }
        // Beginning of rendering.
        {
            float color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            m_pDevice->GetID3D11DeviceContext()->ClearRenderTargetView(pD3D11RenderTargetView, color);
        }
        {
            D3D11_VIEWPORT viewportdesc = {};
            viewportdesc.Width = RENDERTARGET_WIDTH;
            viewportdesc.Height = RENDERTARGET_HEIGHT;
            viewportdesc.MaxDepth = 1.0f;
            m_pDevice->GetID3D11DeviceContext()->RSSetViewports(1, &viewportdesc);
        }
        m_pDevice->GetID3D11DeviceContext()->OMSetRenderTargets(1, &pD3D11RenderTargetView.p, nullptr);
        m_pDevice->GetID3D11DeviceContext()->VSSetShader(pD3D11VertexShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->PSSetShader(pD3D11PixelShader, nullptr, 0);
        m_pDevice->GetID3D11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_pDevice->GetID3D11DeviceContext()->IASetInputLayout(pD3D11InputLayout);
        static float angle = 0;
        for (int i = 0; i < 10; ++i)
        {
            const float angleThis = angle + i * 0.1f;
            DCDrawLine({ 160, 120 }, { 160 + sinf(angleThis) * 100, 120 - cosf(angleThis) * 100 });
        }
        angle += 0.01f;
        DCFlush();
        m_pDevice->GetID3D11DeviceContext()->Flush();
        // End of rendering; send to display.
        m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
    }
};

std::shared_ptr<Sample> CreateSample_DrawingContext(std::shared_ptr<DXGISwapChain> pSwapChain, std::shared_ptr<Direct3D11Device> pDevice)
{
    return std::shared_ptr<Sample>(new Sample_DrawingContext(pSwapChain, pDevice));
}