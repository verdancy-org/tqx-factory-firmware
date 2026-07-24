#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "BitsButtonXR.hpp"
#include "Dial.hpp"
#include "DisplaySurface.hpp"
#include "LSM6DS3TRC.hpp"
#include "WS2812PWM.hpp"
#include "app_framework.hpp"
#include "gpio.hpp"
#include "message.hpp"
#include "pwm.hpp"
#include "timebase.hpp"
#include "uart.hpp"

class FactoryFirmware : public LibXR::Application
{
 public:
  FactoryFirmware(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& app,
                  BitsButtonXR& buttons, Dial& dial, DisplaySurface& display,
                  WS2812PWM<4>& rgb)
      : buttons_(buttons),
        dial_(dial),
        display_(display),
        rgb_(rgb),
        gyro_topic_(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_gyro")),
        accl_topic_(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_accl")),
        gyro_sub_(gyro_topic_),
        accl_sub_(accl_topic_),
        wireless_link_(hw.template Find<LibXR::GPIO>("wireless_link")),
        buzzer_(hw.template Find<LibXR::PWM>("buzzer_pwm")),
        debug_uart_(hw.template Find<LibXR::UART>("uart_debug")),
        wireless_uart_(hw.template Find<LibXR::UART>("uart_wireless"))
  {
    gyro_sub_.StartWaiting();
    accl_sub_.StartWaiting();

    if (buzzer_ != nullptr)
    {
      (void)buzzer_->SetConfig({2000U});
      (void)buzzer_->SetDutyCycle(0.0F);
      (void)buzzer_->Disable();
    }

    rgb_.SetBrightness(rgb_brightness_);
    rgb_.Clear();
    app.Register(*this);
  }

  void OnMonitor() override
  {
    const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());

    DrainInputs();
    DrainImu();
    UpdateRgb(now);
    UpdateBuzzer(now);

    const bool animated = IsAnimatedPage();
    if (!render_started_ || render_requested_ || animated ||
        (now - last_render_ms_) >= kIdleRefreshMs)
    {
      render_started_ = true;
      render_requested_ = false;
      last_render_ms_ = now;
      frame_++;
      Render();
    }
  }

 private:
  enum class Page : std::uint8_t
  {
    MAIN_MENU,
    SETTINGS,
    RGB,
    GYRO,
    UART,
    ROBOT,
    GAMES_MENU,
    THEME,
    MORE,
    GAME_DINO,
    GAME_BIRD,
    GAME_PLANE,
    GAME_BRICK,
    GAME_SNAKE,
  };

  enum class RgbMode : std::uint8_t
  {
    OFF,
    STATIC,
    FLOWING,
    RUNNING,
    BREATHE,
  };

  struct MenuItem
  {
    const char* label;
    Page page;
  };

  static constexpr std::uint32_t kIdleRefreshMs = 250U;
  static constexpr std::uint32_t kRgbFrameMs = 45U;
  static constexpr std::uint32_t kBuzzerPulseMs = 35U;
  static constexpr std::uint8_t kRgbLedCount = 4U;

  static constexpr MenuItem kMainMenu[] = {
      {"Settings", Page::SETTINGS}, {"RGB LED", Page::RGB}, {"Gyro", Page::GYRO},
      {"UART", Page::UART},         {"Robot", Page::ROBOT}, {"Games", Page::GAMES_MENU},
      {"Theme", Page::THEME},       {"More", Page::MORE},
  };

  static constexpr MenuItem kGameMenu[] = {
      {"Dino", Page::GAME_DINO},   {"Bird", Page::GAME_BIRD},
      {"Plane", Page::GAME_PLANE}, {"Brick", Page::GAME_BRICK},
      {"Snake", Page::GAME_SNAKE},
  };

  static constexpr const char* kRgbModes[] = {
      "Off", "Static", "Flowing", "Running", "Breathe",
  };

  static constexpr const char* kSettings[] = {
      "OLED bright",
      "Sound",
      "RGB bright",
      "Frame rate",
  };

  template <typename T, std::size_t N>
  static constexpr std::size_t CountOf(const T (&)[N])
  {
    return N;
  }

  static std::int16_t Wrap(std::int16_t value, std::int16_t count)
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

  static std::int32_t ClipInt(std::int32_t value, std::int32_t lo, std::int32_t hi)
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

  static bool IsAlias(const char* got, const char* wanted)
  {
    return got != nullptr && wanted != nullptr && std::strcmp(got, wanted) == 0;
  }

  static std::int32_t ScaleToInt(float value, float scale)
  {
    return static_cast<std::int32_t>(value * scale);
  }

  MonoCanvas::Color Fg() const
  {
    return dark_mode_ ? MonoCanvas::Color::CLEAR : MonoCanvas::Color::SET;
  }

  MonoCanvas::Color Bg() const
  {
    return dark_mode_ ? MonoCanvas::Color::SET : MonoCanvas::Color::CLEAR;
  }

  MonoCanvas& Canvas() { return display_.GetCanvas(); }

  void ClearCanvas() { Canvas().Clear(dark_mode_); }

  void Text(std::int16_t x, std::int16_t y, const char* text)
  {
    Canvas().DrawText(x, y, text, Fg());
  }

  void TextSelected(std::int16_t y, const char* text)
  {
    Canvas().FillRect(0, static_cast<std::int16_t>(y - 1), 128, 9, Fg());
    Canvas().DrawText(8, y, text, Bg());
  }

  void Header(const char* title)
  {
    Canvas().DrawText(0, 0, title, Fg());
    Canvas().DrawHorizontalLine(0, 9, 128, Fg());
  }

  void Footer(const char* left, const char* right)
  {
    Canvas().DrawHorizontalLine(0, 54, 128, Fg());
    Canvas().DrawText(0, 56, left, Fg());
    if (right != nullptr)
    {
      const auto width = MonoCanvas::TextWidth(right);
      Canvas().DrawText(static_cast<std::int16_t>(128 - width), 56, right, Fg());
    }
  }

  void DrainInputs()
  {
    BitsButtonXR::ButtonEventResult btn{};
    while (buttons_.GetEventResult(btn))
    {
      last_button_ = btn.key_alias;
      if (btn.event_type != BitsButtonXR::ButtonEvent::PRESSED)
      {
        continue;
      }

      if (IsEnter(btn.key_alias))
      {
        OnEnter();
      }
      else if (IsBack(btn.key_alias))
      {
        OnBack();
      }
      render_requested_ = true;
    }

    Dial::EventResult dial{};
    while (dial_.GetEventResult(dial))
    {
      ApplyDial(dial.delta);
      render_requested_ = true;
    }
  }

  void DrainImu()
  {
    if (gyro_sub_.Available())
    {
      last_gyro_ = gyro_sub_.GetData();
      gyro_valid_ = true;
      gyro_sub_.StartWaiting();
      if (page_ == Page::GYRO)
      {
        render_requested_ = true;
      }
    }

    if (accl_sub_.Available())
    {
      last_accl_ = accl_sub_.GetData();
      accl_valid_ = true;
      accl_sub_.StartWaiting();
      if (page_ == Page::GYRO)
      {
        render_requested_ = true;
      }
    }
  }

  bool IsEnter(const char* alias) const
  {
    return IsAlias(alias, "btn_enter") || IsAlias(alias, "btn_encoder") ||
           IsAlias(alias, "btn_b24") || IsAlias(alias, "nav_enter_alt");
  }

  bool IsBack(const char* alias) const
  {
    return IsAlias(alias, "btn_back") || IsAlias(alias, "btn_a24") ||
           IsAlias(alias, "nav_back_alt");
  }

  void OnEnter()
  {
    switch (page_)
    {
      case Page::MAIN_MENU:
        page_ = kMainMenu[main_selected_].page;
        break;
      case Page::GAMES_MENU:
        page_ = kGameMenu[game_selected_].page;
        ResetGame();
        break;
      case Page::SETTINGS:
        ActivateSetting();
        break;
      case Page::RGB:
        rgb_brightness_ = static_cast<std::uint8_t>(rgb_brightness_ + 32U);
        if (rgb_brightness_ < 16U)
        {
          rgb_brightness_ = 16U;
        }
        rgb_.SetBrightness(rgb_brightness_);
        break;
      case Page::THEME:
        dark_mode_ = !dark_mode_;
        break;
      case Page::ROBOT:
        robot_mood_ = static_cast<std::uint8_t>(
            Wrap(static_cast<std::int16_t>(robot_mood_ + 1), 3));
        break;
      case Page::GAME_DINO:
      case Page::GAME_BIRD:
        game_jump_ = true;
        break;
      default:
        break;
    }
  }

  void OnBack()
  {
    switch (page_)
    {
      case Page::MAIN_MENU:
        break;
      case Page::GAME_DINO:
      case Page::GAME_BIRD:
      case Page::GAME_PLANE:
      case Page::GAME_BRICK:
      case Page::GAME_SNAKE:
        page_ = Page::GAMES_MENU;
        break;
      default:
        page_ = Page::MAIN_MENU;
        break;
    }
  }

  void ApplyDial(std::int32_t delta)
  {
    if (delta == 0)
    {
      return;
    }

    switch (page_)
    {
      case Page::MAIN_MENU:
        main_selected_ = static_cast<std::uint8_t>(
            Wrap(static_cast<std::int16_t>(main_selected_ + delta),
                 static_cast<std::int16_t>(CountOf(kMainMenu))));
        break;
      case Page::GAMES_MENU:
        game_selected_ = static_cast<std::uint8_t>(
            Wrap(static_cast<std::int16_t>(game_selected_ + delta),
                 static_cast<std::int16_t>(CountOf(kGameMenu))));
        break;
      case Page::SETTINGS:
        setting_selected_ = static_cast<std::uint8_t>(
            Wrap(static_cast<std::int16_t>(setting_selected_ + delta),
                 static_cast<std::int16_t>(CountOf(kSettings))));
        break;
      case Page::RGB:
        rgb_mode_ = static_cast<RgbMode>(
            Wrap(static_cast<std::int16_t>(static_cast<std::uint8_t>(rgb_mode_) + delta),
                 static_cast<std::int16_t>(CountOf(kRgbModes))));
        break;
      case Page::ROBOT:
        robot_mood_ = static_cast<std::uint8_t>(
            Wrap(static_cast<std::int16_t>(robot_mood_ + delta), 3));
        break;
      case Page::GAME_PLANE:
      case Page::GAME_BRICK:
      case Page::GAME_SNAKE:
        game_player_x_ =
            static_cast<std::int16_t>(ClipInt(game_player_x_ + delta * 4, 4, 120));
        break;
      default:
        break;
    }
  }

  void ActivateSetting()
  {
    switch (setting_selected_)
    {
      case 0:
        oled_brightness_ = static_cast<std::uint8_t>((oled_brightness_ + 25U) % 125U);
        if (oled_brightness_ == 0U)
        {
          oled_brightness_ = 25U;
        }
        break;
      case 1:
        sound_enabled_ = !sound_enabled_;
        if (sound_enabled_)
        {
          StartBuzzerPulse();
        }
        else
        {
          StopBuzzer();
        }
        break;
      case 2:
        rgb_brightness_ = static_cast<std::uint8_t>(rgb_brightness_ + 32U);
        if (rgb_brightness_ < 16U)
        {
          rgb_brightness_ = 16U;
        }
        rgb_.SetBrightness(rgb_brightness_);
        break;
      case 3:
        show_fps_ = !show_fps_;
        break;
      default:
        break;
    }
  }

  void UpdateRgb(std::uint32_t now)
  {
    if ((now - last_rgb_ms_) < kRgbFrameMs)
    {
      return;
    }
    last_rgb_ms_ = now;

    switch (rgb_mode_)
    {
      case RgbMode::OFF:
        rgb_.Clear();
        return;
      case RgbMode::STATIC:
        rgb_.Fill(80U, 16U, 160U);
        (void)rgb_.Show();
        return;
      case RgbMode::FLOWING:
        for (std::uint8_t i = 0; i < kRgbLedCount; ++i)
        {
          const std::uint8_t phase = static_cast<std::uint8_t>(rgb_phase_ + i * 32U);
          rgb_.SetPixel(i, phase, static_cast<std::uint8_t>(255U - phase),
                        static_cast<std::uint8_t>(phase >> 1U));
        }
        rgb_phase_ = static_cast<std::uint8_t>(rgb_phase_ + 3U);
        (void)rgb_.Show();
        return;
      case RgbMode::RUNNING:
        rgb_.ClearPixels();
        rgb_.SetPixel(static_cast<std::uint16_t>((rgb_phase_ >> 3U) % kRgbLedCount), 255U,
                      60U, 0U);
        rgb_phase_ = static_cast<std::uint8_t>(rgb_phase_ + 1U);
        (void)rgb_.Show();
        return;
      case RgbMode::BREATHE:
      {
        const std::uint8_t wave =
            (rgb_phase_ & 0x40U) != 0U
                ? static_cast<std::uint8_t>(127U - (rgb_phase_ & 0x3FU) * 2U)
                : static_cast<std::uint8_t>((rgb_phase_ & 0x3FU) * 2U);
        rgb_.Fill(wave, wave, 255U);
        rgb_phase_ = static_cast<std::uint8_t>(rgb_phase_ + 2U);
        (void)rgb_.Show();
        return;
      }
    }
  }

  void StartBuzzerPulse()
  {
    if (buzzer_ == nullptr)
    {
      return;
    }

    const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());
    (void)buzzer_->SetConfig({2400U});
    (void)buzzer_->SetDutyCycle(0.25F);
    (void)buzzer_->Enable();
    buzzer_active_ = true;
    buzzer_off_ms_ = now + kBuzzerPulseMs;
  }

