#pragma once
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "font5x7.h"

// =========================================================================
// Hyper-Optimized SSD1306 OLED Driver for ATtiny402 (MegaAVR 0-Series)
// Zero-RAM Framebuffer (Direct-to-Display Streaming)
// Direct Hardware TWI0 Register Control (Zero external library overhead)
// =========================================================================

#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x3C
#endif

#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT 64 // Default: 64 (128x64). Set to 32 for SSD1306 128x32 (12832)
#endif

#define SSD1306_WIDTH 128
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)
#define SSD1306_WRITE_ADDR (SSD1306_I2C_ADDR << 1)

// SSD1306 Initialization sequence in PROGMEM
static const uint8_t ssd1306_init_seq[] PROGMEM = {
    0xAE,                 // Display OFF
    0xD5, 0x80,           // Clock divide ratio / osc freq
    0xA8, (SSD1306_HEIGHT == 32 ? 0x1F : 0x3F), // Multiplex ratio (0x1F for 32 lines, 0x3F for 64 lines)
    0xD3, 0x00,           // Display offset 0
    0x40,                 // Start line 0
    0x8D, 0x14,           // Enable charge pump
    0x20, 0x02,           // Page Addressing Mode
    0xA1,                 // Segment remap (column 127 mapped to SEG0)
    0xC8,                 // COM output scan direction (remapped)
    0xDA, (SSD1306_HEIGHT == 32 ? 0x02 : 0x12), // COM pins config (0x02 sequential for 32, 0x12 alternative for 64)
    0x81, (SSD1306_HEIGHT == 32 ? 0x8F : 0xCF), // Contrast
    0xD9, 0xF1,           // Pre-charge period
    0xDB, 0x40,           // VCOMH deselect level
    0xA4,                 // Resume to RAM content
    0xA6,                 // Normal display (non-inverted)
    0xAF                  // Display ON
};

// -------------------------------------------------------------------------
// Low-Level Hardware TWI0 Functions
// ATtiny402: SDA = PA1 (Pin 4), SCL = PA2 (Pin 5)
// -------------------------------------------------------------------------

static inline void i2c_init(void) {
    // Enable internal pullups on PA1 (SDA) and PA2 (SCL)
    PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
    PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;

    // Set baud rate for 400kHz Fast-mode TWI
    // MBAUD = (F_CPU / (2 * 400000)) - 5
    #if F_CPU >= 20000000L
    TWI0.MBAUD = 20; // 20 MHz / (2 * 400kHz) - 5 = 20
    #elif F_CPU >= 16000000L
    TWI0.MBAUD = 15; // 16 MHz / (2 * 400kHz) - 5 = 15
    #elif F_CPU >= 10000000L
    TWI0.MBAUD = 7;
    #else
    TWI0.MBAUD = 3;
    #endif

    // Enable TWI master
    TWI0.MCTRLA = TWI_ENABLE_bm;

    // Force bus state to IDLE
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

static inline void i2c_start(uint8_t addr) {
    TWI0.MADDR = addr;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
}

static inline void i2c_write(uint8_t data) {
    TWI0.MDATA = data;
    while (!(TWI0.MSTATUS & TWI_WIF_bm));
}

static inline void i2c_stop(void) {
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}

// -------------------------------------------------------------------------
// High-Level Display Functions
// -------------------------------------------------------------------------

static inline void ssd1306_set_cursor(uint8_t page, uint8_t col) {
    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x00); // Command stream
    i2c_write(0xB0 | (page & 0x07));
    i2c_write(0x00 | (col & 0x0F));
    i2c_write(0x10 | ((col >> 4) & 0x0F));
    i2c_stop();
}

static inline void ssd1306_init(void) {
    i2c_init();

    // Stream initialization commands
    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x00); // Command stream
    for (uint8_t i = 0; i < sizeof(ssd1306_init_seq); i++) {
        i2c_write(pgm_read_byte(&ssd1306_init_seq[i]));
    }
    i2c_stop();
}

static inline void ssd1306_clear(void) {
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        ssd1306_set_cursor(page, 0);
        i2c_start(SSD1306_WRITE_ADDR);
        i2c_write(0x40); // Data stream
        for (uint8_t c = 0; c < SSD1306_WIDTH; c++) {
            i2c_write(0x00);
        }
        i2c_stop();
    }
}

static inline void ssd1306_clear_line(uint8_t page) {
    ssd1306_set_cursor(page, 0);
    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x40);
    for (uint8_t c = 0; c < SSD1306_WIDTH; c++) {
        i2c_write(0x00);
    }
    i2c_stop();
}

static inline void ssd1306_draw_char(char c) {
    if (c < 32 || c > 126) c = ' ';
    uint8_t idx = c - 32;

    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x40);
    for (uint8_t i = 0; i < 5; i++) {
        i2c_write(pgm_read_byte(&font5x7[idx][i]));
    }
    i2c_write(0x00); // 1-pixel column spacing
    i2c_stop();
}

static inline void ssd1306_print(const char *str) {
    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x40); // Stream character bitmaps in one transaction
    while (*str) {
        char c = *str++;
        if (c < 32 || c > 126) c = ' ';
        uint8_t idx = c - 32;
        for (uint8_t i = 0; i < 5; i++) {
            i2c_write(pgm_read_byte(&font5x7[idx][i]));
        }
        i2c_write(0x00);
    }
    i2c_stop();
}

static inline void ssd1306_print_p(const char *pstr) {
    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x40);
    char c;
    while ((c = pgm_read_byte(pstr++)) != 0) {
        if (c < 32 || c > 126) c = ' ';
        uint8_t idx = c - 32;
        for (uint8_t i = 0; i < 5; i++) {
            i2c_write(pgm_read_byte(&font5x7[idx][i]));
        }
        i2c_write(0x00);
    }
    i2c_stop();
}

// Lightweight integer printing with optional fixed-width right-padding
static inline void ssd1306_print_u16_padded(uint16_t val, uint8_t min_digits) {
    char buf[6];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    uint8_t count = 0;

    if (val == 0) {
        *--p = '0';
        count++;
    } else {
        while (val > 0) {
            *--p = (val % 10) + '0';
            val /= 10;
            count++;
        }
    }

    while (count < min_digits) {
        *--p = ' ';
        count++;
    }

    ssd1306_print(p);
}

// Sleek horizontal bar gauge (width pixels wide + 2 border caps)
// page: 0..7, x: starting column 0..127, width: inner bar width, percent: 0..100
static inline void ssd1306_draw_bar(uint8_t page, uint8_t x, uint8_t width, uint8_t percent) {
    if (percent > 100) percent = 100;
    uint8_t fill = ((uint16_t)width * percent) / 100;

    ssd1306_set_cursor(page, x);
    i2c_start(SSD1306_WRITE_ADDR);
    i2c_write(0x40);

    // Left border cap (0b01111110)
    i2c_write(0x7E);

    // Bar interior: 0x7E for filled, 0x42 (0b01000010) for empty border
    for (uint8_t i = 0; i < width; i++) {
        if (i < fill) {
            i2c_write(0x7E);
        } else {
            i2c_write(0x42);
        }
    }

    // Right border cap
    i2c_write(0x7E);

    i2c_stop();
}
