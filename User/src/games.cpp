#include "runtime.hpp"

#include <cstdio>

void DrawGameHeader(Hardware& hardware, const Ui& ui, const Game& game, const char* name)
{
  Canvas(hardware).DrawText(0, 0, name, Fg(ui));
  char buf[16]{};
  (void)std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(game.score));
  Canvas(hardware).DrawText(static_cast<std::int16_t>(128 - MonoCanvas::TextWidth(buf)),
                            0, buf, Fg(ui));
  Canvas(hardware).DrawHorizontalLine(0, 9, 128, Fg(ui));
}

void ResetGame(Game& game)
{
  game.player_x = 20;
  game.player_y = 40;
  game.velocity = 0;
  game.score = 0;
  game.jump = false;
}

void DrawDino(Hardware& hardware, const Ui& ui, Game& game)
{
  DrawGameHeader(hardware, ui, game, "DINO");
  if (game.jump && game.player_y >= 42)
  {
    game.velocity = -8;
    game.jump = false;
  }
  game.velocity++;
  game.player_y =
      static_cast<std::int16_t>(ClipInt(game.player_y + game.velocity, 20, 44));
  if (game.player_y >= 44)
  {
    game.velocity = 0;
  }
  const auto obstacle_x = static_cast<std::int16_t>(128 - ((ui.frame * 3U) % 140U));
  Canvas(hardware).DrawHorizontalLine(0, 52, 128, Fg(ui));
  Canvas(hardware).FillRect(20, static_cast<std::int16_t>(game.player_y - 10), 10, 10,
                            Fg(ui));
  Canvas(hardware).FillRect(obstacle_x, 42, 6, 10, Fg(ui));
  game.score++;
  Footer(hardware, ui, "Enter Jump", "Back");
}

void DrawBird(Hardware& hardware, const Ui& ui, Game& game)
{
  DrawGameHeader(hardware, ui, game, "BIRD");
  if (game.jump)
  {
    game.velocity = -5;
    game.jump = false;
  }
  game.velocity++;
  game.player_y =
      static_cast<std::int16_t>(ClipInt(game.player_y + game.velocity, 14, 50));
  const auto pipe_x = static_cast<std::int16_t>(128 - ((ui.frame * 2U) % 150U));
  Canvas(hardware).FillCircle(30, game.player_y, 4, Fg(ui));
  Canvas(hardware).FillRect(pipe_x, 10, 8, 18, Fg(ui));
  Canvas(hardware).FillRect(pipe_x, 44, 8, 12, Fg(ui));
  game.score++;
  Footer(hardware, ui, "Enter Flap", "Back");
}

void DrawPlane(Hardware& hardware, const Ui& ui, Game& game)
{
  DrawGameHeader(hardware, ui, game, "PLANE");
  Canvas(hardware).DrawLine(game.player_x, 49,
                            static_cast<std::int16_t>(game.player_x - 6), 58, Fg(ui));
  Canvas(hardware).DrawLine(game.player_x, 49,
                            static_cast<std::int16_t>(game.player_x + 6), 58, Fg(ui));
  Canvas(hardware).DrawLine(static_cast<std::int16_t>(game.player_x - 6), 58,
                            static_cast<std::int16_t>(game.player_x + 6), 58, Fg(ui));
  for (std::uint8_t i = 0; i < 4; ++i)
  {
    const auto x = static_cast<std::int16_t>((i * 31U + ui.frame * 3U) % 128U);
    const auto y = static_cast<std::int16_t>(14 + ((i * 11U + ui.frame) % 26U));
    Canvas(hardware).DrawRect(x, y, 7, 5, Fg(ui));
  }
  Canvas(hardware).DrawVerticalLine(game.player_x, 18, 25, Fg(ui));
  game.score++;
  Footer(hardware, ui, "Dial Move", "Back");
}

void DrawBrick(Hardware& hardware, const Ui& ui, Game& game)
{
  DrawGameHeader(hardware, ui, game, "BRICK");
  for (std::uint8_t row = 0; row < 3; ++row)
  {
    for (std::uint8_t col = 0; col < 8; ++col)
    {
      if (((row + col + (ui.frame >> 4U)) & 0x01U) == 0U)
      {
        Canvas(hardware).DrawRect(static_cast<std::int16_t>(4 + col * 15),
                                  static_cast<std::int16_t>(14 + row * 7), 12, 5,
                                  Fg(ui));
      }
    }
  }
  const auto ball_x = static_cast<std::int16_t>((ui.frame * 2U) % 120U + 4U);
  const auto ball_y = static_cast<std::int16_t>(35 + ((ui.frame >> 1U) % 12U));
  Canvas(hardware).FillCircle(ball_x, ball_y, 2, Fg(ui));
  Canvas(hardware).FillRect(static_cast<std::int16_t>(game.player_x - 12), 56, 24, 4,
                            Fg(ui));
  game.score++;
  Footer(hardware, ui, "Dial Paddle", "Back");
}

void DrawSnake(Hardware& hardware, const Ui& ui, Game& game)
{
  DrawGameHeader(hardware, ui, game, "SNAKE");
  const auto base_x = static_cast<std::int16_t>(ClipInt(game.player_x, 12, 110));
  const auto base_y = static_cast<std::int16_t>(32 + ((ui.frame >> 3U) & 0x07U));
  for (std::uint8_t i = 0; i < 8; ++i)
  {
    Canvas(hardware).FillRect(static_cast<std::int16_t>(base_x - i * 6),
                              static_cast<std::int16_t>(base_y + ((i & 1U) ? 2 : -2)), 5,
                              5, Fg(ui));
  }
  Canvas(hardware).DrawRect(104, 20, 6, 6, Fg(ui));
  game.score++;
  Footer(hardware, ui, "Dial Steer", "Back");
}
