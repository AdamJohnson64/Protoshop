#include "Core_OpenGL.h"
#include "Core_Window.h"

#include <Windows.h>
#include <gl/GL.h>
#include <memory>

#pragma comment(lib, "Opengl32.lib")

class OpenGLDeviceImpl : public OpenGLDevice {
public:
  OpenGLDeviceImpl() {
    m_hDeviceContext = CreateDC(L"DISPLAY", nullptr, nullptr, nullptr);
    // Describe a 32-bit ARGB pixel format.
    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {};
    pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
    pixelFormatDescriptor.nVersion = 1;
    pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pixelFormatDescriptor.cColorBits = 32;
    pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
    pixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;
    // Choose a pixel format that is a close approximation of this suggested
    // format.
    int nPixelFormat =
        ChoosePixelFormat(m_hDeviceContext, &pixelFormatDescriptor);
    if (0 == nPixelFormat)
      throw FALSE;
    // Use this format (this may only be set ONCE).
    if (TRUE !=
        SetPixelFormat(m_hDeviceContext, nPixelFormat, &pixelFormatDescriptor))
      throw FALSE;
    // Create a GL context for this window.
    m_hOpenGLResourceContext = wglCreateContext(m_hDeviceContext);
    if (nullptr == m_hOpenGLResourceContext)
      throw FALSE;
  }
  HGLRC GetHLGRC() override { return m_hOpenGLResourceContext; }

private:
  HDC m_hDeviceContext;
  HGLRC m_hOpenGLResourceContext;
};

std::shared_ptr<OpenGLDevice> CreateOpenGLDevice() {
  return std::shared_ptr<OpenGLDevice>(new OpenGLDeviceImpl());
}