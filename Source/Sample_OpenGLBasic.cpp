#include "Core_ISample.h"
#include "Core_IWindow.h"
#include "Core_Object.h"
#include "Core_OpenGL.h"
#include <functional>
#include <gl/GL.h>
#include <memory>

class Sample_OpenGLBasic : public Object, public ISample {
private:
  std::function<void()> m_fnRender;
  std::shared_ptr<IWindow> m_Window;
  HDC m_hDC;

public:
  Sample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device,
                     std::shared_ptr<IWindow> window)
      : m_Window(window) {
    m_hDC = GetDC(m_Window->GetWindowHandle());
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
    int nPixelFormat = ChoosePixelFormat(m_hDC, &pixelFormatDescriptor);
    if (0 == nPixelFormat)
      throw FALSE;
    // Use this format (this may only be set ONCE).
    if (TRUE != SetPixelFormat(m_hDC, nPixelFormat, &pixelFormatDescriptor))
      throw FALSE;
    if (TRUE != wglMakeCurrent(m_hDC, device->GetHLGRC()))
      throw FALSE;
    m_fnRender = [=]() {
      // Establish a viewport as big as the window area.
      RECT rectClient = {};
      GetClientRect(m_Window->GetWindowHandle(), &rectClient);
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
      if (TRUE != wglMakeCurrent(m_hDC, nullptr))
        throw FALSE;
    };
  }
  ~Sample_OpenGLBasic() { ReleaseDC(m_Window->GetWindowHandle(), m_hDC); }
  void Render() override { m_fnRender(); }
};

std::shared_ptr<ISample>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device,
                         std::shared_ptr<IWindow> pWindow) {
  return std::shared_ptr<ISample>(new Sample_OpenGLBasic(device, pWindow));
}