  void UpdateBuzzer(std::uint32_t now)
  {
    if (!buzzer_active_)
    {
      return;
    }

    if (static_cast<std::int32_t>(now - buzzer_off_ms_) >= 0)
    {
      StopBuzzer();
    }
  }

  void StopBuzzer()
  {
    buzzer_active_ = false;
    if (buzzer_ != nullptr)
    {
      (void)buzzer_->SetDutyCycle(0.0F);
      (void)buzzer_->Disable();
    }
  }

  bool IsAnimatedPage() const
  {
    switch (page_)
    {
      case Page::ROBOT:
      case Page::RGB:
      case Page::GAME_DINO:
      case Page::GAME_BIRD:
      case Page::GAME_PLANE:
      case Page::GAME_BRICK:
      case Page::GAME_SNAKE:
        return true;
      default:
        return false;
    }
  }

  void Render()
  {
    ClearCanvas();
    switch (page_)
    {
      case Page::MAIN_MENU:
        DrawMenu("TQX FACTORY", kMainMenu, CountOf(kMainMenu), main_selected_);
        break;
      case Page::SETTINGS:
        DrawSettings();
        break;
      case Page::RGB:
        DrawRgb();
        break;
      case Page::GYRO:
        DrawGyro();
        break;
      case Page::UART:
        DrawUart();
        break;
      case Page::ROBOT:
        DrawRobot();
        break;
      case Page::GAMES_MENU:
        DrawMenu("MINI GAMES", kGameMenu, CountOf(kGameMenu), game_selected_);
        break;
      case Page::THEME:
        DrawTheme();
        break;
      case Page::MORE:
        DrawMore();
        break;
      case Page::GAME_DINO:
        DrawDino();
        break;
      case Page::GAME_BIRD:
        DrawBird();
        break;
      case Page::GAME_PLANE:
        DrawPlane();
        break;
      case Page::GAME_BRICK:
        DrawBrick();
        break;
      case Page::GAME_SNAKE:
        DrawSnake();
        break;
    }

    display_.PublishFullFrame();
  }

