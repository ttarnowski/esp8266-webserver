#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <WiFiManager.hpp>

#define SSID "myssid"
#define PASSWORD "mypass"

ESP8266WiFiMulti wifiMulti;
EventDispatcher dispatcher;
Timer timer;
ESP8266WebServer server(80);

WiFiManager wifiManager(&wifiMulti, &dispatcher, &timer, SSID, PASSWORD);

void setup() {
  Serial.begin(115200);
  delay(5000);

  server.on("/stream-index", HTTPMethod::HTTP_GET, []() {
    LittleFS.begin();

    File file = LittleFS.open("index.html", "r");

    if (!file) {
      Serial.println("could not open file for read");
      server.send(500, "application/json",
                  "{\"error\":\"could not open file\"}");
    } else {
      server.streamFile<File>(file, "text/html");
      file.close();
    }

    LittleFS.end();
  });

  server.on("/", HTTPMethod::HTTP_GET,
            []() { server.send(200, "text/plain", "OK"); });

  wifiManager.connect([](wl_status_t status) {
    if (status != WL_CONNECTED) {
      Serial.println("could not connect to wifi");
      return;
    }

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }

    server.begin();

    timer.setOnLoop([]() {
      server.handleClient();
      MDNS.update();
    });

    Serial.println("HTTP server started");
  });
}

void loop() { timer.tick(); }