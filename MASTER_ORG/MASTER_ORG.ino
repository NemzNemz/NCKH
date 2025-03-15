#include "MASTER_PIN_CFG.h"
#include "FUNCTION.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include "driver/uart.h"
#include "freertos/queue.h"
//Firebase lib
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <WiFi.h>

//Config Firebase
#define WIFI_SSID "Trieu Ninh"
#define WIFI_PASS "12344321"
#define API_KEY "AIzaSyCNLfTrT6w3K2ipz9DBT198YocfGbZarII"
#define DATABASE_URL "https://nckh-dd303-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

lcd_pin lcd;
Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

//Poll ID
unsigned long prev = 0;
uint16_t interval = 2000;

//Nhan Data
unsigned long prev2 = 0;
uint16_t interval2 = 1700;
uint8_t slave_id = 1;

//Gui Data
unsigned long prev_send_data = 0;
uint16_t interval3 = 2000;
bool signUP_OK = false;

//Cau hinh UART
struct uart_variable{
  //Kenh UART1
  const uart_port_t uart_num = UART_NUM_1;
  //Bo nho dem nhan(RX) 2KB
  const int rx_buffer_size = 2048;
  //Bo nho dem gui(TX) 2KB
  const int tx_buffer_size = 1024;
  //Hang doi 20
  const int queue_size = 20;
}uart_var;
QueueHandle_t uart_queue;

char buffer[40];
int buf_index = 0;
float lastValueSLV1 = 1.23;
float lastValueSLV2 = 1.23;

const int led = 15;
const int button = 32;
bool state_of_led = false;

HardwareSerial zigbeeSerial(1); // ESP32: TX=17, RX=16 (Zigbee)

void IRAM_ATTR ISR(){
  state_of_led = true;
}

void in_text_ra_lcd(){
    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
    tft.setTextSize(2);

    tft.setCursor(20, 10);
    tft.println("HELLO NCKH");

    tft.setCursor(20, 35);
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.println("WTR_SLV1:");
    tft.setCursor(20, 55);
    tft.println("WTR_SLV2:");
}

void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    pinMode(button, INPUT_PULLUP);
    attachInterrupt(button, ISR, FALLING);
    digitalWrite(led, 0);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("Dang ket noi voi Wifi");
    while(WiFi.status() != WL_CONNECTED){
      Serial.println(".");
      delay(400);
    }
    Serial.println("DA KET NOI");
    Serial.println(WiFi.localIP());
    Serial.println();

    //Cau hinh UART_1, voi che do 8N1, xai xung mac dinh
    // uart_config_t uart_config = {
    //   .baud_rate = 115200,
    //   .data_bits = UART_DATA_8_BITS,
    //   .parity = UART_PARITY_DISABLE,
    //   .stop_bits = UART_STOP_BITS_1,
    //   .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    //   .source_clk = UART_SCLK_APB
    // };
    // uart_param_config(uart_var.uart_num, &uart_config);
    // //Gan chan UART1 bang TX_RX, khong su dung CTS/RST
    // uart_set_pin(uart_var.uart_num, pin.ZIGBEE_TX, pin.ZIGBEE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // //Cai dat driver ngat UART voi hang doi su kien
    // uart_driver_install(uart_var.uart_num, uart_var.rx_buffer_size, uart_var.tx_buffer_size, uart_var.queue_size, &uart_queue, 0);

    //Ket noi Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if(Firebase.signUp(&config, &auth, "", "")){
      Serial.println("Sign UP!");
      signUP_OK = true;
    }
    else{
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    peripheral_init(&pin);
    in_text_ra_lcd();
    zigbeeSerial.begin(115200, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX);
    //xTaskCreate(uart_interrupt, "uart_task", 4096, NULL, 5, NULL);
}

// void uart_interrupt(void *pvParameters){
//   uart_event_t event;
//   char rx_buf[256];
//   while(true){
//     if(xQueueReceive(uart_queue, (void*)&event, pdMS_TO_TICKS(500))){
//       if(event.type == UART_DATA){
//         int len = uart_read_bytes(uart_var.uart_num, rx_buf, event.size, 0);
//         rx_buf[len] = '\0';
//         Serial.print("Da nhan data UART!");
//         Serial.println(rx_buf);
//       }
//     }
//   }
// }

void xu_ly_data(char* data){
  //tim chuoi con trong chuoi lon
  if(strncmp(data, "WTR: ", 5) == 0){
    char* valStr = data + 5;
    float value = atof(valStr);

    if(slave_id == 1){
      if(value != lastValueSLV1){
        tft.setCursor(130, 35); 
        tft.print(value);
        lastValueSLV1 = value;
      }
    }
    else if(slave_id == 2){
      if(value != lastValueSLV2){
        tft.setCursor(130, 55); 
        tft.print(value);
        lastValueSLV2 = value;
      }
    }
  }
}

void nhan_data(){
  if(zigbeeSerial.available()){
   char c = zigbeeSerial.read();
    if(c == '\n'){
      buffer[buf_index] = '\0';
      //Serial.println("Nhan: " +String(buffer));
      unsigned long now2 = millis();
      if(now2 - prev2 > interval2){
      xu_ly_data(buffer);
      prev2 = now2;
      }
      buf_index = 0;
    }
    else{
      if(buf_index < sizeof(buffer) - 1){
        buffer[buf_index++] = c;
      }
    }
  }
}

void loop() {
  // unsigned long now = millis();
  // if (now - prev > interval) {
  //   if (slave_id == 1) {
  //     uart_write_bytes(uart_var.uart_num, "SLV_01\n", 7);
  //     Serial.println("Gui: SLV_01");
  //     slave_id = 2;
  //   } else if (slave_id == 2) {
  //     uart_write_bytes(uart_var.uart_num, "SLV_02\n", 7);
  //     Serial.println("Gui: SLV_02");
  //     slave_id = 1;
  //   }
  //   prev = now;
  // }
  poll_id();
  nhan_data();
  // unsigned long now3 = millis();
  // if(now3 - prev_send_data > interval3){
  //   if(Firebase.ready() && signUP_OK == true){
  //     float WTR_Value1 = lastValueSLV1;
  //     float WTR_Value2 = lastValueSLV2;
  //     if(Firebase.RTDB.setFloat(&fbdo, "Sensor/WTR_data1", WTR_Value1)){
  //       Serial.println();
  //       Serial.println("- thanh cong luu den: " + fbdo.dataPath());
  //       Serial.println(" (" + fbdo.dataType() + ") ");
  //     }
  //     else {
  //       Serial.println("That bai: " + fbdo.errorReason());
  //     }

  //     if(Firebase.RTDB.setFloat(&fbdo, "Sensor/WTR_data2", WTR_Value2)){
  //       Serial.println();
  //       Serial.println("- thanh cong luu den: " + fbdo.dataPath());
  //       Serial.println(" (" + fbdo.dataType() + ") ");
  //     }
  //     else {
  //       Serial.println("That bai: " + fbdo.errorReason());
  //     }
  //   }
  //  prev_send_data = now3;
  // }
  if(state_of_led == true){
    digitalWrite(led, 1);
    delay(1000);
    state_of_led = false;
  }
  else{
    digitalWrite(led, 0);
  }
} 