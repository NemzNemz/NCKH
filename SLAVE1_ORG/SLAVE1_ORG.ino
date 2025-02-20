#include "PIN_CFG.h"
#include <WiFi.h>
#include "Sensor_data.h"
#include "GG_SHEET.h"
#include "driver/uart.h"
//Khai bao bien peripheral ten la pin
unsigned long prev_main = 0;
uint16_t interval_main = 2000;

typedef struct SLAVE_CFG{
  const uint8_t SLAVE_ID = 1; 
  const char* SLAVE_COMMAND = "SLV_01";  
  const int BAUD_RATE = 115200;
}Slave;
Slave slv;

//UART 3 KENH, XAI KENH 1
//Forum Adruino
#define RX_BUF_SIZE 1024
uart_config_t UART_CFG{
  .baud_rate = 115200,
  .data_bits = UART_DATA_8_BITS,
  .parity = UART_PARITY_DISABLE,
  .stop_bits = UART_STOP_BITS_1,
  .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

// uart_param_config(UART_NUM_1, &UART_CFG);
// uart_set_pin(UART_NUM_1, pin.ZIGBEE_TX, pin.ZIGBEE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
// uart_driver_install(UART_NUM_1, 1024 *2, 0, 0, NULL, 0);

static void IRAM_ATTR uart_intr_handle(void *arg){
  //Chua su dung
}

void setup() {
  Serial.begin(slv.BAUD_RATE);
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  Serial.println("\nWiFi Connected!");
  pin_cfg_init(&pin);
  
  uart_param_config(UART_NUM_1, &UART_CFG);
  uart_set_pin(UART_NUM_1, pin.ZIGBEE_TX, pin.ZIGBEE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_1, RX_BUF_SIZE, 0, 0, NULL, 0);

  pinMode(pin.VCC_WTR, OUTPUT);
  pinMode(pin.WTR_PIN, INPUT);
}

void loop() {
  unsigned long now_main = millis();
  if(now_main - prev_main >= interval_main){
    raw_wtr(&data);
    water_lv(&data);

    Serial.print("RAW ");
    Serial.println(data.raw_value_wtr);
    Serial.print("% ");
    Serial.println(data.water_level);
    sendDataToGoogleSheets();

    prev_main = now_main;
  }
}
