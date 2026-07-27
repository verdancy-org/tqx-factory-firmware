#include "Scheduler.hpp"

#include "runtime.hpp"

#include "SSD1306.hpp"
#include "W25QXX.hpp"

namespace
{

constexpr std::uint32_t EventId(Scheduler::Event event)
{
  return static_cast<std::uint32_t>(event);
}

void OnSchedulerEvent(bool in_isr, Runtime* rt, std::uint32_t event_id)
{
  UNUSED(in_isr);

  switch (static_cast<Scheduler::Event>(event_id))
  {
    case Scheduler::Event::ENTER:
      rt->ui.last_button = "enter";
      OnEnter(rt->hardware, rt->ui, rt->feedback, rt->game);
      RequestRender(rt->ui);
      break;
    case Scheduler::Event::BACK:
      rt->ui.last_button = "back";
      OnBack(rt->ui);
      RequestRender(rt->ui);
      break;
    case Scheduler::Event::DIAL_CLOCKWISE:
      rt->ui.last_button = "dial+";
      ApplyDial(rt->ui, rt->feedback, rt->game, 1);
      RequestRender(rt->ui);
      break;
    case Scheduler::Event::DIAL_COUNTER_CLOCKWISE:
      rt->ui.last_button = "dial-";
      ApplyDial(rt->ui, rt->feedback, rt->game, -1);
      RequestRender(rt->ui);
      break;
    case Scheduler::Event::DIAL_FAST_CLOCKWISE:
      rt->ui.last_button = "dial++";
      ApplyDial(rt->ui, rt->feedback, rt->game, 3);
      RequestRender(rt->ui);
      break;
    case Scheduler::Event::DIAL_FAST_COUNTER_CLOCKWISE:
      rt->ui.last_button = "dial--";
      ApplyDial(rt->ui, rt->feedback, rt->game, -3);
      RequestRender(rt->ui);
      break;
  }
}

void RegisterSchedulerEvents(Runtime& rt)
{
  const auto cb = LibXR::Event::Callback::Create(OnSchedulerEvent, &rt);
  rt.events.Register(EventId(Scheduler::Event::ENTER), cb);
  rt.events.Register(EventId(Scheduler::Event::BACK), cb);
  rt.events.Register(EventId(Scheduler::Event::DIAL_CLOCKWISE), cb);
  rt.events.Register(EventId(Scheduler::Event::DIAL_COUNTER_CLOCKWISE), cb);
  rt.events.Register(EventId(Scheduler::Event::DIAL_FAST_CLOCKWISE), cb);
  rt.events.Register(EventId(Scheduler::Event::DIAL_FAST_COUNTER_CLOCKWISE), cb);
}

}  // namespace

Hardware::Hardware(LibXR::HardwareContainer& hw, DisplaySurface& display,
                   WS2812PWM<4>& rgb)
    : display(display),
      rgb(rgb),
      wireless_link(hw.Find<LibXR::GPIO>("wireless_link")),
      buzzer(hw.Find<LibXR::PWM>("buzzer_pwm")),
      debug_uart(hw.Find<LibXR::UART>("uart_debug")),
      wireless_uart(hw.Find<LibXR::UART>("uart_wireless"))
{
  if (buzzer != nullptr)
  {
    (void)buzzer->SetConfig({2000U});
    (void)buzzer->SetDutyCycle(0.0F);
    (void)buzzer->Disable();
  }

  rgb.Clear();
}

Sensors::Sensors()
    : gyro_topic(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_gyro")),
      accl_topic(LibXR::Topic::CreateTopic<LSM6DS3TRC::Vector3f>("lsm6ds3trc_accl")),
      gyro_sub(gyro_topic),
      accl_sub(accl_topic)
{
  gyro_sub.StartWaiting();
  accl_sub.StartWaiting();
}

Runtime::Runtime(LibXR::HardwareContainer& hw, DisplaySurface& display, WS2812PWM<4>& rgb)
    : hardware(hw, display, rgb)
{
}

void ApplyInitialFeedbackOutputs(Runtime& rt)
{
  rt.hardware.rgb.SetBrightness(rt.feedback.rgb_brightness);
}

void RunFactorySlot(Runtime& rt)
{
  const auto now = static_cast<std::uint32_t>(LibXR::Timebase::GetMilliseconds());

  DrainImu(rt.sensors, rt.ui);
  UpdateRgb(rt.hardware, rt.feedback, now);
  UpdateBuzzer(rt.hardware, rt.feedback, now);

  const bool animated = IsAnimatedPage(rt.ui);
  const bool render_requested = rt.ui.render_requested.TestAndClear();
  if (!rt.ui.render_started || render_requested || animated ||
      (now - rt.ui.last_render_ms) >= kIdleRefreshMs)
  {
    rt.ui.render_started = true;
    rt.ui.last_render_ms = now;
    rt.ui.frame++;
    Render(rt);
  }
}

Scheduler::Scheduler(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& manager,
                     DisplaySurface& display, WS2812PWM<4>& rgb)
{
  static Runtime factory_runtime(hw, display, rgb);
  static bool initialized = false;

  runtime_ = &factory_runtime;
  if (!initialized)
  {
    RegisterSchedulerEvents(*runtime_);
    ApplyInitialFeedbackOutputs(*runtime_);
    initialized = true;
  }
  manager.Register(*this);
}

LibXR::Event& Scheduler::GetEvent()
{
  return runtime_->events;
}

void Scheduler::Update()
{
  if (runtime_ == nullptr)
  {
    return;
  }

  RunFactorySlot(*runtime_);
}
