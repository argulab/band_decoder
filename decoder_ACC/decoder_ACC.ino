/*
 * Conmutador automático de dos antenas para ICOM
 * Arsenio "Enio" Gutiérrez, EA2J, junio de 2021, V1.0
 * URE ** Charla sobre "El Arduino en la Radio", 7 de julio de 2021
 */
 
#include <Wire.h>                                                                                                                             #include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WString.h>

const int pin_relay_hexbeam = 32;
const int pin_relay_aperiodic = 34;
const int button_antennas_mode = 15;
const int bands_input = A1;
const int power_input = A3;

enum power {
    power_off, power_on
};
enum band {
    b0, b6, b10, b15, b20, b30, b40, b80, b160
};
enum antenna_mode {
    automatic, manual_hexbeam, manual_aperiodic
};
enum antenna {
    none, hexbeam, aperiodic
};

power old_active_rig;
band old_active_band;
antenna old_active_antenna;
antenna_mode antenna_mode;

LiquidCrystal_I2C lcd(0x27, 16, 2);

//Recibe la lectura analógica de la tensión de BANDA y devuelve la banda de trabajo

band decode_band(int analog_band_reading) {
  if (analog_band_reading <= 100)
      return b30;
  if (analog_band_reading > 200 && analog_band_reading <= 230)
      return b6; //raw = 218, 1,07V
  if (analog_band_reading > 231 && analog_band_reading <= 330)
      return b10; //raw = 259, 1,27V
  if (analog_band_reading > 331 && analog_band_reading <= 430)
      return b15; //raw = 371, 1,81V
  if (analog_band_reading > 431 && analog_band_reading <= 530)
      return b20; //raw = 474, 2,32V
  if (analog_band_reading > 531 && analog_band_reading <= 630)
      return b40; //raw = 591, 2,89V
  if (analog_band_reading > 631 && analog_band_reading <= 730)
      return b80; //raw = 707, 3,46V
  if (analog_band_reading > 731)
      return b160; //raw = 864, 4,22V
   return b0;
}

//Recibe la banda de trabajo y devuelve una cadena con el nombre de la banda

String name_of_band(band band) {
  switch (band) {
    case b0: return "NULL";
    case b6: return "6m";
    case b10: return "10m12m";
    case b15: return "15m17m";
    case b20: return "20m";
    case b30: return "30m";
    case b40: return "40m";
    case b80: return "80m";
    case b160: return "160m";
    }
}

//Recibe la antena activa y devuelve una cadena con el nombre de la antena

String name_of_antenna(antenna antenna) {
  switch (antenna) {
    case hexbeam: return "HEXBEAM";
    case aperiodic: return "APERIODIC";
    case none: return "NONE";
  }
}

//Cambia el modo de conmutación de antena secuencialmente

enum antenna_mode next_antenna_mode(enum antenna_mode mode) {
  switch (mode) {
    case automatic: return manual_hexbeam;
    case manual_hexbeam: return manual_aperiodic;
    case manual_aperiodic: return automatic;
  }
}

//Realiza 100 lecturas de la entrada analógica y devuelve el promedio truncado

int read_analog_io(int pin) {
  long accum = 0;
  for (int i = 1; i <= 100; i++)
    accum += analogRead(pin);
  return accum / 100;
}

//Informa si el transceptor está encendido o apagado

power check_active_rig() {
  return read_analog_io(power_input) > 400 ? power_on : power_off;
}

//Devuelve la banda activa

band check_active_band() {
  return decode_band(read_analog_io(bands_input));
}

//Devuelve la antena que corresponde a la banda activa

antenna check_active_antenna(band band) {
  if (band == b6 || band == b10 || band == b15 || band == b20)
    return hexbeam;
  if (band == b30 || band == b40 || band == b80 || band == b160)
    return aperiodic;
  return none;
}

//Indica si se ha pulsado un botón

