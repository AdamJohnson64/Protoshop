////////////////////////////////////////////////////////////////////////////////
// Direct3D 11 Shader Toy
////////////////////////////////////////////////////////////////////////////////
// This sample borrows from the concept of Shader Toy (www.shadertoy.com).
// The only real difference here is we're not on the web and we're HLSL.
// We're going to write this in Win32 because I'm not feeling intelligent today.
// Also, Win32 sucks.
////////////////////////////////////////////////////////////////////////////////

#include "Core_D3D.h"
#include "Core_D3D11.h"
#include "Core_D3D11Util.h"
#include "Core_D3DCompiler.h"
#include "Core_DXGI.h"
#include "Core_Math.h"
#include "Core_Object.h"
#include "Core_Util.h"
#include <Windows.h>
#include <memory>
#include <string>

static const int RENDERTARGET_WIDTH = 1024;
static const int RENDERTARGET_HEIGHT = 1024;

class Sample_D3D11ShaderToy : public Object {
private:
  const int defaultCodeWidth = 512;
  const int defaultErrorHeight = 64;
  HWND m_hWindowHost;
  HWND m_hWindowRender;
  HWND m_hWindowCode;
  HWND m_hWindowError;
  std::shared_ptr<Direct3D11Device> m_pDevice;
  std::shared_ptr<DXGISwapChain> m_pSwapChain;
  CComPtr<ID3D11Buffer> m_pBufferConstants;
  CComPtr<ID3D11ComputeShader> m_pComputeShader;
  __declspec(align(16)) struct Constants {
    Matrix44 TransformClipToWorld;
    Vector2 WindowDimensions;
    float Time;
  };

public:
  Sample_D3D11ShaderToy(std::shared_ptr<Direct3D11Device> device)
      : m_pDevice(device) {
    ////////////////////////////////////////////////////////////////////////////////
    // Create all window classes.
    static bool windowClassInitialized = false;
    if (!windowClassInitialized) {
      {
        WNDCLASS wndclass = {};
        wndclass.lpfnWndProc = WindowProcHost;
        wndclass.cbWndExtra = sizeof(this);
        wndclass.lpszClassName = L"ProtoshopShaderToyHost";
        if (0 == RegisterClass(&wndclass))
          throw FALSE;
      }
      {
        WNDCLASS wndclass = {};
        wndclass.lpfnWndProc = WindowProcRender;
        wndclass.cbWndExtra = sizeof(this);
        wndclass.lpszClassName = L"ProtoshopShaderToyRender";
        if (0 == RegisterClass(&wndclass))
          throw FALSE;
      }
      windowClassInitialized = true;
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Create a window of this class.
    {
      RECT rect = {64, 64, 64 + RENDERTARGET_WIDTH + defaultCodeWidth,
                   64 + RENDERTARGET_HEIGHT};
      AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
      m_hWindowHost = CreateWindow(
          L"ProtoshopShaderToyHost", L"Protoshop Shader Toy (Win32)",
          WS_OVERLAPPEDWINDOW | WS_VISIBLE, rect.left, rect.top,
          rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
          nullptr, this);
    }
    m_hWindowRender = CreateWindow(L"ProtoshopShaderToyRender",
                                   L"Render Window", WS_CHILD | WS_VISIBLE, 0,
                                   0, RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT,
                                   m_hWindowHost, nullptr, nullptr, this);
    m_pSwapChain = CreateDXGISwapChain(m_pDevice, m_hWindowRender);
    std::string strShaderCode =
        R"SHADER(cbuffer Constants
{
    float4x4 TransformClipToWorld;
    float2 WindowDimensions;
    float Time;
};

RWTexture2D<float4> renderTarget;

float lambert(float3 normal)
{
    const float3 light = normalize(float3(1, 1, -1));
    return max(0, dot(normal, light));
}

float phong(float3 reflect, float power)
{
    const float3 light = normalize(float3(1, 1, -1));
    return pow(max(0, dot(light, reflect)), power);
}

float SignedDistanceFunctionBlob(float3 position)
{
    position.y -= 1;
    float distance = sqrt(dot(position, position)) - 1;
    distance = distance + sin(position.x * 10 + Time * 2) * 0.05;
    distance = distance + sin(position.y * 10 + Time * 3) * 0.05;
    distance = distance + sin(position.z * 10 + Time * 5) * 0.05;
    distance /= 2;
    return distance;
}

float SignedDistanceFunctionPlane(float3 position)
{
    return position.y;
}

float SignedDistanceFunction(float3 position)
{
    return min(SignedDistanceFunctionPlane(position), SignedDistanceFunctionBlob(position));
}

bool IsHit(float3 origin, float3 distance)
{
    const float EPSILON = 0.01;
    float walk = 0;
    for (int i = 0; i < 1000; ++i)
    {
        float sdf = SignedDistanceFunction(origin + distance * walk);
        if (walk > 100) return false;
        if (sdf < EPSILON) return true;
        walk = walk + sdf;
    }
    return false;
}

float3 CalculateNormal(float3 position)
{
    const float EPSILON = 0.01;
    float deltax = SignedDistanceFunction(float3(position.x + EPSILON, position.y, position.z)) - SignedDistanceFunction(float3(position.x - EPSILON, position.y, position.z));
    float deltay = SignedDistanceFunction(float3(position.x, position.y + EPSILON, position.z)) - SignedDistanceFunction(float3(position.x, position.y - EPSILON, position.z));
    float deltaz = SignedDistanceFunction(float3(position.x, position.y, position.z + EPSILON)) - SignedDistanceFunction(float3(position.x, position.y, position.z - EPSILON));
    return normalize(float3(deltax, deltay, deltaz));
}

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const float EPSILON = 0.001;
    const float3 light = normalize(float3(1, 1, -1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form up normalized screen coordinates.
    const float2 Normalized = mad(float2(2, -2) / WindowDimensions, float2(dispatchThreadId.xy), float2(-1, 1));
    ////////////////////////////////////////////////////////////////////////////////
    // Form the world ray.
    float4 front = mul(TransformClipToWorld, float4(Normalized.xy, 0, 1));
    front /= front.w;
    float4 back = mul(TransformClipToWorld, float4(Normalized.xy, 1, 1));
    back /= back.w;
    float3 origin = front.xyz;
    float3 direction = normalize(back.xyz - front.xyz);
    ////////////////////////////////////////////////////////////////////////////////
    // Walk the ray
    float walk = 0;
    for (int i = 0; i < 1000; ++i)
    {
        float3 position = origin + direction * walk;
        float sdf = SignedDistanceFunction(position);
        if (walk > 100) break;
        if (sdf < EPSILON)
        {
            float3 normal = CalculateNormal(position);
            float3 illum = lambert(normal) + phong(reflect(direction, normal), 64);
            if (IsHit(position + normal * 0.08, light))
            {
                renderTarget[dispatchThreadId.xy] = float4(illum, 1) * 0.6;
                return;
            }
            renderTarget[dispatchThreadId.xy] = float4(illum, 1);
            return;
        }
        walk = walk + sdf;
    }
    renderTarget[dispatchThreadId.xy] = float4(0.4, 0.2, 0.8, 1);
})SHADER";
    {
      // Because Win32 is dumb we need to replace newline characters.
      size_t findnewline = strShaderCode.find("\n");
      while (findnewline != std::string::npos) {
        strShaderCode.replace(findnewline, 1, "\r\n");
        findnewline = strShaderCode.find("\n", findnewline + 2);
      }
    }
    m_hWindowCode =
        CreateWindowA("EDIT", strShaderCode.c_str(),
                      WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL |
                          ES_AUTOHSCROLL | ES_WANTRETURN,
                      RENDERTARGET_WIDTH, 0, RENDERTARGET_WIDTH,
                      RENDERTARGET_HEIGHT - defaultErrorHeight, m_hWindowHost,
                      nullptr, nullptr, nullptr);
    m_hWindowError = CreateWindowA(
        "EDIT", "Compilation successful.", WS_CHILD | WS_VISIBLE | ES_MULTILINE,
        RENDERTARGET_WIDTH, RENDERTARGET_HEIGHT - defaultErrorHeight,
        defaultCodeWidth, defaultErrorHeight, m_hWindowHost, nullptr, nullptr,
        nullptr);
    ////////////////////////////////////////////////////////////////////////////////
    // Create a constant buffer.
    m_pBufferConstants =
        D3D11_Create_Buffer(m_pDevice->GetID3D11Device(),
                            D3D11_BIND_CONSTANT_BUFFER, sizeof(Constants));
    ////////////////////////////////////////////////////////////////////////////////
    // Create the shader above.
    {
      CComPtr<ID3DBlob> pD3DBlobCodeCS =
          CompileShader("cs_5_0", "main", strShaderCode.c_str());
      m_pComputeShader.Release();
      TRYD3D(m_pDevice->GetID3D11Device()->CreateComputeShader(
          pD3DBlobCodeCS->GetBufferPointer(), pD3DBlobCodeCS->GetBufferSize(),
          nullptr, &m_pComputeShader));
    }
    ////////////////////////////////////////////////////////////////////////////////
    // Start a timer to drive frames.
    SetTimer(m_hWindowRender, 0, 1000 / 30,
             [](HWND hWnd, UINT, UINT_PTR, DWORD) {
               InvalidateRect(hWnd, nullptr, FALSE);
             });
  }
  static LRESULT WINAPI WindowProcHost(HWND hWnd, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam) {
    if (uMsg == WM_CREATE) {
      const CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
      SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return 0;
    }
    Sample_D3D11ShaderToy *window =
        reinterpret_cast<Sample_D3D11ShaderToy *>(GetWindowLongPtr(hWnd, 0));
    if (uMsg == WM_CLOSE) {
      PostQuitMessage(0);
      return 0;
    }
    if (uMsg == WM_COMMAND &&
        reinterpret_cast<HWND>(lParam) == window->m_hWindowCode) {
      if (HIWORD(wParam) == EN_CHANGE) {
        const int buffersize = 65536;
        std::unique_ptr<char[]> code(new char[buffersize]);
        GetWindowTextA(window->m_hWindowCode, code.get(), buffersize);
        try {
          CComPtr<ID3DBlob> pD3DBlobCodeCS =
              CompileShader("cs_5_0", "main", code.get());
          window->m_pComputeShader.Release();
          TRYD3D(window->m_pDevice->GetID3D11Device()->CreateComputeShader(
              pD3DBlobCodeCS->GetBufferPointer(),
              pD3DBlobCodeCS->GetBufferSize(), nullptr,
              &window->m_pComputeShader));
          SetWindowTextA(window->m_hWindowError, "Compilation successful.");
        } catch (std::exception &ex) {
          SetWindowTextA(window->m_hWindowError, ex.what());
        }
        return 0;
      }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  static LRESULT WINAPI WindowProcRender(HWND hWnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam) {
    if (uMsg == WM_CREATE) {
      const CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
      SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
      return 0;
    }
    Sample_D3D11ShaderToy *window =
        reinterpret_cast<Sample_D3D11ShaderToy *>(GetWindowLongPtr(hWnd, 0));
    if (uMsg == WM_CLOSE) {
      PostQuitMessage(0);
      return 0;
    }
    if (uMsg == WM_PAINT) {
      CComPtr<ID3D11Texture2D> textureBackbuffer;
      TRYD3D(window->m_pSwapChain->GetIDXGISwapChain()->GetBuffer(
          0, __uuidof(ID3D11Texture2D), (void **)&textureBackbuffer.p));
      D3D11_TEXTURE2D_DESC descBackbuffer = {};
      textureBackbuffer->GetDesc(&descBackbuffer);
      CComPtr<ID3D11UnorderedAccessView> pUAVTarget =
          D3D11_Create_UAV_From_Texture2D(window->m_pDevice->GetID3D11Device(),
                                          textureBackbuffer);
      window->m_pDevice->GetID3D11DeviceContext()->ClearState();
      // Upload the constant buffer.
      {
        static float t = 0;
        Matrix44 TransformWorldToView = {
            // clang-format off
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, -1, 5, 1
            // clang-format on
        };
        Matrix44 TransformViewToClip = CreateProjection<float>(
            0.01f, 100.0f, 45 * (Pi<float> / 180), 45 * (Pi<float> / 180));
        Constants constants;
        constants.TransformClipToWorld =
            Invert(TransformWorldToView * TransformViewToClip);
        constants.WindowDimensions = {static_cast<float>(RENDERTARGET_WIDTH),
                                      static_cast<float>(RENDERTARGET_HEIGHT)};
        constants.Time = t;
        window->m_pDevice->GetID3D11DeviceContext()->UpdateSubresource(
            window->m_pBufferConstants, 0, nullptr, &constants, 0, 0);
        t += 0.01;
      }
      // Render if we have a shader.
      if (window->m_pComputeShader != nullptr) {
        window->m_pDevice->GetID3D11DeviceContext()->CSSetUnorderedAccessViews(
            0, 1, &pUAVTarget.p, nullptr);
        window->m_pDevice->GetID3D11DeviceContext()->CSSetShader(
            window->m_pComputeShader, nullptr, 0);
        window->m_pDevice->GetID3D11DeviceContext()->CSSetConstantBuffers(
            0, 1, &window->m_pBufferConstants.p);
        window->m_pDevice->GetID3D11DeviceContext()->Dispatch(
            descBackbuffer.Width, descBackbuffer.Height, 1);
        window->m_pDevice->GetID3D11DeviceContext()->ClearState();
        window->m_pDevice->GetID3D11DeviceContext()->Flush();
      }
      window->m_pSwapChain->GetIDXGISwapChain()->Present(0, 0);
      ValidateRect(hWnd, nullptr);
      return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
};

std::shared_ptr<Object>
CreateSample_D3D11ShaderToy(std::shared_ptr<Direct3D11Device> device) {
  return std::shared_ptr<Object>(new Sample_D3D11ShaderToy(device));
}