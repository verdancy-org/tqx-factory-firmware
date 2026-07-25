#include "factory.hpp"

namespace Factory
{

void UpdateRgb(Hardware& hardware, Feedback& feedback, std::uint32_t now)
{
  if ((now - feedback.last_rgb_ms_) < kRgbFrameMs)
  {
    return;
  }
  feedback.last_rgb_ms_ = now;

  switch (feedback.rgb_mode_)
  {
    case RgbMode::OFF:
      hardware.rgb_.Clear();
      return;
    case RgbMode::STATIC:
      hardware.rgb_.Fill(80U, 16U, 160U);
      (void)hardware.rgb_.Show();
      return;
    case RgbMode::FLOWING:
      for (std::uint8_t i = 0; i < kRgbLedCount; ++i)
      {
        const std::uint8_t phase =
            static_cast<std::uint8_t>(feedback.rgb_phase_ + i * 32U);
        hardware.rgb_.SetPixel(i, phase, static_cast<std::uint8_t>(255U - phase),
                               static_cast<std::uint8_t>(phase >> 1U));
      }
      feedback.rgb_phase_ = static_cast<std::uint8_t>(feedback.rgb_phase_ + 3U);
      (void)hardware.rgb_.Show();
      return;
    case RgbMode::RUNNING:
      hardware.rgb_.ClearPixels();
      hardware.rgb_.SetPixel(
          static_cast<std::uint16_t>((feedback.rgb_phase_ >> 3U) % kRgbLedCount), 255U,
          60U, 0U);
      feedback.rgb_phase_ = static_cast<std::uint8_t>(feedback.rgb_phase_ + 1U);
      (void)hardware.rgb_.Show();
      return;
    case RgbMode::BREATHE:
    {
      const std::uint8_t wave =
          (feedback.rgb_phase_ & 0x40U) != 0U
              ? static_cast<std::uint8_t>(127U - (feedback.rgb_phase_ & 0x3FU) * 2U)
              : static_cast<std::uint8_t>((feedback.rgb_phase_ & 0x3FU) * 2U);
      hardware.rgb_.Fill(wave, wave, 255U);
      feedback.rgb_phase_ = static_cast<std::uint8_t>(feedback.rgb_phase_ + 2U);
      (void)hardware.rgb_.Show();
      return;
    }
  }
}

void StartBuzzerPulse(Hardware& hardware, Feedback& feedback)
{
  if (hardware.buzzer_ == nullptr)
  {
    return;
  }

  const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());
  (void)hardware.buzzer_->SetConfig({2400U});
  (void)hardware.buzzer_->SetDutyCycle(0.25F);
  (void)hardware.buzzer_->Enable();
  feedback.buzzer_active_ = true;
  feedback.buzzer_off_ms_ = now + kBuzzerPulseMs;
}

void UpdateBuzzer(Hardware& hardware, Feedback& feedback, std::uint32_t now)
{
  if (!feedback.buzzer_active_)
  {
    return;
  }

  if (static_cast<std::int32_t>(now - feedback.buzzer_off_ms_) >= 0)
  {
    StopBuzzer(hardware, feedback);
  }
}

void StopBuzzer(Hardware& hardware, Feedback& feedback)
{
  feedback.buzzer_active_ = false;
  if (hardware.buzzer_ != nullptr)
  {
    (void)hardware.buzzer_->SetDutyCycle(0.0F);
    (void)hardware.buzzer_->Disable();
  }
}

}  // namespace Factory
