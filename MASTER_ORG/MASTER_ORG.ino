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

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signUpOK = false;

extern const int LED_PIN = 5;
const int BUTTON_PIN = 32; 
bool lastButtonState = HIGH; 

//Cac bien trang thai
int ledStatus = 0;
int oldLedStatus = -1;  
int old_firebase_status = -1;

lcd_pin lcd;
Adafruit_ILI9341 tft = Adafruit_ILI9341(lcd.TFT_CS, lcd.TFT_DC, lcd.TFT_MOSI, lcd.TFT_SCLK, lcd.TFT_RST);

//Poll ID
unsigned long prev = 0;
uint16_t interval = 2000;

//Nhan Data
unsigned long prev2 = 0;
uint16_t interval2 = 1700;

//Gui Data
int lastValueSLV1 = 0;          
int lastValueSLV2 = 1;          
unsigned long prev_send_data = 0;   
const uint16_t interval3 = 2000;    

char buffer[40];
int buf_index = 0;

bool state_of_led = false;
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

    tft.setCursor(20, 150);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.println("LED_STATE:");
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, 0);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("Dang ket noi voi Wifi");
    while(WiFi.status() != WL_CONNECTED){
      Serial.println(".");
      delay(400);
    }
    Serial.println("DA KET NOI");
    Serial.println(WiFi.localIP());
    Serial.println();

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

    peripheral_init(&pin);
    in_text_ra_lcd();
    zigbeeSerial.begin(115200, SERIAL_8N1, pin.ZIGBEE_RX, pin.ZIGBEE_TX);
}

void xu_ly_data(char* data, uint8_t slave_id) {
  if (strncmp(data, "SLV1: WTR: ", 11) == 0) {
    char* valStr = data + 11;
    float value = atof(valStr);
    lastValueSLV1 = value;
    //Xoa vung hien thi cu, set mau xanh la
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.fillRect(130, 35, 50, 15, ILI9341_BLACK);
    tft.setCursor(130, 35);
    //Han che x.000
    tft.print(value, 0);
    }
   else if (strncmp(data, "SLV2: WTR: ", 11) == 0) {
    char* valStr = data + 11;
    float value = atof(valStr);
    lastValueSLV2 = value;

    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.fillRect(130, 55, 50, 15, ILI9341_BLACK);
    tft.setCursor(130, 55);
    //Han che x.000
    tft.print(value, 0);
  }
}

void check_button(){
  bool cur_state = digitalRead(BUTTON_PIN);
  if(cur_state == LOW && lastButtonState == HIGH){
    ledStatus = !ledStatus; 
    digitalWrite(LED_PIN, ledStatus);
    Serial.println("Da an nut!");
  }
  lastButtonState = cur_state;
}

void nhan_data(uint8_t slave_id) {
  if (zigbeeSerial.available()) {
    char c = zigbeeSerial.read();
    if (c == '\n') {
      buffer[buf_index] = '\0';
      unsigned long now2 = millis();
      if (now2 - prev2 > interval2) {
        xu_ly_data(buffer, slave_id);
        prev2 = now2;
      }
      buf_index = 0;
    } else {
      if (buf_index < sizeof(buffer) - 1) {
        buffer[buf_index++] = c;
      }
    }
  }
}

void send_to_firebase(){
  unsigned long now3 = millis();
    if (now3 - prev_send_data > interval3) {
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
    prev_send_data = now3;
  }
}

void loop() {
  static uint8_t slave_id = 1;
  poll_id(slave_id);
  nhan_data(slave_id);
  readFirebaseData();
  send_to_firebase();
  check_button();

  //Chi cap nhat Firebase khi co thay doi de do lagg
  if(ledStatus != old_firebase_status){
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
 
  digitalWrite(LED_PIN, ledStatus);
  delay(50);
} 