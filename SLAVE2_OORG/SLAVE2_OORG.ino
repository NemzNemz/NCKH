#include "PIN_CFG.h"
#include <WiFi.h>
#include "Sensor_data.h"
#include "GG_SHEET.h"
//Thu vien cua Espressif
#include "driver/uart.h"
#include "freertos/queue.h" 

//Khai bao bien peripheral ten la pin
unsigned long prev_main = 0;
uint16_t interval_main = 2000;

typedef struct SLAVE_CFG{
  const uint8_t SLAVE_ID = 2; 
  const char* SLAVE_COMMAND = "SLV_02";  
  const int BAUD_RATE = 115200;
}Slave;
Slave slv;

//Cau hinh UART
typedef struct uart_variable{
  const uart_port_t uart_num = UART_NUM_1;
  const int rx_buffer_size = 1024;
  const int tx_buffer_size = 1024;
  const int queue_size = 10;
}uart_variable;
uart_variable uart_var;
QueueHandle_t uart_queue;

void setup() {
  Serial.begin(slv.BAUD_RATE);
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  Serial.println("\nWiFi Connected!");
  pin_cfg_init(&pin);

  //Cau hinh UART 
  uart_config_t uart_config{
  .baud_rate = slv.BAUD_RATE,
  .data_bits = UART_DATA_8_BITS,
  .parity = UART_PARITY_DISABLE,
  .stop_bits = UART_STOP_BITS_1,
  .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  .source_clk = UART_SCLK_APB,
  };
  uart_param_config(uart_var.uart_num, &uart_config);

  //Set pins
  uart_set_pin(uart_var.uart_num, pin.ZIGBEE_TX, pin.ZIGBEE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  //Cai driver UART
  uart_driver_install(uart_var.uart_num, uart_var.rx_buffer_size, uart_var.tx_buffer_size, uart_var.queue_size, &uart_queue, 0);

  pinMode(pin.VCC_WTR, OUTPUT);
  pinMode(pin.WTR_PIN, INPUT);
}

void loop() {
  uart_event_t event;
    if(xQueueReceive(uart_queue, &event, pdMS_TO_TICKS(300))){
      if(event.type == UART_DATA){
        char rx_buf[256];
        //delay(500);
        int len = uart_read_bytes(uart_var.uart_num, rx_buf, event.size, 0);
        //Ki tu ket thuc chuoi
        rx_buf[len] = '\0';

        if(strstr(rx_buf, slv.SLAVE_COMMAND) != NULL){
          //1 Mang luu chuoi chuan bi gui
          char buffer[40];
          snprintf(buffer, sizeof(buffer), "WTR: %d\n", data.water_level);
          uart_write_bytes(uart_var.uart_num, buffer, strlen(buffer));
          //In log
          Serial.println("Gui: "+String(buffer));
        }
      }
    else Serial.println("No data yet");

    unsigned long now_main = millis();
    if(now_main - prev_main >= interval_main){
      raw_wtr(&data);
      water_lv(&data);

      // Serial.print("RAW ");
      // Serial.println(data.raw_value_wtr);
      // Serial.print("% ");
      // Serial.println(data.water_level);
      // sendDataToGoogleSheets();
      prev_main = now_main;
    }
  }
}
