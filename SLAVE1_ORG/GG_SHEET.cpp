#include "GG_SHEET.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "Sensor_data.h"

extern sensor data;

const char* ssid = "Trieu Ninh";
const char* password = "12344321";
String googleScriptUrl = "https://script.google.com/macros/s/AKfycbzH_dHcqu70IVUvkaOhfx5NeWxMAMupB5VZIlk7CL-ko0Q6QkMQv0Exfls3hGosaAec/exec"; // Dán URL đã sao chép ở đây

void sendDataToGoogleSheets() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = googleScriptUrl + "?raw=" + String(data.raw_value_wtr, 2) + "&water=" + String(data.water_level, 2);

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode > 0) {
            Serial.println("Data Sent!");
        } else {
            Serial.println("Error Sending Data");
        }

        http.end();
    }
}

