#include "FUNCTION.h"

extern HardwareSerial zigbeeSerial;
extern FirebaseData fbdo;
extern bool signUpOK;
extern int ledStatus;
extern const int LED_PIN;

void tokenStatusCallback(token_info_t info) {
    if (info.status == token_status_error) {
        String errorMessage = String(info.error.message.c_str());
        Serial.println("Lỗi token: " + errorMessage);
    }
}

void readFirebaseData() {
    if (Firebase.ready() && signUpOK) {
        if (Firebase.RTDB.getInt(&fbdo, "Sensor/LED")) {
            ledStatus = atoi(fbdo.stringData().c_str());
            digitalWrite(LED_PIN, ledStatus);
            //Serial.println(ledStatus ? "LED BẬT!" : "LED TẮT!");
        } else {
            String errorReason = String(fbdo.errorReason().c_str());
            Serial.println("Lỗi đọc từ Firebase: " + errorReason);
        }
    }
}

void poll_id(uint8_t &slave_id) {
  unsigned long now = millis();
  if(now - prev > interval) {
    if(slave_id == 1) {
      zigbeeSerial.println("SLV_01");
      Serial.println("Gui: SLV_01");
      slave_id = 2;
    } else if(slave_id == 2) {
      zigbeeSerial.println("SLV_02");
      Serial.println("Gui: SLV_02");
      slave_id = 1;
    }
    prev = now;
  }
}