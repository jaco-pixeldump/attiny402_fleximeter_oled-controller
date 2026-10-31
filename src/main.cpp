#include <Arduino.h>
#include <avr/pgmspace.h>
#include "ssd1306.h"
#include "adc_reader.h"

// =========================================================================
// ATtiny402 Fleximeter OLED Display Monitor
// -------------------------------------------------------------------------
// Pinout:
// Pin 1: VDD (3.3V or 5V)
// Pin 2: PA6 (AIN6) -> Flex sensor voltage divider junction
// Pin 3: PA7 (Unused / Spare GPIO)
// Pin 4: PA1 (SDA)  -> SSD1306 SDA
// Pin 5: PA2 (SCL)  -> SSD1306 SCL
// Pin 6: PA0 (UPDI) -> SerialUPDI programmer
// Pin 7: PA3 (Unused / Spare GPIO)
// Pin 8: GND
// =========================================================================

// Calibration for 3x3cm pad sensor (open/infinite R at rest -> drops to ~10k at ~45° bend):
// With a 4.7k pull-down resistor:
//   - At rest (open):   V_out = 0V   -> ADC ~ 0..20   (0% bend)
//   - At 45° (~10k):    V_out ~ 1.0V -> ADC ~ 320..340 (100% bend)
// (If using 10k pull-down instead: change FLEX_CAL_MAX to 520)
#define FLEX_CAL_MIN 20
#define FLEX_CAL_MAX 330

static uint8_t calculate_percent(uint16_t adc) {
    if (adc <= FLEX_CAL_MIN) return 0;
    if (adc >= FLEX_CAL_MAX) return 100;
    return (uint8_t)(((uint32_t)(adc - FLEX_CAL_MIN) * 100UL) / (FLEX_CAL_MAX - FLEX_CAL_MIN));
}

void setup() {
    // 1. Initialize hardware peripherals
    adc_init();
    ssd1306_init();

    // 2. Clear entire display
    ssd1306_clear();

    // 3. Render static UI layout once to conserve CPU cycles & bus bandwidth
    #if SSD1306_HEIGHT == 64
    // Page 0: Title banner
    ssd1306_set_cursor(0, 10);
    ssd1306_print_p(PSTR("* ATtiny402 FLEX *"));

    // Page 2: ADC measurement label
    ssd1306_set_cursor(2, 6);
    ssd1306_print_p(PSTR("ADC :"));

    // Page 4: Bend percentage label
    ssd1306_set_cursor(4, 6);
    ssd1306_print_p(PSTR("BEND:"));

    // Page 7: Gauge scale ticks
    ssd1306_set_cursor(7, 10);
    ssd1306_print_p(PSTR("FLAT           BENT"));

    #else // 128x32 compact layout (4 pages: 0, 1, 2, 3)
    // Page 0: Label
    ssd1306_set_cursor(0, 0);
    ssd1306_print_p(PSTR("FLEX:"));

    // Page 3: Scale ticks under the bar
    ssd1306_set_cursor(3, 10);
    ssd1306_print_p(PSTR("FLAT           BENT"));
    #endif
}

void loop() {
    // Read smoothed 10-bit ADC value from PA6
    uint16_t adc_val = adc_read_averaged();
    uint8_t percent = calculate_percent(adc_val);

    #if SSD1306_HEIGHT == 64
    // Update numeric values (in-place update with fixed width)
    ssd1306_set_cursor(2, 42);
    ssd1306_print_u16_padded(adc_val, 4);
    ssd1306_print_p(PSTR(" / 1023"));

    ssd1306_set_cursor(4, 42);
    ssd1306_print_u16_padded(percent, 3);
    ssd1306_print_p(PSTR(" %"));

    // Update real-time progress bar on Page 6 (100 pixels wide)
    ssd1306_draw_bar(6, 13, 100, percent);

    #else // 128x32 layout
    ssd1306_set_cursor(0, 36);
    ssd1306_print_u16_padded(percent, 3);
    ssd1306_print_p(PSTR("% (ADC "));
    ssd1306_print_u16_padded(adc_val, 4);
    ssd1306_print_p(PSTR(")"));

    ssd1306_draw_bar(2, 13, 100, percent);
    #endif

    delay(40); // ~25 FPS refresh rate
}
