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

const uint8_t RELAY1pin = 32;
const uint8_t RELAY2pin = 34;
const uint8_t RELAY3pin = 36;
const uint8_t RELAY4pin = 38;
const uint8_t RELAY5pin = 40;
const uint8_t RELAY6pin = 42;
const uint8_t RELAY7pin = 44;
const uint8_t RELAY8pin = 46;

uint32_t PreviousKhertz = 0;
uint8_t PreviousHertz = 0;
uint8_t FrequencyFrame [11];
uint8_t FramePos = 0;

//-------------------------- enum variables
enum bands {
  b0, b6, b10, b12, b15, b17, b20, b30, b40, b60, b80, b160
  };
enum antennas {
  none, hexbeam, aperiodic, ant3, ant4
  };

//------------------------------------- decoders
uint8_t hex_to_dec (uint8_t hexValue) {
  return ((hexValue >> 4) * 10) + (hexValue & 0x0F);
}

uint8_t dec_to_hex (uint8_t value) {
  return ((value/10) *16) + (value % 10);
}

void init_relays_pins () {
  pinMode (RELAY1pin, OUTPUT); 
  pinMode (RELAY2pin, OUTPUT);
  pinMode (RELAY3pin, OUTPUT);
  pinMode (RELAY4pin, OUTPUT);
  pinMode (RELAY5pin, OUTPUT);
  pinMode (RELAY6pin, OUTPUT);
  pinMode (RELAY7pin, OUTPUT);
  pinMode (RELAY8pin, OUTPUT);

  digitalWrite (RELAY1pin, LOW);
  digitalWrite (RELAY2pin, LOW);
  digitalWrite (RELAY3pin, LOW);
  digitalWrite (RELAY4pin, LOW);
  digitalWrite (RELAY5pin, LOW);
  digitalWrite (RELAY6pin, LOW);
  digitalWrite (RELAY7pin, LOW);
  digitalWrite (RELAY8pin, LOW);
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

//-------------------------------------------- DECODER FUNCTIONS
antennas select_antenna (bands band) {
  switch (band) {
    case b6: return ;
    case b10: return hexbeam;
    case b12: return hexbeam;
    case b15: return hexbeam;
    case b17: return hexbeam;
    case b20: return hexbeam;
    case b30: return aperiodic;
    case b40: return aperiodic;
    case b60: return aperiodic;
    case b80: return ant3;
    case b160: return ant4;
    default: return none;
  }
}

String name_of_antenna (antennas ant) {
  switch (ant) {
    case none:      return "NONE";
    case hexbeam:   return "HEXBEAM";
    case aperiodic: return "APERIODIC";
    case ant3:      return "Antenna 3";
    case ant4:      return "Antenna 4";
  }
}

bands decode_band (uint32_t frq) {
  if (frq >=  181000 && frq <=  200000) return b160;
  if (frq >=  350000 && frq <=  380000) return b80;
  if (frq >=  535100 && frq <=  536600) return b60;
  if (frq >=  700000 && frq <=  720000) return b40;
  if (frq >= 1010000 && frq <= 1015000) return b30;
  if (frq >= 1400000 && frq <= 1435000) return b20;
  if (frq >= 1806800 && frq <= 1816800) return b17;
  if (frq >= 2100000 && frq <= 2145000) return b15;
  if (frq >= 2489000 && frq <= 2499000) return b12;
  if (frq >= 2800000 && frq <= 2970000) return b10;
  if (frq >= 5000000 && frq <= 5200000) return b6;

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

uint32_t decode_frequency () {
  uint32_t frq = 0;

  if (FrequencyFrame [SOFpos] != SOFrame)
    return 0;
    
  if (FrequencyFrame [COMMANDpos] != COMrequest && FrequencyFrame [COMMANDpos] != COMtransceive)
    return 0;
    
  if (FrequencyFrame [EOFpos] != EOFrame)
    return 0;

  for (uint8_t bytePos = SOFpos; bytePos <= EOFpos; bytePos++) {
    uint32_t hexValue = FrequencyFrame [bytePos];
    uint32_t decValue = hex_to_dec (hexValue);

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
