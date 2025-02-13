#include <HardwareSerial.h>
//Cau hinh chan
#define ZIGBEE_RX 16
#define ZIGBEE_TX 17

HardwareSerial zigbeeSerial(1); // ESP 17-TX 16-RX Zigbee

//Giao thuc ket noi
#define BAUD_RATE 115200
#define LOG_SIZE 10
#define MESS_SIZE 100
//#define TIME_OUT 2000 //Thoi gian rep la 2s cho Slave

//Cau hinh Slave
#define SLAVE_ID 2  
#define SLAVE_COMMAND "SLV_02"  
#define SLAVE_REP "Temp_SL2: 2222"  

QueueHandle_t slave_queue;

// Danh sach task (ENG)
TaskHandle_t print_log;
TaskHandle_t read_send_zigbee;

void setup() {
    Serial.begin(115200);
    zigbeeSerial.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17

    slave_queue = xQueueCreate(LOG_SIZE, sizeof(char[100]));
    if (!slave_queue) {
        Serial.println("Khong tao duoc queue log!!");
        while (1);
    }

    xTaskCreatePinnedToCore(
        print_log_f,
        "in tin nhan",
        4096,
        NULL,
        1,
        &print_log,
        1
    );

    xTaskCreatePinnedToCore(
        read_send_zigbee_f,
        "doc gui zigbee",
        4096,
        NULL,
        1,
        &read_send_zigbee,
        1
    );
}

void read_send_zigbee_f(void *pvParameters) {
    char rcv_data[100];
    unsigned long truoc_do = 0;
    const uint16_t INTERVAL = 1000;
    //Luu lenh cuoi cung da xu ly
    String last_cmd = "";

    while (true) {
        while (zigbeeSerial.available()) {
            //Doc chuoi den ky tu xuong dong
            String command = zigbeeSerial.readStringUntil('\n');
            //Serial.println("Received command: " + command);
            
            // Xoa het ki tu tao lao nhu /r /n
            command.trim();
            if(command != last_cmd){
              last_cmd = command;

              String log = "SLV2 RCV:" + command;
              log.toCharArray(rcv_data, sizeof(rcv_data));
              xQueueSend(slave_queue, rcv_data, 0);
              
              if (command == SLAVE_COMMAND) {
                unsigned long hien_tai = millis();
                if(hien_tai - truoc_do >INTERVAL){
                  zigbeeSerial.println(SLAVE_REP);
                  truoc_do = hien_tai;
                  
                  // Log vao Queue
                  String rep = "Slave1 responded: Temp_SL2: 2222";
                  rep.toCharArray(rcv_data, sizeof(rcv_data));
                  xQueueSend(slave_queue, rcv_data, 0);
              }
            }
          }
        }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void print_log_f(void *pvParameters) {
    char log_array[MESS_SIZE];
    while (true) {
        if (xQueueReceive(slave_queue, &log_array, 0)) {
            Serial.println(log_array);
            Serial.println();
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void loop() {
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
