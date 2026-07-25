#include <cstdio>

#include "factory.hpp"

namespace Factory
{

MonoCanvas::Color Fg(const Ui& ui)
{
  return ui.dark_mode_ ? MonoCanvas::Color::CLEAR : MonoCanvas::Color::SET;
}

MonoCanvas::Color Bg(const Ui& ui)
{
  return ui.dark_mode_ ? MonoCanvas::Color::SET : MonoCanvas::Color::CLEAR;
}

MonoCanvas& Canvas(Hardware& hardware) { return hardware.display_.GetCanvas(); }

void ClearCanvas(Hardware& hardware, const Ui& ui)
{
  Canvas(hardware).Clear(ui.dark_mode_);
}

void Text(Hardware& hardware, const Ui& ui, std::int16_t x, std::int16_t y,
          const char* text)
{
  Canvas(hardware).DrawText(x, y, text, Fg(ui));
}

void TextSelected(Hardware& hardware, const Ui& ui, std::int16_t y, const char* text)
{
  Canvas(hardware).FillRect(0, static_cast<std::int16_t>(y - 1), 128, 9, Fg(ui));
  Canvas(hardware).DrawText(8, y, text, Bg(ui));
}

void Header(Hardware& hardware, const Ui& ui, const char* title)
{
  Canvas(hardware).DrawText(0, 0, title, Fg(ui));
  Canvas(hardware).DrawHorizontalLine(0, 9, 128, Fg(ui));
}

void Footer(Hardware& hardware, const Ui& ui, const char* left, const char* right)
{
  Canvas(hardware).DrawHorizontalLine(0, 54, 128, Fg(ui));
  Canvas(hardware).DrawText(0, 56, left, Fg(ui));
  if (right != nullptr)
  {
    const auto width = MonoCanvas::TextWidth(right);
    Canvas(hardware).DrawText(static_cast<std::int16_t>(128 - width), 56, right, Fg(ui));
  }
}

bool IsAnimatedPage(const Ui& ui)
{
  switch (ui.page_)
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

void Render(Hardware& hardware, const Sensors& sensors, Feedback& feedback, Ui& ui,
            Game& game)
{
  ClearCanvas(hardware, ui);
  switch (ui.page_)
  {
    case Page::MAIN_MENU:
      DrawMenu(hardware, ui, "TQX FACTORY", kMainMenu, CountOf(kMainMenu),
               ui.main_selected_);
      break;
    case Page::SETTINGS:
      DrawSettings(hardware, ui, feedback);
      break;
    case Page::RGB:
      DrawRgb(hardware, ui, feedback);
      break;
    case Page::GYRO:
      DrawGyro(hardware, ui, sensors);
      break;
    case Page::UART:
      DrawUart(hardware, ui);
      break;
    case Page::ROBOT:
      DrawRobot(hardware, ui);
      break;
    case Page::GAMES_MENU:
      DrawMenu(hardware, ui, "MINI GAMES", kGameMenu, CountOf(kGameMenu),
               ui.game_selected_);
      break;
    case Page::THEME:
      DrawTheme(hardware, ui);
      break;
    case Page::MORE:
      DrawMore(hardware, ui);
      break;
    case Page::GAME_DINO:
      DrawDino(hardware, ui, game);
      break;
    case Page::GAME_BIRD:
      DrawBird(hardware, ui, game);
      break;
    case Page::GAME_PLANE:
      DrawPlane(hardware, ui, game);
      break;
    case Page::GAME_BRICK:
      DrawBrick(hardware, ui, game);
      break;
    case Page::GAME_SNAKE:
      DrawSnake(hardware, ui, game);
      break;
  }

  hardware.display_.PublishFullFrame();
}

void DrawMenu(Hardware& hardware, const Ui& ui, const char* title, const MenuItem* items,
              std::size_t count, std::uint8_t selected)
{
  Header(hardware, ui, title);
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
      TextSelected(hardware, ui, y, items[index].label);
    }
    else
    {
      Text(hardware, ui, 8, y, items[index].label);
    }
  }

  char buf[20]{};
  (void)std::snprintf(buf, sizeof(buf), "%u/%u", static_cast<unsigned>(selected + 1U),
                      static_cast<unsigned>(count));
  Footer(hardware, ui, "Dial Select", buf);
}

