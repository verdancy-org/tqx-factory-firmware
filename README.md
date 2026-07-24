# bsp-lckfb-tqx-mspm0g3519

LibXR/XRobot-style BSP for the LCKFB TQX MSPM0G3519 board, based on the
MSPM0G3507 LibXR template and the factory firmware SysConfig pinout.

## Layout

- `sysconfig/` contains the MSPM0G3519 LQFP-64(PM) SysConfig source and generated `ti_msp_dl_config.*`.
- `src/app_main.cpp` creates the LibXR hardware container and runs the generated-style XRobot entry, matching the no-board-layer BSP style.
- `Modules/` contains XRobot modules, including external modules plus the board-specific `WS2812PWM`.
- `User/xrobot.yaml` is the XRobot module instance config.
- `User/xrobot_main.hpp` is the generated-style XRobot entry used by `src/app_main.cpp`.

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
  --product 'D:\Projects\bsp-lckfb-tqx-mspm0g3519\mspm0-sdk\.metadata\product.json' `
  --device MSPM0G351X --part Default --package 'LQFP-64(PM)' `
  --script 'D:\Projects\bsp-lckfb-tqx-mspm0g3519\sysconfig\untitled.syscfg' `
  --compiler gcc `
  --output 'D:\Projects\bsp-lckfb-tqx-mspm0g3519\sysconfig'
```

## XRobot Modules

Default instantiated modules:

- `BitsButtonXR` for PB21/PA18/PA31/PA24/PB24 keys.
- `Dial` on PA30/PA29 GPIO phase inputs for the EC11-style front-panel dial,
  leaving TIMG8 QEI free for external encoders.
- `W25QXX` on SPI1 + PB6 chip select.
- `LSM6DS3TRC` on I2C0 PA0/PA1, matching the factory firmware's 7-bit `0x6A` IMU.
- `WS2812PWM` on TIMA1/PB26 + DMA CH0 via the WS2812-local `TimedWaveform` backend.

Cloned but optional modules:

- `WS2812`: shared WS2812 color/brightness/demo core, `WS2812SPI`, and the WS2812-local `TimedWaveform` interface/backend used by `WS2812PWM`.
- `CaFeZn/WS2812`: the SPI transport remains available as `WS2812SPI`; it is not default because this board uses PWM/DMA.

Regenerate the entry after editing `User/xrobot.yaml`:

```powershell
$env:PYTHONIOENCODING='utf-8'
xrobot_gen_main --output User/xrobot_main.hpp --config User/xrobot.yaml
```
