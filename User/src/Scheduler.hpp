#pragma once

#include <cstdint>

#include "DisplaySurface.hpp"
#include "WS2812PWM.hpp"
#include "app_framework.hpp"
#include "event.hpp"

struct Runtime;

class Scheduler : public LibXR::Application
{
 public:
  enum class Event : std::uint32_t
  {
    ENTER = 1,
    BACK = 2,
    DIAL_CLOCKWISE = 3,
    DIAL_COUNTER_CLOCKWISE = 4,
    DIAL_FAST_CLOCKWISE = 5,
    DIAL_FAST_COUNTER_CLOCKWISE = 6,
  };

  Scheduler(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& manager,
            DisplaySurface& display, WS2812PWM<4>& rgb);

  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;

  LibXR::Event& GetEvent();
  void Update();
  void OnMonitor() override { Update(); }

 private:
  Runtime* runtime_ = nullptr;
};
