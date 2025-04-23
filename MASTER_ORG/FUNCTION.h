#ifndef FUNCTION_H
#define FUNCTION_H 

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Firebase_ESP_Client.h>
#include "CONFIG.h"

void poll_id(uint8_t &slave_id, buffer_t &buffer);
void tokenStatusCallback(token_info_t info);
void readFirebaseData(peripheral *pin, status_var *status);
void readDailyTaskSchedule(DAILY_TASK &daily_task);
void send_value_to_firebase(timing_variables *timing, last_data_value *data);
void send_state_to_firebase(status_var *status);

#endif // FUNCTION_H