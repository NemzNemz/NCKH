#include "CONFIG.h"
#include "FUNCTION.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include <WiFi.h>

// Config Firebase
#define WIFI_SSID "Been"
#define WIFI_PASS "11119999"
#define API_KEY "AIzaSyCNLfTrT6w3K2ipz9DBT198YocfGbZarII"
#define DATABASE_URL "https://nckh-dd303-default-rtdb.firebaseio.com/"

bool frameOn = false;     // đang trong giai đoạn ghi frame
size_t frameIdx = 0;      // chỉ số write buffer
char frameBuf[64];        // lưu khung "<Sx…>\n"
buffer_t buffer = {{0}, 0};

// Noi luu gio, phut chay daily_task
DAILY_TASK daily_task = {20, 10, 20, 11}; // Hour_on: 20:10, Hour_off: 20:11
const char* timezone = "ICT-7"; // Time zone VN
time_t now;
struct tm timeinfo;

// Bien sync NTP
unsigned long lastNTPUpdate = 0;
const unsigned long ntpSyncInterval = 30 * 60 * 1000; // 30 phut 1 lan

// Cac bien danh cho Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signUpOK = false;

// Chi tiet bien kiem soat gia tri lan truoc
last_data_value data = {
  .last_WTR_SLV1 = 0.0,  
  .last_WTR_SLV2 = 0.0,
  .last_PH_SLV1 = 0.0,
  .last_PH_SLV2 = 0.0,
  .last_TDS_SLV1 = 0.0,
  .last_TDS_SLV2 = 0.0
};
// lan luot la thoi gian doc firebase, thoi gian gui data len firebase
timing_variables timing = {
  .prev_send_fb = 0,
  .interval_send_fb = 5000,         
  .prev_send_data = 0,
  .interval_send_data = 5000
};
// lan luot la led, old_led_status, old_firebase_status, bien trang thai da chay ngay do hay chua
status_var status = {
  .motor_status = 0,
  .old_motor_status = -1, 
  .old_firebase_status = 1,
  .last_run_day_on = -1,
  .last_run_day_off = -1
};
// Cau hinh chi tiet pwm cho motor
pwm_propeties motor_cfg = {
  .frequency = 5000,    // Tan so 5kHz
  .pwm_channel = 0,     // Kenh PWM 0
  .resolution = 8,      // 8 bit phan giai
  .duty_cycle = 180     // ~70% tốc độ
};
peripheral pin;

// Khoi tao LCD
lcd_pin lcd = {0, 2, 14, 12, 13};
Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

// Chong xung dot
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
HardwareSerial zigbeeSerial(1); 

volatile bool buttonPressed = false;
static unsigned long startTime = 0;      // Mốc thời gian bắt đầu quay
const unsigned long runDuration = 3000;  // Thời gian chạy motor (3 giây)
static int motorDirection = 0;           // 0: dừng, 1: quay thuận, 2: quay nghịch
static int previousStatus = 0;           // Trạng thái motor trước đó để kiểm tra thay đổi

// Add these variables to the global declaration section
String gateState = ""; // Current gate state
String lastGateState = ""; // Store the previous gate state
bool commandExecuted = true; // Flag to indicate if the command has been executed

// Hàm xác định trạng thái cống dựa trên PPM
String determineGateState(float ppm_sea, float ppm_river) {
    if (ppm_sea > ppm_river) { // Nước Biển > Sông
        if (ppm_sea > 1000) {
            return "ĐÓNG";
        } else {
            return "ĐÓNG";
        }
    } else { // Nước Biển < Sông
        if (ppm_sea > 1000) {
            return "KIỂM SOÁT (ĐÓNG)";
        } else {
            return "MỞ";
        }
    }
}

void IRAM_ATTR nhan_nut() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = micros();
  if (interruptTime - lastInterruptTime > 100000) { // debounce tinh bang micros
    buttonPressed = true;
    lastInterruptTime = interruptTime;
  }
}

