#include "Arduino.h"

#define DEBUG_FRAME

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
 * When the frequency data comming by a change in the rig, (transceive) the command is 0x00
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

void debug_frame (uint8_t pos, uint8_t val) {
  Serial.print ("Pos = ");
  Serial.print (pos);
  Serial.print (" Val = ");
  Serial.println (val, HEX);
}

uint16_t hex_to_dec (uint8_t hexValue) {
  return ((hexValue >> 4) * 10) + (hexValue & 0x0F);//
}

/*
 * The frequency range is of 10 Hz
 */

uint32_t decode_frequency () {
  uint32_t frq = 0;

  if (FrequencyFrame [SOFpos] != SOFrame)
    return frq;
  if (FrequencyFrame [COMMANDpos] != COMrequest && FrequencyFrame [COMMANDpos] != COMtransceive)
    return frq;
  if (FrequencyFrame [EOFpos] != EOFrame)
    return frq;

  for (uint8_t bytePos = 0; bytePos <= EOFpos; bytePos++) {
    uint8_t byteValue = FrequencyFrame [bytePos];
    
    #ifdef DEBUG_FRAME
      debug_frame (bytePos, byteValue);
    #endif  //DEBUG_FRAME
          
    if (bytePos == DATA1pos)
      frq += int (hex_to_dec (byteValue) / 10); 
    if (bytePos == DATA2pos)
      frq += hex_to_dec (byteValue) * 10;
    if (bytePos == DATA3pos)
      frq += hex_to_dec (byteValue) * 1000;
    if (bytePos == DATA4pos)
      frq += hex_to_dec (byteValue) * 100000;
    if (bytePos == DATA5pos)
      frq += hex_to_dec (byteValue) * 10000000;
  }
  return frq;
}  

void process_frequency () {
  uint8_t incomingByte = Serial1.read ();
  FrequencyFrame [FramePos] = incomingByte;
  FramePos++;

  if (incomingByte == EOFrame) {
    FramePos = 0;
    uint32_t frequency = decode_frequency ();
    show_frequency (frequency);
  }    
}
