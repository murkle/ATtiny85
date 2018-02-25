/* ATtiny85 password generator from hardware random numbers

   ATtiny85 @ 8 MHz (internal oscillator; BOD disabled)

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/

   Adapted from https://halfbyteblog.wordpress.com/2015/11/29/using-a-max7219-8x8-led-matrix-with-an-attiny85-like-trinket-or-digispark/
   by Michael Borcherds, Feb 2018

   Needs entropy library from https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library
   which uses "the watchdog timer's natural jitter to produce a reliable stream of true random numbers."

   Generates a 16-character password which can be displayed on an 8-digit 7-segment display
   (password is displayed in 2 parts alternately)

   Each digit has about 4.5 bits of entropy so total password has about 16 * 4.5 = 72 bits

   Connect the 8-digit 7-segment display to PB0 (DIN), PB1 (CS), PB2 (CLK)

*/

#include <Entropy.h>
#include <avr/io.h>
#include <util/delay.h>

#define CLK_HIGH()  PORTB |= (1<<PB2)
#define CLK_LOW()   PORTB &= ~(1<<PB2)
#define CS_HIGH()   PORTB |= (1<<PB1)
#define CS_LOW()    PORTB &= ~(1<<PB1)
#define DATA_HIGH() PORTB |= (1<<PB0)
#define DATA_LOW()  PORTB &= ~(1<<PB0)
// DDRB = data direction register for Port B
// 0 = Input
// 1 = Output
#define INIT_PORT() DDRB |= (1<<PB0) | (1<<PB1) | (1<<PB2)



//
//   _    7
//  | |  2 6
//   _    1
//  | |  3 5
//   _    4
//
uint8_t ZERO =  0b01111110;
uint8_t ONE =   0b00110000;
uint8_t TWO =   0b01101101;
uint8_t THREE = 0b01111001;
uint8_t FOUR =  0b00110011;
uint8_t FIVE =  0b01011011;
uint8_t SIX =   0b01011111;
uint8_t SEVEN = 0b01110000;
uint8_t EIGHT = 0b01111111;
uint8_t NINE =  0b01110011;
uint8_t HEXA =  0b01110111;
uint8_t HEXB =  0b00011111;
uint8_t HEXC =  0b00001101;
uint8_t HEXD =  0b00111101;
uint8_t HEXE =  0b01001111;
uint8_t HEXF =  0b01000111;
uint8_t DOT =   0b10000000;
uint8_t LETTER_H =   0b00010111;
uint8_t LETTER_I =   0b00010000;
uint8_t LETTER_J =   0b00111000;
uint8_t LETTER_L =   ONE;
uint8_t LETTER_O =   ZERO;
uint8_t LETTER_P =   0b01100111;
uint8_t LETTER_R =   0b00000101;
uint8_t LETTER_S =   FIVE;
uint8_t LETTER_U =   0b00011100;
uint8_t LETTER_Y =   0b00111011;
uint8_t CHAR_PLING =   0b10110000;
uint8_t CHAR_MINUS =  0b00000001;
uint8_t CHAR_UNDERSCORE =  0b00001000;


uint8_t HEXDIGITS[16] = { ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, HEXA, HEXB, HEXC, HEXD, HEXE, HEXF };
uint8_t CHARACTERS[28] = { ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, HEXA, HEXB, HEXC, HEXD, HEXE, HEXF,LETTER_H,LETTER_I,LETTER_J,LETTER_P,LETTER_R,LETTER_S,LETTER_U,LETTER_Y, DOT, CHAR_PLING, CHAR_MINUS, CHAR_UNDERSCORE };

// "rEAdy..."
uint8_t ready[8] = {
    DOT,
    DOT,
    DOT,
    LETTER_Y,
    HEXD,
    HEXA,
    HEXE,
    LETTER_R,
};

// "E..rOpy."
uint8_t entropy[8] = {
    DOT,
    LETTER_Y,
    LETTER_P,
    LETTER_O,
    LETTER_R,
    DOT,
    DOT,
    HEXE,
};

// "pASS.Ord"
uint8_t password[8] = {
    HEXD,
    LETTER_R,
    LETTER_O,
    DOT,
    LETTER_S,
    LETTER_S,
    HEXA,
    LETTER_P,
};

