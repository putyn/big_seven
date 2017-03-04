#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

AsyncWebServer server(80);

//helper functions
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}
//rssi to procentage
int rssi2quality(int rssi) {
  int quality;

  if ( rssi <= -100) {
    quality = 0;
  } else if ( rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * ( rssi + 100);
  }
  return quality;
}


void handle_wifi_save(AsyncWebServerRequest *request) {
  String json_resp;
  String ssid;
  String pass;

  if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
    //get ssid and pass from POST request
    ssid = request->getParam("ssid", true)->value();
    pass = request->getParam("pass", true)->value();

    if (ssid.length() == 0 || pass.length() == 0) {
      json_resp = F("{\"error\":true, \"message\": \"Password can't be empty\"}");
    } else {
      //debug message
      Serial.printf("Got ssid: %s with password: %s, saving them to flash\n", ssid.c_str(), pass.c_str());
      File config_file = SPIFFS.open("/config.txt", "w");
      if (config_file) {
        config_file.println(ssid.c_str());
        config_file.println(pass.c_str());
        json_resp = F("{\"error\":false, \"message\": \"Settings saved, reboot to load new network config\"}");
      } else {
        json_resp = F("{\"error\":true, \"message\": \"Settings could not be saved\"}");
      }
    }
  } else {
    json_resp = F("{\"error\":true, \"message\": \"Bad request\"}");
    goto send_resp;
  }

send_resp:
  request->send(200, "text/json", json_resp);
}

void handle_wifi(AsyncWebServerRequest *request) {

  uint8_t found_networks = 0;
  uint8_t idx = 0;
  String json_resp;

  if ((WiFi.getMode() & WIFI_AP_STA) == WIFI_AP_STA) {
    json_resp = "{\"status\":{\"Mode\":\"Access Point\",\"IP\":\"" + WiFi.softAPIP().toString() + "\",\"Hostname\":\"" + WiFi.hostname() + "\"},\"networks\":[";
  } else {
    json_resp = "{\"status\":{\"Mode\":\"Client\",\"SSID\":\"" + WiFi.SSID() + "\",\"IP\":\"" + WiFi.localIP().toString() + "\",\"Hostname\":\"" + WiFi.hostname() + "\"},\"networks\":[";
  }

  found_networks = WiFi.scanComplete();
  if ( found_networks > 0) {
    for (idx = 0; idx < found_networks; ++idx) {
      if (idx)
        json_resp += ",";
      json_resp += "{\"ssid\":\"" + WiFi.SSID(idx) + "\",\"auth\":" + (WiFi.encryptionType(idx) == ENC_TYPE_NONE ? 0 : 1) + ",\"quality\":" + rssi2quality(WiFi.RSSI(idx)) + "}";
    }
    WiFi.scanDelete();
    WiFi.scanNetworks(1);
  }
  json_resp += "]}";
  request->send(200, "text/json", json_resp);
}

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
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  Serial.printf("\n");

  //WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  //check to see if we have config file
  Serial.println(F("Checking to see if we have new config..."));
  if (SPIFFS.exists("/config.txt")) {
    File config_file = SPIFFS.open("/config.txt", "r");
    String ssid = config_file.readStringUntil('\n'); //first line ssid
    String pass = config_file.readStringUntil('\n'); //second line pass
    ssid.trim(); //remove spaces
    pass.trim(); //remove spaces
    Serial.printf("Found config trying to connect to ssid %s with pass %s\n", ssid.c_str(), pass.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    //remove config
    SPIFFS.remove("/config.txt");
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
    //ESP.restart();
  });
  server.on("/wifi", HTTP_GET,  handle_wifi);
  server.on("/wifi", HTTP_POST,  handle_wifi_save);
  server.serveStatic("/", SPIFFS, "/").setCacheControl("max-age:600");

  server.begin();
  Serial.println(F("HTTP server started on port 80"));
  Serial.println(F("Starting network scanning async"));
  WiFi.scanNetworks(1);
  Serial.println(F("Starting mdns server"));
  MDNS.begin(new_hostname.c_str());
  MDNS.addService("http", "tcp", 80);
}

void loop() {
}
