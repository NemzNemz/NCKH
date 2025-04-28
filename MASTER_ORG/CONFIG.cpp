#include "CONFIG.h"

void peripheral_init(peripheral *pin){
  pin ->ZIGBEE_RX = 16;
  pin ->ZIGBEE_TX = 17;
  pin ->LED_PIN = 5;
  pin ->BUTTON_PIN = 32;
  pin ->ENABLE = 25;
  pin ->MOTOR_PIN1 = 27;
  pin ->MOTOR_PIN2 = 26;
}
