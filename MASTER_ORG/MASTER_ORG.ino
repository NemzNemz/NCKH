#include "MASTER_PIN_CFG.h"
#include "FUNCTION.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include <WiFi.h>

//Config Firebase
#define WIFI_SSID "Trieu Ninh"
#define WIFI_PASS "12344321"
#define API_KEY "AIzaSyCNLfTrT6w3K2ipz9DBT198YocfGbZarII"
#define DATABASE_URL "https://nckh-dd303-default-rtdb.firebaseio.com/"

bool    frameOn  = false;     // đang trong giai đoạn ghi frame
size_t  frameIdx = 0;         // chỉ số write buffer
char    frameBuf[64];         // lưu khung "<Sx…>\n"

char buffer[40];
int  buf_index = 0;

//Noi luu gio, phut chay daily_task
int taskHour_on = 0;    
int taskMinute_on = 30;    
int taskHour_off = 21;    
int taskMinute_off = 31;  
const char* timezone = "ICT-7"; //Time zone VN

//Bien toan cuc chay Daily task
time_t now;
struct tm timeinfo;

//Bien sync NTP
unsigned long lastNTPUpdate = 0;
const unsigned long ntpSyncInterval = 30 * 60 * 1000; //30 phut 1 lan

//Cac bien danh cho Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signUpOK = false;

//Cac bien trang thai
volatile int ledStatus = 0;
int oldLedStatus = -1;  
int old_firebase_status = -1;
int last_run_day_on = -1;
int last_run_day_off = -1;

//Bien kiem soat data
typedef struct last_data_value{
  float last_WTR_SLV1;          
  float last_WTR_SLV2; 
  float last_PH_SLV1;
  float last_PH_SLV2;
  float last_TDS_SLV1;
  float last_TDS_SLV2 ;
}last_data;
last_data data = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

//Khoi tao LCD
lcd_pin lcd;
Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

//Poll ID
unsigned long prev = 0;
uint16_t interval = 5000;

typedef struct timing_variables{
  //Gui len firebase
  unsigned long prev_send_fb;   
  uint16_t interval_send_fb;    
  //Gui Data         
  unsigned long prev_send_data;   
  uint16_t interval_send_data;    
  //Debounce nut nhan
  volatile unsigned long prev_debounce;
  uint8_t interval_debounce;
}timing_var;
timing_var timing ={0, 1000, 0, 5000, 0, 150};

//Chong xung dot
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
HardwareSerial zigbeeSerial(1); // ESP32: TX=17, RX=16 (Zigbee)

void IRAM_ATTR nhan_nut(){
  unsigned long now_debounce = millis();
  portENTER_CRITICAL_ISR(&mux);

  if(now_debounce - timing.prev_debounce > timing.interval_debounce){
    //Serial.println("Da an nut!");
    ledStatus = !ledStatus;
    digitalWrite(pin.LED_PIN, ledStatus);
    timing.prev_debounce = now_debounce;
  }
  portEXIT_CRITICAL_ISR(&mux);
}

void in_text_ra_lcd(){
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
    tft.setTextSize(2);

    tft.setCursor(100, 10);
    tft.println("HELLO NCKH");

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.setCursor(20, 40);
    tft.println("SLV1_DATA");
    tft.setCursor(20, 70);
    tft.println("TDS:");
    tft.setCursor(20, 100);
    tft.println("P.H:");
    tft.setCursor(20, 130);
    tft.println("WTR:");
    
    tft.setCursor(200, 40);
    tft.println("SLV2_DATA");
    tft.setCursor(200, 70);
    tft.println("TDS:");
    tft.setCursor(200, 100);
    tft.println("P.H:");
    tft.setCursor(200, 130);
    tft.println("WTR:");

    tft.setCursor(20, 160);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.println("LED_STATE:");
}

