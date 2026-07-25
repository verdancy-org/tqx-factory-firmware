#include "runtime.hpp"

void UpdateRgb(Hardware& hardware, Feedback& feedback, std::uint32_t now)
{
  if ((now - feedback.last_rgb_ms) < kRgbFrameMs)
  {
    return;
  }
  feedback.last_rgb_ms = now;

  switch (feedback.rgb_mode)
  {
    case RgbMode::OFF:
      hardware.rgb.Clear();
      return;
    case RgbMode::STATIC:
      hardware.rgb.Fill(80U, 16U, 160U);
      (void)hardware.rgb.Show();
      return;
    case RgbMode::FLOWING:
      for (std::uint8_t i = 0; i < kRgbLedCount; ++i)
      {
        const std::uint8_t phase =
            static_cast<std::uint8_t>(feedback.rgb_phase + i * 32U);
        hardware.rgb.SetPixel(i, phase, static_cast<std::uint8_t>(255U - phase),
                              static_cast<std::uint8_t>(phase >> 1U));
      }
      feedback.rgb_phase = static_cast<std::uint8_t>(feedback.rgb_phase + 3U);
      (void)hardware.rgb.Show();
      return;
    case RgbMode::RUNNING:
      hardware.rgb.ClearPixels();
      hardware.rgb.SetPixel(
          static_cast<std::uint16_t>((feedback.rgb_phase >> 3U) % kRgbLedCount), 255U,
          60U, 0U);
      feedback.rgb_phase = static_cast<std::uint8_t>(feedback.rgb_phase + 1U);
      (void)hardware.rgb.Show();
      return;
    case RgbMode::BREATHE:
    {
      const std::uint8_t wave =
          (feedback.rgb_phase & 0x40U) != 0U
              ? static_cast<std::uint8_t>(127U - (feedback.rgb_phase & 0x3FU) * 2U)
              : static_cast<std::uint8_t>((feedback.rgb_phase & 0x3FU) * 2U);
      hardware.rgb.Fill(wave, wave, 255U);
      feedback.rgb_phase = static_cast<std::uint8_t>(feedback.rgb_phase + 2U);
      (void)hardware.rgb.Show();
      return;
    }
  }
}

void StartBuzzerPulse(Hardware& hardware, Feedback& feedback)
{
  if (hardware.buzzer == nullptr)
  {
    return;
  }

  const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());
  (void)hardware.buzzer->SetConfig({2400U});
  (void)hardware.buzzer->SetDutyCycle(0.25F);
  (void)hardware.buzzer->Enable();
  feedback.buzzer_active = true;
  feedback.buzzer_off_ms = now + kBuzzerPulseMs;
}

void StopBuzzer(Hardware& hardware, Feedback& feedback)
{
  feedback.buzzer_active = false;
  if (hardware.buzzer != nullptr)
  {
    (void)hardware.buzzer->SetDutyCycle(0.0F);
    (void)hardware.buzzer->Disable();
  }
}

void UpdateBuzzer(Hardware& hardware, Feedback& feedback, std::uint32_t now)
{
  if (!feedback.buzzer_active)
  {
    return;
  }

  if (static_cast<std::int32_t>(now - feedback.buzzer_off_ms) >= 0)
  {
    StopBuzzer(hardware, feedback);
  }
}
