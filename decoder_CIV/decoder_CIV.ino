#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include "switch.hpp"

uint8_t PreviousMhz = 0;
uint16_t PreviousKhz = 0;
uint8_t PreviousActiveBand = 0;
uint8_t PreviousActiveAntenna = 0;

uint8_t MaxCol = 19;
uint8_t FrequencyRow = 2;
uint8_t AntennaRow = 3;

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);

//---------------------------------- frequency service
void init_display () {
  lcd.setCursor (0, FrequencyRow);
  lcd.print ("  .   ,    Band=");
  lcd.setCursor (0, AntennaRow);
  lcd.print ("Antenna=");
}

 void display_freq_mhz (uint16_t mhz) {
  uint8_t mhzCol = 0;
  if (mhz < 10)
    mhzCol = 1;
  lcd.setCursor (0, FrequencyRow);
  lcd.print ("  ");
  lcd.setCursor (mhzCol, FrequencyRow);
  lcd.print (mhz);
 }

 void display_freq_khz (uint16_t khz) {
  uint8_t khzCol = 3;
  lcd.setCursor (khzCol, FrequencyRow);
  if (khz < 100 && khz >= 10)
    lcd.print ("0");
  if (khz < 10)
    lcd.print ("0");
  lcd.print (khz);
 }

 void display_freq_hz (uint16_t hz) {
  uint8_t hzCol = 7;
  lcd.setCursor (hzCol, FrequencyRow);
  if (hz < 10)
    lcd.print ("0");
  lcd.print (hz);
 }

 void display_band (String band) {
  uint8_t bandPos = 16;
  lcd.setCursor (bandPos, FrequencyRow);
  lcd.print ("    ");
  lcd.setCursor (bandPos, FrequencyRow);
  lcd.print (band);
 }

void display_active_antenna (uint8_t ant) {
  uint8_t antennaPos = 9;
  lcd.setCursor (antennaPos, AntennaRow);
  lcd.print ("         ");
  lcd.setCursor (antennaPos, AntennaRow);
  lcd.print (name_of_antenna (ant));
 }

void service_switch_antenna (uint8_t ant) {
  switch (anf) 
    
 }

void frequency_service (uint32_t frq) {
  uint16_t frqMhz = int (frq / 100000);
  uint16_t frqKhz = int (frq / 100) - (frqMhz * 1000);
  uint16_t frqHz = frq % 100;

  if (frqMhz != PreviousMhz) {
    PreviousMhz = frqMhz;
    display_freq_mhz (frqMhz);
  }
  if (frqKhz != PreviousKhz) {
    PreviousKhz = frqKhz;
    display_freq_khz (frqKhz);
  }
  display_freq_hz (frqHz);

  uint8_t activeBand = decode_band (frq);
  if (activeBand != PreviousActiveBand) {
    PreviousActiveBand = activeBand;
    display_band (name_of_band (activeBand));

    uint8_t activeAntenna = select_antenna (activeBand);
    if (activeAntenna != PreviousActiveAntenna) {
      PreviousActiveAntenna = activeAntenna;
      display_active_antenna (activeAntenna);
      service_switch_antenna (activeAntenna);
    }
  } 
}

void setup () {
  const unsigned long serial0Rate = 115200;
  const uint16_t serial1Rate = 19200;
    
  Serial.begin (serial0Rate);
  Serial.flush ();
  Serial1.begin (serial1Rate);
  Serial1.flush ();

  lcd.init ();
  rtc.begin (); 
  lcd.backlight();
  lcd.clear();

  init_relays_pins ();
  init_display ();
  request_frequency ();
  
  DateTime dtNow = rtc.now ();
//  set_date (dtNow);
//  set_time (dtNow);
}

void loop () {  
  if (Serial1.available ()) {
    uint32_t frequency = process_frequency (); 
    if (frequency != 0) {
      frequency_service (frequency);
    }
  }
}
