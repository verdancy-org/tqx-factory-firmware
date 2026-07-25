#include "factory.hpp"

namespace Factory
{

void DrainImu(Sensors& sensors, Ui& ui)
{
  if (sensors.gyro_sub_.Available())
  {
    sensors.last_gyro_ = sensors.gyro_sub_.GetData();
    sensors.gyro_valid_ = true;
    sensors.gyro_sub_.StartWaiting();
    if (ui.page_ == Page::GYRO)
    {
      ui.render_requested_ = true;
    }
  }

  if (sensors.accl_sub_.Available())
  {
    sensors.last_accl_ = sensors.accl_sub_.GetData();
    sensors.accl_valid_ = true;
    sensors.accl_sub_.StartWaiting();
    if (ui.page_ == Page::GYRO)
    {
      ui.render_requested_ = true;
    }
  }
}

}  // namespace Factory