void test_dc_quay_thuan() {
  // Bật motor quay thuận
  digitalWrite(pin.MOTOR_PIN1, HIGH);
  digitalWrite(pin.MOTOR_PIN2, LOW);
  ledcWrite(motor_cfg.pwm_channel, motor_cfg.duty_cycle);
  Serial.println("Thuan");
}

void test_dc_quay_nghich() {
  // Bật motor quay nghịch
  digitalWrite(pin.MOTOR_PIN1, LOW);
  digitalWrite(pin.MOTOR_PIN2, HIGH);
  ledcWrite(motor_cfg.pwm_channel, motor_cfg.duty_cycle);
  Serial.println("Nghich");
}

void test_dc_ngung() {
  // Tắt motor
  digitalWrite(pin.MOTOR_PIN1, LOW);
  digitalWrite(pin.MOTOR_PIN2, LOW);
  ledcWrite(motor_cfg.pwm_channel, 0);
  Serial.println("Ngung");
}

void in_text_ra_lcd() {
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

void daily_task_test_on() {
  motorDirection = 1; // Quay thuận
  startTime = 0; // Reset thời gian để chạy lại
  status.motor_status = 1; // Cập nhật trạng thái motor
  digitalWrite(pin.LED_PIN, status.motor_status);
  send_state_to_firebase(&status);
}

void daily_task_test_off() {
  motorDirection = 2; // Quay nghịch
  startTime = 0; // Reset thời gian để chạy lại
  status.motor_status = 0; // Cập nhật trạng thái motor
  digitalWrite(pin.LED_PIN, status.motor_status);
  send_state_to_firebase(&status);
}

void setup() {
  Serial.begin(115200);
  peripheral_init(&pin);
  pinMode(pin.LED_PIN, OUTPUT);
  pinMode(pin.BUTTON_PIN, INPUT_PULLUP);
  pinMode(pin.MOTOR_PIN1, OUTPUT);
  pinMode(pin.MOTOR_PIN2, OUTPUT);
  ledcSetup(motor_cfg.pwm_channel, motor_cfg.frequency, motor_cfg.resolution);
  ledcAttachPin(pin.ENABLE, motor_cfg.pwm_channel);
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
  sync_time();

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
  readDailyTaskSchedule(daily_task);

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
  float wtr, ph, tds;
  if (sscanf(s, "<S1,WTR=%f,PH=%f,TDS=%f>", &wtr, &ph, &tds) == 3) {
    data.last_WTR_SLV1 = wtr;
    data.last_PH_SLV1  = ph;
    data.last_TDS_SLV1 = tds;

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.fillRect(80, 70, 60, 15, ILI9341_BLACK);
    tft.setCursor(80, 70);  
    tft.print(tds, 1);

    tft.fillRect(80, 100, 60, 15, ILI9341_BLACK);
    tft.setCursor(80,100);  
    tft.print(ph,  1);

    tft.fillRect(80, 130, 60, 15, ILI9341_BLACK);
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
      Serial.println(frameBuf);

      if (strncmp(frameBuf, "<S1,", 4) == 0) processSLV1Data(frameBuf);
      else if (strncmp(frameBuf, "<S2,", 4) == 0) processSLV2Data(frameBuf);
      else Serial.println("Khong nhan dien duoc slave_id");

      frameOn = false;
      frameIdx = 0;
      buffer.index = 0;
    }
  }
}

void lcd_change_motor_status() {
  if (status.motor_status != status.old_motor_status) {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.fillRect(170, 160, 60, 15, ILI9341_BLACK);
    tft.setCursor(170, 160);
    if (status.motor_status == 1) {
      tft.print("OPEN");
    } else {
      tft.print("CLOSED");
    }
    status.old_motor_status = status.motor_status; 
  }
}

