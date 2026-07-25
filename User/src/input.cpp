#include "factory.hpp"

namespace Factory
{

void DrainInputs(Hardware& hardware, Ui& ui, Feedback& feedback, Game& game)
{
  BitsButtonXR::ButtonEventResult btn{};
  while (hardware.buttons_.GetEventResult(btn))
  {
    ui.last_button_ = btn.key_alias;
    if (btn.event_type != BitsButtonXR::ButtonEvent::PRESSED)
    {
      continue;
    }

    if (IsEnter(btn.key_alias))
    {
      OnEnter(hardware, ui, feedback, game);
    }
    else if (IsBack(btn.key_alias))
    {
      OnBack(ui);
    }
    ui.render_requested_ = true;
  }

  Dial::EventResult dial{};
  while (hardware.dial_.GetEventResult(dial))
  {
    ApplyDial(ui, feedback, game, dial.delta);
    ui.render_requested_ = true;
  }
}

bool IsEnter(const char* alias)
{
  return IsAlias(alias, "btn_enter") || IsAlias(alias, "btn_encoder") ||
         IsAlias(alias, "btn_b24") || IsAlias(alias, "nav_enter_alt");
}

bool IsBack(const char* alias)
{
  return IsAlias(alias, "btn_back") || IsAlias(alias, "btn_a24") ||
         IsAlias(alias, "nav_back_alt");
}

}  // namespace Factory
