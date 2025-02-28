#include "PIN_CFG.h"

peripheral pin;

void pin_cfg_init(peripheral *pin){
  pin->ZIGBEE_RX = 16; //UART
  pin->ZIGBEE_TX = 17;
  pin->VCC_WTR = 12;
  pin->WTR_PIN = 14;
}