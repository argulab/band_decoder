#include <LiquidCrystal_I2C.h>

#define BAUD_RATE 19200     
#define ICOM_ADDRESS (0x94)  
#define CONTROL_ADDRESS (0x0E)
#define COMMAND_MODE_POSITION 4 
//#define DEBUG_INCOMIG_BYTE
//#define DEBUG_FRAME_READ
//#define DEBUG_PROCESS_FRAME

int byteCounter = 0;
byte frequencyFrame [64];
int megaHertz = 0;
int kiloHertz = 0;
int hertz = 0;
int previousMegaHertz = 0;
int previousKiloHertz = 0;
int previousHertz = 0;

enum bands {
    b0, b6, b10, b12, b15, b17, b20, b30, b40, b60, b80, b160
};

bands previousActiveBand = b0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

//-------------------------------------DEBUG--------------------------------------------------------
void debug_incomingByte (int pos, byte byte_frame) {
  Serial.print ("Byte frame position = ");
  Serial.print (pos);
  Serial.print (" Byte HEX value --> ");
  Serial.println (byte_frame, HEX);
}

void debug_frame_read (int frameLenght) {
  Serial.println ("---------------------- DEBUG_FRAME_READ ---------------------------");
  for (int i=0; i<=frameLenght - 1; i++) {
    Serial.print ("Byte pos = ");
    Serial.print (i);
    Serial.print (" Byte value --> ");
    Serial.println (frequencyFrame [i], HEX);
  }
}

void debug_process_frame () {
  Serial.println ("--------------------- DEBUG PROCESS_FRAME ------------------------");
  Serial.print ("MegaHerzios = ");
  Serial.println (megaHertz);
  Serial.print ("KiloHertcios = ");
  Serial.println (kiloHertz);
  Serial.print ("Hertcios = ");
  Serial.println (hertz);
}

//----------------------------------------------------------------------------------------------
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

void request_date () {
  Serial1.flush();
  Serial1.write(0xFE); 
  Serial1.write(0xFE); 
  Serial1.write(ICOM_ADDRESS); 
  Serial1.write(CONTROL_ADDRESS);
  Serial1.write(0x1A); 
  Serial1.write(0x05);
  Serial1.write(0x95);
  Serial1.write (0x09);
  Serial1.write (0x21); 
  Serial1.write(0xFD); 
  delay(20);  
}

String bands_name (bands b) {
  switch (b) {
    case  b0:   return "NULL";
    case  b160: return "160m";
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

bool process_frame (int frameLenght) {
  int byteTo = 2;
  int byteFrom = 3;
  int byteCommand = 4;
  int decimalValues[5];
  int counter = 0;

  //Filter conditions
  if (frequencyFrame[byteTo] != 0x00 && frequencyFrame[byteTo] != 0x0E)
    return false;

  if (frequencyFrame[byteFrom] != 0x94)
    return false;

  if (frequencyFrame[byteCommand] != 0x00 && frequencyFrame[byteCommand] != 0x03)
    return false;

  for (int byteData = 9; byteData >= 5; byteData--) { 
    decimalValues[counter] = 
    (frequencyFrame[byteData]>>4)*10 +  //0x28 = 00101000 >> 0010 = 2*10 = 20
    (frequencyFrame[byteData]&0x0F);    //0x28 = 00101000 (AND) 00001111 = 00001000 = 8 
//    (frequencyFrame[byteData]%16);    //0x28 = 40 / 16 = 2; resto 8
    counter++;
  }

  megaHertz = decimalValues[1];
  kiloHertz = (decimalValues[2] * 10) + int (decimalValues[3] / 10);
  hertz = ((decimalValues[3]%10)*100) + decimalValues[4]; 

  #ifdef DEBUG_PROCESS_FRAME              
    debug_process_frame ();
  #endif //DEBUG_PROCESS_FRAME

  return true;
}

void display_kilo_hertz () {
  int kiloPos = 3;
  
  lcd.setCursor (kiloPos, 0);
  if (kiloHertz == 0) {
    lcd.print ("000");
  }
  
  if (kiloHertz >= 1 && kiloHertz < 10) {
    lcd.print ("00");
    lcd.print (kiloHertz);
  }
  
  if (kiloHertz >= 10 && kiloHertz < 100) {
    lcd.print ("0");
    lcd.print (kiloHertz);
  }
  
  if (kiloHertz >= 100) {
    lcd.print (kiloHertz);  
  }  
}

void display_hertz () {
  int hertzPos = 7;
  
  lcd.setCursor (hertzPos, 0);
  if (hertz < 100) {
    lcd.print ("0");
    lcd.print (int(hertz/10));
  }
  if (hertz >= 100) {
    lcd.print (int(hertz/10));
  }
}

void display_band (bands band) {
  int colPos = 11;
  if (band == b160)
    colPos = 10;

  if (band == b6)
    colPos = 12;

  lcd.setCursor (10, 0);
  lcd.print ("       ");
  lcd.setCursor(colPos, 0);
  lcd.print ("B=");
  lcd.print (bands_name(band)); 
}

void process_band () {
  int megaPos = 0;
  String kHertz = "000";
  
  if (megaHertz < 10)
    megaPos = 1;
    
  lcd.clear ();
  lcd.setCursor (megaPos, 0);
  lcd.print (megaHertz);
  lcd.print (".");
  display_kilo_hertz ();
  lcd.print (",");
  display_hertz ();  
}

void frequency_changed () {
  previousMegaHertz = megaHertz;
  previousKiloHertz = kiloHertz;
  previousHertz = hertz;  
}

bands decode_band () {
  switch (megaHertz) {
    case 1: return b160;
    case 3: return b80;
    case 5: return b60;
    case 7: return b40;
    case 10: return b30;
    case 14: return b20;
    case 18: return b17;
    case 21: return b15;
    case 24: return b12;
    case 28: return b10;
    case 50: return b6;

    default: return b0;
  }  
}

void process_change_of_frequency (bool mHz, bool kHz, bool Hz) {
  if (mHz) {
    process_band ();
    bands activeBand = decode_band ();
    previousActiveBand = activeBand;
    display_band (activeBand);
    
    //Process_relays (); --------------------- TO DO ---------------------------
    frequency_changed ();
    return;
  }

  if (kHz) 
    display_kilo_hertz ();

  if (Hz) 
    display_hertz ();

  frequency_changed ();
}

//---------------------------------------------------------------------------------------------

void setup() { 
  Serial1.begin(BAUD_RATE);
  Serial.begin (9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  request_frequency (); 
}

//---------------------------------------------------------------------------------------------

void loop() {
  if (Serial1.available () > 0) {
    byte incomingByte = Serial1.read ();
    frequencyFrame[byteCounter] = incomingByte;

    #ifdef DEBUG_INCOMIG_BYTE
      debug_incomingByte (byteCounter, incomingByte);  
    #endif //DEBUG

    byteCounter++;
    
    if (incomingByte == 0xFD) {
      bool processed = process_frame (byteCounter);
      byteCounter = 0;

      bool changeMegaHertz = previousMegaHertz != megaHertz;
      bool changeKiloHertz = previousKiloHertz != kiloHertz;
      bool changeHertz = previousHertz != hertz;

      process_change_of_frequency (changeMegaHertz, changeKiloHertz, changeHertz);
    }
  } 
}
