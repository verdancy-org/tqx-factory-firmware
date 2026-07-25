#include "runtime.hpp"

void DrainImu(Sensors& sensors, Ui& ui)
{
  if (sensors.gyro_sub.Available())
  {
    sensors.last_gyro = sensors.gyro_sub.GetData();
    sensors.gyro_valid = true;
    sensors.gyro_sub.StartWaiting();
    if (ui.page == Page::GYRO)
    {
      RequestRender(ui);
    }
  }

  if (sensors.accl_sub.Available())
  {
    sensors.last_accl = sensors.accl_sub.GetData();
    sensors.accl_valid = true;
    sensors.accl_sub.StartWaiting();
    if (ui.page == Page::GYRO)
    {
      RequestRender(ui);
    }
  }
}
