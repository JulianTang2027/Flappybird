#include "speaker.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#define SPEAKER_PIN 0

// speaker pin
void speaker_init(void) {
    nrf_gpio_cfg_output(SPEAKER_PIN);
    nrf_gpio_pin_clear(SPEAKER_PIN);
}

// 
// blocking
//
//!
//!!
//!!!
//!!!!
//!!!!!
//!!!!!!

void speaker_play_tone(uint16_t frequency, uint16_t duration_ms) {
    if (frequency == 0) return; // Avoid division by zero

    //no of cycles to produce for the given duration.
    uint32_t cycles = (duration_ms * frequency) / 1000;
    //half period in microseconds (for high and low phases)
    uint32_t half_period_us = 1000000 / (frequency * 2);

    for (uint32_t i = 0; i < cycles; i++) {
        nrf_gpio_pin_set(SPEAKER_PIN);
        nrf_delay_us(half_period_us);
        nrf_gpio_pin_clear(SPEAKER_PIN);
        nrf_delay_us(half_period_us);
    }
}
// BLOCKING DECREASE TIME
void speaker_play_jump_melody(void) {
    speaker_play_tone(523, 30);  
    nrf_delay_ms(10);            
    speaker_play_tone(659, 30);  
}

void speaker_play_game_over_melody(void) {
    speaker_play_tone(293, 150);  
    nrf_delay_ms(30);
    speaker_play_tone(261, 150); 
    nrf_delay_ms(30);
    speaker_play_tone(220, 300);  
}