  void DrawMenu(const char* title, const MenuItem* items, std::size_t count,
                std::uint8_t selected)
  {
    Header(title);
    const std::uint8_t visible = 4U;
    std::uint8_t first = 0U;
    if (selected >= visible)
    {
      first = static_cast<std::uint8_t>(selected - visible + 1U);
    }
    if (first + visible > count)
    {
      first = static_cast<std::uint8_t>(count > visible ? count - visible : 0U);
    }

    for (std::uint8_t row = 0; row < visible && (first + row) < count; ++row)
    {
      const auto index = static_cast<std::uint8_t>(first + row);
      const auto y = static_cast<std::int16_t>(14 + row * 10);
      if (index == selected)
      {
        TextSelected(y, items[index].label);
      }
      else
      {
        Text(8, y, items[index].label);
      }
    }

    char buf[20]{};
    (void)std::snprintf(buf, sizeof(buf), "%u/%u", static_cast<unsigned>(selected + 1U),
                        static_cast<unsigned>(count));
    Footer("Dial Select", buf);
  }

  void DrawSettings()
  {
    Header("SETTINGS");
    for (std::uint8_t i = 0; i < CountOf(kSettings); ++i)
    {
      char buf[28]{};
      const char* value = "";
      if (i == 0)
      {
        (void)std::snprintf(buf, sizeof(buf), "%s %u%%", kSettings[i],
                            static_cast<unsigned>(oled_brightness_));
        value = buf;
      }
      else if (i == 1)
      {
        (void)std::snprintf(buf, sizeof(buf), "%s %s", kSettings[i],
                            sound_enabled_ ? "on" : "off");
        value = buf;
      }
      else if (i == 2)
      {
        (void)std::snprintf(buf, sizeof(buf), "%s %u", kSettings[i],
                            static_cast<unsigned>(rgb_brightness_));
        value = buf;
      }
      else
      {
        (void)std::snprintf(buf, sizeof(buf), "%s %s", kSettings[i],
                            show_fps_ ? "on" : "off");
        value = buf;
      }

      const auto y = static_cast<std::int16_t>(14 + i * 10);
      if (i == setting_selected_)
      {
        TextSelected(y, value);
      }
      else
      {
        Text(4, y, value);
      }
    }
    Footer("Enter Edit", "Back");
  }