void loop() {
  static uint8_t slave_id = 1;
  poll_id(slave_id, buffer);
  nhan_data(buffer);
  send_value_to_firebase(&timing, &data);
  now = time(nullptr);
  localtime_r(&now, &timeinfo);
  // Update the current gate state based on the latest sensor readings
  gateState = determineGateState(data.last_TDS_SLV1, data.last_TDS_SLV2);

  // Đọc dữ liệu từ Firebase mỗi 5 giây
  unsigned long nowww = millis();
  if (nowww - timing.prev_send_fb > timing.interval_send_fb) {
    readFirebaseData(&pin, &status);
    timing.prev_send_fb = nowww;
  }

   // Chỉ thực hiện khi trạng thái thay đổi và lệnh chưa được thực hiện
  if (gateState != lastGateState && commandExecuted == true) {
    Serial.println("Trạng thái cống thay đổi từ: " + lastGateState + " sang: " + gateState);
    lastGateState = gateState; // Cập nhật trạng thái cống
    commandExecuted = false; // Đánh dấu lệnh chưa được thực hiện
    
    // Cập nhật trạng thái motor theo trạng thái cổng mới
    if (gateState == "MỞ" && status.motor_status != 1) {
      status.motor_status = 1; // Bật motor để mở cống
      motorDirection = 1;      // Quay thuận để mở
      startTime = 0;           // Reset thời gian
    } else if ((gateState == "ĐÓNG" || gateState == "KIỂM SOÁT (ĐÓNG)") && status.motor_status != 0) {
      status.motor_status = 0; // Tắt motor để đóng cống
      motorDirection = 2;      // Quay nghịch để đóng
      startTime = 0;           // Reset thời gian
    } else {
      // Nếu không cần thay đổi trạng thái motor, đánh dấu đã thực hiện lệnh
      commandExecuted = true;
    }
  }

  // Đặt lệnh đã thực hiện sau khi motor hoàn thành
  if (motorDirection == 0) {
    commandExecuted = true;
  }

  // Kiểm tra thay đổi trạng thái motor từ Firebase
  if (status.motor_status != previousStatus) {
    motorDirection = (status.motor_status == 1) ? 1 : 2;
    startTime = 0;
    Serial.print("Huong dong co (Firebase): ");
    if (motorDirection == 1) Serial.println("Thuan");
    else Serial.println("Nghich");
    previousStatus = status.motor_status;
  }

  // Xử lý nhấn nút
  if (buttonPressed) {
    buttonPressed = false;
    status.motor_status = !status.motor_status;
    digitalWrite(pin.LED_PIN, status.motor_status);
    motorDirection = (status.motor_status == 1) ? 1 : 2;
    startTime = 0;
    Serial.print("Huong dong co (Nut nhan): ");
    if (motorDirection == 1) Serial.println("Thuan");
    else Serial.println("Nghich");
    previousStatus = status.motor_status;
  }

  // Điều khiển động cơ
  if (startTime == 0 && motorDirection != 0) {
    if (motorDirection == 1) {
      test_dc_quay_thuan();
    } else if (motorDirection == 2) {
      test_dc_quay_nghich();
    }
    startTime = millis();
  } else if (startTime != 0 && (millis() - startTime >= runDuration)) {
    test_dc_ngung();
    startTime = 0;
    motorDirection = 0;
  }

  // Kiểm tra và thực hiện daily task
  if (timeinfo.tm_hour == daily_task.taskHour_on && timeinfo.tm_min == daily_task.taskMinute_on && status.last_run_day_on != timeinfo.tm_mday) {
    daily_task_test_on();
    lcd_change_motor_status();
    status.last_run_day_on = timeinfo.tm_mday;
    previousStatus = status.motor_status;
  }

  if (timeinfo.tm_hour == daily_task.taskHour_off && timeinfo.tm_min == daily_task.taskMinute_off && status.last_run_day_off != timeinfo.tm_mday) {
    daily_task_test_off();
    lcd_change_motor_status();
    status.last_run_day_off = timeinfo.tm_mday;
    previousStatus = status.motor_status;
  }

  // Cập nhật trạng thái trên Firebase và LCD
  if (status.motor_status != status.old_firebase_status) {
    send_state_to_firebase(&status);
    lcd_change_motor_status();
    status.old_firebase_status = status.motor_status;
  }
}