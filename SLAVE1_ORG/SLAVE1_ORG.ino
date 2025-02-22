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

const uart_port_t uart_num = UART_NUM_1;

//esp-idf/components/hal/esp32/include/haluart_ll.h
#define UART_RXFIFO_FULL_INT_CLR (0x01<<0)
#define UART_RXFIFO_TOUT_INT_CLR (0x01<<8)
const int rx_buffer_size = 1024;
const int tx_buffer_size = 1024;
const int queue_size = 10;

uart_config_t uart_config{
  .baud_rate = slv.BAUD_RATE,
  .data_bits = UART_DATA_8_BITS,
  .parity = UART_PARITY_DISABLE,
  .stop_bits = UART_STOP_BITS_1,
  .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  // EXTEND UART_CONFIG_T
  //.rx_flow_ctrl_thresh = 122,
  // .source_clk =
  // .backup_before_sleep =
  // .flags = {.enable = true, .low_power_mode = false}
};

//Ham ngat prototype
static void IRAM_ATTR uart_intr_handle(void *arg){
  //6 bit vua du cho "SLV_01"
  uint8_t buf[7];
  size_t rx_fifo_len;

  uart_get_buffered_data_len(uart_num, &rx_fifo_len);
  //Neu data vua du SLV_01
  if(rx_fifo_len >= 6){
    uart_read_bytes(uart_num, buf, 6, 0);
    buf[7] = '\0';
    Serial.printf("RX Length: %d\n", rx_fifo_len);

    if(strstr((char*)buf, slv.SLAVE_COMMAND)!=NULL){
      //Gui data qua UART
      char buffer[40];
      snprintf(buffer, sizeof(buffer),"Temp_SL:%d\n", data.water_level);
      uart_write_bytes(uart_num, buffer, strlen(buffer));
    }
  }
  //Xoa co ngat
  uart_flush(uart_num);
  uart_clear_intr_status(uart_num, UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
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

  //Cau hinh UART
  uart_param_config(uart_num, &uart_config);
  //Cau hinh chan UART
  uart_set_pin(uart_num, pin.ZIGBEE_TX, pin.ZIGBEE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  //Cai dat driver
  uart_driver_install(uart_num, rx_buffer_size, tx_buffer_size, queue_size, NULL, 0);

  //Cau hinh ngat UART
  uart_enable_rx_intr(uart_num);
  //uart_isr_register ko xai duoc
  gpio_isr_register(uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, NULL);

  pinMode(pin.VCC_WTR, OUTPUT);
  pinMode(pin.WTR_PIN, INPUT);
}

void loop() {
  unsigned long now_main = millis();
  if(now_main - prev_main >= interval_main){
    raw_wtr(&data);
    water_lv(&data);

    // Serial.print("RAW ");
    // Serial.println(data.raw_value_wtr);
    // Serial.print("% ");
    // Serial.println(data.water_level);
    //sendDataToGoogleSheets();

    prev_main = now_main;
  }
}
