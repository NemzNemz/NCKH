#include "FUNCTION.h"

extern HardwareSerial zigbeeSerial;
extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;
extern bool signUpOK;
extern bool  frameOn;
extern size_t frameIdx;
extern char frameBuf[64];
extern last_data_value data;

void reconnect_firebase() {
    if (!Firebase.ready() && signUpOK) {
        Serial.println("Tru to reconnect!...");
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

void send_state_to_firebase(control_status *status) {
  static unsigned long last_firebase_send = 0;
  unsigned long current_time = millis();
  
  // De tranh gui qua nhieu lan (log qua nhieu lan) dan den lag
  if (current_time - last_firebase_send < 1000) {
    return; //Thoat khoi chuong trinh
  }
  
  if (Firebase.ready() && signUpOK) {
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/LED", status->motor_status)) {
      Serial.println("Gui len Firebase!");
      status->old_firebase_status = status->motor_status;
      last_firebase_send = current_time; // Record the time of successful send
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

void readFirebaseData(peripheral *pin, control_status *status) {
    if (Firebase.ready() && signUpOK) {
        if (Firebase.RTDB.getInt(&fbdo, "Sensor/LED")) {
            status->motor_status = atoi(fbdo.stringData().c_str());
            digitalWrite(pin->LED_PIN, status->motor_status);
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

String gate_status(float ppm_sea, float ppm_river) {
    //Tranh viec tu thay doi trang thai cong vao luc khoi dau
    if (ppm_sea == 0 && ppm_river == 0) return "NONE";
    //Neu ppm bien cao hon ppm song
    if (ppm_sea > ppm_river) { 
        if (ppm_sea > 1000) {
            return "ĐÓNG";
        } else {
            return "ĐÓNG";
        }
    //Neu ppm bien thap hon ppm song
    } else { 
        if (ppm_sea > 1000) {
            return "ĐÓNG";
        } else {
            return "MỞ";
        }
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

      if (strncmp(frameBuf, "<S1,", 4) == 0) SLV1_data(frameBuf, data);
      else if (strncmp(frameBuf, "<S2,", 4) == 0) SLV2_data(frameBuf, data);
      else Serial.println("Khong nhan dien duoc slave_id");

      frameOn = false;
      frameIdx = 0;
      buffer.index = 0;
    }
  }
}

void SLV2_data(const char *s, last_data_value &data) {
  float wtr, ph, tds;
  if (sscanf(s, "<S2,WTR=%f,PH=%f,TDS=%f>", &wtr, &ph, &tds) == 3) {
    data.last_WTR_SLV2 = wtr;
    data.last_PH_SLV2  = ph;
    data.last_TDS_SLV2 = tds;
  }
}

void SLV1_data(const char *s, last_data_value &data) {
  float wtr, ph, tds;
  if (sscanf(s, "<S1,WTR=%f,PH=%f,TDS=%f>", &wtr, &ph, &tds) == 3) {
    data.last_WTR_SLV1 = wtr;
    data.last_PH_SLV1  = ph;
    data.last_TDS_SLV1 = tds;
  }
}

void handle_gate_state_and_direction_by_firebase(control_status &status, int &motor_direction, timing_variables *timing, int &previous_motor_status){
    if (status.motor_status != previous_motor_status) {
      if (status.motor_status == 1) {
          motor_direction = MOTOR_OPENING; // Quay thuận
          Serial.println("Huong dong co (Firebase): Thuan");
      } else {
          motor_direction = MOTOR_CLOSING; // Quay nghịch
          Serial.println("Huong dong co (Firebase): Nghich");
      }
      timing->motor_start_time = 0;
      previous_motor_status = status.motor_status;
  }
}

