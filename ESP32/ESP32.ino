/*
COMMANDS AND RESPONSES:
> CONNECT
< CONNECT IP

> WEATHER
< WEATHER temp humidity

> DISPLAY MSG

< SEND MSG
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "secrets.h"
#include "webpage.h"

// Web server
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Web page data
char displayed[7];
int mode = 0;

void setup() {
  //Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, D7, D6); // RX, TX

  // Setup web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/send", HTTP_GET, handleSend);
  server.addHandler(&events); // SSE for real-time webpage updates
  events.onConnect(handleEventsConnection); // When client connects to event handler
}

void loop() {
  // Check if received command
  if (!Serial1.available()) {
    return;
  }
  
  String command = Serial1.readStringUntil('\n');
  String arg;
  command.trim();
  
  // Check if command has argument
  int spaceIndex = command.indexOf(' '); // Find the index of the first space
  if (spaceIndex > -1) {
      arg = command.substring(spaceIndex + 1); // after the space
      command = command.substring(0, spaceIndex); // before the space
  }

  // > CONNECT
  if (command == "CONNECT") {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED) {
      delay(100);
    }

    // Start web server
    server.begin();
    
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

  // > DISPLAY MSG
  if (command == "DISPLAY") {
    arg.toCharArray(displayed, 7);
    if (events.count()) { // check if there are clients connected to the web server
      events.send(displayed, "displayedMsg"); // Send displayed message to clients
    }
    return;
  }
}

// Send webpage
void handleRoot(AsyncWebServerRequest *request) {
  request->send(200, "text/html", HTML);
}

// On webpage load, client connects to events handler
void handleEventsConnection(AsyncEventSourceClient *client) {
  // Send current displayed message and mode
  client->send(displayed, "displayedMsg");
  client->send(String(mode).c_str(), "mode");
}

// Change mode
void handleMode(AsyncWebServerRequest *request) {
  if (!request->hasParam("id")) {
    request->send(400, "text/plain", "Bad Request");
    return;
  }
  mode = request->getParam("id")->value().toInt();
  
  //Serial.print("Mode set to "); Serial.println(mode);
  request->send(200, "text/plain", "Mode changed");
}

// Send message
void handleSend(AsyncWebServerRequest *request) {
  if (!request->hasParam("msg")) {
    request->send(400, "text/plain", "Bad Request");
    return;
  }
  // Send message over UART to Arduino
  String message = request->getParam("msg")->value();
  Serial1.printf("SEND %s\n", message.c_str());
  
  request->send(200, "text/plain", "Message received");
}