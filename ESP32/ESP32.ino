/*
COMMANDS AND RESPONSES:
> CONNECT
< CONNECT IP

> WEATHER
< WEATHER temp humidity
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include "secrets.h"

void setup() {
  Serial1.begin(115200, SERIAL_8N1, D7, D6); // RX, TX
}

void loop() {
  // Check if received command
  if (!Serial1.available()) {
    return;
  }
  String command = Serial1.readStringUntil('\n');
  command.trim();

  // > CONNECT
  if (command == "CONNECT") {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED) {
      delay(100);
    }
    // < CONNECT IP
    Serial1.printf("CONNECT %s\n", WiFi.localIP().toString().c_str());

    return;
  }

  // > WEATHER
  if (command == "WEATHER") {
    HTTPClient http;

    // GET request to url
    http.begin(WEATHER_API_URL);
    int res = http.GET();
    
    // res will be negative on error
    if (res == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument json;
      deserializeJson(json, payload);

      // Extract values from JSON
      float temperature = json["current"]["temperature_2m"];
      int humidity = json["current"]["relative_humidity_2m"];
      //float max_temp = json["daily"]["temperature_2m_max"][0];
      //float min_temp = json["daily"]["temperature_2m_min"][0];

      // < WEATHER temp humidity
      Serial1.printf("WEATHER %d %d\n", (int)round(temperature), (int)round(humidity));
    }
      
    // Free resources
    http.end();

    return;
  }
}