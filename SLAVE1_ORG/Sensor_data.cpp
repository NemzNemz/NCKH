#include "Sensor_data.h"
#include "PIN_CFG.h"

sensor data;
const uint8_t samples = 30;

OneWire* oneWire = nullptr;
DallasTemperature* sensors = nullptr;

void init_sensors() {
  if (oneWire == nullptr) {
    oneWire = new OneWire(pin.DS18B20_PIN);
    sensors = new DallasTemperature(oneWire);
    sensors->begin();
  }
}

float water_temp(sensor *data) {
  sensors->requestTemperatures();
  return data->water_temp = sensors->getTempCByIndex(0);
}

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

float trung_binh_tds(uint8_t pin){
  float raw_tds = 0;
  for(int i = 0; i < samples; i++){
    raw_tds += analogRead(pin);
    delay(2);
  }
  return raw_tds/ (float)samples;
}

float tds_calculate(sensor *data, uint8_t pin){
  float adc_avg = trung_binh_tds(pin);
  float voltage = adc_avg *3.3/ 4095.0;
  float ec_value = 133.42 * voltage * voltage * voltage
                  - 255.86 * voltage * voltage
                  + 857.39 * voltage;
  float tds_raw = ec_value * 0.5;
  float bu_nhiet = 1.0 + 0.02 * (data->water_temp - 25.0);
  return data->real_tds_value = tds_raw/ bu_nhiet;
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