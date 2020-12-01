#pragma once

#include <Windows.h>
#include <memory>

class ISample;

class IWindow {
public:
  virtual HWND GetWindowHandle() = 0;
  virtual void SetSample(std::shared_ptr<ISample> sample) = 0;
};

std::shared_ptr<IWindow> CreateNewWindow();