#ifndef PIN_CFG_H
#define PIN_CFG_H

#include <Arduino.h>

typedef struct peripheral_pin{
  uint8_t ZIGBEE_RX;
  uint8_t ZIGBEE_TX;
  uint8_t VCC_WTR;
  uint8_t WTR_PIN;
} peripheral;

extern peripheral pin;

void pin_cfg_init(peripheral *pin);

#endif