#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Ticker.h>

//data types & protoypes
struct d_t {
  uint8_t update_display;
  uint8_t update_time;
  uint8_t mseconds;
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
};

d_t ntp_get_time();
void ntp_send_request(IPAddress& address);
void update_displays(d_t t);
void tock();

//defines and variables
#define CSN_PIN 16
#define OE_PIN 2
#define TICK_PIN 12

//wifi ssid and password
static char ssid[] = "****";
static char pass[] = "****";  

Ticker ticks;
d_t local_time;
WiFiUDP ntp;

void setup() {
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //configure SPI for displays
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  
  pinMode(CSN_PIN, OUTPUT);
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, HIGH);
  pinMode(TICK_PIN, OUTPUT);
  
  //update clock 
  local_time = ntp_get_time();
  //activate display
  //digitalWrite(OE_PIN, LOW);
  //analogWriteFreq(500);
  analogWrite(OE_PIN, 768);
  //start ticker
  ticks.attach_ms(500, tock);
}

void loop() {
  if(local_time.update_display) {
    local_time.update_display = 0;
    update_displays(local_time);
  }
  if(local_time.update_time) {
    local_time.update_time = 0;
    local_time = ntp_get_time();
  }
}


