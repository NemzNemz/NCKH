#include "MASTER_PIN_CFG.h"

peripheral pin;

void peripheral_init(peripheral *pin){
  pin ->ZIGBEE_RX = 16;
  pin ->ZIGBEE_TX = 17;
}
