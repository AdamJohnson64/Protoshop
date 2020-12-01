#include "Core_ISample.h"
#include "Core_IWindow.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include <gl/GL.h>
#include <memory>

class Sample_OpenGLBasic : public Object, public ISample {
private:
  std::shared_ptr<OpenGLDevice> m_pDevice;
  std::shared_ptr<IWindow> m_pWindow;

public:
  Sample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device,
                     std::shared_ptr<IWindow> pWindow)
      : m_pDevice(device), m_pWindow(pWindow) {}
  void Render() override {
    HDC hDC = GetDC(m_pWindow->GetWindowHandle());
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
    int nPixelFormat = ChoosePixelFormat(hDC, &pixelFormatDescriptor);
    if (0 == nPixelFormat)
      throw FALSE;
    // Use this format (this may only be set ONCE).
    if (TRUE != SetPixelFormat(hDC, nPixelFormat, &pixelFormatDescriptor))
      throw FALSE;
    if (TRUE != wglMakeCurrent(hDC, m_pDevice->GetHLGRC()))
      throw FALSE;
    // Establish a viewport as big as the window area.
    RECT rectClient = {};
    GetClientRect(m_pWindow->GetWindowHandle(), &rectClient);
    glViewport(0, 0, rectClient.right - rectClient.left,
               rectClient.bottom - rectClient.top);
    // Clear the backbuffer.
    // Our OpenGL backbuffer will be BLUE; any BLUE on screen means OpenGL has
    // rendered it.
    glClearColor(0.0f, 0.0f, 1.0f, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw a line.
    // This is just to make OpenGL rendering a little more interesting.
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4f(1, 1, 1, 1);
    glVertex2f(-1, -1);
    glVertex2f(1, 1);
    glEnd();
    glFlush();
    if (TRUE != wglMakeCurrent(hDC, nullptr))
      throw FALSE;
  }
};

std::shared_ptr<ISample>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device,
                         std::shared_ptr<IWindow> pWindow) {
  return std::shared_ptr<ISample>(new Sample_OpenGLBasic(device, pWindow));
}