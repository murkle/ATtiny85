#define CLK_HIGH()  PORTB |= (1<<PB4)
#define CLK_LOW()   PORTB &= ~(1<<PB4)
#define CS_HIGH()   PORTB |= (1<<PB1)
#define CS_LOW()    PORTB &= ~(1<<PB1)
#define DATA_HIGH() PORTB |= (1<<PB3)
#define DATA_LOW()  PORTB &= ~(1<<PB3)
// DDRB = data direction register for Port B
// 0 = Input
// 1 = Output
#define INIT_PORT() DDRB |= (1<<PB3) | (1<<PB1) | (1<<PB4)



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

uint8_t test1[8] = {
    LETTER_H,
    LETTER_I,
    LETTER_J,
    LETTER_P,
    LETTER_R,
    LETTER_U,
    LETTER_Y,
    HEXF
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

void displayHexDigit(uint32_t digit, boolean dot, boolean dot2) {
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
    if (dot2) {
        hexdigit[6] |= DOT;
    }

    image(hexdigit);
    update_display();
}
void displayDecimalDigit(uint32_t binaryInput, boolean dot, boolean dot2) {

    // convert to BCD
    // https://stackoverflow.com/questions/13247647/convert-integer-from-pure-binary-to-bcd
    uint32_t bcdResult = 0;
    uint32_t shift = 0;

    while (binaryInput > 0) {
        bcdResult |= (binaryInput % 10) << (shift++ << 2);
        binaryInput /= 10;
    }

    displayHexDigit(bcdResult, dot, dot2);


}


