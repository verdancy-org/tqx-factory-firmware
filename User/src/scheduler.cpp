#include "Scheduler.hpp"

#include "runtime.hpp"

#include "SSD1306.hpp"
#include "W25QXX.hpp"

Hardware::Hardware(LibXR::HardwareContainer& hw, BitsButtonXR& buttons, Dial& dial,
                   DisplaySurface& display, WS2812PWM<4>& rgb)
    : buttons(buttons),
      dial(dial),
      display(display),
      rgb(rgb),
      wireless_link(hw.Find<LibXR::GPIO>("wireless_link")),
      buzzer(hw.Find<LibXR::PWM>("buzzer_pwm")),
      debug_uart(hw.Find<LibXR::UART>("uart_debug")),
      wireless_uart(hw.Find<LibXR::UART>("uart_wireless"))
{
  if (buzzer != nullptr)
  {
    (void)buzzer->SetConfig({2000U});
    (void)buzzer->SetDutyCycle(0.0F);
    (void)buzzer->Disable();
  }

  rgb.Clear();
}

Sensors::Sensors()
    : gyro_topic(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_gyro")),
      accl_topic(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_accl")),
      gyro_sub(gyro_topic),
      accl_sub(accl_topic)
{
  gyro_sub.StartWaiting();
  accl_sub.StartWaiting();
}

Runtime::Runtime(LibXR::HardwareContainer& hw, BitsButtonXR& buttons, Dial& dial,
                 DisplaySurface& display, WS2812PWM<4>& rgb)
    : hardware(hw, buttons, dial, display, rgb)
{
}

void ApplyInitialFeedbackOutputs(Runtime& rt)
{
  rt.hardware.rgb.SetBrightness(rt.feedback.rgb_brightness);
}

void RunFactorySlot(Runtime& rt)
{
  const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());

  DrainInputs(rt.hardware, rt.ui, rt.feedback, rt.game);
  DrainImu(rt.sensors, rt.ui);
  UpdateRgb(rt.hardware, rt.feedback, now);
  UpdateBuzzer(rt.hardware, rt.feedback, now);

  const bool animated = IsAnimatedPage(rt.ui);
  const bool render_requested = rt.ui.render_requested.TestAndClear();
  if (!rt.ui.render_started || render_requested || animated ||
      (now - rt.ui.last_render_ms) >= kIdleRefreshMs)
  {
    rt.ui.render_started = true;
    rt.ui.last_render_ms = now;
    rt.ui.frame++;
    Render(rt);
  }
}

Scheduler::Scheduler(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& manager,
                     BitsButtonXR& buttons, Dial& dial, DisplaySurface& display,
                     WS2812PWM<4>& rgb)
{
  static Runtime factory_runtime(hw, buttons, dial, display, rgb);
  static bool initialized = false;

  runtime_ = &factory_runtime;
  if (!initialized)
  {
    ApplyInitialFeedbackOutputs(*runtime_);
    initialized = true;
  }
  manager.Register(*this);
}

void Scheduler::Update()
{
  if (runtime_ == nullptr)
  {
    return;
  }

  RunFactorySlot(*runtime_);
}
