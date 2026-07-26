#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: Framebuffer surface compositor with dirty-frame publishing.
constructor_args:
  - frame_topic_name: "display_frame"
  - refresh_interval_ms: 33
template_args: []
required_hardware: []
depends: []
=== END MANIFEST === */
// clang-format on

#include <array>
#include <cstddef>
#include <cstdint>

#include "MonoCanvas.hpp"
#include "app_framework.hpp"
#include "message.hpp"
#include "timebase.hpp"
#include "timer.hpp"

class DisplaySurface : public LibXR::Application
{
 public:
  static constexpr const char* DEFAULT_FRAME_TOPIC = "display_frame";
  static constexpr std::uint16_t WIDTH = 128;
  static constexpr std::uint16_t HEIGHT = 64;
  static constexpr std::uint16_t PAGES = HEIGHT / 8U;
  static constexpr std::size_t FRAMEBUFFER_SIZE = static_cast<std::size_t>(WIDTH) * PAGES;

  enum class PixelFormat : std::uint8_t
  {
    MONO_VTILED_LSB = 0,
  };

  struct Frame
  {
    const std::uint8_t* data = nullptr;
    std::uint32_t size = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint16_t pitch = 0;
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    std::uint16_t dirty_width = 0;
    std::uint16_t dirty_height = 0;
    std::uint32_t sequence = 0;
    PixelFormat pixel_format = PixelFormat::MONO_VTILED_LSB;
    bool full_update = true;
  };

  DisplaySurface(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& app,
                 const char* frame_topic_name = DEFAULT_FRAME_TOPIC,
                 std::uint32_t refresh_interval_ms = 33)
      : frame_topic_(LibXR::Topic::CreateTopic<Frame>(frame_topic_name)),
        canvas_(framebuffer_.data(), framebuffer_.size(), WIDTH, HEIGHT, WIDTH),
        refresh_interval_ms_(refresh_interval_ms == 0U ? 1U : refresh_interval_ms),
        refresh_timer_(LibXR::Timer::CreateTask<DisplaySurface*>(OnRefreshTimer, this,
                                                                 refresh_interval_ms_))
  {
    UNUSED(hw);
    canvas_.Clear(false);
    DrawBootFrame();
    PublishFullFrame();
    LibXR::Timer::Add(refresh_timer_);
    LibXR::Timer::Start(refresh_timer_);
    app.Register(*this);
  }

  MonoCanvas& GetCanvas() { return canvas_; }
  const MonoCanvas& GetCanvas() const { return canvas_; }

  bool PublishDirtyFrame()
  {
    MonoCanvas::Rect dirty{};
    bool full_update = false;
    if (!canvas_.GetDirtyRect(dirty, full_update))
    {
      return false;
    }

    PublishFrame(dirty, full_update);
    canvas_.ClearDirty();
    return true;
  }

  void PublishFullFrame()
  {
    PublishFrame({0, 0, WIDTH, HEIGHT}, true);
    canvas_.ClearDirty();
  }

  void OnMonitor() override {}

 private:
  static void OnRefreshTimer(DisplaySurface* self)
  {
    if (self == nullptr)
    {
      return;
    }

    (void)self->PublishDirtyFrame();
  }

  void PublishFrame(MonoCanvas::Rect dirty, bool full_update)
  {
    frame_.data = framebuffer_.data();
    frame_.size = static_cast<std::uint32_t>(framebuffer_.size());
    frame_.width = WIDTH;
    frame_.height = HEIGHT;
    frame_.pitch = WIDTH;
    frame_.x = dirty.x;
    frame_.y = dirty.y;
    frame_.dirty_width = dirty.width;
    frame_.dirty_height = dirty.height;
    frame_.sequence = sequence_++;
    frame_.pixel_format = PixelFormat::MONO_VTILED_LSB;
    frame_.full_update = full_update;
    frame_topic_.Publish(frame_);
  }

  void DrawBootFrame()
  {
    canvas_.DrawRect(0, 0, WIDTH, HEIGHT);
    canvas_.DrawText(6, 8, "LCKFB MSPM0G3519");
    canvas_.DrawText(6, 20, "DisplaySurface ready");
    canvas_.DrawHorizontalLine(6, 32, 116);
    canvas_.DrawText(6, 40, "MonoCanvas");
  }

  std::array<std::uint8_t, FRAMEBUFFER_SIZE> framebuffer_{};
  LibXR::Topic frame_topic_;
  MonoCanvas canvas_;
  Frame frame_{};
  std::uint32_t refresh_interval_ms_ = 500;
  LibXR::Timer::TimerHandle refresh_timer_ = nullptr;
  std::uint32_t sequence_ = 0;
};
