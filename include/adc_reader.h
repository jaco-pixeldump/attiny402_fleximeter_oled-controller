#pragma once
#include <avr/io.h>

// =========================================================================
// Hyper-Optimized ADC Driver for ATtiny402 (MegaAVR 0-Series)
// Zero Library Overhead - Direct Register Configuration
// =========================================================================

// Default Analog Channel: PA6 = AIN6 (Physical Pin 2)
// Other available analog pins on ATtiny402:
//   PA7 = AIN7 (Physical Pin 3) -> ADC_MUXPOS_AIN7_gc
//   PA3 = AIN3 (Physical Pin 7) -> ADC_MUXPOS_AIN3_gc
#ifndef FLEX_ADC_PIN
#define FLEX_ADC_PIN 6
#endif

static inline void adc_init(void) {
    #if FLEX_ADC_PIN == 6
    // Disable digital input buffer on PA6 to eliminate digital switching noise & current leakage
    PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    ADC0.MUXPOS = ADC_MUXPOS_AIN6_gc;
    #elif FLEX_ADC_PIN == 7
    PORTA.PIN7CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;
    #elif FLEX_ADC_PIN == 3
    PORTA.PIN3CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    ADC0.MUXPOS = ADC_MUXPOS_AIN3_gc;
    #endif

    // CTRLC: Reference = VDD, Prescaler = CLK_PER / 16 (1.25 MHz @ 20MHz clock)
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV16_gc;

    // CTRLA: 10-bit resolution (RESSEL=0), Enable ADC
    ADC0.CTRLA = ADC_ENABLE_bm;
}

static inline uint16_t adc_read_single(void) {
    ADC0.COMMAND = ADC_STCONV_bm; // Start conversion
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)); // Wait until result is ready
    ADC0.INTFLAGS = ADC_RESRDY_bm; // Clear result ready flag
    return ADC0.RES;
}

// 16x oversampling for noise rejection, smooth display values & stability
static inline uint16_t adc_read_averaged(void) {
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 16; i++) {
        sum += adc_read_single();
    }
    return (sum + 8) >> 4; // Rounded division by 16
}