void DrawSettings(Hardware& hardware, const Ui& ui, const Feedback& feedback)
{
  Header(hardware, ui, "SETTINGS");
  for (std::uint8_t i = 0; i < CountOf(kSettings); ++i)
  {
    char buf[28]{};
    const char* value = "";
    if (i == 0)
    {
      (void)std::snprintf(buf, sizeof(buf), "%s %u%%", kSettings[i],
                          static_cast<unsigned>(ui.oled_brightness_));
      value = buf;
    }
    else if (i == 1)
    {
      (void)std::snprintf(buf, sizeof(buf), "%s %s", kSettings[i],
                          feedback.sound_enabled_ ? "on" : "off");
      value = buf;
    }
    else if (i == 2)
    {
      (void)std::snprintf(buf, sizeof(buf), "%s %u", kSettings[i],
                          static_cast<unsigned>(feedback.rgb_brightness_));
      value = buf;
    }
    else
    {
      (void)std::snprintf(buf, sizeof(buf), "%s %s", kSettings[i],
                          ui.show_fps_ ? "on" : "off");
      value = buf;
    }

    const auto y = static_cast<std::int16_t>(14 + i * 10);
    if (i == ui.setting_selected_)
    {
      TextSelected(hardware, ui, y, value);
    }
    else
    {
      Text(hardware, ui, 4, y, value);
    }
  }
  Footer(hardware, ui, "Enter Edit", "Back");
}

void DrawRgb(Hardware& hardware, const Ui& ui, const Feedback& feedback)
{
  Header(hardware, ui, "RGB LED");
  const auto mode = static_cast<std::uint8_t>(feedback.rgb_mode_);
  char buf[32]{};
  (void)std::snprintf(buf, sizeof(buf), "Mode: %s", kRgbModes[mode]);
  Text(hardware, ui, 4, 14, buf);
  (void)std::snprintf(buf, sizeof(buf), "Brightness: %u",
                      static_cast<unsigned>(feedback.rgb_brightness_));
  Text(hardware, ui, 4, 24, buf);
  Canvas(hardware).DrawRect(4, 36, 120, 10, Fg(ui));
  const auto bar = static_cast<std::int16_t>((feedback.rgb_brightness_ * 118U) / 255U);
  Canvas(hardware).FillRect(5, 37, bar, 8, Fg(ui));
  for (std::uint8_t i = 0; i < kRgbLedCount; ++i)
  {
    Canvas(hardware).DrawRect(static_cast<std::int16_t>(18 + i * 24), 48, 12, 5, Fg(ui));
    if (feedback.rgb_mode_ != RgbMode::OFF &&
        (feedback.rgb_mode_ != RgbMode::RUNNING ||
         ((feedback.rgb_phase_ >> 3U) % kRgbLedCount) == i))
    {
      Canvas(hardware).FillRect(static_cast<std::int16_t>(20 + i * 24), 49, 8, 3, Fg(ui));
    }
  }
  Footer(hardware, ui, "Dial Mode", "Enter Bright");
}

void DrawGyro(Hardware& hardware, const Ui& ui, const Sensors& sensors)
{
  Header(hardware, ui, "GYROSCOPE");
  char buf[32]{};
  if (sensors.gyro_valid_)
  {
    (void)std::snprintf(buf, sizeof(buf), "G %4ld %4ld %4ld",
                        static_cast<long>(ScaleToInt(sensors.last_gyro_.x(), 1000.0F)),
                        static_cast<long>(ScaleToInt(sensors.last_gyro_.y(), 1000.0F)),
                        static_cast<long>(ScaleToInt(sensors.last_gyro_.z(), 1000.0F)));
    Text(hardware, ui, 0, 14, buf);
    Text(hardware, ui, 0, 23, "mrad/s");
  }
  else
  {
    Text(hardware, ui, 0, 14, "G waiting...");
  }

  if (sensors.accl_valid_)
  {
    (void)std::snprintf(buf, sizeof(buf), "A %4ld %4ld %4ld",
                        static_cast<long>(ScaleToInt(sensors.last_accl_.x(), 1000.0F)),
                        static_cast<long>(ScaleToInt(sensors.last_accl_.y(), 1000.0F)),
                        static_cast<long>(ScaleToInt(sensors.last_accl_.z(), 1000.0F)));
    Text(hardware, ui, 0, 34, buf);
    Text(hardware, ui, 0, 43, "mg");
  }
  else
  {
    Text(hardware, ui, 0, 34, "A waiting...");
  }
  Footer(hardware, ui, "LSM6DS3TRC", "Back");
}

