#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

//Danh rieng cho LCD
typedef struct {
    uint8_t TFT_CS;
    uint8_t TFT_DC;
    uint8_t TFT_MOSI;
    uint8_t TFT_SCLK;
    uint8_t TFT_RST;
} lcd_pin;

//Cac chan ngoai vi
typedef struct {
    uint8_t ZIGBEE_RX;
    uint8_t ZIGBEE_TX;
    uint8_t LED_PIN;
    uint8_t BUTTON_PIN;
    uint8_t MOTOR_PIN1;
    uint8_t MOTOR_PIN2;
    uint8_t ENABLE;
} peripheral;

//cac bien thoi gian lien quan den gui nhan firebase
typedef struct{
  unsigned long prev_send_fb;   
  uint16_t interval_send_fb;         
  unsigned long prev_send_data;   
  uint16_t interval_send_data;  
  unsigned long motor_start_time;
  uint16_t motor_run_duration;
}timing_variables;

//Cac bien cau hinh pwm
typedef struct{
  const uint16_t frequency;
  uint8_t pwm_channel;
  uint8_t resolution;
  uint8_t duty_cycle;
}pwm_propeties;

//Bien kiem soat gia tri lan truoc co trung voi lan sau ko, neu trung thi ko update
typedef struct {
  float last_WTR_SLV1;          
  float last_WTR_SLV2; 
  float last_PH_SLV1;
  float last_PH_SLV2;
  float last_TDS_SLV1;
  float last_TDS_SLV2 ;
}last_data_value;

//nhu ten struct, danh cho daily task
typedef struct{
  int taskHour_on;    
  int taskMinute_on;    
  int taskHour_off;    
  int taskMinute_off;
}DAILY_TASK; 

//Trang thai motor, trang thai firebase, trang thai hoat dong cua daily task
typedef struct{
  volatile int motor_status;   
  uint8_t old_motor_status;           
  uint8_t old_firebase_status;
  uint8_t last_run_day_on;
  uint8_t last_run_day_off;
}control_status;
extern control_status status;

typedef struct {
    char data[40];
    int index;
} buffer_t;
  
void peripheral_init(peripheral *pin);

#endif //CONFIG_H