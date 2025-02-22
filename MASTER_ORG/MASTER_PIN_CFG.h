#ifndef MASTER_PIN_CFG_H
#define MASTER_PIN_CFG_H
#include <Arduino.h>

typedef struct lcd_pins{
  uint8_t TFT_CS;
  uint8_t TFT_DC;
  uint8_t TFT_MOSI;
  uint8_t TFT_SCLK;
  uint8_t TFT_RST;
} lcd_pin;

extern lcd_pin lcd;

typedef struct peripheral__pin{
  uint8_t ZIGBEE_RX;
  uint8_t ZIGBEE_TX;
} peripheral;

extern peripheral pin;

void lcd_init(lcd_pin *lcd);
void peripheral_init(peripheral *pin);

#endif 
