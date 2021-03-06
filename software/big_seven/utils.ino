/**
 * basic clock in software, using system ticker to cound seconds
*/
void tock() {
  static uint8_t tick_state = 0;

  /**
   * clock part
   * increase seconds every 2 ticks
  */
  local_time.mseconds += 1;
  if ( local_time.mseconds > 1) {
    local_time.mseconds = 0;
    local_time.seconds += 1;
  }
  if (local_time.seconds > 59) {
    local_time.seconds = 0;
    local_time.minutes += 1;
  }
  if (local_time.minutes > 59) {
    local_time.minutes = 0;
    local_time.hours += 1;
  }
  if (local_time.hours > 23) {
    local_time.hours = 0;
  } 
  /**
   * since this is an interrupt we dont want to execute too much code here
   * we will update the clock in the loop
  */
  settings.update_display = 1;

  /**
   * visual feedback using the led from GPIO12
   * used also as debug - if it blinks the esp8266 its working correct
  */
  tick_state = !tick_state;
  digitalWrite(TICK_PIN, tick_state);
}
/**
 * updates the display
 * @parm ctime_t t
*/
void update_displays(ctime_t t) {
  /**
   * since display 1 & 2 are rotated 180 deg, data for the 7 segment display looks a bit different
  */
  static uint8_t numbers[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
  };
  static uint8_t numbers_r[] = {
    0x3F, 0x06, 0x9B, 0x8F, 0xA6, 0xAD, 0xBD, 0x07, 0xBF, 0xAF
  };
  if (t.hours > 9) {
    SPI.transfer(numbers[t.hours / 10]);
    SPI.transfer(t.mseconds ? numbers[t.hours % 10] | 0x80 : numbers[t.hours % 10]);
  }
  else {
    SPI.transfer(numbers[0]);
    SPI.transfer(t.mseconds ? numbers[t.hours] | 0x80 : numbers[t.hours]);
  }

  if (t.minutes > 9) {
    SPI.transfer(t.mseconds ? numbers_r[t.minutes / 10] | 0x40 : numbers_r[t.minutes / 10]);
    SPI.transfer(numbers_r[t.minutes % 10]);
  }
  else {
    SPI.transfer(t.mseconds ? numbers_r[0] | 0x40 : numbers_r[0]);
    SPI.transfer(numbers_r[t.minutes]);
  }
  //clock the data out
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}
/*
 * return size of file human readable
 */
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
/*
 * rssi to quality for sorting networks
 * based on http://www.speedguide.net/faq/how-does-rssi-dbm-relate-to-signal-quality-percent-439
 */
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
/*
 * helper function for saving data structures to file
 * havely based on  https://github.com/letscontrolit/ESPEasy/blob/mega/src/Misc.ino#L767
 */
boolean save_file(char* fname, byte* memAddress, int datasize){

  fs::File f = SPIFFS.open(fname, "w+");
  if (f) {
    byte *pointerToByteToSave = memAddress;
    for (int x = 0; x < datasize ; x++) {
      f.write(*pointerToByteToSave);
      pointerToByteToSave++;
    }
    f.close();
  }
  return true;
}
/*
 * helper function for reading data structures from file
 * havely based on  https://github.com/letscontrolit/ESPEasy/blob/mega/src/Misc.ino#L801
 */
void read_file(char* fname, byte* memAddress, int datasize) {
  fs::File f = SPIFFS.open(fname, "r+");
  if (f) {
    byte *pointerToByteToRead = memAddress;
    for (int x = 0; x < datasize; x++) {
      *pointerToByteToRead = f.read();
      pointerToByteToRead++;// next byte
    }
    f.close();
  }
}
/*
 * pretty uptime
 
String mkuptime(uint32_t seconds) {
  uint8_t days;
  uint8_t hours;
  uint8_t minutes;

  days = seconds / 86400;
  seconds = seconds % days;
  hours = seconds / 3600;
  seconds = seconds % hours;
  minutes = seconds / 60;
  seconds = seconds % minutes;

  char buf[32] = {0};
  sprintf(buf, "%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, seconds);
  Serial.println(buf);
}
*/
