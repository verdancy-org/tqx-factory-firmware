#include "runtime.hpp"

#include <cstring>

std::int16_t Wrap(std::int16_t value, std::int16_t count)
{
  if (count <= 0)
  {
    return 0;
  }
  while (value < 0)
  {
    value = static_cast<std::int16_t>(value + count);
  }
  while (value >= count)
  {
    value = static_cast<std::int16_t>(value - count);
  }
  return value;
}

std::int32_t ClipInt(std::int32_t value, std::int32_t lo, std::int32_t hi)
{
  if (value < lo)
  {
    return lo;
  }
  if (value > hi)
  {
    return hi;
  }
  return value;
}

bool IsAlias(const char* got, const char* wanted)
{
  return got != nullptr && wanted != nullptr && std::strcmp(got, wanted) == 0;
}

std::int32_t ScaleToInt(float value, float scale)
{
  return static_cast<std::int32_t>(value * scale);
}

void RequestRender(Ui& ui)
{
  ui.render_requested.Set();
}
