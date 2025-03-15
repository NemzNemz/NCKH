#include "FUNCTION.h"

void poll_id(){
  unsigned long now = millis();
  if(now - prev > interval){
    if(slave_id == 1){
      zigbeeSerial.println("SLV_01");
      Serial.println("Gui: SLV_01");

      if(zigbeeSerial.available()){
        String data = zigbeeSerial.readStringUntil('\n');
        Serial.println("Nhan: "+ data);
      }
      slave_id = 2;
    }
    else if(slave_id == 2){
      zigbeeSerial.println("SLV_02");
      Serial.println("Gui: SLV_02");

      if(zigbeeSerial.available()){
        String data = zigbeeSerial.readStringUntil('\n');
        Serial.println("Nhan: "+ data);
      }
      slave_id = 1;
    }
    prev = now;
  }
}

void firebase(){
  
}