# DisplaySurface

Framebuffer surface compositor with dirty-frame publishing.

The module owns a `MonoCanvas` and publishes `DisplaySurface::Frame` messages on the
configured frame topic. Backends such as SSD1306 subscribe to that topic and
flush the framebuffer to hardware. The current canvas helper targets 128x64
monochrome vertical-tiled framebuffers, while `DisplaySurface::Frame::PixelFormat` keeps
the backend contract open for later color formats.

`MonoCanvas` is intentionally close to Zephyr CFB's small-surface model:

- `Clear`, `SetPixel`, `GetPixel`
- `DrawLine`, `DrawHorizontalLine`, `DrawVerticalLine`
- `DrawRect`, `FillRect`
- `DrawCircle`, `FillCircle`
- `InvertArea`
- `DrawChar`, `DrawText`

Drawing calls update a dirty rectangle. `DisplaySurface::PublishDirtyFrame()` publishes
only that dirty region; `DisplaySurface::PublishFullFrame()` forces a full refresh.

## Required Hardware

None

## Constructor Arguments

- `frame_topic_name`: topic name used for `DisplaySurface::Frame`
- `refresh_interval_ms`: periodic dirty-frame publish interval

## Template Arguments

None

## Depends

None
