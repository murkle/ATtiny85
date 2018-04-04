#include "TinyWireM.h"        // I2C library for ATtiny AVR
#include "nunchuck_funcs.h"   // Wii Nunchuck helper functions
#include "max7219.h"

// Useful pins definitions
#define LASERPIN PB1            // Laser module signal pin

// X axis values for nunchuck's joystick. You may adapt these for your own nunchuck
#define JOYXLEFT   34         // X Axis - Full left value
#define JOYXCENTER 133        // X Axis - Center value
#define JOYXRIGHT  230        // X Axis - Full right value

// Y axis values for nunchuck's joystick. You may adapt these for your own nunchuck
#define JOYYDOWN   36         // Y Axis - Full down value
#define JOYYCENTER 128        // Y Axis - Center value
#define JOYYUP     220        // Y Axis - Full up value


// nunchuck pins
// GND (-)
// VCC (+)
// SDA (d) -> PB0 with 4.7k pull-up resistor
// SCL (c) -> PB2 with 4.7k pull-up resistor

// max7219 pins (7-segment 8-digit display)
// DIN -> PB3
// CS -> PB1
// CLK -> PB4

// Nunchuck useful variables
byte joyx, joyy, zbut, cbut;

// approx delay, needs "tuning"
void myDelay (unsigned long millis) {
  for (volatile unsigned long i = 300*millis; i!=0; i--);
}


void blink(int n) {

  for (int i = 0 ; i < n ; i++) {
    digitalWrite(LASERPIN, HIGH);
  myDelay(200);
  digitalWrite(LASERPIN, LOW);
  myDelay(100);
  }

  myDelay(400);
  
}

void setup() {

  max7219_init();
  
  // Nunchuck init
  nunchuck_init();
  
  image(READY);
    update_display();
    myDelay(1000);
  
}

void loop() {
  // Get nunchuck data
  nunchuck_get_data();
  
  joyx = nunchuck_joyx();
  joyy = nunchuck_joyy();
  zbut = nunchuck_zbutton();
  cbut = nunchuck_cbutton();

  int32_t x = joyx;
  int32_t y = joyy;

  displayDecimalDigit(x+(y*10000), cbut == 1, zbut == 1);
  
  if (joyx == JOYXCENTER) {
    //
  } else if (joyx < JOYXCENTER) {
    //
  } else {
    //
  }

  // Move tilt servo
  if (joyy == JOYYCENTER) {
    //
  } else if (joyy < JOYYCENTER) {
    //
  } else {
    //
  }
  
  myDelay(100);
}
