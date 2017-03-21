/*
 * handle wifi setting save
 */
void handle_wifi_save(AsyncWebServerRequest *request) {
  //json response
  String json_resp;
  
  if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
    
    //get ssid and pass from POST request
    strncpy(wifi.ssid, request->getParam("ssid", true)->value().c_str(), sizeof (wifi.ssid));
    strncpy(wifi.pass, request->getParam("pass", true)->value().c_str(), sizeof (wifi.pass));

    if (save_file((char *)"/wifi.dat", (byte *)&wifi, sizeof(struct wifi_settings_t))) {
      json_resp = F("{\"error\":false, \"message\": \"Settings saved, reboot to load new network config\"}");
    } else {
      json_resp = F("{\"error\":true, \"message\": \"Settings could not be saved\"}");
    }
  } else {
    json_resp = F("{\"error\":true, \"message\": \"Bad request\"}");
  }
  request->send(200, "text/json", json_resp);
}
/*
 * Handle [GET] /wifi
 * returns wifi status
 * available network list
 */
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
/*
 * placeholder template for time, untill its finished
 */
void handle_time(AsyncWebServerRequest *request) {
  String json_resp;

  json_resp = "{\"time_server\":\"ro.pool.ntp.org\",\"time_zone\":7200, \"time_dts\": 1}";
  request->send(200, "text/json", json_resp);
}

