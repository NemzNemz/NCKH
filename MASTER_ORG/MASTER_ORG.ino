#include <HardwareSerial.h>
#include "MASTER_PIN_CFG.h"
#include "FUNCTION.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

unsigned long prev = 0;
uint16_t interval = 2000;

unsigned long prev2 = 0;
uint16_t interval2 = 1700;
uint8_t slave_id = 1;

//Cau hinh UART
typedef struct uart_variable{
  const uart_port_t uart_num = UART_NUM_1;
  const int rx_buffer_size = 1024;
  const int tx_buffer_size = 1024;
  const int queue_size = 10;
}uart_variable;
uart_variable uart_var;
QueueHandle_t uart_queue;

char buffer[40];
int buf_index = 0;
float lastValueSLV1 = 1;
float lastValueSLV2 = 1;

HardwareSerial zigbeeSerial(1); // ESP32: TX=17, RX=16 (Zigbee)

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
    peripheral_init(&pin);
    zigbeeSerial.begin(115200, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX);

    in_text_ra_lcd();
}

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
  nhan_data();
  poll_id();
} 