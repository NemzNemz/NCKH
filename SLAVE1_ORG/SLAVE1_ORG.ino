  #include "PIN_CFG.h"
  #include <WiFi.h>
  #include "Sensor_data.h"
  #include "GG_SHEET.h"
  #include "driver/uart.h"
  #include "freertos/queue.h"

  unsigned long prev_main = 0;
  uint16_t interval_main = 2000;

  struct SLAVE_CFG{
    const char* SLAVE_COMMAND = "SLV_01";  
    const int BAUD_RATE = 115200;
  }slv;

  //Cau hinh UART
  struct uart_variable{
    const uart_port_t uart_num = UART_NUM_1;
    //Bo dem nhan(rx) 1024 byte
    const int rx_buffer_size = 1024;
    //Bo dem gui(tx) 1024 byte
    const int tx_buffer_size = 1024;
    //Hang doi size 10
    const int queue_size = 10;
  }uart_var;
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
    
    //Cau hinh UART_1, voi che do 8N1, xai xung mac dinh
    uart_config_t uart_config = {
      .baud_rate = slv.BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB
    };
    uart_param_config(uart_var.uart_num, &uart_config);
    //Gan chan UART1 bang TX_RX, khong su dung CTS/RST
    uart_set_pin(uart_var.uart_num, pin.ZIGBEE_TX, pin.ZIGBEE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Cai dat driver ngat UART voi hang doi su kien
    uart_driver_install(uart_var.uart_num, uart_var.rx_buffer_size, uart_var.tx_buffer_size, uart_var.queue_size, &uart_queue, 0);

    pinMode(pin.VCC_WTR, OUTPUT);
    pinMode(pin.WTR_PIN, INPUT);
    pinMode(pin.P0_PIN, INPUT);
    //Do phan giai Analog 12bit de su dung TCRT tam thoi
    analogReadResolution(12);
  }

void loop() {
  // raw_wtr(&data);
  // Serial.println("Raw WTR: " + String(data.raw_value_wtr));
  // water_lv(&data);
  // Serial.println("Water Level: " + String(data.water_level));
  // cal_ph(pin.P0_PIN, &data);
  // Serial.println(data.ph_value);
  // delay(2000);

    uart_event_t event;
    //Neu co su kien (wait max 300ms)
    if(xQueueReceive(uart_queue, &event, pdMS_TO_TICKS(500))){
      //Neu su kien la UART_DATA
      if(event.type == UART_DATA){
        //Tao buffer luu data
        char rx_buf[256];
        //doc tung byte data UART vao bo nho dem(buffer) bang UART_read_bytes
        int len = uart_read_bytes(uart_var.uart_num, rx_buf, event.size, 0);
        //Mo rong neu nhu co van de ve data, dam bao data khong rong
        //if(len > 0){}
        //Dam bao ket thuc chuoi
        rx_buf[len] = '\0';
        
        //Neu data nhan la SLAVE_COMMAND(SLAVE_1)
        if(strstr(rx_buf, slv.SLAVE_COMMAND) != NULL){
          raw_wtr(&data);
          water_lv(&data);
          cal_ph(pin.P0_PIN, &data);
          char res_buf[120];
          //Luu buffer bang snprintf voi sizeof (data cuar tcrt_buf)
          snprintf(res_buf, sizeof(res_buf), "SLV1: WTR: %.1f\nSLV1: P.H: %.1f\n", data.water_level, data.ph_value);
          //Gui qua master bang uart_write_bytes
          uart_write_bytes(uart_var.uart_num, res_buf, strlen(res_buf));
          Serial.println(String(res_buf));
        }
      }
    } else {
      Serial.println("No data yet");
    }
    // // unsigned long now_main = millis();
    // // if(now_main - prev_main >= interval_main){
    // //   raw_wtr(&data);
    // //   water_lv(&data);
    // //   cal_ph(pin.P0_PIN, &data);
    // //   Serial.println(data.ph_value);
    // //   prev_main = now_main;
    // // }
  }
