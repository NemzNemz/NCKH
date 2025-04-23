#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>

typedef struct {
  float water_level;
  int raw_value_wtr;
  float ph_value;
  float real_tds_value;
  float water_temp;
} sensor;
extern sensor data;

extern OneWire* oneWire;
extern DallasTemperature* sensors;

void init_sensors();

float trung_binh_tds(uint8_t pin);
float tds_calculate(sensor *data, uint8_t pin);
int raw_wtr(sensor *data);
float water_lv(sensor *data);
float water_temp(sensor *data);
float cal_ph(uint8_t pin, sensor *data);

#endif