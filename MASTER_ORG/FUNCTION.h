#ifndef FUNCTION_H
#define FUNCTION_H 

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Firebase_ESP_Client.h>
#include "MASTER_PIN_CFG.h"

extern unsigned long prev;
extern uint16_t interval;
extern peripheral pin;
extern char buffer[];
extern int  buf_index;

void poll_id(uint8_t &slave_id);
void tokenStatusCallback(token_info_t info);
void readFirebaseData();
void readDailyTaskSchedule();

#endif FUNCTION_H