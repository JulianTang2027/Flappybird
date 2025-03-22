#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "nrf_gpio.h"
#include "lcd.h"
#include "font.h"  // draw_char() and draw_string()
#include "speaker.h"
//#include "res_touch.h"
#include "microphone.h"



///////////////////////////////////////////////
//////////////////////////////////////////////
/////////////////////////////////////////////
////////////////////////////////////////////

// 1-bit background placeholder

static const struct {
    unsigned int     width;
    unsigned int     height;
    unsigned int     bytes_per_pixel;
    unsigned char    pixel_data[128 * 64 * 2];
} background = {
    .width = 128,
    .height = 64,
    .bytes_per_pixel = 2,
    .pixel_data = {
        [0 ... (128*64*2 - 1)] = 0xFF
    }
};


// flappy logic
#define BUTTON_A_PIN     14

#ifndef SSD1309_WIDTH
#define SSD1309_WIDTH    128
#endif

#ifndef SSD1309_HEIGHT
#define SSD1309_HEIGHT   64
#endif

#define FRAME_DELAY_MS   50
#define GRAVITY          0.2f
#define JUMP_VELOCITY    -2.0f

#define GROUND_HEIGHT    4
#define GROUND_Y         (SSD1309_HEIGHT - GROUND_HEIGHT) + 4

// bird to 5×5
#define BIRD_ORIGINAL_WIDTH  5
#define BIRD_ORIGINAL_HEIGHT 5
#define BIRD_WIDTH           5
#define BIRD_HEIGHT          5

// Bird's X in the middle horizontally
#define BIRD_X               (SSD1309_WIDTH / 2 - BIRD_WIDTH / 2)

// Bird can only be in 0 to GROUND_Y - BIRD_HEIGHT
#define PLAYABLE_HEIGHT      (GROUND_Y)

// Obstacles
#define OBSTACLE_WIDTH       5
#define GAP_HEIGHT           20
#define COLLISION_TOLERANCE  2

// Maximum number of obstacle columns
#define MAX_OBSTACLES 3

// Arrays to hold obstacles' X position and gap Y
static int obstacle_x_arr[MAX_OBSTACLES];
static int obstacle_gap_y_arr[MAX_OBSTACLES];

// Array to track if an obstacle has been scored already
static bool obstacle_passed[MAX_OBSTACLES];

// Number of obstacles active 
static int obstacle_count = 1;

// Frame counter for elapsed time
static uint32_t frame_count = 0;

static int score = 0;

static volatile bool flap_flag = false;

// Game state vars
static float bird_y;
static float bird_velocity;
static bool game_over;
static bool game_started = false; // this is for first splash screen menu 


static void init_button(void) {
    nrf_gpio_cfg_input(BUTTON_A_PIN, NRF_GPIO_PIN_PULLUP);
}


static void reset_game(void) {
    bird_y = (PLAYABLE_HEIGHT / 2.0f) - (BIRD_HEIGHT / 2.0f);
    bird_velocity = 0.0f;
    game_over = false;
    flap_flag = false;

    //button_was_pressed = false;
    score = 0;
    frame_count = 0;
    obstacle_count = 1;
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacle_x_arr[i] = SSD1309_WIDTH + i * 40; // Spaced ~40px apart ?? 
        obstacle_gap_y_arr[i] = 10 + (rand() % (PLAYABLE_HEIGHT - GAP_HEIGHT - 20));
        obstacle_passed[i] = false;
    }
}


static void draw_ground(void) {
    ssd1309_fill_rect(0, GROUND_Y, SSD1309_WIDTH, GROUND_HEIGHT, true);
}

// SUPER IMAGINATIVE BIRD DESIGN - PINNACLE OF CREATIVITY
static const struct {
    unsigned int     width;
    unsigned int     height;
    unsigned int     bytes_per_pixel;
    unsigned char    pixel_data[5 * 5 * 2];
} bird = {
    .width = 5,
    .height = 5,
    .bytes_per_pixel = 2,
    .pixel_data = {
        0xFF,0xFF, 0xFF,0xFF, 0x00,0x00, 0x00,0x00, 0xFF,0xFF,
        0xFF,0xFF, 0x00,0x00, 0xFF,0xFF, 0x00,0x00, 0xFF,0xFF,
        0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
        0xFF,0xFF, 0x00,0x00, 0xFF,0xFF, 0x00,0x00, 0xFF,0xFF,
        0xFF,0xFF, 0xFF,0xFF, 0x00,0x00, 0x00,0x00, 0xFF,0xFF,
    }
};

