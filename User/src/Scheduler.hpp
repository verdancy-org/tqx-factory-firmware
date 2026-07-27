#pragma once

#include "BitsButtonXR.hpp"
#include "Dial.hpp"
#include "DisplaySurface.hpp"
#include "WS2812PWM.hpp"
#include "app_framework.hpp"

struct Runtime;

class Scheduler : public LibXR::Application
{
 public:
  Scheduler(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& manager,
            BitsButtonXR& buttons, Dial& dial, DisplaySurface& display,
            WS2812PWM<4>& rgb);

  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;

  void Update();
  void OnMonitor() override { Update(); }

 private:
  Runtime* runtime_ = nullptr;
};
