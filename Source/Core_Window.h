#pragma once

#include "Core_Object.h"
#include "Sample.h"
#include <Windows.h>
#include <memory>

class Window : public Object {
public:
  virtual HWND GetWindowHandle() = 0;
  virtual void SetSample(std::shared_ptr<Sample> sample) = 0;
};

std::shared_ptr<Window> CreateNewWindow();