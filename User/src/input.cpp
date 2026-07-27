#include "InputEvents.hpp"

#include "runtime.hpp"

namespace
{

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

}  // namespace

InputEvents::InputEvents(LibXR::HardwareContainer& hw,
                         LibXR::ApplicationManager& manager, BitsButtonXR& buttons,
                         Dial& dial)
    : buttons_(buttons), dial_(dial)
{
  UNUSED(hw);
  manager.Register(*this);
}

void InputEvents::Update()
{
  BitsButtonXR::ButtonEventResult btn{};
  while (buttons_.GetEventResult(btn))
  {
    if (btn.event_type != BitsButtonXR::ButtonEvent::PRESSED)
    {
      continue;
    }

    if (IsEnter(btn.key_alias))
    {
      events_.Active(Id(Event::ENTER));
    }
    else if (IsBack(btn.key_alias))
    {
      events_.Active(Id(Event::BACK));
    }
  }

  Dial::EventResult dial{};
  while (dial_.GetEventResult(dial))
  {
    switch (dial.event_type)
    {
      case Dial::Event::CLOCKWISE:
        events_.Active(Id(Event::DIAL_CLOCKWISE));
        break;
      case Dial::Event::COUNTER_CLOCKWISE:
        events_.Active(Id(Event::DIAL_COUNTER_CLOCKWISE));
        break;
      case Dial::Event::FAST_CLOCKWISE:
        events_.Active(Id(Event::DIAL_FAST_CLOCKWISE));
        break;
      case Dial::Event::FAST_COUNTER_CLOCKWISE:
        events_.Active(Id(Event::DIAL_FAST_COUNTER_CLOCKWISE));
        break;
    }
  }
}
