#include "app_main.h"

#include <array>
#include <cstdint>

#include "MSPM0TimedWaveform.hpp"
#include "app_framework.hpp"
#include "factory_xrobot_main.hpp"
#include "gpio.hpp"
#include "libxr.hpp"
#include "mspm0_gpio.hpp"
#include "mspm0_i2c.hpp"
#include "mspm0_pwm.hpp"
#include "mspm0_spi.hpp"
#include "mspm0_timebase.hpp"
#include "mspm0_uart.hpp"
#include "ramfs.hpp"
#include "thread.hpp"
#include "ti_msp_dl_config.h"

extern "C" void app_main(void)
{
  using namespace LibXR;

  alignas(CACHE_LINE_SIZE) static std::array<std::uint8_t, 128> uart_debug_rx{};
  alignas(CACHE_LINE_SIZE) static std::array<std::uint8_t, 64> uart_wireless_rx{};
  alignas(CACHE_LINE_SIZE) static std::array<std::uint8_t, 512> spi_rx{};
  alignas(CACHE_LINE_SIZE) static std::array<std::uint8_t, 512> spi_tx{};
  alignas(CACHE_LINE_SIZE) static std::array<std::uint8_t, 64> i2c_stage{};

  static MSPM0Timebase timebase;
  static RamFS ramfs("ramfs");

  static MSPM0UART uart_debug(
      MSPM0_UART_INIT(UART_DEBUG, uart_debug_rx.data(), uart_debug_rx.size(), 8, 256));
  static MSPM0UART uart_wireless(MSPM0_UART_INIT(UART_WIRELESS, uart_wireless_rx.data(),
                                                 uart_wireless_rx.size(), 8, 128));

  static MSPM0GPIO debug_led(DEBUG_LED_PORT, DEBUG_LED_PIN_22_PIN,
                             DEBUG_LED_PIN_22_IOMUX);
  static MSPM0GPIO flash_cs(FLASH_PORT, FLASH_CS_PIN, FLASH_CS_IOMUX);
  static MSPM0GPIO wireless_link(WIRELESS_PORT, WIRELESS_LINK_PIN, WIRELESS_LINK_IOMUX);

  static MSPM0GPIO key_enter(KEY_ENTER_PORT, KEY_ENTER_PIN, KEY_ENTER_IOMUX);
  static MSPM0GPIO key_back(KEY_BACK_PORT, KEY_BACK_PIN, KEY_BACK_IOMUX);
  static MSPM0GPIO key_encoder_sw(KEY_ENCODER_SW_PORT, KEY_ENCODER_SW_PIN,
                                  KEY_ENCODER_SW_IOMUX);
  static MSPM0GPIO key_a24(KEY_A24_PORT, KEY_A24_PIN, KEY_A24_IOMUX);
  static MSPM0GPIO key_b24(KEY_B24_PORT, KEY_B24_PIN, KEY_B24_IOMUX);

  static MSPM0PWM pwm_buzzer({BUZZER_INST, GPIO_BUZZER_C1_IDX,
                              static_cast<std::uint32_t>(BUZZER_INST_CLK_FREQ)});
  static MSPM0GPIO dial_a(DIAL_PORT, DIAL_A_PIN, DIAL_A_IOMUX);
  static MSPM0GPIO dial_b(DIAL_PORT, DIAL_B_PIN, DIAL_B_IOMUX);
  static MSPM0TimedWaveform ws2812_waveform(
      MSPM0_TIMED_WAVEFORM_INIT(WS2812, GPIO_WS2812_C0, DMA_CH0));
  static MSPM0I2C i2c_imu(
      {I2C_OLED_IMU_INST, I2C_OLED_IMU_INST_INT_IRQN, 40000000U,
       I2C_OLED_IMU_BUS_SPEED_HZ, MSPM0I2C::ResolveIndex(I2C_OLED_IMU_INST_INT_IRQN)},
      RawData(i2c_stage.data(), i2c_stage.size()), 8U, {400000U});

  static MSPM0SPI spi_flash(
      {SPI_FLASH_INST, SPI_FLASH_INST_INT_IRQN, CPUCLK_FREQ,
       MSPM0SPI::ResolveIndex(SPI_FLASH_INST_INT_IRQN), 0xFFU, 0xFFU},
      RawData(spi_rx.data(), spi_rx.size()), RawData(spi_tx.data(), spi_tx.size()),
      1024U);

  static HardwareContainer hw(
      Entry<RamFS>{ramfs, {"ramfs", "fs"}},
      Entry<UART>{uart_debug, {"uart_debug", "debug_uart", "console"}},
      Entry<UART>{uart_wireless, {"uart_wireless", "wireless_uart"}},
      Entry<GPIO>{debug_led, {"debug_led", "led", "status_led"}},
      Entry<GPIO>{flash_cs, {"spi_w25qxx_cs", "flash_cs", "w25qxx_cs"}},
      Entry<GPIO>{wireless_link, {"wireless_link"}},
      Entry<GPIO>{key_enter, {"btn_enter", "enter", "btn1"}},
      Entry<GPIO>{key_back, {"btn_back", "back", "btn2"}},
      Entry<GPIO>{key_encoder_sw, {"btn_encoder", "encoder_sw", "btn3"}},
      Entry<GPIO>{key_a24, {"btn_a24", "a24", "btn4"}},
      Entry<GPIO>{key_b24, {"btn_b24", "b24", "btn5"}},
      Entry<PWM>{pwm_buzzer, {"buzzer_pwm", "buzzer"}},
      Entry<GPIO>{dial_a, {"dial_a", "encoder_a"}},
      Entry<GPIO>{dial_b, {"dial_b", "encoder_b"}},
      Entry<TimedWaveform>{ws2812_waveform, {"ws2812_waveform", "ws2812_timed_waveform"}},
      Entry<I2C>{i2c_imu, {"i2c_imu", "i2c0", "i2c_oled"}},
      Entry<SPI>{spi_flash, {"spi_w25qxx", "spi_flash"}});

  STDIO::read_ = &uart_debug._read_port;
  STDIO::write_ = &uart_debug._write_port;

  debug_led.SetConfig(
      {.direction = GPIO::Direction::OUTPUT_PUSH_PULL, .pull = GPIO::Pull::DOWN});
  debug_led.Write(false);

  (void)timebase;
  XRobotFactoryMain(hw);
}