bool button_pressed(int pin) {
  delay(250);
  return digitalRead(pin) == HIGH;
}

//Muestra en el display la banda activa

void show_active_band(band band) {
  lcd.setCursor(0, 0);
  lcd.print("BAND=           ");
  lcd.setCursor(5, 0);
  lcd.print(name_of_band(band));
}

//Muestra en el display la antena activa

void show_active_antenna(antenna antenna) {
  lcd.setCursor(0, 1);
  lcd.print("ANT=            ");
  lcd.setCursor(4, 1);
  lcd.print(name_of_antenna(antenna));
}

//Muestra en el display si el transceptor está encendido o apagado

void show_rig_off() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("TRANSCEPTOR OFF ");
}

//Conecta y desconecta los relés correspondientes a las antenas activas y pasivas

void process_relays(antenna antenna) {  
  digitalWrite(pin_relay_hexbeam, antenna == hexbeam);
  digitalWrite(pin_relay_aperiodic, antenna == aperiodic);
}

//Ejecuta los procesos de encendido o apagado cuando el transceptor cambia de estado

void service_rig(power power) {
  band active_band;
  antenna active_antenna;
  if (power == power_off) {
    active_band = b0;
    active_antenna = none;
  } else {
    active_band = check_active_band();
    active_antenna = check_active_antenna(active_band);
  }

  if (power == power_off) {
    show_rig_off();
  } else {
    show_active_band(active_band);
    show_active_antenna(active_antenna);
  }

  old_active_band = active_band;
  old_active_antenna = active_antenna;
  old_active_rig = power;
  antenna_mode = automatic;
  process_relays(active_antenna);
}

//Ejecuta los procesos correspondientes a un cambio de banda

void service_bands(band band) {
  antenna active_antenna = check_active_antenna(band);
  old_active_band = band;
  show_active_band(band);

  if (active_antenna != old_active_antenna) {
    old_active_antenna = active_antenna;
    show_active_antenna(active_antenna);
    process_relays(active_antenna);
  }
}

//Ejecuta los procesos correspondientes a un cambio de antena en el modo manual

void service_antennas_mode() {
  process_relays(antenna_mode == manual_hexbeam ? hexbeam : aperiodic);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MODO MANUAL");
  lcd.setCursor(0, 1);
  lcd.print(antenna_mode == manual_hexbeam ? "HEXBEAM    " : "APERIODIC  ");
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(50);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(pin_relay_hexbeam, OUTPUT);
  pinMode(pin_relay_aperiodic, OUTPUT);
  pinMode(button_antennas_mode);

//Ejecuta los procesos iniciales

  power active_rig = check_active_rig();
  band active_band;
  antenna active_antenna;

  if (active_rig == power_on) {
    delay(4000);
    active_band = check_active_band();
    active_antenna = check_active_antenna(active_band);
    show_active_band(active_band);
    show_active_antenna(active_antenna);
  } else {
    active_band = b0;
    active_antenna = none;
    show_rig_off();
  }

  old_active_rig = active_rig;
  old_active_band = active_band;
  old_active_antenna = active_antenna;
  antenna_mode = automatic;

  process_relays(active_antenna);
}

void loop() {
  int raw = read_analog_io (bands_input);  
    power active_rig = check_active_rig();
    if (active_rig != old_active_rig) {
      service_rig(active_rig);
    }

    if (active_rig == power_on && antenna_mode == automatic) {
      band active_band = check_active_band();
      if (active_band != old_active_band) {
        service_bands(active_band);
      }
    }

    if (active_rig == power_on && button_pressed(button_antennas_mode) == HIGH) {
      antenna_mode = next_antenna_mode(antenna_mode);
    if (antenna_mode == automatic) {
      band active_band = check_active_band();
      antenna active_antenna = check_active_antenna(active_band);
      show_active_band(active_band);
      show_active_antenna(active_antenna);
      process_relays(active_antenna);
    } else {
      service_antennas_mode();
    }
  }
}
