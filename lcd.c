#include "lcd.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_spim.h"
#include <string.h>

static const nrfx_spim_t spim = NRFX_SPIM_INSTANCE(2);
static nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG;

// each byte covers 8 vertical pixels
// 1024 bytes for ssd1309
static uint8_t framebuffer[SSD1309_WIDTH * (SSD1309_HEIGHT / 8)];

// Helper: control chip-select manually.
static inline void cs_low(void) {
    nrf_gpio_pin_clear(SSD1309_CS);
}
static inline void cs_high(void) {
    nrf_gpio_pin_set(SSD1309_CS);
}



// one command byte DC low
static void ssd1309_send_command(uint8_t cmd) {
    nrf_gpio_pin_clear(SSD1309_DC); // Command 
    cs_low();
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&cmd, 1);
    nrfx_spim_xfer(&spim, &xfer, 0);
    cs_high();
}

// data bytes (DC = high)
static void ssd1309_send_data(uint8_t *data, size_t length) {
    nrf_gpio_pin_set(SSD1309_DC);   // Data 
    cs_low();
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(data, length);
    nrfx_spim_xfer(&spim, &xfer, 0);
    cs_high();
}

void ssd1309_init(void) {
  
    //spim
    spim_config.sck_pin  = SSD1309_SPI_SCK;
    spim_config.mosi_pin = SSD1309_SPI_MOSI;
    spim_config.miso_pin = SSD1309_SPI_MISO; // not used
    spim_config.frequency = NRF_SPIM_FREQ_8M;
    spim_config.mode = NRF_SPIM_MODE_0;
    spim_config.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST;
    spim_config.irq_priority = 0;
    nrfx_spim_init(&spim, &spim_config, NULL, NULL);
    
    // control pins
    nrf_gpio_cfg_output(SSD1309_CS);
    nrf_gpio_cfg_output(SSD1309_DC);
    nrf_gpio_cfg_output(SSD1309_RES);
    cs_high();
    
    //  reset sequence
    nrf_gpio_pin_set(SSD1309_RES);
    nrf_delay_ms(5);
    nrf_gpio_pin_clear(SSD1309_RES);
    nrf_delay_ms(20);
    nrf_gpio_pin_set(SSD1309_RES);
    nrf_delay_ms(150);
    
ssd1309_send_command(SSD1309_CMD_DISPLAY_OFF);

ssd1309_send_command(SSD1309_CMD_SET_MULTIPLEX);
ssd1309_send_command(0x3F); // for 64

ssd1309_send_command(SSD1309_CMD_SET_DISPLAY_OFFSET);
ssd1309_send_command(0x24); //nooffset

ssd1309_send_command(SSD1309_CMD_SET_START_LINE | 0x00);

ssd1309_send_command(SSD1309_CMD_CHARGE_PUMP);
ssd1309_send_command(0x14);

// mem (0x02 = page config)
ssd1309_send_command(SSD1309_CMD_MEMORY_MODE);
ssd1309_send_command(0x02);

// re-map, flipped horizontally in .h, we could have as well done it here

ssd1309_send_command(SSD1309_CMD_SEG_REMAP | 0x00);

ssd1309_send_command(0xC0);

ssd1309_send_command(SSD1309_CMD_SET_COM_PINS);
ssd1309_send_command(0x12);

ssd1309_send_command(SSD1309_CMD_SET_CONTRAST);
ssd1309_send_command(0x7F);

ssd1309_send_command(SSD1309_CMD_PRE_CHARGE);
ssd1309_send_command(0xF1);

ssd1309_send_command(SSD1309_CMD_VCOM_DESELECT);
ssd1309_send_command(0x40);

ssd1309_send_command(SSD1309_CMD_NORMAL_DISPLAY);
ssd1309_send_command(SSD1309_CMD_DISPLAY_ON);

ssd1309_clear_fb();
ssd1309_update();

}

void ssd1309_clear(void) {
    ssd1309_clear_fb();
    ssd1309_update();
}

//  Framebuffer drawing 

// clearing the internal framebuffer
void ssd1309_clear_fb(void) {
    memset(framebuffer, 0x00, sizeof(framebuffer));
}

// drawing a pixel
void ssd1309_draw_pixel(uint8_t x, uint8_t y, bool color) {
    if (x >= SSD1309_WIDTH || y >= SSD1309_HEIGHT) return;
    uint16_t index = x + (y / 8) * SSD1309_WIDTH;
    uint8_t bit = 1 << (y % 8);
    if (color)
        framebuffer[index] |= bit;
    else
        framebuffer[index] &= ~bit;
}

void ssd1309_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color) {
    for (uint8_t i = x; i < x + w && i < SSD1309_WIDTH; i++) {
        for (uint8_t j = y; j < y + h && j < SSD1309_HEIGHT; j++) {
            ssd1309_draw_pixel(i, j, color);
        }
    }
}

void ssd1309_update(void) {
    for (uint8_t page = 0; page < SSD1309_HEIGHT / 8; page++) {
        // current page address (0xB0 to 0xB7)
        ssd1309_send_command(0xB0 | page);
        // column address to 0 (low nibble then high nibble)
        ssd1309_send_command(0x00);  // lower column start address
        ssd1309_send_command(0x10);  // higher column start address

        ssd1309_send_command(SSD1309_CMD_WRITE_RAM);

        // wiritng one full page with frambuffer
        ssd1309_send_data(&framebuffer[page * SSD1309_WIDTH], SSD1309_WIDTH);
    }
}







