# tqx-factory-firmware

LibXR/XRobot-style factory firmware for the LCKFB TQX MSPM0G3519 board. This
repo is created from `verdancy-org/bsp-lckfb-tqx-mspm0g3519` and implements an
equivalent of the original `factory_firmware/ccs/oeldui` demo without copying
the original OLED UI/game sources verbatim.

## Layout

- `sysconfig/` contains the MSPM0G3519 LQFP-64(PM) SysConfig source and generated `ti_msp_dl_config.*`.
- `User/app_main.cpp` creates the LibXR hardware container, owns the cooperative main loop, and calls the user scheduler.
- `User/src/scheduler.cpp` instantiates the Factory XRobot modules and runs the cooperative user slots.
- `User/src/ui.cpp`, `light.cpp`, `input.cpp`, `navigation.cpp`, `sensors.cpp`, and `games.cpp` split the Factory logic by concern.
- `User/src/runtime.hpp` is a private source-level header for shared Factory runtime declarations used by the scheduler slots; it is not a public API.
- `Modules/` contains XRobot modules, including external modules plus the board-specific `WS2812PWM`.
- `User/xrobot.yaml` is the XRobot module instance config.
- `User/xrobot_main.hpp` remains ignored; regenerate it only if you want to return to CLI-generated module wiring.

## Dependencies

The project uses Git submodules for LibXR and the TI MSPM0 SDK:

```powershell
git submodule update --init --recursive
```

Override them explicitly when needed:

```powershell
cmake -S . -B build -G Ninja `
  -DLIBXR_DIR=D:/path/to/libxr `
  -DMSPM0_SDK_DIR=D:/path/to/mspm0-sdk
```

## Regenerate SysConfig

```powershell
& 'C:\ti\sysconfig_1.26.2\sysconfig_cli.bat' `
  --product 'D:\Projects\tqx-factory-firmware\mspm0-sdk\.metadata\product.json' `
  --device MSPM0G351X --part Default --package 'LQFP-64(PM)' `
  --script 'D:\Projects\tqx-factory-firmware\sysconfig\untitled.syscfg' `
  --compiler gcc `
  --output 'D:\Projects\tqx-factory-firmware\sysconfig'
```

## XRobot Modules

Default instantiated modules:

- `BitsButtonXR` for PB21/PA18/PA31/PA24/PB24 keys.
- `Dial` on PA30/PA29 GPIO phase inputs for the EC11-style front-panel dial,
  leaving TIMG8 QEI free for external encoders.
- `W25QXX` on SPI1 + PB6 chip select.
- `LSM6DS3TRC` on I2C0 PA0/PA1, matching the factory firmware's 7-bit `0x6A` IMU.
- `WS2812PWM` on TIMA1/PB26 + DMA CH0 via the WS2812-local `TimedWaveform` backend. Its built-in demo is disabled so the Factory user task can own RGB effects.

## Factory Firmware Equivalent

The original CCS project starts UART, wireless UART, keys, buzzer, WS2812,
OLED UI, W25QXX settings storage, LSM6DS3 IMU, robot face, UART monitor, and
five small games. This port keeps the same hardware coverage and user-facing
demo categories through LibXR/XRobot modules:

- Main menu: Settings, RGB LED, Gyro, UART, Robot, Games, Theme, More.
- RGB LED: off/static/flowing/running/breathe effects over the `WS2812PWM` module.
- Gyro: live LSM6DS3TRC accel/gyro topic values.
- UART: debug and wireless UART RX queue status plus wireless link GPIO.
- Robot face: animated OLED face controlled by the dial.
- Games: lightweight Dino, Bird, Plane, Brick, and Snake OLED demos.

Cloned but optional modules:

- `WS2812`: shared WS2812 color/brightness/demo core, `WS2812SPI`, and the WS2812-local `TimedWaveform` interface/backend used by `WS2812PWM`.
- `CaFeZn/WS2812`: the SPI transport remains available as `WS2812SPI`; it is not default because this board uses PWM/DMA.

Regenerate the entry after editing `User/xrobot.yaml`:

```powershell
$env:PYTHONIOENCODING='utf-8'
xrobot_gen_main --output User/xrobot_main.hpp --config User/xrobot.yaml
```
