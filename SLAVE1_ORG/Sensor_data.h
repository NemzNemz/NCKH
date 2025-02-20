#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <Arduino.h>

typedef struct Sensor_Data{
  float water_level;
  int raw_value_wtr;
  //Se con nhieu cam bien hon nua (3~5)
} sensor;

//bien toan cuc nhieu file
extern sensor data;

int raw_wtr(sensor *data);
float water_lv(sensor *data);

#endif