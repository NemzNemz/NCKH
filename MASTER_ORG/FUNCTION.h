#ifndef FUNCTION_H
#define FUNCTION_H 

#include <Arduino.h>
#include <HardwareSerial.h>

extern unsigned long prev;
extern uint16_t interval;
extern uint8_t slave_id;
extern HardwareSerial zigbeeSerial;

void poll_id();
void firebase();

#endif FUNCTION_H