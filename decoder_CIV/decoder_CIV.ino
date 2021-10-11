#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include "switch.hpp"

LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS3231 rtc;

void setup () {
  const unsigned long serial0Rate = 115200;
  const uint16_t serial1Rate = 19200;

  Serial.begin (serial0Rate);
  Serial.flush ();
  Serial1.begin (serial1Rate);
  Serial1.flush ();

  rtc.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear(); 
}

void loop () {
  if (Serial1.available ()) {
    process_frequency ();
  }
}
