#include "CONFIG.h"
#include "FUNCTION.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include <WiFi.h>

//Config Firebase
#define WIFI_SSID "Been"
#define WIFI_PASS "11119999"
#define API_KEY "AIzaSyCNLfTrT6w3K2ipz9DBT198YocfGbZarII"
#define DATABASE_URL "https://nckh-dd303-default-rtdb.firebaseio.com/"

bool frameOn  = false;     // đang trong giai đoạn ghi frame
size_t frameIdx = 0;         // chỉ số write buffer
char frameBuf[64];         // lưu khung "<Sx…>\n"
buffer_t buffer = {{0}, 0};

//Noi luu gio, phut chay daily_task
DAILY_TASK daily_task = {0, 30, 21, 31};
const char* timezone = "ICT-7"; //Time zone VN
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

//Chi tiet bien kiem soat gia tri lan truoc
last_data_value data = {
  .last_WTR_SLV1 = 0.0,  
  .last_WTR_SLV2 = 0.0,
  .last_PH_SLV1 = 0.0,
  .last_PH_SLV2 = 0.0,
  .last_TDS_SLV1 = 0.0,
  .last_TDS_SLV2 = 0.0
};
//lan luot la thoi gian doc firebase, thoi gian gui data len firebase
timing_variables timing ={
  .prev_send_fb = 0,
  .interval_send_fb = 5000,         
  .prev_send_data = 0,
  .interval_send_data = 5000
};
//lan luot la led, old_led_status, old_firebase_status, bien trang thai da chay ngay do hay chua
status_var status = {
  .motor_status = 0,
  .old_motor_status = -1, 
  .old_firebase_status = 1,
  .last_run_day_on = -1,
  .last_run_day_off = 1
};
//Cau hinh chi tiet pwm cho motor
pwm_propeties motor_cfg = {
  .frequency = 30000,    //Tan so 30kHz
  .pwm_channel = 0,      //Kenh PWM 0
  .resolution = 8,       //8 bit phan giai
  .duty_cycle = 190      
};
peripheral pin;

//Khoi tao LCD
lcd_pin lcd = {0, 2, 14, 12, 13};
Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

//Chong xung dot
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
HardwareSerial zigbeeSerial(1); 

//volatile unsigned long lastButtonPress = 0;
volatile bool buttonPressed = false;
void IRAM_ATTR nhan_nut() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = micros();
  if (interruptTime - lastInterruptTime > 20000) { // 20ms debounce
    buttonPressed = true;
    lastInterruptTime = interruptTime;
  }
}

void test_dc_quay_thuan(){
  Serial.println("Quay deu quay deu!");
  digitalWrite(pin.MOTOR_PIN1, LOW);
  digitalWrite(pin.MOTOR_PIN2, HIGH);
  ledcWrite(motor_cfg.pwm_channel, motor_cfg.duty_cycle);
}

void test_dc_quay_nghich(){
  Serial.println("Quay ko deu quay ko deu!");
  digitalWrite(pin.MOTOR_PIN1, HIGH);
  digitalWrite(pin.MOTOR_PIN2, LOW);
  ledcWrite(motor_cfg.pwm_channel, motor_cfg.duty_cycle);
}

void test_dc_ngung(){
  Serial.println("Ngung di ngung di!");
  digitalWrite(pin.MOTOR_PIN1, LOW);
  digitalWrite(pin.MOTOR_PIN2, LOW);
  ledcWrite(motor_cfg.pwm_channel, 0);
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
    tft.println("MOTOR_STATE:");
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
  status.motor_status = 1;
  digitalWrite(pin.LED_PIN, status.motor_status);
  send_state_to_firebase(&status);
}

void daily_task_test_off(){
  status.motor_status = 0;
  digitalWrite(pin.LED_PIN, status.motor_status);
  send_state_to_firebase(&status);
}

