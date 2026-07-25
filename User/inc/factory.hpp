#pragma once

#include <cstddef>
#include <cstdint>

#include "BitsButtonXR.hpp"
#include "Dial.hpp"
#include "DisplaySurface.hpp"
#include "LSM6DS3TRC.hpp"
#include "WS2812PWM.hpp"
#include "gpio.hpp"
#include "hardware.hpp"
#include "message.hpp"
#include "pwm.hpp"
#include "timebase.hpp"
#include "uart.hpp"

namespace Factory
{

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

inline constexpr std::uint32_t kIdleRefreshMs = 250U;
inline constexpr std::uint32_t kRgbFrameMs = 45U;
inline constexpr std::uint32_t kBuzzerPulseMs = 35U;
inline constexpr std::uint8_t kRgbLedCount = 4U;

inline constexpr MenuItem kMainMenu[] = {
    {"Settings", Page::SETTINGS}, {"RGB LED", Page::RGB}, {"Gyro", Page::GYRO},
    {"UART", Page::UART},         {"Robot", Page::ROBOT}, {"Games", Page::GAMES_MENU},
    {"Theme", Page::THEME},       {"More", Page::MORE},
};

inline constexpr MenuItem kGameMenu[] = {
    {"Dino", Page::GAME_DINO},   {"Bird", Page::GAME_BIRD},   {"Plane", Page::GAME_PLANE},
    {"Brick", Page::GAME_BRICK}, {"Snake", Page::GAME_SNAKE},
};

inline constexpr const char* kRgbModes[] = {
    "Off", "Static", "Flowing", "Running", "Breathe",
};

inline constexpr const char* kSettings[] = {
    "OLED bright",
    "Sound",
    "RGB bright",
    "Frame rate",
};

template <typename T, std::size_t N>
constexpr std::size_t CountOf(const T (&)[N])
{
  return N;
}

struct Hardware
{
  Hardware(LibXR::HardwareContainer& hw, BitsButtonXR& buttons, Dial& dial,
           DisplaySurface& display, WS2812PWM<4>& rgb);

  BitsButtonXR& buttons_;
  Dial& dial_;
  DisplaySurface& display_;
  WS2812PWM<4>& rgb_;

  LibXR::GPIO* wireless_link_ = nullptr;
  LibXR::PWM* buzzer_ = nullptr;
  LibXR::UART* debug_uart_ = nullptr;
  LibXR::UART* wireless_uart_ = nullptr;
};

struct Sensors
{
  Sensors();

  LibXR::Topic gyro_topic_;
  LibXR::Topic accl_topic_;
  LibXR::Topic::ASyncSubscriber<LSM6DS3TRC::Vector3f> gyro_sub_;
  LibXR::Topic::ASyncSubscriber<LSM6DS3TRC::Vector3f> accl_sub_;

  LSM6DS3TRC::Vector3f last_gyro_{};
  LSM6DS3TRC::Vector3f last_accl_{};
  bool gyro_valid_ = false;
  bool accl_valid_ = false;
};

struct Feedback
{
  RgbMode rgb_mode_ = RgbMode::FLOWING;
  std::uint8_t rgb_brightness_ = 48;
  std::uint8_t rgb_phase_ = 0;
  std::uint32_t last_rgb_ms_ = 0;

  bool sound_enabled_ = false;
  bool buzzer_active_ = false;
  std::uint32_t buzzer_off_ms_ = 0;
};

struct Ui
{
  Page page_ = Page::MAIN_MENU;
  std::uint8_t main_selected_ = 0;
  std::uint8_t game_selected_ = 0;
  std::uint8_t setting_selected_ = 0;

  const char* last_button_ = nullptr;
  bool dark_mode_ = false;
  bool show_fps_ = false;
  std::uint8_t oled_brightness_ = 100;
  std::uint8_t robot_mood_ = 0;

  bool render_started_ = false;
  bool render_requested_ = true;
  std::uint32_t last_render_ms_ = 0;
  std::uint32_t frame_ = 0;
};

struct Game
{
  std::int16_t player_x_ = 20;
  std::int16_t player_y_ = 40;
  std::int16_t velocity_ = 0;
  bool jump_ = false;
  std::uint32_t score_ = 0;
};

void InitializeFeedbackOutputs(Hardware& hardware, const Feedback& feedback);
void RunOnce(Hardware& hardware, Sensors& sensors, Feedback& feedback, Ui& ui,
             Game& game);

std::int16_t Wrap(std::int16_t value, std::int16_t count);
std::int32_t ClipInt(std::int32_t value, std::int32_t lo, std::int32_t hi);
bool IsAlias(const char* got, const char* wanted);
std::int32_t ScaleToInt(float value, float scale);

MonoCanvas::Color Fg(const Ui& ui);
MonoCanvas::Color Bg(const Ui& ui);
MonoCanvas& Canvas(Hardware& hardware);
void ClearCanvas(Hardware& hardware, const Ui& ui);
void Text(Hardware& hardware, const Ui& ui, std::int16_t x, std::int16_t y,
          const char* text);
void TextSelected(Hardware& hardware, const Ui& ui, std::int16_t y, const char* text);
void Header(Hardware& hardware, const Ui& ui, const char* title);
void Footer(Hardware& hardware, const Ui& ui, const char* left, const char* right);

void DrainInputs(Hardware& hardware, Ui& ui, Feedback& feedback, Game& game);
void DrainImu(Sensors& sensors, Ui& ui);
bool IsEnter(const char* alias);
bool IsBack(const char* alias);
void OnEnter(Hardware& hardware, Ui& ui, Feedback& feedback, Game& game);
void OnBack(Ui& ui);
void ApplyDial(Ui& ui, Feedback& feedback, Game& game, std::int32_t delta);
void ActivateSetting(Hardware& hardware, Ui& ui, Feedback& feedback);

void UpdateRgb(Hardware& hardware, Feedback& feedback, std::uint32_t now);
void StartBuzzerPulse(Hardware& hardware, Feedback& feedback);
void UpdateBuzzer(Hardware& hardware, Feedback& feedback, std::uint32_t now);
void StopBuzzer(Hardware& hardware, Feedback& feedback);

bool IsAnimatedPage(const Ui& ui);
void Render(Hardware& hardware, const Sensors& sensors, Feedback& feedback, Ui& ui,
            Game& game);
void DrawMenu(Hardware& hardware, const Ui& ui, const char* title, const MenuItem* items,
              std::size_t count, std::uint8_t selected);
void DrawSettings(Hardware& hardware, const Ui& ui, const Feedback& feedback);
void DrawRgb(Hardware& hardware, const Ui& ui, const Feedback& feedback);
void DrawGyro(Hardware& hardware, const Ui& ui, const Sensors& sensors);
void DrawUart(Hardware& hardware, const Ui& ui);
void DrawRobot(Hardware& hardware, const Ui& ui);
void DrawTheme(Hardware& hardware, const Ui& ui);
void DrawMore(Hardware& hardware, const Ui& ui);

void ResetGame(Game& game);
void DrawGameHeader(Hardware& hardware, const Ui& ui, const Game& game, const char* name);
void DrawDino(Hardware& hardware, const Ui& ui, Game& game);
void DrawBird(Hardware& hardware, const Ui& ui, Game& game);
void DrawPlane(Hardware& hardware, const Ui& ui, Game& game);
void DrawBrick(Hardware& hardware, const Ui& ui, Game& game);
void DrawSnake(Hardware& hardware, const Ui& ui, Game& game);

}  // namespace Factory