void sync_time() {
  Serial.print("Dang dong bo thoi gian NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  //Doi den khi co thoi gian hop le
  while (now < 24 * 3600) 
  {
    delay(100);
    now = time(nullptr);
  }
  Serial.println("Done!");

  setenv("TZ", timezone, 1);
  tzset();

  lastNTPUpdate = millis(); 
}

void daily_task_test_on(){
  ledStatus = 1;
  digitalWrite(pin.LED_PIN, ledStatus);
}

void daily_task_test_off(){
  ledStatus = 0;
  digitalWrite(pin.LED_PIN, ledStatus);
}

void setup() {
  Serial.begin(115200);
  peripheral_init(&pin);
  pinMode(pin.LED_PIN, OUTPUT);
  pinMode(pin.BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin.BUTTON_PIN), nhan_nut, FALLING);
  digitalWrite(pin.LED_PIN, 0);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Dang ket noi voi Wifi");
  while(WiFi.status() != WL_CONNECTED){
    Serial.println(".");
    delay(400);
  }
  Serial.println("DA KET NOI");
  Serial.println(WiFi.localIP());
  Serial.println();
  //Lay thoi gian
  sync_time();

  //Ket noi Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Sign UP!");
    signUpOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  //Doc cau hinh thoi gian
  //readDailyTaskSchedule();  

  in_text_ra_lcd();
  zigbeeSerial.begin(115200, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX);
}

void processSLV2Data(const char *s) {
  float wtr, ph, tds;
  if (sscanf(s, "<S2,WTR=%f,PH=%f,TDS=%f>", &wtr, &ph, &tds) == 3) {
    data.last_WTR_SLV2 = wtr;
    data.last_PH_SLV2  = ph;
    data.last_TDS_SLV2 = tds;

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    //Ve TDS, PH va WTR
    tft.fillRect(260, 70, 60, 15, ILI9341_BLACK);
    tft.setCursor(260, 70);  
    tft.print(tds, 1);

    tft.fillRect(260, 100, 60, 15, ILI9341_BLACK);
    tft.setCursor(260, 100);  
    tft.print(ph, 1);

    tft.fillRect(260, 130, 60, 15, ILI9341_BLACK);
    tft.setCursor(260, 130);  
    tft.print(wtr, 1);
  }
}

void processSLV1Data(const char *s) {
  //Tham so nhan vao la 1 chuoi
  float wtr, ph, tds;
  // sscanf bo qua '<', 'S1,' chi doc WTR, PH, TDS
  if (sscanf(s, "<S1,WTR=%f,PH=%f,TDS=%f>", &wtr, &ph, &tds) == 3) {
    data.last_WTR_SLV1 = wtr;
    data.last_PH_SLV1  = ph;
    data.last_TDS_SLV1 = tds;

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    //Ve TDS, PH va WTR
    tft.fillRect( 80, 70, 60, 15, ILI9341_BLACK);
    tft.setCursor(80, 70);  
    tft.print(tds, 1);

    tft.fillRect( 80, 100, 60, 15, ILI9341_BLACK);
    tft.setCursor(80,100);  
    tft.print(ph,  1);

    tft.fillRect( 80, 130, 60, 15, ILI9341_BLACK);
    tft.setCursor(80,130);  
    tft.print(wtr, 1);
  }
}

void nhan_data() {
  while (zigbeeSerial.available()) {
    char c = zigbeeSerial.read();

    // Bỏ CR luôn 
    if (c == '\r') continue;

    //Nếu chưa bắt đầu frame, chỉ quan tâm đến '<' 
    if (!frameOn) {
      if (c == '<') {
        frameOn = true;
        frameIdx = 0;
        frameBuf[frameIdx++] = c;   // ghi '<'
      }
      continue;  // bỏ qua mọi byte khác
    }

    //Đang trong frame: ghi tiếp nếu đủ chỗ
    if (frameIdx < sizeof(frameBuf) - 1) {
      frameBuf[frameIdx++] = c;
    } 
    else {
    //overflow: hủy khung
      frameOn  = false;
      frameIdx = 0;
      continue;
    }

    // Khi gặp newline, kết thúc frame
    if(c == '\n') {
      frameBuf[frameIdx] = '\0';   // đóng chuỗi
      Serial.print("Chuoi nhan duoc: ");
      Serial.println(frameBuf);

    // GỌI HÀM XỬ LÝ
    if(strncmp(frameBuf, "<S1,", 4) == 0) processSLV1Data(frameBuf);
    else if(strncmp(frameBuf, "<S2,", 4) == 0) processSLV2Data(frameBuf);
    else Serial.println("Khong nhan dien duoc slave_id");

    // Reset để đón frame kế
    frameOn  = false;
    frameIdx = 0;
    }
  }
}

