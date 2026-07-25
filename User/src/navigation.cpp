#include "runtime.hpp"

void ActivateSetting(Hardware& hardware, Ui& ui, Feedback& feedback)
{
  switch (ui.setting_selected)
  {
    case 0:
      ui.oled_brightness = static_cast<std::uint8_t>((ui.oled_brightness + 25U) % 125U);
      if (ui.oled_brightness == 0U)
      {
        ui.oled_brightness = 25U;
      }
      break;
    case 1:
      feedback.sound_enabled = !feedback.sound_enabled;
      if (feedback.sound_enabled)
      {
        StartBuzzerPulse(hardware, feedback);
      }
      else
      {
        StopBuzzer(hardware, feedback);
      }
      break;
    case 2:
      feedback.rgb_brightness =
          static_cast<std::uint8_t>(feedback.rgb_brightness + 32U);
      if (feedback.rgb_brightness < 16U)
      {
        feedback.rgb_brightness = 16U;
      }
      hardware.rgb.SetBrightness(feedback.rgb_brightness);
      break;
    case 3:
      ui.show_fps = !ui.show_fps;
      break;
    default:
      break;
  }
}

void OnEnter(Hardware& hardware, Ui& ui, Feedback& feedback, Game& game)
{
  switch (ui.page)
  {
    case Page::MAIN_MENU:
      ui.page = kMainMenu[ui.main_selected].page;
      break;
    case Page::GAMES_MENU:
      ui.page = kGameMenu[ui.game_selected].page;
      ResetGame(game);
      break;
    case Page::SETTINGS:
      ActivateSetting(hardware, ui, feedback);
      break;
    case Page::RGB:
      feedback.rgb_brightness =
          static_cast<std::uint8_t>(feedback.rgb_brightness + 32U);
      if (feedback.rgb_brightness < 16U)
      {
        feedback.rgb_brightness = 16U;
      }
      hardware.rgb.SetBrightness(feedback.rgb_brightness);
      break;
    case Page::THEME:
      ui.dark_mode = !ui.dark_mode;
      break;
    case Page::ROBOT:
      ui.robot_mood =
          static_cast<std::uint8_t>(Wrap(static_cast<std::int16_t>(ui.robot_mood + 1), 3));
      break;
    case Page::GAME_DINO:
    case Page::GAME_BIRD:
      game.jump = true;
      break;
    default:
      break;
  }
}

void OnBack(Ui& ui)
{
  switch (ui.page)
  {
    case Page::MAIN_MENU:
      break;
    case Page::GAME_DINO:
    case Page::GAME_BIRD:
    case Page::GAME_PLANE:
    case Page::GAME_BRICK:
    case Page::GAME_SNAKE:
      ui.page = Page::GAMES_MENU;
      break;
    default:
      ui.page = Page::MAIN_MENU;
      break;
  }
}

void ApplyDial(Ui& ui, Feedback& feedback, Game& game, std::int32_t delta)
{
  if (delta == 0)
  {
    return;
  }

  switch (ui.page)
  {
    case Page::MAIN_MENU:
      ui.main_selected = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.main_selected + delta),
               static_cast<std::int16_t>(CountOf(kMainMenu))));
      break;
    case Page::GAMES_MENU:
      ui.game_selected = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.game_selected + delta),
               static_cast<std::int16_t>(CountOf(kGameMenu))));
      break;
    case Page::SETTINGS:
      ui.setting_selected = static_cast<std::uint8_t>(
          Wrap(static_cast<std::int16_t>(ui.setting_selected + delta),
               static_cast<std::int16_t>(CountOf(kSettings))));
      break;
    case Page::RGB:
      feedback.rgb_mode = static_cast<RgbMode>(
          Wrap(static_cast<std::int16_t>(static_cast<std::uint8_t>(feedback.rgb_mode) +
                                         delta),
               static_cast<std::int16_t>(CountOf(kRgbModes))));
      break;
    case Page::ROBOT:
      ui.robot_mood =
          static_cast<std::uint8_t>(Wrap(static_cast<std::int16_t>(ui.robot_mood + delta),
                                         3));
      break;
    case Page::GAME_PLANE:
    case Page::GAME_BRICK:
    case Page::GAME_SNAKE:
      game.player_x =
          static_cast<std::int16_t>(ClipInt(game.player_x + delta * 4, 4, 120));
      break;
    default:
      break;
  }
}
