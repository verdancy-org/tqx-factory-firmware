#include "runtime.hpp"

#include "SSD1306.hpp"
#include "W25QXX.hpp"

namespace
{
Runtime* runtime = nullptr;
}

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

namespace Scheduler
{

void Init(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& manager)
{
  if (runtime != nullptr)
  {
    return;
  }

  static BitsButtonXR buttons(
      hw, manager,
      {
          {"btn_enter", false, {50, 1000, 500, 300}},
          {"btn_back", true, {50, 1000, 500, 300}},
          {"btn_encoder", false, {50, 1000, 500, 300}},
          {"btn_a24", false, {50, 1000, 500, 300}},
          {"btn_b24", false, {50, 1000, 500, 300}},
      },
      {
          {"nav_back_alt", true, {"btn_back", "btn_a24"}, {50, 1000, 500, 300}},
          {"nav_enter_alt", true, {"btn_enter", "btn_b24"}, {50, 1000, 500, 300}},
      });

  static W25QXX<128> flash(hw, manager);
  static Dial dial(hw, manager, "dial", "dial_a", "dial_b", 4, false, 5, 80, 16);
  static LSM6DS3TRC imu(hw, manager, "i2c_imu", 0x6A, LSM6DS3TRC::DataRate::DATA_RATE_52HZ,
                        LSM6DS3TRC::DataRate::DATA_RATE_52HZ,
                        LSM6DS3TRC::AcclRange::RANGE_2G,
                        LSM6DS3TRC::GyroRange::DPS_2000, 20, "lsm6ds3trc_gyro",
                        "lsm6ds3trc_accl");
  static DisplaySurface display(hw, manager, "display_frame", 33);
  static SSD1306 oled(hw, manager, "i2c_oled", 0x3C, "display_frame", 64);
  static WS2812PWM<4> ws2812(hw, manager, "ws2812_waveform", 4, 48, 0);
  static Runtime factory_runtime(hw, buttons, dial, display, ws2812);

  (void)flash;
  (void)imu;
  (void)oled;

  runtime = &factory_runtime;
  ApplyInitialFeedbackOutputs(*runtime);
}

void Update()
{
  if (runtime == nullptr)
  {
    return;
  }

  RunFactorySlot(*runtime);
}

}  // namespace Scheduler
