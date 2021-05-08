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

  server.on("/", HTTPMethod::HTTP_POST, []() {
    Serial.printf("count body param: %s\n", server.arg("count").c_str());

    char cookie[64];
    server.header("Cookie").toCharArray(cookie, sizeof(cookie) - 1);

    Serial.printf("X-Test: %s\n", server.header("X-Test").c_str());
    Serial.printf("Content-Type: %s\n", server.header("Content-Type").c_str());

    char body[128];
    server.arg("plain").toCharArray(body, sizeof(body) - 1);
    Serial.printf("Body: %s\n", body);

    char respBody[100];
    sprintf(respBody, "Cookie Header: %s", cookie);

    server.sendHeader("X-User", "123");
    server.send(200, "text/plain", respBody);
  });

  server.onNotFound(
      []() { server.send(404, "text/plain", "This is not found"); });

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

    const char *headerKeys[] = {"Cookie", "X-Test", "Content-Type"};
    size_t headerKeysSize = sizeof(headerKeys) / sizeof(char *);
    server.collectHeaders(headerKeys, headerKeysSize);
    server.begin();

    timer.setOnLoop([]() {
      server.handleClient();
      MDNS.update();
    });

    Serial.println("HTTP server started");
  });
}

void loop() { timer.tick(); }