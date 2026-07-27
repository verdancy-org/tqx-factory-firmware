# LCKFB TQX MSPM0G3519 BSP pin map

This pin map is generated from the factory `empty.syscfg` and targets
MSPM0G3519 LQFP-64(PM).

| Function | Peripheral | Pins | LibXR/XRobot aliases |
| --- | --- | --- | --- |
| Debug UART | UART0 | TX PA10, RX PA11 | `uart_debug`, `debug_uart` |
| Wireless UART / STDIO | UART7 @ 9600 baud | TX PB17, RX PB18 | `uart_wireless`, `wireless_uart`, `console`, `stdio_uart` |
| W25QXX Flash | SPI1 + GPIO CS | SCLK PB9, MOSI PB8, MISO PB7, CS PB6 | `spi_w25qxx`, `spi_flash`, `spi_w25qxx_cs` |
| Debug LED | GPIO | PB22 | `debug_led`, `led`, `status_led` |
| OLED / I2C shared bus | I2C0 | SDA PA0, SCL PA1 | `i2c_imu`, `i2c0`, `i2c_oled` |
| Keys | GPIO | PB21, PA18, PA31, PA24, PB24 | `btn_enter`, `btn_back`, `btn_encoder`, `btn_a24`, `btn_b24` |
| WS2812 | TIMA1 PWM + DMA CH0 | PB26 | `ws2812_waveform` hardware alias, consumed by `WS2812PWM` |
| Buzzer | TIMG6 PWM | PB27 | `buzzer_pwm`, `buzzer` |
| Dial | GPIO phase inputs | A PA30, B PA29 | `dial_a`, `dial_b`, logical module alias `dial` |
| Wireless link | GPIO | PB23 | `wireless_link` |

Notes:

- `WS2812.hpp` holds the shared color/brightness/demo logic; `WS2812SPI.hpp` is the optional SPI backend, while the factory board routes WS2812 to TIMA1/PB26 and instantiates `WS2812PWM` over the WS2812-local `TimedWaveform` backend.
- The factory IMU code targets LSM6DS3TRC over I2C0 and reports `WHO_AM_I = 0x6A`; this BSP instantiates the local `LSM6DS3TRC` module by default.
- The provided SysConfig source uses LQFP-64(PM). The previous placeholder `untitled.syscfg` used LQFP-48(PT), which does not match the factory pinout.
- The front-panel dial is decoded through GPIO interrupts and no longer consumes `TIMG8`; `TIMG8` and `TIMG9` remain available as hardware QEI candidates subject to pinmux conflicts.
- STDIO is bound to UART2/UART7 (`console`) at 9600 baud with a 1 KiB TX software buffer. LibXR STDIO still preserves only the prefix that fits in the remaining write queue, so very large or bursty writes should be chunked or made blocking.
