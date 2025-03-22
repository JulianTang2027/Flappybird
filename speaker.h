#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>

void speaker_init(void);
void speaker_play_tone(uint16_t frequency, uint16_t duration_ms);
void speaker_play_jump_melody(void);
void speaker_play_game_over_melody(void);

#endif 
