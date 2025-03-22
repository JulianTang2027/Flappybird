#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "nrfx_spim.h"
#include "microbit_v2.h"  


#define SSD1309_WIDTH   128
#define SSD1309_HEIGHT  64


#define SSD1309_SPI_SCK    EDGE_P13   
#define SSD1309_SPI_MOSI   EDGE_P15   
#define SSD1309_SPI_MISO   -1         // DO NOT CONNECT
#define SSD1309_CS         EDGE_P8    // PIN CONFLICT WITH BUTTON B
#define SSD1309_DC         EDGE_P12   
#define SSD1309_RES        EDGE_P11   

// SSD1309 command definitions
#define SSD1309_CMD_DISPLAY_OFF        0xAE
#define SSD1309_CMD_DISPLAY_ON         0xAF
#define SSD1309_CMD_NORMAL_DISPLAY     0xA6
#define SSD1309_CMD_INVERSE_DISPLAY    0xA7
#define SSD1309_CMD_SET_MULTIPLEX      0xA8
#define SSD1309_CMD_SET_DISPLAY_OFFSET 0xD3
#define SSD1309_CMD_SET_START_LINE      0x40
#define SSD1309_CMD_CHARGE_PUMP        0x8D
#define SSD1309_CMD_MEMORY_MODE        0x20
#define SSD1309_CMD_SEG_REMAP          0xA0  // USE 0XA1  !!!!
#define SSD1309_CMD_COM_SCAN_DEC       0xC8
#define SSD1309_CMD_SET_COM_PINS       0xDA
#define SSD1309_CMD_SET_CONTRAST       0x81
#define SSD1309_CMD_PRE_CHARGE         0xD9
#define SSD1309_CMD_VCOM_DESELECT      0xDB

//  SSD1309 RAM addressing in SPI 
#define SSD1309_CMD_SET_COLUMN_ADDRESS 0x15
#define SSD1309_CMD_SET_ROW_ADDRESS    0x75
#define SSD1309_CMD_WRITE_RAM          0x5C

// Public 
void ssd1309_init(void);
void ssd1309_clear(void);          
void ssd1309_update(void);         // framebuffer to display


void ssd1309_clear_fb(void);
void ssd1309_draw_pixel(uint8_t x, uint8_t y, bool color);
void ssd1309_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);

#endif 







