#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

AsyncWebServer server(80);

struct wifi_settings_t {
  char ssid[32];
  char pass[32];
} wifi; 

struct settings_t {
  char time_server[32];
  int16_t time_zone;
  int16_t time_dst;   
} settings; 

void setup() {
  Serial.begin(74880);

  //set a nice hostname
  String new_hostname = "big_seven_" + String(ESP.getChipId(), HEX);
  WiFi.hostname(new_hostname);

  //open file system
  if (SPIFFS.begin())
    Serial.println(F("File system opened, generating file list"));
  
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("\tFS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  Serial.printf("\n");

  //load settings
  if (SPIFFS.exists("/settings.dat")) {
      read_file((char *)"/settings.dat", (byte *)&settings, sizeof(struct settings_t));
      Serial.printf("Time server: %s\n", settings.time_server);
      Serial.printf("Time zone: %d\n", settings.time_zone);
      Serial.printf("Time DST: %d\n", settings.time_dst);
  }
  
  //WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  
  //check to see if we have config file
  Serial.println(F("Checking to see if we have new config..."));
  if (SPIFFS.exists("/wifi.dat")) {
    //read wifi settings from file
    read_file((char *)"/wifi.dat", (byte *)&wifi, sizeof(struct wifi_settings_t));
    Serial.printf("Found config trying to connect to ssid %s\n", wifi.ssid);
    WiFi.begin(wifi.ssid, wifi.pass);
    //remove config
    SPIFFS.remove("/wifi.dat");
  } else {
    //try to connect to previous network if any else start in AP mode
    Serial.println(F("Connecting to existing network..."));
    WiFi.begin();
  }

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.printf("Successfully connected to ssid: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP addess: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("Configuring access point...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();
    if (WiFi.softAP(new_hostname.c_str())) {
      Serial.printf("Successfully created access point: %s\n", new_hostname.c_str());
      Serial.printf("IP addess: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
      Serial.println(F("Something failed"));
      while (1);
    }
  }

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request ) {
    request->redirect("/");
    ESP.restart();
  });
  server.on("/wifi", HTTP_GET,  handle_wifi);
  server.on("/time", HTTP_GET,  handle_time);
  server.on("/overview", HTTP_GET,  handle_overview);
  server.on("/wifi", HTTP_POST,  handle_wifi_save);
  server.on("/time", HTTP_POST,  handle_time_save);
  server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age:600");

  server.begin();
  Serial.println(F("HTTP server started on port 80"));
  Serial.println(F("Starting network scanning async"));
  WiFi.scanNetworks(1);
}

void loop() {
}
