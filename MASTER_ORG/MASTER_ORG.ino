#include "MASTER_PIN_CFG.h"
#include "FUNCTION.h"
#include "Adafruit_ILI9341.h"
#include "SPI.h"
#include <WiFi.h>

//Config Firebase
#define WIFI_SSID "Been"
#define WIFI_PASS "11119999"
#define API_KEY "AIzaSyCNLfTrT6w3K2ipz9DBT198YocfGbZarII"
#define DATABASE_URL "https://nckh-dd303-default-rtdb.firebaseio.com/"

//Noi luu gio, phut chay daily_task
const int taskHour = 13;    
const int taskMinute = 39;    
const char* timezone = "ICT-7"; //Time zone VN

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
int last_run_day = -1;

//Bien kiem soat data
int lastValueSLV1 = 0;          
int lastValueSLV2 = 1; 

//Khoi tao LCD
lcd_pin lcd;
Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

//Poll ID
unsigned long prev = 0;
uint16_t interval = 5000;

//Nhan Data
unsigned long prev_receive = 0;
uint16_t interval_receive = 1700;

//Gui Data         
unsigned long prev_send_data = 0;   
const uint16_t interval_send_data = 5000;    

//Debounce nut nhan
volatile unsigned long prev_debounce = 0;
const uint8_t interval_debounce = 130;

char buffer[40];
int buf_index = 0;

//Chong xung dot
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
HardwareSerial zigbeeSerial(1); // ESP32: TX=17, RX=16 (Zigbee)

void IRAM_ATTR nhan_nut(){
  unsigned long now_debounce = millis();
  portENTER_CRITICAL_ISR(&mux);

  if(now_debounce - prev_debounce > interval_debounce){
    Serial.println("Da an nut!");
    ledStatus = !ledStatus;
    digitalWrite(pin.LED_PIN, ledStatus);
    prev_debounce = now_debounce;
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
    tft.println("P.H:");
    tft.setCursor(20, 100);
    tft.println("WTR:");
    
    tft.setCursor(200, 40);
    tft.println("SLV2_DATA");
    tft.setCursor(200, 70);
    tft.println("P.H:");
    tft.setCursor(200, 100);
    tft.println("WTR:");

    tft.setCursor(20, 150);
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

void daily_task_morning();
void daily_task_night();

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

    in_text_ra_lcd();
    zigbeeSerial.begin(115200, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX);
}

void xu_ly_data(char* data, uint8_t slave_id) {
  if (strncmp(data, "SLV1: P.H: ", 11) == 0) {
    char* valStr = data + 11;
    float value = atof(valStr);
    lastValueSLV1 = value;
    //Xoa vung hien thi cu, set mau xanh la
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.fillRect(80, 70, 60, 15, ILI9341_RED);
    delay(1000);
    tft.setCursor(80, 70);
    //Han che x.000
    tft.print(value, 1);
  }

  else if (strncmp(data, "SLV2: WTR: ", 11) == 0) {
    char* valStr = data + 11;
    float value = atof(valStr);
    lastValueSLV2 = value;

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.fillRect(260, 70, 60, 15, ILI9341_RED);
    tft.setCursor(260, 70);
    //Han che x.000
    tft.print(value, 1);
  }

  else if(strncmp(data, "SLV1: WTR: ", 11) == 0){
    char* valStr = data + 11;
    float value = atof(valStr);

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.fillRect(80, 100, 60, 15, ILI9341_RED);
    delay(1000);
    tft.setCursor(80, 100);
    //Han che x.000
    tft.print(value, 1);
  }
}

void nhan_data(uint8_t slave_id) {
  if (zigbeeSerial.available()) {
    char c = zigbeeSerial.read();
    if (c == '\n') {
      buffer[buf_index] = '\0';
      
      Serial.print("Chuoi nhan duoc: ");
      Serial.println(buffer);

      unsigned long now_receive = millis();
      if (now_receive - prev_receive > interval_receive) {
        xu_ly_data(buffer, slave_id);
        prev_receive = now_receive;
      }
      buf_index = 0;
    } else {
      if (buf_index < sizeof(buffer) - 1) {
        buffer[buf_index++] = c;
      }
    }
  }
}

void send_value_to_firebase(){
  unsigned long now_send_data = millis();
    if (now_send_data - prev_send_data > interval_send_data) {
      if (Firebase.ready() && signUpOK) {
        float WTR_Value1 = lastValueSLV1;
        float WTR_Value2 = lastValueSLV2;
          if (Firebase.RTDB.setFloat(&fbdo, "Sensor/WTR_data1", WTR_Value1)) {
            Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else{
            Serial.println("Thất bại: " + fbdo.errorReason());
          }
          if(Firebase.RTDB.setFloat(&fbdo, "Sensor/WTR_data2", WTR_Value2)) {
            Serial.println();
            //Serial.println("- Thành công lưu đến: " + fbdo.dataPath());
            //Serial.println(" (" + fbdo.dataType() + ") ");
          }
          else {
            Serial.println("Thất bại: " + fbdo.errorReason());
          }
        }
    prev_send_data = now_send_data;
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

void loop() {
  static uint8_t slave_id = 1;
  poll_id(slave_id);
  //nhan_data(slave_id);
  //readFirebaseData();
  //send_value_to_firebase();

  //Chi cap nhat Firebase khi co thay doi de do lagg
  if(ledStatus != old_firebase_status){
    send_state_to_firebase();
  }

  if (ledStatus != oldLedStatus) {
  //Chi cap nhat LCD khi co thay doi de do lagg
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.fillRect(160, 150, 60, 15, ILI9341_BLACK);
  tft.setCursor(150, 150);
  if (ledStatus == 1) {
    tft.print("ON");
  } 
  else {
    tft.print("OFF");
  }
    oldLedStatus = ledStatus; 
  }
 
  digitalWrite(pin.LED_PIN, ledStatus);
  delay(50);
} 