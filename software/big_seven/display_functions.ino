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
   * periodically update the time via ntp @1:00AM
   */
  if(local_time.hours == 1 && local_time.minutes == 0 && local_time.seconds == 0 && local_time.mseconds == 0) {
    local_time.update_time = 1;
  } 
  /**
   * since this is an interrupt we dont want to execute too much code here
   * we will update the clock in the loop
  */
  local_time.update_display = 1;

  /**
   * visual feedback using the led from GPIO12
   * used also as debug - if it blinks the esp8266 its working correct
  */
  tick_state = !tick_state;
  digitalWrite(TICK_PIN, tick_state);
}
/**
 * updates the display
 * @parm t dt
*/
void update_displays(d_t t) {
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
  digitalWrite(CSN_PIN, LOW);
  digitalWrite(CSN_PIN, HIGH);
  digitalWrite(CSN_PIN, LOW);
}
