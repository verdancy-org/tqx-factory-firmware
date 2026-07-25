#include <cstring>

#include "factory.hpp"

namespace Factory
{

Hardware::Hardware(LibXR::HardwareContainer& hw, BitsButtonXR& buttons, Dial& dial,
                   DisplaySurface& display, WS2812PWM<4>& rgb)
    : buttons_(buttons),
      dial_(dial),
      display_(display),
      rgb_(rgb),
      wireless_link_(hw.template Find<LibXR::GPIO>("wireless_link")),
      buzzer_(hw.template Find<LibXR::PWM>("buzzer_pwm")),
      debug_uart_(hw.template Find<LibXR::UART>("uart_debug")),
      wireless_uart_(hw.template Find<LibXR::UART>("uart_wireless"))
{
  if (buzzer_ != nullptr)
  {
    (void)buzzer_->SetConfig({2000U});
    (void)buzzer_->SetDutyCycle(0.0F);
    (void)buzzer_->Disable();
  }

  rgb_.Clear();
}

Sensors::Sensors()
    : gyro_topic_(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_gyro")),
      accl_topic_(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_accl")),
      gyro_sub_(gyro_topic_),
      accl_sub_(accl_topic_)
{
  gyro_sub_.StartWaiting();
  accl_sub_.StartWaiting();
}

void InitializeFeedbackOutputs(Hardware& hardware, const Feedback& feedback)
{
  hardware.rgb_.SetBrightness(feedback.rgb_brightness_);
}

void RunOnce(Hardware& hardware, Sensors& sensors, Feedback& feedback, Ui& ui, Game& game)
{
  const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());

  DrainInputs(hardware, ui, feedback, game);
  DrainImu(sensors, ui);
  UpdateRgb(hardware, feedback, now);
  UpdateBuzzer(hardware, feedback, now);

  const bool animated = IsAnimatedPage(ui);
  if (!ui.render_started_ || ui.render_requested_ || animated ||
      (now - ui.last_render_ms_) >= kIdleRefreshMs)
  {
    ui.render_started_ = true;
    ui.render_requested_ = false;
    ui.last_render_ms_ = now;
    ui.frame_++;
    Render(hardware, sensors, feedback, ui, game);
  }
}

std::int16_t Wrap(std::int16_t value, std::int16_t count)
{
  if (count <= 0)
  {
    return 0;
  }
  while (value < 0)
  {
    value = static_cast<std::int16_t>(value + count);
  }
  while (value >= count)
  {
    value = static_cast<std::int16_t>(value - count);
  }
  return value;
}

std::int32_t ClipInt(std::int32_t value, std::int32_t lo, std::int32_t hi)
{
  if (value < lo)
  {
    return lo;
  }
  if (value > hi)
  {
    return hi;
  }
  return value;
}

bool IsAlias(const char* got, const char* wanted)
{
  return got != nullptr && wanted != nullptr && std::strcmp(got, wanted) == 0;
}

std::int32_t ScaleToInt(float value, float scale)
{
  return static_cast<std::int32_t>(value * scale);
}

}  // namespace Factory
