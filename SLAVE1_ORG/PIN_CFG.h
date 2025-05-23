#ifndef PIN_CFG_H
#define PIN_CFG_H

#include <Arduino.h>

typedef struct{
  uint8_t ZIGBEE_RX;
  uint8_t ZIGBEE_TX;
  uint8_t VCC_WTR;
  uint8_t WTR_PIN;
  uint8_t P0_PIN;
  uint8_t TDS_PIN;
  uint8_t DS18B20_PIN;
} peripheral;

extern peripheral pin;

void pin_cfg_init(peripheral *pin);

#endif