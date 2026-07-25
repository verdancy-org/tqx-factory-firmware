#include "factory.hpp"

namespace Factory
{

void OnEnter(Hardware& hardware, Ui& ui, Feedback& feedback, Game& game)
{
  switch (ui.page_)
  {
    case Page::MAIN_MENU:
      ui.page_ = kMainMenu[ui.main_selected_].page;
      break;
    case Page::GAMES_MENU:
      ui.page_ = kGameMenu[ui.game_selected_].page;
      ResetGame(game);
      break;
    case Page::SETTINGS:
      ActivateSetting(hardware, ui, feedback);
      break;
    case Page::RGB:
      feedback.rgb_brightness_ =
          static_cast<std::uint8_t>(feedback.rgb_brightness_ + 32U);
      if (feedback.rgb_brightness_ < 16U)
      {
        feedback.rgb_brightness_ = 16U;
      }
      hardware.rgb_.SetBrightness(feedback.rgb_brightness_);
      break;
    case Page::THEME:
      ui.dark_mode_ = !ui.dark_mode_;
      break;
    case Page::ROBOT:
      ui.robot_mood_ = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.robot_mood_ + 1), 3));
      break;
    case Page::GAME_DINO:
    case Page::GAME_BIRD:
      game.jump_ = true;
      break;
    default:
      break;
  }
}

void OnBack(Ui& ui)
{
  switch (ui.page_)
  {
    case Page::MAIN_MENU:
      break;
    case Page::GAME_DINO:
    case Page::GAME_BIRD:
    case Page::GAME_PLANE:
    case Page::GAME_BRICK:
    case Page::GAME_SNAKE:
      ui.page_ = Page::GAMES_MENU;
      break;
    default:
      ui.page_ = Page::MAIN_MENU;
      break;
  }
}

void ApplyDial(Ui& ui, Feedback& feedback, Game& game, std::int32_t delta)
{
  if (delta == 0)
  {
    return;
  }

  switch (ui.page_)
  {
    case Page::MAIN_MENU:
      ui.main_selected_ = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.main_selected_ + delta),
               static_cast<std::int16_t>(CountOf(kMainMenu))));
      break;
    case Page::GAMES_MENU:
      ui.game_selected_ = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.game_selected_ + delta),
               static_cast<std::int16_t>(CountOf(kGameMenu))));
      break;
    case Page::SETTINGS:
      ui.setting_selected_ = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.setting_selected_ + delta),
               static_cast<std::int16_t>(CountOf(kSettings))));
      break;
    case Page::RGB:
      feedback.rgb_mode_ = static_cast<RgbMode>(
          Wrap(static_cast<std::int16_t>(static_cast<std::uint8_t>(feedback.rgb_mode_) +
                                         delta),
               static_cast<std::int16_t>(CountOf(kRgbModes))));
      break;
    case Page::ROBOT:
      ui.robot_mood_ = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.robot_mood_ + delta), 3));
      break;
    case Page::GAME_PLANE:
    case Page::GAME_BRICK:
    case Page::GAME_SNAKE:
      game.player_x_ =
          static_cast<std::int16_t>(ClipInt(game.player_x_ + delta * 4, 4, 120));
      break;
    default:
      break;
  }
}

void ActivateSetting(Hardware& hardware, Ui& ui, Feedback& feedback)
{
  switch (ui.setting_selected_)
  {
    case 0:
      ui.oled_brightness_ = static_cast<std::uint8_t>((ui.oled_brightness_ + 25U) % 125U);
      if (ui.oled_brightness_ == 0U)
      {
        ui.oled_brightness_ = 25U;
      }
      break;
    case 1:
      feedback.sound_enabled_ = !feedback.sound_enabled_;
      if (feedback.sound_enabled_)
      {
        StartBuzzerPulse(hardware, feedback);
      }
      else
      {
        StopBuzzer(hardware, feedback);
      }
      break;
    case 2:
      feedback.rgb_brightness_ =
          static_cast<std::uint8_t>(feedback.rgb_brightness_ + 32U);
      if (feedback.rgb_brightness_ < 16U)
      {
        feedback.rgb_brightness_ = 16U;
      }
      hardware.rgb_.SetBrightness(feedback.rgb_brightness_);
      break;
    case 3:
      ui.show_fps_ = !ui.show_fps_;
      break;
    default:
      break;
  }
}

}  // namespace Factory
