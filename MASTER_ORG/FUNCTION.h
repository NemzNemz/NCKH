#ifndef FUNCTION_H
#define FUNCTION_H 

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Firebase_ESP_Client.h>
#include "CONFIG.h"

void poll_id(uint8_t &slave_id, buffer_t &buffer);
void tokenStatusCallback(token_info_t info);
void readFirebaseData(peripheral *pin, control_status *status);
void readDailyTaskSchedule(DAILY_TASK &daily_task);
void send_value_to_firebase(timing_variables *timing, last_data_value *data);
void send_state_to_firebase(control_status *status);
String gate_status(float ppm_sea, float ppm_river);
void nhan_data(buffer_t &buffer);
void SLV1_data(const char *s, last_data_value &data);
void SLV2_data(const char *s, last_data_value &data);
void handle_gate_state_and_direction_by_firebase(control_status &status, int &motor_direction, timing_variables *timing, int &previous_motor_status);

#endif // FUNCTION_H