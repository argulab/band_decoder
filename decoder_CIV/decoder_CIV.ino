#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

#define BAUD_RATE 19200
#define ICOM_ADDRESS 0x94
#define CONTROL_ADDRESS 0x0E

const uint8_t pinHEX = 32;
const uint8_t pinAPE = 34;

uint8_t frequencyFrame [11];
uint8_t byteCounter = 0;
uint16_t frequency = 0;
uint16_t previousFrequency = frequency;

enum bands {
  b0, b6, b10, b12, b15, b17, b20, b30, b40, b60, b80, b160
  };
enum antennas {
  none, hexbeam, aperiodic
  };
  
bands previousBand = b0;
antennas previousAntenna = none;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

//----------------------------------- INTERNAL WATCH
int dec_to_hex (int value) {
  return ((value/10) *16) + (value % 10);
}

void set_date () {
  DateTime dt_now = rtc.now();
  int now_year = dt_now.year();
  int now_month = dt_now.month();
  int now_day = dt_now.day();
 
  Serial1.flush();
  Serial1.write(0xFE); 
  Serial1.write(0xFE); 
  Serial1.write(ICOM_ADDRESS); 
  Serial1.write(CONTROL_ADDRESS);
  Serial1.write(0x1A); 
  Serial1.write(0x05);
  Serial1.write(0x00);
  Serial1.write(0x94);
  Serial1.write(dec_to_hex(int(now_year/100)));
  Serial1.write(dec_to_hex(now_year % 100));
  Serial1.write(dec_to_hex(now_month));
  Serial1.write(dec_to_hex(now_day));
  Serial1.write(0xFD); 
  delay(20); 
}

void set_hour () {
  DateTime dt_now = rtc.now ();
  int now_hour = dt_now.hour ();
  int now_minute = dt_now.minute ();
 
  Serial1.flush();
  Serial1.write(0xFE); 
  Serial1.write(0xFE); 
  Serial1.write(ICOM_ADDRESS); 
  Serial1.write(CONTROL_ADDRESS);
  Serial1.write(0x1A); 
  Serial1.write(0x05);
  Serial1.write(0x00);
  Serial1.write(0x95);
  Serial1.write(dec_to_hex(now_hour));
  Serial1.write(dec_to_hex(now_minute));
  Serial1.write(0xFD); 
  delay(20); 
}

//------------------------------------ REQUEST 
void request_frequency () {
  Serial1.flush();
  Serial1.write(0xFE); 
  Serial1.write(0xFE); 
  Serial1.write(ICOM_ADDRESS); 
  Serial1.write(CONTROL_ADDRESS);
  Serial1.write(0x03); 
  Serial1.write(0xFD); 
  delay(20); 
}

//-------------------------------------------- DECODER FUNCTIONS
antennas select_antenna (bands b) {
  if (b == b6 || b == b10 || b == b12 || b == b15 || b == b17 || b == b20) 
    return hexbeam;
  if (b == b30 || b == b40 || b == b60 || b == 80 || b == b160) 
    return aperiodic;  
}
bands decode_band () {
  if (frequency >=  1810 && frequency <=  2000) return b160;
  if (frequency >=  3500 && frequency <=  3800) return b80;
  if (frequency >=  5351 && frequency <=  5366) return b60;
  if (frequency >=  7000 && frequency <=  7200) return b40;
  if (frequency >= 10100 && frequency <= 10150) return b30;
  if (frequency >= 14000 && frequency <= 14350) return b20;
  if (frequency >= 18068 && frequency <= 18168) return b17;
  if (frequency >= 21000 && frequency <= 21450) return b15;
  if (frequency >= 24890 && frequency <= 24990) return b12;
  if (frequency >= 28000 && frequency <= 29700) return b10;
  if (frequency >= 50000 && frequency <= 52000) return b6;

  return b0;
}

String name_of_band (bands b) {
  switch (b) {
    case b0:   return "NULL";
    case b160: return "160m";
    case b80:  return "80m";
    case b60:  return "60m";
    case b40:  return "40m";
    case b30:  return "30m";
    case b20:  return "20m";
    case b17:  return "17m";
    case b15:  return "15m";
    case b12:  return "12m";
    case b10:  return "10m";
    case b6:   return "6m";
  }
}

//-------------------------------------------- DISPLAY
void show_frequency () {
  lcd.setCursor (5, 0);
  lcd.print("     ");
  lcd.setCursor (5, 0);
  lcd.print (frequency);
}

void show_band (uint8_t band) {
  lcd.setCursor (11, 0);
  lcd.print ("    ");
  lcd.setCursor (11, 0);
  lcd.print (name_of_band (band));
}

void show_antenna (antennas a) {
  lcd.setCursor (0, 1);
  if (a == hexbeam)
    lcd.print ("    HEXBEAM     ");
  if (a == aperiodic)
    lcd.print ("   APERIODIC    ");
  if (a == none)
    lcd.print ("      NONE      ");
}

//----------------------------------- FREQUENCY_FUNCTIONS
void switch_antenna_relay (antennas a) {
  switch (a) {
    case none: 
      digitalWrite (pinHEX, LOW);
      digitalWrite (pinAPE, LOW);
      break;
    case hexbeam:
      digitalWrite (pinHEX, HIGH);
      digitalWrite (pinAPE, LOW);
      break;
    case aperiodic:
      digitalWrite (pinHEX, LOW);
      digitalWrite (pinAPE, HIGH);
      break;
  }
}

uint16_t hex_to_dec (uint8_t hexValue) {
  return ((hexValue >> 4) * 10) + (hexValue & 0x0F);
}

void process_frequency () {
  if (frequency != previousFrequency) {
    previousFrequency = frequency;
    show_frequency ();
    
    bands activeBand = decode_band ();
    if (activeBand != previousBand) {
      previousBand = activeBand;
      show_band (activeBand);
      antennas activeAntenna = select_antenna (activeBand);
      if (activeAntenna != previousAntenna) {
        previousAntenna = activeAntenna;
        show_antenna(activeAntenna);
        switch_antenna_relay (activeAntenna);
      }
    } 
  }
}

void setup() { 
  Serial1.begin(BAUD_RATE);
  Serial.begin (9600);
  Serial.flush ();
  rtc.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor (0, 0);
  lcd.print ("Dial:");

  pinMode (pinHEX, OUTPUT);
  pinMode (pinAPE, OUTPUT);
  digitalWrite (pinHEX, LOW);
  digitalWrite (pinAPE, LOW);

//  set_hour ();
//  set_date ();
  
  request_frequency ();
}

void loop () {
  uint8_t endByte = 0xFD;
  uint8_t endPos = 10;
  uint8_t data3Pos = 6; //KHz/10
  uint8_t data2Pos = 7; //KHz*10
  uint8_t data1Pos = 8; //MHz*1000 

  if (Serial1.available ()) {
    int incomingByte = Serial1.read ();
    
    frequencyFrame [byteCounter] = incomingByte;
    byteCounter++;

    if (incomingByte == endByte) {
      if (frequencyFrame [endPos] == endByte) {
        for (uint8_t bytePos = data1Pos; bytePos >= data3Pos; bytePos--) {
          if (bytePos == data1Pos) frequency+= hex_to_dec (frequencyFrame [bytePos]) * 1000;
          if (bytePos == data2Pos) frequency+= hex_to_dec (frequencyFrame [bytePos]) * 10;
          if (bytePos == data3Pos) frequency+= int (hex_to_dec (frequencyFrame [bytePos]) / 10);
        }
        process_frequency ();
        frequency = 0;       
      }
      byteCounter = 0;
    }    
  }
}
