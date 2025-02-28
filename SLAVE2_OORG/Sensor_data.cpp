#include "Sensor_data.h"
#include "PIN_CFG.h"
sensor data;

int raw_wtr(sensor *data){
  static unsigned long prev = 0;
  const uint16_t interval = 2000;
  unsigned long now = millis();
  if(now - prev >= interval){
    digitalWrite(pin.VCC_WTR, HIGH);
    data->raw_value_wtr = analogRead(pin.WTR_PIN);
    digitalWrite(pin.VCC_WTR, LOW);
    prev = now;
    return data->raw_value_wtr;
  }
  //ko co gi thi khoi return 
  return -1;
}

float water_lv(sensor *data){
  static unsigned long prev = 0;
  const uint16_t interval = 2000;
  unsigned long now = millis();
  if(now - prev >= interval){
    data->water_level = (data->raw_value_wtr / 1450.0) * 100.0;
    prev = now;
  }
  return data->water_level;
}