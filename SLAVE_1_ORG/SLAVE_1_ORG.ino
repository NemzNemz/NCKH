#include <HardwareSerial.h>
//Cau hinh chan
typedef struct peripheral_pin{
  const uint8_t ZIGBEE_RX = 16;
  const uint8_t ZIGBEE_TX = 17;
  const uint8_t VCC_WTR = 12;
  const uint8_t WTR_PIN = 14;
} PIN;
PIN pin;

HardwareSerial zigbeeSerial(1); // ESP 17-TX 16-RX Zigbee

//Cau hinh Slave
#define SLAVE_ID 1  
#define SLAVE_COMMAND "SLV_01"  
#define LOG_SIZE 10  // Kích thước queue log 
#define BAUD_RATE 115200

typedef struct Sensor_Data{
  float water_level;
  int raw_value_wtr;
  //Se con nhieu cam bien hon nua (3~5)
} sensor;
sensor data;

unsigned long millis_truoc = 0;
uint8_t interval = 100;

QueueHandle_t slave_queue;

// Danh sach task (ENG)
TaskHandle_t read_send_zigbee;
TaskHandle_t WTR_TASK;

int raw_wtr(){
  unsigned long millis_ht = millis();
  if(millis_ht - millis_truoc > interval){
    digitalWrite(pin.VCC_WTR, 1);
    data.raw_value_wtr = analogRead(pin.WTR_PIN);
    digitalWrite(pin.VCC_WTR, 0);
    millis_truoc = millis_ht;
  }
  return data.raw_value_wtr;
}

float water_lv() {
  data.water_level = (data.raw_value_wtr / 1450.0) * 100.0;
  return data.water_level;
}

void setup() {
    Serial.begin(115200);
    zigbeeSerial.begin(115200, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX); // RX=16, TX=17
    pinMode(pin.VCC_WTR, OUTPUT);
    pinMode(pin.WTR_PIN, INPUT);

    slave_queue = xQueueCreate(LOG_SIZE, sizeof(char[100]));
    if (!slave_queue) {
        Serial.println("Khong tao duoc queue log!!");
        while (1);
    }

    //Cac task duoc tao
    //xTaskCreatePinnedToCore(send_wtr, "Gui muc nuoc", 4096, NULL, 1, &WTR_TASK, 1);
    xTaskCreatePinnedToCore(read_send_zigbee_f, "doc gui zigbee", 4096, NULL, 1, &read_send_zigbee, 1);
}

// void send_wtr(void *pvParameters){
//   while(true){
//     raw_wtr();
//     water_lv();
//     //Ham Prototype chua hoan thien
//     zigbeeSerial.println(data.water_level);
//     vTaskDelay(3000/ portTICK_PERIOD_MS);
//   }
// }

void read_send_zigbee_f(void *pvParameters) {
    char rcv_data[100];
    //co thoi gian chong gui 2 lan
    unsigned long millis_truoc = 0;
    const uint16_t INTERVAL = 1500;
    //Luu lenh cuoi cung da xu ly
    String last_cmd = "";

    while (true) {
      while (zigbeeSerial.available()) {
        //Doc chuoi den ky tu xuong dong
        String command = zigbeeSerial.readStringUntil('\n');
        //Serial.println("Received command: " + command);
        command.trim();

        if(command != last_cmd){
          last_cmd = command;

          String log = "SLV1 RCV:" + command;
          log.toCharArray(rcv_data, sizeof(rcv_data));
          xQueueSend(slave_queue, rcv_data, 0);
              
          if (command == SLAVE_COMMAND) {
            unsigned long millis_ht = millis();
            if(millis_ht - millis_truoc > INTERVAL){
              millis_truoc = millis_ht;
              raw_wtr();
              water_lv();

              zigbeeSerial.println(data.water_level, 2);
              zigbeeSerial.println();
            }
          }
        }
      }
    }
  vTaskDelay(100 / portTICK_PERIOD_MS);
}



void loop() {
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