// scaled bitmap drawer
static void draw_bitmap(
    int x, 
    int y,
    int target_width,
    int target_height,
    const struct {
        unsigned int width;
        unsigned int height;
        unsigned int bytes_per_pixel;
        unsigned char pixel_data[];
    } *bitmap
) {
    float x_scale = (float)bitmap->width / target_width;
    float y_scale = (float)bitmap->height / target_height;
    for (int dy = 0; dy < target_height; dy++) {
        for (int dx = 0; dx < target_width; dx++) {
            int src_x = (int)(dx * x_scale);
            int src_y = (int)(dy * y_scale);
            if (src_x >= (int)bitmap->width || src_y >= (int)bitmap->height)
                continue;
            int idx = (src_y * bitmap->width + src_x) * bitmap->bytes_per_pixel;
            if (bitmap->pixel_data[idx] != 0xFF || bitmap->pixel_data[idx+1] != 0xFF) {
                ssd1309_draw_pixel(x + dx, y + dy, true);
            }
        }
    }
}

static void draw_background(void) {
    draw_bitmap(0, 0, SSD1309_WIDTH, SSD1309_HEIGHT, &background);
}

static void draw_bird(void) {
    draw_bitmap(BIRD_X, (int)bird_y, BIRD_WIDTH, BIRD_HEIGHT, &bird);
}

// columns
static void draw_obstacle(void) {
    ssd1309_fill_rect(obstacle_x_arr[0], 0, OBSTACLE_WIDTH, obstacle_gap_y_arr[0], true);
    if ((obstacle_gap_y_arr[0] + GAP_HEIGHT) < GROUND_Y) {
        ssd1309_fill_rect(obstacle_x_arr[0],
                          obstacle_gap_y_arr[0] + GAP_HEIGHT,
                          OBSTACLE_WIDTH,
                          GROUND_Y - (obstacle_gap_y_arr[0] + GAP_HEIGHT),
                          true);
    }
}

static bool check_collision(void) {
    int bx = BIRD_X;
    int by = (int)bird_y;
    int bw = BIRD_WIDTH;
    int bh = BIRD_HEIGHT;
    for (int i = 0; i < obstacle_count; i++) {
        int ox = obstacle_x_arr[i];
        int ow = OBSTACLE_WIDTH;
        if ((ox + ow) > 0 && ox < SSD1309_WIDTH) {
            if ((bx + bw) > ox && bx < (ox + ow)) {
                bool above_gap = ((by + COLLISION_TOLERANCE) < obstacle_gap_y_arr[i]);
                bool below_gap = ((by + bh - COLLISION_TOLERANCE) > (obstacle_gap_y_arr[i] + GAP_HEIGHT));
                if (above_gap || below_gap)
                    return true;
            }
        }
    }
    return false;
}


