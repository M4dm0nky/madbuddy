#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "../config.h"

#ifndef CYD_PANEL_ST7789
#define CYD_PANEL_ST7789 1
#endif
#ifndef CYD_INVERT
#define CYD_INVERT 0
#endif

class LGFX : public lgfx::LGFX_Device {
#if CYD_PANEL_ST7789
  lgfx::Panel_ST7789 _panel;
#else
  lgfx::Panel_ILI9341 _panel;
#endif
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;
  lgfx::Touch_XPT2046 _touch;

public:
  static constexpr const char *PANEL_NAME =
#if CYD_PANEL_ST7789
      "ST7789";
#else
      "ILI9341";
#endif

  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = (uint32_t)CYD_SPI_WRITE_MHZ * 1000000UL;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc = TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = TFT_CS;
      cfg.pin_rst = TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = true;
      cfg.invert = (CYD_INVERT != 0);
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl = TFT_BL;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      auto cfg = _touch.config();
      cfg.x_min = TOUCH_RAW_X_MIN;
      cfg.x_max = TOUCH_RAW_X_MAX;
      cfg.y_min = TOUCH_RAW_Y_MIN;
      cfg.y_max = TOUCH_RAW_Y_MAX;
      cfg.pin_int = TOUCH_IRQ;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.spi_host = VSPI_HOST;
      cfg.freq = 1000000;
      cfg.pin_sclk = TOUCH_CLK;
      cfg.pin_mosi = TOUCH_MOSI;
      cfg.pin_miso = TOUCH_MISO;
      cfg.pin_cs = TOUCH_CS;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