  void DrawRgb()
  {
    Header("RGB LED");
    const auto mode = static_cast<std::uint8_t>(rgb_mode_);
    char buf[32]{};
    (void)std::snprintf(buf, sizeof(buf), "Mode: %s", kRgbModes[mode]);
    Text(4, 14, buf);
    (void)std::snprintf(buf, sizeof(buf), "Brightness: %u",
                        static_cast<unsigned>(rgb_brightness_));
    Text(4, 24, buf);
    Canvas().DrawRect(4, 36, 120, 10, Fg());
    const auto bar = static_cast<std::int16_t>((rgb_brightness_ * 118U) / 255U);
    Canvas().FillRect(5, 37, bar, 8, Fg());
    for (std::uint8_t i = 0; i < kRgbLedCount; ++i)
    {
      Canvas().DrawRect(static_cast<std::int16_t>(18 + i * 24), 48, 12, 5, Fg());
      if (rgb_mode_ != RgbMode::OFF &&
          (rgb_mode_ != RgbMode::RUNNING || ((rgb_phase_ >> 3U) % kRgbLedCount) == i))
      {
        Canvas().FillRect(static_cast<std::int16_t>(20 + i * 24), 49, 8, 3, Fg());
      }
    }
    Footer("Dial Mode", "Enter Bright");
  }

