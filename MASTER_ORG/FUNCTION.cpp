#include "FUNCTION.h"

extern HardwareSerial zigbeeSerial;
extern FirebaseData fbdo;
extern bool signUpOK;
extern int ledStatus;

extern int taskHour_on;    
extern int taskMinute_on;    
extern int taskHour_off;    
extern int taskMinute_off;  

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
            digitalWrite(pin.LED_PIN, ledStatus);
        } else {
            String errorReason = String(fbdo.errorReason().c_str());
            Serial.println("Lỗi đọc từ Firebase: " + errorReason);
        }
    }
}

void readDailyTaskSchedule() {
  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Hour_on")) {
    taskHour_on = atoi(fbdo.stringData().c_str());
    Serial.print("Hour_on: ");
    Serial.println(taskHour_on);
  }

  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Minutes_on")) {
    taskMinute_on = atoi(fbdo.stringData().c_str());
    Serial.print("Minutes_on: ");
    Serial.println(taskMinute_on);
  }

  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Hour_off")) {
    taskHour_off = atoi(fbdo.stringData().c_str());
    Serial.print("Hour_off: ");
    Serial.println(taskHour_off);
  }

  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Minutes_off")) {
    taskMinute_off = atoi(fbdo.stringData().c_str());
    Serial.print("Minutes_off: ");
    Serial.println(taskMinute_off);
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