void DrawUart(Hardware& hardware, const Ui& ui)
{
  Header(hardware, ui, "UART MONITOR");
  char buf[32]{};
  const auto dbg_rx =
      hardware.debug_uart_ != nullptr && hardware.debug_uart_->read_port_ != nullptr
          ? static_cast<unsigned>(hardware.debug_uart_->read_port_->Size())
          : 0U;
  const auto wl_rx =
      hardware.wireless_uart_ != nullptr && hardware.wireless_uart_->read_port_ != nullptr
          ? static_cast<unsigned>(hardware.wireless_uart_->read_port_->Size())
          : 0U;
  (void)std::snprintf(buf, sizeof(buf), "Debug RX: %u", dbg_rx);
  Text(hardware, ui, 4, 14, buf);
  (void)std::snprintf(buf, sizeof(buf), "Wireless RX: %u", wl_rx);
  Text(hardware, ui, 4, 24, buf);
  (void)std::snprintf(
      buf, sizeof(buf), "Link: %s",
      hardware.wireless_link_ != nullptr && hardware.wireless_link_->Read() ? "on"
                                                                            : "off");
  Text(hardware, ui, 4, 34, buf);
  Text(hardware, ui, 4, 44, ui.last_button_ == nullptr ? "Key: -" : ui.last_button_);
  Footer(hardware, ui, "115200 8N1", "Back");
}

void DrawRobot(Hardware& hardware, const Ui& ui)
{
  Header(hardware, ui, "ROBOT FACE");
  const bool blink = (ui.frame_ % 48U) > 42U;
  const std::int16_t eye_y = blink ? 32 : 26;
  Canvas(hardware).DrawRect(14, 16, 100, 34, Fg(ui));
  if (blink)
  {
    Canvas(hardware).DrawHorizontalLine(32, eye_y, 18, Fg(ui));
    Canvas(hardware).DrawHorizontalLine(78, eye_y, 18, Fg(ui));
  }
  else
  {
    Canvas(hardware).FillCircle(40, eye_y, ui.robot_mood_ == 1U ? 7 : 6, Fg(ui));
    Canvas(hardware).FillCircle(86, eye_y, ui.robot_mood_ == 2U ? 4 : 6, Fg(ui));
    Canvas(hardware).FillCircle(42, static_cast<std::int16_t>(eye_y - 2), 2, Bg(ui));
    Canvas(hardware).FillCircle(88, static_cast<std::int16_t>(eye_y - 2), 2, Bg(ui));
  }

  if (ui.robot_mood_ == 0U)
  {
    Canvas(hardware).DrawLine(49, 42, 64, 47, Fg(ui));
    Canvas(hardware).DrawLine(64, 47, 79, 42, Fg(ui));
  }
  else if (ui.robot_mood_ == 1U)
  {
    Canvas(hardware).DrawHorizontalLine(50, 43, 28, Fg(ui));
  }
  else
  {
    Canvas(hardware).DrawLine(48, 46, 64, 41, Fg(ui));
    Canvas(hardware).DrawLine(64, 41, 80, 46, Fg(ui));
  }
  Footer(hardware, ui, "Dial Mood", "Back");
}

void DrawTheme(Hardware& hardware, const Ui& ui)
{
  Header(hardware, ui, "THEME");
  Text(hardware, ui, 8, 16, ui.dark_mode_ ? "Dark pixels" : "Light pixels");
  Canvas(hardware).DrawRect(8, 30, 112, 16, Fg(ui));
  Canvas(hardware).FillRect(ui.dark_mode_ ? 70 : 12, 33, 36, 10, Fg(ui));
  Footer(hardware, ui, "Enter Toggle", "Back");
}

void DrawMore(Hardware& hardware, const Ui& ui)
{
  Header(hardware, ui, "ABOUT");
  Text(hardware, ui, 4, 14, "MCU: MSPM0G3519");
  Text(hardware, ui, 4, 24, "OLED: SSD1306/1312");
  Text(hardware, ui, 4, 34, "Port: LibXR/XRobot");
  Text(hardware, ui, 4, 44, "Template: verdancy BSP");
  Footer(hardware, ui, "Factory equiv", "Back");
}

}  // namespace Factory
