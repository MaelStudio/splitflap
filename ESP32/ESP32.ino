/*
COMMANDS:
> CONNECT
< IP ip

> DISPLAY msg
> MODE id

< WEATHER temp humidity
< YOUTUBE subs
< SEND msg
< MODE id
< ESP_RESET
*/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "webpage.h"

// WiFi
IPAddress staticIP(192, 168, 0, 100); // ESP32 static local IP
IPAddress gateway(192, 168, 0, 1);    // IP Address of network gateway 
IPAddress subnet(255, 255, 255, 0);   // Subnet mask
IPAddress primaryDNS(1, 1, 1, 1);     // Primary DNS
IPAddress secondaryDNS(8, 8, 8, 8);   // Secondary DNS
bool wifiOn = false;

// Web server
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Web page data
char displayed[7];
int mode = 0;

// Requests
#define REQUEST_INTERVAL (1000UL * 60 * 10) // 10 minutes

void setup() {
  Serial1.begin(115200, SERIAL_8N1, D7, D6); // RX, TX
  //Serial.begin(115200);

  // Config static IP
  WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS);

  // Setup web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/send", HTTP_GET, handleSend);
  server.addHandler(&events); // SSE for real-time webpage updates
  events.onConnect(handleEventsConnection); // When client connects to event handler

  Serial1.println(""); // Make sure commands start on a new line

  // < ESP_RESET
  Serial1.println("ESP_RESET");
}

void loop() {
  unsigned long now = millis();

  if (wifiOn && WiFi.status() != WL_CONNECTED) {
    esp_restart();
  }

  // < WEATHER temp humidity
  static unsigned long lastWeatherReqTime = now - REQUEST_INTERVAL;
  if (wifiOn && now - lastWeatherReqTime >= REQUEST_INTERVAL) {
    // GET request to url
    HTTPClient http;
    http.begin(WEATHER_API_URL);
    int res = http.GET();

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
      lastWeatherReqTime = now;
    }

    // Free resources
    http.end();
  }

  // < YOUTUBE subs
  static unsigned long lastYoutubeReqTime = now - REQUEST_INTERVAL;
  if (wifiOn && now - lastYoutubeReqTime >= REQUEST_INTERVAL) {
    // GET request to url
    HTTPClient http;
    http.begin(YOUTUBE_API_URL);
    int res = http.GET();

    if (res == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument json;
      deserializeJson(json, payload);
      int subscribers = json["items"][0]["statistics"]["subscriberCount"].as<int>();

      // < YOUTUBE subs
      Serial1.printf("YOUTUBE %d\n", subscribers);
      lastYoutubeReqTime = now;
    }

    // Free resources
    http.end();
  }
  
  // Check if received command
  if (!Serial1.available()) {
    return;
  }

  // Get command
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
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED) {
      delay(100);
    }
    wifiOn = true;

    // Start web server
    server.begin();
    
    // < IP ip
    Serial1.printf("IP %s\n", WiFi.localIP().toString().c_str());
    return;
  }

  // > DISPLAY msg
  if (command == "DISPLAY") {
    arg.toCharArray(displayed, 7);
    if (events.count()) { // check if there are clients connected to the web server
      events.send(displayed, "displayedMsg"); // Send displayed message to clients
    }
    return;
  }

  // > MODE id
  if (command == "MODE") {
    mode = arg.toInt();
    if (events.count()) { // check if there are clients connected to the web server
      events.send(String(mode).c_str(), "mode"); // Send mode to clients
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
  // Send mode over UART to Arduino
  // < MODE id
  int id = request->getParam("id")->value().toInt();
  Serial1.printf("MODE %d\n", id);
  
  request->send(200, "text/plain", "Mode changed");
}

// Send message
void handleSend(AsyncWebServerRequest *request) {
  if (!request->hasParam("msg")) {
    request->send(400, "text/plain", "Bad Request");
    return;
  }
  // Send message over UART to Arduino
  // < SEND msg
  String message = request->getParam("msg")->value();
  Serial1.printf("SEND %s\n", message.c_str());
  
  request->send(200, "text/plain", "Message received");
}