#include "FUNCTION.h"

extern HardwareSerial zigbeeSerial;
extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;
extern bool signUpOK;

void reconnect_firebase() {
    if (!Firebase.ready() && signUpOK) {
        Serial.println("Thử kết nối lại Firebase...");
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
    }
}

bool send_firebase_float(FirebaseData *fbdo, const char *path, float value) {
    if (Firebase.RTDB.setFloat(fbdo, path, value)) {
        return true;
    } else {
        Serial.println("Thất bại khi gửi " + String(path) + ": " + fbdo->errorReason());
        return false;
    }
}

void send_value_to_firebase(timing_variables *timing, last_data_value *data) {
    unsigned long now_send_data = millis();
    if (now_send_data - timing->prev_send_data > timing->interval_send_data) {
        if (Firebase.ready() && signUpOK) {
            send_firebase_float(&fbdo, "Sensor/WTR_data1", data->last_WTR_SLV1);
            send_firebase_float(&fbdo, "Sensor/PH_data1", data->last_PH_SLV1);
            send_firebase_float(&fbdo, "Sensor/TDS_data1", data->last_TDS_SLV1);
            
            send_firebase_float(&fbdo, "Sensor//WTR_data2", data->last_WTR_SLV2);
            send_firebase_float(&fbdo, "Sensor//TDS_data2", data->last_TDS_SLV2);
            send_firebase_float(&fbdo, "Sensor//PH_data2", data->last_PH_SLV2);
            timing->prev_send_data = now_send_data;
        }
        if (!Firebase.ready() && signUpOK) {
            reconnect_firebase();
        }
    }
}

void send_state_to_firebase(status_var *status) {
    if (Firebase.ready() && signUpOK) {
        if (Firebase.RTDB.setInt(&fbdo, "Sensor/LED", status->ledStatus)) {
            Serial.println("Gui trang thai len Firebase thanh cong");
            status->old_firebase_status = status->ledStatus;
        } else {
            Serial.println("Gui firebase that bai: " + String(fbdo.errorReason().c_str()));
        }
    }
}

void tokenStatusCallback(token_info_t info) {
    if (info.status == token_status_error) {
        String errorMessage = String(info.error.message.c_str());
        Serial.println("Lỗi token: " + errorMessage);
    }
}

void readFirebaseData(peripheral *pin, status_var *status) {
    if (Firebase.ready() && signUpOK) {
        if (Firebase.RTDB.getInt(&fbdo, "Sensor/LED")) {
            status->ledStatus = atoi(fbdo.stringData().c_str());
            digitalWrite(pin->LED_PIN, status->ledStatus);
        } else {
            String errorReason = String(fbdo.errorReason().c_str());
            Serial.println("Lỗi đọc từ Firebase: " + errorReason);
        }
    }
}

void readDailyTaskSchedule(DAILY_TASK &daily_task) {
  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Hour_on")) {
    daily_task.taskHour_on = atoi(fbdo.stringData().c_str());
    Serial.print("Hour_on: ");
    Serial.println(daily_task.taskHour_on);
  }

  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Minutes_on")) {
    daily_task.taskMinute_on = atoi(fbdo.stringData().c_str());
    Serial.print("Minutes_on: ");
    Serial.println(daily_task.taskMinute_on);
  }

  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Hour_off")) {
    daily_task.taskHour_off = atoi(fbdo.stringData().c_str());
    Serial.print("Hour_off: ");
    Serial.println(daily_task.taskHour_off);
  }

  if (Firebase.RTDB.getString(&fbdo, "Daily_Task/Minutes_off")) {
    daily_task.taskMinute_off = atoi(fbdo.stringData().c_str());
    Serial.print("Minutes_off: ");
    Serial.println(daily_task.taskMinute_off);
  }
}

void poll_id(uint8_t &slave_id, buffer_t &buffer) {
    static unsigned long prev = 0;
    static uint16_t interval = 5000;
    unsigned long now = millis();

    if (now - prev > interval) {
        if (slave_id == 1) {
            zigbeeSerial.print("SLV_01\n");
            //Serial.println("Gui: SLV_01");
            slave_id = 2;
        } else if (slave_id == 2) {
            zigbeeSerial.print("SLV_02\n");
            //Serial.println("Gui: SLV_02");
            slave_id = 1;
        }
        prev = now;
        buffer.index = 0;
        buffer.data[0] = '\0';
    }
}