void setup() {
  Serial.begin(115200);
  //Khoi tao cau hinh chan ngoai vi
  peripheral_init(&pin);
  pinMode(pin.LED_PIN, OUTPUT);
  pinMode(pin.BUTTON_PIN, INPUT_PULLUP);
  pinMode(pin.MOTOR_PIN1, OUTPUT);
  pinMode(pin.MOTOR_PIN2, OUTPUT);
  //Cau hinh PWM, bao gom chon kenh va chi dinh kenh pwm vao chan cu the
  ledcSetup(motor_cfg.pwm_channel, motor_cfg.frequency, motor_cfg.resolution);
  ledcAttachPin(pin.ENABLE, motor_cfg.pwm_channel);
  //Cau hinh ngat
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
  //readDailyTaskSchedule(daily_task);  

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

void nhan_data(buffer_t &buffer) {
  while (zigbeeSerial.available()) {
    char c = zigbeeSerial.read();
      if (c == '\r') continue;

      if (!frameOn) {
        if (c == '<') {
          frameOn = true;
          frameIdx = 0;
          frameBuf[frameIdx++] = c;
        }
          continue;
      }

      if (frameIdx < sizeof(frameBuf) - 1) {
        frameBuf[frameIdx++] = c;
      } else {
        frameOn = false;
        frameIdx = 0;
        continue;
      }

      if (c == '\n') {
        frameBuf[frameIdx] = '\0';
        //Serial.print("Chuoi nhan duoc: ");
        Serial.println(frameBuf);

      if (strncmp(frameBuf, "<S1,", 4) == 0) processSLV1Data(frameBuf);
      else if (strncmp(frameBuf, "<S2,", 4) == 0) processSLV2Data(frameBuf);
      else Serial.println("Khong nhan dien duoc slave_id");

      frameOn = false;
      frameIdx = 0;
      buffer.index = 0; // Làm sạch buffer nếu cần
    }
  }
}

void lcd_change_motor_status(){
  if (status.motor_status != status.old_motor_status) {
  //Chi cap nhat LCD khi co thay doi de do lagg
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.fillRect(170, 160, 60, 15, ILI9341_BLACK);
    tft.setCursor(170, 160);
    if (status.motor_status == 1) {
      tft.print("ON");
    } 
    else {
      tft.print("OFF");
    }
      status.old_motor_status = status.motor_status; 
  }
}

void loop() {
  // test_dc_quay_thuan();
  // delay(5000);
  // test_dc_ngung();
  // delay(1500);
  // test_dc_quay_nghich();
  // delay(5000);
  // test_dc_ngung();
  // delay(1500);
  static uint8_t slave_id = 1;
  poll_id(slave_id, buffer);
  nhan_data(buffer);;
  send_value_to_firebase(&timing, &data);
  //readDailyTaskSchedule(daily_task);

  //Moi 5s doc data tren firebase 1 lan
  unsigned long nowww = millis();
  if(nowww - timing.prev_send_fb > timing.interval_send_fb){
    readFirebaseData(&pin, &status);
    timing.prev_send_fb = nowww;
  }

  //Chi cap nhat Firebase khi co thay doi de do lagg
  if (buttonPressed) {
    status.motor_status = !status.motor_status;
    digitalWrite(pin.LED_PIN, status.motor_status);
    buttonPressed = false; // Reset cờ
  }

  if (status.motor_status != status.old_firebase_status) {
    if(status.motor_status){
      test_dc_quay_thuan();
      send_state_to_firebase(&status);
      lcd_change_motor_status();
    }
    else if(status.motor_status== false){
      test_dc_ngung();
      send_state_to_firebase(&status);
      lcd_change_motor_status();
    }
  }

  // delay(3000);
  // Serial.printf("Giờ hiện tại: %02d:%02d:%02d - Ngày: %04d-%02d-%02d\n",
  //               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
  //               timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  now = time(nullptr);
  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_hour == daily_task.taskHour_on && timeinfo.tm_min == daily_task.taskMinute_on && status.last_run_day_on != timeinfo.tm_mday) {
    daily_task_test_on();  
    //Dam bao chi chay 1 lan trong ngay
    //status.last_run_day_on = timeinfo.tm_mday;  
  }
  if (timeinfo.tm_hour == daily_task.taskHour_off && timeinfo.tm_min == daily_task.taskMinute_off && status.last_run_day_off != timeinfo.tm_mday) {
    daily_task_test_off();
    //Dam bao chi chay 1 lan trong ngay  
    //status.last_run_day_off = timeinfo.tm_mday;  
  }
} 