  void DrawGyro()
  {
    Header("GYROSCOPE");
    char buf[32]{};
    if (gyro_valid_)
    {
      (void)std::snprintf(buf, sizeof(buf), "G %4ld %4ld %4ld",
                          static_cast<long>(ScaleToInt(last_gyro_.x(), 1000.0F)),
                          static_cast<long>(ScaleToInt(last_gyro_.y(), 1000.0F)),
                          static_cast<long>(ScaleToInt(last_gyro_.z(), 1000.0F)));
      Text(0, 14, buf);
      Text(0, 23, "mrad/s");
    }
    else
    {
      Text(0, 14, "G waiting...");
    }

    if (accl_valid_)
    {
      (void)std::snprintf(buf, sizeof(buf), "A %4ld %4ld %4ld",
                          static_cast<long>(ScaleToInt(last_accl_.x(), 1000.0F)),
                          static_cast<long>(ScaleToInt(last_accl_.y(), 1000.0F)),
                          static_cast<long>(ScaleToInt(last_accl_.z(), 1000.0F)));
      Text(0, 34, buf);
      Text(0, 43, "mg");
    }
    else
    {
      Text(0, 34, "A waiting...");
    }
    Footer("LSM6DS3TRC", "Back");
  }

  void DrawUart()
  {
    Header("UART MONITOR");
    char buf[32]{};
    const auto dbg_rx = debug_uart_ != nullptr && debug_uart_->read_port_ != nullptr
                            ? static_cast<unsigned>(debug_uart_->read_port_->Size())
                            : 0U;
    const auto wl_rx = wireless_uart_ != nullptr && wireless_uart_->read_port_ != nullptr
                           ? static_cast<unsigned>(wireless_uart_->read_port_->Size())
                           : 0U;
    (void)std::snprintf(buf, sizeof(buf), "Debug RX: %u", dbg_rx);
    Text(4, 14, buf);
    (void)std::snprintf(buf, sizeof(buf), "Wireless RX: %u", wl_rx);
    Text(4, 24, buf);
    (void)std::snprintf(
        buf, sizeof(buf), "Link: %s",
        wireless_link_ != nullptr && wireless_link_->Read() ? "on" : "off");
    Text(4, 34, buf);
    Text(4, 44, last_button_ == nullptr ? "Key: -" : last_button_);
    Footer("115200 8N1", "Back");
  }

