#ifndef MASTER_PIN_CFG_H
#define MASTER_PIN_CFG_H
#include <Arduino.h>

typedef struct lcd_pins{
  uint8_t TFT_CS = 0;
  uint8_t TFT_DC = 2;
  uint8_t TFT_MOSI = 14;
  uint8_t TFT_SCLK = 12;
  uint8_t TFT_RST = 13;
} lcd_pin;
extern lcd_pin lcd;

typedef struct peripheral__pin{
  uint8_t ZIGBEE_RX;
  uint8_t ZIGBEE_TX;
  uint8_t LED_PIN;
  uint8_t BUTTON_PIN;
} peripheral;
extern peripheral pin;

void peripheral_init(peripheral *pin);

#endif 
