#ifndef FUNCTION_H
#define FUNCTION_H 

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Firebase_ESP_Client.h>

extern unsigned long prev;
extern uint16_t interval;

void poll_id(uint8_t &slave_id);
void tokenStatusCallback(token_info_t info);
void readFirebaseData();

#endif FUNCTION_H