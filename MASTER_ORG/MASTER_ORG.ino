#include <HardwareSerial.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Đám này cho LCD ILI9341
#define TFT_CS 0
#define TFT_DC 2
#define TFT_MOSI 14
#define TFT_SCLK 12
#define TFT_RST 13
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

//Cau hinh chan
#define ZIGBEE_RX 16
#define ZIGBEE_TX 17
HardwareSerial zigbeeSerial(1); // ESP32: TX=17, RX=16 (Zigbee)

//Giao thuc ket noi
#define BAUD_RATE 115200
#define LOG_SIZE 10
#define MESS_SIZE 100
#define TIME_OUT 2000 //Thoi gian rep la 2s cho Slave

uint8_t slave_ID = 1;
uint8_t* ptr_slave_ID = &slave_ID;

//Hang doi
QueueHandle_t queue1;

//Cac task
TaskHandle_t slave_id;
TaskHandle_t read_zigbee;
TaskHandle_t print_data;

//bien kiem soat reply cua Slave
volatile bool rep;

void Display(){
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  tft.setTextSize(2);

  tft.setCursor(20, 10);
  tft.println("NCKH HELLO");
}

void setup() {
    Serial.begin(BAUD_RATE);
    zigbeeSerial.begin(BAUD_RATE, SERIAL_8N1, ZIGBEE_RX, ZIGBEE_TX);

    // Tao queue
    queue1 = xQueueCreate(LOG_SIZE, sizeof(char[MESS_SIZE]));
    if (!queue1) {
        Serial.println("Không tạo được queue log!");
        while (1);
    }

    Display();

    // Tao tak
    xTaskCreatePinnedToCore(poll_id, "Chon ID", 2048, NULL, 1, &slave_id, 1);
    xTaskCreatePinnedToCore(doc_zigbee, "Doc zigbee", 4096, NULL, 1, &read_zigbee, 1);
    xTaskCreatePinnedToCore(in_data, "In data", 4096, NULL, 1, &print_data, 1);
}

void poll_id(void *pvParameters) {
  char log[MESS_SIZE];
    while (true) {
        if (*ptr_slave_ID == 1) {
            zigbeeSerial.println("SLV_01");
            //
            snprintf(log, sizeof(log), "Gui Slave 1");
            xQueueSend(queue1, &log, 0);
            
            //mac dinh xem slave chua rep
            rep = false;

            //Bat dau dem thoi gian va bien kiem tra het thoi gian cho
            unsigned long bat_dau = millis();
            bool het_gio = true;

            //Neu thoi gian rep duoi 2s, out 
            while((millis()- bat_dau) < TIME_OUT){
              if(rep){
                het_gio = false;
                break;
              }
              vTaskDelay(50 / portTICK_PERIOD_MS);
            }

            //Neu qua 2s, in ra log 
            if(het_gio){
              snprintf(log, sizeof(log), "Het gio cho tn cua SLAVE 1");
              xQueueSend(queue1, &log, 0);
            }
            *ptr_slave_ID = 2;

        //Tuong tu ben tren
        } else if (*ptr_slave_ID == 2) {
            zigbeeSerial.println("SLV_02");
            snprintf(log, sizeof(log), "Gui Slave 2");
            xQueueSend(queue1, &log, 0);

            rep = false;

            unsigned long bat_dau2 = millis();
            bool het_gio2 = true;

            while((millis() - bat_dau2) < TIME_OUT){
              if(rep){
                het_gio2 = false;
                break;
              }
              vTaskDelay(50 / portTICK_PERIOD_MS);
            }

            if(het_gio2){
              snprintf(log, sizeof(log), "Het gio cho tn cua SLAVE 2");
              xQueueSend(queue1, &log, 0);
            }

            *ptr_slave_ID = 1;
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void doc_zigbee(void *pvParameters) {
    char rcv_data[MESS_SIZE];
    while (true) {
        while (zigbeeSerial.available()) {
            String rcvData = zigbeeSerial.readStringUntil('\n');
            rcvData.trim();

            //Data co dong dai 0 thi 
            if (rcvData.length() == 0) {
                continue;
            }
            
            //data nhan duoc co dang ... xem nhu da rep
            if(rcvData.startsWith("Temp_SL")){
              rep = true;
            }

            //Gui vao Queue
            rcvData.toCharArray(rcv_data, sizeof(rcv_data));
            xQueueSend(queue1, &rcv_data, 0);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void in_data(void *pvParameters) {
    char received_data[100];
    while (true) {
        if (xQueueReceive(queue1, &received_data, 0)) {
            Serial.println(received_data);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void loop() {}