void send_value_to_firebase(){
  unsigned long now_send_data = millis();
    if (now_send_data - timing.prev_send_data > timing.interval_send_data) {
      if (Firebase.ready() && signUpOK) {
        float WTR_1 = data.last_WTR_SLV1;
        float WTR_2 = data.last_WTR_SLV2;
        float PH_1 = data.last_PH_SLV1;
        float PH_2 = data.last_PH_SLV2;
        float TDS_1 = data.last_TDS_SLV1;
        float TDS_2 = data.last_TDS_SLV2;
          //Gui WTR_SLV1
          if (Firebase.RTDB.setFloat(&fbdo, "Sensor/WTR_data1", WTR_1)) {
            //Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else{
            Serial.println("Thất bại: " + fbdo.errorReason());
          }

          //Gui PH_SLV1
          if(Firebase.RTDB.setFloat(&fbdo, "Sensor/PH_data1", PH_1)) {
            //Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else {
            Serial.println("Thất bại: " + fbdo.errorReason());
          }

          //Gui TDS_SLV1
          if(Firebase.RTDB.setFloat(&fbdo, "Sensor/TDS_data1", TDS_1)) {
            //Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else {
            Serial.println("Thất bại: " + fbdo.errorReason());
          }

          //Gui WTR_SLV2
          if(Firebase.RTDB.setFloat(&fbdo, "Sensor/WTR_data2", WTR_2)) {
            //Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else {
            Serial.println("Thất bại: " + fbdo.errorReason());
          }

          //Gui PH_SLV2
          if(Firebase.RTDB.setFloat(&fbdo, "Sensor/PH_data2", PH_2)) {
            //Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else {
            Serial.println("Thất bại: " + fbdo.errorReason());
          }

          //Gui TDS_SLV2
          if(Firebase.RTDB.setFloat(&fbdo, "Sensor/TDS_data2", TDS_2)) {
            //Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else {
            Serial.println("Thất bại: " + fbdo.errorReason());
        }
      timing.prev_send_data = now_send_data;
    }
  }
}

void send_state_to_firebase(){
  if (Firebase.ready() && signUpOK) {
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/LED", ledStatus)) {
      Serial.println("Gui trang thai len Firebase thanh cong");
      old_firebase_status = ledStatus;
      } 
    else {
      Serial.println("Gui firebase that bai: " + String(fbdo.errorReason().c_str()));
    }
  }
}

void lcd_change_led_state(){
  if (ledStatus != oldLedStatus) {
  //Chi cap nhat LCD khi co thay doi de do lagg
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.fillRect(160, 160, 60, 15, ILI9341_BLACK);
    tft.setCursor(150, 160);
    if (ledStatus == 1) {
      tft.print("ON");
    } 
    else {
      tft.print("OFF");
    }
      oldLedStatus = ledStatus; 
  }
}

void loop() {
  static uint8_t slave_id = 1;
  poll_id(slave_id);
  nhan_data();
  //readFirebaseData();
  //send_value_to_firebase();

  unsigned long nowww = millis();
  if(nowww - timing.prev_send_fb > timing.interval_send_fb){
    readFirebaseData();
    timing.prev_send_fb = nowww;
  }

  //Chi cap nhat Firebase khi co thay doi de do lagg
  if(ledStatus != old_firebase_status){
    send_state_to_firebase();
    lcd_change_led_state();
  }
  //delay(3000);
  //readDailyTaskSchedule();
  // Serial.printf("Giờ hiện tại: %02d:%02d:%02d - Ngày: %04d-%02d-%02d\n",
  //               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
  //               timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  now = time(nullptr);
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_hour == taskHour_on && timeinfo.tm_min == taskMinute_on && last_run_day_on != timeinfo.tm_mday) {
    daily_task_test_on();  
    //Dam bao chi chay 1 lan trong ngay
    last_run_day_on = timeinfo.tm_mday;  
  }
  if (timeinfo.tm_hour == taskHour_off && timeinfo.tm_min == taskMinute_off && last_run_day_off != timeinfo.tm_mday) {
    daily_task_test_off();
    //Dam bao chi chay 1 lan trong ngay  
    last_run_day_off = timeinfo.tm_mday;  
  }
} 