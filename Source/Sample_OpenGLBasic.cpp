#include "Core_OpenGL.h"
#include <functional>
#include <gl/GL.h>
#include <memory>

std::function<void()>
CreateSample_OpenGLBasic(std::shared_ptr<OpenGLDevice> device) {
  return [=]() {
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
  };
}