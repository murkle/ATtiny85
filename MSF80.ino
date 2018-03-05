/* Radio Clock

   Michael Borcherds March 2018

   Adapted from http://www.technoblogy.com/list?ORG by David Johnson-Davies

   Requires TimerOne library with ATtiny85 support: https://github.com/PaulStoffregen/TimerOne

   Connect 8-digit 7-segment display to PB0, PB1, PB2

   Connect MAS6180C AM receiver "TCON" or "T" to PB3

   Connect pin "P1" or "PON" to 0V (GND) to turn receiver on

   Tested with this one: https://universal-solder.com/product/60khz-wwvb-atomic-radio-clock-receiver-replaces-c-max-cmmr-6p-60/
   Datasheet: http://canaduino.ca/downloads/60khz.pdf

   Optional: LED on PB4

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/
*/


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

#include <TimerOne.h>

//Create global variables
volatile uint8_t BitCount;
unsigned char BitBuffer;
boolean Parity;
uint32_t BitBsum;
uint32_t BitB;
char digit;
uint32_t RadioHours, RadioMins;

// invalid time to start
uint32_t Hours = 43;
uint32_t Mins = 21;

boolean dot = false;

uint32_t seconds = 0;

const int RadioPin = PB3;

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

uint8_t HEXDIGITS[16] = { ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, HEXA, HEXB, HEXC, HEXD, HEXE, HEXF };

