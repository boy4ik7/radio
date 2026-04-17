#define BAT_PIN A0 //пін дільника
#define CHAR_PIN 5 //пін індекатора заряджання
#define CHAR_DONE_PIN 6 // пін індикатора закінчення заряджання
#include <Arduino.h>
#include <EEPROM.h>
#include <GyverOLED.h>
//GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
#include <EncButton.h>
EncButton eb(2, 3, 4); // піни енкодера (вниз, вгору, кнопка)
#include <RDA5807.h>
RDA5807 rx;
#include <GTimer.h>
GTimer<millis> tmr1;
GTimer<millis> show_tmr;

int16_t bat, freq, vol;
bool oled_power = true, eng_mode = false, charging, charging_done;
float voltage;
const float VREF = 1.10; //коефіцієнт = реальне/виміряне ардуїною
//дільник напруги
const float R1 = 330000.0; //Ом
const float R2 = 100000.0; //Ом

//char *rdsMsg;
char *stationName;
char *rdsTime;

//іконка батареї
const uint8_t bat25[] PROGMEM = {
0x3F, 0x21, 0x2D, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21,
0x3F, 0x0C
};
const uint8_t bat50[] PROGMEM = {
0x3F, 0x21, 0x2D, 0x21, 0x2D, 0x21, 0x21, 0x21, 0x21, 0x21,
0x3F, 0x0C
};
const uint8_t bat75[] PROGMEM = {
0x3F, 0x21, 0x2D, 0x21, 0x2D, 0x21, 0x2D, 0x21, 0x21, 0x21,
0x3F, 0x0C
};
const uint8_t bat100[] PROGMEM = {
0x3F, 0x21, 0x2D, 0x21, 0x2D, 0x21, 0x2D, 0x21, 0x2D, 0x21,
0x3F, 0x0C
};
const uint8_t  bat_charging[] PROGMEM=
{
0x3F, 0x31, 0x29, 0x29, 0x29, 0x2D, 0x25, 0x25, 0x25, 0x23,
0x3F, 0x0C
};
const uint8_t bat_charging_done[] PROGMEM = {
0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
0x3F, 0x0C
};
//іконка динаміка
const uint8_t vol_ico[] PROGMEM= {
0x1C, 0x3E, 0x7F, 0x00, 0x22, 0x1C
};

float readBatteryVoltage() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(BAT_PIN);
    delay(5);
  }
  float adc = sum / 20.0;
  float v = adc * (VREF / 1023.0);
  float batteryVoltage = v * ((R1 + R2) / R2);
  return batteryVoltage;
}

int voltageToPercent(float v) {
  if (v >= 4.2) return 100;
  if (v <= 3.2) return 0;
  return (int)((v - 3.2) / (4.2 - 3.2) * 100);
}

void show_bat() {
  oled.setScale(1);
  charging = !digitalRead(CHAR_PIN);
  charging_done = !digitalRead(CHAR_DONE_PIN);
  if (charging) oled.drawBitmap(1, 1, bat_charging, 12, 6, BITMAP_NORMAL, BUF_ADD);
  if (charging_done) oled.drawBitmap(1, 1, bat_charging_done, 12, 6, BITMAP_NORMAL, BUF_ADD);
  if (charging == false && charging_done == false) {
    voltage = readBatteryVoltage();
    bat = voltageToPercent(voltage);
    if (bat > 75) oled.drawBitmap(1, 1, bat100, 12, 6, BITMAP_NORMAL, BUF_ADD);
    else if (bat > 50) oled.drawBitmap(1, 1, bat75, 12, 6, BITMAP_NORMAL, BUF_ADD);
    else if (bat > 25) oled.drawBitmap(1, 1, bat50, 12, 6, BITMAP_NORMAL, BUF_ADD);
    else if (bat >= 0) oled.drawBitmap(1, 1, bat25, 12, 6, BITMAP_NORMAL, BUF_ADD);
    if (eng_mode) {
      oled.setCursorXY(14, 1);
      oled.print(bat);
      oled.print("% ");
      oled.print(voltage);
      oled.print("v");
      oled.print("  ");
    }
  }
}

void show_freq() {
  oled.setScale(2);
  freq = rx.getFrequency();
  int16_t one, two;
  one = freq / 100;
  two = freq % 100 / 10;
  oled.setCursorXY(18, 20);
  if (one < 100) oled.print(" ");
  oled.print(one);
  oled.print(".");
  oled.print(two);
  oled.setCursorXY(82, 20);
  oled.print("MHz");
}

