##big_seven hardware

Initital hardware was suppose to have individual current sources for every segement of the display similar to this https://www.dorkbotpdx.org/blog/paul/big_7segment_countdown_timer, until i found the TPIC6x595 which has the same functionality as the 74HC595 (shift register) only higher output voltage.

For the brains the ESP8266 was chosen, simply because its cheap its supporeted by the Arduino IDE and has WiFi. 

##leasens to learn
1. never again use LDO to drop high voltage to low voltage @ high currents https://github.com/esp8266/Arduino/issues/1840