int main(void) {

    //printf("Version: Flappy Bird on micro:bit V2 + SSD1309 (B->A). Bird = 5x5.\n");

    srand(12345);
    ssd1309_init();
    ssd1309_clear();

    speaker_init();

    //capacitive_touch_init();
    //es_touch_init(); 

    //init_button_b(); // CONFLICTS WITH CHIPSELECT OF SCREEN DO NOT USE
    init_button();

    sound_detector_init();

    game_started = false;

    while (1) {
        // Splash screen until game starts
        if (!game_started) {
            ssd1309_clear_fb();
            draw_string(10, 20, "PRESS BUTTON A");
            draw_string(10, 30, "TO START");
            ssd1309_update();
            if (!nrf_gpio_pin_read(BUTTON_A_PIN)) {
                reset_game();
                game_started = true;
            }
            nrf_delay_ms(50);
            continue;
        }

        //
        // Main game loop
        //


        ssd1309_clear_fb();
        draw_background();
        frame_count++;
        uint32_t elapsed_ms = frame_count * FRAME_DELAY_MS;

        // Making the game harder over time because why not - column count increase each second approx 10 columns later
        if (elapsed_ms >= 60000) {
            obstacle_count = 3;
        } else if (elapsed_ms >= 30000) {
            obstacle_count = 2;
        } else {
            obstacle_count = 1;
        }

        // Polling Button A (debounce) 
        static bool button_was_pressed = false;
        bool pressed_now = (nrf_gpio_pin_read(BUTTON_A_PIN) == 0);
        if (pressed_now && !button_was_pressed) {

            flap_flag = true;

            speaker_play_jump_melody();  // blocking 60ms

            button_was_pressed = true;

        } 
        else if (!pressed_now) {
            button_was_pressed = false;
        }

        /*
        static bool res_touch_was_pressed = false;
        bool res_touch_pressed = res_touch_is_active();
        if (res_touch_pressed && !res_touch_was_pressed)
        {
            flap_flag = true;
            speaker_play_jump_melody();
            res_touch_was_pressed = true;
            //nrf_delay_ms(10);
        }
        else if (!res_touch_pressed)
        {
            res_touch_was_pressed = false;
        } */

        // SAME AS ABOVE for microphone
        static bool mic_detect_prev = false;
        bool mic_curr = sound_detector_check();
        if (mic_curr && !mic_detect_prev)
        {
            flap_flag = true;
            speaker_play_jump_melody(); // blocking 60ms
            mic_detect_prev = true;
            //nrf_delay_ms(10);
        }
        else if (!mic_curr)
        {
            mic_detect_prev = false;
        }

        /*
        // Polling the capacitive touch sensor (nonblocking).
        static bool touch_was_pressed = false;
        bool capacitive_pressed = capacitive_touch_is_active();
        if (capacitive_pressed && !touch_was_pressed) {
            flap_flag = true;
            speaker_play_jump_melody();
            touch_was_pressed = true;
        } else if (!capacitive_pressed) {
            touch_was_pressed = false;
        }
        */



        // flap flag jump indicator
        if (flap_flag) {
            bird_velocity = JUMP_VELOCITY;
            flap_flag = false;
        }

        // Bird physics
        bird_velocity += GRAVITY;
        bird_y += bird_velocity;
        if (bird_y < 0.0f) {
            bird_y = 0.0f;
            bird_velocity = 0.0f;
        }
        if ((bird_y + BIRD_HEIGHT) >= GROUND_Y) {
            bird_y = (float)(GROUND_Y - BIRD_HEIGHT);
            game_over = true;
        }

        // Moving obstacles anbd drawing
        for (int i = 0; i < obstacle_count; i++) {
            obstacle_x_arr[i] -= 2;
            if ((obstacle_x_arr[i] + OBSTACLE_WIDTH) < 0) {
                obstacle_x_arr[i] = SSD1309_WIDTH;
                obstacle_gap_y_arr[i] = 10 + (rand() % (PLAYABLE_HEIGHT - GAP_HEIGHT - 20));
                obstacle_passed[i] = false;
            }
            // score increment
            if (!obstacle_passed[i] && (obstacle_x_arr[i] + OBSTACLE_WIDTH) < BIRD_X) {
                score++;
                obstacle_passed[i] = true;
            }
            // top pipe
            ssd1309_fill_rect(obstacle_x_arr[i], 
                              0,
                              OBSTACLE_WIDTH,
                              obstacle_gap_y_arr[i],
                              true);
            // bot pipe
            int lower_start = obstacle_gap_y_arr[i] + GAP_HEIGHT;
            if (lower_start < GROUND_Y) {
                ssd1309_fill_rect(obstacle_x_arr[i],
                                  lower_start,
                                  OBSTACLE_WIDTH,
                                  GROUND_Y - lower_start,
                                  true);
            }
            // collision check
            int bx = BIRD_X;
            int by = (int)bird_y;
            int bw = BIRD_WIDTH;
            int bh = BIRD_HEIGHT;
            if ((bx + bw) > obstacle_x_arr[i] && bx < (obstacle_x_arr[i] + OBSTACLE_WIDTH)) {
                bool above_gap = ((by + COLLISION_TOLERANCE) < obstacle_gap_y_arr[i]);
                bool below_gap = ((by + bh - COLLISION_TOLERANCE) > (obstacle_gap_y_arr[i] + GAP_HEIGHT));
                if (above_gap || below_gap) {
                    game_over = true;
                }
            }
        }

        draw_ground();
        draw_bird();

        // score ctr top right corner finalized box design
        {
            char score_text[20];
            sprintf(score_text, "Score: %02d", score);
            int rect_x = SSD1309_WIDTH - 60;
            int rect_y = 0;
            int rect_width = 60;
            int rect_height = 12;
            for (int i = rect_x; i < rect_x + rect_width; i++) {
                ssd1309_draw_pixel(i, rect_y, true);
                ssd1309_draw_pixel(i, rect_y + rect_height - 1, true);
            }
            for (int j = rect_y; j < rect_y + rect_height; j++) {
                ssd1309_draw_pixel(rect_x, j, true);
                ssd1309_draw_pixel(rect_x + rect_width - 1, j, true);
            }
            draw_string(rect_x + 2, rect_y + 2, score_text);
        }

        if (check_collision()) {
            game_over = true;
        }

        ssd1309_update();

        if (game_over) {
            printf("GAME OVER!\n");
            
            speaker_play_game_over_melody();
            
            ssd1309_clear_fb();
            draw_string(30, 25, "GAME OVER!");
            ssd1309_update();
            nrf_delay_ms(2000);
            reset_game();
            game_started = false;
        }

        nrf_delay_ms(FRAME_DELAY_MS);
    }
    return 0;
}