void show_rssi() { 
  oled.setCursorXY(1, 56);
  if (eng_mode) oled.print("RSSI:");
  oled.print(rx.getRssi());
  oled.print(" dbuv   ");
}

void show_rds() {
  oled.setScale(1);
  if (rx.getRdsReady()) {
    oled.setScale(1);
    oled.setCursorXY(40, 40);
    //if ( (rdsMsg = rx.getRdsText2A()) != NULL) {
      //Serial.println(rdsMsg);
    //}
    if ( (stationName = rx.getRdsText0A()) != NULL) {
      oled.print(stationName);
      oled.print("     ");
    }
    if ( (rdsTime = rx.getRdsTime()) != NULL ) {
      oled.setCursorXY(99, 1);
      oled.print(rdsTime);
    }
  }
}

void show_vol() {
  oled.setScale(1);
  vol = rx.getVolume();
  oled.drawBitmap(110, 56, vol_ico, 6, 7, BITMAP_NORMAL, BUF_ADD);
  oled.setScale(1);
  oled.setCursorXY(117, 56);
  oled.print(vol);
  oled.print(" ");
}

void setup() {
  analogReference(INTERNAL);
  //Serial.begin(9600);
  oled.init();
  oled.setContrast(120); 
  oled.clear();
  //oled.update();
  rx.setup();
  rx.setMono(true);
  rx.setRDS(false);
  rx.setBass(false);
  EEPROM.get(0, freq);
  delay(50);
  if (freq > 10800 || freq < 8750) freq = 8750;
  EEPROM.get(5, vol);
  delay(50);
  if (vol > 15 || freq < 0) freq = 5;
  rx.setVolume(vol);
  rx.setFrequency(freq);
  rx.setSeekThreshold(30);
  delay(100);
  //oled.setPower(true);
  //oled.flipH(true);
  //oled.flipV(true);
  pinMode(CHAR_PIN, INPUT_PULLUP);
  pinMode(CHAR_DONE_PIN, INPUT_PULLUP);
  tmr1.setMode(GTMode::Timeout);
  tmr1.setTime(120000);
  show_tmr.setMode(GTMode::Interval);
  show_tmr.setTime(1000);
  show_bat();
  show_freq();
  show_rds();
  show_rssi();
  show_vol();
  delay(150);
  tmr1.start();
  show_tmr.start();
}

void loop() {
  if (tmr1) {
    oled_power = !oled_power;
    oled.setPower(oled_power);
    EEPROM.put(0, freq);
    delay(50);
    EEPROM.put(5, vol);
    delay(50);
  }
  eb.tick();
  if (eb.turn() && eb.pressing()) {
    if (oled_power == false) {
      oled_power = true;
      oled.setPower(oled_power);
    }
    if (eb.turn()) {
      vol += 1 * eb.dir();
      if (vol > 15) vol = 15;
      if (vol < 0) vol = 0;
      rx.setVolume(vol);
      show_vol();
    }
    show_tmr.start();
    tmr1.start();
  } 
  if (eb.turn() && !eb.pressing()) {
    if (oled_power == false) {
      oled_power = true;
      oled.setPower(oled_power);
    }
    freq += 10 * eb.dir();
    if (freq > 10800) freq = 10800;
    if (freq < 8750) freq = 8750;
    rx.setFrequency(freq);
    show_freq();
    show_tmr.start();
    tmr1.start();
    //var3 += (eb.fast() ? 5 : 1) * eb.dir();
  }
  if (eb.click()) {
    if (oled_power == false) {
      oled_power = true;
      oled.setPower(oled_power);
    }
    rx.seek(RDA_SEEK_WRAP, RDA_SEEK_UP, show_freq);
    delay(500);
    show_freq();
    show_tmr.start();
    tmr1.start();
  }
  if (eb.hold()) {
    eng_mode = !eng_mode;
    if (oled_power == false) {
      oled_power = true;
      oled.setPower(oled_power);
    }
    oled.clear();
    show_bat();
    show_freq();
    show_vol();
    show_tmr.start();
    tmr1.start();
  }
  if (show_tmr && oled_power) {
    show_bat();
    //show_freq();
    show_rds();
    show_rssi();
    //show_vol();
  }
}