  void DrawRobot()
  {
    Header("ROBOT FACE");
    const bool blink = (frame_ % 48U) > 42U;
    const std::int16_t eye_y = blink ? 32 : 26;
    Canvas().DrawRect(14, 16, 100, 34, Fg());
    if (blink)
    {
      Canvas().DrawHorizontalLine(32, eye_y, 18, Fg());
      Canvas().DrawHorizontalLine(78, eye_y, 18, Fg());
    }
    else
    {
      Canvas().FillCircle(40, eye_y, robot_mood_ == 1U ? 7 : 6, Fg());
      Canvas().FillCircle(86, eye_y, robot_mood_ == 2U ? 4 : 6, Fg());
      Canvas().FillCircle(42, static_cast<std::int16_t>(eye_y - 2), 2, Bg());
      Canvas().FillCircle(88, static_cast<std::int16_t>(eye_y - 2), 2, Bg());
    }

    if (robot_mood_ == 0U)
    {
      Canvas().DrawLine(49, 42, 64, 47, Fg());
      Canvas().DrawLine(64, 47, 79, 42, Fg());
    }
    else if (robot_mood_ == 1U)
    {
      Canvas().DrawHorizontalLine(50, 43, 28, Fg());
    }
    else
    {
      Canvas().DrawLine(48, 46, 64, 41, Fg());
      Canvas().DrawLine(64, 41, 80, 46, Fg());
    }
    Footer("Dial Mood", "Back");
  }

