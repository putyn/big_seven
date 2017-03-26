#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Ticker.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

//data types & protoypes
struct dtime_t {
  uint8_t mseconds;
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
} local_time;

struct wifi_settings_t {
  char ssid[32];
  char pass[32];
} wifi;

struct settings_t {
  char time_server[32];
  char hostname[32];
  int16_t time_zone;
  int16_t time_dst;
  boolean update_display;
  boolean update_time;
  boolean reboot;
  boolean online;
  boolean soft_ap;
} settings;

dtime_t ntp_get_time();
void ntp_send_request(IPAddress& address);
void update_displays(dtime_t t);
void tock();

//defines and variables
#define CSN_PIN 16
#define OE_PIN 2
#define TICK_PIN 12

//web server, dns server, ticker, udp client
AsyncWebServer server(80);
DNSServer dns_server;
Ticker ticks;
WiFiUDP ntp;


void setup() {

  //for debug reasons
  Serial.begin(74880);

  //open file system
  if (SPIFFS.begin())
    Serial.println(F("[FS]File system opened, generating file list"));
    
  //file list, also for debug
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("\tFS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }

  //load settings, not all settings are saved to flash
  if (SPIFFS.exists("/settings.dat")) {
    read_file((char *)"/settings.dat", (byte *)&settings, sizeof(struct settings_t));
    Serial.printf("[FS]Time server: %s\n", settings.time_server);
    Serial.printf("[FS]Time zone: %d\n", settings.time_zone);
    Serial.printf("[FS]Time DST: %d\n", settings.time_dst);
  }
  //settings that are not saved
  settings.reboot = false;
  settings.soft_ap = false;
  settings.update_time = false;
  settings.update_display = false;

  //set a nice hostname
  sprintf(settings.hostname, "big_seven_%06X", ESP.getChipId());
  WiFi.hostname(settings.hostname);

  //WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  //check to see if we have config file
  Serial.println(F("[WiFi] Checking to see if we have new config..."));
  if (SPIFFS.exists("/wifi.dat")) {
    //read wifi settings from file
    read_file((char *)"/wifi.dat", (byte *)&wifi, sizeof(struct wifi_settings_t));
    Serial.printf("[WiFi] Found config trying to connect to ssid %s\n", wifi.ssid);
    WiFi.begin(wifi.ssid, wifi.pass);
    //remove config
    SPIFFS.remove("/wifi.dat");
  } else {
    //try to connect to previous network if any else start in AP mode
    Serial.println(F("[WiFi] Connecting to existing network..."));
    WiFi.begin();
  }

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.printf("[WiFi] Successfully connected to ssid: %s\n", WiFi.SSID().c_str());
    Serial.printf("[WiFi] IP addess: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println(F("[WiFi] Configuring access point..."));
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();
    if (WiFi.softAP(settings.hostname)) {

      Serial.printf("[WiFi] Successfully created access point: %s\n", settings.hostname);
      Serial.printf("[WiFi] IP addess: %s\n", WiFi.softAPIP().toString().c_str());

      //start DNS server for easier configuration
      Serial.println(F("[WiFi] Starting DNS server"));
      dns_server.start(53, "*", WiFi.softAPIP());
      settings.soft_ap = true;

    } else {
      Serial.println(F("Something failed"));
      while (1);
    }
  }

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request ) {
    settings.reboot = true;
    request->redirect("/");
  });
  server.on("/wifi", HTTP_GET,  handle_wifi);
  server.on("/time", HTTP_GET,  handle_time);
  server.on("/overview", HTTP_GET,  handle_overview);
  server.on("/wifi", HTTP_POST,  handle_wifi_save);
  server.on("/time", HTTP_POST,  handle_time_save);
  server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age:600");

  Serial.println(F("Starting HTTP server"));
  server.begin();
  Serial.println(F("Starting network scanning async"));
  WiFi.scanNetworks(1);

   //configure SPI for displays
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  
  //configure I/O
  pinMode(CSN_PIN, OUTPUT);
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, HIGH);
  pinMode(TICK_PIN, OUTPUT);

  //start ticker
  ticks.attach_ms(500, tock);
  
  /*
   * activate display
   * to be updated to take values from settings
   */
  analogWrite(OE_PIN, 768);

  //check to see if time server is online (check internet connection also)
  settings.online = (boolean) Ping.ping(settings.time_server);
  if (settings.online) {
    local_time = ntp_get_time();
  } else {
    local_time.hours = 12;
    local_time.minutes = 0;
    local_time.seconds = 0;
  } 
}

void loop() {
  if (settings.reboot) {
    Serial.println("Got restart request from the wire!");
    settings.reboot = false;
    ESP.restart();
  }
  if (settings.soft_ap) {
    dns_server.processNextRequest();
  }
  if (settings.update_display) {
    settings.update_display = 0;
    update_displays(local_time);
  }
  if (settings.update_time) {
    settings.update_time = 0;
    local_time = ntp_get_time();
  }
}
