#include "Sensor_data.h"
#include "PIN_CFG.h"
sensor data;

float cal_ph(uint8_t pin, sensor *data){
  float adc_raw = analogRead(pin);
  float voltage = (adc_raw*3.3)/4095.0;
  
  const float V7 = 2.50;
  const float V10 = 2.94;
  float slope = (V7 - V10) / (7.0 - 10.0);

  float ph = 7.0 +((voltage - V7) / slope);
  data->ph_value = ph;
  return data->ph_value;
}

int raw_wtr(sensor *data){
  static unsigned long prev = 0;
  const uint16_t interval = 20;
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