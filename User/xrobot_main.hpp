#include "app_framework.hpp"
#include "libxr.hpp"

// Module headers
#include "BitsButtonXR.hpp"
#include "W25QXX.hpp"
#include "Dial.hpp"
#include "LSM6DS3TRC.hpp"
#include "WS2812PWM.hpp"
#include "SSD1306.hpp"
#include "DisplaySurface.hpp"
#include "Scheduler.hpp"

static void XRobotMain(LibXR::HardwareContainer &hw) {
  using namespace LibXR;
  ApplicationManager appmgr;

  // Auto-generated module instantiations
  static BitsButtonXR buttons(
      hw,
      appmgr,
      {{"btn_enter", false, {50, 1000, 500, 300}}, {"btn_back", true, {50, 1000, 500, 300}}, {"btn_encoder", false, {50, 1000, 500, 300}}, {"btn_a24", false, {50, 1000, 500, 300}}, {"btn_b24", false, {50, 1000, 500, 300}}},
      {{"nav_back_alt", true, {"btn_back", "btn_a24"}, {50, 1000, 500, 300}}, {"nav_enter_alt", true, {"btn_enter", "btn_b24"}, {50, 1000, 500, 300}}}
  );
  static W25QXX<128> flash(hw, appmgr);
  static Dial dial(hw, appmgr, "dial", "dial_a", "dial_b", 4, false, 5, 80, 16);
  static LSM6DS3TRC imu(
      hw,
      appmgr,
      "i2c_imu",
      106,
      LSM6DS3TRC::DataRate::DATA_RATE_52HZ,
      LSM6DS3TRC::DataRate::DATA_RATE_52HZ,
      LSM6DS3TRC::AcclRange::RANGE_2G,
      LSM6DS3TRC::GyroRange::DPS_2000,
      20,
      "lsm6ds3trc_gyro",
      "lsm6ds3trc_accl"
  );
  static WS2812PWM<4> ws2812(hw, appmgr, "ws2812_waveform", 4, 32, 0);
  static SSD1306 SSD1306_0(hw, appmgr, "i2c_oled", 60, "display_frame", 64);
  static DisplaySurface DisplaySurface_0(hw, appmgr, "display_frame", 33);
  static Scheduler scheduler(hw, appmgr, buttons, dial, DisplaySurface_0, ws2812);

  while (true) {
    Timer::Refresh();
    appmgr.MonitorAll();
    Thread::Sleep(5);
  }
}
