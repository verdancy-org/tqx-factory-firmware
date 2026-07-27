#pragma once

#include <cstdint>

#include "BitsButtonXR.hpp"
#include "Dial.hpp"
#include "app_framework.hpp"
#include "event.hpp"

class InputEvents : public LibXR::Application
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

  InputEvents(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& manager,
              BitsButtonXR& buttons, Dial& dial);

  InputEvents(const InputEvents&) = delete;
  InputEvents& operator=(const InputEvents&) = delete;

  LibXR::Event& GetEvent() { return events_; }
  void Update();
  void OnMonitor() override { Update(); }

 private:
  static constexpr std::uint32_t Id(Event event)
  {
    return static_cast<std::uint32_t>(event);
  }

  BitsButtonXR& buttons_;
  Dial& dial_;
  LibXR::Event events_;
};
