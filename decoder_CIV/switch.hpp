#include "Arduino.h"

//#define DEBUG_FRAME

//--------------------------- byte frame value
const uint8_t SOFrame = 0xFE;
const uint8_t EOFrame = 0xFD;
const uint8_t COMrequest = 0x03;
const uint8_t COMtransceive = 0x00;
const uint8_t RIGaddress = 0x94;
const uint8_t CONTROLaddress = 0x0E;

/*                Byte position in the frame
 * -------------------------------------------------------------
 * |0xFE|0xFE|0x0E|0x94|0x03/0x00|BCD1|BCD2|BCD3|BCD4|BCD5|0xFD|
 * -------------------------------------------------------------
 * | 0  | 1  | 2  | 3  |    4    | 5  | 6  | 7  | 8  | 9  | 10 |
 * -------------------------------------------------------------
 * |Prelude  | To |From|Command  |          Data          |End |
 * ------------------------------------------------------------
 * When the frequency data comming by a request, the command is 0x03
 * When the frequency data comming by a change in the rig, (transceive 'ON' mode) the command is 0x00
 */

const uint8_t SOFpos = 0;
const uint8_t COMMANDpos = 4;
const uint8_t DATA1pos = 5;
const uint8_t DATA2pos = 6;
const uint8_t DATA3pos = 7;
const uint8_t DATA4pos = 8;
const uint8_t DATA5pos = 9;
const uint8_t EOFpos = 10;

uint32_t PreviousKhertz = 0;
uint8_t PreviousHertz = 0;
uint8_t FrequencyFrame [11];
uint8_t FramePos = 0;

uint8_t hex_to_dec (uint8_t hexValue) {
  return ((hexValue >> 4) * 10) + (hexValue & 0x0F);//
}

uint8_t dec_to_hex (uint8_t value) {
  return ((value/10) *16) + (value % 10);
}

//------------------------------------ REQUEST 
void request_frequency () {
  Serial1.flush();
  Serial1.write(SOFrame); 
  Serial1.write(SOFrame); 
  Serial1.write(RIGaddress); 
  Serial1.write(CONTROLaddress);
  Serial1.write(COMrequest); 
  Serial1.write(EOFrame); 
  delay(20); 
}

void debug_frame (uint16_t pos, uint16_t vHex, uint16_t vDec) {
  Serial.print ("Pos = ");
  Serial.print (pos);
  Serial.print (" ValHex = ");
  Serial.print (vHex, HEX);
  Serial.print (" ValDec = ");
  Serial.println (vDec, DEC);
}

//-------------------------- set Date and Time
void set_date (DateTime dtn) {
  uint8_t now_year = dtn.year();
  uint8_t now_month = dtn.month();
  uint8_t now_day = dtn.day();
 
  Serial1.flush();
  Serial1.write(SOFrame); 
  Serial1.write(SOFrame); 
  Serial1.write(RIGaddress); 
  Serial1.write(CONTROLaddress);
  Serial1.write(0x1A); 
  Serial1.write(0x05);
  Serial1.write(0x00);
  Serial1.write(0x94);
  Serial1.write(dec_to_hex(int(now_year/100)));
  Serial1.write(dec_to_hex(now_year % 100));
  Serial1.write(dec_to_hex(now_month));
  Serial1.write(dec_to_hex(now_day));
  Serial1.write(EOFrame); 
  delay(20); 
}

void set_time (DateTime dtn) {
  int now_hour = dtn.hour ();
  int now_minute = dtn.minute ();
 
  Serial1.flush();
  Serial1.write(SOFrame); 
  Serial1.write(SOFrame); 
  Serial1.write(RIGaddress); 
  Serial1.write(CONTROLaddress);
  Serial1.write(0x1A); 
  Serial1.write(0x05);
  Serial1.write(0x00);
  Serial1.write(0x95);
  Serial1.write(dec_to_hex(now_hour));
  Serial1.write(dec_to_hex(now_minute));
  Serial1.write(EOFrame); 
  delay(20); 
}

uint32_t decode_frequency () {
  uint32_t frq = 0;

  if (FrequencyFrame [SOFpos] != SOFrame)
    return 0;
    
  if (FrequencyFrame [COMMANDpos] != COMrequest && FrequencyFrame [COMMANDpos] != COMtransceive)
    return 0;
    
  if (FrequencyFrame [EOFpos] != EOFrame)
    return 0;

  for (uint8_t bytePos = SOFpos; bytePos <= EOFpos; bytePos++) {
    uint16_t hexValue = FrequencyFrame [bytePos];
    uint16_t decValue = hex_to_dec (hexValue);

    #ifdef DEBUG_FRAME
      debug_frame (bytePos, hexValue, decValue);
    #endif  //DEBUG_FRAME    

    switch (bytePos) {
      case DATA1pos:
        frq += int (decValue / 10); break;
      case DATA2pos:
        frq += decValue * 10; break;
      case DATA3pos:
        frq += decValue * 1000; break;
      case DATA4pos:
        frq += decValue * 100000; break;
      case DATA5pos:
        frq += decValue * 10000000; break;
    }
  }
  return frq; 
}

uint32_t process_frequency () {
  while (Serial1.available ()) {
    uint8_t incomingByte = Serial1.read ();
    FrequencyFrame [FramePos] = incomingByte;
    FramePos++;
    if (incomingByte == EOFrame){
      FramePos = 0; 
      return decode_frequency ();      
    }
  }
}