// "ErrOr..."
uint8_t error[8] = {
    DOT,
    DOT,
    DOT,
    LETTER_R,
    LETTER_O,
    LETTER_R,
    LETTER_R,
    HEXE,
};

void spi_send(uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8; i++, data <<= 1)
    {
        CLK_LOW();
        if (data & 0x80)
            DATA_HIGH();
        else
            DATA_LOW();
        CLK_HIGH();
    }

}

void max7219_writec(uint8_t high_byte, uint8_t low_byte)
{
    CS_LOW();
    spi_send(high_byte);
    spi_send(low_byte);
    CS_HIGH();
}

void max7219_clear(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        max7219_writec(i+1, 0);
    }
}

void max7219_init(void)
{
    INIT_PORT();
    // Decode mode: none
    max7219_writec(0x09, 0);
    // Intensity: 3 (0-15)
    max7219_writec(0x0A, 1);
    // Scan limit: All "digits" (rows) on
    max7219_writec(0x0B, 7);
    // Shutdown register: Display on
    max7219_writec(0x0C, 1);
    // Display test: off
    max7219_writec(0x0F, 0);
    max7219_clear();
}


uint8_t display[8];

void update_display(void)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        max7219_writec(i+1, display[i]);
    }
}

void image(uint8_t im[8])
{
    uint8_t i;

    for (i = 0; i < 8; i++)
        display[i] = im[i];
}

void set_pixel(uint8_t r, uint8_t c, uint8_t value)
{
    switch (value)
    {
    case 0: // Clear bit
        display[r] &= (uint8_t) ~(0x80 >> c);
        break;
    case 1: // Set bit
        display[r] |= (0x80 >> c);
        break;
    default: // XOR bit
        display[r] ^= (0x80 >> c);
        break;
    }
}

uint8_t display1[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t display2[8] = {0,0,0,0,0,0,0,0};

void displayHexDigit(uint32_t digit, uint32_t timems) {
    uint8_t hexdigit[8] = {
        HEXDIGITS[digit & 0x0f],
        HEXDIGITS[(digit >> 4) & 0x0f],
        HEXDIGITS[(digit >> 8) & 0x0f],
        HEXDIGITS[(digit >> 12) & 0x0f],
        HEXDIGITS[(digit >> 16) & 0x0f],
        HEXDIGITS[(digit >> 20) & 0x0f],
        HEXDIGITS[(digit >> 24) & 0x0f],
        HEXDIGITS[(digit >> 28) & 0x0f],

    };

    image(hexdigit);
    update_display();
    //delay(timems);
}

void setup()
{
    Entropy.initialize();

    // display "rEAdy..." and
    // and flash LED on PB4

    max7219_init();
    pinMode(PB4,OUTPUT);
    digitalWrite(PB4,HIGH);

    image(ready);
    update_display();
    digitalWrite(PB4,LOW);
    delay(250);

    /* for testing, display entropy as hex
      image(entropy);
      update_display();
      delay(1000);

      uint32_t seed = Entropy.random();
      displayHexDigit(seed, 1000);

      seed = Entropy.random();
      displayHexDigit(seed, 1000);

      seed = Entropy.random();
      displayHexDigit(seed, 1000);
      seed = Entropy.random();
      displayHexDigit(seed, 1000);
      seed = Entropy.random();
      displayHexDigit(seed, 1000);
      */

    // generate 16 characters for the password
    for (int i = 0 ; i < 16 ; i++) {

        // make scrolling smooth
        // also gives time to "make" entropy
        delay(200);

        // cool scrolling
        for (int j = 15 ; j >=1 ; j--) {
            display1[j] = display1[j-1];
        }

        while (Entropy.available() == 0) {
            // wait for enough entropy
        }
        display1[0] = CHARACTERS[Entropy.random(0,28)];

        image(display1);
        update_display();

    }

    // split into two 8-digit parts
    for (int i = 0 ; i < 8 ; i++) {
        display2[i] = display1[i+8];
    }



}

void loop() {
    // alternate display between two parts
    image(display1);
    update_display();
    delay(2000);
    image(display2);
    update_display();
    delay(2000);
}