  void DrawTheme()
  {
    Header("THEME");
    Text(8, 16, dark_mode_ ? "Dark pixels" : "Light pixels");
    Canvas().DrawRect(8, 30, 112, 16, Fg());
    Canvas().FillRect(dark_mode_ ? 70 : 12, 33, 36, 10, Fg());
    Footer("Enter Toggle", "Back");
  }

  void DrawMore()
  {
    Header("ABOUT");
    Text(4, 14, "MCU: MSPM0G3519");
    Text(4, 24, "OLED: SSD1306/1312");
    Text(4, 34, "Port: LibXR/XRobot");
    Text(4, 44, "Template: verdancy BSP");
    Footer("Factory equiv", "Back");
  }

  void ResetGame()
  {
    game_player_x_ = 20;
    game_player_y_ = 40;
    game_velocity_ = 0;
    game_score_ = 0;
    game_jump_ = false;
  }

  void DrawGameHeader(const char* name)
  {
    Canvas().DrawText(0, 0, name, Fg());
    char buf[16]{};
    (void)std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(game_score_));
    Canvas().DrawText(static_cast<std::int16_t>(128 - MonoCanvas::TextWidth(buf)), 0, buf,
                      Fg());
    Canvas().DrawHorizontalLine(0, 9, 128, Fg());
  }

  void DrawDino()
  {
    DrawGameHeader("DINO");
    if (game_jump_ && game_player_y_ >= 42)
    {
      game_velocity_ = -8;
      game_jump_ = false;
    }
    game_velocity_++;
    game_player_y_ =
        static_cast<std::int16_t>(ClipInt(game_player_y_ + game_velocity_, 20, 44));
    if (game_player_y_ >= 44)
    {
      game_velocity_ = 0;
    }
    const auto obstacle_x = static_cast<std::int16_t>(128 - ((frame_ * 3U) % 140U));
    Canvas().DrawHorizontalLine(0, 52, 128, Fg());
    Canvas().FillRect(20, static_cast<std::int16_t>(game_player_y_ - 10), 10, 10, Fg());
    Canvas().FillRect(obstacle_x, 42, 6, 10, Fg());
    game_score_++;
    Footer("Enter Jump", "Back");
  }

  void DrawBird()
  {
    DrawGameHeader("BIRD");
    if (game_jump_)
    {
      game_velocity_ = -5;
      game_jump_ = false;
    }
    game_velocity_++;
    game_player_y_ =
        static_cast<std::int16_t>(ClipInt(game_player_y_ + game_velocity_, 14, 50));
    const auto pipe_x = static_cast<std::int16_t>(128 - ((frame_ * 2U) % 150U));
    Canvas().FillCircle(30, game_player_y_, 4, Fg());
    Canvas().FillRect(pipe_x, 10, 8, 18, Fg());
    Canvas().FillRect(pipe_x, 44, 8, 12, Fg());
    game_score_++;
    Footer("Enter Flap", "Back");
  }

  void DrawPlane()
  {
    DrawGameHeader("PLANE");
    Canvas().DrawLine(game_player_x_, 49, static_cast<std::int16_t>(game_player_x_ - 6),
                      58, Fg());
    Canvas().DrawLine(game_player_x_, 49, static_cast<std::int16_t>(game_player_x_ + 6),
                      58, Fg());
    Canvas().DrawLine(static_cast<std::int16_t>(game_player_x_ - 6), 58,
                      static_cast<std::int16_t>(game_player_x_ + 6), 58, Fg());
    for (std::uint8_t i = 0; i < 4; ++i)
    {
      const auto x = static_cast<std::int16_t>((i * 31U + frame_ * 3U) % 128U);
      const auto y = static_cast<std::int16_t>(14 + ((i * 11U + frame_) % 26U));
      Canvas().DrawRect(x, y, 7, 5, Fg());
    }
    Canvas().DrawVerticalLine(game_player_x_, 18, 25, Fg());
    game_score_++;
    Footer("Dial Move", "Back");
  }

  void DrawBrick()
  {
    DrawGameHeader("BRICK");
    for (std::uint8_t row = 0; row < 3; ++row)
    {
      for (std::uint8_t col = 0; col < 8; ++col)
      {
        if (((row + col + (frame_ >> 4U)) & 0x01U) == 0U)
        {
          Canvas().DrawRect(static_cast<std::int16_t>(4 + col * 15),
                            static_cast<std::int16_t>(14 + row * 7), 12, 5, Fg());
        }
      }
    }
    const auto ball_x = static_cast<std::int16_t>((frame_ * 2U) % 120U + 4U);
    const auto ball_y = static_cast<std::int16_t>(35 + ((frame_ >> 1U) % 12U));
    Canvas().FillCircle(ball_x, ball_y, 2, Fg());
    Canvas().FillRect(static_cast<std::int16_t>(game_player_x_ - 12), 56, 24, 4, Fg());
    game_score_++;
    Footer("Dial Paddle", "Back");
  }

  void DrawSnake()
  {
    DrawGameHeader("SNAKE");
    const auto base_x = static_cast<std::int16_t>(ClipInt(game_player_x_, 12, 110));
    const auto base_y = static_cast<std::int16_t>(32 + ((frame_ >> 3U) & 0x07U));
    for (std::uint8_t i = 0; i < 8; ++i)
    {
      Canvas().FillRect(static_cast<std::int16_t>(base_x - i * 6),
                        static_cast<std::int16_t>(base_y + ((i & 1U) ? 2 : -2)), 5, 5,
                        Fg());
    }
    Canvas().DrawRect(104, 20, 6, 6, Fg());
    game_score_++;
    Footer("Dial Steer", "Back");
  }

  BitsButtonXR& buttons_;
  Dial& dial_;
  DisplaySurface& display_;
  WS2812PWM<4>& rgb_;

  LibXR::Topic gyro_topic_;
  LibXR::Topic accl_topic_;
  LibXR::Topic::ASyncSubscriber<LSM6DS3TRC::Vector3f> gyro_sub_;
  LibXR::Topic::ASyncSubscriber<LSM6DS3TRC::Vector3f> accl_sub_;

  LibXR::GPIO* wireless_link_ = nullptr;
  LibXR::PWM* buzzer_ = nullptr;
  LibXR::UART* debug_uart_ = nullptr;
  LibXR::UART* wireless_uart_ = nullptr;

  Page page_ = Page::MAIN_MENU;
  std::uint8_t main_selected_ = 0;
  std::uint8_t game_selected_ = 0;
  std::uint8_t setting_selected_ = 0;

  RgbMode rgb_mode_ = RgbMode::FLOWING;
  std::uint8_t rgb_brightness_ = 48;
  std::uint8_t rgb_phase_ = 0;
  std::uint32_t last_rgb_ms_ = 0;

  LSM6DS3TRC::Vector3f last_gyro_{};
  LSM6DS3TRC::Vector3f last_accl_{};
  bool gyro_valid_ = false;
  bool accl_valid_ = false;

  const char* last_button_ = nullptr;
  bool dark_mode_ = false;
  bool sound_enabled_ = false;
  bool buzzer_active_ = false;
  bool show_fps_ = false;
  std::uint8_t oled_brightness_ = 100;
  std::uint8_t robot_mood_ = 0;
  std::uint32_t buzzer_off_ms_ = 0;

  std::int16_t game_player_x_ = 20;
  std::int16_t game_player_y_ = 40;
  std::int16_t game_velocity_ = 0;
  bool game_jump_ = false;
  std::uint32_t game_score_ = 0;

  bool render_started_ = false;
  bool render_requested_ = true;
  std::uint32_t last_render_ms_ = 0;
  std::uint32_t frame_ = 0;
};