// "rEAdy..."
uint8_t READY[8] = {
    DOT,
    DOT,
    DOT,
    LETTER_Y,
    HEXD,
    HEXA,
    HEXE,
    LETTER_R,
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

void displayDecimalDigit(uint32_t binaryInput) {

    // convert to BCD
    // https://stackoverflow.com/questions/13247647/convert-integer-from-pure-binary-to-bcd
    uint32_t bcdResult = 0;
    uint32_t shift = 0;

    while (binaryInput > 0) {
        bcdResult |= (binaryInput % 10) << (shift++ << 2);
        binaryInput /= 10;
    }

    displayHexDigit(bcdResult, dot);


}

void displayHexDigit(uint32_t digit, boolean dot) {
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

    if (dot) {
        hexdigit[7] |= DOT;
    }

    image(hexdigit);
    update_display();
}

// blink n times
void blink(uint8_t n) {

    for (uint8_t i = 0 ; i < n ; i++) {
        digitalWrite(PB4, HIGH);
        delay(200);
        digitalWrite(PB4, LOW);
        delay(200);
    }

}

void handleInterrupt() {

    // read bits (use average of several)
    Average();

    // copy radio output to option LED on PB3
    uint8_t a = digitalRead(PB3);
    if (a == 0) {
        digitalWrite(PB4, LOW);
    } else {
        digitalWrite(PB4, HIGH);
    }

}

void setup() {

    pinMode(PB3, INPUT);
    pinMode(PB4, OUTPUT);

    blink(2);

    // timed interrupt every 2000ms
    Timer1.initialize(2000);
    Timer1.attachInterrupt(handleInterrupt);




    max7219_init();

    image(READY);
    update_display();
    delay(500);

    blink(3);

}

boolean firstMarkerFound = false;


void loop() {

    int bitbs, timeParity, LastHours, LastMins;

    int h;
    // Wait for minute marker (500ms, accept anything over 450)
    while ((h = HighPulse()) < 450) {

        //if (!firstMarkerFound) {
        // display (high) pulse length while waiting
        displayDecimalDigit(h);
        //}
    }

    firstMarkerFound = true;

    seconds = 1;

    // Skip seconds 1 to 16
    // TODO: what about leap seconds?!
    for (int i=1; i<=16; i++) {
        boolean bit = GetBit();
        //displayDecimalDigit((bit?100000:0)+(RadioMins * 100) + (i+1));
    }

    // Read time
    LastHours = RadioHours;
    LastMins = RadioMins;
    Parity = 0;
    BitBsum = 0;

    Parity = 0;
    uint32_t year = GetBit() * 80 + GetBit() * 40 + GetBit() * 20 + GetBit() * 10 + GetBit() * 8 + GetBit() * 4 + GetBit() * 2 + GetBit();
    uint32_t yearParity = Parity;

    Parity = 0;
    uint32_t month = GetBit() * 10 + GetBit() * 8 + GetBit() * 4 + GetBit() * 2 + GetBit();
    uint32_t date = GetBit() * 20 + GetBit() * 10 + GetBit() * 8 + GetBit() * 4 + GetBit() * 2 + GetBit();
    uint32_t dateParity = Parity;

    Parity = 0;
    // Sunday = 0
    uint32_t day = GetBit() * 4 + GetBit() * 2 + GetBit();
    uint32_t dayParity = Parity;

    Parity = 0;
    int TenHours = GetBit() * 2 + GetBit();
    //displayDecimalDigit(TenHours);
    int UnitHours = GetBit() * 8 + GetBit() * 4 + GetBit() * 2 + GetBit();
    //displayDecimalDigit(UnitHours*10000);
    RadioHours = TenHours*10 + UnitHours;
    int TenMins = GetBit() * 4 + GetBit() * 2 + GetBit();
    //displayDecimalDigit(TenMins);
    int UnitMins = GetBit() * 8 + GetBit() * 4 + GetBit() * 2 + GetBit();
    //displayDecimalDigit(UnitMins);
    RadioMins = TenMins*10 + UnitMins;
    timeParity = Parity;
    bitbs = BitBsum; // Should be zero

    boolean valid = true;

    // minute marker
    valid = valid && (GetBit() == 0);

    // Summer Time warning
    valid = valid && (GetBit() == 1);
    // summerWarn = BitB;

    // year parity
    valid = valid && (GetBit() == 1);
    valid = valid && (yearParity != BitB);

    // date parity
    valid = valid && (GetBit() == 1);
    valid = valid && (dateParity != BitB);

    // day parity
    valid = valid && (GetBit() == 1);
    valid = valid && (dayParity != BitB);

    // time parity
    valid = valid && (GetBit() == 1);
    valid = valid && (timeParity != BitB);


    if (valid) {
        Hours = RadioHours;
        Mins = RadioMins;
        dot = true;
    } else {
        Mins = Mins + 1;
        Hours = (Hours + Mins/60) % 24;
        Mins = Mins % 60;
        dot = false;
    }

// displayDecimalDigit(year * 1000000 + month * 10000 + date * 100 + day);
// delay(1500);

}

// Buffers last 8 states of radio output (16msec)
void Average () {
    boolean Bit = digitalRead(RadioPin);
    BitCount = BitCount - ((BitBuffer & 0x80) >> 7) + Bit;
    BitBuffer = (BitBuffer<<1) | Bit;
}

// Returns true if averaged radio input is high
boolean Radio() {
    boolean b;
    //uint8_t oldSREG = SREG;
    //cli();
    b = BitCount >= 4;
    //SREG = oldSREG;
    return b;
}

// Wait for next carrier off pulse (HIGH) and time it
int HighPulse() {
    unsigned long Start;
    while (!Radio()); // Low - carrier on
    Start = millis();
    while (Radio()); // High - carrier off
    return millis() - Start;
}

// Reads Bits A & B from the timecode; assumes you started in inter-byte gap
// BitA is returned
// BitB is put in global variable
boolean GetBit() {

    seconds++;
    if (seconds > 59) {
        seconds = 0;
    }

    boolean result;
    while (!Radio());

    uint32_t start = millis();

    boolean bit0, bit1, bit2, bit3, bit4, bit5, bit6, bit7;

    // length of bits in ms
    // might need to adjust this slightly if ATtiny85 runs too fast/slow
    uint32_t del = 100;

    //delay();
    bit0 = 1;
    //delay(del + del/2);
    while ((millis() - start) < del + del/2);

    bit1 = Radio();
//  delay(del);
    while ((millis() - start) < 2*del + del/2);

    bit2 = Radio();
    //delay(del);
    while ((millis() - start) < 3*del + del/2);

    bit3 = Radio();
    //delay(del);
    while ((millis() - start) < 4*del + del/2);

    bit4 = Radio();
    //delay(del);
    while ((millis() - start) < 5*del + del/2);

    bit5 = 0;//Radio();
    //delay(del);
    //while ((millis() - start) < 6*del + del/2);

    bit6 = 0;//Radio();
    //delay(del);
    //while ((millis() - start) < 7*del + del/2);

    bit7 = 0;//Radio();
//  delay(del);

// for debugging, shows raw BitA and BitB
//uint32_t disp = (bit0 ? 10000 : 0) + (bit1 ? 100000 : 0) + (bit2 ? 1000000 : 0) + (bit3 ? 10000000 : 0) + (seconds) + (RadioMins * 100);

    uint32_t disp = Mins + (Hours * 100);

    //displayDecimalDigit((bit0 ? 1 : 0) + (bit1 ? 10 : 0) + (bit2 ? 100 : 0) + (bit3 ? 1000 : 0) + (bit4 ? 10000 : 0) + (bit5 ? 100000 : 0) + (bit6 ? 1000000 : 0) + (bit7 ? 10000000 : 0));
    displayDecimalDigit(disp);

    BitB = bit2;
    BitBsum += BitB;
    Parity ^= bit1;
    return bit1;

}

