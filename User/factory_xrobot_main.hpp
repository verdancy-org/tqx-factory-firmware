#pragma once

#include "BitsButtonXR.hpp"
#include "Dial.hpp"
#include "DisplaySurface.hpp"
#include "LSM6DS3TRC.hpp"
#include "SSD1306.hpp"
#include "W25QXX.hpp"
#include "WS2812PWM.hpp"
#include "app_framework.hpp"
#include "factory.hpp"
#include "thread.hpp"

inline void XRobotFactoryMain(LibXR::HardwareContainer& hw)
{
  static LibXR::ApplicationManager app;

  static BitsButtonXR buttons(
      hw, app,
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

  static W25QXX<128> flash(hw, app);
  static Dial dial(hw, app, "dial", "dial_a", "dial_b", 4, false, 5, 80, 16);
  static LSM6DS3TRC imu(hw, app, "i2c_imu", 0x6A, LSM6DS3TRC::DataRate::DATA_RATE_52HZ,
                        LSM6DS3TRC::DataRate::DATA_RATE_52HZ,
                        LSM6DS3TRC::AcclRange::RANGE_2G, LSM6DS3TRC::GyroRange::DPS_2000,
                        20, "lsm6ds3trc_gyro", "lsm6ds3trc_accl");
  static DisplaySurface display(hw, app, "display_frame", 33);
  static SSD1306 oled(hw, app, "i2c_oled", 0x3C, "display_frame", 64);
  static WS2812PWM<4> ws2812(hw, app, "ws2812_waveform", 4, 48, 0);
  static Factory::Hardware factory_hardware(hw, buttons, dial, display, ws2812);
  static Factory::Sensors factory_sensors;
  static Factory::Feedback factory_feedback;
  static Factory::Ui factory_ui;
  static Factory::Game factory_game;

  (void)flash;
  (void)imu;
  (void)oled;
  Factory::InitializeFeedbackOutputs(factory_hardware, factory_feedback);
  while (true)
  {
    app.MonitorAll();
    Factory::RunOnce(factory_hardware, factory_sensors, factory_feedback, factory_ui,
                     factory_game);
    LibXR::Thread::Sleep(5);
  }
}
