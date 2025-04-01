  #include "FUNCTION.h"

void poll_id(uint8_t &slave_id){
  unsigned long now = millis();
  if(now - prev > interval){
    if(slave_id == 1){
      zigbeeSerial.println("SLV_01");
      Serial.println("Gui: SLV_01");
      slave_id = 2;
    }
    else if(slave_id == 2){
      zigbeeSerial.println("SLV_02");
      Serial.println("Gui: SLV_02");
      slave_id = 1;
    }
    prev = now;
  }
}
