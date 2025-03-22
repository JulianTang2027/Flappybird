#include "microphone.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrfx_saadc.h"
#include "microbit_v2.h"
#include "nrf_gpio.h"
#include <stdio.h>
#include <stdlib.h>

// config macros
#define ANALOG_MIC_IN        NRF_SAADC_INPUT_AIN3

#define ADC_MIC_CHANNEL      0
#define ADC_MAX_COUNTS       (1024)   // 10-bit 
#define SAMPLING_FREQUENCY   16000    // 16 kHz 
#define TIMER_TICKS          (16000000 / SAMPLING_FREQUENCY)
#define BUFFER_SIZE          1000     

#define RAW_THRESHOLD        300

static int16_t samples[BUFFER_SIZE] = {0};
volatile bool samples_complete = false;
volatile bool sound_detected = false;
volatile int32_t last_raw_level = 0;


// Forward declaration for SAADC callback
static void saadc_event_callback(nrfx_saadc_evt_t const * event);


void TIMER4_IRQHandler(void) {
    NRF_TIMER4->EVENTS_COMPARE[0] = 0;
    NRF_TIMER4->CC[0] += TIMER_TICKS;
    nrfx_saadc_sample();
}

static void saadc_event_callback(nrfx_saadc_evt_t const * event) {
    if (event->type == NRFX_SAADC_EVT_DONE) {
        // Stops the timer to end the current sampling session
        NRF_TIMER4->CC[0] = 0;
        NRF_TIMER4->TASKS_STOP = 1;

        printf("ADC sampling complete (%d samples)\n", event->data.done.size);

        // max raw ADC value in the buffer
        int16_t max_val = samples[0];
        for (int i = 1; i < BUFFER_SIZE; i++) {
            if (samples[i] > max_val) {
                max_val = samples[i];
            }
        }
        last_raw_level = max_val;

        //printf("MIC ADC DETECTED: %d\n", max_val);

        // Flag sound detected - jump - if the val > threshold
        sound_detected = (max_val > RAW_THRESHOLD);

        samples_complete = true;
    } else {
        printf("Unexpected SAADC event received\n");
    }
}

void sound_detector_init(void) {
    printf("Initializing sound detector\n");
    
    // micled
    nrf_gpio_pin_dir_set(LED_MIC, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg(LED_MIC, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0H1, NRF_GPIO_PIN_NOSENSE);

    nrf_gpio_pin_set(LED_MIC); // LED off initially

    //  SAADC w 10bit resolution
    nrfx_saadc_config_t saadc_config = {
        .resolution = NRF_SAADC_RESOLUTION_10BIT,
        .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
        .interrupt_priority = 1,
        .low_power_mode = false,
    };
    nrfx_saadc_init(&saadc_config, saadc_event_callback);
    

    // lower gain (GAIN1/2) avoids saturating the ADC input
    nrf_saadc_channel_config_t mic_channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ANALOG_MIC_IN);
    mic_channel_config.gain = NRF_SAADC_GAIN1_2;
    mic_channel_config.acq_time = NRF_SAADC_ACQTIME_3US;
    nrfx_saadc_channel_init(ADC_MIC_CHANNEL, &mic_channel_config);
    

    // TIMER4 for periodic SAADC sampling
    NRF_TIMER4->BITMODE = 3;
    NRF_TIMER4->PRESCALER = 0;
    NRF_TIMER4->CC[0] = 0;
    NRF_TIMER4->INTENSET = 1 << TIMER_INTENSET_COMPARE0_Pos;
    
    NVIC_ClearPendingIRQ(TIMER4_IRQn);

    NVIC_SetPriority(TIMER4_IRQn, 7);

    NVIC_EnableIRQ(TIMER4_IRQn);

    
    // first SAADC sampling session
    sample_microphone();
}

// new sampling session
void sample_microphone(void) {
    samples_complete = false;

    nrfx_saadc_buffer_convert(samples, BUFFER_SIZE);
    NRF_TIMER4->TASKS_CLEAR = 1;
    NRF_TIMER4->TASKS_START = 1;

    NRF_TIMER4->CC[0] = TIMER_TICKS;
}

// check status nonblocking
// When a sampling session completes, returns true if the ADC value exceeds the threshold
bool sound_detector_check(void) 
{
    if (samples_complete) {
        bool ret = sound_detected;
        samples_complete = false;
        sample_microphone();
        return ret;
    }

    return false;
}

// last computed normalized ADC level
int32_t sound_detector_get_level(void) 
{
    return last_raw_level;
}
