#include <HardwareSerial.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "MASTER_PIN_CFG.h"

//Chu y, can khoi tao 1 con tro toan cuc, neu ko se bi gan gia tri rac?? (Trong khi da dinh nghia toan cuc?), LCD ko chay! Kien thuc chua duoc hoc !
Adafruit_ILI9341 *tft = NULL;

HardwareSerial zigbeeSerial(1); // ESP32: TX=17, RX=16 (Zigbee)

//Giao thuc ket noi
typedef struct protocol_cfg{
  const int BAUD_RATE = 115200;
  const uint8_t LOG_SIZE = 10;
  const uint8_t MESS_SIZE = 100;
  const uint16_t TIME_OUT = 2000;//2s Slave rep
}protocol;
protocol ptl;

//Hang doi
QueueHandle_t queue1;

//Cac task
typedef struct RTOS_Task{
  TaskHandle_t slave_id;
  TaskHandle_t read_zigbee;
  TaskHandle_t print_data;
}Task_Handle;
Task_Handle task;

uint8_t slave_ID = 1;
uint8_t* ptr_slave_ID = &slave_ID;

//bien kiem soat reply cua Slave
volatile bool rep;

void freeTFT() {
    if (tft) { 
        delete tft; 
        tft = NULL; //Tranh dangling pointer
    }
}

void Display(){
  tft->begin();
  tft->setRotation(2);
  tft->fillScreen(ILI9341_BLACK);
  tft->setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  tft->setTextSize(2);

  tft->setCursor(20, 20);
  tft->println("NCKH HELLO");
}

void setup() {
    Serial.begin(ptl.BAUD_RATE);
    zigbeeSerial.begin(ptl.BAUD_RATE, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX);
    lcd_init(&lcd);
    peripheral_init(&pin);

    freeTFT();
    //Cach fix cua Claude, kien thuc chua duoc hoc!
    //Lien quan den thu tu khoi tao, nhung goi ham truoc Adafruit thi se ko hop le
    //Chu y cap phat dong "new"
    tft = new Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

    queue1 = xQueueCreate(ptl.LOG_SIZE, sizeof(char[ptl.MESS_SIZE]));
    if (!queue1) {
        Serial.println("Không tạo được queue log!");
        while (1);
    }
    Display();

    // Tao tak
    xTaskCreatePinnedToCore(poll_id, "Chon ID", 2048, NULL, 1, &task.slave_id, 1);
    xTaskCreatePinnedToCore(doc_zigbee, "Doc zigbee", 4096, NULL, 1, &task. read_zigbee, 1);
    xTaskCreatePinnedToCore(in_data, "In data", 8192, NULL, 1, &task.print_data, 1);
}

void poll_id(void *pvParameters) {
  char log[ptl.MESS_SIZE];
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
            while((millis()- bat_dau) < ptl.TIME_OUT){
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

            while((millis() - bat_dau2) < ptl.TIME_OUT){
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
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void doc_zigbee(void *pvParameters) {
    char rcv_data[ptl.MESS_SIZE];
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
    char received_data[ptl.MESS_SIZE];
    while (true) {
        if (xQueueReceive(queue1, &received_data, 0)) {
            Serial.println(received_data);
            
            String str = String(received_data);
            //Chuyen doi chu thanh so
            float waterVal = str.toFloat();
            
            if (waterVal >= 0.0) {  
                tft->fillRect(30, 90, 200, 30, ILI9341_BLACK);
                tft->setCursor(30, 90);
                tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
                tft->setTextSize(2);
                tft->print("Water = ");
                tft->print(waterVal, 2);
                tft->println("%");
            }
      }
        vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void loop() {}
