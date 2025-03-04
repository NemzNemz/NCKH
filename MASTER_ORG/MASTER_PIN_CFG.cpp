#include "MASTER_PIN_CFG.h"

peripheral pin;

// void lcd_init(lcd_pin *lcd){
//   lcd -> TFT_CS = 0;
//   lcd -> TFT_DC = 2;
//   lcd -> TFT_MOSI = 14;
//   lcd -> TFT_SCLK = 12;
//   lcd -> TFT_RST = 13;
// }

void peripheral_init(peripheral *pin){
  pin ->ZIGBEE_RX = 16;
  pin ->ZIGBEE_TX = 17;
}
