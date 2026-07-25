#pragma once

#include <cstddef>
#include <cstdint>

#include "BitsButtonXR.hpp"
#include "Dial.hpp"
#include "DisplaySurface.hpp"
#include "LSM6DS3TRC.hpp"
#include "WS2812PWM.hpp"
#include "flag.hpp"
#include "gpio.hpp"
#include "hardware.hpp"
#include "message.hpp"
#include "pwm.hpp"
#include "timebase.hpp"
#include "uart.hpp"

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

  BitsButtonXR& buttons;
  Dial& dial;
  DisplaySurface& display;
  WS2812PWM<4>& rgb;

  LibXR::GPIO* wireless_link = nullptr;
  LibXR::PWM* buzzer = nullptr;
  LibXR::UART* debug_uart = nullptr;
  LibXR::UART* wireless_uart = nullptr;
};

struct Sensors
{
  Sensors();

  LibXR::Topic gyro_topic;
  LibXR::Topic accl_topic;
  LibXR::Topic::ASyncSubscriber<LSM6DS3TRC::Vector3f> gyro_sub;
  LibXR::Topic::ASyncSubscriber<LSM6DS3TRC::Vector3f> accl_sub;

  LSM6DS3TRC::Vector3f last_gyro{};
  LSM6DS3TRC::Vector3f last_accl{};
  bool gyro_valid = false;
  bool accl_valid = false;
};

struct Feedback
{
  RgbMode rgb_mode = RgbMode::FLOWING;
  std::uint8_t rgb_brightness = 48;
  std::uint8_t rgb_phase = 0;
  std::uint32_t last_rgb_ms = 0;

  bool sound_enabled = false;
  bool buzzer_active = false;
  std::uint32_t buzzer_off_ms = 0;
};

struct Ui
{
  Page page = Page::MAIN_MENU;
  std::uint8_t main_selected = 0;
  std::uint8_t game_selected = 0;
  std::uint8_t setting_selected = 0;

  const char* last_button = nullptr;
  bool dark_mode = false;
  bool show_fps = false;
  std::uint8_t oled_brightness = 100;
  std::uint8_t robot_mood = 0;

  bool render_started = false;
  LibXR::Flag::Plain render_requested;
  std::uint32_t last_render_ms = 0;
  std::uint32_t frame = 0;
};

struct Game
{
  std::int16_t player_x = 20;
  std::int16_t player_y = 40;
  std::int16_t velocity = 0;
  bool jump = false;
  std::uint32_t score = 0;
};

struct Runtime
{
  Runtime(LibXR::HardwareContainer& hw, BitsButtonXR& buttons, Dial& dial,
          DisplaySurface& display, WS2812PWM<4>& rgb);

  Hardware hardware;
  Sensors sensors;
  Feedback feedback;
  Ui ui;
  Game game;
};

std::int16_t Wrap(std::int16_t value, std::int16_t count);
std::int32_t ClipInt(std::int32_t value, std::int32_t lo, std::int32_t hi);
bool IsAlias(const char* got, const char* wanted);
std::int32_t ScaleToInt(float value, float scale);
void RequestRender(Ui& ui);

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
void Render(Runtime& rt);
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

void ApplyInitialFeedbackOutputs(Runtime& rt);
void RunFactorySlot(Runtime& rt);
